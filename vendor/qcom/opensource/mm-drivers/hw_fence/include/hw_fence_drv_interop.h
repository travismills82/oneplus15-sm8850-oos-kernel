/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HW_FENCE_INTEROP_H
#define __HW_FENCE_INTEROP_H

#include <synx_api.h>

extern struct hw_fence_driver_data *hw_fence_drv_data;
extern struct synx_hwfence_interops synx_interops;

/**
 * hw_fence_interop_add_cb() - if Synx is enabled, adds callback without calling enable_signaling;
 * else calls dma_fence_add_callback
 *
 * @param fence  : dma-fence structure
 * @param cb     : callback to register
 * @param func   : the function to call
 * @return 0 upon success, -ENOENT if already signaled, -EINVAL in case of error.
 */
int hw_fence_interop_add_cb(struct dma_fence *fence,
	struct dma_fence_cb *cb, dma_fence_func_t func);

/**
 * hw_fence_interop_to_synx_status() - Converts hw-fence status code to synx status code
 *
 * @param code  : hw-fence status code
 * @return synx status code corresponding to hw-fence status code
 */
int hw_fence_interop_to_synx_status(int hw_fence_status_code);

/**
 * hw_fence_interop_to_synx_signal_status() - Converts hw-fence flags and error to
 * synx signaling status
 *
 * @param flags  : hw-fence flags
 * @param error  : hw-fence error
 *
 * @return synx signaling status
 */
u32 hw_fence_interop_to_synx_signal_status(u32 flags, u32 error);

/**
 * hw_fence_interop_to_hw_fence_error() - Convert synx signaling status to hw-fence error
 *
 * @param status  : synx signaling status
 * @return hw-fence error
 */
u32 hw_fence_interop_to_hw_fence_error(u32 status);

/**
 * hw_fence_interop_create_fence_from_import() - Creates hw-fence if necessary during synx_import,
 * e.g. if there is no backing hw-fence for a synx fence.
 *
 * @param params  : pointer to import params
 * @return SYNX_SUCCESS upon success, -SYNX_INVALID if failed
 */
int hw_fence_interop_create_fence_from_import(struct synx_import_indv_params *params);

/**
 * hw_fence_interop_share_handle_status() - updates HW fence table with synx handle
 * (if not already signaled) and return hw-fence handle by populating params.new_h_synx
 * and returning signal status
 *
 * @param params  : pointer to import params
 * @param h_synx  : synx handle
 * @param signal_status: signalin status of fence
 *
 * @return SYNX_SUCCESS upon success, -SYNX_INVALID if failed
 */
int hw_fence_interop_share_handle_status(struct synx_import_indv_params *params, u32 h_synx,
	u32 *signal_status);

/**
 * hw_fence_interop_get_fence() – return the dma-fence associated with the given handle
 *
 * @param h_synx : hw-fence handle
 *
 * @return dma-fence associated with hw-fence handle. Null or error pointer in case of error.
 */
void *hw_fence_interop_get_fence(u32 h_synx);

/**
 * hw_fence_interop_signal_synx_fence() – Signal h_synx with hw-fence error, used to signal synx
 * waiting clients from hw-fence driver directly, e.g. for ssr use cases
 *
 * @param drv_data : driver data
 * @param is_soccp_ssr : signaling h_synx with hw-fence error in soccp ssr scenario
 * @param h_synx : synx handle
 * @param error : hw-fence error
 *
 * @return 0 upon success, -EINVAL if failed
 */
int hw_fence_interop_signal_synx_fence(struct hw_fence_driver_data *drv_data, bool is_soccp_ssr,
	u32 h_synx, u32 error);


/**
 * hw_fence_interop_notify_recover() – Request Synx Driver to perform recovery in SOCCP SSR
 * scenario, i.e. signal all inter-op fences with Synx producer and unlock any lock held by SOCCP
 * at time of crash.
 *
 * @param drv_data : driver data
 *
 * @return 0 upon success, -EINVAL if failed
 */
int hw_fence_interop_notify_recover(struct hw_fence_driver_data *drv_data);
#endif /* __HW_FENCE_INTEROP_H */
