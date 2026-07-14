/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_APS_GENL_H_
#define _RMNET_APS_GENL_H_

#include <net/genetlink.h>
#include <uapi/linux/rmnet_aps.h>

/* Make sure to change this if you EVER add a new attribute in UAPI */
#define RMNET_APS_GENL_ATTR_MAX 	RMNET_APS_GENL_ATTR_DATA_REPORT

int rmnet_aps_genl_init(void);

void rmnet_aps_genl_deinit(void);

#endif /*_RMNET_APS_GENL_H_*/
