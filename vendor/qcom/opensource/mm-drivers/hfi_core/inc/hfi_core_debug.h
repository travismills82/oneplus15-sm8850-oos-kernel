/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_CORE_DEBUG_H__
#define __HFI_CORE_DEBUG_H__

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/printk.h>

struct hfi_core_drv_data;

extern u32 msm_hfi_core_debug_level;
extern bool msm_hfi_fail_client_0_reg;
extern u32 msm_hfi_packet_cmd_id;
#if IS_ENABLED(CONFIG_DEBUG_FS)
extern bool hfi_core_loop_back_mode_enable;
#endif

enum hfi_core_drv_prio {
	/* High density debug messages (noisy) */
	HFI_CORE_HIGH = 0x000001,
	/* Low density debug messages */
	HFI_CORE_LOW = 0x000002,
	/* Informational prints */
	HFI_CORE_INFO = 0x000004,
	/* Initialization logs */
	HFI_CORE_INIT = 0x00008,
	HFI_CORE_PRINTK = 0x010000,
};

#define dprintk(__level, __fmt, ...) \
	do { \
		if (msm_hfi_core_debug_level & __level) \
			if (msm_hfi_core_debug_level & HFI_CORE_PRINTK) \
				pr_err(__fmt, ##__VA_ARGS__); \
	} while (0)


#define HFI_CORE_ERR(fmt, ...) \
	pr_err("[hfi_core_error:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

#define HFI_CORE_ERR_ONCE(fmt, ...) \
	pr_err_once("[hfi_core_error:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

#define HFI_CORE_DBG_H(fmt, ...) \
	dprintk(HFI_CORE_HIGH, "[hfi_core_dbgh:%s:%d]"fmt, __func__, \
		__LINE__, ##__VA_ARGS__)

#define HFI_CORE_DBG_L(fmt, ...) \
	dprintk(HFI_CORE_LOW, "[hfi_core_dbgl:%s:%d]"fmt, __func__, \
		__LINE__, ##__VA_ARGS__)

#define HFI_CORE_DBG_INFO(fmt, ...) \
	dprintk(HFI_CORE_INFO, "[hfi_core_dbgi:%s:%d]"fmt, __func__, \
		__LINE__, ##__VA_ARGS__)

#define HFI_CORE_DBG_INIT(fmt, ...) \
	dprintk(HFI_CORE_INIT, "[hfi_core_dbg:%s:%d]"fmt, __func__, \
		__LINE__, ##__VA_ARGS__)

#define HFI_CORE_DBG_DUMP(prio, fmt, ...) \
	dprintk(prio, "[hfi_core_dbgd:%s:%d]"fmt, __func__, __LINE__, \
		##__VA_ARGS__)

#define HFI_CORE_WARN(fmt, ...) \
	pr_warn("[hfi_core_warn:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

/**
 * hfi_core_dbg_debugfs_register() - Register Debugfs for HFI core.
 *
 * This call registers the debugfs handle for hfi core module
 *
 * Return: 0 on success or negative errno
 */
int hfi_core_dbg_debugfs_register(struct hfi_core_drv_data *drv_data);

/**
 * hfi_core_dbg_debugfs_unregister() - Unregister Debugfs for HFI core.
 *
 * This call deregisters the debugfs handle for hfi core module.
 *
 * Return: 0 on success or negative errno
 */
void hfi_core_dbg_debugfs_unregister(struct hfi_core_drv_data *drv_data);

#endif // __HFI_CORE_DEBUG_H__
