/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_ADAPTER_H_
#define _HFI_ADAPTER_H_

#include <linux/idr.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#if IS_ENABLED(CONFIG_MDSS_HFI_ADAPTER)
#include "hfi_pack_unpack_common.h"
#if IS_ENABLED(CONFIG_QTI_HFI_CORE)
#include "hfi_interface.h"
#endif
#include "hfi_packer.h"
#include "hfi_unpacker.h"
#endif

/**
 * HFI_PACK_KEY - Pack given property id, version and dsize to u32 key.
 * @key: Packed key with given property id, version and dsize
 * @property_id: Property ID to pack
 * @version : Property version to pack
 * @dsize: Property value size in dwords
 */
#define HFI_PACKKEY(property_id, version, dsize) \
	(property_id | (version << 20) | (dsize << 24))

#define HFI_ADAPTER_WORK_QUEUE_SIZE 4

/**
 * struct callback_work - Structure for containing work queue items
 * @work: kthread_work instance
 * @host: pointer to adapter module instance
 * @index: index of kthread queue position
 */
struct callback_work {
	struct kthread_work work;
	struct hfi_adapter_t *host;
	u32 index;
};

/**
 * struct hfi_adapter_t - Structure for defining Adapter Module instance handle
 * @sde_or_vm_instance: index of VM owning adapter
 * @cb_ops: callback ops supplied to HFI core driver for receiving IRQ
 * @session: hfi_core_session handle for interfacing with HFI Core driver
 * @cb_work: Callback work structure
 * @cb_worker: Callback worker structure
 * @cb_worker_thread: Callback worker thread
 * @client_ids: ID allocation for client ID's
 * @pool: Pointer to hfi_buffer_pool struct
 * @packet_id_lock: Lock for packet id
 * @hfi_adapter_cmd_buf_list_lock:Lock for cmd_buf list
 */
struct hfi_adapter_t {
	u32  sde_or_vm_instance;
	struct list_head client_list;
#if IS_ENABLED(CONFIG_MDSS_HFI_ADAPTER)
	struct hfi_core_cb_ops *cb_ops;
	struct hfi_core_session *session;  /* handle to hfi core device */
#endif
	struct callback_work cb_work[HFI_ADAPTER_WORK_QUEUE_SIZE];
	struct kthread_worker cb_worker;
	struct task_struct *cb_worker_thread;
	struct idr client_ids;
	struct hfi_buffer_pool *pool;
	spinlock_t packet_id_lock;
	struct mutex hfi_adapter_cmd_buf_list_lock;
};

/**
 * enum hfi_cmdbuf_type - enumeration of command buffer usage classification
 * @HFI_CMDBUF_TYPE_ATOMIC_CHECK: buffer used for atomic validation with FW
 * @HFI_CMDBUF_TYPE_ATOMIC_COMMIT: buffer used for submitting frame config to Firmware
 * @HFI_CMDBUF_TYPE_DISPLAY_INFO_NO_BLOCK: buffer used for display info non-blocking
 * @HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING: buffer used for display info blocking with listener
 * @HFI_CMDBUF_TYPE_GET_DEBUG_DATA: buffer used for getting debug data info blocking
 * @HFI_CMDBUF_TYPE_DEVICE_INFO: buffer used for set or get of device info
 * @HFI_CMDBUF_TYPE_MAX: Maximum value for hfi_cmdbuf_type
 */
enum hfi_cmdbuf_type {
	HFI_CMDBUF_TYPE_ATOMIC_CHECK,
	HFI_CMDBUF_TYPE_ATOMIC_COMMIT,
	HFI_CMDBUF_TYPE_DISPLAY_INFO_NO_BLOCK,
	HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING,
	HFI_CMDBUF_TYPE_GET_DEBUG_DATA,
	HFI_CMDBUF_TYPE_DEVICE_INFO,
	HFI_CMDBUF_TYPE_MAX,
};

/**
 * enum hfi_payload_type - enumeration of packet payload type
 * @HFI_PAYLOAD_TYPE_U32_ARRAY: indicates payload to be treated as u32
 * @HFI_PAYLOAD_TYPE_U32_ARRAY: indicates payload to be treated as u32 array
 * @HFI_PAYLOAD_TYPE_U64: indicates payload to be treated as u64
 * @HFI_PAYLOAD_TYPE_U64_ARRAY: indicates payload to be treated as u64 array
 * @HFI_PAYLOAD_TYPE_MAX: maximum value for packet payload type
 */
enum hfi_payload_type {
	HFI_PAYLOAD_TYPE_NONE,
	HFI_PAYLOAD_TYPE_U32,
	HFI_PAYLOAD_TYPE_U32_ARRAY,
	HFI_PAYLOAD_TYPE_U64,
	HFI_PAYLOAD_TYPE_U64_ARRAY,
	HFI_PAYLOAD_TYPE_MAX,
};

/**
 * enum hfi_host_flags - enumeration of flags while sending a command buffer
 * @HFI_HOST_FLAGS_NONE: default enum value
 * @HFI_HOST_FLAGS_INTR_REQUIRED: enum indicating to send the command buffer synchrnously
 * @HFI_HOST_FLAGS_RESPONSE_REQUIRED: enum indicating to send the command buffer & receive response
 * @HFI_HOST_FLAGS_NON_DISCARDABLE: yet to define usage by HFI Core?
 */
enum hfi_host_flags {
	HFI_HOST_FLAGS_NONE = HFI_TX_FLAGS_NONE,
	HFI_HOST_FLAGS_INTR_REQUIRED = HFI_TX_FLAGS_INTR_REQUIRED,
	HFI_HOST_FLAGS_RESPONSE_REQUIRED = HFI_TX_FLAGS_RESPONSE_REQUIRED,
	HFI_HOST_FLAGS_NON_DISCARDABLE = HFI_TX_FLAGS_NON_DISCARDABLE,
};

/**
 * struct hfi_kv_pairs - structure definition of a kv pair
 * @key: HFI property as key
 * @value_ptr: Pointer to value of HFI Property
 */
struct hfi_kv_pairs {
	u32 key;
	void *value_ptr;
};

/**
 * struct hfi_cmdbuf_t - HFI Command buffer wrapper structure
 * @lock: Mutex to serialize buffer access
 * @cmd_type: enum value of hfi_cmdbuf_type to classify buffers and help look up
 * @unique_id: unique identifier maintaining client_id with 2 LSB and a random unique id
 * @obj_id: ID of Client objects owning this command buffer (Ex. display or device ID)
 * @size: indicates current size tracking fill level of cmd buffer memory
 * @buf: handle of actual buffer provided by HFI Core driver
 * @node: list node for command buffers tracking by a Client
 * @cmd_buf_chain: list of command buffers in chain
 * @ctx: handle of the HFI adapter Client
 * @pool: Pointer to hfi_buffer_pool structure
 * @buffer_send_done: atomic variable to signal when unpack is finished
 */
struct hfi_cmdbuf_t {
	struct mutex lock;
	enum hfi_cmdbuf_type cmd_type;
	u32 unique_id;
	u32 obj_id;
	u32 size;
#if IS_ENABLED(CONFIG_QTI_HFI_CORE)
	struct hfi_core_cmds_buf_desc buf;
#endif
	struct list_head node;
	struct list_head cmd_buf_chain;
	struct hfi_client_t *ctx;
	struct hfi_buffer_pool *pool;
	atomic_t buffer_send_done;
};

/**
 * struct hfi_buffer_pool - Structure to store and track pre-allocated command buffer information
 * @lock: Mutex to serialize buffer access
 * @node: List node to link unique buffers
 * @available: Atomic variable to signify whether the buffer is in use
 * @buffer_t: Pointer to hfi_cmdbuf_t structure
 */
struct hfi_buffer_pool {
	struct mutex lock;
	struct list_head node;
	atomic_t available;
	struct hfi_cmdbuf_t buffer_t;
};

/**
 * struct hfi_prop_listener - Clients (HFI_DRM objects) implement hfi_prop_listener interface
 * and supposed to  provide listener while adding a Get_Property. HFI Adapter invokes respective
 * listener of each packet while unpacking HFI command buffer.
 */
struct hfi_prop_listener {
	void (*hfi_prop_handler)(u32 obj_id, u32 cmd_id, void *payload, u32 size,
			struct hfi_prop_listener *listener);
};

/**
 * Clients listener implementation pointers cached in a list
 * @list_ptr: List head
 * @packet_id: ID of HFI Packet used a key for listener list
 * @listener_obj: Void Pointer of listener object
 */
struct listener_list {
	struct list_head list_ptr;
	u32 packet_id;
	void *listener_obj;
};

/**
 * struct hfi_client_t - Client handle of adapter interface
 * @node: list node for adapter
 * @lock: Mutex to protect cmd_buf_list
 * @cmd_buf_list: list of command buffers attached to the client
 * @process_cmd_buf: callback function pointer populated by client
 * @host: pointer to adapter module instance
 * @priv: Client provate data pointer
 * @client_id: client identifier
 */
struct hfi_client_t {
	struct list_head node;
	struct mutex lock;
	struct list_head cmd_buf_list;
	struct listener_list packet_listeners;
	int (*process_cmd_buf)(struct hfi_client_t *hfi_client, struct hfi_cmdbuf_t *cmd_buf);
	struct hfi_adapter_t *host;
	void *priv;
	int client_id;
};

/*
 * struct hfi_shared_addr_map -  HFI shared memory address map structure
 *@remote_addr:  pointer to the HFI mapped base address
 *@local_addr:  pointer to the kernel mapped base address
 *@size: size of the buffer
 *@aligned_size: aligned size of the buffer to map
 *@alloc_info: hfi structure to store memory allocation information
 */
struct hfi_shared_addr_map {
	unsigned long remote_addr;
	void __iomem *local_addr;
	u32 size;
	u32 aligned_size;
#if IS_ENABLED(CONFIG_QTI_HFI_CORE)
	struct hfi_core_mem_alloc_info alloc_info;
#endif
};

#if IS_ENABLED(CONFIG_QTI_HFI_CORE)

/**
 * hfi_adapter_init - Creates HFI adapter module object to connect with HFI driver.
 * Adapter registers with HFI driver as a client (@hfi_device_open) and provides/registers
 * top level callback for processing HFI command from HFI Core(@ struct hfi_core_cb_ops)
 * @instance: Specifies this is a primary or secondary vm instance
 */
struct hfi_adapter_t *hfi_adapter_init(int instance);

/**
 * hfi_adapter_client_register - Register a HFI adapter client implementation that would use
 * adapter API to drive HFI communication and provides callbacks to process RX Command buffers
 * (from FW to HLOS). API performs validation of provided client implementation object and adds
 * to list in adapter module context
 * @hfi_adapter: Pointer to hfi_adapter host structure, returned by hfi_adapter_init
 * @hfi_client: Pointer to hfi_client struct used to register with adapter.
 */
int hfi_adapter_client_register(struct hfi_adapter_t *hfi_adapter,
		struct hfi_client_t *hfi_client);

/**
 * hfi_adapter_get_cmd_buf - Gets a TX HFI Command buffer from the HFI Core Driver.
 * Initializes command buffer header with basic info.
 * @ctx: Pointer to hfi_client struct.
 * @obj_id: Identifier to tag cmdbuf with an object. Ex. display id or device id
 * @cmdbuf_type: enum hfi_cmdbuf_type value for which all the packets are aplicable.
 */
struct hfi_cmdbuf_t *hfi_adapter_get_cmd_buf(struct hfi_client_t *ctx, u32 obj_id,
		u32 cmdbuf_type);

/**
 * hfi_adapter_add_hfi_command - Validate available size in HFI command buffer based on current
 * fill level, if sufficient, populate HFI Command packet and payload into HFI cmd-buffer.
 * Update the HFI header accordingly. If size is not sufficient, internally/seamlessly get
 * another buffer from HFI core and chain-up
 * @cmd_buf: Pointer to hfi_adapter command buffer, returned from hfi_adapter_get_cmd_buf.
 * @cmd: HFI COMMAND ID to be added to command buffer.
 * @object_id: ID of display or device, this command is meant for
 * @payload_type: Value of enum hfi_payload_type, specifies the payload data layout
 * @payload: Pointer to payload structure to memcpy into buffer.
 * @size: Size of the payload data in bytes. This does not include packet size, only payload size.
 * @flags: Flags to indicate hints attached with HFI Commands buffer.
 */
int hfi_adapter_add_set_property(struct hfi_cmdbuf_t *cmd_buf, u32 cmd, u32 object_id,
		enum hfi_payload_type payload_type, void *payload, u32 size, u32 flags);

/**
 * hfi_adapter_add_get_property - Similar to 'hfi_adapter_add_set_property'
 * Additionally expects listener handle or object that requires to get notified back
 * if caller expects any response from fw (status flags, response packets etc..)
 * @cmd_buf: Pointer to hfi_adapter command buffer, returned from hfi_adapter_get_cmd_buf.
 * @cmd: HFI COMMAND ID to be added to command buffer.
 * @object_id: ID of display or device, this command is meant for
 * @payload_type: Value of enum hfi_payload_type, specifies the payload data layout
 * @payload: Pointer to payload structure to memcpy into buffer.
 * @size: Size of the payload data in bytes. This does not include packet size, only payload size.
 * @listener: Pointer to listener object, required to pass data from adapter to caller.
 * @flags: Flags to indicate hints attached with HFI Commands buffer.
 */
int hfi_adapter_add_get_property(struct hfi_cmdbuf_t *cmd_buf, u32 cmd_id,
		u32 obj_id, enum hfi_payload_type payload_type,
		void *payload, u32 size, struct hfi_prop_listener *listener, u32 flags);

/**
 * hfi_adapter_add_prop_array - Same as above just payload is an array of key-value pairs.
 * Validates available size in HFI command buffer based on current fill level, then populates
 * property payload into HFI cmd-buffer and updates the HFI header accordingly.
 * @cmd_buf: Pointer to hfi_adapter command buffer, returned from hfi_adapter_get_cmd_buf.
 * @cmd: HFI COMMAND ID to be added to command buffer.
 * @object_id: ID of display or device, this command is meant for
 * @payload_type: Value of enum hfi_payload_type, specifies the payload data layout
 * @payload: Pointer to payload structure to memcpy into buffer.
 * @cnt: Count of the number of elements in the hfi_kv_pairs array.
 * @size: Size of the payload data in bytes. This does not include packet size, only payload size.
 */
int hfi_adapter_add_prop_array(struct hfi_cmdbuf_t *cmd_buf, u32 cmd,
		u32 object_id, enum hfi_payload_type payload_type,
		struct hfi_kv_pairs *payload, u32 cnt, u32 size);

/**
 * hfi_adapter_set_cmd_buf - Submit command buf to HFI Core (destined to FW). If it's a chain of
 * command buffers, posts one by one. This releases to HFI tx Queue using HFI core driver API
 * @cmd_buf: Pointer to hfi_adapter command buffer, returned from hfi_adapter_get_cmd_buf.
 */
int hfi_adapter_set_cmd_buf(struct hfi_cmdbuf_t *cmd_buf);

/**
 * hfi_adapter_set_cmd_buf_blocking - Submit command buf to HFI Core (destined to FW). If it's
 * a chain of command buffers, posts one by one. This releases to HFI tx Queue using HFI core
 * driver API. The blocking call waits for response for the command from FW.
 * @cmd_buf: Pointer to hfi_adapter command buffer, returned from hfi_adapter_get_cmd_buf.
 */
int hfi_adapter_set_cmd_buf_blocking(struct hfi_cmdbuf_t *cmd_buf);

/**
 * hfi_adapter_unpack_cmd_buf - Invokes unpacker API. Recommended to Invoke in Non-IRQ thread
 * @ctx: Pointer to hfi_client struct.
 * @cmd_buf: Pointer to hfi_adapter command buffer.
 */
int hfi_adapter_unpack_cmd_buf(struct hfi_client_t *ctx, struct hfi_cmdbuf_t *cmd_buf);

/**
 * hfi_adapter_release_cmd_buf - Release-buffer explicit API. Client require to invoke
 * this API explicitly for HFI command buffers from Firmware after handling unpack
 * @cmd_buf: Pointer to hfi_adapter command buffer.
 */
int hfi_adapter_release_cmd_buf(struct hfi_cmdbuf_t *cmd_buf);

/**
 * hfi_adapter_buffer_alloc - API to allocate shared memory between HFI & kernel
 * @addr_map: Pointer to hfi_adapter address map which stores the size to allocate
 * and pointers to kernel & hfi address of the shared space.
 */
int hfi_adapter_buffer_alloc(struct hfi_shared_addr_map *addr_map);

/**
 * hfi_adapter_buffer_dealloc - API to deallocate shared memory between HFI & kernel
 * @addr_map: Pointer to hfi_adapter address map which stores the size to allocate
 * and pointers to kernel & hfi address of the shared space.
 */
int hfi_adapter_buffer_dealloc(struct hfi_shared_addr_map *addr_map);

#else

static inline struct hfi_adapter_t *hfi_adapter_init(int instance)
{
	return NULL;
}

static inline int hfi_adapter_client_register(struct hfi_adapter_t *hfi_adapter,
		struct hfi_client_t *hfi_client)
{
	return 0;
}

static inline struct hfi_cmdbuf_t *hfi_adapter_get_cmd_buf(struct hfi_client_t *ctx, u32 obj_id,
		enum hfi_cmdbuf_type cmdbuf_type)
{
	return NULL;
}

static inline int hfi_adapter_add_set_property(struct hfi_cmdbuf_t *cmd_buf, u32 cmd,
		u32 object_id, enum hfi_payload_type payload_type,
		void *payload, u32 size, u32 flags)
{
	return 0;
}

static inline int hfi_adapter_add_get_property(struct hfi_cmdbuf_t *cmd_buf, u32 cmd_id,
		u32 obj_id, enum hfi_payload_type payload_type,
		void *payload, u32 size, struct hfi_prop_listener *listener, u32 flags)
{
	return 0;
}

static inline int hfi_adapter_add_prop_array(struct hfi_cmdbuf_t *cmd_buf, u32 cmd,
		u32 object_id, enum hfi_payload_type payload_type,
		struct hfi_kv_pairs *payload, u32 cnt, u32 size)
{
	return 0;
}

static inline int hfi_adapter_set_cmd_buf(struct hfi_cmdbuf_t *cmd_buf)
{
	return 0;
}

static inline int hfi_adapter_set_cmd_buf_blocking(struct hfi_cmdbuf_t *cmd_buf)
{
	return 0;
}

static inline int hfi_adapter_unpack_cmd_buf(struct hfi_client_t *ctx, struct hfi_cmdbuf_t *cmd_buf)
{
	return 0;
}

static inline int hfi_adapter_release_cmd_buf(struct hfi_cmdbuf_t *cmd_buf)
{
	return 0;
}

static inline int hfi_adapter_buffer_alloc(struct hfi_shared_addr_map *addr_map)
{
	return 0;
}

static inline int hfi_adapter_buffer_dealloc(struct hfi_shared_addr_map *addr_map)
{
	return 0;
}

#endif /*#if IS_ENABLED(CONFIG_QTI_HFI_CORE)*/

#endif  /* _HFI_ADAPTER_H_ */
