// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _GEAS_CPU_PARA_H
#define _GEAS_CPU_PARA_H

extern struct geas_perf_state em_para[MAX_EM_NUM][GEAS_SCHED_CLUSTER_NR][MAX_FREQ_OPP_NUM];
extern struct pipline_cpu_map_info pipline_cpu_map[MAX_PIPLINE_TASK];
extern struct cpufreq_dsufreq_map cpu0freq_dsufreq_map[12];
extern struct geas_static_pwr static_pwr[GEAS_SCHED_CLUSTER_NR][MAX_FREQ_OPP_NUM];
#endif /* _GEAS_CPU_PARA_H */
