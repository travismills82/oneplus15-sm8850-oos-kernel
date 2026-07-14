/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _MSM_VIDC_MD_H_
#define _MSM_VIDC_MD_H_

#ifdef CONFIG_MSM_VIDC_MINIDUMP
const struct msm_vidc_md_ops *get_md_ops(void);
#else
static inline const struct msm_vidc_md_ops *get_md_ops(void)
{
	return NULL;
}
#endif

#endif
