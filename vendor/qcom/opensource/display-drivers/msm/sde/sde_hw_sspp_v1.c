// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include "sde_hwio.h"
#include "sde_hw_catalog.h"
#include "sde_hw_lm.h"
#include "sde_hw_sspp.h"
#include "sde_hw_color_processing.h"
#include "sde_dbg.h"
#include "sde_kms.h"
#include "sde_hw_reg_dma_v1_color_proc.h"
#include "sde_hw_vbif.h"

#define SDE_FETCH_CONFIG_RESET_VALUE   0x00000087

/* CMN Registers -> Source Surface Processing Pipe Common SSPP registers */
/*      Name	                                Offset */
#define SSPP_CMN_CLK_CTRL	                    0x0
#define SSPP_CMN_CLK_STATUS	                    0x4
#define SSPP_CMN_MULTI_REC_OP_MODE	            0x10
#define SSPP_CMN_ADDR_CONFIG	                0x14
#define SSPP_CMN_CAC_CTRL	                    0x20
#define SSPP_CMN_SYS_CACHE_MODE	                0x24
#define SSPP_CMN_QOS_CTRL	                    0x28
#define SSPP_CMN_DANGER_LUT	                    0x2C
#define SSPP_CMN_SAFE_LUT	                    0x30
#define SSPP_CMN_CREQ_LUT_0	                    0x34
#define SSPP_CMN_CREQ_LUT_1	                    0x38
#define SSPP_CMN_FILL_LEVEL_SCALE	            0x3C
#define SSPP_CMN_FILL_LEVELS	                0x40
#define SSPP_CMN_STATUS	                        0x44
#define SSPP_CMN_FETCH_DMA_RD_OTS	            0x48
#define SSPP_CMN_FETCH_DTB_WR_PLANE0	        0x4C
#define SSPP_CMN_FETCH_DTB_WR_PLANE1	        0x50
#define SSPP_CMN_FETCH_DTB_WR_PLANE2	        0x54
#define SSPP_CMN_DTB_UNPACK_RD_PLANE0	        0x58
#define SSPP_CMN_DTB_UNPACK_RD_PLANE1	        0x5C
#define SSPP_CMN_DTB_UNPACK_RD_PLANE2	        0x60
#define SSPP_CMN_UNPACK_LINE_COUNT	            0x64
#define SSPP_CMN_TPG_CONTROL	                0x68
#define SSPP_CMN_TPG_CONFIG	                    0x6C
#define SSPP_CMN_TPG_COMPONENT_LIMITS	        0x70
#define SSPP_CMN_TPG_RECTANGLE	                0x74
#define SSPP_CMN_TPG_BLACK_WHITE_PATTERN_FRAMES	0x78
#define SSPP_CMN_TPG_RGB_MAPPING	            0x7C
#define SSPP_CMN_TPG_PATTERN_GEN_INIT_VAL	    0x80

/* REC Register set */
/*      Name	                                Offset */
#define SSPP_REC_SRC_FORMAT	                        0x0
#define SSPP_REC_SRC_UNPACK_PATTERN	                0x4
#define SSPP_REC_SRC_OP_MODE	                    0x8
#define SSPP_REC_SRC_CONSTANT_COLOR	                0xC
#define SSPP_REC_SRC_IMG_SIZE	                    0x10
#define SSPP_REC_SRC_SIZE	                        0x14
#define SSPP_REC_SRC_XY	                            0x18
#define SSPP_REC_OUT_SIZE	                        0x1C
#define SSPP_REC_OUT_XY	                            0x20
#define SSPP_REC_SW_PIX_EXT_LR	                    0x24
#define SSPP_REC_SW_PIX_EXT_TB	                    0x28
#define SSPP_REC_SRC_SIZE_ODX	                    0x30
#define SSPP_REC_SRC_XY_ODX	                        0x34
#define SSPP_REC_OUT_SIZE_ODX	                    0x38
#define SSPP_REC_OUT_XY_ODX	                        0x3C
#define SSPP_REC_SW_PIX_EXT_LR_ODX	                0x40
#define SSPP_REC_SW_PIX_EXT_TB_ODX	                0x44
#define SSPP_REC_PRE_DOWN_SCALE	                    0x48
#define SSPP_REC_SRC0_ADDR	                        0x4C
#define SSPP_REC_SRC1_ADDR	                        0x50
#define SSPP_REC_SRC2_ADDR	                        0x54
#define SSPP_REC_SRC3_ADDR	                        0x58
#define SSPP_REC_SRC_YSTRIDE0	                    0x5C
#define SSPP_REC_SRC_YSTRIDE1	                    0x60
#define SSPP_REC_CURRENT_SRC0_ADDR	                0x64
#define SSPP_REC_CURRENT_SRC1_ADDR	                0x68
#define SSPP_REC_CURRENT_SRC2_ADDR	                0x6C
#define SSPP_REC_CURRENT_SRC3_ADDR	                0x70
#define SSPP_REC_SRC_ADDR_SW_STATUS	                0x74
#define SSPP_REC_CDP_CNTL	                        0x78
#define SSPP_REC_TRAFFIC_SHAPER	                    0x7C
#define SSPP_REC_TRAFFIC_SHAPER_PREFILL	            0x80
#define SSPP_REC_PD_MEM_ALLOC	                    0x84
#define SSPP_REC_QOS_CLAMP	                        0x88
#define SSPP_REC_UIDLE_CTRL_VALUE	                0x8C
#define SSPP_REC_UBWC_STATIC_CTRL	                0x90
#define SSPP_REC_UBWC_STATIC_CTRL_OVERRIDE	        0x94
#define SSPP_REC_UBWC_STATS_ROI	                    0x98
#define SSPP_REC_UBWC_STATS_WORST_TILE_ROW_BW_ROI0	0x9C
#define SSPP_REC_UBWC_STATS_TOTAL_BW_ROI0	        0xA0
#define SSPP_REC_UBWC_STATS_WORST_TILE_ROW_BW_ROI1	0xA4
#define SSPP_REC_UBWC_STATS_TOTAL_BW_ROI1	        0xA8
#define SSPP_REC_UBWC_STATS_WORST_TILE_ROW_BW_ROI2	0xAC
#define SSPP_REC_UBWC_STATS_TOTAL_BW_ROI2	        0xB0
#define SSPP_REC_EXCL_REC_CTRL	                    0xB4
#define SSPP_REC_EXCL_REC_SIZE	                    0xB8
#define SSPP_REC_EXCL_REC_XY	                    0xBC
#define SSPP_REC_LINE_INSERTION_CTRL	            0xC0
#define SSPP_REC_LINE_INSERTION_OUT_SIZE	        0xC4
#define SSPP_REC_FETCH_PIPE_ACTIVE	                0xC8
#define SSPP_REC_META_ERROR_STATUS	                0xCC
#define SSPP_REC_UBWC_ERROR_STATUS	                0xD0
#define SSPP_REC_FLUSH_CTRL	                        0xD4
#define SSPP_REC_INTR_EN	                        0xD8
#define SSPP_REC_INTR_STATUS	                    0xDC
#define SSPP_REC_INTR_CLEAR	                        0xE0
#define SSPP_REC_HSYNC_STATUS	                    0xE4
#define SSPP_REC_FP16_CONFIG	                    0x150
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_R_0	        0x154
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_R_1	        0x158
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_G_0	        0x15C
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_G_1	        0x160
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_B_0	        0x164
#define SSPP_REC_FP16_CSC_MATRIX_COEFF_B_1	        0x168
#define SSPP_REC_FP16_CSC_PRE_CLAMP_R	            0x16C
#define SSPP_REC_FP16_CSC_PRE_CLAMP_G	            0x170
#define SSPP_REC_FP16_CSC_PRE_CLAMP_B	            0x174
#define SSPP_REC_FP16_CSC_POST_CLAMP	            0x178

/* SSPP_DGM */
#define SSPP_DGM_0                         0x9F0
#define SSPP_DGM_1                         0x19F0
#define SSPP_DGM_SIZE                      0x420
#define SSPP_DGM_CSC_0                     0x800
#define SSPP_DGM_CSC_1                     0x1800
#define SSPP_DGM_CSC_SIZE                  0xFC
#define VIG_GAMUT_SIZE                     0x1CC
#define SSPP_UCSC_SIZE                     0x80

#define MDSS_MDP_OP_DEINTERLACE            BIT(22)
#define MDSS_MDP_OP_DEINTERLACE_ODD        BIT(23)
#define MDSS_MDP_OP_IGC_ROM_1              BIT(18)
#define MDSS_MDP_OP_IGC_ROM_0              BIT(17)
#define MDSS_MDP_OP_IGC_EN                 BIT(16)
#define MDSS_MDP_OP_FLIP_UD                BIT(14)
#define MDSS_MDP_OP_FLIP_LR                BIT(13)
#define MDSS_MDP_OP_SPLIT_ORDER            BIT(4)
#define MDSS_MDP_OP_BWC_EN                 BIT(0)
#define MDSS_MDP_OP_ROT_90                 BIT(15)
#define MDSS_MDP_OP_PE_OVERRIDE            BIT(31)
#define MDSS_MDP_OP_BWC_LOSSLESS           (0 << 1)
#define MDSS_MDP_OP_BWC_Q_HIGH             (1 << 1)
#define MDSS_MDP_OP_BWC_Q_MED              (2 << 1)

#define SSPP_DECIMATION_CONFIG             0xB4

#define SSPP_VIG_OP_MODE                   0x4
#define SSPP_VIG_CSC_10_OP_MODE            0x0
#define SSPP_TRAFFIC_SHAPER_BPC_MAX        0xFF

static inline int _sspp_calculate_rect_off(enum sde_sspp_multirect_index rect_index)
{
	return (rect_index == SDE_SSPP_RECT_SOLO || rect_index == SDE_SSPP_RECT_0) ?
			SSPP_REC0_OFFSET_FROM_SSPP_CMN : SSPP_REC1_OFFSET_FROM_SSPP_CMN;
}

static inline int _sspp_check_rect_err(enum sde_sspp_multirect_index rect_index)
{
	return (rect_index == SDE_SSPP_RECT_SOLO || rect_index == SDE_SSPP_RECT_0 ||
			rect_index == SDE_SSPP_RECT_1) ? 0 : -1;
}

static void sde_hw_sspp_update_multirect_v1(struct sde_hw_pipe *ctx,
		bool enable,
		enum sde_sspp_multirect_index index,
		enum sde_sspp_multirect_mode mode)
{
	u32 mode_mask;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	if (index == SDE_SSPP_RECT_SOLO) {
		/**
		 * if rect index is RECT_SOLO, we cannot expect a
		 * virtual plane sharing the same SSPP id. So we go
		 * and disable multirect
		 */
		mode_mask = 0;
	} else {
		mode_mask = SDE_REG_READ(&ctx->hw, SSPP_CMN_MULTI_REC_OP_MODE + idx);

		if (enable)
			mode_mask |= index;
		else
			mode_mask &= ~index;

		if (enable && (mode == SDE_SSPP_MULTIRECT_TIME_MX))
			mode_mask |= BIT(2);
		else
			mode_mask &= ~BIT(2);
	}

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_MULTI_REC_OP_MODE + idx, mode_mask);
}

static void _sspp_setup_opmode_v1(struct sde_hw_pipe *ctx,
		u32 mask, u8 en)
{
	u32 idx;
	u32 opmode;

	if (!test_bit(SDE_SSPP_SCALER_QSEED2, &ctx->cap->features) ||
			sspp_subblk_offset(ctx, SDE_SSPP_SCALER_QSEED2, &idx) ||
			!test_bit(SDE_SSPP_CSC, &ctx->cap->features))
		return;

	opmode = SDE_REG_READ(&ctx->hw, SSPP_VIG_OP_MODE + idx);

	if (en)
		opmode |= mask;
	else
		opmode &= ~mask;

	SDE_REG_WRITE(&ctx->hw, SSPP_VIG_OP_MODE + idx, opmode);
}

static void _sspp_setup_csc10_opmode_v1(struct sde_hw_pipe *ctx,
		u32 mask, u8 en)
{
	u32 idx;
	u32 opmode;

	if (sspp_subblk_offset(ctx, SDE_SSPP_CSC_10BIT, &idx))
		return;

	opmode = SDE_REG_READ(&ctx->hw, SSPP_VIG_CSC_10_OP_MODE + idx);
	if (en)
		opmode |= mask;
	else
		opmode &= ~mask;

	SDE_REG_WRITE(&ctx->hw, SSPP_VIG_CSC_10_OP_MODE + idx, opmode);
}

static void sde_hw_sspp_set_src_split_order_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index rect_mode, bool enable)
{
	struct sde_hw_blk_reg_map *c;
	u32 opmode, idx, op_mode_off;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(rect_mode);

	op_mode_off = SSPP_REC_SRC_OP_MODE;

	c = &ctx->hw;
	opmode = SDE_REG_READ(c, op_mode_off + idx);

	if (enable)
		opmode |= MDSS_MDP_OP_SPLIT_ORDER;
	else
		opmode &= ~MDSS_MDP_OP_SPLIT_ORDER;

	SDE_REG_WRITE(c, op_mode_off + idx, opmode);
}

static void sde_hw_sspp_setup_ubwc_v1(struct sde_hw_pipe *ctx, struct sde_hw_blk_reg_map *c,
		const struct sde_format *fmt, bool const_alpha_en, bool const_color_en,
		enum sde_sspp_multirect_index rect_mode)
{
	u32 alpha_en_mask = 0, color_en_mask = 0, ubwc_ctrl_off, ctrl_val = 0;

	if ((rect_mode == SDE_SSPP_RECT_SOLO || rect_mode == SDE_SSPP_RECT_0) ||
			!test_bit(SDE_SSPP_UBWC_STATS, &ctx->cap->features)) {
		ubwc_ctrl_off = SSPP_REC_UBWC_STATIC_CTRL + SSPP_REC0_OFFSET_FROM_SSPP_CMN;
	} else {
		ubwc_ctrl_off = SSPP_REC_UBWC_STATIC_CTRL + SSPP_REC1_OFFSET_FROM_SSPP_CMN;
	}

	if (SDE_HW_MAJOR(ctx->catalog->ubwc_rev) >= SDE_HW_MAJOR(SDE_HW_UBWC_VER_50)) {
		if (!SDE_FORMAT_IS_YUV(fmt))
			ctrl_val |= (SDE_FORMAT_IS_DX(fmt) || SDE_FORMAT_IS_FP16(fmt)) ?
					BIT(30) : (BIT(31) | BIT(30));
		ctrl_val |= SDE_FORMAT_IS_UBWC_LOSSY_2_1(fmt) ? (0x3 << 16) : 0;
		ctrl_val |= SDE_FORMAT_IS_UBWC_LOSSY_8_5(fmt) ? BIT(16) : 0;
		SDE_REG_WRITE(c, ubwc_ctrl_off, ctrl_val);
	} else if (IS_UBWC_40_SUPPORTED(ctx->catalog->ubwc_rev)) {
		SDE_REG_WRITE(c, ubwc_ctrl_off, SDE_FORMAT_IS_YUV(fmt) ? 0 : BIT(30));
	} else if (IS_UBWC_30_SUPPORTED(ctx->catalog->ubwc_rev)) {
		color_en_mask = const_color_en ? BIT(30) : 0;
		SDE_REG_WRITE(c, ubwc_ctrl_off,
			color_en_mask | (ctx->mdp->ubwc_swizzle) |
			(ctx->mdp->highest_bank_bit << 4));
	} else if (IS_UBWC_20_SUPPORTED(ctx->catalog->ubwc_rev)) {
		alpha_en_mask = const_alpha_en ? BIT(31) : 0;
		SDE_REG_WRITE(c, ubwc_ctrl_off,
			alpha_en_mask | (ctx->mdp->ubwc_swizzle) |
			(ctx->mdp->highest_bank_bit << 4));
	} else if (IS_UBWC_10_SUPPORTED(ctx->catalog->ubwc_rev)) {
		alpha_en_mask = const_alpha_en ? BIT(31) : 0;
		SDE_REG_WRITE(c, ubwc_ctrl_off,
			alpha_en_mask | (ctx->mdp->ubwc_swizzle & 0x1) |
			BIT(8) | (ctx->mdp->highest_bank_bit << 4));
	}
}

/**
 * Setup source pixel format, flip,
 */
static void sde_hw_sspp_setup_format_v1(struct sde_hw_pipe *ctx,
		const struct sde_format *fmt,
		bool const_alpha_en, u32 flags,
		enum sde_sspp_multirect_index rect_mode)
{
	struct sde_hw_blk_reg_map *c;
	u32 chroma_samp, unpack, src_format;
	u32 opmode = 0;
	u32 idx;
	bool const_color_en = true;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx) || !fmt)
		return;

	idx += _sspp_calculate_rect_off(rect_mode);

	c = &ctx->hw;
	opmode = SDE_REG_READ(c, SSPP_REC_SRC_OP_MODE + idx);
	opmode &= ~(MDSS_MDP_OP_FLIP_LR | MDSS_MDP_OP_FLIP_UD |
			MDSS_MDP_OP_BWC_EN | MDSS_MDP_OP_PE_OVERRIDE
			| MDSS_MDP_OP_ROT_90);

	if (flags & SDE_SSPP_FLIP_LR)
		opmode |= MDSS_MDP_OP_FLIP_LR;
	if (flags & SDE_SSPP_FLIP_UD)
		opmode |= MDSS_MDP_OP_FLIP_UD;
	if (flags & SDE_SSPP_ROT_90)
		opmode |= MDSS_MDP_OP_ROT_90; /* ROT90 */

	chroma_samp = fmt->chroma_sample;
	if (flags & SDE_SSPP_SOURCE_ROTATED_90) {
		if (chroma_samp == SDE_CHROMA_H2V1)
			chroma_samp = SDE_CHROMA_H1V2;
		else if (chroma_samp == SDE_CHROMA_H1V2)
			chroma_samp = SDE_CHROMA_H2V1;
	}

	src_format = (chroma_samp << 23) | (fmt->fetch_planes << 19) |
		(fmt->bits[C3_ALPHA] << 6) | (fmt->bits[C2_R_Cr] << 4) |
		(fmt->bits[C1_B_Cb] << 2) | (fmt->bits[C0_G_Y] << 0);

	if (fmt->alpha_enable && fmt->fetch_planes == SDE_PLANE_INTERLEAVED)
		src_format |= BIT(8); /* SRCC3_EN */

	if (flags & SDE_SSPP_SOLID_FILL)
		src_format |= BIT(22);

	unpack = (fmt->element[3] << 24) | (fmt->element[2] << 16) |
		(fmt->element[1] << 8) | (fmt->element[0] << 0);
	src_format |= ((fmt->unpack_count - 1) << 12) |
		(fmt->unpack_tight << 17) |
		(fmt->unpack_align_msb << 18);

	if (fmt->bpp <= 8)
		src_format |= ((fmt->bpp - 1) << 9);

	if ((flags & SDE_SSPP_ROT_90) && test_bit(SDE_SSPP_INLINE_CONST_CLR,
			&ctx->cap->features))
		const_color_en = false;

	if (fmt->fetch_mode != SDE_FETCH_LINEAR) {
		if (SDE_FORMAT_IS_UBWC(fmt))
			opmode |= MDSS_MDP_OP_BWC_EN;
		src_format |= (fmt->fetch_mode & 3) << 30; /*FRAME_FORMAT */

		sde_hw_sspp_setup_ubwc_v1(ctx, c, fmt, const_alpha_en, const_color_en, rect_mode);
	}

	opmode |= MDSS_MDP_OP_PE_OVERRIDE;

	/* if this is YUV pixel format, enable CSC */
	if (SDE_FORMAT_IS_YUV(fmt))
		src_format |= BIT(15);

	if (SDE_FORMAT_IS_DX(fmt))
		src_format |= BIT(14);

	/* update scaler opmode, if appropriate */
	if (test_bit(SDE_SSPP_CSC, &ctx->cap->features))
		_sspp_setup_opmode_v1(ctx, VIG_OP_CSC_EN | VIG_OP_CSC_SRC_DATAFMT,
			SDE_FORMAT_IS_YUV(fmt));
	else if (test_bit(SDE_SSPP_CSC_10BIT, &ctx->cap->features))
		_sspp_setup_csc10_opmode_v1(ctx,
			VIG_CSC_10_EN | VIG_CSC_10_SRC_DATAFMT,
			SDE_FORMAT_IS_YUV(fmt));

	SDE_REG_WRITE(c, SSPP_REC_SRC_FORMAT + idx, src_format);
	SDE_REG_WRITE(c, SSPP_REC_SRC_UNPACK_PATTERN + idx, unpack);
	SDE_REG_WRITE(c, SSPP_REC_SRC_OP_MODE + idx, opmode);

	/* clear previous UBWC error */
	SDE_REG_WRITE(c, SSPP_REC_UBWC_ERROR_STATUS + idx, BIT(31));
}

static void sde_hw_sspp_clear_ubwc_error_v2(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;

	c = &ctx->hw;

	SDE_REG_WRITE(c, SSPP_REC_UBWC_ERROR_STATUS, BIT(31));
}

static u32 sde_hw_sspp_get_ubwc_error_v2(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;

	c = &ctx->hw;

	return SDE_REG_READ(c, SSPP_REC_UBWC_ERROR_STATUS);
}

static void sde_hw_sspp_clear_ubwc_error_v3(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(multirect_index);

	c = &ctx->hw;

	SDE_REG_WRITE(c, SSPP_REC_UBWC_ERROR_STATUS + idx, BIT(31));
}

static u32 sde_hw_sspp_get_ubwc_error_v3(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return -1;

	idx += _sspp_calculate_rect_off(multirect_index);

	c = &ctx->hw;

	return SDE_REG_READ(c, SSPP_REC_UBWC_ERROR_STATUS + idx);
}

static void sde_hw_sspp_clear_meta_error_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(multirect_index);

	c = &ctx->hw;

	SDE_REG_WRITE(c, SSPP_REC_META_ERROR_STATUS + idx, BIT(31));
}

static u32 sde_hw_sspp_get_meta_error_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return -1;

	idx += _sspp_calculate_rect_off(multirect_index);

	c = &ctx->hw;

	return SDE_REG_READ(c, SSPP_REC_META_ERROR_STATUS + idx);
}

static void sde_hw_sspp_ubwc_stats_set_roi_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index,
		struct sde_drm_ubwc_stats_roi *roi)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx;
	u32 ctrl_val = 0, roi_val = 0;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(multirect_index);

	c = &ctx->hw;

	ctrl_val = SDE_REG_READ(c, SSPP_REC_UBWC_STATIC_CTRL + idx);

	if (roi) {
		ctrl_val |= BIT(24);
		if (roi->y_coord0) {
			ctrl_val |= BIT(25);
			roi_val |= roi->y_coord0;

			if (roi->y_coord1) {
				ctrl_val |= BIT(26);
				roi_val |= (roi->y_coord1) << 0x10;
			}
		}
	} else {
		ctrl_val &= ~(BIT(24) | BIT(25) | BIT(26));
	}

	SDE_REG_WRITE(c, SSPP_REC_UBWC_STATIC_CTRL + idx, ctrl_val);
	SDE_REG_WRITE(c, SSPP_REC_UBWC_STATS_ROI + idx, roi_val);
}

static void sde_hw_sspp_ubwc_stats_get_data_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index multirect_index,
		struct sde_drm_ubwc_stats_data *data)
{
	struct sde_hw_blk_reg_map *c;
	u32 idx, value = 0;
	int i;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(multirect_index);

	idx += SSPP_REC_UBWC_STATS_WORST_TILE_ROW_BW_ROI0;

	c = &ctx->hw;

	for (i = 0; i < UBWC_STATS_MAX_ROI; i++) {
		value = SDE_REG_READ(c, idx);
		data->worst_bw[i] = value & 0xFFFF;
		data->worst_bw_y_coord[i] = (value >> 0x10) & 0xFFFF;
		data->total_bw[i] = SDE_REG_READ(c, idx + 4);
		idx += 8;
	}
}

static void sde_hw_sspp_setup_secure_v1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index rect_mode,
		bool enable)
{
	struct sde_hw_blk_reg_map *c;
	u32 secure = 0, secure_bit_mask;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	c = &ctx->hw;

	idx += _sspp_calculate_rect_off(rect_mode);

	if (rect_mode == SDE_SSPP_RECT_SOLO)
		secure_bit_mask = 0xF;
	else if (rect_mode == SDE_SSPP_RECT_0 || rect_mode == SDE_SSPP_RECT_1)
		secure_bit_mask = 0x5;
	else
		secure_bit_mask = 0xA;

	secure = SDE_REG_READ(c, SSPP_REC_SRC_ADDR_SW_STATUS + idx);

	if (enable)
		secure |= secure_bit_mask;
	else
		secure &= ~secure_bit_mask;

	SDE_REG_WRITE(c, SSPP_REC_SRC_ADDR_SW_STATUS + idx, secure);

	/* multiple planes share same sw_status register */
	wmb();
}


static void sde_hw_sspp_setup_pe_config(struct sde_hw_pipe *ctx,
		struct sde_hw_pixel_ext *pe_ext, bool cac_en)
{
	struct sde_hw_blk_reg_map *c;
	u8 color;
	u32 lr_pe[4], tb_pe[4], tot_req_pixels[4];
	const u32 bytemask = 0xff;
	const u32 shortmask = 0xffff;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx) || !pe_ext)
		return;

	c = &ctx->hw;

	/* program SW pixel extension override for all pipes*/
	for (color = 0; color < SDE_MAX_PLANES; color++) {
		/* color 2 has the same set of registers as color 1 */

		lr_pe[color] = ((pe_ext->right_ftch[color] & bytemask) << 24)|
			((pe_ext->right_rpt[color] & bytemask) << 16)|
			((pe_ext->left_ftch[color] & bytemask) << 8)|
			(pe_ext->left_rpt[color] & bytemask);

		tb_pe[color] = ((pe_ext->btm_ftch[color] & bytemask) << 24)|
			((pe_ext->btm_rpt[color] & bytemask) << 16)|
			((pe_ext->top_ftch[color] & bytemask) << 8)|
			(pe_ext->top_rpt[color] & bytemask);

		tot_req_pixels[color] = (((pe_ext->roi_h[color] +
			pe_ext->num_ext_pxls_top[color] +
			pe_ext->num_ext_pxls_btm[color]) & shortmask) << 16) |
			((pe_ext->roi_w[color] +
			pe_ext->num_ext_pxls_left[color] +
			pe_ext->num_ext_pxls_right[color]) & shortmask);
	}

	/* Use rec 0 */
	idx += SSPP_REC0_OFFSET_FROM_SSPP_CMN;
	/* color 0 */
	SDE_REG_WRITE(c, SSPP_REC_SW_PIX_EXT_LR + idx, lr_pe[0]);
	SDE_REG_WRITE(c, SSPP_REC_SW_PIX_EXT_TB + idx, tb_pe[0]);

	/* color 1 and color 2 */
	SDE_REG_WRITE(c, SSPP_REC_SW_PIX_EXT_LR_ODX + idx, lr_pe[1]);
	SDE_REG_WRITE(c, SSPP_REC_SW_PIX_EXT_TB_ODX + idx, tb_pe[1]);
}

static void sde_hw_sspp_setup_pre_downscale_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_inline_pre_downscale_cfg *pre_down)
{
	u32 idx, val;

	if (!ctx || !pre_down || sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	val = pre_down->pre_downscale_x_0 |
			(pre_down->pre_downscale_x_1 << 4) |
			(pre_down->pre_downscale_y_0 << 8) |
			(pre_down->pre_downscale_y_1 << 12);

	SDE_REG_WRITE(&ctx->hw, SSPP_REC_PRE_DOWN_SCALE +
		SSPP_REC0_OFFSET_FROM_SSPP_CMN + idx, val);
}

static void sde_hw_sspp_setup_rects_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_cfg *cfg,
		enum sde_sspp_multirect_index rect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 src_size, src_xy, dst_size, dst_xy, ystride0, ystride1;
	u32 decimation = 0;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx) || !cfg)
		return;

	c = &ctx->hw;

	idx += _sspp_calculate_rect_off(rect_index);

	/* src and dest rect programming */
	src_xy = (cfg->src_rect.y << 16) | (cfg->src_rect.x);
	src_size = (cfg->src_rect.h << 16) | (cfg->src_rect.w);
	dst_xy = (cfg->dst_rect.y << 16) | (cfg->dst_rect.x);
	dst_size = (cfg->dst_rect.h << 16) | (cfg->dst_rect.w);

	ystride0 = (cfg->layout.plane_pitch[0]) | (cfg->layout.plane_pitch[2] << 16);
	ystride1 = (cfg->layout.plane_pitch[1]) | (cfg->layout.plane_pitch[3] << 16);

	/* program scaler, phase registers, if pipes supporting scaling */
	if (ctx->cap->features & SDE_SSPP_SCALER) {
		/* program decimation */
		decimation = ((1 << cfg->horz_decimation) - 1) << 8;
		decimation |= ((1 << cfg->vert_decimation) - 1);
	}

	/* rectangle register programming */
	SDE_REG_WRITE(c, SSPP_REC_SRC_SIZE + idx, src_size);
	SDE_REG_WRITE(c, SSPP_REC_SRC_XY + idx, src_xy);
	SDE_REG_WRITE(c, SSPP_REC_OUT_SIZE + idx, dst_size);
	SDE_REG_WRITE(c, SSPP_REC_OUT_XY + idx, dst_xy);

	SDE_REG_WRITE(c, SSPP_REC_SRC_YSTRIDE0 + idx, ystride0);
	SDE_REG_WRITE(c, SSPP_REC_SRC_YSTRIDE1 + idx, ystride1);
	SDE_REG_WRITE(c, SSPP_REC_UBWC_STATS_ROI + idx, decimation);
}

/**
 * _sde_hw_sspp_setup_excl_rect_v1() - set exclusion rect configs
 * @ctx: Pointer to pipe context
 * @excl_rect: Exclusion rect configs
 */
static void _sde_hw_sspp_setup_excl_rect_v1(struct sde_hw_pipe *ctx,
		struct sde_rect *excl_rect,
		enum sde_sspp_multirect_index rect_index)
{
	struct sde_hw_blk_reg_map *c;
	u32 size, xy;
	u32 idx;
	u32 excl_ctrl = BIT(0);
	u32 enable_bit;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx) || !excl_rect)
		return;

	idx += _sspp_calculate_rect_off(rect_index);
	enable_bit = BIT(0);

	c = &ctx->hw;

	xy = (excl_rect->y << 16) | (excl_rect->x);
	size = (excl_rect->h << 16) | (excl_rect->w);

	/* Set if multi-rect disabled, read+modify only if multi-rect enabled */
	if (rect_index != SDE_SSPP_RECT_SOLO)
		excl_ctrl = SDE_REG_READ(c, SSPP_REC_EXCL_REC_CTRL + idx);

	if (!size) {
		SDE_REG_WRITE(c, SSPP_REC_EXCL_REC_CTRL + idx,
				excl_ctrl & ~enable_bit);
	} else {
		SDE_REG_WRITE(c, SSPP_REC_EXCL_REC_CTRL + idx,
				excl_ctrl | enable_bit);
		SDE_REG_WRITE(c, SSPP_REC_EXCL_REC_SIZE + idx, size);
		SDE_REG_WRITE(c, SSPP_REC_EXCL_REC_XY + idx, xy);
	}
}

static void sde_hw_sspp_setup_sourceaddress_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_cfg *cfg,
		enum sde_sspp_multirect_index rect_mode)
{
	int i;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(rect_mode);

	for (i = 0; i < ARRAY_SIZE(cfg->layout.plane_addr); i++)
		SDE_REG_WRITE(&ctx->hw, SSPP_REC_SRC0_ADDR + idx + i * 0x4,
				cfg->layout.plane_addr[i]);
}

u32 sde_hw_sspp_get_source_addr_v1(struct sde_hw_pipe *ctx, bool is_virtual)
{
	u32 idx;
	u32 offset = 0;

	if (!ctx || sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return 0;

	/* Assume REC0	*/
	idx += SSPP_REC0_OFFSET_FROM_SSPP_CMN;
	offset =  is_virtual ? (SSPP_REC_SRC1_ADDR + idx) : (SSPP_REC_SRC0_ADDR + idx);

	return SDE_REG_READ(&ctx->hw, offset);
}

static void sde_hw_sspp_setup_solidfill_v1(struct sde_hw_pipe *ctx, u32 color, enum
		sde_sspp_multirect_index rect_index)
{
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(rect_index);

	SDE_REG_WRITE(&ctx->hw, SSPP_REC_SRC_CONSTANT_COLOR + idx, color);
}

static void sde_hw_sspp_setup_qos_lut_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_qos_cfg *cfg)
{
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_DANGER_LUT + idx, cfg->danger_lut);
	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_SAFE_LUT + idx, cfg->safe_lut);

	if (ctx->cap && test_bit(SDE_PERF_SSPP_QOS_8LVL,
				&ctx->cap->perf_features)) {
		SDE_REG_WRITE(&ctx->hw, SSPP_CMN_CREQ_LUT_0 + idx, cfg->creq_lut);
		SDE_REG_WRITE(&ctx->hw, SSPP_CMN_CREQ_LUT_1 + idx,
				cfg->creq_lut >> 32);
	}
}

static void sde_hw_sspp_setup_qos_ctrl_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_qos_cfg *cfg)
{
	u32 idx;
	u32 qos_ctrl = 0;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	if (cfg->vblank_en) {
		qos_ctrl |= ((cfg->creq_vblank &
				SSPP_QOS_CTRL_CREQ_VBLANK_MASK) <<
				SSPP_QOS_CTRL_CREQ_VBLANK_OFF);
		qos_ctrl |= ((cfg->danger_vblank &
				SSPP_QOS_CTRL_DANGER_VBLANK_MASK) <<
				SSPP_QOS_CTRL_DANGER_VBLANK_OFF);
		qos_ctrl |= SSPP_QOS_CTRL_VBLANK_EN;
	}

	if (cfg->danger_safe_en)
		qos_ctrl |= SSPP_QOS_CTRL_DANGER_SAFE_EN;

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_QOS_CTRL + idx, qos_ctrl);
}

static void sde_hw_sspp_setup_ts_prefill_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_ts_cfg *cfg,
		enum sde_sspp_multirect_index index)
{
	u32 idx;
	u32 ts_offset, ts_prefill_offset;
	u32 ts_count = 0, ts_bytes = 0;
	const struct sde_sspp_cfg *cap;

	if (!ctx || !cfg || !ctx->cap)
		return;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	cap = ctx->cap;

	if ((index == SDE_SSPP_RECT_SOLO || index == SDE_SSPP_RECT_0) &&
			test_bit(SDE_PERF_SSPP_TS_PREFILL,
				&cap->perf_features)) {
		idx += SSPP_REC0_OFFSET_FROM_SSPP_CMN;
	} else if (index == SDE_SSPP_RECT_1 &&
			test_bit(SDE_PERF_SSPP_TS_PREFILL_REC1,
				&cap->perf_features)) {
		idx += SSPP_REC1_OFFSET_FROM_SSPP_CMN;
	} else {
		pr_err("%s: unexpected idx:%d\n", __func__, index);
		return;
	}

	ts_offset = SSPP_REC_TRAFFIC_SHAPER;
	ts_prefill_offset = SSPP_REC_TRAFFIC_SHAPER_PREFILL;

	if (cfg->time) {
		ts_count = DIV_ROUND_UP_ULL(TS_CLK * cfg->time, 1000000ULL);
		ts_bytes = DIV_ROUND_UP_ULL(cfg->size, ts_count);
		if (ts_bytes > SSPP_TRAFFIC_SHAPER_BPC_MAX)
			ts_bytes = SSPP_TRAFFIC_SHAPER_BPC_MAX;
	}

	if (ts_count)
		ts_bytes |= BIT(31) | BIT(27);

	SDE_REG_WRITE(&ctx->hw, ts_offset + idx, ts_bytes);
	SDE_REG_WRITE(&ctx->hw, ts_prefill_offset + idx, ts_count);
}

static void sde_hw_sspp_setup_cdp_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_cdp_cfg *cfg,
		enum sde_sspp_multirect_index index)
{
	u32 idx;
	u32 cdp_cntl = 0;
	u32 cdp_cntl_offset = 0;

	if (!ctx || !cfg)
		return;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(index);

	if (_sspp_check_rect_err(index) != 0) {
		pr_err("%s: unexpected idx:%d\n", __func__, index);
		return;
	}

	cdp_cntl_offset = SSPP_REC_CDP_CNTL + idx;

	if (cfg->enable)
		cdp_cntl |= BIT(0);
	if (cfg->ubwc_meta_enable)
		cdp_cntl |= BIT(1);
	if (cfg->tile_amortize_enable)
		cdp_cntl |= BIT(2);
	if (cfg->preload_ahead == SDE_SSPP_CDP_PRELOAD_AHEAD_64)
		cdp_cntl |= BIT(3);

	SDE_REG_WRITE(&ctx->hw, cdp_cntl_offset, cdp_cntl);
}

static void sde_hw_sspp_setup_sys_cache_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_sc_cfg *cfg)
{
	u32 idx, val;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	if (!cfg)
		return;

	val = SDE_REG_READ(&ctx->hw, SSPP_CMN_SYS_CACHE_MODE + idx);

	if (cfg->flags & SYS_CACHE_EN_FLAG)
		val = (val & ~BIT(15)) | ((cfg->rd_en & 0x1) << 15);

	if (cfg->flags & SYS_CACHE_SCID)
		val = (val & ~0x1F00) | ((cfg->rd_scid & 0x1f) << 8);

	if (cfg->flags & SYS_CACHE_OP_MODE)
		val = (val & ~0xC0000) | ((cfg->op_mode & 0x3) << 18);

	if (cfg->flags & SYS_CACHE_OP_TYPE)
		val = (val & ~0xF) | ((cfg->rd_op_type & 0xf) << 0);

	if (cfg->flags & SYS_CACHE_NO_ALLOC)
		val = (val & ~0x10) | ((cfg->rd_noallocate & 0x1) << 4);

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_SYS_CACHE_MODE + idx, val);
}

static void sde_hw_sspp_setup_uidle_fill_scale_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_uidle_cfg *cfg)
{
	u32 idx, fill_lvl;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	/* duplicate the v1 scale values for V2 and fal10 exit */
	fill_lvl = cfg->fill_level_scale & 0xF;
	fill_lvl |= (cfg->fill_level_scale & 0xF) << 8;
	fill_lvl |= (cfg->fill_level_scale & 0xF) << 16;

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_FILL_LEVEL_SCALE + idx, fill_lvl);
}

static void sde_hw_sspp_setup_uidle_v1(struct sde_hw_pipe *ctx,
		struct sde_hw_pipe_uidle_cfg *cfg,
		enum sde_sspp_multirect_index index)
{
	u32 idx, val;
	u32 offset;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	idx += _sspp_calculate_rect_off(index);

	offset = SSPP_REC_UIDLE_CTRL_VALUE;

	val = SDE_REG_READ(&ctx->hw, offset + idx);
	val = (val & ~BIT(31)) | (cfg->enable ? 0x0 : BIT(31));
	val = (val & ~0xFF00000) | (cfg->fal_allowed_threshold << 20);
	val = (val & ~0xF0000) | (cfg->fal10_exit_threshold << 16);
	val = (val & ~0xF00) | (cfg->fal10_threshold << 8);
	val = (val & ~0xF) | (cfg->fal1_threshold << 0);

	SDE_REG_WRITE(&ctx->hw, offset + idx, val);
}

bool sde_hw_sspp_setup_clk_force_ctrl_v1(struct sde_hw_blk_reg_map *hw,
		enum sde_clk_ctrl_type clk_ctrl, bool enable)
{
	u32 reg_val, new_val;

	if (!hw)
		return false;

	if (!SDE_CLK_CTRL_SSPP_VALID(clk_ctrl))
		return false;

	reg_val = SDE_REG_READ(hw, SSPP_CMN_CLK_CTRL);

	if (enable)
		new_val = reg_val | BIT(0);
	else
		new_val = reg_val & ~BIT(0);

	SDE_REG_WRITE(hw, SSPP_CMN_CLK_CTRL, new_val);
	wmb(); /* ensure write finished before progressing */

	return !(reg_val & BIT(0));
}

int sde_hw_sspp_get_clk_ctrl_status_v1(struct sde_hw_blk_reg_map *hw,
		enum sde_clk_ctrl_type clk_ctrl, bool *status)
{
	if (!hw)
		return -EINVAL;

	if (!SDE_CLK_CTRL_SSPP_VALID(clk_ctrl))
		return -EINVAL;

	*status = SDE_REG_READ(hw, SSPP_CMN_CLK_STATUS) & BIT(0);

	return 0;
}

static void sde_hw_sspp_setup_line_insertion_v1(struct sde_hw_pipe *ctx,
					     enum sde_sspp_multirect_index rect_index,
					     struct sde_hw_pipe_line_insertion_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c;
	u32 ctl_val = 0;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx) || !cfg)
		return;

	c = &ctx->hw;

	idx += _sspp_calculate_rect_off(rect_index);

	if (cfg->enable)
		ctl_val = BIT(31) |
			(cfg->dummy_lines << 16) |
			(cfg->first_active_lines << 8) |
			(cfg->active_lines);

	SDE_REG_WRITE(c, SSPP_REC_LINE_INSERTION_CTRL + idx, ctl_val);
	SDE_REG_WRITE(c, SSPP_REC_LINE_INSERTION_OUT_SIZE + idx, cfg->dst_h << 16);
}

static void sde_hw_sspp_setup_cac_v1(struct sde_hw_pipe *ctx, u32 cac_mode,
		bool fov_en, u32 pp_idx)
{
	u32 opmode;
	u32 idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	opmode = SDE_REG_READ(&ctx->hw, SSPP_CMN_CAC_CTRL + idx);
	if (cac_mode == SDE_CAC_UNPACK) {
		opmode |= BIT(8);
	} else if (cac_mode == SDE_CAC_FETCH) {
		opmode |= BIT(0) | BIT(8);
	} else if (cac_mode == SDE_CAC_LOOPBACK_FETCH) {
		if (SDE_SSPP_VALID_VIG(ctx->idx)) {
			opmode &= (pp_idx << 24);
			opmode |= BIT(0) | BIT(8) | BIT(16);
		} else
			opmode |= BIT(0) | BIT(8);
	} else {
		opmode |= 0xF << 24;
		opmode &= ~(BIT(0) | BIT(8) | BIT(16));
	}

	if (fov_en)
		opmode |= BIT(12);
	else
		opmode &= ~BIT(12);

	SDE_REG_WRITE(&ctx->hw, SSPP_CMN_CAC_CTRL + idx, opmode);
}

static void sde_hw_sspp_setup_img_size_v1(struct sde_hw_pipe *ctx,
	struct sde_rect *img_rec)
{
	u32 img_size, idx;

	if (sspp_subblk_offset(ctx, SDE_SSPP_SRC, &idx))
		return;

	img_size = img_rec->h << 16 | img_rec->w;

	/* Assume rec0 */
	idx += SSPP_REC0_OFFSET_FROM_SSPP_CMN;

	/* Assumes one rec, what happens on 2 recs */
	SDE_REG_WRITE(&ctx->hw, SSPP_REC_SRC_IMG_SIZE + idx, img_size);
}

void setup_layer_ops_v1(struct sde_hw_pipe *c,
		unsigned long features, unsigned long perf_features,
		bool is_virtual_pipe)
{
	int ret;

	if (test_bit(SDE_SSPP_SRC, &features)) {
		c->ops.setup_format[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_format_v1;
		c->ops.setup_rects[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_rects_v1;
		c->ops.setup_sourceaddress[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_sourceaddress_v1;
		c->ops.get_sourceaddress[MSM_DISP_OP_HWIO] = sde_hw_sspp_get_source_addr_v1;
		c->ops.setup_solidfill[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_solidfill_v1;
		c->ops.setup_pe[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_pe_config;
		c->ops.setup_secure_address[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_secure_v1;
		c->ops.set_src_split_order[MSM_DISP_OP_HWIO] = sde_hw_sspp_set_src_split_order_v1;
	}

	if (test_bit(SDE_SSPP_EXCL_RECT, &features))
		c->ops.setup_excl_rect[MSM_DISP_OP_HWIO] = _sde_hw_sspp_setup_excl_rect_v1;

	if (test_bit(SDE_PERF_SSPP_QOS, &features)) {
		c->ops.setup_qos_lut[MSM_DISP_OP_HWIO] =
			sde_hw_sspp_setup_qos_lut_v1;
		c->ops.setup_qos_ctrl[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_qos_ctrl_v1;
	}

	if (test_bit(SDE_PERF_SSPP_TS_PREFILL, &perf_features))
		c->ops.setup_ts_prefill[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_ts_prefill_v1;

	if (test_bit(SDE_SSPP_CSC, &features) ||
		test_bit(SDE_SSPP_CSC_10BIT, &features))
		c->ops.setup_csc[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_csc;

	if (test_bit(SDE_SSPP_DGM_CSC, &features))
		c->ops.setup_dgm_csc[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_dgm_csc;

	if (test_bit(SDE_SSPP_SCALER_QSEED2, &features)) {
		c->ops.setup_sharpening[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_sharpening;
		c->ops.setup_scaler[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_scaler;
	}

	if (sde_hw_sspp_multirect_enabled(c->cap))
		c->ops.update_multirect[MSM_DISP_OP_HWIO] = sde_hw_sspp_update_multirect_v1;

	if (test_bit(SDE_SSPP_CAC_V2, &features)) {
		c->ops.setup_cac_ctrl[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_cac_v1;
		c->ops.setup_img_size[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_img_size_v1;
	}

	if (test_bit(SDE_SSPP_SCALER_QSEED3, &features) ||
			test_bit(SDE_SSPP_SCALER_QSEED3LITE, &features)) {
		c->ops.setup_scaler[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_scaler3;
		c->ops.setup_scaler_lut[MSM_DISP_OP_HWIO] = is_qseed3_rev_qseed3lite(
				c->catalog) ? reg_dmav1_setup_scaler3lite_lut
				: reg_dmav1_setup_scaler3_lut;
		ret = reg_dmav1_init_sspp_op_v4(is_qseed3_rev_qseed3lite(
					c->catalog) ? SDE_SSPP_SCALER_QSEED3LITE
					: SDE_SSPP_SCALER_QSEED3, c);
		if (!ret) {
			c->ops.setup_scaler[MSM_DISP_OP_HWIO] = reg_dmav1_setup_vig_qseed3;
			c->ops.setup_scaler[MSM_DISP_OP_HFI] = reg_dmav1_setup_vig_qseed3;
			c->ops.setup_scaler_lut[MSM_DISP_OP_HFI] =
					c->ops.setup_scaler_lut[MSM_DISP_OP_HWIO];
			if (test_bit(SDE_SSPP_SRC, &features)) {
				c->ops.reg_dma_setup_pe[MSM_DISP_OP_HWIO] =
					reg_dmav1_setup_pe_config;
				c->ops.reg_dma_setup_pe[MSM_DISP_OP_HFI] =
					reg_dmav1_setup_pe_config;
			}

			if (test_bit(SDE_SSPP_PREDOWNSCALE, &features)) {
				c->ops.reg_dma_setup_pre_downscale[MSM_DISP_OP_HWIO] =
						reg_dmav1_setup_pre_downscale;
				c->ops.reg_dma_setup_pre_downscale[MSM_DISP_OP_HFI] =
						reg_dmav1_setup_pre_downscale;
			}
		} else {
			c->ops.setup_scaler_cac[MSM_DISP_OP_HWIO] =
				test_bit(SDE_SSPP_CAC_V2, &features) ?
				sde_hw_sspp_setup_scaler_cac : NULL;
		}
	}

	if (test_bit(SDE_SSPP_MULTIRECT_ERROR, &features)) {
		c->ops.get_meta_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_get_meta_error_v1;
		c->ops.clear_meta_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_clear_meta_error_v1;

		c->ops.get_ubwc_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_get_ubwc_error_v3;
		c->ops.clear_ubwc_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_clear_ubwc_error_v3;
	} else {
		c->ops.get_ubwc_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_get_ubwc_error_v2;
		c->ops.clear_ubwc_error[MSM_DISP_OP_HWIO] = sde_hw_sspp_clear_ubwc_error_v2;
	}

	if (test_bit(SDE_SSPP_PREDOWNSCALE, &features))
		c->ops.setup_pre_downscale[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_pre_downscale_v1;

	if (test_bit(SDE_PERF_SSPP_SYS_CACHE, &perf_features))
		c->ops.setup_sys_cache[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_sys_cache_v1;

	if (test_bit(SDE_PERF_SSPP_CDP, &perf_features))
		c->ops.setup_cdp[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_cdp_v1;

	if (test_bit(SDE_PERF_SSPP_UIDLE, &perf_features)) {
		c->ops.setup_uidle[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_uidle_v1;
		if (test_bit(SDE_PERF_SSPP_UIDLE_FILL_LVL_SCALE, &perf_features))
			c->ops.setup_uidle_fill_scale[MSM_DISP_OP_HWIO] =
					sde_hw_sspp_setup_uidle_fill_scale_v1;
	}

	setup_layer_ops_colorproc(c, features, is_virtual_pipe);

	if (test_bit(SDE_SSPP_DGM_INVERSE_PMA, &features))
		c->ops.setup_inverse_pma[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_dgm_inverse_pma;
	else if (test_bit(SDE_SSPP_INVERSE_PMA, &features))
		c->ops.setup_inverse_pma[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_inverse_pma;

	if (test_bit(SDE_SSPP_UBWC_STATS, &features)) {
		c->ops.set_ubwc_stats_roi[MSM_DISP_OP_HWIO] = sde_hw_sspp_ubwc_stats_set_roi_v1;
		c->ops.get_ubwc_stats_data[MSM_DISP_OP_HWIO] = sde_hw_sspp_ubwc_stats_get_data_v1;
	}
	if (test_bit(SDE_SSPP_LINE_INSERTION, &features))
		c->ops.setup_line_insertion[MSM_DISP_OP_HWIO] = sde_hw_sspp_setup_line_insertion_v1;
}
