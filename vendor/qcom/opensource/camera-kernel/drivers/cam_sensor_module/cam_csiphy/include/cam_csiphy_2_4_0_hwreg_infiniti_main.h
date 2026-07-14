/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_CSIPHY_2_4_0_HWREG_INFINITI_MAIN_H_
#define _CAM_CSIPHY_2_4_0_HWREG_INFINITI_MAIN_H_

#include "../cam_csiphy_dev.h"

struct cam_csiphy_aon_sel_params_t aon_cam_select_params_2_4_0_infiniti_main = {
	.aon_cam_sel_offset[0] = 0x01E0,
	.aon_cam_sel_offset[1] = 0x01E4,
	.cam_sel_mask = BIT(0),
	.mclk_sel_mask = BIT(8),
};

struct cam_cphy_dphy_status_reg_params_t status_regs_2_4_0_infiniti_main = {
	.csiphy_3ph_status0_offset = 0x0340,
	.csiphy_2ph_status0_offset = 0x00C0,
	.refgen_status_offset = 0x1184,
	.cphy_lane_status = {0x0358, 0x0758, 0x0B58},
	.csiphy_3ph_status_size = 24,
	.csiphy_2ph_status_size = 20,
};

struct csiphy_reg_t csiphy_lane_en_reg_2_4_0_infiniti_main[] = {
	{0x1014, 0x00, 0x00, CSIPHY_LANE_ENABLE},
};

struct csiphy_reg_t csiphy_common_reg_2_4_0_infiniti_main[] = {
	{0x1084, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x108C, 0x00, 0x01, CSIPHY_2PH_REGS},
	{0x108C, 0x0E, 0x01, CSIPHY_3PH_REGS},
	{0x101C, 0x7A, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1018, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t csiphy_reset_enter_reg_2_4_0_infiniti_main[] = {
	{0x1000, 0x01, 0x14, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t csiphy_reset_exit_reg_2_4_0_infiniti_main[] = {
	{0x1000, 0x02, 0x0A, CSIPHY_2PH_REGS},
	{0x1000, 0x00, 0x0A, CSIPHY_2PH_COMBO_REGS},
	{0x1000, 0x0E, 0x0A, CSIPHY_3PH_REGS},
};

struct csiphy_reg_t csiphy_irq_reg_2_4_0_infiniti_main[] = {
	{0x102c, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1030, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1034, 0xfb, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1038, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x103c, 0x7f, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1040, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1044, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1048, 0xef, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x104c, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1050, 0xff, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x1054, 0xff, 0x64, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t csiphy_2ph_v2_4_0_reg_infiniti_main[] = {
	{0x0094, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00A0, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x0F, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0098, 0x08, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0094, 0x07, 0xd1, CSIPHY_DEFAULT_PARAMS},
	{0x0030, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0028, 0x0A, 0x00, CSIPHY_2PH_SEC_CLK_LN_SETTINGS},
	{0x0000, 0x8C, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0038, 0xFE, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x002C, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0034, 0x0F, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x001C, 0x0A, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x60, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x003C, 0xB8, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0004, 0x0C, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0020, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0008, 0x19, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0010, 0x52, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0094, 0xD7, 0x00, CSIPHY_SKEW_CAL},
	{0x005C, 0x54, 0x00, CSIPHY_SKEW_CAL},
	{0x0060, 0xFD, 0x00, CSIPHY_SKEW_CAL},
	{0x0064, 0x7F, 0x00, CSIPHY_SKEW_CAL},
};

struct csiphy_reg_t csiphy_2ph_v2_4_0_clk_ln_reg_infiniti_main[] = {
	{0x0028, 0x00, 0x00, CSIPHY_2PH_SEC_CLK_LN_SETTINGS},
	{0x0000, 0x80, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x000C, 0xFF, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0038, 0x1F, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t csiphy_3ph_v2_4_0_reg_infiniti_main[] = {
	{0x0094, 0x0D, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00F4, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00F8, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00FC, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00F0, 0xEF, 0xd3, CSIPHY_DEFAULT_PARAMS},

	{0x0004, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00E4, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00E8, 0x7F, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00EC, 0x7F, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0018, 0x3E, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x001C, 0x41, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0020, 0x41, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0024, 0x7F, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0028, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x002C, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0064, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0044, 0xB2, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0110, 0x35, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00BC, 0xD0, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0054, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0040, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0060, 0xA8, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0084, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_80Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1F, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x02, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x6B, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_100Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0xB6, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x01, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x6B, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_200Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0xE4, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x33, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_300Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x9E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_350Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x8A, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_400Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x7B, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_500Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x66, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_600Msps_RFI_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x03, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x58, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_700Msps_RFI_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x03, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x4E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_600Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x58, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_700Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x4E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x20, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_800Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x46, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_900Msps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x40, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_1p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x3C, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_1p2Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x35, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_1p5Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x2E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_1p7Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x2A, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x09, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x27, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p1Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x26, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p35Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x23, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p5Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x22, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p6Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x22, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_2p8Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x21, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_3p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x20, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_3p3Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_3p5Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1E, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_4p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1C, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_4p5Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1B, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_5p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x1A, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_5p5Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x01, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x19, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t datarate_240_6p0Gsps_infiniti_main[] = {
	/* AFE Settings */
	{0x0068, 0xF1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x006C, 0x18, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0070, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0074, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0078, 0x20, 0x00, CSIPHY_CDR_LN_SETTINGS},
	{0x0088, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x008C, 0x65, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0090, 0x02, 0x00, CSIPHY_DEFAULT_PARAMS},
	/* Datarate Sensitive */
	{0x000C, 0x19, 0x00, CSIPHY_SETTLE_CNT_LOWER_BYTE},
	{0x0008, 0x00, 0x00, CSIPHY_SETTLE_CNT_HIGHER_BYTE},
	{0x0010, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0014, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
};

static struct data_rate_reg_info_t data_rate_settings_2_4_0_infiniti_main[] = {
	{
		/* ((80 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 182400000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_80Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_80Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_80Msps_infiniti_main,
	},
	{
		/* ((100 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 228000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_100Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_100Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_100Msps_infiniti_main,
	},
	{
		/* ((200 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 456000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_200Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_200Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_200Msps_infiniti_main,
	},
	{
		/* ((300 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 684000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_300Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_300Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_300Msps_infiniti_main,
	},
	{
		/* ((350 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 798000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_350Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_350Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_350Msps_infiniti_main,
	},
	{
		/* ((400 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 912000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_400Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_400Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_400Msps_infiniti_main,
	},
	{
		/* ((500 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 1140000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_500Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_500Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_500Msps_infiniti_main,
	},
	{
		/* ((600 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 1368000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_600Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_600Msps_RFI_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_600Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_600Msps_infiniti_main,
	},
	{
		/* ((700 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 1596000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_700Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_700Msps_RFI_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_700Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_700Msps_infiniti_main,
	},
	{
		/* ((800 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 1824000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_800Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_800Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_800Msps_infiniti_main,
	},
	{
		/* ((900 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 2052000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_900Msps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_900Msps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_900Msps_infiniti_main,
	},
	{
		/* ((1000 MSpS) * (10^6) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 2280000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_1p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_1p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_1p0Gsps_infiniti_main,
	},
	{
		/* ((1.2 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 2736000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_1p2Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_1p2Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_1p2Gsps_infiniti_main,
	},
	{
		/* ((1.5 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 3420000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_1p5Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_1p5Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_1p5Gsps_infiniti_main,
	},
	{
		/* ((1.7 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 3876000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_1p7Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_1p7Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_1p7Gsps_infiniti_main,
	},
	{
		/* ((2.0 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 4560000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p0Gsps_infiniti_main,
	},
	{
		/* ((2.1 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 4788000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p1Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p1Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p1Gsps_infiniti_main,
	},
	{
		/* ((2.35 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 5358000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p35Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p35Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p35Gsps_infiniti_main,
	},
	{
		/* ((2.5 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 5700000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p5Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p5Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p5Gsps_infiniti_main,
	},
	{
		/* ((2.6 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 5928000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p6Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p6Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p6Gsps_infiniti_main,
	},
	{
		/* ((2.8 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 6384000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_2p8Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_2p8Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_2p8Gsps_infiniti_main,
	},
	{
		/* ((3.0 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 6840000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_3p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_3p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_3p0Gsps_infiniti_main,
	},
	{
		/* ((3.3 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 7524000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_3p3Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_3p3Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_3p3Gsps_infiniti_main,
	},
	{
		/* ((3.5 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 7980000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_3p5Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_3p5Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_3p5Gsps_infiniti_main,
	},
	{
		/* ((4.0 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 9120000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_4p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_4p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_4p0Gsps_infiniti_main,
	},
	{
		/* ((4.5 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 10260000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_4p5Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_4p5Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_4p5Gsps_infiniti_main,
	},
	{
		/* ((5.0 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 11400000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_5p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_5p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_5p0Gsps_infiniti_main,
	},
	{
		/* ((5.5 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 12540000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_5p5Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_5p5Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_5p5Gsps_infiniti_main,
	},
	{
		/* ((6.0 GSpS) * (10^9) * (2.28 bits/symbol)) rounded value */
		.bandwidth = 13680000000,
		.data_rate_reg_array_size = ARRAY_SIZE(datarate_240_6p0Gsps_infiniti_main),
		.data_rate_reg_array[0][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[1][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[2][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[3][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[4][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[5][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[6][0] = datarate_240_6p0Gsps_infiniti_main,
		.data_rate_reg_array[7][0] = datarate_240_6p0Gsps_infiniti_main,
	},
};

struct csiphy_reg_t bist_3ph_arr_2_4_0_infiniti_main[] = {
	{0x0030, 0x1C, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0034, 0xFA, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0038, 0xD4, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x003C, 0x59, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0058, 0x10, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00C8, 0xAA, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00D0, 0xAA, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00D4, 0x64, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x00D8, 0x3E, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0048, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x004C, 0x07, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0050, 0x00, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0044, 0xB1, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x005C, 0x04, 0x00, CSIPHY_DEFAULT_PARAMS},
	{0x0040, 0x85, 0x00, CSIPHY_DEFAULT_PARAMS},
};

struct csiphy_reg_t bist_status_arr_2_4_0_infiniti_main[] = {
	{0x0344, 0x00, 0x00, CSIPHY_3PH_REGS},
	{0x0744, 0x00, 0x00, CSIPHY_3PH_REGS},
	{0x0B44, 0x00, 0x00, CSIPHY_3PH_REGS},
	{0x00C0, 0x00, 0x00, CSIPHY_2PH_REGS},
	{0x04C0, 0x00, 0x00, CSIPHY_2PH_REGS},
	{0x08C0, 0x00, 0x00, CSIPHY_2PH_REGS},
	{0x0CC0, 0x00, 0x00, CSIPHY_2PH_REGS},
};

struct bist_reg_settings_t bist_setting_2_4_0_infiniti_main = {
	.error_status_val_3ph = 0x10,
	.error_status_val_2ph = 0x10,
	.set_status_update_3ph_base_offset = 0x0240,
	.set_status_update_2ph_base_offset = 0x0050,
	.bist_status_3ph_base_offset = 0x0344,
	.bist_status_2ph_base_offset = 0x00C0,
	.bist_sensor_data_3ph_status_base_offset = 0x0340,
	.bist_counter_3ph_base_offset = 0x0348,
	.bist_counter_2ph_base_offset = 0x00C8,
	.number_of_counters = 2,
	.num_3ph_bist_settings = ARRAY_SIZE(bist_3ph_arr_2_4_0_infiniti_main),
	.bist_3ph_settings_arry = bist_3ph_arr_2_4_0_infiniti_main,
	.bist_2ph_settings_arry = NULL,
	.num_2ph_bist_settings = 0,
	.num_status_reg = ARRAY_SIZE(bist_status_arr_2_4_0_infiniti_main),
	.bist_status_arr = bist_status_arr_2_4_0_infiniti_main,
};

struct data_rate_settings_t data_rate_delta_table_2_4_0_infiniti_main = {
	.num_data_rate_settings = ARRAY_SIZE(data_rate_settings_2_4_0_infiniti_main),
	.data_rate_settings = data_rate_settings_2_4_0_infiniti_main,
};

struct csiphy_reg_parms_t csiphy_v2_4_0_infiniti_main = {
	.mipi_csiphy_interrupt_status0_addr = 0x1138,
	.mipi_csiphy_interrupt_clear0_addr = 0x1058,
	.mipi_csiphy_glbl_irq_cmd_addr = 0x1028,
	.size_offset_betn_lanes = 0x400,
	.status_reg_params = &status_regs_2_4_0_infiniti_main,
	.csiphy_common_reg_array_size = ARRAY_SIZE(csiphy_common_reg_2_4_0_infiniti_main),
	.csiphy_reset_enter_array_size = ARRAY_SIZE(csiphy_reset_enter_reg_2_4_0_infiniti_main),
	.csiphy_reset_exit_array_size = ARRAY_SIZE(csiphy_reset_exit_reg_2_4_0_infiniti_main),
	.csiphy_2ph_config_array_size = ARRAY_SIZE(csiphy_2ph_v2_4_0_reg_infiniti_main),
	.csiphy_2ph_clk_cfg_array_size = ARRAY_SIZE(csiphy_2ph_v2_4_0_clk_ln_reg_infiniti_main),
	.csiphy_3ph_config_array_size = ARRAY_SIZE(csiphy_3ph_v2_4_0_reg_infiniti_main),
	.csiphy_interrupt_status_size = ARRAY_SIZE(csiphy_irq_reg_2_4_0_infiniti_main),
	.csiphy_num_common_status_regs = 20,
	.aon_sel_params = &aon_cam_select_params_2_4_0_infiniti_main,
};

struct csiphy_ctrl_t ctrl_reg_2_4_0_infiniti_main = {
	.csiphy_common_reg = csiphy_common_reg_2_4_0_infiniti_main,
	.csiphy_2ph_reg = csiphy_2ph_v2_4_0_reg_infiniti_main,
	.csiphy_2ph_clk_ln_reg = csiphy_2ph_v2_4_0_clk_ln_reg_infiniti_main,
	.csiphy_3ph_reg = csiphy_3ph_v2_4_0_reg_infiniti_main,
	.csiphy_reg = &csiphy_v2_4_0_infiniti_main,
	.csiphy_irq_reg = csiphy_irq_reg_2_4_0_infiniti_main,
	.csiphy_reset_enter_regs = csiphy_reset_enter_reg_2_4_0_infiniti_main,
	.csiphy_reset_exit_regs = csiphy_reset_exit_reg_2_4_0_infiniti_main,
	.csiphy_lane_config_reg = csiphy_lane_en_reg_2_4_0_infiniti_main,
	.csiphy_ln_offsets[DPHY_DATA_LANE_POS_0] = 0x0000,
	.csiphy_ln_offsets[DPHY_DATA_LANE_POS_1] = 0x0400,
	.csiphy_ln_offsets[DPHY_DATA_LANE_POS_2] = 0x0800,
	.csiphy_ln_offsets[DPHY_DATA_LANE_POS_3] = 0x0C00,
	.csiphy_ln_offsets[DPHY_CLOCK_LANE_POS] = 0x0E00,
	.csiphy_ln_offsets[CPHY_LANE_POS_0] = 0x0200,
	.csiphy_ln_offsets[CPHY_LANE_POS_1] = 0x0600,
	.csiphy_ln_offsets[CPHY_LANE_POS_2] = 0x0A00,
	.data_rates_settings_table = &data_rate_delta_table_2_4_0_infiniti_main,
	.csiphy_bist_reg = &bist_setting_2_4_0_infiniti_main,
	.getclockvoting = get_clk_voting_dynamic,
};

#endif /* _CAM_CSIPHY_2_4_0_HWREG_INFINITI_MAIN_H_ */
