/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_INTERFACE_H__
#define __HFI_INTERFACE_H__
#include <linux/types.h>
#include <linux/bits.h>

/**
 * HFI_CORE_SET_FLAGS_TRIGGER_IPC - Trigger IPC flag.
 *
 * HFI core should trigger the IPCC irq and send the Message.
 */
#define HFI_CORE_SET_FLAGS_TRIGGER_IPC	        0x1
#define HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT       SZ_4K

/**
 * @brief Enumerate the client index for host/device.
 *
 * This enumeration defines the indices for clients in an HFI environment,
 * distinguishing between different host or device clients.
 *
 * @HFI_CORE_CLIENT_ID_0: HLOS for Host or Device0 for Client.
 * @HFI_CORE_CLIENT_ID_1: TVM for Host or Device1 for Client.
 * @HFI_CORE_CLIENT_ID_LOOPBACK_DCP: Debugfs DCP loopback Client.
 * @HFI_CORE_CLIENT_ID_MAX: Maximum number of clients.
 */
enum hfi_core_client_id {
	HFI_CORE_CLIENT_ID_0               = 0x1,
	HFI_CORE_CLIENT_ID_1               = 0x2,
	/* loopback dcp */
	HFI_CORE_CLIENT_ID_LOOPBACK_DCP    = 0x3,
	HFI_CORE_CLIENT_ID_MAX             = 0x4,
};

/**
 * @brief Enumerate the core types for HFI.
 *
 * This enumeration lists the types of cores that can be used with HFI,
 * distinguishing between host and device types.
 *
 * @HFI_CORE_HOST: HFI core of host type (HLOS, TVM, etc.).
 * @HFI_CORE_DEVICE: HFI core of device type (DCP FW).
 * @HFI_CORE_TYPE_MAX: Maximum number of core types.
 */
enum hfi_core_type {
	HFI_CORE_HOST                  = 0x0,
	HFI_CORE_DEVICE,
	HFI_CORE_TYPE_MAX,
};

/**
 * @brief Enumerate the DMA allocation type for memory.
 *
 * This enumeration lists the DMA allocation type for
 * the memory getting allocated.
 *
 * @HFI_CORE_DMA_ALLOC_UNCACHE: uncached memory allocation.
 * @HFI_CORE_DMA_ALLOC_CACHE: cached memory allocation.
 */
enum hfi_core_dma_alloc_type {
	HFI_CORE_DMA_ALLOC_UNCACHE             = 0x1,
	HFI_CORE_DMA_ALLOC_CACHE               = 0x2,
};

/**
 * @brief Enumerate the memory map access type.
 *
 * This enumeration lists the types of access
 * requested for the memory map.
 *
 * @HFI_CORE_MMAP_READ: read access for the memory map.
 * @HFI_CORE_MMAP_WRITE: write access for the memory map.
 */
enum hfi_core_mmap_flags {
	HFI_CORE_MMAP_READ                      = 0x1,
	HFI_CORE_MMAP_WRITE                     = 0x2,
	HFI_CORE_MMAP_CACHE                     = 0x4,
};

/**
 * @brief Buffer priority hint types.
 *
 * This enumeration defines the priority levels for buffers within the
 * HFI Core, indicating the urgency of the payload expected.
 *
 * @HFI_CORE_PRIO_0: Highest priority payload expected.
 * @HFI_CORE_PRIO_1: Payload with priority less than HFI_CORE_PRIO_0.
 * @HFI_CORE_PRIO_2: Payload with priority less than HFI_CORE_PRIO_1.
 * @HFI_CORE_PRIO_3: Payload with priority less than HFI_CORE_PRIO_2.
 * @HFI_CORE_PRIO_MAX: Maximum number of priority levels.
 */
enum hfi_core_priority_type {
	HFI_CORE_PRIO_0                = 0x0,
	HFI_CORE_PRIO_1,
	HFI_CORE_PRIO_2,
	HFI_CORE_PRIO_3,
	HFI_CORE_PRIO_MAX,
};

/**
 * @brief HFI Core session structure.
 *
 * @client_id: Client Id representing the Host Id when the HFI Core is
 *             set up as a Host and the Device id when the core is used as a
 *             Client.
 * @priv: Private field holding the core type for the HFI session, used for
 *        off-target testing.
 */
struct hfi_core_session {
	u32 client_id;
	void *priv;
};

/**
 * @brief Commands buffer descriptor.
 *
 * This structure is used to describe the buffer that holds commands for HFI
 * core sessions. It includes a pointer to the buffer's virtual address,
 * the buffer's size, priority information, a private field for queue indexing,
 * and a flag to determine the buffer type.
 *
 * @pbuf_vaddr: Pointer to the commands buffer virtual address accessible for
 *              the Client.
 *              (i.e. for Host-Clients, it provides the Virtual Address
 *              accessible to the Host, for Device-Clients, it provides the
 *              Virtual Address accessible to the Device).
 * @priv_dva: Commands buffer device address (Private to HFI Core)
 * @size: Total size in bytes of the commands buffer.
 * @prio_info: Priority hint info. See hfi_core_priority_type definition.
 * @priv_idx: Index of the queue used. (Private to HFI Core)
 * @flag: Flag to determine if the buffer is for tx/rx type.
 */
struct hfi_core_cmds_buf_desc {
	void *pbuf_vaddr;
	u64 priv_dva;
	size_t size;
	enum hfi_core_priority_type prio_info;
	u32 priv_idx;
	u32 flag;
};

/**
 * @brief Callback function received by the client when an HFI message is
 *        received
 *
 * @hfi_session: handle to the session.
 * @cb_data: pointer to the opaque pointer registered with the callback.
 * @flags: reserved for flags.
 */
typedef	int (*hfi_core_cb)(struct hfi_core_session *hfi_session,
	const void *cb_data, u32 flags);

/**
 * @brief HFI Core callback operations.
 *
 * This structure defines the callback operations for an HFI core session.
 * It includes a function pointer for handling notifications and a pointer for
 * callback data.
 *
 * @hfi_core_cb_fn: Notification function called when HFI receives a
 *                  notification for the client.
 * @cb_data: Pointer returned during the receive callback.
 */
struct hfi_core_cb_ops {
	hfi_core_cb hfi_cb_fn;
	void *cb_data;
};

/**
 * @brief Parameters to open an HFI core session.
 *
 * This structure is used to pass parameters when opening an HFI core session.
 * It includes the client ID, which can represent either the Host ID or the
 * Device ID, callbacks for channel operations, and the core type of the HFI
 * session.
 *
 * @client_id: Client Id representing the Host Id when the HFI Core is
 *             setup as a Host and the Device id when core is used as a Client.
 * @ops: Callbacks to the caller for each of the channel ops.
 * @core_type: Core type for the given HFI core session.
 */
struct hfi_core_open_params {
	u32 client_id;
	struct hfi_core_cb_ops *ops;
	enum hfi_core_type core_type;
};

/**
 * @brief Memory allocation information.
 *
 * This structure is used to pass memory allocation information to allocate
 * shared dynamic memory.
 *
 * @phy_addr: Stores the physical address of memory allocated
 * @cpu_va: Stores the virtual/drivers access address of memory
 * @mapped_iova: Stores the FW access address of memory
 * @size_allocated: stores the aligned size(in bytes) of actual memory allocated
 */
struct hfi_core_mem_alloc_info {
	phys_addr_t phy_addr;
	void *__iomem cpu_va;
	unsigned long mapped_iova;
	size_t size_allocated;
};

#if IS_ENABLED(CONFIG_QTI_HFI_CORE)
/**
 * hfi_core_open_session() - Open Handle to HFI core.
 *
 * @params [in]: parameters required to initialize the HFI core.
 *
 * This call initializes the required settings for the Host/Device Client
 * communication channel, where: For Clients acting as Hosts, this opens
 * the communication channel with the Device. For Clients acting as
 * Devices, this setup the infrastructure to wait for the communication
 * channel to be opened by the Host.
 *
 * Return: Handle to the hfi core object that must be used for further calls
 *         or negative errno in case of error.
 */
struct hfi_core_session *hfi_core_open_session(
	struct hfi_core_open_params *params);

/**
 * hfi_core_close_session() - Close handle to hfi core.
 *
 * @hfi_session [in]: Hfi core session, this was returned during
 *               'hfi_core_open_session'.
 *
 * Only close the core when no more HFI queue operations are
 * expected.
 *
 * Return: 0 on success or negative errno.
 */
int hfi_core_close_session(struct hfi_core_session *hfi_session);

/**
 * hfi_core_get_info() - Get the core info like number of queues and size.
 * @hfi_session [in]: HFI core session, this was returned during
 *                   'hfi_core_open'.
 * @num_queues [out]: Number of queues for the session.
 * @queue_size [out]: Size of each queue for the session.
 *
 * This API gets the number of queues and size of each queue for the session.
 *
 * Return: 0 on success or negative errno.
 */
int hfi_core_get_info(struct hfi_core_session *hfi_session, u32 *num_queues,
	u32 *queue_size);

/**
 * hfi_core_cmds_tx_buf_get() - Get a hfi_cmds buffer descriptor.
 *
 * @hfi_session [in]: HFI core session, this was returned during
 *                   'hfi_core_open'.
 * @buff_desc: Info about the requested buffer.
 *
 * Input/output params of buff_desc:
 *	pbuf_vaddr: [OUT] vq buffer pointer.
 *	size: [OUT] vq buffer size
 *	prio_info: [IN] PRIO
 *	priv: [OUT] index of queue
 *	flags: [NA]
 *
 * This API gets the Buffer Descriptor where the data can be updated to be,
 * sent. User must not exceed the size within the descriptor, and instead query
 * another buffer if more data must be sent.
 * Caller should also provide within the params, the info with the hints of the
 * size of the buffer and its priority.
 *
 * Return: 0 on success or negative errno.
 */
int hfi_core_cmds_tx_buf_get(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc);

/**
 * hfi_core_cmds_rx_buf_get() - Get a hfi_cmds buffer descriptor.
 *
 * @hfi_session [in]: Hfi core session, this was returned during
 *                   'hfi_core_open'.
 * @buff_desc [out]: Info about the requested buffer.
 *
 * This API gets the Buffer Descriptor where the data has been updated to be,
 * read. Caller can get within the params, the info with the hints of the
 * size of the buffer and its priority.
 *
 * Return: 0 on success or negative errno in case of error.
 */
int hfi_core_cmds_rx_buf_get(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc);

/**
 * hfi_core_cmds_tx_buf_send() - Send the hfi tx buffer.
 *
 * @hfi_session [in]: HFI core session, this was returned during
 *                   'hfi_core_open'.
 * @buff_desc [in]: Array of buffer descriptors to be set. For chaining
 *                  multiple buffers, an array with all the buffer
 *                  descriptors must be passed here.
 * @num_buff_desc [in]: Number of buffer descriptors in the buff_desc array
 * @flags [in]: Flags to be set when the HFI commands buffer is sent.
 *              For more info, see: HFI_CORE_SET_FLAGS_*.
 *
 *
 * This API sends a Tx buffer into the transport layer (which makes it
 * unavailable for the caller after this point).Note that for chaining of
 * multiple buffers, the list of buffers must be passed within the params.
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_cmds_tx_buf_send(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc,
	u32 flags);

/**
 * hfi_core_release_rx_buffer() - Release the hfi rx buffer.
 *
 * @hfi_session [in]: HFI core session, this was returned during
 *                   'hfi_core_open'.
 * @buff_desc [in]: Array of buffer descriptors to be set. For chaining
 *                  multiple buffers, an array with all the buffer
 *                  descriptors must be passed here.
 * @num_buff_desc [in]: Number of buffer descriptors in the buff_desc array.
 *
 * This API releases the Rx buffer and puts the buffer into the used list.
 * Note that for chaining of multiple buffers, the list of buffers must be
 * passed within the params.
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_release_rx_buffer(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * hfi_core_release_tx_buffer - Release the hfi tx buffer.
 *
 * @hfi_session [in]: HFI core session, this was returned during
 *                   'hfi_core_open'.
 * @buff_desc [in]: Array of buffer descriptors to be set. For chaining
 *                  multiple buffers, an array with all the buffer
 *                  descriptors must be passed here.
 * @num_buff_desc [in]: Number of buffer descriptors in the buff_desc array.
 *
 * This API releases the Tx buffer and puts the buffer into the available list
 * for re-use. Note that for chaining of multiple buffers, the list of buffers
 * must be passed within the params.
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_release_tx_buffer(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * hfi_core_allocate_shared_mem() - Allocate and map memory
 * for drivers and FW access.
 *
 * @alloc_info [out]: info about the allocated shared memory
 * @size       [in]: size(in bytes) of the memory to allocate and map
 * @type       [in]: specifies allocation type cached/uncached
 * @flags      [in]: bitmask flags to set read, write mmap
 *
 * This API allocates shared dynamic memory of requested size and maps it for
 * drivers access. The output of this API includes the physical and virtual
 * addresses of allocated memory, and maps it for drivers and firmware access.
 *
 * Return: 0 on success or negative errno.
 */
int hfi_core_allocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info,
	u32 size, enum hfi_core_dma_alloc_type type, u32 flags);

/**
 * hfi_core_deallocate_shared_mem() - Unmap memory for drivers
 * and firmware, and deallocates the memory.
 *
 * @alloc_info [in]: info about the allocated shared memory
 *
 * This API unmaps the memory in the FW and HLOS and frees the dynamic memory allocated.
 * User must call this API only when it is guaranteed that FW and HLOS won't access the
 * memory freed anymore.
 *
 * Return: 0 on success or negative errno.
 */
int hfi_core_deallocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info);

#else // CONFIG_QTI_HFI_CORE

static inline struct hfi_core_session *hfi_core_open_session(
	struct hfi_core_open_params *params)
{
	return NULL;
}

static inline int hfi_core_close_session(
	struct hfi_core_session *hfi_session)
{
	return -EINVAL;
}

static inline int hfi_core_get_info(
	struct hfi_core_session *hfi_session, u32 *num_queues, u32 *queue_size)
{
	return -EINVAL;
}

static inline int hfi_core_cmds_tx_buf_get(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc)
{
	return -EINVAL;
}

static inline int hfi_core_cmds_tx_buf_send(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc,
	u32 num_buff_desc, u32 flags)
{
	return -EINVAL;
}

static inline int hfi_core_release_rx_buffer(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc,
	u32 num_buff_desc)
{
	return -EINVAL;
}

static inline int hfi_core_release_tx_buffer(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc,
	u32 num_buff_desc)
{
	return -EINVAL;
}

static inline int hfi_core_cmds_tx_device_buf_send(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc,
	u32 num_buff_desc, u32 flags)
{
	return -EINVAL;
}

static inline int hfi_core_cmds_rx_buf_get(
	struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc)
{
	return -EINVAL;
}

static inline int hfi_core_allocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info,
	u32 size, enum hfi_core_dma_alloc_type type, u32 flags)
{
	return -EINVAL;
}

static inline int hfi_core_deallocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info)
{
	return -EINVAL;
}

#endif // CONFIG_QTI_HFI_CORE
#endif // __HFI_INTERFACE_H__
