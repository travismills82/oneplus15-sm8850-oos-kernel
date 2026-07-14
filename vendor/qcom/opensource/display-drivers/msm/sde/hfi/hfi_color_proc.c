// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "hfi_color_proc.h"
#include "hfi_defs_layer_color.h"
#include "hfi_defs_display_color.h"
#include "hfi_properties_display.h"
#include "sde_kms.h"
#include "sde_hw_dspp.h"

static struct hfi_display_pa_dither hfi_pa_dither_cached[DPU_MAX][DSPP_MAX] = {};

void hfi_sspp_setup_csc(struct sde_hw_pipe *ctx, struct sde_csc_cfg *data)
{
	struct hfi_csc hfi_cfg;
	int ret = 0;
	u32 prop_id = HFI_PROPERTY_LAYER_COLOR_CSC;

	if (!ctx || !data) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK\n", ctx, data);
		return;
	}

	prop_id = HFI_PACK_VERSION(1, 0, prop_id);
	hfi_cfg.flags = HFI_BUFF_FEATURE_ENABLE;
	for (int  i = 0; i < HFI_CSC_MATRIX_COEFF_SIZE; i++)
		hfi_cfg.ctm_coeff[i] = data->csc_mv[i];

	for (int  i = 0; i < HFI_CSC_BIAS_SIZE; i++) {
		hfi_cfg.pre_bias[i] = data->csc_pre_bv[i];
		hfi_cfg.post_bias[i] = data->csc_post_bv[i];
	}

	for (int  i = 0; i < HFI_CSC_CLAMP_SIZE; i++) {
		hfi_cfg.pre_clamp[i] = data->csc_pre_lv[i];
		hfi_cfg.post_bias[i] = data->csc_post_lv[i];
	}

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(ctx->prop_helper,
		prop_id, ctx->obj_id, HFI_VAL_U32_ARRAY, &hfi_cfg,
		sizeof(struct hfi_csc));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", prop_id, ret);

}

void hfi_setup_ucsc_igcv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data)
{
	struct sde_hw_cp_cfg *hw_cfg = data;
	u32 hfi_cfg = UCSC_IGC_MODE_DISABLE;
	int *ucsc_igc;
	int ret = 0;

	if (!ctx || !data || index == SDE_SSPP_RECT_MAX) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK index: %d\n",
				ctx, data, index);
		return;
	}

	hw_cfg->prop_id = HFI_PACK_VERSION(1, 1, hw_cfg->prop_id);
	ucsc_igc = (int *)(hw_cfg->payload);
	if (!ucsc_igc || (hw_cfg->len != sizeof(int))) {
		SDE_ERROR("invalid payload pipe: %d index: %d payload: %pK len: %d\n",
				ctx->idx, index, ucsc_igc, hw_cfg->len);
		return;
	}

	switch (*ucsc_igc) {
	case UCSC_IGC_MODE_SRGB:
		hfi_cfg = HFI_COLOR_LAYER_IGC_SRGB;
		break;
	case UCSC_IGC_MODE_REC709:
		hfi_cfg = HFI_COLOR_LAYER_IGC_709;
		break;
	case UCSC_IGC_MODE_GAMMA2_2:
		hfi_cfg = HFI_COLOR_LAYER_IGC_2_2;
		break;
	case UCSC_IGC_MODE_HLG:
		hfi_cfg = HFI_COLOR_LAYER_IGC_HLG;
		break;
	case UCSC_IGC_MODE_PQ:
		hfi_cfg = HFI_COLOR_LAYER_IGC_PQ;
		break;
	case UCSC_IGC_MODE_DISABLE:
		hfi_cfg = HFI_COLOR_LAYER_IGC_NONE;
		break;
	default:
		SDE_ERROR("Invalid UCSC IGC mode: %d\n", *ucsc_igc);
		return;
	}

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(hw_cfg->prop_helper, hw_cfg->prop_id,
			hw_cfg->obj_id, HFI_VAL_U32, &hfi_cfg, sizeof(u32));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", hw_cfg->prop_id, ret);
}

void hfi_setup_ucsc_gcv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data)
{
	struct sde_hw_cp_cfg *hw_cfg = data;
	u32 hfi_cfg = UCSC_GC_MODE_DISABLE;
	int *ucsc_gc;
	int ret = 0;

	if (!ctx || !data || index == SDE_SSPP_RECT_MAX) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK index: %d\n",
			ctx, data, index);
		return;
	}

	hw_cfg->prop_id = HFI_PACK_VERSION(1, 1, hw_cfg->prop_id);
	ucsc_gc = (int *)(hw_cfg->payload);
	if (!ucsc_gc || (hw_cfg->len != sizeof(int))) {
		SDE_ERROR("invalid payload pipe: %d index: %d payload: %pK len: %d\n",
			ctx->idx, index, ucsc_gc, hw_cfg->len);
		return;
	}

	switch (*ucsc_gc) {
	case UCSC_GC_MODE_SRGB:
		hfi_cfg = HFI_COLOR_LAYER_GC_SRGB;
		break;
	case UCSC_GC_MODE_PQ:
		hfi_cfg = HFI_COLOR_LAYER_GC_PQ;
		break;
	case UCSC_GC_MODE_GAMMA2_2:
		hfi_cfg = HFI_COLOR_LAYER_GC_2_2;
		break;
	case UCSC_GC_MODE_HLG:
		hfi_cfg = HFI_COLOR_LAYER_GC_HLG;
		break;
	case UCSC_GC_MODE_DISABLE:
		hfi_cfg = HFI_COLOR_LAYER_GC_NONE;
		break;
	default:
		SDE_ERROR("Invalid UCSC GC mode: %d\n", *ucsc_gc);
		return;
	}

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(hw_cfg->prop_helper, hw_cfg->prop_id,
			hw_cfg->obj_id, HFI_VAL_U32, &hfi_cfg, sizeof(u32));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", hw_cfg->prop_id, ret);
}

void hfi_setup_ucsc_cscv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data)
{
	struct sde_hw_cp_cfg *hw_cfg = data;
	struct hfi_ucsc_csc hfi_cfg;
	drm_msm_ucsc_csc *ucsc_csc;
	int ret = 0;

	if (!ctx || !data || index == SDE_SSPP_RECT_MAX) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK index: %d\n",
			ctx, data, index);
		return;
	}

	hw_cfg->prop_id = HFI_PACK_VERSION(1, 1, hw_cfg->prop_id);
	ucsc_csc = (drm_msm_ucsc_csc *)(hw_cfg->payload);
	if (!ucsc_csc) {
		hfi_cfg.flags = 0;
		goto send_payload;
	}

	if (hw_cfg->len != sizeof(drm_msm_ucsc_csc)) {
		SDE_ERROR("invalid payload length pipe: %d index: %d len: %d expected len: %lu\n",
			ctx->idx, index, hw_cfg->len, sizeof(drm_msm_ucsc_csc));
		return;
	}

	hfi_cfg.flags = HFI_BUFF_FEATURE_ENABLE;
	for (int i = 0; i < HFI_UCSC_CSC_MATRIX_COEFF_SIZE; i++)
		hfi_cfg.ctm_coeff[i] = ucsc_csc->cfg_param_0[i];

	for (int i = 0; i < HFI_UCSC_CSC_PRE_CLAMP_SIZE; i++)
		hfi_cfg.pre_clamp[i] = ucsc_csc->cfg_param_1[i];

	for (int i = 0; i < HFI_UCSC_CSC_POST_CLAMP_SIZE; i++)
		hfi_cfg.post_clamp[i] = ucsc_csc->cfg_param_1[HFI_UCSC_CSC_PRE_CLAMP_SIZE + i];

send_payload:
	ret = hfi_util_u32_prop_helper_add_prop_by_obj(hw_cfg->prop_helper, hw_cfg->prop_id,
			hw_cfg->obj_id, HFI_VAL_U32_ARRAY, &hfi_cfg, sizeof(struct hfi_ucsc_csc));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", hw_cfg->prop_id, ret);
}

void hfi_setup_ucsc_unmultv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data)
{
	u32 hfi_cfg = 0;
	struct sde_hw_cp_cfg *hw_cfg = data;
	bool *ucsc_unmult;
	int ret = 0;

	if (!ctx || !data || index == SDE_SSPP_RECT_MAX) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK index: %d\n",
			ctx, data, index);
		return;
	} else if (!hw_cfg->payload || hw_cfg->len != sizeof(bool)) {
		SDE_ERROR("invalid payload pipe: %d index: %d payload: %pK len: %d\n",
			ctx->idx, index, hw_cfg->payload, hw_cfg->len);
		return;
	}

	hw_cfg->prop_id = HFI_PACK_VERSION(1, 1, hw_cfg->prop_id);
	ucsc_unmult = (bool *)(hw_cfg->payload);
	if (ucsc_unmult && *ucsc_unmult)
		hfi_cfg = HFI_BUFF_FEATURE_ENABLE;

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(hw_cfg->prop_helper, hw_cfg->prop_id,
			hw_cfg->obj_id, HFI_VAL_U32, &hfi_cfg, sizeof(u32));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", hw_cfg->prop_id, ret);
}

void hfi_setup_ucsc_alpha_ditherv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data)
{
	u32 hfi_cfg = 0;
	struct sde_hw_cp_cfg *hw_cfg = data;
	bool *ucsc_alpha_dither;
	int ret = 0;

	if (!ctx || !data || index == SDE_SSPP_RECT_MAX) {
		SDE_ERROR("invalid parameter ctx: %pK data: %pK index: %d\n",
			ctx, data, index);
		return;
	} else if (!hw_cfg->payload || hw_cfg->len != sizeof(bool)) {
		SDE_ERROR("invalid payload pipe: %d index: %d payload: %pK len: %d\n",
			ctx->idx, index, hw_cfg->payload, hw_cfg->len);
		return;
	}

	hw_cfg->prop_id = HFI_PACK_VERSION(1, 0, hw_cfg->prop_id);
	ucsc_alpha_dither  = (bool *)(hw_cfg->payload);
	if (ucsc_alpha_dither && *ucsc_alpha_dither)
		hfi_cfg = HFI_BUFF_FEATURE_ENABLE;

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(hw_cfg->prop_helper, hw_cfg->prop_id,
			hw_cfg->obj_id, HFI_VAL_U32, &hfi_cfg, sizeof(u32));
	if (ret)
		SDE_ERROR("failed to add HFI prop: %d ret: %d\n", hw_cfg->prop_id, ret);
}

/* Helper function to send hfi_buff packet for AHB non-broadcast use-case */
static int hfi_buff_send_payload(void *cfg, void *hfi_cfg, u32 prop_id,
	u32 major_ver, u32 minor_ver)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct hfi_shared_addr_map *hfi_buff_map = NULL;
	u32 ret = 0, indx, payload_size;
	struct hfi_buff prop_hfi_buff;
	u64 fw_buff_addr;

	hfi_buff_map = hw_cfg->hfi_buff_map;
	hfi_cfg = (struct hfi_display_dither *)hfi_cfg;
	payload_size = sizeof(struct hfi_display_dither);

	if (!hfi_buff_map || !hfi_buff_map->remote_addr ||
		!hfi_buff_map->local_addr) {
		SDE_ERROR("Invalid inputs: hfi_buff_map %pK, remote_addr %lu, local_addr %pK\n",
			hfi_buff_map, (hfi_buff_map ? hfi_buff_map->remote_addr : 0),
			(hfi_buff_map ? hfi_buff_map->local_addr : NULL));
		return -EINVAL;
	}

	if (hw_cfg->dspp_idx < hw_cfg->dspp_start_idx) {
		SDE_ERROR("Invalid dspp_idx %d or dspp_start_idx %d\n", hw_cfg->dspp_idx,
				hw_cfg->dspp_start_idx);
		return -EINVAL;
	}

	indx = (hw_cfg->dspp_idx - hw_cfg->dspp_start_idx) * payload_size;
	if (indx + payload_size > hfi_buff_map->size) {
		SDE_ERROR("Not enough memory left, remaining size %u, payload_size %u\n",
			hfi_buff_map->size - indx, payload_size);
		return -EINVAL;
	}
	memcpy(hfi_buff_map->local_addr + indx, hfi_cfg, payload_size);

	/* non-broadcast and it is the last dspp idx - send the packet */
	if (hw_cfg->dspp_idx == (hw_cfg->dspp_start_idx + hw_cfg->num_of_mixers - 1)) {
		// populate hfi_buff to send over hfi packet.
		fw_buff_addr = (u64) hfi_buff_map->remote_addr;
		prop_hfi_buff.addr_l = (fw_buff_addr & 0xFFFFFFFF);
		prop_hfi_buff.addr_h = (fw_buff_addr >> 32);
		prop_hfi_buff.size = (payload_size / sizeof(u32)) * hw_cfg->num_of_mixers;

		ret = hfi_util_u32_prop_helper_add_prop(hw_cfg->prop_helper,
				HFI_PACK_VERSION(major_ver, minor_ver, prop_id),
				HFI_VAL_U32_ARRAY, &prop_hfi_buff,
				sizeof(struct hfi_buff));
		if (ret)
			SDE_ERROR("Failed to add hfi prop %d ret %d\n", prop_id, ret);
		else
			SDE_DEBUG("non-broadcast feature: submitted to prop_helper\n");
	}

	return ret;
}

void hfi_setup_dspp_pa_dither_v1_7(struct sde_hw_dspp *ctx, void *cfg)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_pa_dither *dither;
	u32 i, ret = 0;
	u32 payload_size = sizeof(struct hfi_display_pa_dither);
	u32 prop_id = HFI_PACK_VERSION(1, 7, HFI_PROPERTY_DISPLAY_COLOR_PA_DITHER);
	struct hfi_display_pa_dither *hfi_cfg = NULL;

	if (!hw_cfg || (hw_cfg->len != sizeof(struct drm_msm_pa_dither) &&
			hw_cfg->payload)) {
		SDE_ERROR("hw %pK payload %pK size %d expected sz %zd\n",
			hw_cfg, ((hw_cfg) ? hw_cfg->payload : NULL),
			((hw_cfg) ? hw_cfg->len : 0),
			sizeof(struct drm_msm_pa_dither));
		return;
	}

	if (ctx->dpu_idx < DPU_0 || ctx->dpu_idx >= DPU_MAX) {
		SDE_ERROR("Invalid dpu idx: %d\n", ctx->dpu_idx);
		return;
	}

	hfi_cfg = &hfi_pa_dither_cached[ctx->dpu_idx][hw_cfg->dspp_idx];

	if (!hw_cfg->payload) {
		/* Turn off feature */
		hfi_cfg->flags = 0;
	} else {
		/* Turn on feature */
		hfi_cfg->flags = HFI_DISPLAY_COLOR_FLAGS_FEATURE_ENABLE;
		dither = hw_cfg->payload;
		for (i = 0; i < DITHER_MATRIX_SZ; i++)
			hfi_cfg->matrix[i] = dither->matrix[i];
		hfi_cfg->strength = dither->strength;
		hfi_cfg->offset_en = dither->offset_en;
	}

	if (hw_cfg->dspp_idx == (hw_cfg->dspp_start_idx + hw_cfg->num_of_mixers - 1)) {
		/* non-broadcast and it is the last dspp idx - send the payload */
		ret = hfi_util_u32_prop_helper_add_prop(hw_cfg->prop_helper,
				prop_id, HFI_VAL_U32_ARRAY,
				&hfi_pa_dither_cached[ctx->dpu_idx][hw_cfg->dspp_start_idx],
				payload_size * hw_cfg->num_of_mixers);
		if (ret)
			SDE_ERROR("Failed to add hfi prop for PA dither %d ret %d\n",
				prop_id, ret);
		else
			SDE_DEBUG("non-broadcast feature: submitted to prop_helper\n");
		/* reset the cached struct for current dpu_idx after submitting to FW */
		memset(&hfi_pa_dither_cached[ctx->dpu_idx], 0,
			   sizeof(hfi_pa_dither_cached[ctx->dpu_idx]));
	}
}

void hfi_setup_dspp_spr_dither_v2(struct sde_hw_dspp *ctx, void *cfg)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_dither *dither;
	u32 i, ret = 0, row, col;
	struct hfi_display_dither hfi_cfg = {};

	if (!hw_cfg || (hw_cfg->len != sizeof(struct drm_msm_dither) &&
			hw_cfg->payload)) {
		DRM_ERROR("hw %pK payload %pK size %d expected sz %zd\n",
			hw_cfg, ((hw_cfg) ? hw_cfg->payload : NULL),
			((hw_cfg) ? hw_cfg->len : 0),
			sizeof(struct drm_msm_dither));
		return;
	}

	if (!hw_cfg->payload) {
		/* Turn off feature */
		DRM_DEBUG_DRIVER("Disable DSPP SPR dither feature\n");
		hfi_cfg.flags = 0;
	} else {
		/* Turn on feature */
		DRM_DEBUG_DRIVER("Enable DSPP SPR dither feature\n");
		hfi_cfg.flags = HFI_DISPLAY_COLOR_FLAGS_FEATURE_ENABLE;

		dither = hw_cfg->payload;
		hfi_cfg.feature_flags = dither->flags;
		hfi_cfg.temporal_en = dither->temporal_en;
		hfi_cfg.c0_bitdepth = dither->c0_bitdepth;
		hfi_cfg.c1_bitdepth = dither->c1_bitdepth;
		hfi_cfg.c2_bitdepth = dither->c2_bitdepth;
		hfi_cfg.c3_bitdepth = dither->c3_bitdepth;
		hfi_cfg.dither_matrix_select =
			dither->dither_matrix_select ? (dither->dither_matrix_select - 1) : 0;

		if (dither->dither_matrix_select == DITHER_MATRIX_SELECT_NONE) {
			/* Legacy Matrix */
			for (i = 0; i < HFI_DITHER_MATRIX_SZ; i++) {
				row = i / 4;
				col = i % 4;
				hfi_cfg.matrix[row * 16 + col] = dither->matrix[i];
			}
		} else {
			memcpy(hfi_cfg.matrix, dither->dither_matrix_extended,
				HFI_DITHER_MATRIX_SZ_EXTENDED * sizeof(s32));
		}
	}

	ret = hfi_buff_send_payload(cfg, &hfi_cfg,
			HFI_PROPERTY_DISPLAY_COLOR_SPR_DITHER, 2, 0);
	if (ret)
		SDE_ERROR("Failed to send hfi_buff from SPR dither ret: %d\n", ret);
}
