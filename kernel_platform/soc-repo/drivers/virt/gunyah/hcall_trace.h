/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _HCALL_TRACE_H
#define _HCALL_TRACE_H

#include <linux/arm-smccc.h>
#include <linux/gunyah.h>

#define GUNYAH_HYPERCALL(fn)                                      \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64, \
			   ARM_SMCCC_OWNER_VENDOR_HYP, fn)

#define GUNYAH_HYPERCALL_TRACE_BUF_MGMT		GUNYAH_HYPERCALL(0x803F)

/**
 * gunyah_hypercall_update_trace_flag() - Config the trace class flags
 * @set_flags: Trace class flags to set
 * @clear_flags: Trace class flags to clear
 * @enabled_flags: Filled with the trace class flags that are enabled
 *
 * See also:
 * https://github.com/quic/gunyah-hypervisor/blob/develop/docs/api/gunyah_api.md#trace-buffer-management
 */
static enum gunyah_error gunyah_hypercall_update_trace_flag(u64 set_flags,
							    u64 clear_flags,
							    u64 *enabled_flags)
{
	struct arm_smccc_res res;

	arm_smccc_1_1_hvc(GUNYAH_HYPERCALL_TRACE_BUF_MGMT, set_flags,
			  clear_flags, 0, &res);

	if (res.a0 == GUNYAH_ERROR_OK)
		*enabled_flags = res.a1;

	return res.a0;
}

/**
 * gunyah_hypercall_config_trace_buf_notify() - Config the trace buffer notification
 * @notify_enable: Enable or disable the trace buffer notification
 *
 * See also:
 * https://github.com/quic/gunyah-hypervisor/blob/develop/docs/api/gunyah_api.md#trace-buffer-management
 */
static enum gunyah_error
gunyah_hypercall_config_trace_buf_notify(bool notify_enable)
{
	struct arm_smccc_res res;

	arm_smccc_1_1_hvc(GUNYAH_HYPERCALL_TRACE_BUF_MGMT, notify_enable, 0, 1,
			  &res);

	return res.a0;
}


#endif
