/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __UAPI_SYNX_H__
#define __UAPI_SYNX_H__

#include <linux/types.h>
#include <linux/ioctl.h>

/* Size of opaque payload sent to kernel for safekeeping until signal time */
#define SYNX_USER_PAYLOAD_SIZE               4

#define SYNX_MAX_WAITING_SYNX                16

#define SYNX_CALLBACK_RESULT_SUCCESS         2
#define SYNX_CALLBACK_RESULT_FAILED          3
#define SYNX_CALLBACK_RESULT_CANCELED        4

/**
 * struct synx_info - Sync object creation information
 *
 * @name     : Optional string representation of the synx object
 * @synx_obj : Sync object returned after creation in kernel
 */
struct synx_info {
	char name[64];
	__s32 synx_obj;
};

/**
 * struct synx_userpayload_info - Payload info from user space
 *
 * @synx_obj:   Sync object for which payload has to be registered for
 * @reserved:   Reserved
 * @payload:    Pointer to user payload
 */
struct synx_userpayload_info {
	__s32 synx_obj;
	__u32 reserved;
	__u64 payload[SYNX_USER_PAYLOAD_SIZE];
};

/**
 * struct synx_signal - Sync object signaling struct
 *
 * @synx_obj   : Sync object to be signaled
 * @synx_state : State of the synx object to which it should be signaled
 */
struct synx_signal {
	__s32 synx_obj;
	__u32 synx_state;
};

/**
 * struct synx_merge - Merge information for synx objects
 *
 * @synx_objs :  Pointer to synx object array to merge
 * @num_objs  :  Number of objects in the array
 * @merged    :  Merged synx object
 */
struct synx_merge {
	__u64 synx_objs;
	__u32 num_objs;
	__s32 merged;
};

/**
 * struct synx_wait - Sync object wait information
 *
 * @synx_obj   : Sync object to wait on
 * @reserved   : Reserved
 * @timeout_ms : Timeout in milliseconds
 */
struct synx_wait {
	__s32 synx_obj;
	__u32 reserved;
	__u64 timeout_ms;
};

/**
 * struct synx_external_desc - info of external sync object
 *
 * @type     : Synx type
 * @reserved : Reserved
 * @id       : Sync object id
 *
 */
struct synx_external_desc {
	__u32 type;
	__u32 reserved;
	__s32 id[2];
};

/**
 * struct synx_bind - info for binding two synx objects
 *
 * @synx_obj      : Synx object
 * @Reserved      : Reserved
 * @ext_sync_desc : External synx to bind to
 *
 */
struct synx_bind {
	__s32 synx_obj;
	__u32 reserved;
	struct synx_external_desc ext_sync_desc;
};

/**
 * struct synx_addrefcount - info for refcount increment
 *
 * @synx_obj : Synx object
 * @count    : Count to increment
 *
 */
struct synx_addrefcount {
	__s32 synx_obj;
	__u32 count;
};

/**
 * struct synx_id_info - info for import and export of a synx object
 *
 * @synx_obj     : Synx object to be exported
 * @secure_key   : Secure key created in export and used in import
 * @new_synx_obj : Synx object created in import
 *
 */
struct synx_id_info {
	__s32 synx_obj;
	__u32 secure_key;
	__s32 new_synx_obj;
	__u32 padding;
};

/**
 * struct synx_fence_desc - info of external fence object
 *
 * @type     : Fence type
 * @reserved : Reserved
 * @id       : Fence object id
 *
 */
struct synx_fence_desc {
	__u32 type;
	__u32 reserved;
	__s32 id[2];
};

/**
 * struct synx_create - Sync object creation information
 *
 * @name     : Optional string representation of the synx object
 * @synx_obj : Synx object allocated
 * @flags    : Create flags
 * @desc     : External fence desc
 */
struct synx_create_v2 {
	char name[64];
	__u32 synx_obj;
	__u32 flags;
	struct synx_fence_desc desc;
};

/**
 * struct synx_userpayload_info - Payload info from user space
 *
 * @synx_obj  : Sync object for which payload has to be registered for
 * @reserved  : Reserved
 * @payload   : Pointer to user payload
 */
struct synx_userpayload_info_v2 {
	__u32 synx_obj;
	__u32 reserved;
	__u64 payload[SYNX_USER_PAYLOAD_SIZE];
};

/**
 * struct synx_signal - Sync object signaling struct
 *
 * @synx_obj   : Sync object to be signaled
 * @synx_state : State of the synx object to which it should be signaled
 * @reserved   : Reserved
 */
struct synx_signal_v2 {
	__u32 synx_obj;
	__u32 synx_state;
	__u64 reserved;
};

/**
 * struct synx_merge - Merge information for synx objects
 *
 * @synx_objs :  Pointer to synx object array to merge
 * @num_objs  :  Number of objects in the array
 * @merged    :  Merged synx object
 * @flags     :  Merge flags
 * @reserved  :  Reserved
 */
struct synx_merge_v2 {
	__u64 synx_objs;
	__u32 num_objs;
	__u32 merged;
	__u32 flags;
	__u32 reserved;
};

/**
 * struct synx_merge_indv_info - Merge information for synx objects
 *
 * @synx_objs :  Pointer to synx object array to merge
 * @num_objs  :  Number of objects in the array
 * @merged    :  Merged synx object
 * @flags     :  Merge flags
 * @reserved  :  Reserved
 * @client_data_hi   : most significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal.
 * @client_data_lo   : least significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal.
 * @security_key_hi  : most significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 * @security_key_lo  : least significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 */
struct synx_merge_indv_info {
	__u64 synx_objs;
	__u32 num_objs;
	__u32 merged;
	__u32 flags;
	__u32 reserved;
	__u32 client_data_hi;
	__u32 client_data_lo;
	__u32 security_key_hi;
	__u32 security_key_lo;
};

/**
 * struct synx_merge_n_info - Merge information for synx objects
 * @type         : Merge params type
 * @reserved     : Reserved
 * @indv         : params to create a single merged handle
 */
struct synx_merge_n_info {
	__u32 type;
	__u32 reserved;
	union {
		struct synx_merge_indv_info indv;
	};
};

/**
 * struct synx_wait - Sync object wait information
 *
 * @synx_obj   : Sync object to wait on
 * @reserved   : Reserved
 * @timeout_ms : Timeout in milliseconds
 */
struct synx_wait_v2 {
	__u32 synx_obj;
	__u32 reserved;
	__u64 timeout_ms;
};

/**
 * struct synx_external_desc - info of external sync object
 *
 * @type     : Synx type
 * @reserved : Reserved
 * @id       : Sync object id
 *
 */
struct synx_external_desc_v2 {
	__u64 id;
	__u32 type;
	__u32 reserved;
};

/**
 * struct synx_bind - info for binding two synx objects
 *
 * @synx_obj      : Synx object
 * @Reserved      : Reserved
 * @ext_sync_desc : External synx to bind to
 *
 */
struct synx_bind_v2 {
	__u32 synx_obj;
	__u32 reserved;
	struct synx_external_desc_v2 ext_sync_desc;
};

/**
 * struct synx_import_info - import info
 *
 * @synx_obj     : Synx handle to be imported
 * @flags        : Import flags
 * @new_synx_obj : Synx object created in import
 * @reserved     : Reserved
 * @desc         : External fence descriptor
 */
struct synx_import_info {
	__u32 synx_obj;
	__u32 flags;
	__u32 new_synx_obj;
	__u32 reserved;
	struct synx_fence_desc desc;
};

/**
 * struct synx_import_info_v2 - import info v2
 *
 * @synx_obj         : Synx handle to be imported
 * @flags            : Import flags
 * @new_synx_obj     : Synx object created in import
 * @reserved         : Reserved
 * @desc             : External fence descriptor
 * @client_data_hi   : most significant 32 bits of the 64-bit client_data
 * @client_data_lo   : least significant 32 bits of the 64-bit client_data
 * @security_key_hi  : most significant 32 bits of the 64-bit security_key
 * @security_key_lo  : least significant 32 bits of the 64-bit security_key
 */
struct synx_import_info_v2 {
	__u32 synx_obj;
	__u32 flags;
	__u32 new_synx_obj;
	__u32 reserved;
	struct synx_fence_desc desc;
	__u32 client_data_hi;
	__u32 client_data_lo;
	__u32 security_key_hi;
	__u32 security_key_lo;
};

/**
 * struct synx_import_arr_info - import list info
 *
 * @list     : List of synx_import_info
 * @num_objs : No of fences to import
 */
struct synx_import_arr_info {
	__u64 list;
	__u32 num_objs;
};

/**
 * struct synx_fence_fd - get fd for synx fence
 *
 * @synx_obj : Synx handle
 * @fd       : fd for synx handle fence
 */
struct synx_fence_fd {
	__u32 synx_obj;
	__s32 fd;
};

/**
 * struct synx_private_ioctl_arg - Sync driver ioctl argument
 *
 * @id        : IOCTL command id
 * @size      : Size of command payload
 * @result    : Result of command execution
 * @reserved  : Reserved
 * @ioctl_ptr : Pointer to user data
 */
struct synx_private_ioctl_arg {
	__u32 id;
	__u32 size;
	__u32 result;
	__u32 reserved;
	__u64 ioctl_ptr;
};


/**
 * struct synx_initialize_v2 - synx initialization information
 *
 * @name     : Optional string representation of the synx object
 * @id       : Client identifier
 * @flags    : synx initialization flags
 * @reserved : Reserved
 */
struct synx_initialize_v2 {
	char name[64];
	__u32 id;
	__u32 flags;
	__u32 reserved;
};

/**
 * struct synx_qdesc_info - info of synx queue
 *
 * @type          : Synx queue memory type
 * @heap_fd       : File descriptor of the queue (dma buf heap)
 * @size          : Size of the memory
 * @base_offset   : Offset for queue base
 * @wr_idx_offset : Offset for write index in the queue
 */
struct synx_qdesc_info {
	__u32 type;
	__u32 heap_fd;
	__u64 size;
	__u64 base_offset;
	__u64 wr_idx_offset;
};

/**
 * struct synx_initialize_v3 - synx initialization information
 *
 * @name      : Optional string representation of the synx object
 * @id        : Client identifier
 * @flags     : synx initialization flags
 * @qdesc     : Memory descriptor of allocated queue
 * @reserved  : Reserved
 */
struct synx_initialize_v3 {
	char name[64];
	__u32 id;
	__u32 flags;
	struct synx_qdesc_info qdesc;
	__u64 reserved;
};

/**
 * struct synx_release_info - Synx release arr parameters
 *
 * @synx_obj     : Synx handle to be released
 * @status       : release status of synx_obj
 * @reserved     : Reserved
 */
struct synx_release_indv_info {
	__u32 synx_obj;
	__s32 status;
	__u32 reserved;
};

/**
 * struct synx_op_arr_info - Generic list info for synx objects for batch
 *                           operations.
 *
 * @synx_objs :  list of individual handle info
 * @num_objs  :  Number of objects in the array
 * @reserved  : Reserved
 */
struct synx_op_arr_info {
	__u64 list;
	__u32 num_objs;
	__u32 reserved;
};

/**
 * struct synx_release_n_info - Release information for synx objects
 *
 * @type     : Release params type
 * @indv     : Params to release an individual handle
 * @arr      : Params to release an array of handles
 */
struct synx_release_n_info {
	__u32 type;
	union {
		struct synx_release_indv_info indv;
		struct synx_op_arr_info arr;
	};
};

/**
 * struct synx_recover_info - synx recover information
 *
 * @id       : Client identifier
 * @reserved : Reserved
 */
struct synx_recover_info {
	__u32 id;
	__u32 reserved;
};

#define SYNX_PRIVATE_MAGIC_NUM 's'

#define SYNX_PRIVATE_IOCTL_CMD \
	_IOWR(SYNX_PRIVATE_MAGIC_NUM, 130, struct synx_private_ioctl_arg)

#define SYNX_CREATE                          0
#define SYNX_RELEASE                         1
#define SYNX_SIGNAL                          2
#define SYNX_MERGE                           3
#define SYNX_REGISTER_PAYLOAD                4
#define SYNX_DEREGISTER_PAYLOAD              5
#define SYNX_WAIT                            6
#define SYNX_BIND                            7
#define SYNX_ADDREFCOUNT                     8
#define SYNX_GETSTATUS                       9
#define SYNX_IMPORT                          10
#define SYNX_EXPORT                          11
#define SYNX_IMPORT_ARR                      12
#define SYNX_GETFENCE_FD                     13
#define SYNX_INITIALIZE                      14
#define SYNX_RECOVER                         15
#define SYNX_RELEASE_N                       16
#define SYNX_INITIALIZE_V3                   17
#define SYNX_IMPORT_V2                       18
#define SYNX_IMPORT_ARR_V2                   19
#define SYNX_MERGE_N                         20
#endif /* __UAPI_SYNX_H__ */
