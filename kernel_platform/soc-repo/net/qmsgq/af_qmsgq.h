/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023,2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef __AF_QMSGQ_H_
#define __AF_QMSGQ_H_

#include <net/af_vsock.h>

struct qmsgq_endpoint;

struct qmsgq_sock {
	/* sk must be the first member. */
	struct sock sk;
	const struct qmsgq_endpoint *ep;
	struct sockaddr_vm local_addr;
	struct sockaddr_vm remote_addr;
	/* Links for the global tables of bound and connected sockets. */
	struct list_head bound_table;
	struct list_head connected_table;
	/* Accessed without the socket lock held. This means it can never be
	 * modified outsided of socket create or destruct.
	 */
	bool trusted;
	bool cached_peer_allow_dgram;	/* Dgram communication allowed to
					 * cached peer?
					 */
	u32 cached_peer;  /* Context ID of last dgram destination check. */
	const struct cred *owner;
	/* Rest are SOCK_STREAM only. */
	long connect_timeout;
	/* Listening socket that this came from. */
	struct sock *listener;
	/* Used for pending list and accept queue during connection handshake.
	 * The listening socket is the head for both lists.  Sockets created
	 * for connection requests are placed in the pending list until they
	 * are connected, at which point they are put in the accept queue list
	 * so they can be accepted in accept().  If accept() cannot accept the
	 * connection, it is marked as rejected so the cleanup function knows
	 * to clean up the socket.
	 */
	struct list_head pending_links;
	struct list_head accept_queue;
	bool rejected;
	struct delayed_work connect_work;
	struct delayed_work pending_work;
	struct delayed_work close_work;
	bool close_work_scheduled;
	u32 peer_shutdown;
	bool sent_request;
	bool ignore_connecting_rst;

	/* Protected by lock_sock(sk) */
	u64 buffer_size;
	u64 buffer_min_size;
	u64 buffer_max_size;
};

#define qsk_sk(__qsk)	(&(__qsk)->sk)
#define sk_qsk(__sk)	((struct qmsgq_sock *)__sk)

struct qmsgq_endpoint {
	struct module *module;

	/* Initialize/tear-down socket. */
	int (*init)(struct qmsgq_sock *qsk, struct qmsgq_sock *psk);
	void (*destruct)(struct qmsgq_sock *qsk);
	void (*release)(struct qmsgq_sock *qsk);

	/* DGRAM. */
	int (*dgram_enqueue)(struct qmsgq_sock *qsk, struct sockaddr_vm *addr,
			     struct msghdr *msg, size_t len);
	bool (*dgram_allow)(u32 cid, u32 port);

	/* Cancel all pending packets sent on vsock. */
	int (*cancel_pkt)(struct qmsgq_sock *qsk);

	/* Connections. */
	int (*connect)(struct qmsgq_sock *qsk);

	/* SEQ_PACKET. */
	ssize_t (*seqpacket_dequeue)(struct qmsgq_sock *qsk, struct msghdr *msg,
				     int flags);
	int (*seqpacket_enqueue)(struct qmsgq_sock *qsk, struct msghdr *msg,
				 size_t len);
	bool (*seqpacket_allow)(u32 remote_cid);
	u32 (*seqpacket_has_data)(struct qmsgq_sock *qsk);

	/* Shutdown. */
	int (*shutdown)(struct qmsgq_sock *qsk, int mode);

	/* Addressing. */
	u32 (*get_local_cid)(void);
};

struct qmsgq_cb {
	u32 src_cid;
	u32 src_port;
	u32 dst_cid;
	u32 dst_port;
};

struct sock *qmsgq_create_connected(struct sock *parent);
struct sock *qmsgq_find_bound_socket(struct sockaddr_vm *addr);
struct sock *qmsgq_find_connected_socket(struct sockaddr_vm *src,
					 struct sockaddr_vm *dst);
int qmsgq_assign_ep(struct qmsgq_sock *qsk, struct qmsgq_sock *psk);
struct qmsgq_sock *qmsgq_port_lookup(int port);
void qmsgq_insert_connected(struct qmsgq_sock *qsk);
void qmsgq_enqueue_accept(struct sock *listener, struct sock *connected);
void qmsgq_remove_bound(struct qmsgq_sock *qsk);
void qmsgq_remove_connected(struct qmsgq_sock *qsk);
void qmsgq_remove_sock(struct qmsgq_sock *qsk);
int qmsgq_endpoint_register(const struct qmsgq_endpoint *ep);
void qmsgq_endpoint_unregister(const struct qmsgq_endpoint *ep);

#endif
