/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_CVP_KAANAPALI_HAL_H__
#define __H_CVP_KAANAPALI_HAL_H__

#include "cvp_core_hfi.h"
#include "cvp_hfi.h"
#include "cvp_hfi_io.h"
#include "msm_cvp_clocks.h"
#include "msm_cvp_debug.h"
#include <linux/delay.h>

static const char *const mid_names_kaanapali[25] = {
	"CVP_FW",
	"ARP_DATA",
	"CDM_DATA",
	"Invalid",
	"CVP_MPU_PIXEL",
	"CVP_MPU_NON_PIXEL",
	"Invalid",
	"Invalid",
	"CVP_FDU_PIXEL",
	"CVP_FDU_NON_PIXEL",
	"Invalid",
	"Invalid",
	"CVP_GCE_PIXEL",
	"CVP_GCE_NON_PIXEL",
	"Invalid",
	"Invalid",
	"CVP_TOF_PIXEL",
	"CVP_TOF_NON_PIXEL",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"CVP_RGE_NON_PIXEL",
};

int set_kaanapali_hal_functions(void);
#endif
