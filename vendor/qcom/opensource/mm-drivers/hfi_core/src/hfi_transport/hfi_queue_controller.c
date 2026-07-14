// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "hfi_interface.h"
#include "hfi_queue_controller.h"
#include "hfi_core_debug.h"
#include "hfi_core.h"
#include "hfi_smmu.h"
#include "hfi_queue.h"
#include "hfi_if_abstraction.h"

#define CLIENT_QUEUE_ALIGNMENT                     1024 // TODO: Revisit: SZ_4K

enum hfi_buffer_source {
	HFI_BUFF_SRC_BUFF_POOL = 0x1,
	HFI_BUFF_SRC_FW_QUEUE = 0x2,
};

struct hfi_queue_hdl {
	enum hfi_core_priority_type priority;
	void *q_tx_hdl;
	void *q_rx_hdl;
};

struct hfi_vq_queues_data {
	u32 num_queues;
	struct hfi_queue_hdl q_hdls[MAX_NUM_VIRTQ];
};

static void *get_queue_handle(struct hfi_vq_queues_data *hfi_queues, u32 prio,
	bool for_tx)
{
	for (int i = 0; i < hfi_queues->num_queues; i++) {
		if (hfi_queues->q_hdls[i].priority == prio) {
			if (for_tx)
				return hfi_queues->q_hdls[i].q_tx_hdl;
			else
				return hfi_queues->q_hdls[i].q_rx_hdl;
		}
	}

	return NULL;
}

static int push_buffers_to_buff_pool(void *q_hdl, u64 kva, u64 dva, u32 size)
{
	int ret = 0;
	struct hfi_queue_buffer buffer;

	HFI_CORE_DBG_H("+\n");

	if (!q_hdl) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct hfi_queue_buffer));
	buffer.kva = kva;
	buffer.dva = dva;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (hfi_core_loop_back_mode_enable)
		buffer.dva = kva;
#endif /* CONFIG_DEBUG_FS */
	buffer.buf_len = size;
	ret = set_param_hfi_queue(q_hdl, hfi_queue_param_buffer_pool, &buffer,
		sizeof(buffer));
	if (ret) {
		HFI_CORE_ERR(
			"failed to push buffer kva: 0x%llx to buff pool\n",
			buffer.kva);
		return ret;
	}

	HFI_CORE_DBG_H(
		"pushed buffer kva: 0x%llx iova :0x%llx size: %u to buff pool\n",
		buffer.kva, buffer.dva, buffer.buf_len);
	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int push_buffers_to_fw_queue(void *q_hdl, u64 kva, u64 dva,
	u32 size, u32 dir)
{
	int ret = 0;
	struct hfi_queue_buffer_queue buffer_payload;
	struct hfi_queue_buffer buffer;

	HFI_CORE_DBG_H("+\n");

	if (!q_hdl) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct hfi_queue_buffer));
	buffer.kva = kva;
	buffer.dva = (u64)dva;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (hfi_core_loop_back_mode_enable)
		buffer.dva = kva;
#endif /* CONFIG_DEBUG_FS */
	buffer.buf_len = size;
	buffer_payload.dir = dir;
	buffer_payload.buf = &buffer;
	ret = set_param_hfi_queue(q_hdl, hfi_queue_param_buffer_queue,
		&buffer_payload, sizeof(buffer_payload));
	if (ret) {
		HFI_CORE_ERR(
			"failed to push buffer kva: 0x%llx to fw queue\n", buffer.kva);
		return ret;
	}

	HFI_CORE_DBG_H("pushed buffer kva: 0x%llx iova :0x%llx size: %u to fw queue\n",
		buffer.kva, buffer.dva, buffer.buf_len);
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int set_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_core_cmds_buf_desc *buf_desc_index;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc || !num_buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < num_buff_desc; i++) {
		if (!buff_desc || !(*buff_desc)) {
			HFI_CORE_ERR("rx buff is null\n");
			return -EINVAL;
		}
		buf_desc_index = *buff_desc;

		queue_hdl = get_queue_handle(hfi_queues,
			buf_desc_index->prio_info, true);
		if (!queue_hdl) {
			HFI_CORE_ERR(
				"q handle with prio: %d is not found for buffer: 0x%llx\n",
				buf_desc_index->prio_info,
				(u64)buf_desc_index);
			return -EINVAL;
		}

		ret = push_buffers_to_fw_queue(queue_hdl,
			(u64)buf_desc_index->pbuf_vaddr,
			buf_desc_index->priv_dva, buf_desc_index->size,
			hfi_queue_tx);
		if (ret) {
			HFI_CORE_ERR(
				"failed to set %d buffer to fw queue\n", i);
			return ret;
		}

		/* tx buffers kick off to DCP */
		ret = set_param_hfi_queue(queue_hdl, hfi_queue_kickoff, NULL, 0);
		if (ret) {
			HFI_CORE_ERR(
				"failed to kick off tx queue buffers\n");
			return ret;
		}
		HFI_CORE_DBG_L("pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx prio: %d\n",
			(u64)buf_desc_index->pbuf_vaddr, buf_desc_index->size,
			buf_desc_index->priv_dva, buf_desc_index->prio_info);

		buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int get_rx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_queue_buffer buffer;
	bool found_rx_buff = false;
	void *queue_hdl;
	u32 num_queues_searched = 0;
	int prio = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	/* start checking from highest priority queue */
	for (prio = HFI_CORE_PRIO_0; prio < HFI_CORE_PRIO_MAX; prio++) {
		if (num_queues_searched == hfi_queues->num_queues)
			break;

		queue_hdl = get_queue_handle(hfi_queues, prio, false);
		if (!queue_hdl)
			continue;

		/* get a buffer from tx buffer pool */
		ret = get_param_hfi_queue(queue_hdl, hfi_queue_param_buffer_queue,
			&buffer, sizeof(buffer));
		if (!ret) {
			found_rx_buff = true;
			break;
		}

		if (ret == -ENOBUFS) {
			HFI_CORE_DBG_H("prio %d queue does not have any rx buf\n", prio);
		} else if (ret) {
			HFI_CORE_ERR("failed to get rx buf for %d queue\n", prio);
			goto exit;
		}
		num_queues_searched += 1;
	}

	if (!found_rx_buff) {
		HFI_CORE_DBG_H("no rx buf available in any queues\n");
		ret = -ENOBUFS;
		goto exit;
	}

	buff_desc->prio_info = prio;
	buff_desc->pbuf_vaddr = (void *)buffer.kva;
	buff_desc->priv_dva = buffer.dva;
	buff_desc->size = buffer.buf_len;

	HFI_CORE_DBG_L("pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx prio: %d\n",
		(u64)buff_desc->pbuf_vaddr, buff_desc->size,
		buff_desc->priv_dva, buff_desc->prio_info);
	HFI_CORE_DBG_H("-\n");
exit:
	return ret;
}

int get_tx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_queue_buffer buffer;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	queue_hdl = get_queue_handle(hfi_queues,
		buff_desc->prio_info, true);
	if (!queue_hdl) {
		HFI_CORE_ERR(
			"q handle with prio: %d is not found for buffer: 0x%llx\n",
			buff_desc->prio_info, (u64)buff_desc);
		return -EINVAL;
	}

	/* get a buffer from tx buffer pool */
	ret = get_param_hfi_queue(queue_hdl, hfi_queue_param_buffer,
		&buffer, sizeof(buffer));
	if (ret) {
		HFI_CORE_ERR("failed to get buffer, ret: %d\n", ret);
		return ret;
	}

	buff_desc->pbuf_vaddr = (void *)buffer.kva;
	buff_desc->priv_dva = buffer.dva;
	buff_desc->size = buffer.buf_len;

	HFI_CORE_DBG_L("pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx prio: %d\n",
		(u64)buff_desc->pbuf_vaddr, buff_desc->size,
		buff_desc->priv_dva, buff_desc->prio_info);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int put_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	int ret = 0;
	struct hfi_core_cmds_buf_desc *buf_desc_index;
	struct hfi_vq_queues_data *hfi_queues;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc || !num_buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < num_buff_desc; i++) {
		if (!buff_desc || !(*buff_desc)) {
			HFI_CORE_ERR("rx buff is null\n");
			return -EINVAL;
		}
		buf_desc_index = *buff_desc;

		queue_hdl = get_queue_handle(hfi_queues,
			buf_desc_index->prio_info, true);
		if (!queue_hdl) {
			HFI_CORE_ERR(
				"q handle with prio: %d is not found for buffer: 0x%llx\n",
				buf_desc_index->prio_info,
				(u64)buf_desc_index);
			return -EINVAL;
		}

		ret = push_buffers_to_buff_pool(queue_hdl,
			(u64)buf_desc_index->pbuf_vaddr,
			buf_desc_index->priv_dva, buf_desc_index->size);
		if (ret) {
			HFI_CORE_ERR(
				"failed to put %d buffer to buff pool\n", i);
			return ret;
		}
		HFI_CORE_DBG_L("pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx prio: %d\n",
			(u64)buf_desc_index->pbuf_vaddr, buf_desc_index->size,
			buf_desc_index->priv_dva, buf_desc_index->prio_info);

		buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int put_rx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	int ret = 0;
	struct hfi_core_cmds_buf_desc *buf_desc_index;
	struct hfi_vq_queues_data *hfi_queues;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc || !num_buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < num_buff_desc; i++) {
		if (!buff_desc || !(*buff_desc)) {
			HFI_CORE_ERR("rx buff is null\n");
			return -EINVAL;
		}
		buf_desc_index = *buff_desc;

		queue_hdl = get_queue_handle(hfi_queues,
			buf_desc_index->prio_info, false);
		if (!queue_hdl) {
			HFI_CORE_ERR(
				"q handle with prio: %d is not found for buffer: 0x%llx\n",
				buf_desc_index->prio_info,
				(u64)buf_desc_index);
			return -EINVAL;
		}

		ret = push_buffers_to_fw_queue(queue_hdl,
			(u64)buf_desc_index->pbuf_vaddr,
			buf_desc_index->priv_dva, buf_desc_index->size,
			hfi_queue_rx);
		if (ret) {
			HFI_CORE_ERR(
				"failed to put %d buffer to fw queue\n", i);
			return ret;
		}

		/* rx buffers kick off to DCP */
		ret = set_param_hfi_queue(queue_hdl, hfi_queue_kickoff, NULL, 0);
		if (ret) {
			HFI_CORE_ERR("failed to kick off %d buffer\n", i);
			return ret;
		}
		HFI_CORE_DBG_L("pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx prio: %d\n",
			(u64)buf_desc_index->pbuf_vaddr, buf_desc_index->size,
			buf_desc_index->priv_dva, buf_desc_index->prio_info);

		buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int get_queue_size_req(u32 qdepth)
{
	return get_queue_mem_req(qdepth, CLIENT_QUEUE_ALIGNMENT);
}

static int submit_all_buffers(enum hfi_buffer_source src, void *q_hdl,
	struct hfi_memory_alloc_info *alloc_info, u32 num_buffs, u32 buff_size,
	bool is_tx_dir)
{
	int ret = 0;
	u8 *cpu_va_byte_addr = NULL;
	unsigned long dva = 0;

	HFI_CORE_DBG_H("+\n");

	if (!q_hdl || !alloc_info || !num_buffs || !buff_size) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (src != HFI_BUFF_SRC_BUFF_POOL && src != HFI_BUFF_SRC_FW_QUEUE) {
		HFI_CORE_ERR("invalid src: %d to submit buffers\n", src);
		return -EINVAL;
	}

	cpu_va_byte_addr = (u8 *)alloc_info->cpu_va + alloc_info->size_wr;
	dva = alloc_info->mapped_iova + alloc_info->size_wr;
	for (int i = 0; i < num_buffs; i++) {
		if (src == HFI_BUFF_SRC_BUFF_POOL) {
			ret = push_buffers_to_buff_pool(q_hdl,
				(u64)cpu_va_byte_addr,
				dva, buff_size);
		} else if (src == HFI_BUFF_SRC_FW_QUEUE) {
			ret = push_buffers_to_fw_queue(q_hdl,
				(u64)cpu_va_byte_addr,
				dva, buff_size,
				is_tx_dir ? hfi_queue_tx : hfi_queue_rx);
		}
		if (ret) {
			HFI_CORE_ERR("failed to submit %d buffer\n", i);
			return ret;
		}

		alloc_info->size_wr += buff_size;
		cpu_va_byte_addr += buff_size;
		dva = dva + buff_size;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int add_tx_buffers_to_pool(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_vq_queues_data *hq;
	struct hfi_res_vq_queue_data *q_buff_desc;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hq = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < hq->num_queues; i++) {
		if (!hq->q_hdls[i].q_tx_hdl)
			continue;

		q_buff_desc = &res_data->vitq_res.q_mem[i];
		ret = submit_all_buffers(HFI_BUFF_SRC_BUFF_POOL,
			hq->q_hdls[i].q_tx_hdl, &q_buff_desc->buff_mem,
			q_buff_desc->q_info.tx_elements,
			q_buff_desc->q_info.tx_buff_size_bytes, true);
		if (ret) {
			HFI_CORE_ERR(
				"failed to add %d queue tx buffers to pool\n",
				i);
			return ret;
		}
	}

	HFI_CORE_DBG_H("-\n");
	return 0;
}

static int add_rx_buffers_to_fw_queue(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_vq_queues_data *hq;
	struct hfi_res_vq_queue_data *q_buff_desc;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hq = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < hq->num_queues; i++) {
		if (!hq->q_hdls[i].q_rx_hdl)
			continue;

		q_buff_desc = &res_data->vitq_res.q_mem[i];
		ret = submit_all_buffers(HFI_BUFF_SRC_FW_QUEUE,
			hq->q_hdls[i].q_rx_hdl, &q_buff_desc->buff_mem,
			q_buff_desc->q_info.rx_elements,
			q_buff_desc->q_info.rx_buff_size_bytes, false);
		if (ret) {
			HFI_CORE_ERR(
				"failed to add %d queue rx buffers to pool\n",
					i);
			return ret;
		}

		/* rx buffers kick off to DCP */
		ret = set_param_hfi_queue(hq->q_hdls[i].q_rx_hdl,
			hfi_queue_kickoff, NULL, 0);
		if (ret) {
			HFI_CORE_ERR(
				"failed to kick off %d rx queue buffers\n", i);
			return ret;
		}
	}

	HFI_CORE_DBG_H("-\n");
	return 0;
}

static int create_queue(struct hfi_core_drv_data *drv_data,
	struct hfi_res_vq_queue_data *buff_desc_mem, struct hfi_queue_hdl *hdl)
{
	u32 min_size_req_tx = 0, min_size_req_rx = 0, allocated_size = 0;
	struct hfi_queue_create queue_params;
	struct hfi_memory_alloc_info *alloc_info;
	bool tx_q_req = false, rx_q_req = false;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->dev || !buff_desc_mem || !hdl) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	alloc_info = &buff_desc_mem->buff_desc_mem;

	/* check for min size memory allocation required for both tx and rx queue */
	min_size_req_tx = get_queue_size_req(buff_desc_mem->q_info.tx_elements);
	min_size_req_rx = get_queue_size_req(buff_desc_mem->q_info.rx_elements);
	allocated_size = alloc_info->size_allocated;
	if ((min_size_req_tx + min_size_req_rx) > allocated_size) {
		HFI_CORE_ERR(
			"insufficient queue size, allocated: %u, req: %u\n",
			allocated_size, (min_size_req_tx + min_size_req_rx));
		return -EINVAL;
	}

	/* assign queue priority */
	hdl->priority = buff_desc_mem->q_info.priority;

	/* check what type of queues are required */
	if (buff_desc_mem->q_info.type == HFI_VIRT_QUEUE_FULL_DUP &&
		buff_desc_mem->q_info.tx_elements &&
		buff_desc_mem->q_info.rx_elements) {
		tx_q_req = true;
		rx_q_req = true;
	} else if (buff_desc_mem->q_info.type == HFI_VIRT_QUEUE_TX &&
		buff_desc_mem->q_info.tx_elements) {
		tx_q_req = true;
	} else if (buff_desc_mem->q_info.type == HFI_VIRT_QUEUE_RX &&
		buff_desc_mem->q_info.rx_elements) {
		rx_q_req = true;
	} else {
		HFI_CORE_ERR("invalid queue type requested: %d for queue id: %d\n",
			buff_desc_mem->q_info.type, buff_desc_mem->queue_id);
		return -EINVAL;
	}

	if (tx_q_req) {
		/* create tx queue */
		queue_params.dev = (struct device *)drv_data->dev;
		queue_params.qname = "HFI_TX_QUEUE";
		queue_params.va = alloc_info->cpu_va;
		queue_params.va_sz = min_size_req_tx;
		queue_params.q_depth = buff_desc_mem->q_info.tx_elements;
		queue_params.align = CLIENT_QUEUE_ALIGNMENT;
		queue_params.dma_premapped = true;

		hdl->q_tx_hdl = create_hfi_queue(&queue_params);
		if (!hdl->q_tx_hdl) {
			HFI_CORE_ERR("failed to create tx queue\n");
			goto tx_q_fail;
		}
		HFI_CORE_DBG_H("TX Q: id:%d prio:%d addr:0x%llx size:0x%x qdepth:%d align:%u\n",
			buff_desc_mem->queue_id,
			buff_desc_mem->q_info.priority,
			(u64)queue_params.va, queue_params.va_sz,
			queue_params.q_depth, queue_params.align);
	}

	if (rx_q_req) {
		/* create rx queue */
		memset(&queue_params, 0, sizeof(struct hfi_queue_create));
		queue_params.dev = (struct device *)drv_data->dev;
		queue_params.qname = "HFI_RX_QUEUE";
		queue_params.va = (alloc_info->cpu_va + min_size_req_tx);
		queue_params.va_sz = min_size_req_rx;
		queue_params.q_depth = buff_desc_mem->q_info.rx_elements;
		queue_params.align = CLIENT_QUEUE_ALIGNMENT;
		queue_params.dma_premapped = true;
		hdl->q_rx_hdl = create_hfi_queue(&queue_params);
		if (!hdl->q_rx_hdl) {
			HFI_CORE_ERR("failed to create rx queue\n");
			goto rx_q_fail;
		}
		HFI_CORE_DBG_H("RX Q: id:%d prio:%d addr:0x%llx size:0x%x qdepth:%d align:%u\n",
			buff_desc_mem->queue_id,
			buff_desc_mem->q_info.priority,
			(u64)queue_params.va, queue_params.va_sz,
			queue_params.q_depth, queue_params.align);
	}

	HFI_CORE_DBG_H("-\n");
	return 0;

rx_q_fail:
	destroy_hfi_queue(hdl->q_tx_hdl);
tx_q_fail:
	return -EINVAL;
}

static int setup_vq_queues(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_vq_queues_data *hq;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;

	HFI_CORE_DBG_H("+\n");

	/* Allocate hfi queues storage memory */
	hq = kzalloc(sizeof(struct hfi_vq_queues_data), GFP_KERNEL);
	if (!hq) {
		HFI_CORE_ERR("failed to allocate hfi queues\n");
		return -ENOMEM;
	}
	drv_data->client_data[client_id].queue_info.data = (void *)hq;

	/* create queues */
	hq->num_queues = res_data->vitq_res.num_queues;
	for (int i = 0; i < hq->num_queues; i++) {
		ret = create_queue(drv_data, &res_data->vitq_res.q_mem[i],
			&hq->q_hdls[i]);
		if (ret) {
			HFI_CORE_ERR("failed to create %d queue\n", i);
			goto error;
		}
	}

	/* add all queues all tx buffers to buffer pool */
	ret = add_tx_buffers_to_pool(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to submit tx buffers\n");
		goto error;
	}

	/* add all queues all rx buffers to buffer pool */
	ret = add_rx_buffers_to_fw_queue(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to submit rx buffers\n");
		goto error;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;

error:
	for (int i = 0; i < hq->num_queues; i++) {
		if (hq->q_hdls[i].q_tx_hdl)
			destroy_hfi_queue(hq->q_hdls[i].q_tx_hdl);
		if (hq->q_hdls[i].q_rx_hdl)
			destroy_hfi_queue(hq->q_hdls[i].q_rx_hdl);
	}
	kfree(hq);
	drv_data->client_data[client_id].queue_info.data = NULL;

	return ret;
}

int init_queues(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_resource_data *res_data;

	HFI_CORE_DBG_H("+\n");

	if (client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client_id);
		return -EINVAL;
	}

	if (!drv_data ||
		!drv_data->client_data[client_id].resource_info.res_data_mem) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;

	if (!res_data->res_count) {
		HFI_CORE_ERR("no resources available to setup queues\n");
		return -EINVAL;
	}

	ret = setup_vq_queues(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to setup virtq hfi queues\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int deinit_queues(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_vq_queues_data *hq;
	struct hfi_queue_hdl *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client_id);
		return -EINVAL;
	}

	if (!drv_data || !drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	hq = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	for (int i = 0; i < hq->num_queues; i++) {
		queue_hdl = &hq->q_hdls[i];
		if (!queue_hdl) {
			HFI_CORE_ERR("queue hdl for %d queue is null\n", i);
			continue;
		}

		/* reset all tx buffers*/
		ret = set_param_hfi_queue(queue_hdl->q_tx_hdl,
			hfi_queue_param_reset_buffer_queue, NULL, 0);
		if (ret) {
			HFI_CORE_ERR(
				"failed to reset %d tx queue buffers\n", i);
			return ret;
		}
		/* destroy tx queue */
		destroy_hfi_queue(queue_hdl->q_tx_hdl);

		/* reset all rx buffers*/
		ret = set_param_hfi_queue(queue_hdl->q_rx_hdl,
			hfi_queue_param_reset_buffer_queue, NULL, 0);
		if (ret) {
			HFI_CORE_ERR(
				"failed to reset %d rx queue buffers\n", i);
			return ret;
		}
		/* destroy rx queue */
		destroy_hfi_queue(queue_hdl->q_rx_hdl);
	}

	/* free hfi queue data */
	kfree(hq);
	drv_data->client_data[client_id].queue_info.data = NULL;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)

static int push_device_buffers_to_queue(void *q_hdl, u64 kva, u32 idx,
	u32 size)
{
	int ret = 0;
	struct hfi_queue_buffer buffer;

	HFI_CORE_DBG_H("+\n");

	if (!q_hdl) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct hfi_queue_buffer));
	buffer.kva = kva;
	buffer.dva = kva;
	buffer.idx = idx;
	buffer.buf_len = size;
	ret = set_param_hfi_queue(q_hdl, hfi_queue_param_device_buffer_queue,
		&buffer, sizeof(buffer));
	if (ret) {
		HFI_CORE_ERR(
			"failed to push device buffer kva: 0x%llx to host queue\n",
			buffer.kva);
		return ret;
	}

	HFI_CORE_DBG_H(
		"pushed device buffer kva: 0x%llx iova :0x%llx size: %u to host queue\n",
		buffer.kva, buffer.dva, buffer.buf_len);
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int set_device_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_core_cmds_buf_desc *buf_desc_index;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		HFI_CORE_ERR("invalid client id: %d\n", client_id);
		return -EINVAL;
	}

	/* loopback client uses client 0 resources */
	client_id = HFI_CORE_CLIENT_ID_0;

	if (!drv_data || !buff_desc || !num_buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	/* queue priority ? */
	for (int i = 0; i < num_buff_desc; i++) {
		if (!buff_desc || !(*buff_desc)) {
			HFI_CORE_ERR("rx buff is null\n");
			return -EINVAL;
		}
		buf_desc_index = *buff_desc;

		queue_hdl = get_queue_handle(hfi_queues,
			buf_desc_index->prio_info, false);
		if (!queue_hdl) {
			HFI_CORE_ERR(
				"q handle with prio: %d is not found for buffer: 0x%llx\n",
				buf_desc_index->prio_info,
				(u64)buf_desc_index);
			return -EINVAL;
		}

		ret = push_device_buffers_to_queue(queue_hdl,
			(u64)buf_desc_index->pbuf_vaddr,
			buf_desc_index->priv_idx,
			buf_desc_index->size);
		if (ret) {
			HFI_CORE_ERR(
				"failed to set %d device buffer to host queue\n",
				i);
			return ret;
		}

		/* tx buffers kick off to DCP */
		ret = set_param_hfi_queue(queue_hdl, hfi_queue_kickoff, NULL, 0);
		if (ret) {
			HFI_CORE_ERR(
				"failed to kick off tx queue buffers\n");
			return ret;
		}
		HFI_CORE_DBG_L(
			"pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx idx: %d prio: %d\n",
			(u64)buf_desc_index->pbuf_vaddr, buf_desc_index->size,
			buf_desc_index->priv_dva, buf_desc_index->priv_idx,
			buf_desc_index->prio_info);

		buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int get_device_rx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_queue_buffer buffer;
	bool found_rx_buff = false;
	void *queue_hdl;
	u32 num_queues_searched = 0;
	int prio = 0;

	HFI_CORE_DBG_H("+\n");

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		HFI_CORE_ERR("invalid client id: %d\n", client_id);
		return -EINVAL;
	}

	/* loopback client uses client 0 resources */
	client_id = HFI_CORE_CLIENT_ID_0;

	if (!drv_data || !buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	/* start checking from highest priority queue */
	for (prio = HFI_CORE_PRIO_0; prio < HFI_CORE_PRIO_MAX; prio++) {
		if (num_queues_searched == hfi_queues->num_queues)
			break;

		queue_hdl = get_queue_handle(hfi_queues, prio, true);
		if (!queue_hdl)
			continue;

		/* get a buffer from tx buffer pool */
		ret = get_param_hfi_queue(queue_hdl,
			hfi_queue_param_device_buffer_queue,
			&buffer, sizeof(buffer));
		if (!ret) {
			found_rx_buff = true;
			break;
		}

		if (ret == -ENODATA) {
			HFI_CORE_DBG_H(
				"prio %d queue does not have any rx buf\n",
				prio);
		} else if (ret) {
			HFI_CORE_ERR(
				"failed to get rx buf for prio %d queue\n",
				prio);
			goto exit;
		}
		num_queues_searched += 1;
	}

	if (!found_rx_buff) {
		HFI_CORE_DBG_H("no rx buf available in any queues\n");
		ret = -ENOBUFS;
		goto exit;
	}

	buff_desc->prio_info = prio;
	buff_desc->pbuf_vaddr = (void *)buffer.kva;
	buff_desc->priv_dva = buffer.dva;
	buff_desc->size = buffer.buf_len;
	buff_desc->priv_idx = buffer.idx;

	HFI_CORE_DBG_L(
		"pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx idx: %d prio: %d\n",
		(u64)buff_desc->pbuf_vaddr, buff_desc->size,
		buff_desc->priv_dva, buff_desc->priv_idx,
		buff_desc->prio_info);

exit:
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int get_device_tx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;
	struct hfi_vq_queues_data *hfi_queues;
	struct hfi_queue_buffer buffer;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		HFI_CORE_ERR("invalid client id: %d\n", client_id);
		return -EINVAL;
	}

	/* loopback client uses client 0 resources */
	client_id = HFI_CORE_CLIENT_ID_0;


	if (!drv_data || !buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	queue_hdl = get_queue_handle(hfi_queues,
		buff_desc->prio_info, false);
	if (!queue_hdl) {
		HFI_CORE_ERR(
			"q handle with prio: %d is not found for buffer: 0x%llx\n",
			buff_desc->prio_info, (u64)buff_desc);
		return -EINVAL;
	}

	/* get a buffer from tx buffer pool */
	ret = get_param_hfi_queue(queue_hdl,
		hfi_queue_param_device_buffer_queue,
		&buffer, sizeof(buffer));
	if (ret) {
		HFI_CORE_ERR("failed to get buffer, ret: %d\n", ret);
		return ret;
	}

	buff_desc->pbuf_vaddr = (void *)buffer.kva;
	buff_desc->priv_dva = buffer.dva;
	buff_desc->size = buffer.buf_len;
	buff_desc->priv_idx = buffer.idx;

	HFI_CORE_DBG_L(
		"pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx idx: %d prio: %d\n",
		(u64)buff_desc->pbuf_vaddr, buff_desc->size,
		buff_desc->priv_dva, buff_desc->priv_idx,
		buff_desc->prio_info);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int put_device_rx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	int ret = 0;
	struct hfi_core_cmds_buf_desc *buf_desc_index;
	struct hfi_vq_queues_data *hfi_queues;
	void *queue_hdl;

	HFI_CORE_DBG_H("+\n");

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		HFI_CORE_ERR("invalid client id: %d\n", client_id);
		return -EINVAL;
	}

	/* loopback client uses client 0 resources */
	client_id = HFI_CORE_CLIENT_ID_0;

	if (!drv_data || !buff_desc || !num_buff_desc ||
		!drv_data->client_data[client_id].queue_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	hfi_queues = (struct hfi_vq_queues_data *)
		drv_data->client_data[client_id].queue_info.data;

	/* queue priority ? */
	for (int i = 0; i < num_buff_desc; i++) {
		if (!buff_desc || !(*buff_desc)) {
			HFI_CORE_ERR("rx buff is null\n");
			return -EINVAL;
		}
		buf_desc_index = *buff_desc;

		queue_hdl = get_queue_handle(hfi_queues,
			buf_desc_index->prio_info, true);
		if (!queue_hdl) {
			HFI_CORE_ERR(
				"q handle with prio: %d is not found for buffer: 0x%llx\n",
				buf_desc_index->prio_info,
				(u64)buf_desc_index);
			return -EINVAL;
		}

		ret = push_device_buffers_to_queue(queue_hdl,
			(u64)buf_desc_index->pbuf_vaddr,
			buf_desc_index->priv_idx,
			buf_desc_index->size);
		if (ret) {
			HFI_CORE_ERR(
				"failed to set %d device buffer to host queue\n",
				i);
			return ret;
		}

		/* rx buffers kick off to DCP */
		ret = set_param_hfi_queue(queue_hdl, hfi_queue_kickoff, NULL, 0);
		if (ret) {
			HFI_CORE_ERR("failed to kick off %d buffer\n", i);
			return ret;
		}
		HFI_CORE_DBG_L(
			"pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx idx: %u prio: %d\n",
			(u64)buf_desc_index->pbuf_vaddr, buf_desc_index->size,
			buf_desc_index->priv_dva, buf_desc_index->priv_idx,
			buf_desc_index->prio_info);

		buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

#endif // CONFIG_DEBUG_FS
