// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/sort.h>
#include <linux/of_platform.h>
#include "../include/uapi/misc/fastrpc.h"
#include <linux/of_reserved_mem.h>
#include "fastrpc_shared.h"
#include <linux/qcom_tvm_heap.h>
#include <linux/dma-heap.h>

struct fastrpc_channel_ctx *scctx = NULL;

/* Max dma heap pools for TVM memory */
#define FASTRPC_MAX_TVM_DMA_HEAPS (6)

/*
 * DMA heap pool size for a session:
 * - Init secure PD memory
 * - 8 persistent headers
 * - 10KB extra for any allocation needed
 * - 1MB for debug log buffer
 */
#define TVM_DMA_HEAP_SIZE (INIT_FILELEN_MAX + \
	FASTRPC_MAX_PERSISTENT_HEADERS*PAGE_SIZE + 10*1024 +\
	DBGLOGBUF_SIZE)

struct fastrpc_dma_heap_info {
	struct mutex heap_mut;
	struct fastrpc_tvm_dma_heap frpc_dma_heap[FASTRPC_MAX_TVM_DMA_HEAPS];
};

// Initialize dma heap's names
static struct fastrpc_dma_heap_info g_frpc_dma_heaps =
{
	.frpc_dma_heap =
	{
		{.name = "qcom,dsp-heap0"},
		{.name = "qcom,dsp-heap1"},
		{.name = "qcom,dsp-heap2"},
		{.name = "qcom,dsp-heap3"},
		{.name = "qcom,dsp-heap4"},
		{.name = "qcom,dsp-heap5"},
	},
};

/*
 * Iterate over list of dma heaps available to fastrpc and returns
 * next unused dma heap.
 *
 * @return 0 on success, negative error code on failure.
 */
static struct fastrpc_tvm_dma_heap *get_frpc_dma_heap(void)
{
	int ii;
	struct mutex *mut = &g_frpc_dma_heaps.heap_mut;
	struct fastrpc_tvm_dma_heap *frpc_dma_heap = g_frpc_dma_heaps.frpc_dma_heap;

	mutex_lock(mut);
	for(ii = 0; ii < ARRAY_SIZE(g_frpc_dma_heaps.frpc_dma_heap); ++ii) {
		if (!frpc_dma_heap[ii].in_use) {
			frpc_dma_heap[ii].in_use = true;
			mutex_unlock(mut);
			return &frpc_dma_heap[ii];
		}
	}
	mutex_unlock(mut);

	return NULL;
}

/*
 * Set TVM dma heap to unused.
 *
 * @param tvm_dma_heap  TVM dma heap to unreserve
 *
 * @return 0 on success, negative error code on failure.
 */
static void put_frpc_dma_heap(struct fastrpc_tvm_dma_heap *tvm_dma_heap)
{
	struct mutex *mut = &g_frpc_dma_heaps.heap_mut;

	mutex_lock(mut);
	tvm_dma_heap->in_use = false;
	tvm_dma_heap->mem_pool = NULL;
	tvm_dma_heap->dmaheap = NULL;
	mutex_unlock(mut);
}

void fastrpc_unreserve_dma_heap(struct fastrpc_tvm_dma_heap *tvm_dma_heap)
{
	qcom_tvm_heap_remove_kernel_pool(tvm_dma_heap->mem_pool);
	put_frpc_dma_heap(tvm_dma_heap);
	return;
}

int fastrpc_reserve_dma_heap(struct fastrpc_tvm_dma_heap **tvm_dma_heap)
{
	int err = 0;
	struct dma_heap *dmaheap = NULL;
	void *tvm_mem_pool = NULL;
	struct fastrpc_tvm_dma_heap *frpc_tvm_dma_heap = NULL;

	frpc_tvm_dma_heap = get_frpc_dma_heap();
	if (!frpc_tvm_dma_heap) {
		err = -EAGAIN;
		return err;
	}

	dmaheap = dma_heap_find(frpc_tvm_dma_heap->name);
	if (!dmaheap) {
		err = -ENOSR;
		goto dma_find_err;
	}

	tvm_mem_pool = qcom_tvm_heap_add_kernel_pool(dmaheap, TVM_DMA_HEAP_SIZE);
	if (IS_ERR_OR_NULL(tvm_mem_pool)) {
		err = PTR_ERR(tvm_mem_pool);
		goto dma_find_err;
	}

	frpc_tvm_dma_heap->mem_pool = tvm_mem_pool;
	frpc_tvm_dma_heap->dmaheap = dmaheap;
	*tvm_dma_heap = frpc_tvm_dma_heap;
	return 0;

dma_find_err:
	put_frpc_dma_heap(frpc_tvm_dma_heap);
	return err;
}

/*
 * __fastrpc_dma_heap_alloc() - Allocates memory from dma heaps.
 * In TVM driver dma heaps are predefined heaps that allow system to
 * donate memory from HLOS to TVM for dma allocation, instead of allocating
 * from carveout memory.
 * After memory is donates from HLOS, it needs to be mapped to SMMU and kernel.
 * @arg1: Fastrpc buffer
 *
 * Return: Returns 0 on success, error code on failure
 */
inline int __fastrpc_dma_alloc(struct fastrpc_buf *buf)
{
	struct dma_buf *dmabuf = NULL;
	struct fastrpc_smmu *smmucb = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct sg_table *table = NULL;
	struct fastrpc_tvm_dma_heap *tvm_dma_heap = buf->fl->tvm_dma_heap;
	int err = 0;

	smmucb = buf->smmucb;

	// Allocate dma buffer
	dmabuf = dma_heap_buffer_alloc(tvm_dma_heap->dmaheap, buf->size,
		O_CLOEXEC| O_RDWR, 0);
	if (IS_ERR_OR_NULL(dmabuf)) {
		err = PTR_ERR(dmabuf);
		return err;
	}

	attach = dma_buf_attach(dmabuf, smmucb->dev);
	if (IS_ERR_OR_NULL(attach)) {
		err = PTR_ERR(attach);
		goto attach_err;
	}

	// Map dma buffer to SMMU
	table = __dma_buf_map_attachment_wrap(attach);
	if (IS_ERR_OR_NULL(table)) {
		err = PTR_ERR(table);
		goto map_err;
	}

	// Map dma buffer to kernel address space
	err = dma_buf_vmap_unlocked(dmabuf, &buf->virt_map);
	if (err)
		goto vmap_err;

	buf->phys = sg_dma_address(table->sgl);
	buf->dmabuf = dmabuf;
	buf->table = table;
	buf->attach = attach;
	buf->virt = buf->virt_map.vaddr;

	return 0;
vmap_err:
	__dma_buf_unmap_attachment_wrap(attach, table);
map_err:
	dma_buf_detach(dmabuf, attach);
attach_err:
	dma_heap_buffer_free(dmabuf);
	return err;
}

void __fastrpc_dma_buf_free(struct fastrpc_buf *buf)
{
	dma_buf_vunmap_unlocked(buf->dmabuf, &buf->virt_map);
	__dma_buf_unmap_attachment_wrap(buf->attach, buf->table);
	dma_buf_detach(buf->dmabuf, buf->attach);
	dma_heap_buffer_free(buf->dmabuf);
}

struct fastrpc_channel_ctx* get_current_channel_ctx(struct device *dev)
{
	int err = 0;
	struct fastrpc_domain *domain = NULL;
	int str_len = 0;

	if (scctx)
		return scctx;

	scctx = kzalloc(sizeof(*scctx), GFP_KERNEL);
	if (IS_ERR_OR_NULL(scctx)) {
		dev_err(dev, "failed to get channel ctx\n");
		return ERR_PTR(-ENOMEM);
	}

	/*
	 * Currently only one NSP domain is supported for trusted driver.
	 */
	domain = kzalloc(sizeof(*domain), GFP_KERNEL);
	if (IS_ERR_OR_NULL(domain)) {
		dev_err(dev, "failed to allocate domain\n");
		return ERR_PTR(-ENOMEM);
	}
	str_len = (sizeof(legacy_domains[CDSP_DOMAIN_ID]) > sizeof(domain->name))?
		sizeof(domain->name): sizeof(legacy_domains[CDSP_DOMAIN_ID]);
	memcpy(domain->name, legacy_domains[CDSP_DOMAIN_ID], str_len);
	domain->id = CDSP_DOMAIN_ID;
	domain->type = FASTRPC_NSP;
	domain->cctx = scctx;
	scctx->domain = domain;

	err = of_property_read_u32(dev->of_node, "qcom,dsp-iova-format",
			&scctx->iova_format);

	scctx->domain_id = CDSP_DOMAIN_ID;
	atomic_set(&scctx->teardown, 0);
	scctx->secure = 0;
	scctx->unsigned_support = false;
	kref_init(&scctx->refcount);
	INIT_LIST_HEAD(&scctx->users);
	INIT_LIST_HEAD(&scctx->gmaps);
	mutex_init(&scctx->wake_mutex);
	spin_lock_init(&scctx->lock);
	spin_lock_init(&(scctx->gmsg_log.tx_lock));
	spin_lock_init(&(scctx->gmsg_log.rx_lock));
	idr_init(&scctx->ctx_idr);
	ida_init(&scctx->tgid_frpc_ida);
	init_waitqueue_head(&scctx->ssr_wait_queue);
	scctx->max_sess_per_proc = FASTRPC_MAX_SESSIONS_PER_PROCESS;
	fastrpc_device_register(dev, scctx, true, true,
		legacy_domains[scctx->domain_id]);
	fastrpc_device_register(dev, scctx, false, true,
		legacy_domains[scctx->domain_id]);
	return scctx;
}

static void fastrpc_recv_new_server(struct frpc_transport_session_control *session_control,
				unsigned int service, unsigned int instance,
				unsigned int node, unsigned int port)
{
	u32 remote_server_instance = session_control->remote_server_instance;
	int err = 0;

	/* Ignore EOF marker */
	if (!node && !port) {
		err = -EINVAL;
		dev_err(scctx->dev, "Ignoring ctrl packet: node %u, port %u, err %d",
					node, port, err);
		return;
	}

	if (service != FASTRPC_REMOTE_SERVER_SERVICE_ID ||
		instance != remote_server_instance) {
		err = -ENOMSG;
		dev_err(scctx->dev, "Ignoring ctrl packet: service id %u, instance id %u, err %d",
					service, instance, err);
		return;
	}

	mutex_lock(&session_control->frpc_socket.socket_mutex);
	session_control->frpc_socket.remote_sock_addr.sq_family = AF_QIPCRTR;
	session_control->frpc_socket.remote_sock_addr.sq_node = node;
	session_control->frpc_socket.remote_sock_addr.sq_port = port;
	session_control->remote_server_online = true;
	mutex_unlock(&session_control->frpc_socket.socket_mutex);
	dev_info(scctx->dev, "Remote server is up: remote ID (0x%x), node %u, port %u",
				remote_server_instance, node, port);
}

static void fastrpc_recv_del_server(struct frpc_transport_session_control *session_control,
				unsigned int node, unsigned int port)
{
	u32 remote_server_instance = session_control->remote_server_instance;
	int err = 0;
	struct fastrpc_user *user;
	unsigned long flags;

	/* Ignore EOF marker */
	if (!node && !port) {
		err = -EINVAL;
		dev_err(scctx->dev, "Ignoring ctrl packet: node %u, port %u, err %d",
					node, port, err);
		return;
	}

	if (node != session_control->frpc_socket.remote_sock_addr.sq_node ||
		port != session_control->frpc_socket.remote_sock_addr.sq_port) {
		dev_err(scctx->dev, "Ignoring ctrl packet: node %u, port %u, err %d", node, port, err);
		return;
	}

	mutex_lock(&session_control->frpc_socket.socket_mutex);
	session_control->frpc_socket.remote_sock_addr.sq_node = 0;
	session_control->frpc_socket.remote_sock_addr.sq_port = 0;
	session_control->remote_server_online = false;
	mutex_unlock(&session_control->frpc_socket.socket_mutex);
	spin_lock_irqsave(&scctx->lock, flags);
	list_for_each_entry(user, &scctx->users, user)
		fastrpc_notify_users(user);
	spin_unlock_irqrestore(&scctx->lock, flags);
	dev_info(scctx->dev, "Remote server is down: remote ID (0x%x)", remote_server_instance);
}

/**
 * fastrpc_recv_ctrl_pkt()
 * @session_control: Data structure that contains information related to socket and
 *                   remote server availability.
 * @buf: Control packet.
 * @len: Control packet length.
 *
 * Handle control packet status notifications from remote domain.
 */
static void fastrpc_recv_ctrl_pkt(struct frpc_transport_session_control *session_control,
					const void *buf, size_t len)
{
	const struct qrtr_ctrl_pkt *pkt = buf;

	if (len < sizeof(struct qrtr_ctrl_pkt)) {
		dev_err(scctx->dev, "Ignoring short control packet (%zu bytes)", len);
		return;
	}

	switch (le32_to_cpu(pkt->cmd)) {
	case QRTR_TYPE_NEW_SERVER:
		fastrpc_recv_new_server(session_control,
				    le32_to_cpu(pkt->server.service),
				    le32_to_cpu(pkt->server.instance),
				    le32_to_cpu(pkt->server.node),
				    le32_to_cpu(pkt->server.port));
		break;
	case QRTR_TYPE_DEL_SERVER:
		fastrpc_recv_del_server(session_control,
				    le32_to_cpu(pkt->server.node),
				    le32_to_cpu(pkt->server.port));
		break;
	default:
		dev_err(scctx->dev, "Ignoring unknown ctrl packet with size %zu", len);
	}
}

/**
 * fastrpc_socket_callback_wq()
 * @work: workqueue structure for incoming socket packets
 *
 * Callback function to receive responses that were posted on workqueue.
 * We expect to receive control packets with remote domain status notifications or
 * RPC data packets from remote domain.
 */
static void fastrpc_socket_callback_wq(struct work_struct *work)
{
	int err = 0, cid = -1, bytes_rx = 0;
	u32 remote_server_instance = (u32)-1;
	bool ignore_err = false;
	struct kvec msg = {0};
	struct sockaddr_qrtr remote_sock_addr = {0};
	struct msghdr remote_server = {0};
	struct frpc_transport_session_control *session_control = NULL;
	__u32 sq_node = 0, sq_port = 0;

	session_control = container_of(work, struct frpc_transport_session_control, work);
	if (session_control == NULL) {
		err = -EFAULT;
		goto bail;
	}

	remote_server.msg_name = &remote_sock_addr;
	remote_server.msg_namelen = sizeof(remote_sock_addr);
	msg.iov_base = session_control->frpc_socket.recv_buf;
	msg.iov_len = FASTRPC_SOCKET_RECV_SIZE;
	remote_server_instance = session_control->remote_server_instance;
	for (;;) {
		err = kernel_recvmsg(session_control->frpc_socket.sock, &remote_server, &msg, 1,
					msg.iov_len, MSG_DONTWAIT);
		if (err == -EAGAIN) {
			ignore_err = true;
			goto bail;
		}
		if (err < 0)
			goto bail;

		bytes_rx = err;
		err = 0;

		sq_node = remote_sock_addr.sq_node;
		sq_port = remote_sock_addr.sq_port;
		if (sq_node == session_control->frpc_socket.local_sock_addr.sq_node &&
			sq_port == QRTR_PORT_CTRL) {
			fastrpc_recv_ctrl_pkt(session_control,
							session_control->frpc_socket.recv_buf,
							bytes_rx);
		} else {
			cid = GET_CID_FROM_SERVER_INSTANCE(remote_server_instance);
			fastrpc_handle_rpc_response(scctx,
							msg.iov_base,
							msg.iov_len,
							false);
		}
	}
bail:
	if (!ignore_err && err < 0) {
		dev_err(scctx->dev,
			"invalid response data %pK (rx %d bytes), buffer len %zu from remote ID (0x%x) err %d\n",
			msg.iov_base, bytes_rx, msg.iov_len, remote_server_instance, err);
	}
}

/**
 * fastrpc_socket_callback()
 * @sk: Sock data structure with information related to the callback response.
 *
 * Callback function to receive responses from socket layer.
 * Responses are posted on workqueue to be process.
 */
static void fastrpc_socket_callback(struct sock *sk)
{
	int err = 0;
	struct frpc_transport_session_control *session_control = NULL;

	if (sk == NULL) {
		dev_err(scctx->dev, "invalid sock received, err %d", err);
		return;
	}

	rcu_read_lock();
	session_control = rcu_dereference_sk_user_data(sk);
	if (session_control)
		queue_work(session_control->wq, &session_control->work);
	rcu_read_unlock();
}

/**
 * fastrpc_transport_send()
 * @cid: Channel ID.
 * @rpc_msg: RPC message to send to remote domain.
 * @rpc_msg_size: RPC message size.
 * @tvm_remote_domain: Remote domain on TVM.
 *
 * Send RPC message to remote domain. Depending on tvm_remote_domain flag message will be
 * sent to one of the remote domains on remote subsystem.
 *
 * Return: 0 on success or negative errno value on failure.
 */
int fastrpc_transport_send(struct fastrpc_channel_ctx *cctx, void *rpc_msg, uint32_t rpc_msg_size)
{
	int ret = 0;
	struct fastrpc_socket *frpc_socket = NULL;
	struct frpc_transport_session_control *session_control = NULL;
	struct msghdr remote_server = {0};
	struct kvec msg = {0};

	session_control = &cctx->session_control;
	frpc_socket = &session_control->frpc_socket;
	remote_server.msg_name = &frpc_socket->remote_sock_addr;
	remote_server.msg_namelen = sizeof(frpc_socket->remote_sock_addr);

	msg.iov_base = rpc_msg;
	msg.iov_len = rpc_msg_size;

	mutex_lock(&frpc_socket->socket_mutex);
	if (frpc_socket->sock == NULL || session_control->remote_server_online == false) {
		mutex_unlock(&frpc_socket->socket_mutex);
		return -EPIPE;
	}

	ret = kernel_sendmsg(frpc_socket->sock, &remote_server, &msg, 1, msg.iov_len);
	if (ret > 0)
		ret = 0;

	mutex_unlock(&frpc_socket->socket_mutex);
	return ret;
}

/**
 * create_socket()
 * @session_control: Data structure that contains information related to socket and
 *                   remote server availability.
 *
 * Initializes and creates a kernel socket.
 *
 * Return: pointer to a socket on success or negative errno value on failure.
 */
static struct socket *create_socket(struct frpc_transport_session_control *session_control)
{
	int err = 0;
	struct socket *sock = NULL;
	struct fastrpc_socket *frpc_socket = NULL;

	err = sock_create_kern(&init_net, AF_QIPCRTR, SOCK_DGRAM,
			   PF_QIPCRTR, &sock);
	if (err < 0) {
		dev_err(scctx->dev, "sock_create_kern failed with err %d\n", err);
		return ERR_PTR(err);
	}
	frpc_socket = &session_control->frpc_socket;
	err = kernel_getsockname(sock, (struct sockaddr *)&frpc_socket->local_sock_addr);
	if (err < 0) {
		sock_release(sock);
		dev_err(scctx->dev, "kernel_getsockname failed with err %d\n", err);
		return ERR_PTR(err);
	}

	rcu_assign_sk_user_data(sock->sk, session_control);
	sock->sk->sk_data_ready = fastrpc_socket_callback;
	sock->sk->sk_error_report = fastrpc_socket_callback;
	return sock;
}

/**
 * register_remote_server_notifications()
 * @frpc_socket: Socket to send message to register for remote service notifications.
 * @remote_server_instance: ID to uniquely identify remote server
 *
 * Register socket to receive status notifications from remote service
 * using remote service ID FASTRPC_REMOTE_SERVER_SERVICE_ID and instance ID.
 *
 * Return: 0 on success or negative errno value on failure.
 */
static int register_remote_server_notifications(struct fastrpc_socket *frpc_socket,
				uint32_t remote_server_instance)
{
	struct qrtr_ctrl_pkt pkt = {0};
	struct sockaddr_qrtr sq = {0};
	struct msghdr remote_server = {0};
	struct kvec msg = { &pkt, sizeof(pkt) };
	int ret = 0;

	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = cpu_to_le32(QRTR_TYPE_NEW_LOOKUP);
	pkt.server.service = cpu_to_le32(FASTRPC_REMOTE_SERVER_SERVICE_ID);
	pkt.server.instance = cpu_to_le32(remote_server_instance);

	sq.sq_family = frpc_socket->local_sock_addr.sq_family;
	sq.sq_node = frpc_socket->local_sock_addr.sq_node;
	sq.sq_port = QRTR_PORT_CTRL;

	remote_server.msg_name = &sq;
	remote_server.msg_namelen = sizeof(sq);

	ret = kernel_sendmsg(frpc_socket->sock, &remote_server, &msg, 1, sizeof(pkt));

	if (ret < 0)
		dev_err(scctx->dev, "failed to send lookup registration: %d\n", ret);

	return ret;
}

/**
 * fastrpc_transport_init() - Initialize sockets for fastrpc driver.
 *
 * Initialize and create all sockets that are enabled from all channels
 * and remote domains.
 * Traverse array configurations and initialize session on glist_session_ctrl if remote
 * domain is enabled.
 *
 * Return: 0 on success or negative errno value on failure.
 */
int fastrpc_transport_init(void)
{
	int err = 0;
	struct socket *sock = NULL;
	struct fastrpc_socket *frpc_socket = NULL;
	struct frpc_transport_session_control *session_control = NULL;
	struct workqueue_struct *wq = NULL;

	if (!scctx) {
		err = -ENOMEM;
		goto bail;
	}

	session_control = &scctx->session_control;
	session_control->remote_server_online = false;
	frpc_socket = &session_control->frpc_socket;
	mutex_init(&frpc_socket->socket_mutex);
	mutex_init(&g_frpc_dma_heaps.heap_mut);

	sock = create_socket(session_control);
	if (!sock) {
		err = PTR_ERR(sock);
		goto bail;
	}

	frpc_socket->sock = sock;
	frpc_socket->recv_buf = kzalloc(FASTRPC_SOCKET_RECV_SIZE, GFP_KERNEL);
	if (!frpc_socket->recv_buf) {
		err = -ENOMEM;
		goto bail;
	}

	INIT_WORK(&session_control->work, fastrpc_socket_callback_wq);
	wq = alloc_workqueue("fastrpc_msg_handler", WQ_UNBOUND|WQ_HIGHPRI, 0);
	if (!wq) {
		err = -ENOMEM;
		goto bail;
	}
	session_control->wq = wq;

	session_control->remote_server_instance = GET_SERVER_INSTANCE(SECURE_PD, scctx->domain_id);
	err = register_remote_server_notifications(frpc_socket,
					session_control->remote_server_instance);
	if (err < 0)
		goto bail;

	dev_info(scctx->dev, "Created and registered socket for remote server (service ID %u, instance ID 0x%x)\n",
		FASTRPC_REMOTE_SERVER_SERVICE_ID, session_control->remote_server_instance);
	err = 0;
bail:
	if (err) {
		kfree(scctx->domain);
		scctx->domain = NULL;
		kfree(scctx);
		scctx = NULL;
		pr_err("fastrpc_transport_init failed with err %d\n", err);
	}
	return err;
}

/**
 * fastrpc_transport_deinit() - Deinitialize sockets for fastrpc driver.
 *
 * Deinitialize and release all sockets that are enabled from all channels
 * and remote domains.
 * Traverse array configurations and deinitialize corresponding session from
 * glist_session_ctrl.
 */
void fastrpc_transport_deinit(void)
{
	struct fastrpc_socket *frpc_socket = NULL;
	struct frpc_transport_session_control *session_control = NULL;

	if (!scctx) {
		pr_err("fastrpc_transport_deinit failed as scctx is NULL\n");
		return;
	}

	session_control = &scctx->session_control;
	if (!session_control)
		return;

	frpc_socket = &session_control->frpc_socket;

	if (frpc_socket->sock)
		sock_release(frpc_socket->sock);

	if (session_control->wq)
		destroy_workqueue(session_control->wq);

	kfree(frpc_socket->recv_buf);
	frpc_socket->recv_buf = NULL;
	frpc_socket->sock = NULL;
	mutex_destroy(&frpc_socket->socket_mutex);
	mutex_destroy(&g_frpc_dma_heaps.heap_mut);
}

/**
 * ssr_timer_callback() - Callback function for SSR timer.
 * @timer: Pointer to the timer structure.
 *
 * This function is called when the SSR timer expires. It does not perform
 * any operations for secure process.
 */
void ssr_timer_callback(struct timer_list *timer)
{
	return;
}

/**
 * fastrpc_is_device_crashing - Check if the device is about to crash
 *
 * This function does not execute any operations for secure process
 * and always returns false.
 *
 * @cctx: Pointer to the fastrpc_channel_ctx structure.
 *
 * @return: false.
 */
bool fastrpc_is_device_crashing(struct fastrpc_channel_ctx *cctx)
{
	return false;
}
