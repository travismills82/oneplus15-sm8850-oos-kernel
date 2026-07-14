/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.​
 */

#ifndef __H_CVP_HFI_H__
#define __H_CVP_HFI_H__

#ifdef CONFIG_EVA_SUN
#include "cvp_hfi_v2.h"
#endif

#ifdef CONFIG_EVA_CANOE
#include "cvp_hfi_v2.h"
#endif

#include "msm_cvp_resources.h"

enum tzbsp_subsys_state {
	TZ_SUBSYS_STATE_SUSPEND = 0,
	TZ_SUBSYS_STATE_RESUME = 1,
	TZ_SUBSYS_STATE_RESTORE_THRESHOLD = 2,
};
enum core_gdsc_dest {
	TO_SW_CTRL = 0x0,
	TO_HW_CTRL = 0x1
};
struct iris_hfi_device;
int __tzbsp_set_cvp_state(enum tzbsp_subsys_state state);
int __resume(struct iris_hfi_device *device);
int __response_handler(struct iris_hfi_device *device);
void __write_register(struct iris_hfi_device *device,
	u32 reg, u32 value);
int __read_register(struct iris_hfi_device *device, u32 reg);
int __read_tcsr_register(struct iris_hfi_device *device, u32 reg);
int __read_gcc_register(struct iris_hfi_device *device, u32 reg);
int switch_core_gdsc_mode(struct iris_hfi_device *device, enum core_gdsc_dest dest);
int __acquire_regulator(struct regulator_info *rinfo,
	struct iris_hfi_device *device);
int __hand_off_regulator(struct regulator_info *rinfo);
int __hand_off_regulators(struct iris_hfi_device *device);
int __enable_gdsc(struct iris_hfi_device *device, const char *name);
int __disable_gdsc(struct iris_hfi_device *device, const char *name);
void __print_reg_details_errlog1_high(u32 val);
void __err_log(bool logging, u32 *data, const char *name, u32 val);
int __reset_control_assert_name(struct iris_hfi_device *device, const char *name);
int __reset_control_deassert_name(struct iris_hfi_device *device, const char *name);
int __reset_control_acquire(struct iris_hfi_device *device, const char *name);
int __reset_control_release(struct iris_hfi_device *device, const char *name);
int __disable_hw_power_collapse(struct iris_hfi_device *device);
void __print_sfr_msg(struct iris_hfi_device *device);

u32 msm_cvp_set_fw_version(char *image_version);
#endif
