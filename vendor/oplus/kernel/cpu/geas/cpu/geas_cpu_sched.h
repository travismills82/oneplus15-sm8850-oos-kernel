// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _GEAS_CPU_SCHED_H
#define _GEAS_CPU_SCHED_H

#include <linux/sched.h>

#define GEAS_EAS_DEBUG

struct geas_opp_table {
	unsigned long frequency_khz;
	unsigned long volt_mv;
};

struct cpufreq_dsufreq_map {
	unsigned int cpufreq_mhz;
	unsigned int memfreq_khz;
	int freq_index;
};

struct geas_util_to_data {
	u32 ps_index : 8;
	u32 freq : 24;
	u16 target_load;
	u16 cost;
};

struct dynamic_energy_model {
	int type;
	struct geas_perf_state *states;
	u16 *freq_to_util;
	u16 *freq_to_ratio;
	u16 *freq_to_cost;
	int size;
	int cluster_id;
	unsigned long coeff_power;
	unsigned long cap_scale;
	unsigned long cap_origin;
	struct geas_util_to_data util_to_data[1024];
	struct shb_config_data **shb_config_data;
};

struct dynamic_em_manager {
	struct geas_sched_cluster_info *cluster;
	struct dynamic_energy_model * ems;
	unsigned int numb_of_em;
	int cur_enable;
	unsigned int nr_freq;
	unsigned int valid;
};

struct geas_sched_rq_info {
	int cpu;
	struct geas_sched_cluster_info * cluster;
};

struct geas_sched_cluster_info {
	struct list_head	list;
	struct cpumask		cpus;
	int		id;
	u8		cpu_num;
	u8		policy_id;	/* first cpu */
	u16		cluster_type;
	unsigned int max_possible_freq;
	unsigned int max_freq;
	unsigned int min_freq;
	unsigned int cur_freq;
	unsigned int cur_freq_index;
	unsigned int cost_scaled;
	u16 cur_cap;
	u16 scale_cpu;
	u16 scaling_min_cap;
	u16 scaling_max_cap;
	/*
	 * Currently only two clusters share bucks
	 * or little shared with dsu.
	 * type 1 share cpu cluster, type 2 cpu /dsu
	 * type 0 none.
	*/
	int shb_type;
	struct geas_sched_cluster_info *shbuck_cluster;
	int 	shb_cost_inited;
	struct dynamic_em_manager dem;
};

u64 geas_sched_ktime_get_ns(void);
struct geas_sched_cluster_info * geas_sched_get_pd_cluster_info(struct em_perf_domain *pd);
struct geas_sched_rq_info * geas_sched_rq_info_byid(int id);
int geas_sched_get_pd_fmax(struct em_perf_domain *pd);
struct geas_sched_cluster_info * geas_sched_cluster_info_byid(int id);
int geas_sched_get_numb_of_clusters(void);
struct geas_sched_cluster_info * geas_sched_cpu_cluster_info(int cpu);
int geas_sched_get_pd_first_cpu(struct em_perf_domain *pd);
u16 geas_sched_get_cpu_util_to_cost(int cpu, unsigned long util, int em_type);
struct shb_config_data *geas_sched_get_cluster_shb_config(struct geas_sched_cluster_info * cl,
	unsigned idx_x, unsigned int idx_y, int em_type);
struct shb_config_data ** geas_sched_get_cluster_shb_config_array(struct geas_sched_cluster_info * cl, int em_type);
struct geas_util_to_data * geas_sched_get_cluster_util_to_data(struct geas_sched_cluster_info * cl,
	unsigned int util, int em_type);
bool geas_sched_dynamic_energy_model_valid(struct geas_sched_cluster_info * cl);
struct geas_util_to_data * geas_sched_get_cluster_util_to_data_array(struct geas_sched_cluster_info * cl, int em_type);
u16 get_cluster_util_to_cost(struct geas_sched_cluster_info *cl, unsigned long util, int em_type);
int update_targetload_ctrl_values(const struct ctl_table *table, int write, void __user *buffer, size_t *lenp, loff_t * ppos);
int get_cpufreq_dsufreq_map(int cpu, struct cpufreq_dsufreq_map **cpufreq_dsufreq_map);
int get_geas_opp_table(int cpu, struct geas_opp_table **freq_table_inp, struct em_perf_domain *pd, u32 *coef);
#define for_each_cluster_info(cluster, idx) \
	for (; (idx) < geas_sched_get_numb_of_clusters() && ((cluster) = geas_sched_cluster_info_byid(idx));\
		idx++)

extern struct geas_sched_rq_info *geas_rq_info;
extern struct geas_sched_cluster_info *geas_sched_info[CLUSTER_NUM];
extern int geas_targetload[CLUSTER_NUM];

#endif /* _GEAS_CPU_SCHED_H */
