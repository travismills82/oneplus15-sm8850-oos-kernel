// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */


#include "cvp_hawi_hal.h"

extern struct cvp_hal_ops hal_ops;

void __check_tensilica_in_reset_hawi(struct iris_hfi_device *device)
{
}

void setup_dsp_uc_memmap_vpu5_hawi(struct iris_hfi_device *device)
{
}

void interrupt_init_iris2_hawi(struct iris_hfi_device *device)
{
}

int __check_ctl_power_on_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __check_core_power_on_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __power_on_controller_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __power_on_core_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __power_off_core_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __power_off_controller_hawi(struct iris_hfi_device *device)
{
	return 0;
}

void __print_sidebandmanager_regs_hawi(struct iris_hfi_device *device)
{
}

int __enable_hw_power_collapse_hawi(struct iris_hfi_device *device)
{
	return 0;
}

int __set_registers_hawi(struct iris_hfi_device *device)
{
	return 0;
}

void __print_reg_details_errlog3_low_hawi(u32 val)
{
}

void __dump_noc_regs_hawi(struct iris_hfi_device *device)
{
}

void __noc_error_info_iris2_hawi(struct iris_hfi_device *device)
{
}

int set_hawi_hal_functions(void)
{
	hal_ops.interrupt_init = interrupt_init_iris2_hawi;
	hal_ops.setup_dsp_uc_memmap = setup_dsp_uc_memmap_vpu5_hawi;
	hal_ops.power_off_controller = __power_off_controller_hawi;
	hal_ops.power_off_core = __power_off_core_hawi;
	hal_ops.power_on_controller = __power_on_controller_hawi;
	hal_ops.power_on_core = __power_on_core_hawi;
	hal_ops.noc_error_info = __noc_error_info_iris2_hawi;
	hal_ops.check_ctl_power_on = __check_ctl_power_on_hawi;
	hal_ops.check_core_power_on = __check_core_power_on_hawi;
	hal_ops.print_sbm_regs = __print_sidebandmanager_regs_hawi;
	hal_ops.enable_hw_power_collapse = __enable_hw_power_collapse_hawi;
	hal_ops.set_registers = __set_registers_hawi;
	hal_ops.dump_noc_regs = __dump_noc_regs_hawi;
	hal_ops.check_tensilica_in_reset = __check_tensilica_in_reset_hawi;
	return 0;
}
