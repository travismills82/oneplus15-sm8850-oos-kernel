/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_CORE_H__
#define __HFI_CORE_H__

#include <linux/device.h>
#include "hfi_interface.h"

#define CLIENT_RESOURCES_MAX                                                  2
#define MAX_NUM_VIRTQ                                                         4
#define HFI_CORE_EVENT_MAX_DATA                                              12
/* event dump data includes one "32-bit" element + "|" separator */
#define HFI_CORE_MAX_DATA_PER_EVENT_DUMP          (HFI_CORE_EVENT_MAX_DATA * 9)

#define HFI_CORE_EVT_MSG               "[%d][t:%llu][evt:0x%llx] data[%d]:%s\n"

/**
 * HFI_CORE_MAX_TRACE_EVENTS:
 * Maximum number of hfi core dcp debug events
 */
#define HFI_CORE_MAX_TRACE_EVENTS                                    (4 * 1000)

enum hfi_core_ipc_type {
	HFI_IPC_TYPE_MBOX = 1,
};

struct hfi_core_ipc_info {
	enum hfi_core_ipc_type type;
	void *data; // struct *hfi_mbox_info;
	uint32_t size;
};

struct hfi_core_smmu_info {
	void *data;
	u32 size;
};

struct hfi_core_swi_info {
	phys_addr_t reg_base;
	void __iomem *io_mem;
	u32 size;
};

struct hfi_core_mdss_info {
	phys_addr_t reg_base;
	u32 size;
	unsigned long iova;
};

struct hfi_core_queue_info {
	void *data;
	u32 size;
};

struct hfi_core_debug_info {
	void *data;
	u32 size;
};

enum hfi_virtqueue_type {
	HFI_VIRT_QUEUE_TX        = 1,
	HFI_VIRT_QUEUE_RX        = 2,
	HFI_VIRT_QUEUE_FULL_DUP  = 3,
};

enum hfi_addr_type {
	HFI_ADDR_DYNAMIC_ALLOC  = 1,
};

enum hfi_hosts {
	HFI_HOST_PRIMARY_VM = 1,
};

enum hfi_core_resource_type {
	HFI_QUEUE_VIRTIO_VIRTQ       = 0x1,
	HFI_SFR_ADDR                 = 0x2,
};

struct hfi_virt_queue_data {
	enum hfi_virtqueue_type type;
	u32 priority;
	u32 tx_elements;
	u32 rx_elements;
	u32 tx_buff_size_bytes;
	u32 rx_buff_size_bytes;
};

struct hfi_virt_queues {
	u32 num_queues;
	struct hfi_virt_queue_data queue[MAX_NUM_VIRTQ];
};

struct hfi_resource_table {
	u32 device_id;
	u32 num_res;
	enum hfi_core_resource_type res_types[CLIENT_RESOURCES_MAX];
	struct hfi_virt_queues virtqueues;
};

struct hfi_core_internal_data {
	enum hfi_hosts host_id;
	u32 hfi_table_version;
	u32 hfi_header_version;
	struct hfi_resource_table hfi_res_table;
};

struct hfi_core_resource_info {
	void *res_data_mem;
	struct hfi_core_internal_data *internal_data;
	bool swi_reg_write;
	bool resource_ready;
	unsigned long dcp_map_addr;
	u32 dcp_map_addr_max_size;
};

struct hfi_memory_alloc_info {
	phys_addr_t phy_addr;
	void *__iomem cpu_va;
	unsigned long mapped_iova;
	size_t size_allocated;
	size_t size_wr;
};

/* struct that holds client info like callback functions, data */
struct client_data {
	struct hfi_core_drv_data *drv_data;
	enum hfi_core_type core_type;
	struct hfi_core_session *session;
	hfi_core_cb cb_fn;
	void *cb_data;
	/* ipcc info */
	struct hfi_core_ipc_info ipc_info;
	/* swi data per device*/
	struct hfi_core_swi_info swi_info;
	/* resource config info and shmem info per device*/
	struct hfi_core_resource_info resource_info;
	/* queue data */
	struct hfi_core_queue_info queue_info;
	bool res_table_initialized;
	void *power_event;
	void *xfer_event;
	void *wait_queue;
};

struct hfi_core_trace_event {
	u64 time;
	u32 data_cnt;
	u32 data[HFI_CORE_EVENT_MAX_DATA];
};

 /* Internal struct that holds data required by the hfi core driver */
struct hfi_core_drv_data {
	/* device handle */
	void *dev;
	enum hfi_core_type core_type;
	struct client_data client_data[HFI_CORE_CLIENT_ID_MAX];
	u32 num_clients;

	/*smmu data */
	struct hfi_core_smmu_info smmu_info;
	/* swi data */
	struct hfi_core_swi_info swi_info;
	/* mdss data */
	struct hfi_core_mdss_info mdss_info;
	/* debug info */
	struct hfi_core_debug_info debug_info;
	/* fw trace info */
	struct hfi_memory_alloc_info *fw_trace_mem;
};

/**
 * hfi_core_init() - HFI core initialization.
 *
 * This call initializes the communication channel required for the device
 * to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_init(struct hfi_core_drv_data *init_drv_data);

/**
 * hfi_core_deinit() - HFI core deinitialization.
 *
 * This call deinitializes the communication channel required for the device
 * to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_deinit(struct hfi_core_drv_data *drv_data);

#endif // __HFI_CORE_H__
