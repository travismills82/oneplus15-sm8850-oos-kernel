/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_DEFS_LAYER_COLOR_H__
#define __H_HFI_DEFS_LAYER_COLOR_H__

#include "hfi_defs_common.h"

#define HFI_COLOR_LAYER_IGC_NONE            0
#define HFI_COLOR_LAYER_IGC_SRGB            1
#define HFI_COLOR_LAYER_IGC_709             2
#define HFI_COLOR_LAYER_IGC_2_2             3
#define HFI_COLOR_LAYER_IGC_HLG             4
#define HFI_COLOR_LAYER_IGC_PQ              5

#define HFI_COLOR_LAYER_GC_NONE             0
#define HFI_COLOR_LAYER_GC_SRGB             1
#define HFI_COLOR_LAYER_GC_2_2              2
#define HFI_COLOR_LAYER_GC_HLG              3
#define HFI_COLOR_LAYER_GC_PQ               4

#define HFI_CSC_MATRIX_COEFF_SIZE           9
#define HFI_CSC_CLAMP_SIZE                  6
#define HFI_CSC_BIAS_SIZE                   3

#define HFI_COLOR_LAYER_FEATURE_ENABLE_FLAG (1 << 0)

/**
 * struct hfi_csc - csc feature structure
 * @flags:              Feature customization flags
 * @ctm_coeff:          Matrix coefficients
 * @pre_bias:           Pre-bias array values
 * @post_bias:          Post-bias array values
 * @pre_clamp:          Pre-clamp array values
 * @post_clamp:         Post-clamp array values
 */
struct hfi_csc {
	u32 flags;
	u32 ctm_coeff[HFI_CSC_MATRIX_COEFF_SIZE];
	u32 pre_bias[HFI_CSC_BIAS_SIZE];
	u32 post_bias[HFI_CSC_BIAS_SIZE];
	u32 pre_clamp[HFI_CSC_CLAMP_SIZE];
	u32 post_clamp[HFI_CSC_CLAMP_SIZE];
};

#define HFI_UCSC_CSC_MATRIX_COEFF_SIZE     12
#define HFI_UCSC_CSC_PRE_CLAMP_SIZE        6
#define HFI_UCSC_CSC_POST_CLAMP_SIZE       2
/**
 * struct hfi_ucsc_csc: ucsc csc feature structure
 * @flags:              Feature customization flags
 * @ctm_coeff:          Matrix coefficients, 3x4, u16
 * @pre_clamp:          Pre-clamp array values
 *                      Per component u16 RL, RH, GL, GH, BL, BH
 * @post_clamp:         Post-clamp array values
 *                      Common for all components H, L
 */
struct hfi_ucsc_csc {
	u32 flags;
	u32 ctm_coeff[HFI_UCSC_CSC_MATRIX_COEFF_SIZE];
	u32 pre_clamp[HFI_UCSC_CSC_PRE_CLAMP_SIZE];
	u32 post_clamp[HFI_UCSC_CSC_POST_CLAMP_SIZE];
};

#endif // __H_HFI_DEFS_LAYER_COLOR_H__
