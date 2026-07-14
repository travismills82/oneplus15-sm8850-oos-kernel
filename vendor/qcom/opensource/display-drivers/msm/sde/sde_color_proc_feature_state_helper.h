/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _SDE_COLOR_PROC_FEATURE_STATE_HELPER_H
#define _SDE_COLOR_PROC_FEATURE_STATE_HELPER_H

enum vig_gamut_mode {
	MODE_17_A = 0,
	MODE_17_B,
};

struct cp_vig_gamut_mode {
	atomic_t vig_gamut_mode;
};

#define PA_HSIC (1 << 0)
#define PA_SIXZONE (1 << 1)
#define PA_MEMC_SKY (1 << 2)
#define PA_MEMC_SKIN (1 << 3)
#define PA_MEMC_FOLIAGE (1 << 4)
#define PA_MEMC_PROT (1 << 5)

struct cp_pa_mode {
	atomic_t pa_mode;
};

enum cp_state_features {
	CP_STATE_VIG_GAMUT = 0x0,
	CP_STATE_PA_HSIC,
	CP_STATE_PA_SIXZONE,
	CP_STATE_PA_MEMC_SKY,
	CP_STATE_PA_MEMC_SKIN,
	CP_STATE_PA_MEMC_FOLIAGE,
	CP_STATE_PA_MEMC_PROT,
	CP_STATE_FEATURES_MAX,
};

enum cp_state_feature_group {
	CP_STATE_FEAT_VIG_GAMUT = 0x0,
	CP_STATE_FEAT_PA,
	CP_STATE_FEAT_GROUP_MAX,
};

struct cp_state_feature_info {
	enum cp_state_features feature;
	enum cp_state_feature_group feature_group;
	u32 mode;
};

static struct cp_state_feature_info cp_state_feat_info[CP_STATE_FEATURES_MAX] = {
	[CP_STATE_VIG_GAMUT] = {CP_STATE_VIG_GAMUT, CP_STATE_FEAT_VIG_GAMUT, 0},
	[CP_STATE_PA_HSIC] = {CP_STATE_PA_HSIC, CP_STATE_FEAT_PA, PA_HSIC},
	[CP_STATE_PA_SIXZONE] = {CP_STATE_PA_SIXZONE, CP_STATE_FEAT_PA, PA_SIXZONE},
	[CP_STATE_PA_MEMC_SKY] = {CP_STATE_PA_MEMC_SKY, CP_STATE_FEAT_PA, PA_MEMC_SKY},
	[CP_STATE_PA_MEMC_SKIN] = {CP_STATE_PA_MEMC_SKIN, CP_STATE_FEAT_PA, PA_MEMC_SKIN},
	[CP_STATE_PA_MEMC_FOLIAGE] = {CP_STATE_PA_MEMC_FOLIAGE,
				CP_STATE_FEAT_PA, PA_MEMC_FOLIAGE},
	[CP_STATE_PA_MEMC_PROT] = {CP_STATE_PA_MEMC_PROT, CP_STATE_FEAT_PA, PA_MEMC_PROT},
};

void cp_feature_get_curr_mode(enum cp_state_features feature, void *src_prop, u32 *mode);
void cp_feature_set_curr_mode(enum cp_state_features feature, void *cp_mode, u64 prop_val);

#endif /* _SDE_COLOR_PROC_FEATURE_STATE_HELPER_H */
