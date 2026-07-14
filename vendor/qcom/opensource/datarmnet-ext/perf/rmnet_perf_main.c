// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/ipv6.h>
#include <net/ip.h>
#include <net/genetlink.h>

#include <uapi/linux/rmnet_perf_stats.h>

#include "rmnet_module.h"
#include "rmnet_descriptor.h"
#include "rmnet_map.h"
#include "rmnet_qmap.h"

#include "rmnet_perf.h"
#include "rmnet_perf_tcp.h"
#include "rmnet_perf_udp.h"

MODULE_LICENSE("GPL v2");

/* Insert newest first, last 4 bytes of the change id */
static char *verinfo[] = {
	"f16980f6",
	"fa851010",
	"5ec3713c",
	"e79ea282",
	"acaa7e6b",
	"173bc5b9",
	"db7d80fd",
};

#define RMNET_PERF_GENL_VERSION 1

enum {
	RMNET_PERF_MULTICAST_GROUP_0,
	RMNET_PERF_MULTICAST_GROUP_1,
	RMNET_PERF_MULTICAST_GROUP_2,
	RMNET_PERF_MULTICAST_GROUP_3,
	__RMNET_PERF_MULTICAST_GROUP_MAX,
};

#define RMNET_PERF_ATTR_MAX RMNET_PERF_ATTR_ECN_DROPS

static struct nla_policy rmnet_perf_nl_policy[RMNET_PERF_ATTR_MAX + 1] = {
	[RMNET_PERF_ATTR_STATS_REQ] = NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_perf_stats_req)),
	[RMNET_PERF_ATTR_STATS_RESP] = NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_perf_stats_resp)),
	[RMNET_PERF_ATTR_MAP_CMD_REQ] = NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_perf_map_cmd_req)),
	[RMNET_PERF_ATTR_MAP_CMD_RESP] = NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_perf_map_cmd_resp)),
	[RMNET_PERF_ATTR_MAP_CMD_IND] = NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_perf_map_cmd_ind)),
	[RMNET_PERF_ATTR_ECN_HASH] = { .type = NLA_U32, },
	[RMNET_PERF_ATTR_ECN_PROB] = { .type = NLA_U32, },
	[RMNET_PERF_ATTR_ECN_TYPE] = { .type = NLA_U8, },
	[RMNET_PERF_ATTR_ECN_DROPS] = { .type = NLA_U32, },
};

static const struct genl_multicast_group rmnet_perf_nl_mcgrps[] = {
	[RMNET_PERF_MULTICAST_GROUP_0] = { .name = RMNET_PERF_GENL_MULTICAST_NAME_0, },
	[RMNET_PERF_MULTICAST_GROUP_1] = { .name = RMNET_PERF_GENL_MULTICAST_NAME_1, },
	[RMNET_PERF_MULTICAST_GROUP_2] = { .name = RMNET_PERF_GENL_MULTICAST_NAME_2, },
	[RMNET_PERF_MULTICAST_GROUP_3] = { .name = RMNET_PERF_GENL_MULTICAST_NAME_3, },
};

int rmnet_perf_netlink_seq = 0;

module_param_array(verinfo, charp, NULL, 0444);
MODULE_PARM_DESC(verinfo, "Version of the driver");

bool enable_tcp = true;
module_param_named(rmnet_perf_knob0, enable_tcp, bool, 0644);

static bool enable_udp = true;
module_param_named(rmnet_perf_knob1, enable_udp, bool, 0644);

#define RMNET_INGRESS_QUIC_PORT 443

struct rmnet_perf_stats_store stats_store[17];

/* Holds all ECN nodes. Synchronized using internal lock */
static DEFINE_XARRAY(rmnet_perf_ecn_map);

struct xarray *rmnet_perf_get_ecn_map(void)
{
	return &rmnet_perf_ecn_map;
}

static void rmnet_perf_ecn_node_free(struct rcu_head *head)
{
	struct rmnet_perf_ecn_node *node;

	node = container_of(head, struct rmnet_perf_ecn_node, rcu);
	kfree(node);
}

static inline bool rmnet_perf_is_quic_packet(struct udphdr *uh)
{
	return be16_to_cpu(uh->source) == RMNET_INGRESS_QUIC_PORT ||
	       be16_to_cpu(uh->dest) == RMNET_INGRESS_QUIC_PORT;
}

static bool rmnet_perf_is_quic_initial_packet(struct sk_buff *skb, int ip_len)
{
	u8 *first_byte, __first_byte;
	struct udphdr *uh, __uh;

	uh = skb_header_pointer(skb, ip_len, sizeof(*uh), &__uh);

	if (!uh || !rmnet_perf_is_quic_packet(uh))
		return false;

	/* Length sanity check. Could check for the full QUIC header length if
	 * need be, but since all we really care about is the first byte, just
	 * make sure there is one.
	 */
	if (be16_to_cpu(uh->len) < sizeof(struct udphdr) + 1)
		return false;

	/* I am a very paranoid accessor of data at this point... */
	first_byte = skb_header_pointer(skb, ip_len + sizeof(struct udphdr),
					1, &__first_byte);
	if (!first_byte)
		return false;

	return ((*first_byte) & 0xC0) == 0xC0;
}

static int rmnet_perf_ingress_handle_quic(struct sk_buff *skb, int ip_len)
{
	if (rmnet_perf_is_quic_initial_packet(skb, ip_len)) {
		skb->hash = 0;
		skb->sw_hash = 1;
		return 0;
	}

	return -EINVAL;
}

int rmnet_perf_ingress_handle(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		if (!iph || ip_is_fragment(iph))
			return -EINVAL;

		if (iph->protocol == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_ingress_handle_udp(skb);

			return rmnet_perf_ingress_handle_quic(skb,
							      iph->ihl * 4);
		}

		if (iph->protocol == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_handle_tcp(skb);

			/* Don't skip SHS processing for TCP */
			return -EINVAL;
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;
		u8 proto;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return -EINVAL;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return -EINVAL;

		if (proto == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_ingress_handle_udp(skb);

			return rmnet_perf_ingress_handle_quic(skb, ip_len);
		}

		if (proto == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_handle_tcp(skb);

			return -EINVAL;
		}
	}

	return -EINVAL;
}

void rmnet_perf_ingress_rx_handler(struct sk_buff *skb)
{
	u8 proto = 0;

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		if (!iph || ip_is_fragment(iph))
			return;

		proto = iph->protocol;
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return;
	}

	if (proto == IPPROTO_TCP) {
		if (enable_tcp)
			rmnet_perf_ingress_rx_handler_tcp(skb);
	}
}

int rmnet_perf_ingress_ecn_handle(struct sk_buff *skb)
{
	u8 proto = 0;
	int ip_len;

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		if (!iph || ip_is_fragment(iph))
			goto pass;

		ip_len = iph->ihl * 4;
		proto = iph->protocol;
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		__be16 frag_off;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			goto pass;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			goto pass;
	}

	if (proto == IPPROTO_TCP) {
		if (rmnet_perf_ingress_tcp_ecn(skb, ip_len))
			return 1;
	}

	if (proto == IPPROTO_UDP) {
		if (rmnet_perf_ingress_udp_ecn(skb, ip_len))
			return 1;
	}

pass:
	return 0;
}

static void rmnet_perf_egress_handle_quic(struct sk_buff *skb, int ip_len)
{
	if (rmnet_perf_is_quic_initial_packet(skb, ip_len))
		skb->priority = 0xDA001A;
}

void rmnet_perf_egress_handle(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		/* Potentially problematic, but the problem is secondary
		 * fragments have no transport header.
		 */
		if (!iph || ip_is_fragment(iph))
			return;

		if (iph->protocol == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_egress_handle_udp(skb);

			rmnet_perf_egress_handle_quic(skb, iph->ihl * 4);
			return;
		}

		if (iph->protocol == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_egress_handle_tcp(skb);

			return;
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;
		u8 proto;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return;

		if (proto == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_egress_handle_udp(skb);

			rmnet_perf_egress_handle_quic(skb, ip_len);
			return;
		}

		if (proto == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_egress_handle_tcp(skb);

			return;
		}
	}
}

void rmnet_perf_nl_map_cmd_multicast(struct sk_buff *skb);

/* skb will be freed by rmnet_qmap_cmd_handler() */
void rmnet_perf_cmd_ingress_handler(struct sk_buff *skb)
{
	if (skb_linearize(skb)) {
		pr_err("%s(): Linearization error\n", __func__);
		return;
	}

	rmnet_perf_nl_map_cmd_multicast(skb);
}

void rmnet_perf_coal_common_stat(uint8_t mux_id, uint32_t type)
{
	if (!mux_id || mux_id > 16)
		goto err0;

	switch (type) {
	case 0:
		stats_store[mux_id].dl_stats.coal_common_stats.csum_error++;
		break;
	case 1:
		stats_store[mux_id].dl_stats.coal_common_stats.pkt_recons++;
		break;
	case 2:
		stats_store[mux_id].dl_stats.coal_common_stats.close_non_coal++;
		break;
	case 3:
		stats_store[mux_id].dl_stats.coal_common_stats.l3_mismatch++;
		break;
	case 4:
		stats_store[mux_id].dl_stats.coal_common_stats.l4_mismatch++;
		break;
	case 5:
		stats_store[mux_id].dl_stats.coal_common_stats.nlo_limit++;
		break;
	case 6:
		stats_store[mux_id].dl_stats.coal_common_stats.pkt_limit++;
		break;
	case 7:
		stats_store[mux_id].dl_stats.coal_common_stats.byte_limit++;
		break;
	case 8:
		stats_store[mux_id].dl_stats.coal_common_stats.time_limit++;
		break;
	case 9:
		stats_store[mux_id].dl_stats.coal_common_stats.eviction++;
		break;
	case 10:
		stats_store[mux_id].dl_stats.coal_common_stats.close_coal++;
		break;
	default:
		break;
	}

err0:
	return;
}

void rmnet_perf_coal_stat(uint8_t mux_id, uint8_t veid, uint64_t len, uint32_t type)
{
	if (!mux_id || mux_id > 16)
		goto err0;

	if (veid >= 16)
		goto err0;

	switch (type) {
	case 0:
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].tcpv4_pkts++;
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].tcpv4_bytes += len;
		break;
	case 1:
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].udpv4_pkts++;
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].udpv4_bytes += len;
		break;
	case 2:
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].tcpv6_pkts++;
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].tcpv6_bytes += len;
		break;
	case 3:
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].udpv6_pkts++;
		stats_store[mux_id].dl_stats.coal_veid_stats[veid].udpv6_bytes += len;
		break;
	}

err0:
	return;
}

void rmnet_perf_seg_stat(uint8_t mux_id, struct sk_buff *skb)
{
	if (!mux_id || mux_id > 16)
		goto err0;

	if (skb->protocol == htons(ETH_P_IP)) {
		if (ip_hdr(skb)->protocol == IPPROTO_TCP) {
			stats_store[mux_id].ul_stats.seg_proto_stats.tcpv4_pkts++;
			stats_store[mux_id].ul_stats.seg_proto_stats.tcpv4_bytes += skb->len;
		} else if (ip_hdr(skb)->protocol == IPPROTO_UDP) {
			stats_store[mux_id].ul_stats.seg_proto_stats.udpv4_pkts++;
			stats_store[mux_id].ul_stats.seg_proto_stats.udpv4_bytes += skb->len;
		}
	}

	if (skb->protocol == htons(ETH_P_IPV6)) {
		if (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP) {
			stats_store[mux_id].ul_stats.seg_proto_stats.tcpv6_pkts++;
			stats_store[mux_id].ul_stats.seg_proto_stats.tcpv6_bytes += skb->len;
		} else if (ipv6_hdr(skb)->nexthdr == IPPROTO_UDP) {
			stats_store[mux_id].ul_stats.seg_proto_stats.udpv6_pkts++;
			stats_store[mux_id].ul_stats.seg_proto_stats.udpv6_bytes += skb->len;
		}
	}

err0:
	return;
}

void rmnet_perf_non_coal_stat(uint8_t mux_id, uint64_t len)
{
	if (!mux_id || mux_id > 16)
		goto err0;

	stats_store[mux_id].dl_stats.non_coal_pkts++;
	stats_store[mux_id].dl_stats.non_coal_bytes += len;

err0:
	return;
}

static const struct rmnet_module_hook_register_info
rmnet_perf_module_hooks[] = {
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_INGRESS,
		.func = rmnet_perf_ingress_handle,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_EGRESS,
		.func = rmnet_perf_egress_handle,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_SET_THRESH,
		.func = rmnet_perf_tcp_update_quickack_thresh,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_INGRESS_RX_HANDLER,
		.func = rmnet_perf_ingress_rx_handler,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_CMD_INGRESS,
		.func = rmnet_perf_cmd_ingress_handler,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_COAL_COMMON_STAT,
		.func = rmnet_perf_coal_common_stat,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_COAL_STAT,
		.func = rmnet_perf_coal_stat,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_SEG_STAT,
		.func = rmnet_perf_seg_stat,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_NON_COAL_STAT,
		.func = rmnet_perf_non_coal_stat,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_ECN_INGRESS,
		.func = rmnet_perf_ingress_ecn_handle,
	},
};

void rmnet_perf_set_hooks(void)
{
	rmnet_module_hook_register(rmnet_perf_module_hooks,
				   ARRAY_SIZE(rmnet_perf_module_hooks));
}

void rmnet_perf_unset_hooks(void)
{
	rmnet_module_hook_unregister(rmnet_perf_module_hooks,
				     ARRAY_SIZE(rmnet_perf_module_hooks));
}

static struct genl_family rmnet_perf_nl_family;

int rmnet_perf_nl_cmd_get_stats(struct sk_buff *skb, struct genl_info *info)
{
	struct rmnet_perf_stats_resp *resp = NULL;
	struct rmnet_perf_stats_req req;
	int bytes = -1, ret = -ENOMEM;
	struct sk_buff *rskb = NULL;
	struct nlattr *na = NULL;
	void *hdrp = NULL;

	rskb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (!rskb) {
		pr_err("%s(): Failed to allocate response skb\n", __func__);
		goto err0;
	}

	hdrp = genlmsg_put(rskb, 0, rmnet_perf_netlink_seq++,
			   &rmnet_perf_nl_family, 0,
			   RMNET_PERF_CMD_GET_STATS);
	if (!hdrp) {
		pr_err("%s(): Failed to set header pointer\n", __func__);
		goto err1;
	}

	resp = kzalloc(sizeof(struct rmnet_perf_stats_resp), GFP_ATOMIC);
	if (!resp) {
		pr_err("%s(): Failed to allocate response cmd\n", __func__);
		goto err1;
	}

	memset(&req, 0, sizeof(struct rmnet_perf_stats_req));
	ret = -EINVAL;
	na = info->attrs[RMNET_PERF_ATTR_STATS_REQ];
	if (!na) {
		pr_err("%s(): Failed to get cmd request attribute\n", __func__);
		goto err2;
	}

	bytes = nla_memcpy(&req, na, sizeof(struct rmnet_perf_stats_req));
	if (bytes <= 0) {
		pr_err("%s(): Failed to copy cmd request attribute\n", __func__);
		goto err2;
	}

	if (req.mux_id > 16) {
		pr_err("%s(): Unsupported mux id %u\n", __func__, req.mux_id);
		goto err2;
	}

	ret = 0;
	memcpy(&resp->stats, &stats_store[req.mux_id],
	       sizeof(struct rmnet_perf_stats_store));

err2:
	resp->error_code = abs(ret);
	if (!nla_put(rskb, RMNET_PERF_ATTR_STATS_RESP,
		     sizeof(struct rmnet_perf_stats_resp), resp)) {
		kfree(resp);
		genlmsg_end(rskb, hdrp);
		return genlmsg_reply(rskb, info);
	} else {
		pr_err("%s(): Failed to copy cmd response attribute\n", __func__);
	}
	kfree(resp);
err1:
	nlmsg_free(rskb);
err0:
	return ret;
}

void rmnet_perf_nl_map_cmd_multicast(struct sk_buff *skb)
{
	uint8_t offset = sizeof(struct qmap_cmd_hdr);
	struct rmnet_perf_map_cmd_ind *ind = NULL;
	struct qmap_cmd_hdr *cmd_hdr = NULL;
	struct sk_buff *iskb = NULL;
	void *hdrp = NULL;
	int rc = -EINVAL;

	iskb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (!iskb) {
		pr_err("%s(): Failed to indication skb\n", __func__);
		goto err0;
	}

	hdrp = genlmsg_put(iskb, 0, rmnet_perf_netlink_seq++,
			   &rmnet_perf_nl_family, 0,
			   RMNET_PERF_CMD_MAP_CMD);
	if (!hdrp) {
		pr_err("%s(): Failed to set header pointer\n", __func__);
		goto err1;
	}

	ind = kzalloc(sizeof(struct rmnet_perf_map_cmd_ind), GFP_ATOMIC);
	if (!ind) {
		pr_err("%s(): Failed to allocate indication cmd\n", __func__);
		goto err1;
	}

	if (skb->len <= offset) {
		pr_err("%s(): Incoming cmd size is invalid\n", __func__);
		goto err2;
	}

	cmd_hdr = (struct qmap_cmd_hdr *)skb->data;
	ind->cmd_len = skb->len - offset;
	ind->cmd_name = cmd_hdr->cmd_name;
	ind->ack = cmd_hdr->cmd_type;
	memcpy(ind->cmd_content, skb->data + offset, ind->cmd_len);

	if (nla_put(iskb, RMNET_PERF_ATTR_MAP_CMD_IND,
		    sizeof(struct rmnet_perf_map_cmd_ind), ind)) {
		pr_err("%s(): Failed to copy cmd indication attribute\n", __func__);
		goto err2;
	}

	genlmsg_end(iskb, hdrp);
	kfree(ind);
	/* -EINVAL is the only error for which the skb is not freed */
	rc = genlmsg_multicast(&rmnet_perf_nl_family, iskb, 0,
			       RMNET_PERF_MULTICAST_GROUP_0, GFP_ATOMIC);
	if (rc == -EINVAL) {
		pr_err("%s(): Invalid group for multicast\n", __func__);
		goto err1;
	}
	return;

err2:
	kfree(ind);
err1:
	nlmsg_free(iskb);
err0:
	return;
}

int rmnet_perf_cmd_xmit(struct rmnet_perf_map_cmd_req *cmd)
{
	struct net_device *dev = dev_get_by_name(&init_net, "rmnet_ipa0");
	int cmd_len = sizeof(struct qmap_cmd_hdr) + cmd->cmd_len;
	struct qmap_cmd_hdr *cmd_hdr = NULL;
	struct sk_buff *skb = NULL;
	char *cmd_content = NULL;
	int ret = -ENODEV;

	if (!dev) {
		pr_err("%s(): Unable to get reference to device\n", __func__);
		goto err0;
	}

	skb = alloc_skb(cmd_len, GFP_ATOMIC);
	if (!skb) {
		pr_err("%s(): Unable to allocate memory for cmd\n", __func__);
		ret = -ENOMEM;
		goto err1;
	}

	skb_put(skb, cmd_len);
	memset(skb->data, 0, cmd_len);

	cmd_hdr = (struct qmap_cmd_hdr *)skb->data;
	cmd_hdr->cd_bit = 1;
	cmd_hdr->mux_id = 0;
	cmd_hdr->pkt_len = htons(sizeof(struct rmnet_map_control_command_header) +
					cmd->cmd_len);
	cmd_hdr->cmd_name = cmd->cmd_name;
	cmd_hdr->cmd_type = cmd->ack;

	cmd_content = (char *)(skb->data + sizeof(struct qmap_cmd_hdr));
	memcpy(cmd_content, cmd->cmd_content, cmd->cmd_len);

	skb->dev = dev;
	skb->protocol = htons(ETH_P_MAP);

	ret = rmnet_qmap_send(skb, RMNET_CH_CTL, false);

err1:
	dev_put(dev);
err0:
	return ret;
}

int rmnet_perf_nl_cmd_map_cmd_req(struct sk_buff *skb, struct genl_info *info)
{
	struct rmnet_perf_map_cmd_req *req = NULL;
	struct rmnet_perf_map_cmd_resp resp;
	int bytes = -1, ret = -ENOMEM;
	struct sk_buff *rskb = NULL;
	struct nlattr *na = NULL;
	void *hdrp = NULL;

	rskb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (!rskb) {
		pr_err("%s(): Failed to allocate response skb\n", __func__);
		goto err0;
	}

	hdrp = genlmsg_put(rskb, 0, rmnet_perf_netlink_seq++,
			   &rmnet_perf_nl_family, 0,
			   RMNET_PERF_CMD_MAP_CMD);
	if (!hdrp) {
		pr_err("%s(): Failed to set header pointer\n", __func__);
		goto err1;
	}

	memset(&resp, 0, sizeof(struct rmnet_perf_map_cmd_resp));
	req = kzalloc(sizeof(struct rmnet_perf_map_cmd_req), GFP_ATOMIC);
	if (!req) {
		pr_err("%s(): Failed to allocate request cmd\n", __func__);
		goto err2;
	}

	ret = -EINVAL;
	na = info->attrs[RMNET_PERF_ATTR_MAP_CMD_REQ];
	if (!na) {
		pr_err("%s(): Failed to get cmd request attribute\n", __func__);
		goto err3;
	}

	bytes = nla_memcpy(req, na, sizeof(struct rmnet_perf_map_cmd_req));
	if (bytes <= 0) {
		pr_err("%s(): Failed to copy cmd request attribute\n", __func__);
		goto err3;
	}

	switch (req->cmd_name) {
	case QMAP_CMD_31:
	case QMAP_CMD_32:
	case QMAP_CMD_40:
	case QMAP_CMD_42:
	case QMAP_CMD_44:
		break;
	default:
		pr_err("%s(): Unsupported command %u\n", __func__, req->cmd_name);
		goto err3;
	}

	if (req->cmd_len > 16000) {
		pr_err("%s(): Unsupported length %u\n", __func__, req->cmd_len);
		goto err3;
	}

	resp.cmd_name = req->cmd_name;
	ret = rmnet_perf_cmd_xmit(req);

err3:
	kfree(req);
err2:
	resp.error_code = abs(ret);
	if (!nla_put(rskb, RMNET_PERF_ATTR_MAP_CMD_RESP,
		     sizeof(struct rmnet_perf_map_cmd_resp), &resp)) {
		genlmsg_end(rskb, hdrp);
		return genlmsg_reply(rskb, info);
	} else {
		pr_err("%s(): Failed to copy cmd response attribute\n", __func__);
	}
err1:
	nlmsg_free(rskb);
err0:
	return ret;
}

static int rmnet_perf_tcp_update_ecn_prob(u32 hash_key, u32 prob, bool should_drop)
{
	struct rmnet_perf_ecn_node *node;
	int err;

	xa_lock(rmnet_perf_get_ecn_map());
	node = __xa_store(rmnet_perf_get_ecn_map(), hash_key, NULL, GFP_ATOMIC);
	if (xa_is_err(node)) {
		xa_unlock(rmnet_perf_get_ecn_map());
		return xa_err(node);
	}

	if (!node) {
		/* Adding the node for the first time */
		node = kzalloc(sizeof(*node), GFP_ATOMIC);
		if (!node) {
			xa_unlock(rmnet_perf_get_ecn_map());
			return -ENOMEM;
		}
	}

	if (!prob) {
		/* 0 drop probability means we no longer need to store
		 * this node and track it.
		 */
		call_rcu(&node->rcu, rmnet_perf_ecn_node_free);
		xa_unlock(rmnet_perf_get_ecn_map());
		return 0;
	}

	node->prob = prob;
	node->should_drop = should_drop;
	err = xa_err(__xa_store(rmnet_perf_get_ecn_map(), hash_key, node,
				GFP_ATOMIC));
	xa_unlock(rmnet_perf_get_ecn_map());
	if (err) {
		kfree(node);
		return err;
	}

	return 0;
}

static int rmnet_perf_nl_cmd_ecn_update(struct sk_buff *skb,
					struct genl_info *info)
{
	u32 hash_key;
	u32 prob;
	u8 type;
	int rc;

	if (!info->attrs[RMNET_PERF_ATTR_ECN_HASH] ||
	    !info->attrs[RMNET_PERF_ATTR_ECN_PROB] ||
	    !info->attrs[RMNET_PERF_ATTR_ECN_TYPE]) {
		GENL_SET_ERR_MSG(info,
				 "Must provide ECN hash, probability, & type");
		return -EINVAL;
	}

	hash_key = nla_get_u32(info->attrs[RMNET_PERF_ATTR_ECN_HASH]);
	prob = nla_get_u32(info->attrs[RMNET_PERF_ATTR_ECN_PROB]);
	type = nla_get_u8(info->attrs[RMNET_PERF_ATTR_ECN_TYPE]);
	rc = rmnet_perf_tcp_update_ecn_prob(hash_key, prob,
					    type == RMNET_PERF_ECN_TYPE_DROP);
	if (rc) {
		GENL_SET_ERR_MSG(info, "Updating probability failed");
		return rc;
	}

	return 0;
}

static int rmnet_perf_get_ecn_drops(u32 hash_key, u32 *drops)
{
	struct rmnet_perf_ecn_node *node;

	if (!drops)
		return -EINVAL;

	rcu_read_lock();
	node = xa_load(rmnet_perf_get_ecn_map(), hash_key);
	if (node)
		*drops = node->drops;

	rcu_read_unlock();
	if (!node)
		return -ESRCH;

	return 0;
}

static int rmnet_perf_nl_cmd_ecn_drop_stat(struct sk_buff *skb,
					   struct genl_info *info)
{
	struct sk_buff *reply;
	int msg_size = nla_total_size(sizeof(u32)) * 2;
	u32 hash_key;
	u32 drops = 0;
	int rc;
	void *hdr;

	if (!info->attrs[RMNET_PERF_ATTR_ECN_HASH]) {
		GENL_SET_ERR_MSG(info, "Must provide hash value");
		return -EINVAL;
	}

	hash_key = nla_get_u32(info->attrs[RMNET_PERF_ATTR_ECN_HASH]);
	rc = rmnet_perf_get_ecn_drops(hash_key, &drops);
	if (rc) {
		GENL_SET_ERR_MSG(info, "Fetching drop count failed");
		return rc;
	}

	reply = genlmsg_new(msg_size, GFP_KERNEL);
	if (!reply) {
		GENL_SET_ERR_MSG(info, "Allocating response failed");
		return -ENOMEM;
	}

	hdr = genlmsg_put_reply(reply, info, &rmnet_perf_nl_family, 0,
				RMNET_PERF_CMD_ECN_DROP_STATS);
	if (!hdr) {
		GENL_SET_ERR_MSG(info, "Building response failed");
		kfree_skb(reply);
		return -EINVAL;
	}

	nla_put_u32(reply, RMNET_PERF_ATTR_ECN_DROPS, drops);
	genlmsg_end(reply, hdr);
	genlmsg_reply(reply, info);
	return 0;
}

static int rmnet_perf_nl_cmd_ecn_flush(struct sk_buff *skb,
				       struct genl_info *info)
{
	struct rmnet_perf_ecn_node *node;
	unsigned long idx;

	xa_lock(rmnet_perf_get_ecn_map());
	xa_for_each(rmnet_perf_get_ecn_map(), idx, node) {
		__xa_erase(rmnet_perf_get_ecn_map(), idx);
		call_rcu(&node->rcu, rmnet_perf_ecn_node_free);
	}

	xa_unlock(rmnet_perf_get_ecn_map());
	return 0;
}

static const struct genl_ops rmnet_perf_nl_ops[] = {
	{
		.cmd = RMNET_PERF_CMD_GET_STATS,
		.doit = rmnet_perf_nl_cmd_get_stats,
	},
	{
		.cmd = RMNET_PERF_CMD_MAP_CMD,
		.doit = rmnet_perf_nl_cmd_map_cmd_req,
	},
	{
		.cmd = RMNET_PERF_CMD_ECN_UPDATE,
		.doit = rmnet_perf_nl_cmd_ecn_update,
	},
	{
		.cmd = RMNET_PERF_CMD_ECN_DROP_STATS,
		.doit = rmnet_perf_nl_cmd_ecn_drop_stat,
	},
	{
		.cmd = RMNET_PERF_CMD_ECN_FLUSH,
		.doit = rmnet_perf_nl_cmd_ecn_flush,
	},
};

static struct genl_family rmnet_perf_nl_family __ro_after_init = {
	.hdrsize = 0,
	.name = RMNET_PERF_GENL_FAMILY_NAME,
	.version = RMNET_PERF_GENL_VERSION,
	.maxattr = RMNET_PERF_ATTR_MAX,
	.policy = rmnet_perf_nl_policy,
	.ops = rmnet_perf_nl_ops,
	.n_ops = ARRAY_SIZE(rmnet_perf_nl_ops),
	.mcgrps = rmnet_perf_nl_mcgrps,
	.n_mcgrps = ARRAY_SIZE(rmnet_perf_nl_mcgrps),
};

int rmnet_perf_nl_register(void)
{
	return genl_register_family(&rmnet_perf_nl_family);
}

void rmnet_perf_nl_unregister(void)
{
	genl_unregister_family(&rmnet_perf_nl_family);
}

static int __init rmnet_perf_init(void)
{
	int rc;

	pr_info("%s(): Loading\n", __func__);
	rc = rmnet_perf_tcp_init();
	if (rc)
		goto err0;

	rc = rmnet_perf_udp_init();
	if (rc)
		goto err1;

	rc = rmnet_perf_nl_register();
	if (rc) {
		pr_err("%s(): Failed to register generic netlink family\n", __func__);
		goto err2;
	}

	rmnet_perf_set_hooks();

err2:
	rmnet_perf_udp_exit();
err1:
	rmnet_perf_tcp_exit();
err0:
	return rc;
}

static void __exit rmnet_perf_exit(void)
{
	struct rmnet_perf_ecn_node *node;
	unsigned long idx;

	rmnet_perf_unset_hooks();
	rmnet_perf_nl_unregister();
	rmnet_perf_udp_exit();
	rmnet_perf_tcp_exit();

	xa_lock(rmnet_perf_get_ecn_map());
	xa_for_each(rmnet_perf_get_ecn_map(), idx, node)
		call_rcu(&node->rcu, rmnet_perf_ecn_node_free);

	xa_unlock(rmnet_perf_get_ecn_map());
	xa_destroy(rmnet_perf_get_ecn_map());

	pr_info("%s(): exiting\n", __func__);
}

module_init(rmnet_perf_init);
module_exit(rmnet_perf_exit);
