/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef __QMSGQ_GUNYAH_H_
#define __QMSGQ_GUNYAH_H_

#define MAX_PKT_SZ  SZ_64K

/* qmsgq_gh_recv_buf: transport buffer
 * @lock: lock for the buffer
 * @len: hdrlen + packet size
 * @copied: size of buffer copied
 * @hdr_received: true if the header is already saved, else false
 * @buf: buffer saved
 */
struct qmsgq_gh_recv_buf {
	/* @lock: lock for the buffer */
	struct mutex lock;
	size_t len;
	size_t copied;
	bool hdr_received;

	char buf[MAX_PKT_SZ];
};

/* qmsgq_gh_device: vm devices attached to this transport
 * @item: list item of all vm devices
 * @dev: device from platform_device
 * @ep: qmsq endpoint
 * @peer_cid: remote cid
 * @msgq_label: msgq label
 * @msgq_hdl: msgq handle
 * @rm_nb: notifier block for vm status from rm
 * @sock_ws: wakeup source
 * @rx_thread: rx thread to receive incoming packets
 */
struct qmsgq_gh_device {
	struct list_head item;
	struct device *dev;
	struct qmsgq_endpoint ep;

	unsigned int peer_cid;
	enum gh_msgq_label msgq_label;
	void *msgq_hdl;
	struct notifier_block rm_nb;

	struct wakeup_source *sock_ws;

	/* @tx_lock: tx lock to queue only one packet at a time */
	struct mutex tx_lock;
	struct task_struct *rx_thread;
	struct qmsgq_gh_recv_buf rx_buf;

	/* transport 'send_pkt' */
	int (*send_pkt)(struct qmsgq_gh_device *qdev, void *buf, size_t len);
};
#endif
