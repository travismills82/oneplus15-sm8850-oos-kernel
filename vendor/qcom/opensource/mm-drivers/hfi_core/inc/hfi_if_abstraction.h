/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_IF_ABSTRACTION_H__
#define __HFI_IF_ABSTRACTION_H__

#include "hfi_core.h"

enum hfi_core_resource_status {
	HFI_RES_STATUS_INACTIVE      = 0x0,
	HFI_RES_STATUS_ACTIVE        = 0x1,
};

struct hfi_core_resource_table_hdr {
	u32 version;
	u32 size;
	u32 res_hdr_offset;
	u32 res_hdr_size;
	u32 res_hdrs_num;
};

struct hfi_core_resource_hdr {
	u32 version;
	enum hfi_core_resource_type type;
	enum hfi_core_resource_status status;
	u32 start_addr_high;
	u32 start_addr_low;
	u32 size;
};

struct hfi_virtio_virtq {
	u32 queue_id;
	u32 queue_priority;
	u32 type;
	u32 queue_size;
	u32 addr_lower;
	u32 addr_higher;
	u32 alignment;
	u32 size;
	u32 padding[8];
};

struct hfi_channel_virtio_virtq {
	u32 dcp_device_id;
	u32 flags;
	u32 num;
};

struct hfi_vq_hdr_data {
	u32 num_hdrs;
	struct hfi_memory_alloc_info mem;
};

struct hfi_res_vq_queue_data {
	u32 queue_id;
	struct hfi_virt_queue_data q_info;
	struct hfi_memory_alloc_info buff_desc_mem;
	struct hfi_memory_alloc_info buff_mem;
};

struct hfi_vq_res_data {
	u32 num_queues;
	u32 total_tx_rx_queues;
	struct hfi_vq_hdr_data q_hdr_mem;
	struct hfi_res_vq_queue_data q_mem[MAX_NUM_VIRTQ];
};

struct hfi_resource_data {
	u32 res_count;
	struct hfi_memory_alloc_info tbl_res_hdr_mem;
	struct hfi_vq_res_data vitq_res;
	/* sfr_res */
};

/**
 * init_resources() - resources initialization.
 *
 * This call initializes the queues required from the compact data
 * for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int init_resources(struct hfi_core_drv_data *drv_data);

/**
 * deinit_resources() - resources deinitialization.
 *
 * This call deinitializes the queues required from the compact data
 * for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int deinit_resources(struct hfi_core_drv_data *drv_data);

/**
 * power_init() - power on sequence
 *
 * This call trigger power on signal to remote processor
 *
 * Return: 0 on success or negative errno
 */
int power_init(u32 client_id, struct hfi_core_drv_data *drv_data);

/**
 * power_deinit() - free all memory allocated for power on sequence
 *
 * This call frees all memory allocated for given client power on sequence.
 *
 * Return: 0 on success or negative errno
 */
int power_deinit(u32 client_id, struct hfi_core_drv_data *drv_data);

/**
 * power_notification() - power notification callback
 *
 * This function will turn power on/off disp_cc clocks upon request
 * received.
 */
int power_notification(uint32_t client_id, struct hfi_core_drv_data *drv_data);

#endif // __HFI_IF_ABSTRACTION_H__
