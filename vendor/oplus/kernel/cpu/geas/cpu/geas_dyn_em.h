// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _GEAS_DYN_EM_H
#define _GEAS_DYN_EM_H
int dynamic_em_proc_enable_handler(struct ctl_table *table,
				int write, void __user *buffer, size_t *lenp, loff_t *ppos);
int geas_dyn_em_update(int em_id, struct geas_perf_state em_para_cl[GEAS_SCHED_CLUSTER_NR][MAX_FREQ_OPP_NUM]);
void update_dyn_em_targetloads(struct geas_sched_cluster_info *info);
int geas_dyn_em_init(void);
u16 dyn_em_freq_to_ratio(unsigned long freq, u8 em_type, struct geas_sched_cluster_info *info);
u32 get_cluster_util_to_raito_by_emtype(struct geas_sched_cluster_info *cl, u8 em_type, u16 util);
int geas_sched_freq_to_util_by_emtype(struct geas_sched_cluster_info *cluster, unsigned long freq, int em_id);
u16 dyn_em_freq_to_cost(unsigned long freq, u8 em_type, struct geas_sched_cluster_info *info);
u16 geas_sched_util_to_target_load_inner(u32 freq, int cpu, int cl_id);
int update_freq_to_target(void);

extern u16 freq_to_index_max[GEAS_SCHED_CLUSTER_NR];
extern u16 *freq_to_target[GEAS_SCHED_CPU_NR];
extern struct geas_target_load geas_targetload_list[GEAS_SCHED_CPU_NR][MAX_TL_SIZE];
extern int dynamic_em_cur_enable;

#endif  /* _GEAS_DYN_EM_H */
