/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __H_MSM_VIDC_POWER_IRIS36_H__
#define __H_MSM_VIDC_POWER_IRIS36_H__

struct msm_vidc_inst;
struct vidc_bus_vote_data;

#define ENABLE_LEGACY_POWER_CALCULATIONS  0

int msm_vidc_ring_buf_count_iris36(struct msm_vidc_inst *inst, u32 data_size);
u64 msm_vidc_calc_freq_iris36(struct msm_vidc_inst *inst, u32 data_size);
int msm_vidc_calc_bw_iris36(struct msm_vidc_inst *inst,
		struct vidc_bus_vote_data *vote_data);

#endif
