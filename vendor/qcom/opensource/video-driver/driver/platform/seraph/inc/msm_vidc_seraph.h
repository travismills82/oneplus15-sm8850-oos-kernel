/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _MSM_VIDC_SERAPH_H_
#define _MSM_VIDC_SERAPH_H_

struct msm_vidc_core;

#if defined(CONFIG_MSM_VIDC_SERAPH)
int msm_vidc_get_platform_data_seraph(struct msm_vidc_core *core);
int msm_vidc_init_platform_seraph(struct msm_vidc_core *core);
#else
int msm_vidc_get_platform_data_seraph(struct msm_vidc_core *core)
{
	return -EINVAL;
}
int msm_vidc_init_platform_seraph(struct msm_vidc_core *core)
{
	return -EINVAL;
}
#endif

#endif // _MSM_VIDC_SERAPH_H_
