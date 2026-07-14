/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 */

#ifndef _DP_PANEL_TU_H_
#define _DP_PANEL_TU_H_

#include <linux/kernel.h>
#include <linux/types.h>

struct dp_tu_calc_input {
	u64 lclk;              /* 162, 270, 540 and 810 MHz*/
	u64 pclk_khz;          /* in KHz */
	u64 hactive;           /* active h-width */
	u64 hporch;            /* bp + fp + pulse */
	int nlanes;            /* no.of.lanes */
	int bpp;               /* bits */
	int pixel_enc;         /* 444, 420, 422 */
	int dsc_en;            /* dsc on/off */
	int async_en;          /* async mode */
	int fec_en;            /* fec */
	int compress_ratio;    /* 2:1 = 200, 3:1 = 300, 3.75:1 = 375 */
	int num_of_dsc_slices; /* number of slices per line */
	s64 comp_bpp;          /* compressed bpp = uncomp_bpp / compression_ratio */
	int ppc_div_factor;    /* pass in ppc mode 2/4 */
};

struct dp_tu_calc_output {
	u32 valid_boundary_link;
	u32 delay_start_link;
	bool boundary_moderation_en;
	u32 valid_lower_boundary_link;
	u32 upper_boundary_count;
	u32 lower_boundary_count;
	u32 tu_size_minus1;
};

struct dp_tu_compression_info {
	u32 src_bpp;
	u32 tgt_bpp;
	u16 pic_width;
	int pclk_per_line;
	int ppc_div_factor;
};

struct dp_tu_dhdr_info {
	u32 mdp_clk;
	u32 lclk;
	u32 pclk;
	u32 h_active;
	u32 nlanes;
	s64 mst_target_sc;
	bool mst_en;
	bool fec_en;
};

struct dp_tu_mst_rg_in {
	u64 lclk_khz;
	u64 pclk_khz;
	u8 nlanes;
	u8 src_bpp;
	u8 tgt_bpp;
	bool fec_en;
	bool dsc_en;
	s64 fec_overhead_fp;
	s64 dsc_overhead_fp;
	u64 pbn;
	/*
	 * margin_ovrd: SW control for handling margin
	 * Only enabled when running 4k60fps displays on 2x MST stream
	 */
	bool margin_ovrd;
};

struct dp_tu_mst_rg_out {
	u64 min_sc_fp;
	u64 max_sc_fp;
	u64 target_sc_fp;
	u64 ts_int;
	u32 x_int;
	u32 y_frac_enum;
};

struct dp_dsc_dto_params {
	u32 tgt_bpp;
	u32 src_bpp;
	u32 num;
	u32 denom;
};

void dp_tu_calculate(struct dp_tu_calc_input *in, struct dp_tu_calc_output *tu_table);
u32 dp_tu_dsc_get_num_extra_pclk(u32 pclk_factor, struct dp_tu_compression_info *input);
u32 dp_tu_dhdr_pkt_limit(struct dp_tu_dhdr_info *input);
void dp_tu_mst_rg_calc(struct dp_tu_mst_rg_in *in, struct dp_tu_mst_rg_out *out);

#endif
