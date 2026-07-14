// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 */

#include "dp_panel_tu.h"
#include <drm/drm_fixed.h>
#include "dp_debug.h"

#define DP_TU_FP_EDGE 0x40000

struct tu_algo_data {
	s64 lclk_fp;
	s64 orig_lclk_fp;

	s64 pclk_fp;
	s64 orig_pclk_fp;
	s64 lwidth;
	s64 lwidth_fp;
	int orig_lwidth;
	s64 hbp_relative_to_pclk;
	s64 hbp_relative_to_pclk_fp;
	int orig_hbp;
	int nlanes;
	int bpp;
	int pixelEnc;
	int dsc_en;
	int async_en;
	int fec_en;
	int bpc;

	int rb2;
	uint delay_start_link_extra_pixclk;
	int extra_buffer_margin;
	s64 ratio_fp;
	s64 original_ratio_fp;

	s64 err_fp;
	s64 n_err_fp;
	s64 n_n_err_fp;
	int tu_size;
	int tu_size_desired;
	int tu_size_minus1;

	int valid_boundary_link;
	s64 resulting_valid_fp;
	s64 total_valid_fp;
	s64 effective_valid_fp;
	s64 effective_valid_recorded_fp;
	int n_tus;
	int n_tus_per_lane;
	int paired_tus;
	int remainder_tus;
	int remainder_tus_upper;
	int remainder_tus_lower;
	int extra_bytes;
	int filler_size;
	int delay_start_link;

	int extra_pclk_cycles;
	int extra_pclk_cycles_in_link_clk;
	s64 ratio_by_tu_fp;
	s64 average_valid2_fp;
	int new_valid_boundary_link;
	int remainder_symbols_exist;
	int n_symbols;
	s64 n_remainder_symbols_per_lane_fp;
	s64 last_partial_tu_fp;
	s64 TU_ratio_err_fp;

	int n_tus_incl_last_incomplete_tu;
	int extra_pclk_cycles_tmp;
	int extra_pclk_cycles_in_link_clk_tmp;
	int extra_required_bytes_new_tmp;
	int filler_size_tmp;
	int lower_filler_size_tmp;
	int delay_start_link_tmp;

	bool boundary_moderation_en;
	int boundary_mod_lower_err;
	int upper_boundary_count;
	int lower_boundary_count;
	int i_upper_boundary_count;
	int i_lower_boundary_count;
	int valid_lower_boundary_link;
	int even_distribution_BF;
	int even_distribution_legacy;
	int even_distribution;
	int hbp_delayStartCheck;
	int pre_tu_hw_pipe_delay;
	int post_tu_hw_pipe_delay;
	int link_config_hactive_time;
	int delay_start_link_lclk;
	int tu_active_cycles;
	s64 parity_symbols;
	int resolution_line_time;
	int last_partial_lclk;
	int min_hblank_violated;
	s64 delay_start_time_fp;
	s64 hbp_time_fp;
	s64 hactive_time_fp;
	s64 diff_abs_fp;
	int second_loop_set;
	s64 ratio;
	s64 comp_bpp;
	int ppc_div_factor;
};

static int _tu_param_compare(s64 a, s64 b)
{
	u32 a_int, a_frac, a_sign;
	u32 b_int, b_frac, b_sign;
	s64 a_temp, b_temp, minus_1;

	if (a == b)
		return 0;

	minus_1 = drm_fixp_from_fraction(-1, 1);

	a_int = (a >> 32) & 0x7FFFFFFF;
	a_frac = a & 0xFFFFFFFF;
	a_sign = (a >> 32) & 0x80000000 ? 1 : 0;

	b_int = (b >> 32) & 0x7FFFFFFF;
	b_frac = b & 0xFFFFFFFF;
	b_sign = (b >> 32) & 0x80000000 ? 1 : 0;

	if (a_sign > b_sign)
		return 2;
	else if (b_sign > a_sign)
		return 1;

	if (!a_sign && !b_sign) { /* positive */
		if (a > b)
			return 1;
		else
			return 2;
	} else { /* negative */
		a_temp = drm_fixp_mul(a, minus_1);
		b_temp = drm_fixp_mul(b, minus_1);

		if (a_temp > b_temp)
			return 2;
		else
			return 1;
	}
}

static s64 fixp_subtract(s64 a, s64 b)
{
	s64 minus_1 = drm_fixp_from_fraction(-1, 1);

	if (a >= b)
		return a - b;

	return drm_fixp_mul(b - a, minus_1);
}

static inline int fixp2int_ceil(s64 a)
{
	return a ? drm_fixp2int_ceil(a) : 0;
}

static int fixp2int_round(s64 a)
{
	s64 half = drm_fixp_from_fraction(1, 2);

	return (a >= 0) ? drm_fixp2int(a + half) : drm_fixp2int(a - half);
}

static void _tu_valid_boundary_calc(struct tu_algo_data *tu)
{
	s64 temp1_fp, temp2_fp;
	int temp, temp1, temp2;
	int compare_result_1, compare_result_2, compare_result_3;

	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = drm_fixp_mul(tu->ratio_fp, temp1_fp);

	tu->new_valid_boundary_link = fixp2int_ceil(temp2_fp);

	temp = (tu->i_upper_boundary_count * tu->new_valid_boundary_link
			+ tu->i_lower_boundary_count * (tu->new_valid_boundary_link - 1));
	temp1 = tu->i_upper_boundary_count + tu->i_lower_boundary_count;
	tu->average_valid2_fp = drm_fixp_from_fraction(temp, temp1);

	temp1_fp = drm_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tu->lwidth_fp;
	temp1_fp = drm_fixp_mul(temp2_fp, temp1_fp);
	temp2_fp = drm_fixp_div(temp1_fp, tu->average_valid2_fp);
	tu->n_tus = drm_fixp2int(temp2_fp);

	if ((unsigned int) temp2_fp > 0xFFFFF000)
		tu->n_tus += 1;

	temp1_fp = drm_fixp_from_fraction(tu->n_tus, 1);
	temp2_fp = drm_fixp_mul(temp1_fp, tu->average_valid2_fp);
	temp1_fp = drm_fixp_from_fraction(tu->n_symbols, 1);
	temp2_fp = temp1_fp - temp2_fp;
	temp1_fp = drm_fixp_from_fraction(tu->nlanes, 1);
	temp2_fp = drm_fixp_div(temp2_fp, temp1_fp);
	tu->n_remainder_symbols_per_lane_fp = temp2_fp;

	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	tu->last_partial_tu_fp = drm_fixp_div(tu->n_remainder_symbols_per_lane_fp, temp1_fp);

	if (tu->n_remainder_symbols_per_lane_fp != 0)
		tu->remainder_symbols_exist = 1;
	else
		tu->remainder_symbols_exist = 0;

	temp1_fp = drm_fixp_from_fraction(tu->n_tus, tu->nlanes);
	tu->n_tus_per_lane = drm_fixp2int(temp1_fp);

	tu->paired_tus = (int)((tu->n_tus_per_lane) / temp1);

	tu->remainder_tus = tu->n_tus_per_lane - tu->paired_tus * temp1;

	if ((tu->remainder_tus - tu->i_upper_boundary_count) > 0) {
		tu->remainder_tus_upper = tu->i_upper_boundary_count;
		tu->remainder_tus_lower = tu->remainder_tus - tu->i_upper_boundary_count;
	} else {
		tu->remainder_tus_upper = tu->remainder_tus;
		tu->remainder_tus_lower = 0;
	}

	temp = tu->paired_tus * (tu->i_upper_boundary_count *
				tu->new_valid_boundary_link +
				tu->i_lower_boundary_count *
				(tu->new_valid_boundary_link - 1)) +
				(tu->remainder_tus_upper *
				 tu->new_valid_boundary_link) +
				(tu->remainder_tus_lower *
				(tu->new_valid_boundary_link - 1));
	tu->total_valid_fp = drm_fixp_from_fraction(temp, 1);

	if (tu->remainder_symbols_exist) {
		temp1_fp = tu->total_valid_fp + tu->n_remainder_symbols_per_lane_fp;
		temp2_fp = drm_fixp_from_fraction(tu->n_tus_per_lane, 1);
		temp2_fp = temp2_fp + tu->last_partial_tu_fp;
		temp1_fp = drm_fixp_div(temp1_fp, temp2_fp);
	} else {
		temp2_fp = drm_fixp_from_fraction(tu->n_tus_per_lane, 1);
		temp1_fp = drm_fixp_div(tu->total_valid_fp, temp2_fp);
	}
	tu->effective_valid_fp = temp1_fp;

	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = drm_fixp_mul(tu->ratio_fp, temp1_fp);
	tu->n_n_err_fp = fixp_subtract(tu->effective_valid_fp, temp2_fp);

	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = drm_fixp_mul(tu->ratio_fp, temp1_fp);
	tu->n_err_fp = fixp_subtract(tu->average_valid2_fp, temp2_fp);

	tu->even_distribution = tu->n_tus % tu->nlanes == 0 ? 1 : 0;

	temp1_fp = drm_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tu->lwidth_fp;
	temp1_fp = drm_fixp_mul(temp2_fp, temp1_fp);
	temp2_fp = drm_fixp_div(temp1_fp, tu->average_valid2_fp);
	tu->n_tus_incl_last_incomplete_tu = fixp2int_ceil(temp2_fp);

	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = drm_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = fixp_subtract(tu->average_valid2_fp, temp2_fp);
	temp2_fp = drm_fixp_from_fraction(tu->n_tus_incl_last_incomplete_tu, 1);
	temp1_fp = drm_fixp_mul(temp2_fp, temp1_fp);
	temp1 = fixp2int_ceil(temp1_fp);

	if ((unsigned int) temp1_fp < DP_TU_FP_EDGE)
		temp1 = drm_fixp2int(temp1_fp);

	temp = tu->i_upper_boundary_count * tu->nlanes;
	temp1_fp = drm_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = drm_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = drm_fixp_from_fraction(tu->new_valid_boundary_link, 1);
	temp2_fp = fixp_subtract(temp1_fp, temp2_fp);
	temp1_fp = drm_fixp_from_fraction(temp, 1);
	temp2_fp = drm_fixp_mul(temp1_fp, temp2_fp);
	temp2 = fixp2int_ceil(temp2_fp);

	if ((unsigned int) temp2_fp < DP_TU_FP_EDGE)
		temp2 = drm_fixp2int(temp2_fp);

	tu->extra_required_bytes_new_tmp = temp1 + temp2;

	temp = tu->extra_required_bytes_new_tmp * 8;
	temp1_fp = drm_fixp_div(temp, tu->bpp);
	tu->extra_pclk_cycles_tmp = fixp2int_ceil(temp1_fp);

	temp1_fp = drm_fixp_from_fraction(tu->extra_pclk_cycles_tmp, 1);
	temp2_fp = drm_fixp_mul(temp1_fp, tu->lclk_fp);

	temp1_fp = drm_fixp_div(temp2_fp, tu->pclk_fp);
	temp = fixp2int_ceil(temp1_fp);

	if ((unsigned int) temp1_fp < DP_TU_FP_EDGE)
		temp = drm_fixp2int(temp1_fp);

	tu->extra_pclk_cycles_in_link_clk_tmp = temp;
	tu->filler_size_tmp = tu->tu_size - tu->new_valid_boundary_link;

	tu->lower_filler_size_tmp = tu->filler_size_tmp + 1;

	tu->delay_start_link_tmp = tu->extra_pclk_cycles_in_link_clk_tmp +
					tu->lower_filler_size_tmp + tu->extra_buffer_margin;

	temp1_fp = drm_fixp_from_fraction(tu->delay_start_link_tmp, 1);
	tu->delay_start_time_fp = drm_fixp_div(temp1_fp, tu->lclk_fp);

	if (tu->rb2) {
		temp1_fp = drm_fixp_mul(tu->delay_start_time_fp, tu->lclk_fp);
		tu->delay_start_link_lclk = fixp2int_ceil(temp1_fp);

		if (tu->remainder_tus > tu->i_upper_boundary_count) {
			temp = (tu->remainder_tus - tu->i_upper_boundary_count) *
					(tu->new_valid_boundary_link - 1);
			temp += (tu->i_upper_boundary_count * tu->new_valid_boundary_link);
			temp *= tu->nlanes;
		} else {
			temp = tu->nlanes * tu->remainder_tus * tu->new_valid_boundary_link;
		}

		temp1 = tu->i_lower_boundary_count * (tu->new_valid_boundary_link - 1);
		temp1 += tu->i_upper_boundary_count * tu->new_valid_boundary_link;
		temp1 *= tu->paired_tus * tu->nlanes;
		temp1_fp = drm_fixp_from_fraction(tu->n_symbols - temp1 - temp, tu->nlanes);
		tu->last_partial_lclk = fixp2int_ceil(temp1_fp);

		tu->tu_active_cycles = (int)((tu->n_tus_per_lane * tu->tu_size)
								+ tu->last_partial_lclk);
		tu->post_tu_hw_pipe_delay = 4 /*BS_on_the_link*/ + 1 /*BE_next_ren*/;
		temp = tu->pre_tu_hw_pipe_delay + tu->delay_start_link_lclk + tu->tu_active_cycles +
				tu->post_tu_hw_pipe_delay;

		if (tu->fec_en == 1) {
			if (tu->nlanes == 1) {
				temp1_fp = drm_fixp_from_fraction(temp, 500);
				tu->parity_symbols = fixp2int_ceil(temp1_fp) * 12 + 1;
			} else {
				temp1_fp = drm_fixp_from_fraction(temp, 250);
				tu->parity_symbols = fixp2int_ceil(temp1_fp) * 6 + 1;
			}
		} else { //no fec BW impact
			tu->parity_symbols = 0;
		}

		tu->link_config_hactive_time = temp + tu->parity_symbols;

		if (tu->resolution_line_time >= tu->link_config_hactive_time + 1 /*margin*/)
			tu->hbp_delayStartCheck = 1;
		else
			tu->hbp_delayStartCheck = 0;
	} else {
		compare_result_3 = _tu_param_compare(tu->hbp_time_fp, tu->delay_start_time_fp);
		if (compare_result_3 < 2)
			tu->hbp_delayStartCheck = 1;
		else
			tu->hbp_delayStartCheck = 0;
	}

	compare_result_1 = _tu_param_compare(tu->n_n_err_fp, tu->diff_abs_fp);
	if (compare_result_1 == 2)
		compare_result_1 = 1;
	else
		compare_result_1 = 0;

	compare_result_2 = _tu_param_compare(tu->n_n_err_fp, tu->err_fp);
	if (compare_result_2 == 2)
		compare_result_2 = 1;
	else
		compare_result_2 = 0;

	if (((tu->even_distribution == 1) ||
			((tu->even_distribution_BF == 0) &&
			(tu->even_distribution_legacy == 0))) &&
			tu->n_err_fp >= 0 && tu->n_n_err_fp >= 0 &&
			compare_result_2 &&
			(compare_result_1 || (tu->min_hblank_violated == 1)) &&
			(tu->new_valid_boundary_link - 1) > 0 &&
			(tu->hbp_delayStartCheck == 1) &&
			(tu->delay_start_link_tmp <= 1023)) {
		tu->upper_boundary_count = tu->i_upper_boundary_count;
		tu->lower_boundary_count = tu->i_lower_boundary_count;
		tu->err_fp = tu->n_n_err_fp;
		tu->boundary_moderation_en = true;
		tu->tu_size_desired = tu->tu_size;
		tu->valid_boundary_link = tu->new_valid_boundary_link;
		tu->effective_valid_recorded_fp = tu->effective_valid_fp;
		tu->even_distribution_BF = 1;
		tu->delay_start_link = tu->delay_start_link_tmp;
	} else if (tu->boundary_mod_lower_err == 0) {
		compare_result_1 = _tu_param_compare(tu->n_n_err_fp, tu->diff_abs_fp);
		if (compare_result_1 == 2)
			tu->boundary_mod_lower_err = 1;
	}
}

static void _dp_calc_boundary(struct tu_algo_data *tu)
{
	s64 temp1_fp = 0, temp2_fp = 0;

	do {
		tu->err_fp = drm_fixp_from_fraction(1000, 1);

		temp1_fp = drm_fixp_div(tu->lclk_fp, tu->pclk_fp);
		temp2_fp = drm_fixp_from_fraction(tu->delay_start_link_extra_pixclk, 1);
		temp1_fp = drm_fixp_mul(temp2_fp, temp1_fp);
		tu->extra_buffer_margin = fixp2int_ceil(temp1_fp);

		temp1_fp = drm_fixp_from_fraction(tu->bpp, 8);
		temp1_fp = drm_fixp_mul(tu->lwidth_fp, temp1_fp);
		tu->n_symbols = fixp2int_ceil(temp1_fp);

		for (tu->tu_size = 32; tu->tu_size <= 64; tu->tu_size++) {
			for (tu->i_upper_boundary_count = 1;
				tu->i_upper_boundary_count <= 15;
				tu->i_upper_boundary_count++) {
				for (tu->i_lower_boundary_count = 1;
					tu->i_lower_boundary_count <= 15;
					tu->i_lower_boundary_count++) {
					_tu_valid_boundary_calc(tu);
				}
			}
		}
		tu->delay_start_link_extra_pixclk--;
	} while (!tu->boundary_moderation_en &&
		tu->boundary_mod_lower_err == 1 &&
		tu->delay_start_link_extra_pixclk != 0 &&
		((tu->second_loop_set == 0 && tu->rb2 == 1) || tu->rb2 == 0));
}

static void _dp_calc_extra_bytes(struct tu_algo_data *tu)
{
	s64 temp1_fp = 0, temp2_fp = 0;

	temp1_fp = drm_fixp_from_fraction(tu->tu_size_desired, 1);
	temp2_fp = drm_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = drm_fixp_from_fraction(tu->valid_boundary_link, 1);

	temp2_fp = fixp_subtract(temp1_fp, temp2_fp);

	if ((unsigned int) temp2_fp <= DP_TU_FP_EDGE) {
		tu->extra_bytes = 0;
		DP_DEBUG("extra_bytes set to 0\n");
	} else {
		temp1_fp = drm_fixp_from_fraction(tu->n_tus + 1, 1);
		temp2_fp = drm_fixp_mul(temp1_fp, temp2_fp);
		tu->extra_bytes = fixp2int_ceil(temp2_fp);
	}

	temp1_fp = drm_fixp_from_fraction(tu->extra_bytes, 1);
	temp2_fp = drm_fixp_from_fraction(8, tu->bpp);
	temp1_fp = drm_fixp_mul(temp1_fp, temp2_fp);
	tu->extra_pclk_cycles = fixp2int_ceil(temp1_fp);

	temp1_fp = drm_fixp_div(tu->lclk_fp, tu->pclk_fp);
	temp2_fp = drm_fixp_from_fraction(tu->extra_pclk_cycles, 1);
	temp1_fp = drm_fixp_mul(temp2_fp, temp1_fp);
	tu->extra_pclk_cycles_in_link_clk = fixp2int_ceil(temp1_fp);
}

void dp_tu_update_timings(struct dp_tu_calc_input *in, struct tu_algo_data *tu)
{
	int nlanes = in->nlanes;
	int dsc_num_slices = in->num_of_dsc_slices;
	int dsc_num_bytes  = 0;
	int numerator;
	s64 pclk_dsc_fp;
	s64 dwidth_dsc_fp;
	s64 hbp_dsc_fp;
	s64 overhead_dsc_fp;
	int tot_num_eoc_symbols       = 0;
	int tot_num_hor_bytes         = 0;
	int tot_num_dummy_bytes       = 0;
	int dwidth_dsc_bytes          = 0;
	int eoc_bytes                 = 0;
	s64 tot_num_hor_bytes_frac_fp = 0;
	s64 temp1_fp, temp2_fp, temp3_fp;

	tu->lclk_fp                   = drm_fixp_from_fraction(in->lclk, 1);
	tu->orig_lclk_fp              = tu->lclk_fp;
	tu->pclk_fp                   = drm_fixp_from_fraction(in->pclk_khz, 1000);
	tu->orig_pclk_fp              = tu->pclk_fp;
	tu->lwidth                    = in->hactive;
	tu->hbp_relative_to_pclk      = in->hporch;
	tu->nlanes                    = in->nlanes;
	tu->bpp                       = in->bpp;
	tu->pixelEnc                  = in->pixel_enc;
	tu->dsc_en                    = in->dsc_en;
	tu->fec_en                    = in->fec_en;
	tu->async_en                  = in->async_en;
	tu->lwidth_fp                 = drm_fixp_from_fraction(in->hactive, 1);
	tu->orig_lwidth               = in->hactive;
	tu->hbp_relative_to_pclk_fp   = drm_fixp_from_fraction(in->hporch, 1);
	tu->orig_hbp                  = in->hporch;
	tu->rb2                       = (in->hporch < 160) ? 1 : 0;
	tu->comp_bpp                  = in->comp_bpp;
	tu->ppc_div_factor            = in->ppc_div_factor;

	if (!in->comp_bpp)
		tu->comp_bpp = 8;

	if (in->ppc_div_factor != 4)
		tu->ppc_div_factor = 2;

	if (tu->pixelEnc == 420) {
		temp1_fp = drm_fixp_from_fraction(2, 1);
		tu->pclk_fp = drm_fixp_div(tu->pclk_fp, temp1_fp);
		tu->lwidth_fp = drm_fixp_div(tu->lwidth_fp, temp1_fp);
		tu->hbp_relative_to_pclk_fp = drm_fixp_div(tu->hbp_relative_to_pclk_fp, temp1_fp);
	}

	if (tu->pixelEnc == 422) {
		switch (tu->bpp) {
		case 24:
			tu->bpp = 16;
			tu->bpc = 8;
			break;
		case 30:
			tu->bpp = 20;
			tu->bpc = 10;
			break;
		default:
			tu->bpp = 16;
			tu->bpc = 8;
			break;
		}
	} else
		tu->bpc = tu->bpp/3;

	if (!in->dsc_en)
		goto fec_check;

	tu->bpp = 24; // hardcode to 24 if DSC is enabled.

	temp1_fp = drm_fixp_from_fraction(in->compress_ratio, 100);
	temp2_fp = drm_fixp_from_fraction(in->bpp, 1);
	temp3_fp = drm_fixp_div(temp2_fp, temp1_fp);
	temp2_fp = drm_fixp_mul(tu->lwidth_fp, temp3_fp);

	temp1_fp = drm_fixp_from_fraction(8, 1);
	temp3_fp = drm_fixp_div(temp2_fp, temp1_fp);

	numerator = drm_fixp2int(temp3_fp);

	dsc_num_bytes  = numerator / dsc_num_slices;
	eoc_bytes           = dsc_num_bytes % nlanes;
	tot_num_eoc_symbols = nlanes * dsc_num_slices;
	tot_num_hor_bytes   = dsc_num_bytes * dsc_num_slices;
	tot_num_dummy_bytes = (nlanes - eoc_bytes) * dsc_num_slices;

	temp1_fp = drm_fixp_from_fraction(8, 1);
	temp2_fp = drm_fixp_from_fraction(tu->orig_lwidth, 1);
	temp3_fp = drm_fixp_div(tu->comp_bpp, temp1_fp);
	tot_num_hor_bytes_frac_fp = drm_fixp_mul(temp3_fp, temp2_fp);

	if (dsc_num_bytes == 0)
		DP_WARN("incorrect no of bytes per slice=%d\n", dsc_num_bytes);

	dwidth_dsc_bytes = (tot_num_hor_bytes + tot_num_eoc_symbols +
						(eoc_bytes == 0 ? 0 : tot_num_dummy_bytes));

	temp1_fp = drm_fixp_from_fraction(dwidth_dsc_bytes, 1);
	overhead_dsc_fp = drm_fixp_div(temp1_fp, tot_num_hor_bytes_frac_fp);

	dwidth_dsc_fp = drm_fixp_from_fraction(dwidth_dsc_bytes, 3);

	temp2_fp = drm_fixp_mul(tu->pclk_fp, dwidth_dsc_fp);
	temp1_fp = drm_fixp_div(temp2_fp, tu->lwidth_fp);
	pclk_dsc_fp = temp1_fp;

	temp1_fp = drm_fixp_div(pclk_dsc_fp, tu->pclk_fp);
	temp2_fp = drm_fixp_mul(tu->hbp_relative_to_pclk_fp, temp1_fp);
	hbp_dsc_fp = temp2_fp;

	/* output */
	tu->pclk_fp = pclk_dsc_fp;
	tu->lwidth_fp = dwidth_dsc_fp;
	tu->hbp_relative_to_pclk_fp = hbp_dsc_fp;

fec_check:
	if (in->fec_en) {
		temp1_fp = drm_fixp_from_fraction(976, 1000); /* 0.976 */
		tu->lclk_fp = drm_fixp_mul(tu->lclk_fp, temp1_fp);
	}
}

void dp_tu_calculate(struct dp_tu_calc_input *in, struct dp_tu_calc_output *tu_table)
{
	struct tu_algo_data tu;
	int compare_result_1, compare_result_2;
	u64 temp = 0, temp1;
	s64 temp_fp = 0, temp1_fp = 0, temp2_fp = 0;

	s64 LCLK_FAST_SKEW_fp = drm_fixp_from_fraction(6, 10000); /* 0.0006 */
	s64 RATIO_SCALE_fp = drm_fixp_from_fraction(1001, 1000);

	u8 DP_BRUTE_FORCE = 1;
	s64 BRUTE_FORCE_THRESHOLD_fp = drm_fixp_from_fraction(1, 10); /* 0.1 */
	uint EXTRA_PIXCLK_CYCLE_DELAY = 4;
	s64 HBLANK_MARGIN = drm_fixp_from_fraction(4, 1);
	s64 HBLANK_MARGIN_EXTRA = 0;

	memset(&tu, 0, sizeof(tu));

	dp_tu_update_timings(in, &tu);

	tu.err_fp = drm_fixp_from_fraction(1000, 1); /* 1000 */

	temp1_fp = drm_fixp_from_fraction(4, 1);
	temp2_fp = drm_fixp_mul(temp1_fp, tu.lclk_fp);
	temp_fp = drm_fixp_div(temp2_fp, tu.pclk_fp);
	tu.extra_buffer_margin = fixp2int_ceil(temp_fp);

	if (in->compress_ratio == 375 && tu.bpp == 30)
		temp1_fp = drm_fixp_from_fraction(24, 8);
	else
		temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);

	temp2_fp = drm_fixp_mul(tu.pclk_fp, temp1_fp);
	temp1_fp = drm_fixp_from_fraction(tu.nlanes, 1);
	temp2_fp = drm_fixp_div(temp2_fp, temp1_fp);
	tu.ratio_fp = drm_fixp_div(temp2_fp, tu.lclk_fp);

	tu.original_ratio_fp = tu.ratio_fp;
	tu.boundary_moderation_en = false;
	tu.upper_boundary_count = 0;
	tu.lower_boundary_count = 0;
	tu.i_upper_boundary_count = 0;
	tu.i_lower_boundary_count = 0;
	tu.valid_lower_boundary_link = 0;
	tu.even_distribution_BF = 0;
	tu.even_distribution_legacy = 0;
	tu.even_distribution = 0;
	tu.hbp_delayStartCheck = 0;
	tu.pre_tu_hw_pipe_delay = 0;
	tu.post_tu_hw_pipe_delay = 0;
	tu.link_config_hactive_time = 0;
	tu.delay_start_link_lclk = 0;
	tu.tu_active_cycles = 0;
	tu.resolution_line_time = 0;
	tu.last_partial_lclk = 0;
	tu.delay_start_time_fp = 0;
	tu.second_loop_set = 0;

	tu.err_fp = drm_fixp_from_fraction(1000, 1);
	tu.n_err_fp = 0;
	tu.n_n_err_fp = 0;

	temp = drm_fixp2int(tu.lwidth_fp);
	if ((((u32)temp % tu.nlanes) != 0) && (_tu_param_compare(tu.ratio_fp, DRM_FIXED_ONE) == 2)
			&& (tu.dsc_en == 0)) {
		tu.ratio_fp = drm_fixp_mul(tu.ratio_fp, RATIO_SCALE_fp);
	}

	if (_tu_param_compare(tu.ratio_fp, DRM_FIXED_ONE) == 1)
		tu.ratio_fp = DRM_FIXED_ONE;

	if (HBLANK_MARGIN_EXTRA != 0) {
		HBLANK_MARGIN += HBLANK_MARGIN_EXTRA;
		DP_DEBUG("increased HBLANK_MARGIN to %lld. (PLUS%lld)\n",
			HBLANK_MARGIN, HBLANK_MARGIN_EXTRA);
	}

	for (tu.tu_size = 32; tu.tu_size <= 64; tu.tu_size++) {
		temp1_fp = drm_fixp_from_fraction(tu.tu_size, 1);
		temp2_fp = drm_fixp_mul(tu.ratio_fp, temp1_fp);
		temp = fixp2int_ceil(temp2_fp);
		temp1_fp = drm_fixp_from_fraction(temp, 1);
		tu.n_err_fp = fixp_subtract(temp1_fp, temp2_fp);

		if (tu.n_err_fp < tu.err_fp) {
			tu.err_fp = tu.n_err_fp;
			tu.tu_size_desired = tu.tu_size;
		}
	}

	tu.tu_size_minus1 = tu.tu_size_desired - 1;

	temp1_fp = drm_fixp_from_fraction(tu.tu_size_desired, 1);
	temp2_fp = drm_fixp_mul(tu.ratio_fp, temp1_fp);
	tu.valid_boundary_link = fixp2int_ceil(temp2_fp);

	temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);
	temp2_fp = tu.lwidth_fp;
	temp2_fp = drm_fixp_mul(temp2_fp, temp1_fp);

	temp1_fp = drm_fixp_from_fraction(tu.valid_boundary_link, 1);
	temp2_fp = drm_fixp_div(temp2_fp, temp1_fp);
	tu.n_tus = drm_fixp2int(temp2_fp);
	if ((temp2_fp & 0xFFFFFFFF) > 0xFFFFF000)
		tu.n_tus += 1;

	tu.even_distribution_legacy = tu.n_tus % tu.nlanes == 0 ? 1 : 0;

	DP_DEBUG("n_sym= %d, n_tus= %d\n", tu.valid_boundary_link, tu.n_tus);

	_dp_calc_extra_bytes(&tu);

	tu.filler_size = tu.tu_size_desired - tu.valid_boundary_link;

	temp1_fp = drm_fixp_from_fraction(tu.tu_size_desired, 1);
	tu.ratio_by_tu_fp = drm_fixp_mul(tu.ratio_fp, temp1_fp);

	tu.delay_start_link = tu.extra_pclk_cycles_in_link_clk +
				tu.filler_size + tu.extra_buffer_margin;

	tu.resulting_valid_fp = drm_fixp_from_fraction(tu.valid_boundary_link, 1);

	temp1_fp = drm_fixp_from_fraction(tu.tu_size_desired, 1);
	temp2_fp = drm_fixp_div(tu.resulting_valid_fp, temp1_fp);
	tu.TU_ratio_err_fp = temp2_fp - tu.original_ratio_fp;

	temp1_fp = drm_fixp_from_fraction((tu.hbp_relative_to_pclk - HBLANK_MARGIN), 1);
	tu.hbp_time_fp = drm_fixp_div(temp1_fp, tu.pclk_fp);

	temp1_fp = drm_fixp_from_fraction(tu.delay_start_link, 1);
	tu.delay_start_time_fp = drm_fixp_div(temp1_fp, tu.lclk_fp);

	compare_result_1 = _tu_param_compare(tu.hbp_time_fp, tu.delay_start_time_fp);
	if (compare_result_1 == 2) /* hbp_time_fp < delay_start_time_fp */
		tu.min_hblank_violated = 1;

	tu.hactive_time_fp = drm_fixp_div(tu.lwidth_fp, tu.pclk_fp);

	compare_result_2 = _tu_param_compare(tu.hactive_time_fp,
						tu.delay_start_time_fp);
	if (compare_result_2 == 2)
		tu.min_hblank_violated = 1;

	/* brute force */

	tu.delay_start_link_extra_pixclk = EXTRA_PIXCLK_CYCLE_DELAY;
	tu.diff_abs_fp = tu.resulting_valid_fp - tu.ratio_by_tu_fp;

	temp = drm_fixp2int(tu.diff_abs_fp);
	if (!temp && tu.diff_abs_fp <= 0xffff)
		tu.diff_abs_fp = 0;

	/* if(diff_abs < 0) diff_abs *= -1 */
	if (tu.diff_abs_fp < 0)
		tu.diff_abs_fp = drm_fixp_mul(tu.diff_abs_fp, -1);

	tu.boundary_mod_lower_err = 0;

	temp1_fp = drm_fixp_div(tu.orig_lclk_fp, tu.orig_pclk_fp);

	temp2_fp = drm_fixp_from_fraction(tu.orig_lwidth + tu.orig_hbp, tu.ppc_div_factor);

	temp_fp = drm_fixp_mul(temp1_fp, temp2_fp);
	tu.resolution_line_time = drm_fixp2int(temp_fp);
	tu.pre_tu_hw_pipe_delay = fixp2int_ceil(temp1_fp)
				+ 2 /*cdc fifo write jitter+2*/ + 3 /*pre-delay start cycles*/
				+ 3 /*post-delay start cycles*/ + 1 /*BE on the link*/;
	tu.post_tu_hw_pipe_delay = 4 /*BS_on_the_link*/ + 1 /*BE_next_ren*/;

	temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);
	temp1_fp = drm_fixp_mul(tu.lwidth_fp, temp1_fp);
	tu.n_symbols = fixp2int_ceil(temp1_fp);

	if (tu.rb2) {
		temp1_fp = drm_fixp_mul(tu.delay_start_time_fp, tu.lclk_fp);
		tu.delay_start_link_lclk = fixp2int_ceil(temp1_fp);

		tu.new_valid_boundary_link = tu.valid_boundary_link;
		tu.i_upper_boundary_count = 1;
		tu.i_lower_boundary_count = 0;

		temp1 = tu.i_upper_boundary_count * tu.new_valid_boundary_link;
		temp1 += tu.i_lower_boundary_count * (tu.new_valid_boundary_link - 1);
		temp = tu.i_upper_boundary_count + tu.i_lower_boundary_count;
		tu.average_valid2_fp = drm_fixp_from_fraction(temp1, temp);

		temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);
		temp1_fp = drm_fixp_mul(tu.lwidth_fp, temp1_fp);
		temp2_fp = drm_fixp_div(temp1_fp, tu.average_valid2_fp);
		tu.n_tus = drm_fixp2int(temp2_fp);

		tu.n_tus_per_lane = tu.n_tus / tu.nlanes;
		tu.paired_tus = (int)((tu.n_tus_per_lane) / temp);

		tu.remainder_tus = tu.n_tus_per_lane - tu.paired_tus * temp;

		if (tu.remainder_tus > tu.i_upper_boundary_count) {
			temp = (tu.remainder_tus - tu.i_upper_boundary_count)
					* (tu.new_valid_boundary_link - 1);
			temp += (tu.i_upper_boundary_count * tu.new_valid_boundary_link);
			temp *= tu.nlanes;
		} else {
			temp = tu.nlanes * tu.remainder_tus * tu.new_valid_boundary_link;
		}

		temp1 = tu.i_lower_boundary_count * (tu.new_valid_boundary_link - 1);
		temp1 += tu.i_upper_boundary_count * tu.new_valid_boundary_link;
		temp1 *= tu.paired_tus * tu.nlanes;
		temp1_fp = drm_fixp_from_fraction(tu.n_symbols - temp1 - temp, tu.nlanes);
		tu.last_partial_lclk = fixp2int_ceil(temp1_fp);

		tu.tu_active_cycles = (int)((tu.n_tus_per_lane * tu.tu_size)
								+ tu.last_partial_lclk);

		temp = tu.pre_tu_hw_pipe_delay + tu.delay_start_link_lclk + tu.tu_active_cycles
				+ tu.post_tu_hw_pipe_delay;

		if (tu.fec_en == 1) {
			if (tu.nlanes == 1) {
				temp1_fp = drm_fixp_from_fraction(temp, 500);
				tu.parity_symbols = fixp2int_ceil(temp1_fp) * 12 + 1;
			} else {
				temp1_fp = drm_fixp_from_fraction(temp, 250);
				tu.parity_symbols = fixp2int_ceil(temp1_fp) * 6 + 1;
			}
		} else { //no fec BW impact
			tu.parity_symbols = 0;
		}

		tu.link_config_hactive_time = temp + tu.parity_symbols;

		if (tu.link_config_hactive_time + 1 /*margin*/ >= tu.resolution_line_time)
			tu.min_hblank_violated = 1;
	}

	tu.delay_start_time_fp = 0;

	if ((tu.diff_abs_fp != 0 &&
			((tu.diff_abs_fp > BRUTE_FORCE_THRESHOLD_fp) ||
			 (tu.even_distribution_legacy == 0) ||
			 (DP_BRUTE_FORCE == 1))) ||
			(tu.min_hblank_violated == 1)) {
		_dp_calc_boundary(&tu);

		if (tu.boundary_moderation_en) {
			temp1_fp = drm_fixp_from_fraction(
						(tu.upper_boundary_count * tu.valid_boundary_link
						+ tu.lower_boundary_count
						* (tu.valid_boundary_link - 1)), 1);
			temp2_fp = drm_fixp_from_fraction(
					(tu.upper_boundary_count + tu.lower_boundary_count), 1);

			tu.resulting_valid_fp = drm_fixp_div(temp1_fp, temp2_fp);

			temp1_fp = drm_fixp_from_fraction(tu.tu_size_desired, 1);
			tu.ratio_by_tu_fp = drm_fixp_mul(tu.original_ratio_fp, temp1_fp);

			tu.valid_lower_boundary_link = tu.valid_boundary_link - 1;

			temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);
			temp1_fp = drm_fixp_mul(tu.lwidth_fp, temp1_fp);
			temp2_fp = drm_fixp_div(temp1_fp, tu.resulting_valid_fp);
			tu.n_tus = drm_fixp2int(temp2_fp);

			tu.tu_size_minus1 = tu.tu_size_desired - 1;
			tu.even_distribution_BF = 1;

			temp1_fp = drm_fixp_from_fraction(tu.tu_size_desired, 1);
			temp2_fp = drm_fixp_div(tu.resulting_valid_fp, temp1_fp);
			tu.TU_ratio_err_fp = temp2_fp - tu.original_ratio_fp;
		}
	}

	if (tu.async_en) {
		temp2_fp = drm_fixp_mul(LCLK_FAST_SKEW_fp, tu.lwidth_fp);
		temp = fixp2int_ceil(temp2_fp);

		temp1_fp = drm_fixp_from_fraction(tu.nlanes, 1);
		temp2_fp = drm_fixp_mul(tu.original_ratio_fp, temp1_fp);
		temp1_fp = drm_fixp_from_fraction(tu.bpp, 8);
		temp2_fp = drm_fixp_div(temp1_fp, temp2_fp);
		temp1_fp = drm_fixp_from_fraction(temp, 1);
		temp2_fp = drm_fixp_mul(temp1_fp, temp2_fp);
		temp = drm_fixp2int(temp2_fp);

		tu.delay_start_link += (int)temp;
	}

	temp1_fp = drm_fixp_from_fraction(tu.delay_start_link, 1);
	tu.delay_start_time_fp = drm_fixp_div(temp1_fp, tu.lclk_fp);

	/* OUTPUTS */
	tu_table->valid_boundary_link       = tu.valid_boundary_link;
	tu_table->delay_start_link          = tu.delay_start_link;
	tu_table->boundary_moderation_en    = tu.boundary_moderation_en;
	tu_table->valid_lower_boundary_link = tu.valid_lower_boundary_link;
	tu_table->upper_boundary_count      = tu.upper_boundary_count;
	tu_table->lower_boundary_count      = tu.lower_boundary_count;
	tu_table->tu_size_minus1            = tu.tu_size_minus1;

	DP_INFO("TU: %d %d %d %d %d %d %d\n",
		tu_table->valid_boundary_link,
		tu_table->delay_start_link,
		tu_table->boundary_moderation_en,
		tu_table->valid_lower_boundary_link,
		tu_table->upper_boundary_count,
		tu_table->lower_boundary_count,
		tu_table->tu_size_minus1);
}

u32 dp_tu_dsc_get_num_extra_pclk(u32 pclk_factor, struct dp_tu_compression_info *input)
{
	u32 extra_width = 0;
	unsigned int dto_n = 0, dto_d = 0, remainder;
	int ack_required, last_few_ack_required, accum_ack;
	int last_few_pclk, last_few_pclk_required;
	int ppc_div_factor = (input->ppc_div_factor == 4) ? 4 : 2;
	int start, temp, line_width = input->pic_width / ppc_div_factor;
	s64 temp1_fp;
	struct dp_dsc_dto_params dsc_params;

	dsc_params.src_bpp = input->src_bpp;
	dsc_params.tgt_bpp = input->tgt_bpp;

	dp_panel_get_dto_params(pclk_factor, &dsc_params);

	dto_n = dsc_params.num;
	dto_d = dsc_params.denom;
	ack_required = input->pclk_per_line;

	/* number of pclk cycles left outside of the complete DTO set */
	last_few_pclk = line_width % dto_d;

	/* number of pclk cycles outside of the complete dto */
	temp1_fp = drm_fixp_from_fraction(line_width, dto_d);
	temp = drm_fixp2int(temp1_fp);
	temp = dto_n * temp;
	last_few_ack_required = ack_required - temp;

	/*
	 * check how many more pclk is needed to
	 * accommodate the last few ack required
	 */
	remainder = dto_n;
	accum_ack = 0;
	last_few_pclk_required = 0;

	/* invalid dto_n */
	if (dto_n == 0)
		return 0;

	while (accum_ack < last_few_ack_required) {
		last_few_pclk_required++;

		if (remainder >= dto_n)
			start = remainder;
		else
			start = remainder + dto_d;

		remainder = start - dto_n;
		if (remainder < dto_n)
			accum_ack++;
	}

	/* if fewer pclk than required */
	if (last_few_pclk < last_few_pclk_required)
		extra_width = last_few_pclk_required - last_few_pclk;
	else
		extra_width = 0;

	DP_DEBUG_V("extra pclks required: %d\n", extra_width);

	return extra_width;
}

u32 dp_tu_dhdr_pkt_limit(struct dp_tu_dhdr_info *input)
{
	s64 mdpclk_fp = drm_fixp_from_fraction(input->mdp_clk, 1000000);
	s64 lclk_fp = drm_fixp_from_fraction(input->lclk, 1000);
	s64 pclk_fp = drm_fixp_from_fraction(input->pclk, 1000);
	s64 nlanes_fp = drm_int2fixp(input->nlanes);
	s64 target_sc = input->mst_target_sc;
	s64 hactive_fp = drm_int2fixp(input->h_active);
	const s64 i1_fp = DRM_FIXED_ONE;
	const s64 i2_fp = drm_int2fixp(2);
	const s64 i10_fp = drm_int2fixp(10);
	const s64 i56_fp = drm_int2fixp(56);
	const s64 i64_fp = drm_int2fixp(64);
	s64 mst_bw_fp = i1_fp;
	s64 fec_factor_fp = i1_fp;
	s64 mst_bw64_fp, mst_bw64_ceil_fp, nlanes56_fp;
	u32 f1, f2, f3, f4, f5, deploy_period, target_period;
	s64 f3_f5_slot_fp;
	u32 calc_pkt_limit;
	const u32 max_pkt_limit = 64;

	if (input->fec_en && input->mst_en)
		fec_factor_fp = drm_fixp_from_fraction(64000, 65537);

	if (input->mst_en)
		mst_bw_fp = drm_fixp_div(target_sc, i64_fp);

	f1 = fixp2int_ceil(drm_fixp_div(drm_fixp_mul(i10_fp, lclk_fp), mdpclk_fp));
	f2 = fixp2int_ceil(drm_fixp_div(drm_fixp_mul(i2_fp, lclk_fp), mdpclk_fp))
			+ fixp2int_ceil(drm_fixp_div(drm_fixp_mul(i1_fp, lclk_fp), mdpclk_fp));

	mst_bw64_fp = drm_fixp_mul(mst_bw_fp, i64_fp);
	if (drm_fixp2int(mst_bw64_fp) == 0)
		f3_f5_slot_fp = drm_fixp_div(i1_fp, drm_int2fixp(fixp2int_ceil(
				drm_fixp_div(i1_fp, mst_bw64_fp))));
	else
		f3_f5_slot_fp = drm_int2fixp(drm_fixp2int(mst_bw64_fp));

	mst_bw64_ceil_fp = drm_int2fixp(fixp2int_ceil(mst_bw64_fp));
	f3 = 2 + drm_fixp2int(drm_fixp_mul(drm_int2fixp(drm_fixp2int(
				drm_fixp_div(i2_fp, f3_f5_slot_fp)) + 1),
				(i64_fp - mst_bw64_ceil_fp)));

	if (!input->mst_en) {
		f4 = 1 + drm_fixp2int(drm_fixp_div(drm_int2fixp(50), nlanes_fp))
				+ drm_fixp2int(drm_fixp_div(nlanes_fp, i2_fp));
		f5 = 0;
	} else {
		f4 = 0;
		nlanes56_fp = drm_fixp_div(i56_fp, nlanes_fp);
		f5 = drm_fixp2int(drm_fixp_mul(drm_int2fixp(drm_fixp2int(
				drm_fixp_div(i1_fp + nlanes56_fp, f3_f5_slot_fp)) + 1),
					(i64_fp - mst_bw64_ceil_fp + i1_fp + nlanes56_fp)));
	}

	deploy_period = f1 + f2 + f3 + f4 + f5 + 19;
	target_period = drm_fixp2int(drm_fixp_mul(fec_factor_fp, drm_fixp_mul(
			hactive_fp, drm_fixp_div(lclk_fp, pclk_fp))));

	calc_pkt_limit = target_period / deploy_period;

	DP_DEBUG("input: %d, %d, %d, %d, %d, 0x%llx, %d, %d\n",
		input->mdp_clk, input->lclk, input->pclk, input->h_active,
		input->nlanes, input->mst_target_sc, input->mst_en ? 1 : 0,
		input->fec_en ? 1 : 0);
	DP_DEBUG("factors: %d, %d, %d, %d, %d\n", f1, f2, f3, f4, f5);
	DP_DEBUG("d_p: %d, t_p: %d, maxPkts: %d%s\n",
		deploy_period, target_period, calc_pkt_limit,
		calc_pkt_limit > max_pkt_limit ? " CAPPED" : "");

	if (calc_pkt_limit > max_pkt_limit)
		calc_pkt_limit = max_pkt_limit;

	DP_DEBUG("packet limit per line = %d\n", calc_pkt_limit);
	return calc_pkt_limit;
}

void dp_tu_mst_rg_calc(struct dp_tu_mst_rg_in *in, struct dp_tu_mst_rg_out *out)
{
	u64 pclk = in->pclk_khz;
	u64 lclk = in->lclk_khz;
	u64 lanes = in->nlanes;
	u64 bpp = in->dsc_en ? in->tgt_bpp : in->src_bpp;
	u64 pbn = in->pbn;
	u64 min_slot_cnt_fp, max_slot_cnt_fp;
	u64 raw_target_sc_fp, target_sc_fp;
	u64 ts_int;
	u32 x_int, y_frac_enum;
	u64 target_strm_sym_fp, y_frac_enum_fp;
	u64 numerator, denominator;
	u64 numerator_fp, denominator_fp;
	u64 temp;
	u64 temp_fp;

	DP_DEBUG("rg calc input: %llu %llu %llu %llu\n", lclk, pclk, bpp, pbn);

	/* min_slot_cnt */
	numerator = pclk * bpp * 64 * 1000;
	denominator = lclk * lanes * 8 * 1000;
	min_slot_cnt_fp = drm_fixp_from_fraction(numerator, denominator);

	/* max_slot_cnt */
	numerator = pbn * 54 * 1000;
	denominator = lclk * lanes;

	if (in->margin_ovrd) {
		numerator *= 1000;
		denominator *= 1006;
	}
	max_slot_cnt_fp = drm_fixp_from_fraction(numerator, denominator);

	/* raw_target_sc */
	numerator_fp = max_slot_cnt_fp + min_slot_cnt_fp;
	denominator_fp = drm_fixp_from_fraction(2, 1);
	raw_target_sc_fp = drm_fixp_div(numerator_fp, denominator_fp);

	/* apply fec and dsc overhead factor */
	if (in->fec_en)
		raw_target_sc_fp = drm_fixp_mul(raw_target_sc_fp, in->fec_overhead_fp);
	if (in->dsc_en)
		raw_target_sc_fp = drm_fixp_mul(raw_target_sc_fp, in->dsc_overhead_fp);

	/* Target_SC from raw_target_sc */
	denominator_fp = drm_fixp_from_fraction(256 * lanes, 1);
	temp_fp = drm_fixp_mul(raw_target_sc_fp, denominator_fp);
	numerator_fp = drm_fixp_from_fraction(fixp2int_round(temp_fp), 1);
	target_sc_fp = drm_fixp_div(numerator_fp, denominator_fp);

	/* TS_INT_PLUS1 */
	ts_int = drm_fixp2int(target_sc_fp);
	temp = drm_fixp2int_ceil(target_sc_fp);
	if (temp != ts_int)
		ts_int = temp;

	/* target_strm_sym */
	temp_fp = drm_fixp_from_fraction(lanes, 1);
	target_strm_sym_fp = drm_fixp_mul(target_sc_fp, temp_fp);

	/* X_INT */
	x_int = drm_fixp2int(target_strm_sym_fp);

	/* Y_FRAC_ENUM */
	temp_fp = drm_fixp_from_fraction(x_int, 1);
	temp_fp = fixp_subtract(target_strm_sym_fp, temp_fp);
	y_frac_enum_fp = drm_fixp_from_fraction(256, 1);
	y_frac_enum_fp = drm_fixp_mul(temp_fp, y_frac_enum_fp);
	y_frac_enum = drm_fixp2int(y_frac_enum_fp);

	out->min_sc_fp = min_slot_cnt_fp;
	out->max_sc_fp = max_slot_cnt_fp;
	out->target_sc_fp = target_sc_fp;
	out->ts_int = ts_int;
	out->x_int = x_int;
	out->y_frac_enum = y_frac_enum;

	DP_INFO("RG: 0x%llx 0x%llx 0x%llx %lld %d %d\n",
		min_slot_cnt_fp, max_slot_cnt_fp, target_sc_fp,
		ts_int, x_int, y_frac_enum);
}
