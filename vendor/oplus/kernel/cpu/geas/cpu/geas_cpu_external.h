// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _GEAS_CPU_EXTERNAL_H
#define _GEAS_CPU_EXTERNAL_H
#define MAX_TL_SIZE 5

#define GEAS_SCHED_CPU_NR 10
#define GEAS_SCHED_CLUSTER_NR 4

#define MAX_CPU_MAP_NUM 10
#define MAX_PIPLINE_TASK 3

#define MAX_FREQ_OPP_NUM 50
#define MAX_EM_NUM 8
#define EM_TYPE_ORIGIN 0

enum em_type_cluster {
	EM_ON_BB = 0,
	EM_ON_BM,
	EM_ON_MB,
	EM_ON_MM,
	EM_TYPE_CLUSTER_BUTTON,
};

enum feq_policy_mode {
	FREQ_POLICY_MAX = 0,
	FREQ_POLICY_MEAN,
	FREQ_POLICY_MEDIAN,
	FREQ_POLICY_BUTTON,
};

struct mask {
	u8 ec;
	u8 pf;
	u8 pt;
	u8 ex;
};

struct multitask_load_out {
	u32 freq_cal[GEAS_SCHED_CLUSTER_NR];  /* eas suggest next freq */
	u8 pipline;  /* valid order in pipline_cpu_map_info */
	u8 pipline_cpu[MAX_PIPLINE_TASK]; /* task order is same with multitask_ofb_thread_info->tid */
};

struct multitask_load_info {
	/* intput */
	struct mask cpu_mask;  /* 36~0: little big partial exclusive ex_free  */
	u32 pre_freq[GEAS_SCHED_CPU_NR];   /* KHZ  default 3 window, window num can config */
	u32 pre_cpu_load[GEAS_SCHED_CPU_NR];  /* util 0~1024 default 3 window, window num can config */
	u8 prev_cpu[MAX_PIPLINE_TASK]; /* task order is same with multitask_ofb_thread_info->tid */
	u8 last_pipline;
	u16 task_util[MAX_PIPLINE_TASK];   /* util 0~1024 task order is same with multitask_ofb_thread_info->tid  3 window, window num can config */
	u16 task_clamp_min[MAX_PIPLINE_TASK];   /* util 0~1024 gpa before target_load util */
	u16 task_clamp_max[MAX_PIPLINE_TASK];   /* util 0~1024 gpa before target_load util */
	u16 resv1;
	u32 soft_freq_max[GEAS_SCHED_CLUSTER_NR];
	u32 soft_freq_min[GEAS_SCHED_CLUSTER_NR];
	u64 resv[2];
	/* out */
	struct multitask_load_out out;
};

struct geas_target_load {
	u16 util;
	u16 val;
	u16 size;
};

struct geas_perf_state {
	u32 frequency;
	u16 util;
	u16 vol_mv;
	u16 power;
	u16 util_ratio;  /* emx_util * 1024/em0_util */
};

struct geas_static_pwr {
	int freq;
	int lkg_cpu;
	int lkg_topo;
	int lkg_sram;
};

struct pipline_cpu_map_info {
	u8 map_num;
	u8 mem_ratio[MAX_CPU_MAP_NUM];
	u8 cpu_map[MAX_CPU_MAP_NUM][MAX_PIPLINE_TASK];
};

struct multitask_ofb_base_info {
	pid_t tid[MAX_PIPLINE_TASK];
	u8 exclusive_mask;
	/* 0~3bit :EM_ON_BB 4~7bit:EM_ON_BM 8~11bit:EM_ON_MB 12~15bit EM_ON_MM
	eg: 8750 EM_ON_BM means tid[0] on p core, tid[1] on e core */
	u16 em_type[MAX_PIPLINE_TASK];
	unsigned int thread_num;
	int em_mask;  /* valid em mask */
	struct geas_perf_state em_para[MAX_EM_NUM][GEAS_SCHED_CLUSTER_NR][MAX_FREQ_OPP_NUM];
	struct pipline_cpu_map_info pipline_cpu_map;
	struct geas_target_load targetload[GEAS_SCHED_CPU_NR][MAX_TL_SIZE];   /*  GPA target load */
	u8 dyn_enable;
	u8 def_em_type;
	u8 pipline_cal_mask;
	u8 ctr_mode[GEAS_SCHED_CLUSTER_NR];  /* FREQ_POLICY_BUTTON */
	u64 resv[2];
};

#define EM_TYPE_WIDTH 4
static inline u8 get_em_type(u16 em_type, enum em_type_cluster type)
{
	u8 val;
	val = em_type >> (type * EM_TYPE_WIDTH);
	val = val & ((1 << EM_TYPE_WIDTH) - 1);
	val = min(val, MAX_EM_NUM - 1);

	return val;
}

int geas_update_load_info(struct multitask_load_info *load_info);
int geas_update_ofb_base_info(struct multitask_ofb_base_info *base_info);

#endif /* _GEAS_CPU_EXTERNAL_H */
