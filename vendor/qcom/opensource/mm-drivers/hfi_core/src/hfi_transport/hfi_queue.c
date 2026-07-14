// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/list.h>
#include <linux/virtio_ring.h>
#include <linux/stdarg.h>
#include <linux/string.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/dma-map-ops.h>
#include <linux/version.h>
#include "hfi_queue.h"

#define HFI_Q_ERR(fmt, ...) \
	pr_err("[hfi_q_error:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

struct hfi_queue_buffer_pool {
	struct hfi_queue_buffer buffer;
	struct list_head list;
};

struct virtqueuehfi {
	struct virtio_device vdev;
	struct list_head avail_list;
	struct list_head wrapper_list;
	struct mutex q_lock;
	/* Virtq shared memory address/len */
	u64 kva;
	u32 len;
	struct virtqueue *vq;
	u32 avail_idx;
	u32 used_idx;
};

typedef int (*hfi_param_func_type)(struct virtqueuehfi *hfi_queue_handle,
			void *payload, u32 payload_sz);

static int set_hfi_buffer_pool(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int get_hfi_buffer_pool(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int set_hfi_buffer_queue(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int get_hfi_buffer_queue(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int set_hfi_buffer_reset(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int set_hfi_buffer_kickoff(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int get_hfi_buffer(struct virtqueuehfi *handle, void *payload,
	u32 payload_sz);
static int hfi_null_imp_get_set_func(struct virtqueuehfi *handle,
	void *payload, u32 payload_sz);
static int set_hfi_buffer_device_queue(struct virtqueuehfi *handle,
	void *payload, u32 payload_sz);
static int get_hfi_buffer_device_queue(struct virtqueuehfi *handle,
	void *payload, u32 payload_sz);


hfi_param_func_type set_param_func[hfi_queue_param_max] = {
	[hfi_queue_param_buffer_pool] = set_hfi_buffer_pool,
	[hfi_queue_param_buffer_queue] = set_hfi_buffer_queue,
	[hfi_queue_param_reset_buffer_queue] = set_hfi_buffer_reset,
	[hfi_queue_param_buffer] = hfi_null_imp_get_set_func,
	[hfi_queue_kickoff] = set_hfi_buffer_kickoff,
	[hfi_queue_param_device_buffer_queue] = set_hfi_buffer_device_queue,
};

hfi_param_func_type get_param_func[hfi_queue_param_max] = {
	[hfi_queue_param_buffer_pool] = get_hfi_buffer_pool,
	[hfi_queue_param_buffer_queue] = get_hfi_buffer_queue,
	[hfi_queue_param_reset_buffer_queue] = hfi_null_imp_get_set_func,
	[hfi_queue_param_buffer] = get_hfi_buffer,
	[hfi_queue_kickoff] = hfi_null_imp_get_set_func,
	[hfi_queue_param_device_buffer_queue] = get_hfi_buffer_device_queue,
};

u32 get_queue_mem_req(u32 q_depth, u32 align)
{
	if (!q_depth) {
		HFI_Q_ERR("invalid queue len %d\n", q_depth);
		return -EINVAL;
	}
	return vring_size(q_depth, align);
}

static void destroy_buffer_pool_wrappers(struct virtqueuehfi *handle)
{
	struct hfi_queue_buffer_pool *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &handle->wrapper_list, list) {
		vfree(entry);
	}
}

static int create_buffer_pool_wrappers(struct virtqueuehfi *handle, u32 qdepth)
{
	int ret = 0, i;
	struct hfi_queue_buffer_pool *pool;

	INIT_LIST_HEAD(&handle->wrapper_list);

	for (i = 0; i < qdepth; i++) {
		pool = NULL;
		pool = vzalloc(sizeof(*pool));
		if (!pool)
			ret = -ENOMEM;
		INIT_LIST_HEAD(&pool->list);
		list_add_tail(&pool->list, &handle->wrapper_list);
	}

	if (ret)
		destroy_buffer_pool_wrappers(handle);
	return ret;
}

static struct hfi_queue_buffer_pool *get_buffer_pool_wrapper(
	struct virtqueuehfi *handle)
{
	struct hfi_queue_buffer_pool *entry = NULL, *tmp;

	list_for_each_entry_safe(entry, tmp, &handle->wrapper_list, list) {
		list_del_init(&entry->list);
		break;
	}
	return entry;
}

static void return_buffer_pool_wrapper(struct virtqueuehfi *handle,
					struct hfi_queue_buffer_pool *entry)
{
	memset(&entry->buffer, 0, sizeof(entry->buffer));
	list_add_tail(&entry->list, &handle->wrapper_list);
}

static bool virtq_notify(struct virtqueue *vq)
{
	return true;
}

#if (KERNEL_VERSION(6, 3, 0) > LINUX_VERSION_CODE)
static dma_addr_t hfi_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs)
{
	return (dma_addr_t)page_address(page);
}

static void hfi_dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
		size_t size, enum dma_data_direction direction,
		unsigned long attrs)
{
}

static const struct dma_map_ops hfi_dma_ops = {
	.map_page = hfi_dma_map_page,
	.unmap_page = hfi_dma_unmap_page,
};
#endif

void *create_hfi_queue(struct hfi_queue_create *qinfo)
{
	struct virtqueuehfi *qhandle;

	if (!qinfo->dev || !qinfo->va || !qinfo->va_sz || !qinfo->q_depth) {
		HFI_Q_ERR("invalid params dev %pK kva %pK sz %d depth %d\n",
			qinfo->dev, qinfo->va, qinfo->va_sz, qinfo->q_depth);
		return NULL;
	}

	if (qinfo->va_sz != vring_size(qinfo->q_depth, qinfo->align)) {
		HFI_Q_ERR("invalid kva size %d exp %d\n", qinfo->va_sz,
			vring_size(qinfo->q_depth, qinfo->align));
		return NULL;
	}

	if (!qinfo->dma_premapped) {
		HFI_Q_ERR("client has to use pre-mapped buffers\n");
		return NULL;
	}

	qhandle = vzalloc(sizeof(*qhandle));
	if (!qhandle)
		return NULL;

	qhandle->vdev.features = VRING_USED_F_NO_NOTIFY;
	qhandle->vdev.features |= BIT_ULL(VIRTIO_F_ACCESS_PLATFORM);
	qhandle->vdev.dev.parent = qinfo->dev;
	INIT_LIST_HEAD(&qhandle->vdev.vqs);
	INIT_LIST_HEAD(&qhandle->avail_list);
	spin_lock_init(&qhandle->vdev.config_lock);
	spin_lock_init(&qhandle->vdev.vqs_list_lock);
	mutex_init(&qhandle->q_lock);

#if (KERNEL_VERSION(6, 3, 0) > LINUX_VERSION_CODE)
	set_dma_ops(qhandle->vdev.dev.parent, &hfi_dma_ops);
#endif
	qhandle->vq = vring_new_virtqueue(0, qinfo->q_depth, qinfo->align, &qhandle->vdev,
					false, false, qinfo->va, virtq_notify, NULL, qinfo->qname);
	if (!qhandle->vq) {
		HFI_Q_ERR("failed to create virtqueue\n");
		goto error;
	}
#if ((KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE) && \
	(KERNEL_VERSION(6, 13, 0) > LINUX_VERSION_CODE))
	if (virtqueue_set_dma_premapped(qhandle->vq)) {
		HFI_Q_ERR("failed to change virtq as permapped\n");
		goto error;
	}
#endif
	if (create_buffer_pool_wrappers(qhandle, qinfo->q_depth)) {
		HFI_Q_ERR("failed to create buffer pool wrapper\n");
		goto error;
	}
	return qhandle;

error:
	if (!qhandle)
		return NULL;

	if (qhandle->vq)
		vring_del_virtqueue(qhandle->vq);
	vfree(qhandle);
	return NULL;
}

int get_param_hfi_queue(void *hfi_queue_handle, enum hfi_queue_param_enum id,
			void *payload, u32 payload_sz)
{
	struct virtqueuehfi *qhandle = (struct virtqueuehfi *) hfi_queue_handle;
	int ret = -EINVAL;

	if (!qhandle || id >= hfi_queue_param_max) {
		HFI_Q_ERR("invalid param qhandle %pK param id %d\n", qhandle, id);
		return -EINVAL;
	}
	mutex_lock(&qhandle->q_lock);
	ret =  get_param_func[id](qhandle, payload, payload_sz);
	mutex_unlock(&qhandle->q_lock);
	return ret;
}

int set_param_hfi_queue(void *hfi_queue_handle, enum hfi_queue_param_enum id,
			void *payload, u32 payload_sz)
{
	struct virtqueuehfi *qhandle = (struct virtqueuehfi *) hfi_queue_handle;
	int ret = -EINVAL;

	if (!qhandle || id >= hfi_queue_param_max) {
		HFI_Q_ERR("invalid param qhandle %pK param id %d\n", qhandle, id);
		return -EINVAL;
	}
	mutex_lock(&qhandle->q_lock);
	ret =  set_param_func[id](qhandle, payload, payload_sz);
	mutex_unlock(&qhandle->q_lock);
	return ret;
}

void destroy_hfi_queue(void *hfi_queue_handle)
{
	if (hfi_queue_handle) {
		mutex_lock(&((struct virtqueuehfi *)hfi_queue_handle)->q_lock);
		vring_del_virtqueue(((struct virtqueuehfi *)hfi_queue_handle)->vq);
		mutex_unlock(&((struct virtqueuehfi *)hfi_queue_handle)->q_lock);
	}
}

static int hfi_null_imp_get_set_func(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	return -EPERM;
}

static bool check_buffer_in_pool(struct virtqueuehfi *handle, struct hfi_queue_buffer *pbuffer)
{
	struct hfi_queue_buffer_pool *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &handle->avail_list, list) {
		if (entry->buffer.kva == pbuffer->kva || entry->buffer.dva == pbuffer->dva)
			return true;
	}
	return false;
}

static int set_hfi_buffer_pool(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	struct hfi_queue_buffer_pool *buffer;
	struct hfi_queue_buffer *pbuffer = (struct hfi_queue_buffer *) payload;

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer) || !pbuffer->kva
	    || !pbuffer->dva || !pbuffer->buf_len) {
		HFI_Q_ERR("invalid params payload %pK size %d kva %pK !dva %x sz %d\n",
			payload, payload_sz, (pbuffer) ? (u32 *)pbuffer->kva : NULL,
				(pbuffer) ? (!pbuffer->dva) : 0, (pbuffer) ? pbuffer->buf_len : 0);
		return -EINVAL;
	}

	if (check_buffer_in_pool(handle, pbuffer))
		return -EALREADY;

	buffer = get_buffer_pool_wrapper(handle);
	if (!buffer)
		return -ENOMEM;

	buffer->buffer = *((struct hfi_queue_buffer *) (payload));
	list_add_tail(&buffer->list, &handle->avail_list);
	return 0;
}

static int get_hfi_buffer_pool(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	struct hfi_queue_buffer_pool *entry, *tmp;
	struct hfi_queue_buffer *pbuffer = (struct hfi_queue_buffer *) payload;

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer)) {
		HFI_Q_ERR("invalid params payload %pK size %d\n", payload, payload_sz);
		return -EINVAL;
	}

	if (list_empty(&handle->avail_list))
		return -ENOBUFS;

	list_for_each_entry_safe(entry, tmp, &handle->avail_list, list) {
		memcpy(pbuffer, &entry->buffer, sizeof(*pbuffer));
		list_del_init(&entry->list);
		return_buffer_pool_wrapper(handle, entry);
		break;
	}

	return 0;
}

static int set_hfi_buffer_queue(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	struct hfi_queue_buffer_queue *pbuffer = (struct hfi_queue_buffer_queue *) payload;
	struct hfi_queue_buffer_pool *token;
	struct scatterlist sglist;
	int ret;

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer_queue) || !pbuffer->buf ||
		!pbuffer->buf->kva || !pbuffer->buf->dva || !pbuffer->buf->buf_len) {
		HFI_Q_ERR("invalid params payload %pK size %d kva %pK !dva %x sz %d\n",
			payload, payload_sz,
			(pbuffer && pbuffer->buf) ? (u32 *)pbuffer->buf->kva : 0,
			(pbuffer && pbuffer->buf) ? !pbuffer->buf->dva : 0,
			(pbuffer && pbuffer->buf) ? pbuffer->buf->buf_len : 0);
		return -EINVAL;
	}

	token = get_buffer_pool_wrapper(handle);
	if (!token) {
		HFI_Q_ERR("token is null\n");
		return -ENOMEM;
	}

	memcpy(&token->buffer, pbuffer->buf, sizeof(token->buffer));
	sg_init_one(&sglist, (void *)((u64)pbuffer->buf->dva), pbuffer->buf->buf_len);
	/* TODO: below line is just workaround. Need to revisit for proper fix */
	sglist.dma_address = (dma_addr_t)pbuffer->buf->dva;

#if (KERNEL_VERSION(6, 13, 0) > LINUX_VERSION_CODE)
	if (pbuffer->dir == hfi_queue_rx)
		ret = virtqueue_add_inbuf(handle->vq, &sglist, 1, token, GFP_KERNEL);
	else
		ret = virtqueue_add_outbuf(handle->vq, &sglist, 1, token, GFP_KERNEL);
#else
	if (pbuffer->dir == hfi_queue_rx)
		ret = virtqueue_add_inbuf_premapped(handle->vq, &sglist, 1, token, NULL,
			GFP_KERNEL);
	else
		ret = virtqueue_add_outbuf_premapped(handle->vq, &sglist, 1, token,
			GFP_KERNEL);
#endif
	if (ret) {
		HFI_Q_ERR("failed: kva 0x%llx ret: %d\n",
			pbuffer->buf->kva, ret);
	}
	return ret;
}

static int get_hfi_buffer_queue(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	uint32_t len;
	struct hfi_queue_buffer_pool *token;

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer)) {
		HFI_Q_ERR("invalid params payload %pK size %d\n", payload, payload_sz);
		return -EINVAL;
	}

	token = (struct hfi_queue_buffer_pool *) virtqueue_get_buf(handle->vq, &len);
	if (!token)
		return -ENOBUFS;

	memcpy(payload, &token->buffer, payload_sz);
	return_buffer_pool_wrapper(handle, token);
	return 0;
}

static int set_hfi_buffer_reset(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	int ret = 0;
	struct hfi_queue_buffer buffer;
	struct hfi_queue_buffer_pool *token;

	/* Get back all used buffers and add it back to pool */
	while (!ret) {
		ret = get_hfi_buffer_queue(handle, &buffer, sizeof(buffer));
		if (!ret)
			set_hfi_buffer_pool(handle, &buffer, sizeof(buffer));
	}

	ret = 0;
	/* Get back all un-used buffers and add it back to pool */
	while (!ret) {
		token = virtqueue_detach_unused_buf(handle->vq);
		if (token) {
			buffer = token->buffer;
			return_buffer_pool_wrapper(handle, token);
			set_hfi_buffer_pool(handle, &buffer, sizeof(buffer));
		} else {
			ret = -ENOBUFS;
		}
	}

	return 0;
}

static int get_hfi_buffer(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	int ret;

	ret = get_hfi_buffer_pool(handle, payload, payload_sz);
	if (ret == -ENOBUFS)
		ret = get_hfi_buffer_queue(handle, payload, payload_sz);

	return ret;
}

static int set_hfi_buffer_kickoff(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
	bool kick_off;

	kick_off = virtqueue_kick(handle->vq);
	return (kick_off) ? 0 : -EINVAL;
}

static int set_hfi_buffer_device_queue(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
#if IS_ENABLED(CONFIG_HFI_CORE_SERAPH)
	const struct vring *ring = NULL;
#else
	const struct vring *ring = virtqueue_get_vring(handle->vq);
#endif

	struct hfi_queue_buffer *buffer = (struct hfi_queue_buffer *) payload;
	struct vring_used_elem *used_desc = NULL;
	u16 used_idx;
	u32 max_q_sz;

	if (!ring) {
		HFI_Q_ERR("invalid ring\n");
		return -EINVAL;
	}

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer)) {
		HFI_Q_ERR("invalid params payload %pK size %d\n", payload, payload_sz);
		return -EINVAL;
	}

	max_q_sz = virtqueue_get_vring_size(handle->vq);
	if (buffer->idx >= max_q_sz) {
		HFI_Q_ERR("invalid buffer idx %d max %d\n", buffer->idx, max_q_sz);
		return -EINVAL;
	}

	used_idx = ring->used->idx & (max_q_sz - 1);
	used_desc = &ring->used->ring[used_idx];
	used_desc->id = buffer->idx;
	used_desc->len = buffer->buf_len;
	ring->used->idx++;

	return 0;
}

static int get_hfi_buffer_device_queue(struct virtqueuehfi *handle, void *payload, u32 payload_sz)
{
#if IS_ENABLED(CONFIG_HFI_CORE_SERAPH)
	const struct vring *ring = NULL;
#else
	const struct vring *ring = virtqueue_get_vring(handle->vq);
#endif
	u32 head_idx, avail_idx;
	struct hfi_queue_buffer *buffer = (struct hfi_queue_buffer *) payload;

	if (!ring) {
		HFI_Q_ERR("invalid ring\n");
		return -EINVAL;
	}

	if (!payload || payload_sz != sizeof(struct hfi_queue_buffer)) {
		HFI_Q_ERR("invalid params payload %pK size %d\n", payload, payload_sz);
		return -EINVAL;
	}

	if (ring->avail->idx == handle->avail_idx)
		return -ENODATA;

	head_idx = handle->avail_idx++ & (virtqueue_get_vring_size(handle->vq) - 1);
	avail_idx = ring->avail->ring[head_idx];

	buffer->kva = ring->desc[avail_idx].addr;
	buffer->dva = ring->desc[avail_idx].addr;
	buffer->buf_len = ring->desc[avail_idx].len;
	buffer->idx = avail_idx;

	return 0;
}
