// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/debugfs.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/io.h>
#include <soc/qcom/of_common.h>
#include "msm_cvp_internal.h"
#include "msm_cvp_debug.h"
#include "cvp_hfi_api.h"
#include "cvp_hfi.h"

#define UBWC_CONFIG(mco, mlo, hbo, bslo, bso, rs, mc, ml, hbb, bsl, bsp) \
{	\
	.override_bit_info.max_channel_override = mco,	\
	.override_bit_info.mal_length_override = mlo,	\
	.override_bit_info.hb_override = hbo,	\
	.override_bit_info.bank_swzl_level_override = bslo,	\
	.override_bit_info.bank_spreading_override = bso,	\
	.override_bit_info.reserved = rs,	\
	.max_channels = mc,	\
	.mal_length = ml,	\
	.highest_bank_bit = hbb,	\
	.bank_swzl_level = bsl,	\
	.bank_spreading = bsp,	\
}

struct msm_cvp_hfi_defs cvp_hfi_defs_v1[MAX_PKT_IDX];
struct msm_cvp_hfi_defs cvp_hfi_msg_defs_v1[MAX_PKT_IDX];
struct msm_cvp_hfi_defs cvp_hfi_defs_v2[MAX_PKT_IDX];
struct msm_cvp_hfi_defs cvp_hfi_msg_defs_v2[MAX_PKT_IDX];

static struct msm_cvp_common_data default_common_data[] = {
	{
		.key = "qcom,auto-pil",
		.value = 1,
	},
};

static struct msm_cvp_common_data sm8450_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
		.value = 1,
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 1,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2,             /*
					 * As per design driver allows 3rd
					 * instance as well since the secure
					 * flags were updated later for the
					 * current instance. Hence total
					 * secure sessions would be
					 * max-secure-instances + 1.
					 */
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,		/*
					 * Maxinum number of SSR before BUG_ON
					 */
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
		.value = 2000,
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	}
};

static struct msm_cvp_common_data sm8550_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
		.value = 1,
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2,             /*
					 * As per design driver allows 3rd
					 * instance as well since the secure
					 * flags were updated later for the
					 * current instance. Hence total
					 * secure sessions would be
					 * max-secure-instances + 1.
					 */
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,		/*
					 * Maxinum number of SSR before BUG_ON
					 */
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
		.value = 2000,
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	}
};

static struct msm_cvp_common_data sm8550_tvm_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
		.value = 0,
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2,             /*
					 * As per design driver allows 3rd
					 * instance as well since the secure
					 * flags were updated later for the
					 * current instance. Hence total
					 * secure sessions would be
					 * max-secure-instances + 1.
					 */
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,		/*
					 * Maxinum number of SSR before BUG_ON
					 */
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
		.value = 2000,
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 0,
	}
};

static struct msm_cvp_common_data sm8650_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
		.value = 0,
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2, /*
					* As per design driver allows 3rd
					* instance as well since the secure
					* flags were updated later for the
					* current instance. Hence total
					* secure sessions would be
					* max-secure-instances + 1.
					*/
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,	/*
					* Maxinum number of SSR before BUG_ON
					*/
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
		.value = 100000,
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 100000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	}
};

static struct msm_cvp_common_data sm8750_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
#ifdef USE_PRESIL
		.value = 0,
#else
		.value = 1,
#endif
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2, /*
					* As per design driver allows 3rd
					* instance as well since the secure
					* flags were updated later for the
					* current instance. Hence total
					* secure sessions would be
					* max-secure-instances + 1.
					*/
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,	/*
					* Maxinum number of SSR before BUG_ON
					*/
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
#ifdef USE_PRESIL
		.value = 15000000,
#else
		.value = 2000,
#endif
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	}
};

static struct msm_cvp_common_data sm8850_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
#ifdef USE_PRESIL
		.value = 0,
#else
		.value = 1,
#endif
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2, /*
					* As per design driver allows 3rd
					* instance as well since the secure
					* flags were updated later for the
					* current instance. Hence total
					* secure sessions would be
					* max-secure-instances + 1.
					*/
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,	/*
					* Maxinum number of SSR before BUG_ON
					*/
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
#ifdef USE_PRESIL
		.value = 15000000,
#else
		.value = 2000,
#endif
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	}
};

static struct msm_cvp_common_data sm8845_common_data[] = {
	{
		.key = "qcom,pm-qos-latency-us",
		.value = 50,
	},
	{
		.key = "qcom,sw-power-collapse",
#ifdef USE_PRESIL
		.value = 0,
#else
		.value = 1,
#endif
	},
	{
		.key = "qcom,domain-attr-non-fatal-faults",
		.value = 0,
	},
	{
		.key = "qcom,max-secure-instances",
		.value = 2,
	},
	{
		.key = "qcom,max-ssr-allowed",
		.value = 1,
	},
	{
		.key = "qcom,power-collapse-delay",
		.value = 3000,
	},
	{
		.key = "qcom,hw-resp-timeout",
#ifdef USE_PRESIL
		.value = 15000000,
#else
		.value = 2000,
#endif
	},
	{
		.key = "qcom,dsp-resp-timeout",
		.value = 1000,
	},
	{
		.key = "qcom,debug-timeout",
		.value = 0,
	},
	{
		.key = "qcom,dsp-enabled",
		.value = 1,
	},
	{
		.key = "qcom,qos_noc_urgency_low_a_bitmask",
		.value = 0x30,
	},
	{
		.key = "qcom,qos_noc_urgency_low_b_bitmask",
		.value = 0x3,
	}
};

/* Default UBWC config for LPDDR5 */
static struct msm_cvp_ubwc_config_data kona_ubwc_data[] = {
	UBWC_CONFIG(1, 1, 1, 0, 0, 0, 8, 32, 16, 0, 0),
};

static struct msm_cvp_qos_setting waipio_noc_qos = {
	.axi_qos = 0x99,
	.prioritylut_low = 0x22222222,
	.prioritylut_high = 0x33333333,
	.urgency_low = 0x1022,
	.dangerlut_low = 0x0,
	.safelut_low = 0xffff,
};

static struct msm_cvp_qos_setting pakala_noc_qos = {
	.axi_qos = 0x99,
	.prioritylut_low = 0x33333333,
	.prioritylut_high = 0x33333333,
	.urgency_low = 0x1033,
	.urgency_low_ro = 0x1003,
	.dangerlut_low = 0x0,
	.safelut_low = 0xffff,
};


static struct msm_cvp_platform_data default_data = {
	.common_data = default_common_data,
	.common_data_length =  ARRAY_SIZE(default_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = 0x0,
	.noc_qos = 0x0,
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v1,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v1,
	.hfi_ver = 1,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8450_data = {
	.common_data = sm8450_common_data,
	.common_data_length =  ARRAY_SIZE(sm8450_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,
	.noc_qos = &waipio_noc_qos,
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v1,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v1,
	.hfi_ver = 1,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8550_data = {
	.common_data = sm8550_common_data,
	.common_data_length =  ARRAY_SIZE(sm8550_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &waipio_noc_qos,	/*Reuse Waipio setting*/
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v1,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v1,
	.hfi_ver = 1,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8550_tvm_data = {
	.common_data = sm8550_tvm_common_data,
	.common_data_length =  ARRAY_SIZE(sm8550_tvm_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &waipio_noc_qos,	/*Reuse Waipio setting*/
	.vm_id = 2,
	.cvp_hfi = cvp_hfi_defs_v1,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v1,
	.hfi_ver = 1,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8650_data = {
	.common_data = sm8650_common_data,
	.common_data_length = ARRAY_SIZE(sm8650_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &waipio_noc_qos,	/*Reuse Waipio setting*/
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v1,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v1,
	.hfi_ver = 1,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8750_data = {
	.common_data = sm8750_common_data,
	.common_data_length = ARRAY_SIZE(sm8650_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &pakala_noc_qos,
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v2,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v2,
	.hfi_ver = 2,
	.hal_version = DEFAULT_HAL_VER,
};

static struct msm_cvp_platform_data sm8850_data = {
	.common_data = sm8850_common_data,
	.common_data_length = ARRAY_SIZE(sm8650_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &pakala_noc_qos,
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v2,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v2,
	.hfi_ver = 2,
	.hal_version = KNP_HAL_VER,
};

static struct msm_cvp_platform_data sm8845_data = {
	.common_data = sm8845_common_data,
	.common_data_length = ARRAY_SIZE(sm8845_common_data),
	.sku_version = 0,
	.vpu_ver = VPU_VERSION_5,
	.ubwc_config = kona_ubwc_data,	/*Reuse Kona setting*/
	.noc_qos = &pakala_noc_qos,
	.vm_id = 1,
	.cvp_hfi = cvp_hfi_defs_v2,
	.cvp_hfi_msg = cvp_hfi_msg_defs_v2,
	.hfi_ver = 2,
	.hal_version = KNP_HAL_VER,
};

static const struct of_device_id msm_cvp_dt_match[] = {
	{
		.compatible = "qcom,waipio-cvp",
		.data = &sm8450_data,
	},
	{
		.compatible = "qcom,kalama-cvp",
		.data = &sm8550_data,
	},
	{
		.compatible = "qcom,kalama-cvp-tvm",
		.data = &sm8550_tvm_data,
	},
	{
		.compatible = "qcom,pineapple-cvp",
		.data = &sm8650_data,
	},
	{
		.compatible = "qcom,sun-cvp",
		.data = &sm8750_data,
	},
	{
		.compatible = "qcom,canoe-cvp",
		.data = &sm8850_data,
	},
	{
		.compatible = "qcom,alor-cvp",
		.data = &sm8845_data,
	},
	{},
};

struct msm_cvp_hfi_defs *cvp_hfi_defs;
struct msm_cvp_hfi_defs *cvp_hfi_msg_defs;

/*
 * WARN: name field CAN NOT hold more than 63 chars
 *	 excluding the ending '\0'
 *
 * NOTE: the def entry index for the command packet is
 *	 "the packet type - HFI_CMD_SESSION_CVP_START"
 */
#ifdef CONFIG_SUN_HFI
struct msm_cvp_hfi_defs cvp_hfi_defs_v1[MAX_PKT_IDX] = {
	[HFI_CMD_SESSION_CVP_DFS_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DFS_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DFS_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DFS_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DFS_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DFS_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DFS_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DFS_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SGM_OF_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SGM_OF_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SGM_OF_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_SGM_OF_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SGM_OF_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SGM_OF_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_WARP_NCC_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_WARP_NCC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_WARP_NCC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_WARP_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_WARP_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_WARP_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_WARP_DS_PARAMS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_WARP_DS_PARAMS,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_WARP_DS_PARAMS",
		},
	[HFI_CMD_SESSION_EVA_WARP_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_WARP_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_WARP_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DMM_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DMM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DMM_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DMM_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DMM_PARAMS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_DMM_PARAMS,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DMM_PARAMS",
		},
	[HFI_CMD_SESSION_CVP_DMM_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DMM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DMM_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DMM_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_PERSIST_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xffffffff,
			.type = HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_DS_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DS_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DS_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DS_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DS - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DS_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DS",
		},
	[HFI_CMD_SESSION_CVP_CV_TME_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_OF_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_TME_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_TME_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_CV_TME_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_OF_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_TME_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_TME_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_CV_ODT_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_ODT_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_ODT_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_ODT_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_CV_ODT_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_ODT_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_ODT_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_ODT_FRAME",
		},
	[HFI_CMD_SESSION_CVP_CV_OD_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_OD_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_OD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_OD_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_CV_OD_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_OD_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_CV_OD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_CV_OD_FRAME",
		},
	[HFI_CMD_SESSION_CVP_NCC_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_NCC_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_NCC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_NCC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_NCC_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_NCC_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_NCC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_NCC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_ICA_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_ICA_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_ICA_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_ICA_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_ICA_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_ICA_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_ICA_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_ICA_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_HCD_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_HCD_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_HCD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_HCD_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_HCD_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_HCD_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_HCD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_HCD_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DC_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DCM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DC_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DCM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DCM_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DCM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DCM_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DCM_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DCM_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_DCM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DCM_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DCM_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_PYS_HCD_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_PYS_HCD_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = HFI_PYS_HCD_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_PYS_HCD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_PYS_HCD_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SET_MODEL_BUFFERS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SET_MODEL_BUFFERS,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_MODEL_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE",
		},
	[HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE",
		},
	[HFI_CMD_SESSION_CVP_FD_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_FD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_FD_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_FD_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_FD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_FD_FRAME",
		},
	[HFI_CMD_SESSION_CVP_XRA_BLOB_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_XRA_BLOB_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_XRA_BLOB_FRAME",
		},
	[HFI_CMD_SESSION_CVP_XRA_BLOB_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_XRA_BLOB_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_XRA_BLOB_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_XRA_MATCH_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_XRA_MATCH_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_XRA_MATCH_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_XRA_MATCH_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_XRA_MATCH_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_XRA_MATCH_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_RGE_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RGE_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RGE_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_RGE_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RGE_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RGE_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_ITOF_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_ITOF_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_ITOF_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_ITOF_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_ITOF_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_ITOF_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_SCALER_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SCALER_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SCALER_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_SCALER_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SCALER_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SCALER_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DLFD_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFD_FRAME",
		},
	[HFI_CMD_SESSION_EVA_DLFD_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFD_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DLFL_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFL_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFL_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_DLFL_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFL_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFL_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_SYNX - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SYNX,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SYNX",
		},
	[HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DME_ONLY_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DME_ONLY_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DME_ONLY_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_GME_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_GME_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_GME_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_GME_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_GME_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_GME_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_LME_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_LME_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_LME_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_LME_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_LME_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_LME_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_SPSTAT_CONFIG - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SPSTAT_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SPSTAT_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_SPSTAT_FRAME - HFI_CMD_SESSION_CVP_START] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SPSTAT_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SPSTAT_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_DFS_FRAME - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_DFS_FRAME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_DFS_FRAME",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_DFS_CONFIG - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_DFS_CONFIG,
		.is_config_pkt = true,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_DFS_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_LME_CONFIG - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_LME_CONFIG,
		.is_config_pkt = true,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_LME_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_LME_FRAME - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_LME_FRAME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_LME_FRAME",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_MATCH_CONFIG - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_MATCH_CONFIG,
		.is_config_pkt = true,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_MATCH_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_MATCH_FRAME - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_MATCH_FRAME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_MATCH_FRAME",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_CVP_FPX_CONFIG - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_CVP_FPX_CONFIG,
		.is_config_pkt = true,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_CVP_FPX_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_CVP_FPX_FRAME - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_CVP_FPX_FRAME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_CVP_FPX_FRAME",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_GME_CONFIG - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_GME_CONFIG,
		.is_config_pkt = true,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_GME_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_CMD_SESSION_EVA_GME_FRAME - HFI_CMD_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_CMD_SESSION_EVA_GME_FRAME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_CMD_SESSION_EVA_GME_FRAME",
		.force_kernel_fence = false,
	}
};

/*
 * Below are for msg packet
 *  "the packet type - HFI_MSG_SESSION_CVP_START"
 */

struct msm_cvp_hfi_defs cvp_hfi_msg_defs_v1[MAX_PKT_IDX] = {
	[HFI_MSG_SESSION_CVP_FPX - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FPX,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FPX",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_BUFFERS - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_BUFFERS ",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DS - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_CV_HOG - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_CV_HOG,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_CV_HOG",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DFS - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DFS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DFS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SVM - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SVM,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SVM",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_NCC - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_NCC,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_NCC",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_TME - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_TME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_TME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_SPSTAT - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_SPSTAT,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_SPSTAT",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_ICA - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_ICA,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_ICA",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DME - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DME_ONLY - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DME_ONLY,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DME_ONLY",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_OPERATION_CONFIG  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_OPERATION_CONFIG,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_OPERATION_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_MODEL_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_MODEL_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_MODEL_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_MODEL_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_MODEL_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_MODEL_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SGM_OF  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SGM_OF,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SGM_OF",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_GCE  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_GCE,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_OPERATION_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_WARP_NCC  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_WARP_NCC,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_WARP_NCC",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DMM  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DMM,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DMM",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SGM_DFS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SGM_DFS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SGM_DFS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_WARP  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_WARP,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_WARP",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DMM_PARAMS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DMM_PARAMS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DMM_PARAMS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_WARP_DS_PARAMS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_WARP_DS_PARAMS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_WARP_DS_PARAMS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_WARP  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_WARP,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_WARP",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_LME   - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_LME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_LME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_FLUSH  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FLUSH,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FLUSH",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_START  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_START,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_START",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_STOP  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_STOP,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_STOP",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_PYS_HCD  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_PYS_HCD,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_PYS_HCD",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DESCRIPTOR  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DESCRIPTOR,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DESCRIPTOR",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_MATCH  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_MATCH,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_MATCH",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_GME  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_GME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_GME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_SCALER  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_SCALER,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_SCALER",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_FPX  - HFI_MSG_SESSION_CVP_START] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FPX,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FPX",
		.force_kernel_fence = false,
	}
};
#else
struct msm_cvp_hfi_defs cvp_hfi_defs_v2[MAX_PKT_IDX] = {
	[HFI_CMD_SESSION_EVA_DFS_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_DFS_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_EVA_DFS_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DFS_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DFS_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_DFS_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_EVA_DFS_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DFS_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SGM_OF_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SGM_OF_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SGM_OF_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_SGM_OF_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SGM_OF_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SGM_OF_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_WARP_NCC_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_WARP_NCC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_WARP_NCC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_WARP_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_WARP_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_WARP_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_WARP_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_WARP_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_WARP_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DMM_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_DMM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DMM_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DMM_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DMM_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_DMM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DMM_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DMM_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS - HFI_CMD_SESSION_EVA_CTRL_OFFSET
								+ CTRL_OFFSET] = {
			.size = HFI_PERSIST_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS - HFI_CMD_SESSION_EVA_CTRL_OFFSET
								+ CTRL_OFFSET] = {
			.size = 0xffffffff,
			.type = HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_NCC_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_NCC_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_NCC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_NCC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_NCC_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_NCC_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_NCC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_NCC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DC_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_DCM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DC_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DC_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DC_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_DCM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DC_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DC_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_DCM_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_DCM_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DCM_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DCM_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_DCM_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_DCM_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_DCM_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_DCM_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = HFI_PYS_HCD_CONFIG_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_PYS_HCD_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = HFI_PYS_HCD_FRAME_CMD_SIZE,
			.type = HFI_CMD_SESSION_CVP_PYS_HCD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_PYS_HCD_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS - HFI_CMD_SESSION_EVA_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS - HFI_CMD_SESSION_EVA_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS",
		},
	[HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE - HFI_CMD_SESSION_EVA_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE",
		},
	[HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE - HFI_CMD_SESSION_EVA_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE",
		},
	[HFI_CMD_SESSION_EVA_BLOB_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_BLOB_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "EVA_BLOB_FRAME",
		},
	[HFI_CMD_SESSION_EVA_BLOB_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_BLOB_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "EVA_BLOB_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_MATCH_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_MATCH_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "EVA_MATCH_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_MATCH_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_MATCH_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "EVA_MATCH_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_RGE_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RGE_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RGE_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_RGE_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_RGE_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_RGE_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_ITOF_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_ITOF_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_ITOF_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_ITOF_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_ITOF_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_ITOF_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_SCALER_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SCALER_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SCALER_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_SCALER_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SCALER_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SCALER_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET
								+ CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DLFD_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFD_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFD_FRAME",
		},
	[HFI_CMD_SESSION_EVA_DLFD_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFD_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFD_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DLFL_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFL_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFL_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_DLFL_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DLFL_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DLFL_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_SYNX - HFI_CMD_SESSION_EVA_CTRL_OFFSET + CTRL_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_SYNX,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_SYNX",
		},
	[HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_DME_ONLY_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_DME_ONLY_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_DME_ONLY_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_GME_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_GME_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_GME_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_GME_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_GME_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_GME_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_LME_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_LME_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_LME_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_LME_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_LME_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_CVP_LME_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_EVA_SPSTAT_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SPSTAT_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SPSTAT_CONFIG",
		},
	[HFI_CMD_SESSION_EVA_SPSTAT_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_EVA_SPSTAT_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "HFI_CMD_SESSION_EVA_SPSTAT_FRAME",
			.force_kernel_fence = false,
		},
	[HFI_CMD_SESSION_CVP_FPX_CONFIG - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_FPX_CONFIG,
			.is_config_pkt = true,
			.resp = HAL_NO_RESP,
			.name = "FPX_CONFIG",
		},
	[HFI_CMD_SESSION_CVP_FPX_FRAME - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET] = {
			.size = 0xFFFFFFFF,
			.type = HFI_CMD_SESSION_CVP_FPX_FRAME,
			.is_config_pkt = false,
			.resp = HAL_NO_RESP,
			.name = "FPX_FRAME",
			.force_kernel_fence = false,
		},

};

/*
 * Below are for msg packet
 */
struct msm_cvp_hfi_defs cvp_hfi_msg_defs_v2[MAX_PKT_IDX] = {
	[HFI_MSG_SESSION_CVP_FPX - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FPX,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FPX",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_BUFFERS - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_BUFFERS  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_BUFFERS ",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DFS - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DFS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DFS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_NCC - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_NCC,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_NCC",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_SPSTAT - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_SPSTAT,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_SPSTAT",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DME - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DME_ONLY - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DME_ONLY,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DME_ONLY",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_OPERATION_CONFIG  - HFI_MSG_SESSION_EVA_OFFSET] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_OPERATION_CONFIG,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_OPERATION_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
							+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
							+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SGM_OF  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SGM_OF,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SGM_OF",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_GCE  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_GCE,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_OPERATION_CONFIG",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_WARP_NCC  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_WARP_NCC,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_WARP_NCC",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_DMM  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_DMM,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_DMM",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SGM_DFS  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SGM_DFS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SGM_DFS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS  - HFI_MSG_SESSION_EVA_OFFSET] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS  - HFI_MSG_SESSION_EVA_OFFSET] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS",
		.force_kernel_fence = false,
	},
	[HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY  - HFI_MSG_SESSION_EVA_OFFSET] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_WARP  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_WARP,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_WARP",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_LME   - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_LME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_LME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_FLUSH  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FLUSH,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FLUSH",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_START_DONE  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_START_DONE,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_START_DONE",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_STOP_DONE  - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_STOP_DONE,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_STOP_DONE",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_PYS_HCD  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_PYS_HCD,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_PYS_HCD",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_DESCRIPTOR  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_DESCRIPTOR,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_DESCRIPTOR",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_MATCH  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_MATCH,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_MATCH",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_GME  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_GME,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_GME",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_EVA_SCALER  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_EVA_SCALER,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_EVA_SCALER",
		.force_kernel_fence = false,
	},
	[HFI_MSG_SESSION_CVP_FPX  - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX] = {
		.size = 0xFFFFFFFF,
		.type = HFI_MSG_SESSION_CVP_FPX,
		.is_config_pkt = false,
		.resp = HAL_NO_RESP,
		.name = "HFI_MSG_SESSION_CVP_FPX",
		.force_kernel_fence = false,
	}
};
#endif

int get_pkt_index(struct cvp_hal_session_cmd_pkt *hdr)
{
	struct msm_cvp_platform_data *pdata = cvp_driver->cvp_core->platform_data;
	uint32_t hfi_ver = pdata->hfi_ver;

	if (hfi_ver == 1) {
		if (!hdr || (hdr->packet_type < HFI_CMD_SESSION_CVP_START)
			|| hdr->packet_type >= (HFI_CMD_SESSION_CVP_START + MAX_PKT_IDX))
			return -EINVAL;

		if (cvp_hfi_defs[hdr->packet_type - HFI_CMD_SESSION_CVP_START].size)
			return (hdr->packet_type - HFI_CMD_SESSION_CVP_START);

		return -EINVAL;
	} else {
		int pkt_idx;
		u32 thirteenth_bit;
		u32 fourteenth_bit;

		if (!hdr)
			return -EINVAL;

		thirteenth_bit = (hdr->packet_type >> 12) & 1;
		fourteenth_bit = (hdr->packet_type >> 13) & 1;

		if (thirteenth_bit && fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET;
		else if (!thirteenth_bit && fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET;
		else if (thirteenth_bit && !fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_EVA_CTRL_OFFSET + CTRL_OFFSET;
		else
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_EVA_OFFSET;

		if ((pkt_idx < 0) || pkt_idx >= (MAX_PKT_IDX))
			return -EINVAL;

		if (cvp_hfi_defs[pkt_idx].size)
			return pkt_idx;

		return -EINVAL;
	}
}

int get_pkt_fenceoverride(struct cvp_hal_session_cmd_pkt *hdr)
{
	struct msm_cvp_platform_data *pdata = cvp_driver->cvp_core->platform_data;
	uint32_t hfi_ver = pdata->hfi_ver;

	if (hfi_ver == 1)
		return cvp_hfi_defs[hdr->packet_type -
			HFI_CMD_SESSION_CVP_START].force_kernel_fence;
	else {
		int pkt_idx;
		u32 thirteenth_bit;
		u32 fourteenth_bit;

		thirteenth_bit = (hdr->packet_type >> 12) & 1;
		fourteenth_bit = (hdr->packet_type >> 13) & 1;

		if (thirteenth_bit && fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET;
		else if (!thirteenth_bit && fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET;
		else if (thirteenth_bit && !fourteenth_bit)
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_EVA_CTRL_OFFSET + CTRL_OFFSET;
		else
			pkt_idx = hdr->packet_type - HFI_CMD_SESSION_EVA_OFFSET;


		return cvp_hfi_defs[pkt_idx].force_kernel_fence;
	}
}

int get_pkt_index_from_type(u32 pkt_type)
{
	struct msm_cvp_platform_data *pdata = cvp_driver->cvp_core->platform_data;
	uint32_t hfi_ver = pdata->hfi_ver;

	if (hfi_ver == 1) {
		if ((pkt_type < HFI_CMD_SESSION_CVP_START) ||
			pkt_type >= (HFI_CMD_SESSION_CVP_START + MAX_PKT_IDX))
			return -EINVAL;

		if (cvp_hfi_defs[pkt_type - HFI_CMD_SESSION_CVP_START].size)
			return (pkt_type - HFI_CMD_SESSION_CVP_START);
	} else {
		int pkt_idx;
		u32 thirteenth_bit;
		u32 fourteenth_bit;

		thirteenth_bit = (pkt_type >> 12) & 1;
		fourteenth_bit = (pkt_type >> 13) & 1;

		if (thirteenth_bit && fourteenth_bit)
			pkt_idx = pkt_type - HFI_CMD_SESSION_FRAME_OFFSET + FRAME_OFFSET;
		else if (!thirteenth_bit && fourteenth_bit)
			pkt_idx = pkt_type - HFI_CMD_SESSION_CONFIG_OFFSET + CONFIG_OFFSET;
		else if (thirteenth_bit && !fourteenth_bit)
			pkt_idx = pkt_type - HFI_CMD_SESSION_EVA_CTRL_OFFSET + CTRL_OFFSET;
		else
			pkt_idx = pkt_type - HFI_CMD_SESSION_EVA_OFFSET;

		if ((pkt_idx < 0) || pkt_idx >= (MAX_PKT_IDX))
			return -EINVAL;

		if (cvp_hfi_defs[pkt_idx].size)
			return pkt_idx;
	}

	return -EINVAL;
}

const char *get_pkt_name_from_type(u32 pkt_type)
{
	struct msm_cvp_platform_data *pdata = cvp_driver->cvp_core->platform_data;
	uint32_t hfi_ver = pdata->hfi_ver;

	if (hfi_ver == 1) {
		if (pkt_type > HFI_CMD_SESSION_CVP_START &&
			pkt_type <= (HFI_CMD_SESSION_CVP_START + MAX_PKT_IDX))
			return cvp_hfi_defs[pkt_type - HFI_CMD_SESSION_CVP_START].name;
		else if (pkt_type > HFI_MSG_SESSION_CVP_START &&
			pkt_type <= (HFI_MSG_SESSION_CVP_START + MAX_PKT_IDX))
			return cvp_hfi_msg_defs[pkt_type - HFI_MSG_SESSION_CVP_START].name;
		else
			return "";
	} else {
		u32 mask;
		int pkt_idx;

		if ((pkt_type & 0x03000000) == HFI_CMD_SESSION_EVA_OFFSET) {
			int pkt_idx = get_pkt_index_from_type(pkt_type);

			if ((pkt_idx < 0) || pkt_idx >= (MAX_PKT_IDX))
				return "";
			else
				return cvp_hfi_defs[pkt_idx].name;
		} else if (((pkt_type & 0x03000000) == HFI_MSG_SESSION_EVA_OFFSET)) {
			mask = pkt_type & 0x3000;
			pkt_idx = -EINVAL;

			if (mask == 0x3000)
				pkt_idx = pkt_type - HFI_MSG_SESSION_OFFSET + MSG_SESSION_INDEX;
			else if (mask == 0x1000)
				pkt_idx = pkt_type - HFI_MSG_SESSION_EVA_CTRL_OFFSET
					+ MSG_SESSION_EVA_CTRL_INDEX;
			else if (mask == 0)
				pkt_idx = pkt_type - HFI_MSG_SESSION_EVA_OFFSET;

			if ((pkt_idx < 0) || pkt_idx >= (MAX_PKT_IDX))
				return "";
			else
				return cvp_hfi_msg_defs[pkt_idx].name;
		}
		return "";
	}
}

const char *get_feature_name_from_type(u32 pkt_type)
{
	switch (pkt_type) {
	case HFI_CV_KERNEL_FPX:
		return "FPX";
	case HFI_CV_KERNEL_WARP:
		return "WARP";
	case HFI_CV_KERNEL_DESCRIPTOR:
		return "DESCRIPTOR";
	case HFI_CV_KERNEL_NCC:
		return "NCC";
	case HFI_CV_KERNEL_DFS:
		return "DFS";
	case HFI_CV_KERNEL_WARP_NCC:
		return "WARP NCC";
	case HFI_CV_KERNEL_ORB:
		return "ORB";
	case HFI_CV_KERNEL_PYS_HCD:
		return "Pyramid HCD";
	case HFI_CV_KERNEL_ICA:
		return "ICA";
	case HFI_CV_KERNEL_GCX:
		return "GSX";
	case HFI_CV_KERNEL_XRA:
		return "XRA";
	case HFI_CV_KERNEL_CSC:
		return "CSC";
	case HFI_CV_KERNEL_LSR:
		return "LSR";
	case HFI_CV_KERNEL_ITOF:
		return "ITOF";
	case HFI_CV_KERNEL_RGE:
		return "RGE";
	case HFI_CV_KERNEL_LME:
		return "LME";
	case HFI_CV_KERNEL_SPSTAT:
		return "Spatial Stats";
	case HFI_CV_KERNEL_GME:
		return "GME";
	case HFI_CV_KERNEL_SCALER:
		return "SCALER";
	case HFI_CV_KERNEL_MATCH:
		return "MATCH";
	case HFI_CV_KERNEL_BLOB:
		return "BLOB";
	default:
		return " ";
	}
}

MODULE_DEVICE_TABLE(of, msm_cvp_dt_match);

int cvp_of_fdt_get_ddrtype(void)
{
#ifdef FIXED_DDR_TYPE
	/* of_fdt_get_ddrtype() is usually unavailable during pre-sil */
	return DDR_TYPE_LPDDR5;
#else
	return of_fdt_get_ddrtype();
#endif
}

void *cvp_get_drv_data(struct device *dev)
{
	struct msm_cvp_platform_data *driver_data;
	const struct of_device_id *match;
	uint32_t ddr_type = DDR_TYPE_LPDDR5;

	driver_data = &default_data;

	if (!IS_ENABLED(CONFIG_OF) || !dev->of_node)
		goto exit;

	match = of_match_node(msm_cvp_dt_match, dev->of_node);

	if (!match)
		return NULL;

	driver_data = (struct msm_cvp_platform_data *)match->data;

	if (!strcmp(match->compatible, "qcom,waipio-cvp")) {
		ddr_type = cvp_of_fdt_get_ddrtype();
		if (ddr_type == -ENOENT) {
			dprintk(CVP_ERR,
				"Failed to get ddr type, use LPDDR5\n");
		}

		if (driver_data->ubwc_config &&
			(ddr_type == DDR_TYPE_LPDDR4 ||
			ddr_type == DDR_TYPE_LPDDR4X))
			driver_data->ubwc_config->highest_bank_bit = 15;
		dprintk(CVP_CORE, "DDR Type 0x%x hbb 0x%x\n",
			ddr_type, driver_data->ubwc_config ?
			driver_data->ubwc_config->highest_bank_bit : -1);
	}
exit:
	return driver_data;
}
