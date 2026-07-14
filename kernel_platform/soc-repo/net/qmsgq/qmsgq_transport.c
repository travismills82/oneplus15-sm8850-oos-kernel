// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/gunyah/gh_rm_drv.h>
#include <linux/gunyah/gh_msgq.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/skbuff.h>
#include <linux/sizes.h>
#include <linux/types.h>

#include "af_qmsgq.h"
#include "qmsgq_gunyah.h"

#define QMSGQ_GH_PROTO_VER_1 1

#define QMSGQ_CLOSE_TIMEOUT (8 * HZ)

enum qmsgq_transport_pkt_type {
	PKT_TYPE_DGRAM = 1,
	PKT_TYPE_SEQPACKET_DATA,
	PKT_TYPE_SEQPACKET_OP_CONN_REQ,
	PKT_TYPE_SEQPACKET_OP_CONN_RSP,
	PKT_TYPE_SEQPACKET_OP_SHUTDOWN,
	PKT_TYPE_SEQPACKET_OP_CLOSE,
	PKT_TYPE_INVALID
};

/**
 * struct qmsgq_hdr - qmsgq packet header
 * @version: protocol version
 * @type: packet type; one of qmsgq_gh_pkt_type
 * @flags: Reserved for future use
 * @optlen: length of optional header data
 * @size: length of packet, excluding this header and optlen
 * @src_node_id: source cid
 * @src_port_id: source port
 * @dst_node_id: destination cid
 * @dst_port_id: destination port
 */
struct qmsgq_hdr {
	u8 version;
	u8 type;
	u8 flags;
	u8 optlen;
	__le32 size;
	__le32 src_cid;
	__le32 src_port_id;
	__le32 dst_cid;
	__le32 dst_port_id;
};

struct qmsgq_seqpacket_op_pkt {
	struct qmsgq_hdr hdr;
	__le16 status;
	__le16 rsvd;
} __packed;

static void reset_buf(struct qmsgq_gh_recv_buf *rx_buf)
{
	memset(rx_buf->buf, 0, MAX_PKT_SZ);
	rx_buf->hdr_received = false;
	rx_buf->copied = 0;
	rx_buf->len = 0;
}

static u32 qmsgq_transport_seqpacket_has_data(struct qmsgq_sock *qsk)
{
	return 0;
}

static int qmsgq_transport_cancel_pkt(struct qmsgq_sock *qsk)
{
	return 0;
}

static void qmsgq_transport_remove_sock(struct qmsgq_sock *qsk)
{
	struct sock *sk = qsk_sk(qsk);

	skb_queue_purge(&sk->sk_receive_queue);
	qmsgq_remove_sock(qsk);
}

static int qmsgq_transport_send_pkt(struct qmsgq_gh_device *qdev, char *buf, unsigned int len)
{
	return qdev->send_pkt(qdev, buf, len);
}

static int qmsgq_transport_dgram_post(const struct qmsgq_endpoint *ep, struct sockaddr_vm *src,
				      struct sockaddr_vm *dst, void *data, int len)
{
	struct qmsgq_sock *qsk;
	struct qmsgq_cb *cb;
	struct sk_buff *skb;
	int rc;

	skb = alloc_skb_with_frags(0, len, 0, &rc, GFP_KERNEL);
	if (!skb) {
		pr_err("%s: Unable to get skb with len:%d\n", __func__, len);
		return -ENOMEM;
	}
	cb = (struct qmsgq_cb *)skb->cb;
	cb->src_cid = src->svm_cid;
	cb->src_port = src->svm_port;
	cb->dst_cid = dst->svm_cid;
	cb->dst_port = dst->svm_port;

	skb->data_len = len;
	skb->len = len;
	skb_store_bits(skb, 0, data, len);

	qsk = qmsgq_port_lookup(dst->svm_port);
	if (!qsk || qsk->ep != ep) {
		pr_err("%s: invalid dst port:%d\n", __func__, dst->svm_port);
		kfree_skb(skb);
		return -EINVAL;
	}

	if (sock_queue_rcv_skb(qsk_sk(qsk), skb)) {
		pr_err("%s: sock_queue_rcv_skb failed\n", __func__);
		sock_put(qsk_sk(qsk));
		kfree_skb(skb);
		return -EINVAL;
	}
	sock_put(qsk_sk(qsk));
	return 0;
}

static int qmsgq_transport_dgram_enqueue(struct qmsgq_sock *qsk,
					 struct sockaddr_vm *remote,
					 struct msghdr *msg, size_t len)
{
	struct sockaddr_vm *local_addr = &qsk->local_addr;
	const struct qmsgq_endpoint *ep;
	struct qmsgq_gh_device *qdev;
	struct qmsgq_hdr *hdr;
	char *buf;
	int rc;

	ep = qsk->ep;
	if (!ep)
		return -ENXIO;
	qdev = container_of(ep, struct qmsgq_gh_device, ep);

	if (!qdev->msgq_hdl) {
		pr_err("%s: Transport not ready\n", __func__);
		return -ENODEV;
	}

	if (len > MAX_PKT_SZ - sizeof(*hdr)) {
		pr_err("%s: Invalid pk size: len: %lu\n", __func__, len);
		return -EMSGSIZE;
	}

	/* Allocate a buffer for the user's message and our packet header. */
	buf = kmalloc(len + sizeof(*hdr), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Populate Header */
	hdr = (struct qmsgq_hdr *)buf;
	hdr->version = QMSGQ_GH_PROTO_VER_1;
	hdr->type = PKT_TYPE_DGRAM;
	hdr->flags = 0;
	hdr->optlen = 0;
	hdr->size = len;
	hdr->src_cid = 0;
	hdr->src_port_id = local_addr->svm_port;
	hdr->dst_cid = 0;
	hdr->dst_port_id = remote->svm_port;
	rc = memcpy_from_msg((void *)buf + sizeof(*hdr), msg, len);
	if (rc) {
		pr_err("%s: failed: memcpy_from_msg rc: %d\n", __func__, rc);
		goto send_err;
	}

	pr_debug("TX DATA: Len:0x%zx src[0x%x] dst[0x%x]\n", len, hdr->src_port_id,
		 hdr->dst_port_id);

	rc = qmsgq_transport_send_pkt(qdev, buf, len + sizeof(*hdr));
	if (rc < 0) {
		pr_err("%s: failed to send msg rc: %d\n", __func__, rc);
		goto send_err;
	}

	kfree(buf);
	return 0;

send_err:
	kfree(buf);
	return rc;
}

static int qmsgq_transport_seqpacket_post(struct qmsgq_sock *qsk, struct sockaddr_vm *src,
					  struct sockaddr_vm *dst, void *data, int len)
{
	struct sock *sk = qsk_sk(qsk);
	struct qmsgq_cb *cb;
	struct sk_buff *skb;
	int rc;

	skb = alloc_skb_with_frags(0, len, 0, &rc, GFP_KERNEL);
	if (!skb) {
		pr_err("%s: Unable to get skb with len:%d\n", __func__, len);
		return -ENOMEM;
	}
	cb = (struct qmsgq_cb *)skb->cb;
	cb->src_cid = src->svm_cid;
	cb->src_port = src->svm_port;
	cb->dst_cid = dst->svm_cid;
	cb->dst_port = dst->svm_port;

	skb->data_len = len;
	skb->len = len;
	skb_store_bits(skb, 0, data, len);

	sock_hold(sk);
	if (sock_queue_rcv_skb(sk, skb)) {
		pr_err("%s: sock_queue_rcv_skb failed\n", __func__);
		sock_put(sk);
		kfree_skb(skb);
		return -EINVAL;
	}
	sock_put(sk);

	return 0;
}

static char *qmsgq_transport_seqpacket_build_op_pkt(u8 type, u16 status,
						    struct sockaddr_vm *src,
						    struct sockaddr_vm *dst)
{
	struct qmsgq_seqpacket_op_pkt *op_pkt;
	char *buf;

	buf = kmalloc(sizeof(*op_pkt), GFP_KERNEL);
	if (!buf)
		return NULL;

	op_pkt = (struct qmsgq_seqpacket_op_pkt *)buf;
	op_pkt->hdr.version = QMSGQ_GH_PROTO_VER_1;
	op_pkt->hdr.type = type;
	op_pkt->hdr.flags = 0;
	op_pkt->hdr.optlen = 0;
	op_pkt->hdr.size = sizeof(*op_pkt) - sizeof(struct qmsgq_hdr);
	op_pkt->hdr.src_cid = src->svm_cid;
	op_pkt->hdr.src_port_id = src->svm_port;
	op_pkt->hdr.dst_cid = dst->svm_cid;
	op_pkt->hdr.dst_port_id = dst->svm_port;

	op_pkt->status = status;

	return buf;
}

static int qmsgq_transport_seqpacket_send_op_pkt(struct qmsgq_sock *qsk, void *buf)
{
	const struct qmsgq_endpoint *ep = qsk->ep;
	struct qmsgq_gh_device *qdev;
	int rc;

	if (!ep)
		return -ENXIO;

	qdev = container_of(ep, struct qmsgq_gh_device, ep);
	if (!qdev)
		return -ENODEV;

	rc = qmsgq_transport_send_pkt(qdev, buf, sizeof(struct qmsgq_seqpacket_op_pkt));
	if (rc < 0) {
		pr_err("%s: failed to send msg rc: %d\n", __func__, rc);
		return -EINVAL;
	}

	return 0;
}

static void qmsgq_transport_seqpacket_no_sock(struct qmsgq_gh_device *qdev,
					      char *rx_buf, u16 err)
{
	struct qmsgq_hdr *rx_hdr = (struct qmsgq_hdr *)rx_buf;
	struct sockaddr_vm src;
	struct sockaddr_vm dst;
	char *buf;
	int rc;

	vsock_addr_init(&src, le32_to_cpu(rx_hdr->src_cid), le32_to_cpu(rx_hdr->src_port_id));
	vsock_addr_init(&dst, le32_to_cpu(rx_hdr->dst_cid), le32_to_cpu(rx_hdr->dst_port_id));

	switch (rx_hdr->type) {
	case PKT_TYPE_SEQPACKET_OP_CONN_REQ:
		buf = qmsgq_transport_seqpacket_build_op_pkt(PKT_TYPE_SEQPACKET_OP_CONN_RSP,
							     err, &dst, &src);
		if (!buf) {
			pr_err("%s: failed to build msg\n", __func__);
			break;
		}

		rc = qmsgq_transport_send_pkt(qdev, buf, sizeof(struct qmsgq_seqpacket_op_pkt));
		if (rc < 0)
			pr_err("%s: failed to send msg rc: %d\n", __func__, rc);

		kfree(buf);
		break;
	default:
		break;
	}
}

static int qmsgq_transport_send_shutdown_op_pkt(struct qmsgq_sock *qsk, int mode)
{
	char *buf;
	int rc;

	buf = qmsgq_transport_seqpacket_build_op_pkt(PKT_TYPE_SEQPACKET_OP_SHUTDOWN, 0,
						     &qsk->local_addr, &qsk->remote_addr);
	if (!buf)
		return -ENOMEM;

	rc = qmsgq_transport_seqpacket_send_op_pkt(qsk, buf);
	if (rc < 0)
		pr_err("%s: failed to send msg rc: %d\n", __func__, rc);

	kfree(buf);

	return rc;
}

static int qmsgq_transport_send_close_op_pkt(struct qmsgq_sock *qsk)
{
	char *buf;
	int rc;

	buf = qmsgq_transport_seqpacket_build_op_pkt(PKT_TYPE_SEQPACKET_OP_CLOSE, 0,
						     &qsk->local_addr, &qsk->remote_addr);
	if (!buf)
		return -ENOMEM;

	rc = qmsgq_transport_seqpacket_send_op_pkt(qsk, buf);
	if (rc < 0)
		pr_err("%s: failed to send msg rc: %d\n", __func__, rc);

	kfree(buf);

	return rc;
}

static void qmsgq_transport_do_close(struct qmsgq_sock *qsk, bool cancel_timeout)
{
	struct sock *sk = qsk_sk(qsk);

	sock_set_flag(sk, SOCK_DONE);

	qsk->peer_shutdown = SHUTDOWN_MASK;
	if (qmsgq_transport_seqpacket_has_data(qsk) <= 0)
		sk->sk_state = TCP_CLOSING;
	sk->sk_state_change(sk);

	if (qsk->close_work_scheduled &&
	    (!cancel_timeout || cancel_delayed_work(&qsk->close_work))) {
		qsk->close_work_scheduled = false;

		qmsgq_transport_remove_sock(qsk);

		/* Release refcnt obtained when we scheduled the timeout */
		sock_put(sk);
	}
}

static void qmsgq_transport_close_timeout(struct work_struct *work)
{
	struct qmsgq_sock *qsk =
		container_of(work, struct qmsgq_sock, close_work.work);
	struct sock *sk = qsk_sk(qsk);

	sock_hold(sk);
	lock_sock(sk);

	if (!sock_flag(sk, SOCK_DONE)) {
		qmsgq_transport_send_close_op_pkt(qsk);
		qmsgq_transport_do_close(qsk, false);
	}

	qsk->close_work_scheduled = false;

	release_sock(sk);
	sock_put(sk);
}

/* qsk->sk needs to be locked by caller */
static bool qmsgq_transport_close(struct qmsgq_sock *qsk)
{
	struct sock *sk = qsk_sk(qsk);

	if (!(sk->sk_state == TCP_ESTABLISHED ||
	      sk->sk_state == TCP_CLOSING))
		return true;

	/* Already received SHUTDOWN from peer, reply with RST */
	if ((qsk->peer_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK) {
		qmsgq_transport_send_close_op_pkt(qsk);
		return true;
	}

	if ((sk->sk_shutdown & SHUTDOWN_MASK) != SHUTDOWN_MASK)
		qmsgq_transport_send_shutdown_op_pkt(qsk, SHUTDOWN_MASK);

	if (sock_flag(sk, SOCK_DONE))
		return true;

	sock_hold(sk);
	INIT_DELAYED_WORK(&qsk->close_work,
			  qmsgq_transport_close_timeout);
	qsk->close_work_scheduled = true;
	schedule_delayed_work(&qsk->close_work, QMSGQ_CLOSE_TIMEOUT);

	return false;
}

static void qmsgq_transport_seqpacket_recv_listen(struct qmsgq_sock *qsk,
						  char *rx_buf,
						  struct sockaddr_vm *src,
						  struct sockaddr_vm *dst)
{
	struct qmsgq_hdr *hdr = (struct qmsgq_hdr *)rx_buf;
	struct qmsgq_sock *qsk_rsp = qsk;
	struct sock *sk = qsk_sk(qsk);
	struct qmsgq_sock *qsk_child;
	struct sock *sk_child;
	char *buf = NULL;
	u16 status = 0;
	int rc = 0;

	if (hdr->type != PKT_TYPE_SEQPACKET_OP_CONN_REQ) {
		pr_err("%s: error-qsk[%p] sk->sk_state[%u], hdr->type[%u]\n",
		       __func__, qsk, sk->sk_state, hdr->type);
		/* Drop packet */
		return;
	}

	if (sk_acceptq_is_full(sk)) {
		pr_err("%s: error-qsk[%p] sk_acceptq_is_full\n", __func__, qsk);
		status = -ENOMEM;
		goto resp;
	}

	sk_child = qmsgq_create_connected(sk);
	if (!sk_child) {
		pr_err("%s: error-qsk[%p] create sk\n", __func__, qsk);
		status = -ENOMEM;
		goto resp;
	}

	sk_acceptq_added(sk);

	lock_sock_nested(sk_child, SINGLE_DEPTH_NESTING);

	sk_child->sk_state = TCP_ESTABLISHED;

	qsk_child = sk_qsk(sk_child);
	vsock_addr_init(&qsk_child->local_addr, dst->svm_cid, dst->svm_port);
	vsock_addr_init(&qsk_child->remote_addr, src->svm_cid, src->svm_port);

	rc = qmsgq_assign_ep(qsk_child, qsk);
	if (rc) {
		pr_err("%s: error-qsk[%p] qmsgq_assign_ep\n", __func__, qsk);
		status = -ENOMEM;
		goto resp;
	}

	/* connected qmsgq sock created */
	qsk_rsp = qsk_child;

	qmsgq_insert_connected(qsk_child);
	qmsgq_enqueue_accept(sk, sk_child);

resp:
	buf = qmsgq_transport_seqpacket_build_op_pkt(PKT_TYPE_SEQPACKET_OP_CONN_RSP, status,
						     &qsk_rsp->local_addr,
						     &qsk_rsp->remote_addr);
	if (!buf) {
		rc = -ENOMEM;
		goto out;
	}

	rc = qmsgq_transport_seqpacket_send_op_pkt(qsk_rsp, buf);
	if (rc) {
		kfree(buf);
		goto out;
	}

	kfree(buf);

	/* connection established */
	if (status == 0) {
		pr_debug("%s: qsk[%p] qsk_child[%p]- SUCCESS\n", __func__, qsk, qsk_child);
		release_sock(sk_child);
		sk->sk_data_ready(sk);
		return;
	}
out:
	release_sock(sk_child);
	sock_put(sk_child);
}

static int qmsgq_transport_seqpacket_recv_connecting(struct qmsgq_sock *qsk, char *rx_buf)
{
	struct qmsgq_seqpacket_op_pkt *op_pkt = (struct qmsgq_seqpacket_op_pkt *)rx_buf;
	struct sock *sk = qsk_sk(qsk);
	int skerr;
	int err;

	switch (op_pkt->hdr.type) {
	case PKT_TYPE_SEQPACKET_OP_CONN_RSP:
		if (op_pkt->status) {
			pr_err("%s: connection err[%d]\n", __func__, op_pkt->status);
			skerr = ECONNREFUSED;
			err = -ECONNREFUSED;
			goto destroy;
		}
		sk->sk_state = TCP_ESTABLISHED;
		sk->sk_socket->state = SS_CONNECTED;
		qmsgq_insert_connected(qsk);
		sk->sk_state_change(sk);
		break;
	case PKT_TYPE_SEQPACKET_OP_SHUTDOWN:
		skerr = ECONNRESET;
		err = 0;
		goto destroy;
	default:
		skerr = EPROTO;
		err = -EINVAL;
		goto destroy;
	}

	return 0;

destroy:
	sk->sk_state = TCP_CLOSE;
	sk->sk_err = skerr;
	sk_error_report(sk);
	return err;
}

static int qmsgq_transport_seqpacket_recv_connected(struct qmsgq_sock *qsk,
						    char *rx_buf, size_t len,
						    struct sockaddr_vm *src,
						    struct sockaddr_vm *dst)
{
	struct qmsgq_hdr *hdr = (struct qmsgq_hdr *)rx_buf;
	struct sock *sk = qsk_sk(qsk);
	int data_len;
	int err = 0;
	void *data;

	switch (hdr->type) {
	case PKT_TYPE_SEQPACKET_DATA:
		data = rx_buf + sizeof(*hdr);
		data_len = len - sizeof(*hdr);
		qmsgq_transport_seqpacket_post(qsk, src, dst, data, data_len);
		break;
	case PKT_TYPE_SEQPACKET_OP_SHUTDOWN:
		qsk->peer_shutdown |= SHUTDOWN_MASK;
		if (!sock_flag(sk, SOCK_DONE)) {
			qmsgq_transport_send_close_op_pkt(qsk);
			qmsgq_transport_do_close(qsk, true);
		}
		qmsgq_remove_sock(qsk);
		sk->sk_state_change(sk);
		break;
	case PKT_TYPE_SEQPACKET_OP_CLOSE:
		qmsgq_transport_do_close(qsk, true);
		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int qmsgq_transport_seqpacket_recv_disconnecting(struct qmsgq_sock *qsk, char *rx_buf)
{
	struct qmsgq_hdr *hdr = (struct qmsgq_hdr *)rx_buf;
	int err = 0;

	switch (hdr->type) {
	case PKT_TYPE_SEQPACKET_OP_CLOSE:
		qmsgq_transport_do_close(qsk, true);
		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static void qmsgq_transport_seqpacket_recv(struct qmsgq_gh_device *qdev,
					   char *rx_buf, size_t len,
					   struct sockaddr_vm *src,
					   struct sockaddr_vm *dst)
{
	struct qmsgq_sock *qsk;
	struct sock *sk;

	sk = qmsgq_find_connected_socket(src, dst);
	if (!sk) {
		sk = qmsgq_find_bound_socket(dst);
		if (!sk) {
			pr_err("%s: no available sock found\n", __func__);
			qmsgq_transport_seqpacket_no_sock(qdev, rx_buf, ENOTSOCK);
			return;
		}
	}

	if (sk->sk_type != SOCK_SEQPACKET) {
		pr_err("%s: sk_type is wrong\n", __func__);
		qmsgq_transport_seqpacket_no_sock(qdev, rx_buf, ENOSTR);
		return;
	}

	qsk = sk_qsk(sk);

	lock_sock(sk);
	/* Check if sk has been closed before lock_sock */
	if (sock_flag(sk, SOCK_DONE)) {
		pr_err("%s: sock closed\n", __func__);
		qmsgq_transport_seqpacket_no_sock(qdev, rx_buf, EOPNOTSUPP);
		release_sock(sk);
		sock_put(sk);
		return;
	}

	switch (sk->sk_state) {
	case TCP_LISTEN:
		qmsgq_transport_seqpacket_recv_listen(qsk, rx_buf, src, dst);
		break;
	case TCP_SYN_SENT:
		qmsgq_transport_seqpacket_recv_connecting(qsk, rx_buf);
		break;
	case TCP_ESTABLISHED:
		qmsgq_transport_seqpacket_recv_connected(qsk, rx_buf, len, src, dst);
		break;
	case TCP_CLOSING:
		qmsgq_transport_seqpacket_recv_disconnecting(qsk, rx_buf);
		break;
	default:
		qmsgq_transport_seqpacket_no_sock(qdev, rx_buf, ESOCKTNOSUPPORT);
		break;
	}

	release_sock(sk);
	sock_put(sk);
}

static void qmsgq_transport_release(struct qmsgq_sock *qsk)
{
	struct sock *sk = qsk_sk(qsk);
	bool remove_sock = true;

	if (sk->sk_type == SOCK_SEQPACKET)
		remove_sock = qmsgq_transport_close(qsk);

	if (remove_sock) {
		sock_set_flag(sk, SOCK_DONE);
		qmsgq_transport_remove_sock(qsk);
	}
}

static bool qmsgq_transport_seqpacket_allow(u32 remote_cid)
{
	return true;
}

static ssize_t qmsgq_transport_seqpacket_dequeue(struct qmsgq_sock *qsk,
						 struct msghdr *msg,
						 int flags)
{
	return 0;
}

static int qmsgq_transport_seqpacket_enqueue(struct qmsgq_sock *qsk, struct msghdr *msg,
					     size_t len)
{
	const struct qmsgq_endpoint *ep;
	struct qmsgq_gh_device *qdev;
	struct qmsgq_hdr *hdr;
	char *buf;
	int rc;

	ep = qsk->ep;
	if (!ep)
		return -ENXIO;
	qdev = container_of(ep, struct qmsgq_gh_device, ep);

	if (!qdev->msgq_hdl) {
		pr_err("%s: Transport not ready\n", __func__);
		return -ENODEV;
	}

	if (len > MAX_PKT_SZ - sizeof(*hdr)) {
		pr_err("%s: Invalid pk size: len: %lu\n", __func__, len);
		return -EMSGSIZE;
	}

	/* Allocate a buffer for the user's message and our packet header. */
	buf = kmalloc(len + sizeof(*hdr), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Populate Header */
	hdr = (struct qmsgq_hdr *)buf;
	hdr->version = QMSGQ_GH_PROTO_VER_1;
	hdr->type = PKT_TYPE_SEQPACKET_DATA;
	hdr->flags = 0;
	hdr->optlen = 0;
	hdr->size = len;
	hdr->src_cid = qsk->local_addr.svm_cid;
	hdr->src_port_id = qsk->local_addr.svm_port;
	hdr->dst_cid = qsk->remote_addr.svm_cid;
	hdr->dst_port_id = qsk->remote_addr.svm_port;
	rc = memcpy_from_msg((void *)buf + sizeof(*hdr), msg, len);
	if (rc) {
		pr_err("%s: failed: memcpy_from_msg rc: %d\n", __func__, rc);
		goto send_err;
	}

	pr_debug("TX DATA: Len:0x%zx src[0x%x] dst[0x%x]\n", len, hdr->src_port_id,
		 hdr->dst_port_id);

	rc = qmsgq_transport_send_pkt(qdev, buf, len + sizeof(*hdr));
	if (rc < 0) {
		pr_err("%s: failed to send msg rc: %d\n", __func__, rc);
		goto send_err;
	}

	kfree(buf);
	return len;

send_err:
	kfree(buf);
	return rc;
}

static int qmsgq_transport_connect(struct qmsgq_sock *qsk)
{
	char *buf;
	int rc;

	buf = qmsgq_transport_seqpacket_build_op_pkt(PKT_TYPE_SEQPACKET_OP_CONN_REQ, 0,
						     &qsk->local_addr, &qsk->remote_addr);
	if (!buf)
		return -ENOMEM;

	rc = qmsgq_transport_seqpacket_send_op_pkt(qsk, buf);
	if (rc < 0)
		pr_err("%s: failed to send seqpacket rc: %d\n", __func__, rc);

	kfree(buf);
	return rc;
}

static int qmsgq_transport_socket_init(struct qmsgq_sock *qsk, struct qmsgq_sock *psk)
{
	return 0;
}

static void qmsgq_transport_destruct(struct qmsgq_sock *qsk)
{
}

static bool qmsgq_transport_allow_rsvd_cid(u32 cid)
{
	/* Allowing for cid 0 as of now as af_qmsgq sends 0 if no cid is
	 * passed by the client.
	 */
	if (cid == 0)
		return true;

	return false;
}

static bool qmsgq_transport_dgram_allow(u32 cid, u32 port)
{
	if (qmsgq_transport_allow_rsvd_cid(cid) || cid == VMADDR_CID_ANY || cid == VMADDR_CID_HOST)
		return true;

	pr_err("%s: dgram not allowed for cid 0x%x\n", __func__, cid);

	return false;
}

static u32 qmsgq_transport_get_local_cid(void)
{
	return VMADDR_CID_HOST;
}

static int qmsgq_transport_post(struct qmsgq_gh_device *qdev, char *rx_buf, size_t len)
{
	unsigned int cid, port, data_len;
	struct qmsgq_hdr *hdr;
	struct sockaddr_vm src;
	struct sockaddr_vm dst;
	void *data;
	int rc = 0;

	if (len < sizeof(*hdr)) {
		pr_err("%s: len: %zu < hdr size\n", __func__, len);
		return -EINVAL;
	}
	hdr = (struct qmsgq_hdr *)rx_buf;

	cid = le32_to_cpu(hdr->src_cid);
	port = le32_to_cpu(hdr->src_port_id);
	vsock_addr_init(&src, cid, port);

	cid = le32_to_cpu(hdr->dst_cid);
	port = le32_to_cpu(hdr->dst_port_id);
	vsock_addr_init(&dst, cid, port);

	data = rx_buf + sizeof(*hdr);
	data_len = len - sizeof(*hdr);

	switch (hdr->type) {
	case PKT_TYPE_DGRAM:
		qmsgq_transport_dgram_post(&qdev->ep, &src, &dst, data, data_len);
		break;
	case PKT_TYPE_SEQPACKET_DATA:
	case PKT_TYPE_SEQPACKET_OP_CONN_REQ:
	case PKT_TYPE_SEQPACKET_OP_CONN_RSP:
	case PKT_TYPE_SEQPACKET_OP_SHUTDOWN:
	case PKT_TYPE_SEQPACKET_OP_CLOSE:
		qmsgq_transport_seqpacket_recv(qdev, rx_buf, len, &src, &dst);
		break;
	default:
		pr_err("%s: invalid hdr type[%u]\n", __func__, hdr->type);
		break;
	}

	return rc;
}

void qmsgq_transport_process_recv(struct qmsgq_gh_device *qdev, void *buf, size_t len)
{
	struct qmsgq_gh_recv_buf *rx_buf = &qdev->rx_buf;
	struct qmsgq_hdr *hdr;
	size_t n;

	mutex_lock(&rx_buf->lock);

	/* Copy message into the local buffer */
	n = (rx_buf->copied + len < MAX_PKT_SZ) ? len : MAX_PKT_SZ - rx_buf->copied;
	memcpy(rx_buf->buf + rx_buf->copied, buf, n);
	rx_buf->copied += n;

	if (!rx_buf->hdr_received) {
		hdr = (struct qmsgq_hdr *)rx_buf->buf;

		if (hdr->version != QMSGQ_GH_PROTO_VER_1) {
			pr_err("%s: Incorrect version:%d\n", __func__, hdr->version);
			goto err;
		}

		if (hdr->size > MAX_PKT_SZ - sizeof(*hdr)) {
			pr_err("%s: Packet size too big:%d\n", __func__, hdr->size);
			goto err;
		}

		rx_buf->len = sizeof(*hdr) + hdr->size;
		rx_buf->hdr_received = true;
	}

	/* Check len size, can not be smaller than amount copied*/
	if (rx_buf->len < rx_buf->copied) {
		pr_err("%s: Size mismatch len:%zu, copied:%zu\n", __func__,
		       rx_buf->len, rx_buf->copied);
		goto err;
	}

	if (rx_buf->len == rx_buf->copied) {
		qmsgq_transport_post(qdev, rx_buf->buf, rx_buf->len);
		reset_buf(rx_buf);
	}

	mutex_unlock(&rx_buf->lock);
	return;

err:
	reset_buf(rx_buf);
	mutex_unlock(&rx_buf->lock);
}
EXPORT_SYMBOL_GPL(qmsgq_transport_process_recv);

int qmsgq_transport_init(struct qmsgq_gh_device *qdev)
{
	qdev->ep.module = THIS_MODULE;
	qdev->ep.init = qmsgq_transport_socket_init;
	qdev->ep.destruct = qmsgq_transport_destruct;
	qdev->ep.release = qmsgq_transport_release;
	qdev->ep.dgram_enqueue = qmsgq_transport_dgram_enqueue;
	qdev->ep.dgram_allow = qmsgq_transport_dgram_allow;

	qdev->ep.cancel_pkt = qmsgq_transport_cancel_pkt;
	qdev->ep.connect = qmsgq_transport_connect;
	qdev->ep.seqpacket_allow = qmsgq_transport_seqpacket_allow;
	qdev->ep.seqpacket_has_data = qmsgq_transport_seqpacket_has_data;
	qdev->ep.seqpacket_enqueue = qmsgq_transport_seqpacket_enqueue;
	qdev->ep.seqpacket_dequeue = qmsgq_transport_seqpacket_dequeue;

	qdev->ep.shutdown = qmsgq_transport_send_shutdown_op_pkt;
	qdev->ep.get_local_cid = qmsgq_transport_get_local_cid;

	return qmsgq_endpoint_register(&qdev->ep);
}
EXPORT_SYMBOL_GPL(qmsgq_transport_init);

MODULE_ALIAS("QMSGQ Transport");
MODULE_DESCRIPTION("QMSGQ Transport driver");
MODULE_LICENSE("GPL");
