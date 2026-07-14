/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __SYNX_API_H__
#define __SYNX_API_H__

#include <linux/list.h>
#include <synx_header.h>

#include "synx_err.h"
#include "synx_extension_api.h"

#define SYNX_NO_TIMEOUT        ((u64)-1)

/**
 * SYNX_INVALID_HANDLE      : client can assign the synx handle variable with this value
 *                            when it doesn't hold a valid synx handle
 */
#define SYNX_INVALID_HANDLE 0
#define SYNX_MAX_SIGNAL_PER_CLIENT 64

/**
* SYNX_NO_SECURITY_KEY     : No security key is associated with Synx obj
*/
#define SYNX_NO_SECURITY_KEY 0

/* synx object states */
#define SYNX_STATE_INVALID             0    // Invalid synx object
#define SYNX_STATE_ACTIVE              1    // Synx object has not been signaled
#define SYNX_STATE_SIGNALED_ERROR      3    // Synx object signaled with error
#define SYNX_STATE_SIGNALED_EXTERNAL   5    // Synx object was signaled by external dma client.
#define SYNX_STATE_SIGNALED_SSR        6    // Synx object signaled with SSR
#define SYNX_STATE_TIMEOUT             7    // Callback status for synx object in case of timeout

/**
 * enum synx_create_flags - Flags passed during synx_create call.
 *
 * SYNX_CREATE_LOCAL_FENCE  : Instructs the framework to create local synx object,
 *                            for local synchronization i.e. within same core.
 * SYNX_CREATE_GLOBAL_FENCE : Instructs the framework to create global synx object
 *                            for global synchronization i.e. across supported core.
 * SYNX_CREATE_DMA_FENCE    : Create a synx object by wrapping the provided dma fence.
 *                            Need to pass the dma_fence ptr through fence variable
 *                            if this flag is set. (NOT SUPPORTED)
 * SYNX_CREATE_CSL_FENCE    : Create a synx object with provided csl fence.
 *                            Establishes interop with the csl fence through
 *                            bind operations. (NOT SUPPORTED)
 */
enum synx_create_flags {
	SYNX_CREATE_LOCAL_FENCE  = 0x01,
	SYNX_CREATE_GLOBAL_FENCE = 0x02,
	SYNX_CREATE_DMA_FENCE    = 0x04,
	SYNX_CREATE_CSL_FENCE    = 0x08,
	SYNX_CREATE_MAX_FLAGS    = 0x10,
};

/**
 * enum synx_init_flags - Session initialization flag
 * SYNX_INIT_DEFAULT   : Initialization flag to be passed
 *                       when initializing session
 * SYNX_INIT_MAX       : Used for internal checks
 */
enum synx_init_flags {
	SYNX_INIT_DEFAULT = 0x00,
	SYNX_INIT_MAX     = 0x01,
};

/**
 * enum synx_import_flags - Import flags
 *
 * SYNX_IMPORT_LOCAL_FENCE  : Instructs the framework to create local synx object,
 *                            for local synchronization i.e. within same core.
 * SYNX_IMPORT_GLOBAL_FENCE : Instructs the framework to create global synx object,
 *                            for global synchronization i.e. across supported core.
 * SYNX_IMPORT_SYNX_FENCE   : Import native Synx handle for synchronization.
 *                            Need to pass the Synx handle ptr through fence variable
 *                            if this flag is set. Client must pass:
 *                            a. SYNX_IMPORT_SYNX_FENCE|SYNX_IMPORT_LOCAL_FENCE
 *                               to import a synx handle as local synx handle.
 *                            b. SYNX_IMPORT_SYNX_FENCE|SYNX_IMPORT_GLOBAL_FENCE
 *                               to import a synx handle as global synx handle.
 *                            If client passes SYNX_IMPORT_SYNX_FENCE without specifying
 *                            SYNX_IMPORT_LOCAL_FENCE or SYNX_IMPORT_GLOBAL_FENCE, and the
 *                            fence passed is NULL, then synx_import returns a failure.
 * SYNX_IMPORT_DMA_FENCE    : Import dma fence and create Synx handle for interop.
 *                            Need to pass the dma_fence ptr through fence variable
 *                            if this flag is set. Client must pass:
 *                            a. SYNX_IMPORT_DMA_FENCE|SYNX_IMPORT_LOCAL_FENCE
 *                               to import a dma fence and create local synx handle
 *                               for interop.
 *                            b. SYNX_IMPORT_DMA_FENCE|SYNX_IMPORT_GLOBAL_FENCE
 *                               to import a dma fence and create global synx handle
 *                               for interop.
 *                            If client passes SYNX_IMPORT_DMA_FENCE without specifying
 *                            SYNX_IMPORT_LOCAL_FENCE or SYNX_IMPORT_GLOBAL_FENCE, and the
 *                            fence passed is NULL, then synx_import returns a failure.
 * SYNX_IMPORT_EX_RELEASE   : Flag to inform relaxed invocation where release call
 *                            need not be called by client on this handle after import.
 *                            (NOT SUPPORTED)
 * SYNX_IMPORT_REUSABLE     : Flag to inform that this synx handle supports repeated signaling.
 *                            Client must pass:
 *                            a. null ptr through fence variable to create reusable fence
 *                            b. unsignaled synx handle through fence variable to import
 *                               reusable fence
 *                            If client passes SYNX_IMPORT_DMA_FENCE with SYNX_IMPORT_REUSABLE
 *                            then synx_import returns a failure as reusable fence dosen't
 *                            have native dma-fence support.
 */
enum synx_import_flags {
	SYNX_IMPORT_LOCAL_FENCE  = 0x01,
	SYNX_IMPORT_GLOBAL_FENCE = 0x02,
	SYNX_IMPORT_SYNX_FENCE   = 0x04,
	SYNX_IMPORT_DMA_FENCE    = 0x08,
	SYNX_IMPORT_EX_RELEASE   = 0x10,
	SYNX_IMPORT_REUSABLE     = 0x20,
};

/**
 * enum synx_signal_status - Signal status
 *
 * SYNX_STATE_SIGNALED_SUCCESS : Signal success
 * SYNX_STATE_SIGNALED_CANCEL  : Signal cancellation
 * SYNX_STATE_SIGNALED_MAX     : Clients can send custom notification
 *                               beyond the max value (only positive)
 */
enum synx_signal_status {
	SYNX_STATE_SIGNALED_SUCCESS = 2,
	SYNX_STATE_SIGNALED_CANCEL  = 4,
	SYNX_STATE_SIGNALED_MAX     = 64,
};

/**
 * enum synx_signal_flags - Signal flags
 *
 * SYNX_SIGNAL_IMMEDIATE : Signal synx object immediately in function call (default behavior)
 * SYNX_SIGNAL_DELAYED : Delay signal of synx object until client updates tx_ptr and sends IPCC;
 *                       supported only for SYNX_HW_FENCE clients, not for other clients
 */
enum synx_signal_flags {
	SYNX_SIGNAL_IMMEDIATE = 0,
	SYNX_SIGNAL_DELAYED = 1,
};

/**
 * synx_callback - Callback invoked by external fence
 *
 * External fence dispatch the registered callback to notify
 * signal to synx framework.
 */
typedef void (*synx_callback)(s32 sync_obj, int status, void *data);

/**
 * synx_user_callback - Callback function registered by clients
 *
 * User callback registered for non-blocking wait. Dispatched when
 * synx object is signaled or timed-out with status of synx object.
 */
typedef void (*synx_user_callback_t)(u32 h_synx, int status, void *data);

/**
 * struct bind_operations - Function pointers that need to be defined
 *    to achieve bind functionality for external fence with synx obj
 *
 * @register_callback   : Function to register with external sync object
 * @deregister_callback : Function to deregister with external sync object
 * @enable_signaling    : Function to enable the signaling on the external
 *                        sync object (optional)
 * @signal              : Function to signal the external sync object
 */
struct bind_operations {
	int (*register_callback)(synx_callback cb_func,
		void *userdata, s32 sync_obj);
	int (*deregister_callback)(synx_callback cb_func,
		void *userdata, s32 sync_obj);
	int (*enable_signaling)(s32 sync_obj);
	int (*signal)(s32 sync_obj, u32 status);
};

/**
 * synx_bind_client_type : External fence supported for bind (NOT SUPPORTED)
 *
 * SYNX_TYPE_CSL : Camera CSL fence
 * SYNX_MAX_BIND_TYPES : Used for internal checks
 */
enum synx_bind_client_type {
	SYNX_TYPE_CSL = 0,
	SYNX_MAX_BIND_TYPES,
};

/**
 * struct synx_register_params - External registration parameters  (NOT SUPPORTED)
 *
 * @ops  : Bind operations struct
 * @name : External client name
 *         Only first 64 bytes are accepted, rest will be ignored
 * @type : Synx bind client type
 */
struct synx_register_params {
	struct bind_operations ops;
	char *name;
	enum synx_bind_client_type type;
};

/**
 * struct synx_queue_desc - Memory descriptor of the queue allocated for
 *                          hw-fence and fence direct clients during
 *                          initialize. (Synx native clients need not
 *                          pass any pointer in synx_initialize_params).
 *
 * @vaddr         : CPU virtual address of the queue.
 * @dev_addr      : Physical address of the memory object.
 * @size          : Size of the memory.
 * @mem_data      : Internal pointer with the attributes of the allocation.
 * @base_offset   : Offset for queue base from start of shared memory.
 * @wr_idx_offset : Offset for write index from start of shared memory.
 */
struct synx_queue_desc {
	void *vaddr;
	u64 dev_addr;
	u64 size;
	void *mem_data;
	u64 base_offset;
	u64 wr_idx_offset;
};

/**
 * enum synx_client_id : Unique identifier of the supported clients
 *
 * Protocol based Client ID range : 0    - 1023 : Synx Clients
 *                                : 1024 - 4095 : HW Fence Clients
 *                                : 4096 - 8191 : Reserved (for future use)
 *                                : 8192 - 9280 : Fence Direct Clients
 *
 * @SYNX_CLIENT_NATIVE   : Native Client
 * @SYNX_CLIENT_GFX_CTX0 : GFX Client 0
 * @SYNX_CLIENT_DPU_CTL0 : DPU Client 0
 * @SYNX_CLIENT_DPU_CTL1 : DPU Client 1
 * @SYNX_CLIENT_DPU_CTL2 : DPU Client 2
 * @SYNX_CLIENT_DPU_CTL3 : DPU Client 3
 * @SYNX_CLIENT_DPU_CTL4 : DPU Client 4
 * @SYNX_CLIENT_DPU_CTL5 : DPU Client 5
 * @SYNX_CLIENT_EVA_CTX0 : EVA Client 0
 * @SYNX_CLIENT_VID_CTX0 : Video Client 0
 * @SYNX_CLIENT_NSP_CTX0 : NSP Client 0
 * @SYNX_CLIENT_IFE_CTX0 : IFE Client 0
 * @SYNX_CLIENT_ICP_CTX0 : ICP Client 0
 * @SYNX_CLIENT_ICP1_CTX0 : ICP1 Client 0 (OFE)
 * @SYNX_CLIENT_HW_FENCE_GFX_CTX0 : HW Fence GFX Client 0
 * @SYNX_CLIENT_HW_FENCE_IPE_CTX0 : HW Fence IPE Client 0
 * @SYNX_CLIENT_HW_FENCE_VID_CTX0 : HW Fence Video Client 0
 * @SYNX_CLIENT_HW_FENCE_DPU0_CTL0 : HW Fence DPU0 Client 0
 * @SYNX_CLIENT_HW_FENCE_DPU1_CTL0 : HW Fence DPU1 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE0_CTX0 : HW Fence IFE0 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE1_CTX0 : HW Fence IFE1 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE2_CTX0 : HW Fence IFE2 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE3_CTX0 : HW Fence IFE3 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE4_CTX0 : HW Fence IFE4 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE5_CTX0 : HW Fence IFE5 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE6_CTX0 : HW Fence IFE6 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE7_CTX0 : HW Fence IFE7 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE8_CTX0 : HW Fence IFE8 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE9_CTX0 : HW Fence IFE9 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE10_CTX0 : HW Fence IFE10 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE11_CTX0 : HW Fence IFE11 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE12_CTX0 : HW Fence IFE12 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE13_CTX0 : HW Fence IFE13 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE14_CTX0 : HW Fence IFE14 Client 0
 * @SYNX_CLIENT_HW_FENCE_IFE15_CTX0 : HW Fence IFE15 Client 0
 * @SYNX_CLIENT_HW_FENCE_TEST_CTX0  : HW Fence Test Client 0
 * @SYNX_CLIENT_HW_FENCE_IPA_CTX0   : HW Fence IPA Client 0
 * @SYNX_CLIENT_HW_FENCE_LSR0_CTX0  : HW Fence LSR0 Client 0
 * @SYNX_CLIENT_HW_FENCE_LSR1_CTX0  : HW Fence LSR1 Client 0
 * @SYNX_CLIENT_HW_FENCE_DCP0_CTX0  : HW Fence DCP0 Client 0
 * @SYNX_CLIENT_HW_FENCE_DCP1_CTX0  : HW Fence DCP1 Client 0
 * @SYNX_CLIENT_HW_FENCE_GFX1_CTX0  : HW Fence GFX1 Client 0
 * @SYNX_CLIENT_HW_FENCE_VID1_CTX0  : HW Fence VID1 Client 0
 * @SYNX_CLIENT_HW_FENCE_IPA1_CTX0  : HW Fence IPA1 Client 0
 * @SYNX_CLIENT_HW_FENCE_TEST1_CTX0 : HW Fence TEST1 Client 0
 * @SYNX_CLIENT_HW_FENCE_TEST2_CTX0 : HW Fence TEST2 Client 0
 * @SYNX_CLIENT_HW_FENCE_TEST3_CTX0 : HW Fence TEST3 Client 0
 * @SYNX_CLIENT_HW_FENCE_TEST4_CTX0 : HW Fence TEST4 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE0_CTX0  : Fence Direct IFE0 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE1_CTX0  : Fence Direct IFE1 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE2_CTX0  : Fence Direct IFE2 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE3_CTX0  : Fence Direct IFE3 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE4_CTX0  : Fence Direct IFE4 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE5_CTX0  : Fence Direct IFE5 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE6_CTX0  : Fence Direct IFE6 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE7_CTX0  : Fence Direct IFE7 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE8_CTX0  : Fence Direct IFE8 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE9_CTX0  : Fence Direct IFE9 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE10_CTX0 : Fence Direct IFE10 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE11_CTX0 : Fence Direct IFE11 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE12_CTX0 : Fence Direct IFE12 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE13_CTX0 : Fence Direct IFE13 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE14_CTX0 : Fence Direct IFE14 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_IFE15_CTX0 : Fence Direct IFE15 Client 0
 * @SYNX_CLIENT_FENCE_DIRECT_TEST_CTX0  : Fence Direct Test Client 0
 */
enum synx_client_id {
	SYNX_CLIENT_NATIVE = 0,
	SYNX_CLIENT_GFX_CTX0,
	SYNX_CLIENT_DPU_CTL0,
	SYNX_CLIENT_DPU_CTL1,
	SYNX_CLIENT_DPU_CTL2,
	SYNX_CLIENT_DPU_CTL3,
	SYNX_CLIENT_DPU_CTL4,
	SYNX_CLIENT_DPU_CTL5,
	SYNX_CLIENT_EVA_CTX0,
	SYNX_CLIENT_VID_CTX0,
	SYNX_CLIENT_NSP_CTX0,
	SYNX_CLIENT_IFE_CTX0,
	SYNX_CLIENT_ICP_CTX0,
	SYNX_CLIENT_ICP1_CTX0,
	SYNX_CLIENT_END,
	SYNX_HW_FENCE_CLIENT_START = 1024,
	SYNX_CLIENT_HW_FENCE_GFX_CTX0 = SYNX_HW_FENCE_CLIENT_START,
	SYNX_CLIENT_HW_FENCE_IPE_CTX0 = SYNX_CLIENT_HW_FENCE_GFX_CTX0 + SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_VID_CTX0 = SYNX_CLIENT_HW_FENCE_IPE_CTX0 + SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_DPU0_CTL0 = SYNX_CLIENT_HW_FENCE_VID_CTX0 + SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_DPU1_CTL0 = SYNX_CLIENT_HW_FENCE_DPU0_CTL0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE0_CTX0 = SYNX_CLIENT_HW_FENCE_DPU1_CTL0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE1_CTX0 = SYNX_CLIENT_HW_FENCE_IFE0_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE2_CTX0 = SYNX_CLIENT_HW_FENCE_IFE1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE3_CTX0 = SYNX_CLIENT_HW_FENCE_IFE2_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE4_CTX0 = SYNX_CLIENT_HW_FENCE_IFE3_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE5_CTX0 = SYNX_CLIENT_HW_FENCE_IFE4_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE6_CTX0 = SYNX_CLIENT_HW_FENCE_IFE5_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE7_CTX0 = SYNX_CLIENT_HW_FENCE_IFE6_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE8_CTX0 = SYNX_CLIENT_HW_FENCE_IFE7_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE9_CTX0 = SYNX_CLIENT_HW_FENCE_IFE8_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE10_CTX0 = SYNX_CLIENT_HW_FENCE_IFE9_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE11_CTX0 = SYNX_CLIENT_HW_FENCE_IFE10_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE12_CTX0 = SYNX_CLIENT_HW_FENCE_IFE11_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE13_CTX0 = SYNX_CLIENT_HW_FENCE_IFE12_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE14_CTX0 = SYNX_CLIENT_HW_FENCE_IFE13_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IFE15_CTX0 = SYNX_CLIENT_HW_FENCE_IFE14_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_TEST_CTX0 = SYNX_CLIENT_HW_FENCE_IFE15_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IPA_CTX0 = SYNX_CLIENT_HW_FENCE_TEST_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_LSR0_CTX0 = SYNX_CLIENT_HW_FENCE_IPA_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_LSR1_CTX0 = SYNX_CLIENT_HW_FENCE_LSR0_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_DCP0_CTX0 = SYNX_CLIENT_HW_FENCE_LSR1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_DCP1_CTX0 = SYNX_CLIENT_HW_FENCE_DCP0_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_GFX1_CTX0 = SYNX_CLIENT_HW_FENCE_DCP1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_VID1_CTX0 = SYNX_CLIENT_HW_FENCE_GFX1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_IPA1_CTX0 = SYNX_CLIENT_HW_FENCE_VID1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_TEST1_CTX0 = SYNX_CLIENT_HW_FENCE_IPA1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_TEST2_CTX0 = SYNX_CLIENT_HW_FENCE_TEST1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_TEST3_CTX0 = SYNX_CLIENT_HW_FENCE_TEST2_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_HW_FENCE_TEST4_CTX0 = SYNX_CLIENT_HW_FENCE_TEST3_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_HW_FENCE_CLIENT_END = 4096,
	SYNX_FENCE_DIRECT_CLIENT_START = 8192,
	SYNX_CLIENT_FENCE_DIRECT_IFE0_CTX0 = SYNX_FENCE_DIRECT_CLIENT_START,
	SYNX_CLIENT_FENCE_DIRECT_IFE1_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE0_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE2_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE1_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE3_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE2_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE4_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE3_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE5_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE4_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE6_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE5_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE7_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE6_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE8_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE7_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE9_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE8_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE10_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE9_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE11_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE10_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE12_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE11_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE13_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE12_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE14_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE13_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_IFE15_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE14_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_FENCE_DIRECT_TEST_CTX0 = SYNX_CLIENT_FENCE_DIRECT_IFE15_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_FENCE_DIRECT_CLIENT_END = SYNX_CLIENT_FENCE_DIRECT_TEST_CTX0 +
		SYNX_MAX_SIGNAL_PER_CLIENT,
	SYNX_CLIENT_MAX = SYNX_FENCE_DIRECT_CLIENT_END,
};

struct synx_ops;

/**
 * struct synx_session - Client session identifier
 *
 * @type   : Session type.
 *           Internal Member. (Do not access/modify)
 * @client : Pointer to client session
 *           Internal Member. (Do not access/modify)
 * @ops    : Pointer to synx operations
 */
struct synx_session {
	u32 type;
	void *client;
	const struct synx_ops *ops;
};

/**
 * struct synx_initialization_params - Session params
 *
 * @name  : Client session name
 *          Only first 64 bytes are accepted, rest will be ignored
 * @ptr   : Memory descriptor of queue allocated by fence during
 *          device register. (filled by function)
 * @id    : Client identifier
 * @flags : Synx initialization flags
 */
struct synx_initialization_params {
	const char *name;
	struct synx_queue_desc *ptr;
	enum synx_client_id id;
	enum synx_init_flags flags;
};

/**
 * struct synx_create_params - Synx creation parameters
 *
 * @name     : Optional parameter associating a name with the synx
 *             object for debug purposes
 *             Only first 64 bytes are accepted,
 *             rest will be ignored
 * @h_synx   : Pointer to synx object handle (filled by function)
 * @fence    : Pointer to external dma fence or csl fence. (NOT SUPPORTED)
 * @flags    : Synx flags for customization
 */

struct synx_create_params {
	const char *name;
	u32 *h_synx;
	void *fence;
	enum synx_create_flags flags;
};

/**
 *enum synx_release_type - Release params type
 *
 * SYNX_RELEASE_INDV_PARAMS : Release filled with synx_release_indv_params struct
 * SYNX_RELEASE_ARR_PARAMS  : Release filled with synx_release_arr_params struct
 */
enum synx_release_type {
	SYNX_RELEASE_INDV_PARAMS = 0x01,
	SYNX_RELEASE_ARR_PARAMS  = 0x02,
};

/**
 * struct synx_release_indv_params - Synx release indv parameters
 *
 * @h_synx      : Synx handle to be released.
 * @result      : Return value of release of h_synx.
 *                (filled by the function)
 */
struct synx_release_indv_params {
	uint32_t h_synx;
	int32_t result;
};

/**
 * struct synx_release_arr_params - Synx release arr parameters
 *
 * @list      : List of synx_release_indv_params.
 * @num_objs  : Number of synx handles to be released.
 */
struct synx_release_arr_params {
	struct synx_release_indv_params *list;
	uint32_t                 num_objs;
};

/**
 * struct synx_release_n_params - Synx release_n parameters
 *
 * @type     : Release params type filled by client.
 * @indv     : Params to release an individual handle.
 * @arr      : Params to release an array of handles.
 */
struct synx_release_n_params {
	enum synx_release_type type;
	union {
		struct synx_release_indv_params indv;
		struct synx_release_arr_params  arr;
	};
};

/**
 * enum synx_merge_flags - Handle merge flags
 *
 * SYNX_MERGE_LOCAL_FENCE   : Create local composite synx object. To be passed along
 *                            with SYNX_MERGE_NOTIFY_ON_ALL.
 * SYNX_MERGE_GLOBAL_FENCE  : Create global composite synx object. To be passed along
 *                            with SYNX_MERGE_NOTIFY_ON_ALL.
 * SYNX_MERGE_NOTIFY_ON_ALL : Notify on signaling of ALL objects.
 *                            Clients must pass:
 *                            a. SYNX_MERGE_LOCAL_FENCE|SYNX_MERGE_NOTIFY_ON_ALL
 *                               to create local composite synx object and notify
 *                               it when all child synx objects are signaled.
 *                            b. SYNX_MERGE_GLOBAL_FENCE|SYNX_MERGE_NOTIFY_ON_ALL
 *                               to create global composite synx object and notify
 *                               it when all child synx objects are signaled.
 * SYNX_MERGE_NOTIFY_ON_ANY : Notify on signaling of ANY object. (NOT SUPPORTED)
 */
enum synx_merge_flags {
	SYNX_MERGE_LOCAL_FENCE   = 0x01,
	SYNX_MERGE_GLOBAL_FENCE  = 0x02,
	SYNX_MERGE_NOTIFY_ON_ALL = 0x04,
	SYNX_MERGE_NOTIFY_ON_ANY = 0x08,
};

/*
 * struct synx_merge_params - Synx merge parameters
 *
 * @h_synxs      : Pointer to a array of synx handles to be merged
 * @flags        : Merge flags
 * @num_objs     : Number of synx handles to be merged (in array h_synxs).
 * @h_merged_obj : Merged synx handle (filled by function)
 */
struct synx_merge_params {
	u32 *h_synxs;
	enum synx_merge_flags flags;
	u32 num_objs;
	u32 *h_merged_obj;
};

/**
 * SYNX_MERGE_INDV_PARAMS    : Merge filled with synx_merge_params struct.
 */
enum synx_merge_type {
	SYNX_MERGE_INDV_PARAMS = 0x01,
};

/**
 * struct synx_merge_indv_params - Synx merge indv parameters
 *
 * @h_synxs          : Pointer to a array of synx handles to be merged.
 * @flags            : Merge flags.
 * @num_objs         : Number of synx handles to be merged.
 * @h_merged_obj     : Pointer to synx object handle passed by client.
 *                     Created merged synx handle filled by function.
 * @client_data_hi   : most significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal. Only supported for fence creation.
 * @client_data_lo   : least significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal. Only supported for fence creation.
 * @security_key_hi  : most significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 * @security_key_lo  : least significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 */
struct synx_merge_indv_params {
	u32 *h_synxs;
	enum synx_merge_flags flags;
	u32 num_objs;
	u32 *h_merged_obj;
	u32 client_data_hi;
	u32 client_data_lo;
	u32 security_key_hi;
	u32 security_key_lo;
};

/**
 * struct synx_merge_n_params - Synx merge parameters
 *
 * @type         : Merge params type
 * @indv         : params to create a single merged handle
 */
struct synx_merge_n_params {
	enum synx_merge_type type;
	union {
		struct synx_merge_indv_params indv;
	};
};

/**
 * enum synx_import_type - Import type
 *
 * SYNX_IMPORT_INDV_PARAMS    : Import/Create  filled with synx_import_indv_params struct
 * SYNX_IMPORT_ARR_PARAMS     : Import/Create  filled with synx_import_arr_params struct
 * SYNX_IMPORT_INDV_PARAMS_V2 : Import/Create  filled with synx_import_indv_params_v2 struct
 * SYNX_IMPORT_ARR_PARAMS_V2  : Import/Create  filled with synx_import_arr_params_v2 struct
 */
enum synx_import_type {
	SYNX_IMPORT_INDV_PARAMS = 0x01,
	SYNX_IMPORT_ARR_PARAMS  = 0x02,
	SYNX_IMPORT_INDV_PARAMS_V2 = 0x03,
	SYNX_IMPORT_ARR_PARAMS_V2 = 0x04,
};

/**
 * struct synx_import_indv_params - Synx import indv parameters
 *
 * @new_h_synxs : Pointer to new synx object
 *                (filled by the function)
 *                The new handle/s should be used by importing
 *                process for all synx api operations and
 *                for sharing with FW cores.
 * @flags       : Synx import flags
 * @fence       : Pointer to DMA fence fd or synx handle(NULL in case of create).
 */
struct synx_import_indv_params {
	u32 *new_h_synx;
	enum synx_import_flags flags;
	void *fence;
};

/**
 * struct synx_import_indv_params_v2 - Synx import indv v2 parameters
 *
 * @new_h_synx       : Pointer to new synx object
 *                      (filled by the function)
 *                      The new handle should be used by importing
 *                      process for all synx api operations and
 *                      for sharing with FW cores.
 * @flags            : Synx import flags
 * @fence            : Pointer to DMA fence fd or synx handle, NULL if creating fence
 * @client_data_hi   : most significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal. Only supported for fence creation.
 * @client_data_lo   : least significant 32 bits of the 64-bit client_data propagated
 *                     to waiting client during signal. Only supported for fence creation.
 * @security_key_hi  : most significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 * @security_key_lo  : least significant 32 bits of the 64-bit security_key for authentication.
 *                     If security_key is not required use SYNX_NO_SECURITY_KEY macro.
 */
struct synx_import_indv_params_v2 {
	u32 *new_h_synx;
	enum synx_import_flags flags;
	void *fence;
	u32 client_data_hi;
	u32 client_data_lo;
	u32 security_key_hi;
	u32 security_key_lo;
};

/**
 * struct synx_import_arr_params_v2 - Synx import arr v2 parameters
 *
 * @list        : List of synx_import_indv_params_v2 params
 * @num_fences  : Number of fences or synx handles to be imported/created
 */
struct synx_import_arr_params_v2 {
	struct synx_import_indv_params_v2 *list;
	u32 num_fences;
};

/**
 * struct synx_import_arr_params - Synx import arr parameters
 *
 * @list        : List of synx_import_indv_params
 * @num_fences  : Number of fences or synx handles to be imported/created
 */
struct synx_import_arr_params {
	struct synx_import_indv_params *list;
	u32 num_fences;
};

/**
 * struct synx_import_params - Synx import parameters
 *
 * @type    : Import/Create params type filled by client
 * @indv    : Params to import/create an individual handle or fence
 * @arr     : Params to import/create an array of handles or fences
 * @indv_v2 : Params to import/create an individual handle or fence of v2 params
 * @arr_v2  : Params to import/create an array of handles or fences of v2 params
 */
struct synx_import_params {
	enum synx_import_type type;
	union {
		struct synx_import_indv_params indv;
		struct synx_import_arr_params  arr;
		struct synx_import_indv_params_v2 indv_v2;
		struct synx_import_arr_params_v2 arr_v2;
	};
};

/**
 * struct synx_callback_params - Synx callback parameters
 *
 * @h_synx         : Synx object handle
 * @cb_func        : Pointer to callback func to be invoked.
 * @userdata       : Opaque pointer passed back with callback as data
 * @cancel_cb_func : Pointer to callback to ack cancellation
 * @timeout_ms     : Timeout in ms. SYNX_NO_TIMEOUT if no timeout.
 */
struct synx_callback_params {
	u32 h_synx;
	synx_user_callback_t cb_func;
	void *userdata;
	synx_user_callback_t cancel_cb_func;
	u64 timeout_ms;
};

/**
 * enum synx_resource_type - Synx resource type
 *
 * SYNX_RESOURCE_SOCCP : Enable resources for SOCCP
 */
enum synx_resource_type {
	SYNX_RESOURCE_SOCCP = 0x01,
};

/**
 * enum synx_signal_type - Signal params type
 *
 * SYNX_SIGNAL_INDV_PARAMS : Signal filled with synx_signal_indv_params struct
 * SYNX_SIGNAL_ARR_PARAMS  : Signal filled with synx_signal_arr_params struct
 */
enum synx_signal_type {
	SYNX_SIGNAL_INDV_PARAMS = 0x01,
	SYNX_SIGNAL_ARR_PARAMS  = 0x02,
};

/**
 * struct synx_signal_indv_params - Synx signal indv parameters
 *
 * @h_synx      : Synx object handle
 * @flags       : Synx signal flags, see enum synx_signal_flags for detail
 * @status      : Status of signaling, see enum synx_signal_status for supported statuses, provide
 *                value greater than SYNX_STATE_SIGNALED_MAX for custom notification
 * @client_data : 64-bit client data propagated to waiting clients
 * @signal_idx  : pointer to tx queue write index (filled by the function
 *                if SYNX_SIGNAL_DELAY is set in flags); supported for
 *                SYNX_HW_FENCE clients only, not by other clients
 */
struct synx_signal_indv_params {
	u32 h_synx;
	enum synx_signal_flags flags;
	u32 status;
	u64 client_data;
	u32 *signal_idx;
};

/**
 * struct synx_signal_arr_params - Synx signal arr parameters
 *
 * @list        : List of synx_signal_indv_params
 * @num_fences  : Number of fences or synx handles to be signaled
 */
struct synx_signal_arr_params {
	struct synx_signal_indv_params *list;
	u32 num_fences;
};

/**
 * struct synx_signal_n_params - Synx signal_n parameters
 *
 * @type : Signal params type filled by client
 * @indv : Params to signal an individual handle
 * @arr  : Params to signal an array of handles
 */
struct synx_signal_n_params {
	enum synx_signal_type type;
	union {
		struct synx_signal_indv_params indv;
		struct synx_signal_arr_params  arr;
	};
};

/**
 * enum synx_read_type - Read params type
 *
 * SYNX_READ_INDV_PARAMS : Read filled with synx_read_indv_params struct
 * SYNX_READ_ARR_PARAMS  : Read filled with synx_read_arr_params struct
 */
enum synx_read_type {
	SYNX_READ_INDV_PARAMS = 0x01,
	SYNX_READ_ARR_PARAMS  = 0x02,
};

/**
 * struct synx_read_indv_params - Synx read indv parameters
 *
 * @h_synx      : pointer to synx object handle (filled in by the function)
 * @status      : optional pointer to signal status (filled in by the function if present), see
 *                enum synx_signal_status for supported statuses; if status is greater than
 *                SYNX_STATE_SIGNALED_MAX, then signaling client provided custom notification status
 * @client_data : optional pointer to 64-bit client_data (filled in by the function if present)
 * @timeout_ms  : timeout for object read in ms. 0 for non-blocking read,
 *                SYNX_NO_TIMEOUT if no timeout.
 */
struct synx_read_indv_params {
	u32 *h_synx;
	u32 *status;
	u64 *client_data;
	u64 timeout_ms;
};

/**
 * struct synx_read_arr_params - Synx read arr parameters
 *
 * @list        : List of synx_read_indv_params
 * @num_fences  : Number of fences or synx handles to be read
 */
struct synx_read_arr_params {
	struct synx_read_indv_params *list;
	u32 num_fences;
};

/**
 * struct synx_read_n_params - Synx read_n parameters
 *
 * @type : Read params type filled by client
 * @indv : Params to read an individual handle
 * @arr  : Params to read an array of handles
 */
struct synx_read_n_params {
	enum synx_read_type type;
	union {
		struct synx_read_indv_params indv;
		struct synx_read_arr_params  arr;
	};
};

/**
 * enum synx_get_type - Synx get params type
 *
 * SYNX_GET_STATUS_PARAMS     : Get synx signaling status
 * SYNX_GET_FENCE_PARAMS      : Get native fence associated with synx object
 * SYNX_GET_CLIENT_DATA       : Get 64-bit client metadata associated with synx object
 * SYNX_GET_MAX_GLOBAL_FENCES : Get maximum number of fences used for cross-core signaling
 */
enum synx_get_type {
	SYNX_GET_STATUS_PARAMS = 0x01,
	SYNX_GET_FENCE_PARAMS = 0x02,
	SYNX_GET_CLIENT_DATA = 0x03,
	SYNX_GET_MAX_GLOBAL_FENCES = 0x04,
};

/**
 * struct synx_get_params - Synx get parameters
 *
 * @type              : Get params type filled by client
 * @h_synx            : handle of synx object filled by client
 * @status            : signaling status of synx object, filled by function call
 * @fence             : native fence associated with synx object, filled by function call
 * @client_data       : 64-bit client metadata associated with synx object, filled by function call
 * @max_global_fences : maximum number of fences used for cross-core signaling, filled by
 *                      function call
 */
struct synx_get_params {
	enum synx_get_type type;
	u32 h_synx;
	union {
		enum synx_signal_status status;
		void *fence;
		u64 client_data;
		u64 max_global_fences;
	};
};

/**
 * struct synx_ops - Synx operations
 *
 * @uninitialize        : destroys the client session
 * @create              : creates synx object
 * @async_wait          : registers a callback with synx object
 * @cancel_async_wait   : de-registers a callback with synx oject
 * @signal              : signals synx object
 * @signal_n            : signals n synx objects
 * @merge               : merges multiple synx objects
 * @wait                : waits for a synx object synchronously
 * @read_n              : reads n synx objects
 * @get_status          : returns status of synx object
 * @import              : imports (looks up) synx object from given handle/fence
 * @get_fence           : gets native fence backing synx object
 * @release             : releases synx object
 * @get                 : gets information associated with synx object
 * @release_n           : releases an array of synx objects
 * @merge_n             : merges multiple synx objects with merge_n params
 */
struct synx_ops {
	int (*uninitialize)(struct synx_session *session);
	int (*create)(struct synx_session *session, struct synx_create_params *params);
	int (*async_wait)(struct synx_session *session, struct synx_callback_params *params);
	int (*cancel_async_wait)(struct synx_session *session, struct synx_callback_params *params);
	int (*signal)(struct synx_session *session, u32 h_synx, enum synx_signal_status status);
	int (*signal_n)(struct synx_session *session, struct synx_signal_n_params *params);
	int (*merge)(struct synx_session *session, struct synx_merge_params *params);
	int (*wait)(struct synx_session *session, u32 h_synx, u64 timeout_ms);
	int (*read_n)(struct synx_session *session, struct synx_read_n_params *params);
	int (*get_status)(struct synx_session *session, u32 h_synx);
	int (*import)(struct synx_session *session, struct synx_import_params *params);
	void *(*get_fence)(struct synx_session *session, u32 h_synx);
	int (*release)(struct synx_session *session, u32 h_synx);
	int (*get)(struct synx_session *session, struct synx_get_params *params);
	int (*release_n)(struct synx_session *session, struct synx_release_n_params *params);
	int (*merge_n)(struct synx_session *session, struct synx_merge_n_params *params);
};

/* Kernel APIs */

/* synx_register_ops - Register operations for external synchronization  (NOT SUPPORTED)
 *
 * Register with synx for enabling external synchronization through bind
 *
 * @param params : Pointer to register params
 *
 * @return Status of operation. SYNX_SUCCESS in case of success.
 * -SYNX_INVALID will be returned if params are invalid.
 * -SYNX_NOMEM will be returned if bind ops cannot be registered due to
 * insufficient memory.
 * -SYNX_ALREADY will be returned if type already in use.
 */
int synx_register_ops(const struct synx_register_params *params);

/**
 * synx_deregister_ops - De-register external synchronization operations  (NOT SUPPORTED)
 *
 * @param params : Pointer to register params
 *
 * @return Status of operation. SYNX_SUCCESS in case of success.
 * -SYNX_INVALID will be returned if record not found.
 */
int synx_deregister_ops(const struct synx_register_params *params);

/**
 * synx_initialize - Initializes a new client session
 *
 * @param params : Pointer to session init params
 *
 * @return Client session pointer on success. NULL or error in case of failure.
 */
struct synx_session *synx_initialize(struct synx_initialization_params *params);

/**
 * synx_uninitialize - Destroys the client session
 *
 * @param session : Session ptr (returned from synx_initialize)
 *
 * @return Status of operation. Negative in case of error, SYNX_SUCCESS otherwise.
 */
int synx_uninitialize(struct synx_session *session);

/**
 * synx_create - Creates a synx object
 *
 * Creates a new synx obj and returns the handle to client. There can be
 * maximum of 4095 global synx handles or local synx handles across
 * sessions.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Pointer to create params
 *
 * @return Status of operation. Negative in case of error, SYNX_SUCCESS otherwise.
 */
int synx_create(struct synx_session *session, struct synx_create_params *params);

/**
 * synx_async_wait - Registers a callback with a synx object
 *
 * Clients can register maximum of 64 callbacks functions per
 * synx session. Clients should register callback functions with minimal computation.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Callback params.
 *                  cancel_cb_func in callback params is optional with this API.
 *
 * @return Status of operation. Negative in case of error, SYNX_SUCCESS otherwise.
 */
int synx_async_wait(struct synx_session *session, struct synx_callback_params *params);

/**
 * synx_cancel_async_wait - De-registers a callback with a synx object
 *
 * This API will cancel one instance of callback function (mapped
 * with userdata and h_synx) provided in cb_func of callback params.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Callback params
 *
 * @return Status of operation.Negative in case of error, SYNX_SUCCESS otherwise.
 */
int synx_cancel_async_wait(struct synx_session *session,
	struct synx_callback_params *params);

/**
 * synx_signal - Signals a synx object with the status argument.
 *
 * This function will signal the synx object referenced by h_synx
 * and invoke any external binding synx objs.
 * The status parameter will indicate whether the entity
 * performing the signaling wants to convey an error case or a success case.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param h_synx  : Synx object handle
 * @param status  : Status of signaling.
 *                  Clients can send custom signaling status
 *                  beyond SYNX_STATE_SIGNALED_MAX.
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_signal(struct synx_session *session, u32 h_synx,
	enum synx_signal_status status);

/*
 * synx_signal_n – Signals n synx objects
 *
 * Function signals an individual handle or N handles
 *
 * @param session      : Session ptr (returned from synx_initialize)
 * @param params       : Pointer to signal params
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_signal_n(struct synx_session *session, struct synx_signal_n_params *params);

/**
 * synx_merge - Merges multiple synx objects
 *
 * This function will merge multiple synx objects into a synx group.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Merge params
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_merge(struct synx_session *session, struct synx_merge_params *params);

/**
 * synx_merge_n - Merges multiple synx objects with synx_merge_n params
 *
 * This function will merge multiple synx objects into a synx group based on synx_merge_type.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Merge_n params
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_merge_n(struct synx_session *session, struct synx_merge_n_params *params);

/**
 * synx_wait - Waits for a synx object synchronously
 *
 * Does a wait on the synx object identified by h_synx for a maximum
 * of timeout_ms milliseconds. Must not be called from interrupt context as
 * this API can sleep.
 *
 * @param session    : Session ptr (returned from synx_initialize)
 * @param h_synx     : Synx object handle to be waited upon
 * @param timeout_ms : Timeout in ms
 *
 * @return Status of synx object if handle is signaled. -SYNX_INVAL if synx object
 * is in bad state or arguments are invalid, -SYNX_TIMEOUT if wait times out.
 */
int synx_wait(struct synx_session *session, u32 h_synx, u64 timeout_ms);

/*
 * synx_read_n - Reads n synx objects
 *
 * Function reads an individual handle, signaling status, and 64-bit client_data
 * with timeout specified by params.
 *
 * @param session      : Session ptr (returned from synx_initialize)
 * @param params       : Pointer to read_n params
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_read_n(struct synx_session *session, struct synx_read_n_params *params);

/**
 * synx_get_status - Returns the status of the synx object.
 *
 * This API should not be used in polling mode to know if the handle
 * is signaled or not.
 * Clients need to explicitly wait using synx_wait() or synx_async_wait()
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param h_synx  : Synx object handle
 *
 * @return Status of the synx object
 */
int synx_get_status(struct synx_session *session, u32 h_synx);

/**
 * synx_import - Imports (looks up) synx object from given handle or fence,
 * or creates a new handle if the fence passed is NULL.
 * *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Pointer to import params
 *
 * @return Status of operation. Negative in case of failure, SYNX_SUCCESS otherwise.
 */
int synx_import(struct synx_session *session, struct synx_import_params *params);

/**
 * synx_get_fence - Get the native fence backing the synx object
 *
 * Synx framework will take additional reference on dma fence and returns the native
 * fence. Clients need to release additional reference explicitly by calling kref_put.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param h_synx  : Synx object handle
 *
 * @return Fence pointer in case of success and NULL in case of failure.
 */
void *synx_get_fence(struct synx_session *session, u32 h_synx);

/**
 * synx_release - Release the synx object.
 *
 * Every created, imported or merged synx object should be released.
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param h_synx  : Synx object handle to be destroyed
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_release(struct synx_session *session, u32 h_synx);

/**
 * @brief: Releases a list of synx objects.
 *         Every created, imported or merged synx object should be
 *         released.
 *
 * @param pSession   : Session ptr (returned from synx_initialize)
 * @param pParams    : pointer to release params
 *
 * @return Status of operation. Negative if at least one of the handle release failed,
 * SYNX_SUCCESS otherwise. This API will continue to release rest of the handles,
 * even if release of some of the handles in the list failed. The status of individual
 * failure can be seen in the result member of the structure synx_release_indv_params.
 */
int synx_release_n(struct synx_session *pSession, struct synx_release_n_params *pParams);

/**
 * synx_recover - Recover any possible handle leaks
 *
 * Function should be called on HW hang/reset to
 * recover the Synx handles shared. This cleans up
 * synx handles owned by subsystem under hang/reset, and avoids
 * potential resource leaks.
 *
 * Function does not destroy the session, but only
 * recover synx handles belonging to the session.
 * Synx session would still be active and clients
 * need to destroy the session explicitly through
 * synx_uninitialize API.
 *
 * All the unsignaled handles owned/imported by the core at the time of reset
 * will be signaled by synx framework on behalf of hung core with SYNX_STATE_SIGNALED_SSR.
 *
 * @param id : Client ID of core to recover
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_recover(enum synx_client_id id);

/**
 * synx_enable_resources - enable any resources needed
 * for the synx client
 *
 * Function should be called with enable=true when
 * client is using fences and with enable=false when
 * client is not using fences, e.g. at use-case boundary.
 *
 * @param id : Client ID of core for which resources are enabled
 * @param resource : type of synx resource to enable
 * @param enable : true if enabling resources, false to disable resources
 *
 * @return Status of operation. Negative in case of error. SYNX_SUCCESS otherwise.
 */
int synx_enable_resources(enum synx_client_id id, enum synx_resource_type resource, bool enable);

/*
 * synx_get - Get relevant information on synx object
 *
 * @param session : Session ptr (returned from synx_initialize)
 * @param params  : Pointer to get params
 *
 * @return Status of operation. Negative in case of error, SYNX_SUCCESS otherwise.
 */
int synx_get(struct synx_session *session, struct synx_get_params *params);

#endif /* __SYNX_API_H__ */
