// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "sde_kms.h"
#include "sde_color_proc_feature_state_helper.h"

void cp_get_gamut_mode(void *src_prop, u32 *mode)
{
	*mode = atomic_read(&((struct cp_vig_gamut_mode *)src_prop)->vig_gamut_mode);
}

void cp_get_pa_mode(void *src_prop, u32 *mode)
{
	*mode = atomic_read(&((struct cp_pa_mode *)src_prop)->pa_mode);
}

typedef void (*cp_feat_state_get_func_t)(void *src_prop, u32 *mode);
static cp_feat_state_get_func_t cp_feat_state_get_funcs[CP_STATE_FEAT_GROUP_MAX] = {
	[CP_STATE_FEAT_VIG_GAMUT] = cp_get_gamut_mode,
	[CP_STATE_FEAT_PA] = cp_get_pa_mode
};

void cp_feature_get_curr_mode(enum cp_state_features feature, void *src_prop, u32 *mode)
{
	enum cp_state_feature_group feature_group;

	if (!src_prop || !mode)
		return;

	if (feature >= CP_STATE_FEATURES_MAX) {
		DRM_ERROR("Feature: %d is not supported in cp state feature", feature);
		return;
	}

	feature_group =  cp_state_feat_info[feature].feature_group;
	if (feature_group >= CP_STATE_FEAT_GROUP_MAX) {
		DRM_ERROR("Feature: %d doesn't have a mapped cp_state_feature_group\n", feature);
		return;
	}

	cp_feat_state_get_funcs[feature_group](src_prop, mode);
}

void cp_set_vig_gamut_mode(void *cp_mode, u64 prop_val, u32 feature_id)
{
	u32 mode;
	struct cp_vig_gamut_mode *cp_gamut_mode = (struct cp_vig_gamut_mode *) cp_mode;

	if (!cp_gamut_mode)
		return;

	if (prop_val) { // Enable
		mode = atomic_read(&cp_gamut_mode->vig_gamut_mode);
		if (mode == MODE_17_A)
			atomic_set(&cp_gamut_mode->vig_gamut_mode, MODE_17_B);
		else if (mode == MODE_17_B)
			atomic_set(&cp_gamut_mode->vig_gamut_mode, MODE_17_A);
	} else { // Disable
		atomic_set(&cp_gamut_mode->vig_gamut_mode, MODE_17_A);
	}

}

void cp_set_pa_mode(void *cp_mode, u64 prop_val, u32 feature_id)
{
	struct cp_pa_mode *pa_mode;

	pa_mode = (struct cp_pa_mode *) cp_mode;
	if (!pa_mode)
		return;

	if (prop_val) { // Enable
		atomic_or(feature_id, &pa_mode->pa_mode);
	} else { // Disable
		atomic_andnot(feature_id, &pa_mode->pa_mode);
	}
}

typedef void (*cp_feat_state_set_func_t)(void *cp_mode_struct, u64 prop_val, u32 feature_id);
static cp_feat_state_set_func_t cp_feat_state_set_funcs[CP_STATE_FEAT_GROUP_MAX] = {
	[CP_STATE_FEAT_VIG_GAMUT] = cp_set_vig_gamut_mode,
	[CP_STATE_FEAT_PA] = cp_set_pa_mode
};

void cp_feature_set_curr_mode(enum cp_state_features feature, void *cp_mode, u64 prop_val)
{
	enum cp_state_feature_group feature_group;

	if (!cp_mode)
		return;

	if (feature >= CP_STATE_FEATURES_MAX) {
		DRM_ERROR("Feature: %d is not supported in cp state feature", feature);
		return;
	}

	feature_group =  cp_state_feat_info[feature].feature_group;
	if (feature_group >= CP_STATE_FEAT_GROUP_MAX) {
		DRM_ERROR("Feature: %d doesn't have a mapped cp_state_feature_group\n", feature);
		return;
	}

	cp_feat_state_set_funcs[feature_group](cp_mode, prop_val, cp_state_feat_info[feature].mode);
}
