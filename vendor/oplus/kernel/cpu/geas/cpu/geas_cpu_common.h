// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _GEAS_CPU_COMMON_H
#define _GEAS_CPU_COMMON_H

#include <linux/sched.h>
#include "geas_cpu_external.h"

#define DEFAULT_TARGET_LOAD_MARGIN 1280
#define CLUSTER_NUM	GEAS_SCHED_CLUSTER_NR
#define MAX_CPU_NR	GEAS_SCHED_CPU_NR
#define DEFAULT_TARGET_LOAD 80
#define MIN_UTIL 10  /* when cpu util < MIN_UTIL, we not cal lkg of this cpu */

struct shb_cpu_loading {
	u16 cum_util;
	u16 temp_part_lkg;
	u8 em_type;
	u8 is_pipiline;
	u16 cost;
	u16 target_load;
	u32 freq;
	int energy;
	int lkg;
};

struct shb_cluster_loading {
	u16 sum_util;
	u16 unpip_util;
	int dyn_energy;
	int lkg_energy;
	int lkg_cpu;
	int lkg_topo;
	int lkg_sram;
	int energy;
	u32 ps_index : 8;
	u32 freq : 24;
	u16 cost;
	u16 max_util;
	u16 max_pipline_util;
	u8 unpipline_cpu_num;
	u8 pipline_cpu_num;
	u16 clam_min_util;
	u16 clam_max_util;
};

struct shb_energy_env {
	u8 dst_cpu[MAX_PIPLINE_TASK];
	u8 resv;
	struct shb_cpu_loading cpu_data[MAX_CPU_NR];
	struct shb_cluster_loading cl_data[CLUSTER_NUM];
};
struct shb_config_data {
	u16 cost_scaled;
	u16 vol_mv;
	u16 lkg_vol_part;
	u16 power;
	u16 cost;
	u16 util;	/* not used */
	u32 ps_index : 8;	/* not used */
	u32 freq : 24;	/* not used */
};

extern int geas_cpu_enable;
extern int geas_cpu_inited;
extern int geas_cpu_dbg_enabled;
extern int numb_of_clusters;
extern u8 geas_cpu_num;
#define GEAS_PRINT_ENABLE (geas_cpu_dbg_enabled & 1)
#define GEAS_TRACE_ENABLE (geas_cpu_dbg_enabled & 2)

#endif /* _GEAS_CPU_COMMON_H */
