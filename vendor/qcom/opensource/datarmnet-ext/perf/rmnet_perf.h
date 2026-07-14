/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __RMNET_PERF_H__
#define __RMNET_PERF_H__

#include <linux/xarray.h>
#include <net/inet_ecn.h>

struct rmnet_perf_ecn_node {
	struct rcu_head rcu;
	u32 prob;
	u32 count;
	u32 drops;
	bool should_drop;
};

struct xarray *rmnet_perf_get_ecn_map(void);

#endif
