/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _QCOM_SDEI_H
#define _QCOM_SDEI_H

#if IS_ENABLED(CONFIG_QCOM_SDEI)
int qcom_sdei_shared_reset(void);
#else
static inline int qcom_sdei_shared_reset(void)
{
	return 0;
}
#endif

#endif