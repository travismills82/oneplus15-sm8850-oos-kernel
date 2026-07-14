// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.​
 */

#include <linux/ktime.h>
#include "hfi_interface.h"
#include "hfi_core.h"
#include "hfi_if_abstraction.h"
#include "hfi_smmu.h"
#include "hfi_core_debug.h"
#include "hfi_queue_controller.h"
#include "hfi_ipc.h"
#include "hfi_swi.h"

#define HFI_LOWER_32_BIT_MASK                                     0xFFFFFFFF
#define HFI_UPPER_32_BIT_MASK                             0xFFFFFFFF00000000
#define ktime_compare_safe(A, B)     \
	ktime_compare(ktime_sub((A), (B)), ktime_set(0, 0))

#define HFI_GET_RES_TBL_RES_HDR_SIZE(__num_res, __size) ({                  \
	/* resource table header size */                                       \
	__size += (sizeof(struct hfi_core_resource_table_hdr));                \
	/* resource headers size */                                            \
	__size += (__num_res * sizeof(struct hfi_core_resource_hdr));          \
})

#define HFI_GET_VIRTQ_HDR_SIZE(__num_queues, __size) ({                     \
	/* virtq channel size */                                               \
	__size += (sizeof(struct hfi_channel_virtio_virtq));                   \
	/* virtq queue headers size */                                         \
	__size += (__num_queues * sizeof(struct hfi_virtio_virtq)) ;           \
})

typedef int (*hfi_res_op_type)(enum hfi_core_client_id,
	struct hfi_core_drv_data *drv_data);

enum hfi_res_ops {
	HFI_RES_CREATE     = 0x1,
	HFI_RES_POPULATE   = 0x2,
	HFI_RES_DESTROY    = 0x3,
	HFI_RES_OP_MAX     = 0x4,
};

static int hfi_create_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data);
static int hfi_populate_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data);
static int hfi_destroy_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data);

hfi_res_op_type hfi_vq_res_op[HFI_RES_OP_MAX] = {
	[HFI_RES_CREATE] = hfi_create_virtq_res_mem,
	[HFI_RES_POPULATE] = hfi_populate_virtq_res_mem,
	[HFI_RES_DESTROY] = hfi_destroy_virtq_res_mem,
};

static int allocate_and_map(struct hfi_core_drv_data *drv_data,
	struct hfi_memory_alloc_info *alloc_info, u32 size, u32 align)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!alloc_info || !size) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	alloc_info->size_allocated = ALIGN(size, align);
	/* allocate memory */
	ret = smmu_alloc_and_map_for_drv(drv_data, &alloc_info->phy_addr,
		alloc_info->size_allocated, &alloc_info->cpu_va, HFI_CORE_DMA_ALLOC_UNCACHE);
	if (ret) {
		HFI_CORE_ERR("failed to alloc, ret: %d\n", ret);
		return ret;
	}

	/* map memory */
	ret = smmu_mmap_for_fw(drv_data, alloc_info->phy_addr, &alloc_info->mapped_iova,
		alloc_info->size_allocated, HFI_CORE_MMAP_READ | HFI_CORE_MMAP_WRITE
						| HFI_CORE_MMAP_CACHE);
	if (ret) {
		HFI_CORE_ERR("failed to map to fw, ret: %d\n", ret);
		goto mmap_fail;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;

mmap_fail:
	/* unmap for drv */
	if (alloc_info->cpu_va)
		smmu_unmap_for_drv(alloc_info->cpu_va, alloc_info->size_allocated);
	alloc_info->size_allocated = 0;
	alloc_info->cpu_va = NULL;

	HFI_CORE_DBG_H("-\n");
	return ret;

}

static int unmap_res(struct hfi_core_drv_data *drv_data,
	struct hfi_memory_alloc_info *alloc_info)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!alloc_info || !alloc_info->size_allocated) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* unmap for fw */
	ret = smmu_unmmap_for_fw(drv_data, alloc_info->mapped_iova,
		alloc_info->size_allocated);
	if (ret) {
		HFI_CORE_ERR("unmap failed\n");
		return -EINVAL;
	}
	/* unmap for drv */
	if (alloc_info->cpu_va)
		smmu_unmap_for_drv(alloc_info->cpu_va, alloc_info->size_allocated);

	HFI_CORE_DBG_H("-\n");
	return ret;

}

static int hfi_create_vq_hdrs(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	u32 num_queue_hdrs_req = 0;
	struct hfi_virt_queues *vqs;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	vqs = &compat_data->hfi_res_table.virtqueues;
	alloc_info = &res_data->vitq_res.q_hdr_mem.mem;

	/* get number of virtio virtq headers required */
	for (int i = 0; i < vqs->num_queues; i++) {
		if (vqs->queue[i].type == HFI_VIRT_QUEUE_TX ||
			vqs->queue[i].type == HFI_VIRT_QUEUE_RX) {
			num_queue_hdrs_req += 1;
		} else if (vqs->queue[i].type == HFI_VIRT_QUEUE_FULL_DUP) {
			num_queue_hdrs_req += 2;
		} else {
			HFI_CORE_ERR("invalid virtq queue type: %d\n",
				vqs->queue[i].type);
			return -EINVAL;
		}
	}
	res_data->vitq_res.num_queues = vqs->num_queues;
	res_data->vitq_res.total_tx_rx_queues = num_queue_hdrs_req;

	/* TODO: below overriding for num_queue_hdrs_req */
	num_queue_hdrs_req = vqs->num_queues;
	HFI_GET_VIRTQ_HDR_SIZE(num_queue_hdrs_req, alloc_info->size_wr);

	ret = allocate_and_map(drv_data, alloc_info, alloc_info->size_wr,
		HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT);
	if (ret)
		return ret;

	HFI_CORE_DBG_INIT("vq_h: #hdrs: %d phys:0x%llx va:0x%p dva:0x%lx sz:%lu szalign:%lu\n",
		num_queue_hdrs_req, alloc_info->phy_addr, alloc_info->cpu_va,
		alloc_info->mapped_iova, alloc_info->size_wr,
		alloc_info->size_allocated);

	res_data->vitq_res.q_hdr_mem.num_hdrs = num_queue_hdrs_req;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_create_vq_buff_descs(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	u32 queue_size = 0, req_mem_size = 0;
	struct hfi_res_vq_queue_data *vq_buff_desc;
	struct hfi_virt_queues *vqs;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	vqs = &compat_data->hfi_res_table.virtqueues;

	vq_buff_desc = &res_data->vitq_res.q_mem[0];
	for (int i = 0; i < res_data->vitq_res.num_queues; i++) {
		queue_size = vqs->queue[i].tx_elements + vqs->queue[i].rx_elements;

		req_mem_size = get_queue_size_req(queue_size);
		ret = allocate_and_map(drv_data, &vq_buff_desc->buff_desc_mem, req_mem_size,
			HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT);
		if (ret)
			return ret;

		vq_buff_desc->queue_id = i;
		vq_buff_desc->q_info = vqs->queue[i];

		HFI_CORE_DBG_INIT("vq_buff_desc[%d]: phys:0x%llx va:0x%p dva:0x%lx sz:%lu[%lu]\n",
			i, vq_buff_desc->buff_desc_mem.phy_addr,
			vq_buff_desc->buff_desc_mem.cpu_va,
			vq_buff_desc->buff_desc_mem.mapped_iova,
			vq_buff_desc->buff_desc_mem.size_wr,
			vq_buff_desc->buff_desc_mem.size_allocated);

		vq_buff_desc++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_create_vq_buffers(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	u32 queue_size = 0, buf_size = 0, req_mem_size = 0;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_res_vq_queue_data *vq_buff_desc;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;


	HFI_CORE_DBG_H("+\n");

	for (int i = 0; i < res_data->vitq_res.num_queues; i++) {
		vq_buff_desc = &res_data->vitq_res.q_mem[i];
		queue_size = vq_buff_desc->q_info.tx_elements + vq_buff_desc->q_info.rx_elements;
		buf_size = max(vq_buff_desc->q_info.tx_buff_size_bytes,
			vq_buff_desc->q_info.rx_buff_size_bytes);
		req_mem_size = queue_size * buf_size;

		alloc_info = &res_data->vitq_res.q_mem[i].buff_mem;
		ret = allocate_and_map(drv_data, alloc_info, req_mem_size,
			HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT);
		if (ret)
			return ret;

		HFI_CORE_DBG_INIT("vq_buf[%d]: phys:0x%llx va:0x%p dva:0x%lx sz:%lu szalign:%lu\n",
			i, alloc_info->phy_addr, alloc_info->cpu_va,
			alloc_info->mapped_iova, alloc_info->size_wr,
			alloc_info->size_allocated);
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_create_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	ret = hfi_create_vq_hdrs(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to create virtio queue headers\n");
		return ret;
	}

	ret = hfi_create_vq_buff_descs(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to create virtq queue buffer descs\n");
		return ret;
	}

	ret = hfi_create_vq_buffers(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to create virtq queue buffers\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_create_tbl_and_res_hdrs_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	res_data->res_count = compat_data->hfi_res_table.num_res;
	if (res_data->res_count > CLIENT_RESOURCES_MAX) {
		HFI_CORE_ERR("client max resources supported: %d, needed: %d\n",
			CLIENT_RESOURCES_MAX, res_data->res_count);
		return -EINVAL;
	}

	/* calculate size */
	alloc_info = &res_data->tbl_res_hdr_mem;
	HFI_GET_RES_TBL_RES_HDR_SIZE(res_data->res_count, alloc_info->size_wr);

	ret = allocate_and_map(drv_data, alloc_info, alloc_info->size_wr,
		HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT);
	if (ret)
		return ret;

	HFI_CORE_DBG_INIT("res_table: phys:0x%llx va:0x%p dva:0x%lx sz:%lu szalign:%lu\n",
		alloc_info->phy_addr, alloc_info->cpu_va,
		alloc_info->mapped_iova, alloc_info->size_wr,
		alloc_info->size_allocated);

	HFI_CORE_DBG_H("allocated: cpu_va: 0x%llx, iova: 0x%lx, size: %zu\n",
		(u64)alloc_info->cpu_va, alloc_info->mapped_iova,
		alloc_info->size_allocated);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static void _dbg_dump_table_header(struct hfi_core_resource_table_hdr *hfi_res_tbl)
{
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table tbl_hdr ++ %p\n", hfi_res_tbl);
	HFI_CORE_DBG_INIT("DbgDcp: version: %u\n", hfi_res_tbl->version);
	HFI_CORE_DBG_INIT("DbgDcp: size: %u\n", hfi_res_tbl->size);
	HFI_CORE_DBG_INIT("DbgDcp: res_hdr_offset: %u\n", hfi_res_tbl->res_hdr_offset);
	HFI_CORE_DBG_INIT("DbgDcp: res_hdr_size: %u\n", hfi_res_tbl->res_hdr_size);
	HFI_CORE_DBG_INIT("DbgDcp: res_hdrs_num: %u\n", hfi_res_tbl->res_hdrs_num);
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table tbl_hdr --\n");
}

static void _dbg_dump_res_header(struct hfi_core_resource_hdr *hfi_res_hdr, int idx)
{
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table res_hdr[%d] ++ %p\n", idx, hfi_res_hdr);
	HFI_CORE_DBG_INIT("DbgDcp: version: %u\n", hfi_res_hdr->version);
	HFI_CORE_DBG_INIT("DbgDcp: type: %u\n", hfi_res_hdr->type);
	HFI_CORE_DBG_INIT("DbgDcp: status: %u\n", hfi_res_hdr->status);
	HFI_CORE_DBG_INIT("DbgDcp: start_addr_high: 0x%x\n", hfi_res_hdr->start_addr_high);
	HFI_CORE_DBG_INIT("DbgDcp: start_addr_low: 0x%x\n", hfi_res_hdr->start_addr_low);
	HFI_CORE_DBG_INIT("DbgDcp: size: %u\n", hfi_res_hdr->size);
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table res_hdr[%d] --\n", idx);
}

static void _dbg_dump_virtq_header(struct hfi_virtio_virtq *virtq_hdr, int idx)
{
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table virtq_hdr[%d] ++ %p\n", idx, virtq_hdr);
	HFI_CORE_DBG_INIT("DbgDcp: queue_id: %u\n", virtq_hdr->queue_id);
	HFI_CORE_DBG_INIT("DbgDcp: prio: %u\n", virtq_hdr->queue_priority);
	HFI_CORE_DBG_INIT("DbgDcp: type: %u\n", virtq_hdr->type);
	HFI_CORE_DBG_INIT("DbgDcp: queue_size: %u\n", virtq_hdr->queue_size);
	HFI_CORE_DBG_INIT("DbgDcp: addr_higher: 0x%x\n", virtq_hdr->addr_higher);
	HFI_CORE_DBG_INIT("DbgDcp: addr_higher: 0x%x\n", virtq_hdr->addr_lower);
	HFI_CORE_DBG_INIT("DbgDcp: alignment: %u\n", virtq_hdr->alignment);
	HFI_CORE_DBG_INIT("DbgDcp: size: %u\n", virtq_hdr->size);
	HFI_CORE_DBG_INIT("DbgDcp: _dbg_dump_table virtq_hdr[%d] --\n", idx);
}

static int hfi_populate_vq_hdrs(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_memory_alloc_info *vq_hdr_alloc_info, *vq_buff_desc_alloc_info;
	struct hfi_channel_virtio_virtq *channel;
	struct hfi_virtio_virtq *virtq_hdr;
	struct hfi_virt_queue_data *queue_data;
	u32 num_queues = 0;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	vq_hdr_alloc_info = &res_data->vitq_res.q_hdr_mem.mem;
	num_queues = res_data->vitq_res.num_queues;
	/* populate channel data */
	channel = (struct hfi_channel_virtio_virtq *)(vq_hdr_alloc_info->cpu_va);
	channel->dcp_device_id = compat_data->hfi_res_table.device_id;
	channel->flags = 0;
	channel->num = num_queues;

	/* populate virtq queue headers */
	virtq_hdr = (struct hfi_virtio_virtq *)
		((u8 *)vq_hdr_alloc_info->cpu_va + (sizeof(struct hfi_channel_virtio_virtq)));
	for (int i = 0; i < num_queues; i++) {
		queue_data = &res_data->vitq_res.q_mem[i].q_info;
		vq_buff_desc_alloc_info = &res_data->vitq_res.q_mem[i].buff_desc_mem;
		virtq_hdr->queue_id = res_data->vitq_res.q_mem[i].queue_id;
		virtq_hdr->queue_priority = queue_data->priority;
		virtq_hdr->type = queue_data->type;
		virtq_hdr->queue_size = queue_data->tx_elements + queue_data->rx_elements;
		virtq_hdr->addr_lower =
			(vq_buff_desc_alloc_info->mapped_iova & HFI_LOWER_32_BIT_MASK);
		virtq_hdr->addr_higher =
			(vq_buff_desc_alloc_info->mapped_iova &
				HFI_UPPER_32_BIT_MASK) >> 32;
		virtq_hdr->alignment = HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT;
		virtq_hdr->size = get_queue_size_req(virtq_hdr->queue_size);
		_dbg_dump_virtq_header(virtq_hdr, i);
		virtq_hdr++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_populate_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	ret = hfi_populate_vq_hdrs(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to create virtio queue headers\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int populate_virtq_res_hdr(struct hfi_core_resource_hdr *res_hdr,
	struct hfi_memory_alloc_info *alloc_info)
{
	if (!res_hdr || !alloc_info) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	res_hdr->type = HFI_QUEUE_VIRTIO_VIRTQ;
	res_hdr->status = HFI_RES_STATUS_ACTIVE;
	res_hdr->start_addr_high =
		(alloc_info->mapped_iova & HFI_UPPER_32_BIT_MASK) >> 32;
	res_hdr->start_addr_low =
		(alloc_info->mapped_iova & HFI_LOWER_32_BIT_MASK);
	res_hdr->size = alloc_info->size_wr;

	return 0;
}

static int hfi_populate_tbl_and_res_hdrs_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_core_resource_table_hdr *tbl_hdr;
	struct hfi_core_resource_hdr *res_hdr;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	alloc_info = &res_data->tbl_res_hdr_mem;

	/* populate table header */
	tbl_hdr = (struct hfi_core_resource_table_hdr *)(alloc_info->cpu_va);
	tbl_hdr->version = compat_data->hfi_table_version;
	tbl_hdr->size = sizeof(struct hfi_core_resource_table_hdr);
	tbl_hdr->res_hdr_offset = sizeof(struct hfi_core_resource_table_hdr);
	tbl_hdr->res_hdr_size = sizeof(struct hfi_core_resource_hdr);
	tbl_hdr->res_hdrs_num = res_data->res_count;
	_dbg_dump_table_header(tbl_hdr);

	/* populate resource headers */
	res_hdr = (struct hfi_core_resource_hdr *)
		((u8 *)alloc_info->cpu_va + tbl_hdr->res_hdr_offset);
	for (int i = 0; i < res_data->res_count; i++) {
		res_hdr->version = compat_data->hfi_header_version;

		if (compat_data->hfi_res_table.res_types[i] ==
			HFI_QUEUE_VIRTIO_VIRTQ) {
			ret = populate_virtq_res_hdr(res_hdr,
				&res_data->vitq_res.q_hdr_mem.mem);
		} else if (compat_data->hfi_res_table.res_types[i] ==
			HFI_SFR_ADDR) {
			// ret = populate_sfr_res_hdr(res_hdr,
			//	&res_data->sfr_res.q_hdr_mem.mem);
		} else {
			HFI_CORE_ERR("invalid restype: %d to populate res headers\n",
				compat_data->hfi_res_table.res_types[i]);
			ret = -EINVAL;
		}
		if (ret) {
			HFI_CORE_ERR("failed to populate res header[%d]\n", i);
			return ret;
		}
		_dbg_dump_res_header(res_hdr, i);
		res_hdr++;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_destroy_virtq_res_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	u32 num_queues = 0;

	HFI_CORE_DBG_H("+\n");

	/* unmap virtq buffers */
	num_queues = res_data->vitq_res.num_queues;
	for (int i = 0; i < num_queues; i++) {
		alloc_info = &res_data->vitq_res.q_mem[i].buff_mem;
		ret = unmap_res(drv_data, alloc_info);
		if (ret)
			return ret;
	}

	/* unmap virtq queue buffer headers */
	for (int i = 0; i < num_queues; i++) {
		alloc_info = &res_data->vitq_res.q_mem[i].buff_desc_mem;
		ret = unmap_res(drv_data, alloc_info);
		if (ret)
			return ret;
	}

	/* unmap virtq queue headers */
	alloc_info = &res_data->vitq_res.q_hdr_mem.mem;
	ret = unmap_res(drv_data, alloc_info);
	if (ret)
		return ret;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_res_mem_op(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data, enum hfi_res_ops type)
{
	int ret = 0;
	char *res_type;
	struct hfi_resource_table *res_tbl;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;
	struct hfi_core_internal_data *compat_data =
		drv_data->client_data[client_id].resource_info.internal_data;

	HFI_CORE_DBG_H("+\n");

	if (!res_data->res_count) {
		HFI_CORE_ERR("no resources available\n");
		return -EINVAL;
	}

	res_tbl = &compat_data->hfi_res_table;
	for (int i = 0; i < res_data->res_count; i++) {
		if (res_tbl->res_types[i] == HFI_QUEUE_VIRTIO_VIRTQ) {
			ret = hfi_vq_res_op[type](client_id, drv_data);
			res_type = "VIRTIO_VIRTQ";
		} else if (res_tbl->res_types[i] == HFI_SFR_ADDR) {
			// ret = hfi_sfr_res_op[type](drv_data);
			res_type = "SFR";
		} else {
			HFI_CORE_ERR(
				"invalid res type: %d to perform operation : %d\n",
				res_tbl->res_types[i], type);
			res_type = "UNKNOWN";
			ret = -EINVAL;
		}
		if (ret) {
			HFI_CORE_ERR(
				"%d operation failed for res header[%d] type: %d[%s]\n",
				type, i, res_tbl->res_types[i],
				res_type);
			return ret;
		}
	}

	return ret;
}

static int hfi_create_resource_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	/* allocate, map resource table header */
	ret = hfi_create_tbl_and_res_hdrs_mem(client_id, drv_data);
	if (ret)
		return ret;

	ret = hfi_res_mem_op(client_id, drv_data, HFI_RES_CREATE);
	if (ret)
		return ret;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_populate_resource_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	/* allocate, map and populate resource table header */
	ret = hfi_populate_tbl_and_res_hdrs_mem(client_id, drv_data);
	if (ret)
		return ret;

	ret = hfi_res_mem_op(client_id, drv_data, HFI_RES_POPULATE);
	if (ret)
		return ret;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_destroy_resource_mem(enum hfi_core_client_id client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_memory_alloc_info *alloc_info;
	struct hfi_resource_data *res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;

	HFI_CORE_DBG_H("+\n");

	ret = hfi_res_mem_op(client_id, drv_data, HFI_RES_DESTROY);
	if (ret)
		return ret;

	/* unmap resource table and resource headers */
	alloc_info = &res_data->tbl_res_hdr_mem;
	ret = unmap_res(drv_data, alloc_info);
	if (ret)
		return ret;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int init_resources(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_resource_data *res_data;
	enum hfi_core_client_id client = HFI_CORE_CLIENT_ID_0;

	HFI_CORE_DBG_H("+\n");

	if (client >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client);
		return -EINVAL;
	}

	if (!drv_data ||
		!drv_data->client_data[client].resource_info.internal_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	res_data = kzalloc(sizeof(struct hfi_resource_data), GFP_KERNEL);
	if (!res_data) {
		HFI_CORE_ERR("failed to allocate resource memory\n");
		return -ENOMEM;
	}
	drv_data->client_data[client].resource_info.res_data_mem =
		(void *)res_data;

	/* allocate and map all required resources */
	ret = hfi_create_resource_mem(client, drv_data);
	if (ret)
		goto destroy;

	/* populate all resources */
	ret = hfi_populate_resource_mem(client, drv_data);
	if (ret)
		goto destroy;


	ret = init_queues(client, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to init queues, ret: %d\n", ret);
		goto destroy;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;

destroy:
	deinit_resources(drv_data);
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int deinit_resources(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	enum hfi_core_client_id client = HFI_CORE_CLIENT_ID_0;

	HFI_CORE_DBG_H("+\n");

	if (client >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client);
		return -EINVAL;
	}

	if (!drv_data ||
		!drv_data->client_data[client].resource_info.res_data_mem) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	ret = deinit_queues(client, drv_data);
	if (ret)
		HFI_CORE_ERR("failed to deinit queues, ret: %d\n", ret);

	/* unmap all resources */
	ret = hfi_destroy_resource_mem(client, drv_data);
	if (ret)
		HFI_CORE_ERR("failed to destroy resource mem, ret: %d\n",
			ret);

	kfree(drv_data->client_data[client].resource_info.res_data_mem);
	drv_data->client_data[client].resource_info.res_data_mem = NULL;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

#define IPC_NOTIFICATION_TIMEOUT                   100000

static int hfi_core_wait_event(struct client_data *client_data, void *wait_on)
{
	int ret = 0;
	atomic_t *notified = (atomic_t *)wait_on;
	wait_queue_head_t *queue =
		(wait_queue_head_t *)client_data->wait_queue;
	s64 wait_time_jiffies = msecs_to_jiffies(IPC_NOTIFICATION_TIMEOUT);
	ktime_t cur_ktime;
	ktime_t exp_ktime = ktime_add_ms(ktime_get(),
		IPC_NOTIFICATION_TIMEOUT);

	HFI_CORE_DBG_H("+\n");

	do {
		ret = wait_event_timeout(*queue,
			atomic_read(notified) == true, wait_time_jiffies);
		cur_ktime = ktime_get();
	} while ((atomic_read(notified) != true) && (ret == 0) &&
		(ktime_compare_safe(exp_ktime, cur_ktime) > 0));

	if (atomic_read(notified) == true) {
		HFI_CORE_DBG_H("notified\n");
		ret = 0;
	} else {
		ret = -ETIMEDOUT;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_core_enable_dcp_clock(u32 client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct client_data *clientd = &drv_data->client_data[client_id];
	wait_queue_head_t *queue =
		(wait_queue_head_t *)clientd->wait_queue;

	HFI_CORE_DBG_H("+\n");

	init_waitqueue_head(queue);
#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (hfi_core_loop_back_mode_enable) {
		ret = trigger_ipc(client_id, drv_data,
			HFI_IPC_EVENT_QUEUE_NOTIFY);
	} else {
		ret = trigger_ipc(client_id, drv_data,
			HFI_IPC_EVENT_POWER_NOTIFY);
	}
#else
	ret = trigger_ipc(client_id, drv_data, HFI_IPC_EVENT_POWER_NOTIFY);
#endif // CONFIG_DEBUG_FS
	if (ret) {
		HFI_CORE_ERR("failed to trigger IPC power notification\n");
		return ret;
	}

	ret = hfi_core_wait_event(clientd, clientd->power_event);
	if (ret) {
		HFI_CORE_ERR("msg ACK not received for swi reg access\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_core_setup_swi_registers(u32 client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct client_data *clientd = &drv_data->client_data[client_id];

	HFI_CORE_DBG_H("+\n");

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (!hfi_core_loop_back_mode_enable) {
		ret = swi_setup_resources(client_id, drv_data);
		if (ret) {
			HFI_CORE_ERR("failed to setup swi register\n");
			return ret;
		}
	}
#else
	ret = swi_setup_resources(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to setup swi register\n");
		return ret;
	}
#endif // CONFIG_DEBUG_FS

	ret = trigger_ipc(client_id, drv_data, HFI_IPC_EVENT_QUEUE_NOTIFY);
	if (ret) {
		HFI_CORE_ERR("failed trigger IPC queue notification\n");
		return ret;
	}

	ret = hfi_core_wait_event(clientd, clientd->xfer_event);
	if (ret) {
		HFI_CORE_ERR("msg ACK not received for swi reg access\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_core_disable_dcp_clock(u32 client_id,
	struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	ret = swi_reg_power_off(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("Failed to set swi POWER_OFF bit\n");
		return ret;
	}

	ret = trigger_ipc(client_id, drv_data, HFI_IPC_EVENT_POWER_NOTIFY);
	if (ret) {
		HFI_CORE_ERR("trigger ipc failed\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int power_init(u32 client_id, struct hfi_core_drv_data *drv_data)
{
	int ret;
	atomic_t *power, *xfer;
	wait_queue_head_t *queue;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	power = kzalloc(sizeof(*power), GFP_KERNEL);
	if (!power) {
		HFI_CORE_ERR("failed to allocate power event memory\n");
		return -ENOMEM;
	}
	drv_data->client_data[client_id].power_event = (void *)power;

	xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if (!xfer) {
		HFI_CORE_ERR("failed to allocate xfer event memory\n");
		ret = -ENOMEM;
		goto error_xfer;
	}
	drv_data->client_data[client_id].xfer_event = (void *)xfer;

	queue = kzalloc(sizeof(*queue), GFP_KERNEL);
	if (!queue) {
		HFI_CORE_ERR("failed to allocate queue memory\n");
		ret = -ENOMEM;
		goto error_queue;
	}
	drv_data->client_data[client_id].wait_queue = (void *)queue;

	ret = hfi_core_enable_dcp_clock(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed with %d\n", ret);
		goto error_exit;
	}

	ret = hfi_core_setup_swi_registers(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed with %d\n", ret);
		goto error_exit;
	}

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (!hfi_core_loop_back_mode_enable) {
		ret = hfi_core_disable_dcp_clock(client_id, drv_data);
		if (ret) {
			HFI_CORE_ERR("failed with %d\n", ret);
			goto error_exit;
		}
	}
#else
	ret = hfi_core_disable_dcp_clock(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed with %d\n", ret);
		goto error_exit;
	}
#endif // CONFIG_DEBUG_FS

	HFI_CORE_DBG_H("-\n");
	return ret;

error_exit:
	kfree(queue);
	drv_data->client_data[client_id].wait_queue = NULL;
error_queue:
	kfree(xfer);
	drv_data->client_data[client_id].xfer_event = NULL;
error_xfer:
	kfree(power);
	drv_data->client_data[client_id].power_event = NULL;
	return ret;
}

int power_deinit(u32 client_id, struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (drv_data->client_data[client_id].wait_queue) {
		kfree(drv_data->client_data[client_id].wait_queue);
		drv_data->client_data[client_id].wait_queue = NULL;
	}
	if (drv_data->client_data[client_id].xfer_event) {
		kfree(drv_data->client_data[client_id].xfer_event);
		drv_data->client_data[client_id].xfer_event = NULL;
	}
	if (drv_data->client_data[client_id].power_event) {
		kfree(drv_data->client_data[client_id].power_event);
		drv_data->client_data[client_id].power_event = NULL;
	}

	return ret;
}

int power_notification(u32 client_id, struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	atomic_t *power;
	struct client_data *client_data;
	wait_queue_head_t *queue;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	client_data = &drv_data->client_data[client_id];
	queue = (wait_queue_head_t *)client_data->wait_queue;

	power = (atomic_t *)client_data->power_event;
	atomic_or(1, power);
	wake_up_all(queue);

	HFI_CORE_DBG_H("-\n");
	return ret;
}
