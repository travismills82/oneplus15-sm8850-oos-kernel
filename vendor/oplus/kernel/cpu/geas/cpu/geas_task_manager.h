// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _GEAS_TASK_MANAGER_H
#define _GEAS_TASK_MANAGER_H

#include <linux/sched.h>

enum GEAS_EM_TYPE {
	GEAS_EM_BB,
	GEAS_EM_BM,
	GEAS_EM_MB,
	GEAS_EM_MM,
	GEAS_EM_BUTTON,
};

#define GEAS_EM_WIDTH 4

struct geas_task_struct {
	u8 exclusive_flag;
	u8 resv;
	u16 em_type;	/* 0~3bit :EM_ON_2B 4~7bit:EM_ON_1B 8~11bit:EM_ON_1M 12~15bit EM_ON_2M */
	int pid;
	char comm[TASK_COMM_LEN];
};

struct multitask_key_thread_info {
	int thread_num;
	u16 def_em_type;
	u8 pipline_cal_mask;
	u8 freq_policy_type;
	struct geas_task_struct task_list[MAX_PIPLINE_TASK];
};

struct multitask_energy_env {
	struct shb_energy_env shb_env;
	cpumask_t exclusive;
	cpumask_t online;
	int task_num;
	int last_pipline;
	u8 unpipline_cpu_num[GEAS_SCHED_CLUSTER_NR];	/* cpu_util > MIN_UTIL can cal lkg */
	u16 def_em_type;
	u16 em_type[MAX_PIPLINE_TASK];
	u32 pre_freq[GEAS_SCHED_CPU_NR];
	u32 pre_cpu_load[GEAS_SCHED_CPU_NR];
	u32 cpu_load_def_em[GEAS_SCHED_CPU_NR];
	u16 cap_origin[GEAS_SCHED_CPU_NR];
	u8 prev_cpu[MAX_PIPLINE_TASK];
	u16 util_em[MAX_PIPLINE_TASK][EM_TYPE_CLUSTER_BUTTON];
	u32 soft_freq_max[GEAS_SCHED_CLUSTER_NR];
	u32 soft_freq_min[GEAS_SCHED_CLUSTER_NR];
	u16 task_clamp_min[MAX_PIPLINE_TASK][GEAS_SCHED_CLUSTER_NR];	/* util 0~1024 gpa befor target_load */
	u16 task_clamp_max[MAX_PIPLINE_TASK][GEAS_SCHED_CLUSTER_NR];	/* util 0~1024 gpa befor target_load */
};

struct multitask_result {
	u8 thread_num;
	u8 resv;
	pid_t tid[MAX_PIPLINE_TASK];
	u16 util[MAX_PIPLINE_TASK];
	int dst_cpu[MAX_PIPLINE_TASK];
	int freq_pip[MAX_CPU_MAP_NUM][GEAS_SCHED_CLUSTER_NR];
};

extern unsigned int multitask_pid_list[];
extern unsigned int multitask_inner_triger_same_count;
extern struct multitask_key_thread_info key_thread_info;

int multitask_eas_fp_dump(const struct ctl_table *ro_table, int write, void *buffer,  unsigned long *lenp, long long *ppos);
int task_manager_init(void);
int update_multitask_ctrl_values(const struct ctl_table *table, int write,
	void __user *buffer,  unsigned long *lenp, long long *ppos);

int geas_add_task_to_group(int tid, u8 exclusive, u16 em_type);

#endif /* _GEAS_TASK_MANAGER_H */
