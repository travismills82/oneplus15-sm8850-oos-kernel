/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _MSM_VIDC_LEMANS_H_
#define _MSM_VIDC_LEMANS_H_

struct msm_vidc_core;

#if defined(CONFIG_MSM_VIDC_LEMANS)
int msm_vidc_get_platform_data_lemans(struct msm_vidc_core *core, struct device *dev);
int msm_vidc_init_platform_lemans(struct msm_vidc_core *core, struct device *dev);
#else
int msm_vidc_get_platform_data_lemans(struct msm_vidc_core *core, struct device *dev)
{
	return -EINVAL;
}
int msm_vidc_init_platform_lemans(struct msm_vidc_core *core, struct device *dev)
{
	return -EINVAL;
}
#endif

#endif // _MSM_VIDC_LEMANS_H_
