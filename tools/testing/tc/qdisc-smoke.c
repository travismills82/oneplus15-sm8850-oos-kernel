/* SPDX-License-Identifier: GPL-2.0-only */
/* Disposable VM probe; always isolate the network namespace before changes. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <linux/gen_stats.h>
#include <linux/pkt_sched.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static unsigned int sequence;
static int route_fd, interface;

static void require(int ok, const char *name)
{
	if (!ok) {
		fprintf(stderr, "TC_SMOKE_FAIL %s errno=%d\n", name, errno);
		exit(1);
	}
}

static void timeout_socket(int fd)
{
	struct timeval timeout = { .tv_sec = 2 };
	require(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			  sizeof(timeout)) == 0, "bounded socket receive");
}

static void send_request(struct nlmsghdr *h)
{
	struct sockaddr_nl kernel = { .nl_family = AF_NETLINK };
	h->nlmsg_seq = ++sequence;
	require(sendto(route_fd, h, h->nlmsg_len, 0, (void *)&kernel,
		       sizeof(kernel)) == (ssize_t)h->nlmsg_len, "netlink send");
}

static int change_qdisc(const char *kind, int remove)
{
	struct {
		struct nlmsghdr header;
		struct tcmsg message;
		char attributes[128];
	} request = {
		.header = {
			.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg)),
			.nlmsg_type = remove ? RTM_DELQDISC : RTM_NEWQDISC,
			.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK |
				(remove ? 0 : NLM_F_CREATE | NLM_F_REPLACE),
		},
		.message = { .tcm_family = AF_UNSPEC, .tcm_parent = TC_H_ROOT,
			     .tcm_handle = 0x10000 },
	};
	request.message.tcm_ifindex = interface;
	if (!remove) {
		struct rtattr *attr = (void *)((char *)&request +
					      NLMSG_ALIGN(request.header.nlmsg_len));
		attr->rta_type = TCA_KIND;
		attr->rta_len = RTA_LENGTH(strlen(kind) + 1);
		memcpy(RTA_DATA(attr), kind, strlen(kind) + 1);
		request.header.nlmsg_len = NLMSG_ALIGN(request.header.nlmsg_len) +
			RTA_ALIGN(attr->rta_len);
	}
	send_request(&request.header);
	char data[8192];
	for (;;) {
		int size = recv(route_fd, data, sizeof(data), 0);
		require(size > 0, "netlink ACK receive");
		for (struct nlmsghdr *h = (void *)data;
		     size >= 0 && NLMSG_OK(h, (unsigned int)size);
		     h = NLMSG_NEXT(h, size)) {
			if (h->nlmsg_seq != sequence)
				continue;
			if (h->nlmsg_type == NLMSG_ERROR) {
				require(h->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr)),
					"complete netlink ACK");
				return ((struct nlmsgerr *)NLMSG_DATA(h))->error;
			}
		}
	}
}

static void packet_test(void)
{
	int server = socket(AF_INET, SOCK_DGRAM, 0);
	int client = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in address = {
		.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
	};
	socklen_t size = sizeof(address);
	require(server >= 0 && client >= 0, "UDP sockets");
	timeout_socket(server);
	require(bind(server, (void *)&address, sizeof(address)) == 0, "UDP bind");
	require(getsockname(server, (void *)&address, &size) == 0, "UDP destination");
	for (unsigned int i = 0; i < 16; i++) {
		uint32_t sent[16], received[16];
		for (unsigned int j = 0; j < 16; j++)
			sent[j] = i * 16 + j;
		require(sendto(client, sent, sizeof(sent), 0, (void *)&address,
			       sizeof(address)) == sizeof(sent), "UDP send");
		require(recv(server, received, sizeof(received), 0) == sizeof(received),
			"UDP receive through qdisc");
		require(!memcmp(sent, received, sizeof(sent)), "UDP content");
	}
	close(client);
	close(server);
}

static void check_stats(const char *kind)
{
	struct {
		struct nlmsghdr header;
		struct tcmsg message;
	} request = {
		.header = { .nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg)),
			    .nlmsg_type = RTM_GETQDISC,
			    .nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP },
		.message = { .tcm_family = AF_UNSPEC },
	};
	int found = 0;
	send_request(&request.header);
	char data[16384];
	for (;;) {
		int size = recv(route_fd, data, sizeof(data), 0);
		require(size > 0, "qdisc dump receive");
		for (struct nlmsghdr *h = (void *)data;
		     size >= 0 && NLMSG_OK(h, (unsigned int)size);
		     h = NLMSG_NEXT(h, size)) {
			if (h->nlmsg_seq != sequence)
				continue;
			if (h->nlmsg_type == NLMSG_DONE) {
				require(found, "root qdisc and packet statistics found");
				return;
			}
			require(h->nlmsg_type == RTM_NEWQDISC &&
				h->nlmsg_len >= NLMSG_LENGTH(sizeof(struct tcmsg)),
				"valid qdisc dump entry");
			struct tcmsg *t = NLMSG_DATA(h);
			if (t->tcm_ifindex != interface || t->tcm_parent != TC_H_ROOT)
				continue;
			int length = NLMSG_PAYLOAD(h, sizeof(*t));
			int matched = 0, stats = 0;
			for (struct rtattr *a = TCA_RTA(t); RTA_OK(a, length);
			     a = RTA_NEXT(a, length)) {
				if (a->rta_type == TCA_KIND)
					matched = RTA_PAYLOAD(a) == strlen(kind) + 1 &&
						!memcmp(RTA_DATA(a), kind, strlen(kind) + 1);
				if (a->rta_type != TCA_STATS2)
					continue;
				int nested = RTA_PAYLOAD(a);
				for (struct rtattr *s = RTA_DATA(a); RTA_OK(s, nested);
				     s = RTA_NEXT(s, nested)) {
					if (s->rta_type == TCA_STATS_BASIC && RTA_PAYLOAD(s) >= 12) {
						uint64_t bytes;
						uint32_t packets;
						memcpy(&bytes, RTA_DATA(s), sizeof(bytes));
						memcpy(&packets, (char *)RTA_DATA(s) + 8, sizeof(packets));
						stats = bytes > 0 && packets >= 16;
					}
				}
			}
			require(matched && stats, "correct qdisc kind and nonzero traffic counters");
			found = 1;
		}
	}
}

int main(void)
{
	/* Even accidental execution cannot alter the parent's network interfaces. */
	require(unshare(CLONE_NEWNET) == 0, "private network namespace");
	int control = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq lo = { .ifr_name = "lo" };
	require(control >= 0, "interface control socket");
	require(ioctl(control, SIOCGIFFLAGS, &lo) == 0, "get private loopback flags");
	lo.ifr_flags |= IFF_UP;
	require(ioctl(control, SIOCSIFFLAGS, &lo) == 0, "enable private loopback");
	close(control);
	interface = if_nametoindex("lo");
	require(interface > 0, "private loopback index");
	route_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	struct sockaddr_nl local = { .nl_family = AF_NETLINK };
	require(route_fd >= 0 && bind(route_fd, (void *)&local, sizeof(local)) == 0,
		"route netlink socket");
	timeout_socket(route_fd);
	require(change_qdisc("cake_missing", 0) == -ENOENT,
		"unavailable qdisc is not silently substituted");
	puts("QDISC_NEGATIVE_CONTROL=PASS");
	const char *kinds[] = { "fq", "fq_codel", "cake" };
	for (unsigned int i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
		for (int round = 0; round < 8; round++) {
			int result = change_qdisc(kinds[i], 0);
			if (i == 2 && round == 0 && result == -ENOENT) {
				puts("QDISC_cake=ABSENT");
				break; /* Baseline control; host must require PASS for candidate. */
			}
			require(result == 0, kinds[i]);
			packet_test();
			check_stats(kinds[i]);
			require(change_qdisc(kinds[i], 1) == 0, "qdisc deletion");
			if (round == 7)
				printf("QDISC_%s=PASS (8 create/traffic/stats/delete cycles)\n", kinds[i]);
		}
	}
	close(route_fd);
	return 0;
}
