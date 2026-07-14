/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _MSM_VIDC_IRIS36_H_
#define _MSM_VIDC_IRIS36_H_

struct msm_vidc_core;
struct v4l2_ctrl;

#if defined(CONFIG_MSM_VIDC_NORDAU)
int msm_vidc_init_iris36(struct msm_vidc_core *core);
int msm_vidc_adjust_bitrate_boost_iris36(void *instance, struct v4l2_ctrl *ctrl);
#else
static inline int msm_vidc_init_iris36(struct msm_vidc_core *core)
{
	return -EINVAL;
}

static inline int msm_vidc_adjust_bitrate_boost_iris36(void *instance, struct v4l2_ctrl *ctrl)
{
	return -EINVAL;
}
#endif

#endif // _MSM_VIDC_IRIS36_H_
