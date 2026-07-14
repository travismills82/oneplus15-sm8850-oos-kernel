/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __H_MSM_VIDC_POWER_IRIS3_5_H__
#define __H_MSM_VIDC_POWER_IRIS3_5_H__

struct msm_vidc_inst;
struct vidc_bus_vote_data;
struct vidc_clock_scaling_data;

#define ENABLE_LEGACY_POWER_CALCULATIONS  0

int msm_vidc_ring_buf_count_iris35(struct msm_vidc_inst *inst, u32 data_size);
int msm_vidc_scale_clocks_iris35(struct msm_vidc_inst *inst);
int msm_vidc_calc_bw_iris35(struct msm_vidc_inst *inst,
		struct vidc_bus_vote_data *vote_data);

#endif
