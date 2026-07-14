// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM geas_cpu

#if !defined(_TRACE_GEAS_CPU_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_GEAS_CPU_H

#include <linux/tracepoint.h>
#include <linux/cpumask.h>
#include "geas_cpu_sched.h"
#include "geas_task_manager.h"

TRACE_EVENT(sched_shb_compute_energy_cpu,

	TP_PROTO(int cpu, u8 em_type, u32 cost, u16 util, long dyn, long lkg, u32 freq),

	TP_ARGS(cpu, em_type, cost, util, dyn, lkg, freq),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(u8, em_type)
		__field(u32, cost)
		__field(u16, util)
		__field(long, dyn)
		__field(long, lkg)
		__field(u32, freq)),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->em_type = em_type;
		__entry->cost = cost;
		__entry->util = util;
		__entry->cost = cost;
		__entry->dyn = dyn;
		__entry->lkg = lkg;
		__entry->freq = freq;),

	TP_printk(" cpu %d em_type %u cost %u util %u dyn %lu lkg %lu freq %u",
			__entry->cpu, __entry->em_type, __entry->cost, __entry->util, __entry->dyn,
			__entry->lkg, __entry->freq)
)

TRACE_EVENT(pipline_find_efficient_cpu,
	TP_PROTO(int ret_pipline, int best_pipline, long best_energy,
			int last_pipline, long last_energy, long *energy, long *energy_mem, u64 delta_ns),

	TP_ARGS(ret_pipline, best_pipline, best_energy,
		last_pipline, last_energy, energy, energy_mem, delta_ns),

	TP_STRUCT__entry(
		__field(int, ret_pipline)
		__field(int, best_pipline)
		__field(long, best_energy)
		__field(int, last_pipline)
		__field(long, last_energy)
		__array(long, energy, MAX_CPU_MAP_NUM)
		__array(long, energy_mem, MAX_CPU_MAP_NUM)
		__field(u64, delta_ns)),

	TP_fast_assign(
		__entry->ret_pipline = ret_pipline;
		__entry->best_pipline = best_pipline;
		__entry->best_energy = best_energy;
		__entry->last_pipline = last_pipline;
		__entry->last_energy = last_energy;
		memcpy(__entry->energy, energy, MAX_CPU_MAP_NUM * sizeof(long));
		memcpy(__entry->energy_mem, energy_mem, MAX_CPU_MAP_NUM * sizeof(long));
		__entry->delta_ns = delta_ns;),

	TP_printk("ret_pipline %d best_pipline %d best_energy %ld last_pipline %d last_energy %ld "
			"energy: %ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld "
			"energy_mem: %ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld "
			"delta_ns %llu ",
			__entry->ret_pipline, __entry->best_pipline, __entry->best_energy, __entry->last_pipline, __entry->last_energy,
			__entry->energy[0], __entry->energy[1], __entry->energy[2], __entry->energy[3], __entry->energy[4],
			__entry->energy[5], __entry->energy[6], __entry->energy[7], __entry->energy[8], __entry->energy[9],
			__entry->energy_mem[0], __entry->energy_mem[1], __entry->energy_mem[2], __entry->energy_mem[3], __entry->energy_mem[4],
			__entry->energy_mem[5], __entry->energy_mem[6], __entry->energy_mem[7], __entry->energy_mem[8], __entry->energy_mem[9],
			__entry->delta_ns)
);

TRACE_EVENT(sched_multitask_energy_env,

	TP_PROTO(struct shb_energy_env *data, struct multitask_energy_env *multitask_env, int ret, u64 delta_ns),

	TP_ARGS(data, multitask_env, ret, delta_ns),

	TP_STRUCT__entry(
		__field(u8, dst_cpu0)
		__field(u8, dst_cpu1)
		__field(int, ret)
		__field(u64, delta_ns)
		__array(u8, unpipline_cpu_num, CLUSTER_NUM)
		__array(u8, pipline_cpu_num, CLUSTER_NUM)
		__array(u16, sum_util, CLUSTER_NUM)
		__array(u16, unpip_util, CLUSTER_NUM)
		__array(u16, max_util, CLUSTER_NUM)
		__array(u16, max_pipline_util, CLUSTER_NUM)
		__array(u32, cl_freq, CLUSTER_NUM)
		__array(u8, em_type, MAX_CPU_NR)
		__array(u8, is_pipiline, MAX_CPU_NR)
		__array(u32, freq, MAX_CPU_NR)
		__array(u16, target_load, MAX_CPU_NR)
		__array(u32, min_freq, CLUSTER_NUM)
		__array(u32, max_freq, CLUSTER_NUM)
		__field(unsigned long , cpu0cum)
		__field(unsigned long , cpu1cum)
		__field(unsigned long , cpu2cum)
		__field(unsigned long , cpu3cum)
		__field(unsigned long , cpu4cum)
		__field(unsigned long , cpu5cum)
		__field(unsigned long , cpu6cum)
		__field(unsigned long , cpu7cum)),

	TP_fast_assign(
		__entry->dst_cpu0 = data->dst_cpu[0];
		__entry->dst_cpu1 = data->dst_cpu[1];
		__entry->ret = ret;
		__entry->unpipline_cpu_num[0] = data->cl_data[0].unpipline_cpu_num;
		__entry->unpipline_cpu_num[1] = data->cl_data[1].unpipline_cpu_num;
		__entry->unpipline_cpu_num[2] = data->cl_data[2].unpipline_cpu_num;
		__entry->unpipline_cpu_num[3] = data->cl_data[3].unpipline_cpu_num;
		__entry->pipline_cpu_num[0] = data->cl_data[0].pipline_cpu_num;
		__entry->pipline_cpu_num[1] = data->cl_data[1].pipline_cpu_num;
		__entry->pipline_cpu_num[2] = data->cl_data[2].pipline_cpu_num;
		__entry->pipline_cpu_num[3] = data->cl_data[3].pipline_cpu_num;
		__entry->sum_util[0] = data->cl_data[0].sum_util;
		__entry->sum_util[1] = data->cl_data[1].sum_util;
		__entry->sum_util[2] = data->cl_data[2].sum_util;
		__entry->sum_util[3] = data->cl_data[3].sum_util;
		__entry->unpip_util[0] = data->cl_data[0].unpip_util;
		__entry->unpip_util[1] = data->cl_data[1].unpip_util;
		__entry->unpip_util[2] = data->cl_data[2].unpip_util;
		__entry->unpip_util[3] = data->cl_data[3].unpip_util;
		__entry->max_util[0] = data->cl_data[0].max_util;
		__entry->max_util[1] = data->cl_data[1].max_util;
		__entry->max_util[2] = data->cl_data[2].max_util;
		__entry->max_util[3] = data->cl_data[3].max_util;
		__entry->max_pipline_util[0] = data->cl_data[0].max_pipline_util;
		__entry->max_pipline_util[1] = data->cl_data[1].max_pipline_util;
		__entry->max_pipline_util[2] = data->cl_data[2].max_pipline_util;
		__entry->max_pipline_util[3] = data->cl_data[3].max_pipline_util;
		__entry->cl_freq[0] = data->cl_data[0].freq;
		__entry->cl_freq[1] = data->cl_data[1].freq;
		__entry->delta_ns = delta_ns;
		__entry->em_type[0] = data->cpu_data[0].em_type;
		__entry->em_type[1] = data->cpu_data[1].em_type;
		__entry->em_type[2] = data->cpu_data[2].em_type;
		__entry->em_type[3] = data->cpu_data[3].em_type;
		__entry->em_type[4] = data->cpu_data[4].em_type;
		__entry->em_type[5] = data->cpu_data[5].em_type;
		__entry->em_type[6] = data->cpu_data[6].em_type;
		__entry->em_type[7] = data->cpu_data[7].em_type;

		__entry->is_pipiline[0] = data->cpu_data[0].is_pipiline;
		__entry->is_pipiline[1] = data->cpu_data[1].is_pipiline;
		__entry->is_pipiline[2] = data->cpu_data[2].is_pipiline;
		__entry->is_pipiline[3] = data->cpu_data[3].is_pipiline;
		__entry->is_pipiline[4] = data->cpu_data[4].is_pipiline;
		__entry->is_pipiline[5] = data->cpu_data[5].is_pipiline;
		__entry->is_pipiline[6] = data->cpu_data[6].is_pipiline;
		__entry->is_pipiline[7] = data->cpu_data[7].is_pipiline;

		__entry->freq[0] = data->cpu_data[0].freq;
		__entry->freq[1] = data->cpu_data[1].freq;
		__entry->freq[2] = data->cpu_data[2].freq;
		__entry->freq[3] = data->cpu_data[3].freq;
		__entry->freq[4] = data->cpu_data[4].freq;
		__entry->freq[5] = data->cpu_data[5].freq;
		__entry->freq[6] = data->cpu_data[6].freq;
		__entry->freq[7] = data->cpu_data[7].freq;

		__entry->target_load[0] = data->cpu_data[0].target_load;
		__entry->target_load[1] = data->cpu_data[1].target_load;
		__entry->target_load[2] = data->cpu_data[2].target_load;
		__entry->target_load[3] = data->cpu_data[3].target_load;
		__entry->target_load[4] = data->cpu_data[4].target_load;
		__entry->target_load[5] = data->cpu_data[5].target_load;
		__entry->target_load[6] = data->cpu_data[6].target_load;
		__entry->target_load[7] = data->cpu_data[7].target_load;

		__entry->min_freq[0] = multitask_env->soft_freq_min[0];
		__entry->min_freq[1] = multitask_env->soft_freq_min[1];
		__entry->min_freq[2] = multitask_env->soft_freq_min[2];

		__entry->max_freq[0] = multitask_env->soft_freq_max[0];
		__entry->max_freq[1] = multitask_env->soft_freq_max[1];
		__entry->max_freq[2] = multitask_env->soft_freq_max[2];

		__entry->cpu0cum = data->cpu_data[0].cum_util;
		__entry->cpu1cum = data->cpu_data[1].cum_util;
		__entry->cpu2cum = data->cpu_data[2].cum_util;
		__entry->cpu3cum = data->cpu_data[3].cum_util;
		__entry->cpu4cum = data->cpu_data[4].cum_util;
		__entry->cpu5cum = data->cpu_data[5].cum_util;
		__entry->cpu6cum = data->cpu_data[6].cum_util;
		__entry->cpu7cum = data->cpu_data[7].cum_util;),

	TP_printk("ret %d dst %u/%u, unpip_num  %u/%u/%u/%u pip_num %u/%u/%u/%u "
			"cpufreq= %u/%u/%u/%u/%u/%u/%u/%u "
			"clfreq %u/%u "
			"cpucum="
			"%lu/%lu/%lu/%lu/%lu/%lu/%lu/%lu "
			"em_type=%x/%x/%x/%x/%x/%x/%x/%x "
			"is_pip=%x/%x/%x/%x/%x/%x/%x/%x "
			"targetload= %u/%u/%u/%u/%u/%u/%u/%u "
			"sum/max/unpip/max_pip cl0 %u/%u/%u/%u "
			"cl1 %u/%u/%u/%u  cl2 %u/%u/%u/%u "
			"cl_clamp min/max freq %u/%u %u/%u %u/%u "
			"delta_ns=%llu,"
			,
		__entry->ret, __entry->dst_cpu0, __entry->dst_cpu1,
		__entry->unpipline_cpu_num[0], __entry->unpipline_cpu_num[1],
		__entry->unpipline_cpu_num[2], __entry->unpipline_cpu_num[3],
		__entry->pipline_cpu_num[0], __entry->pipline_cpu_num[1],
		__entry->pipline_cpu_num[2], __entry->pipline_cpu_num[3],
		__entry->freq[0], __entry->freq[1], __entry->freq[2], __entry->freq[3],
		__entry->freq[4], __entry->freq[5], __entry->freq[6], __entry->freq[7],
		__entry->cl_freq[0], __entry->cl_freq[1],
		__entry->cpu0cum, __entry->cpu1cum, __entry->cpu2cum, __entry->cpu3cum,
		__entry->cpu4cum, __entry->cpu5cum, __entry->cpu6cum, __entry->cpu7cum,
		__entry->em_type[0], __entry->em_type[1], __entry->em_type[2], __entry->em_type[3],
		__entry->em_type[4], __entry->em_type[5], __entry->em_type[6], __entry->em_type[7],
		__entry->is_pipiline[0], __entry->is_pipiline[1], __entry->is_pipiline[2], __entry->is_pipiline[3],
		__entry->is_pipiline[4], __entry->is_pipiline[5], __entry->is_pipiline[6], __entry->is_pipiline[7],
		__entry->target_load[0], __entry->target_load[1], __entry->target_load[2], __entry->target_load[3],
		__entry->target_load[4], __entry->target_load[5], __entry->target_load[6], __entry->target_load[7],
		__entry->sum_util[0], __entry->max_util[0], __entry->unpip_util[0], __entry->max_pipline_util[0],
		__entry->sum_util[1], __entry->max_util[1], __entry->unpip_util[1], __entry->max_pipline_util[1],
		__entry->sum_util[2], __entry->max_util[2], __entry->unpip_util[2], __entry->max_pipline_util[2],
		__entry->min_freq[0], __entry->max_freq[0], __entry->min_freq[1], __entry->max_freq[1],
		__entry->min_freq[2], __entry->max_freq[2],
		__entry->delta_ns)
);

TRACE_EVENT(sched_shb_compute_energy_cl,

	TP_PROTO(struct shb_energy_env *env, long energy_sum, u64 delta_ns),

	TP_ARGS(env, energy_sum, delta_ns),

	TP_STRUCT__entry(
		__field(long, energy_sum)
		__array(int, energy, CLUSTER_NUM)
		__array(int, dyn, CLUSTER_NUM)
		__array(int, lkg, CLUSTER_NUM)
		__array(int, lkg_cpu, CLUSTER_NUM)
		__array(int, lkg_topo, CLUSTER_NUM)
		__array(int, lkg_sram, CLUSTER_NUM)
		__field(u64, delta_ns)),

	TP_fast_assign(
		__entry->delta_ns = delta_ns;
		__entry->energy_sum = energy_sum;
		__entry->energy[0] = env->cl_data[0].energy;
		__entry->energy[1] = env->cl_data[1].energy;
		__entry->energy[2] = env->cl_data[2].energy;
		__entry->dyn[0] = env->cl_data[0].dyn_energy;
		__entry->dyn[1] = env->cl_data[1].dyn_energy;
		__entry->dyn[2] = env->cl_data[2].dyn_energy;
		__entry->lkg[0] = env->cl_data[0].lkg_energy;
		__entry->lkg[1] = env->cl_data[1].lkg_energy;
		__entry->lkg[2] = env->cl_data[2].lkg_energy;
		__entry->lkg_cpu[0] = env->cl_data[0].lkg_cpu;
		__entry->lkg_cpu[1] = env->cl_data[1].lkg_cpu;
		__entry->lkg_cpu[2] = env->cl_data[2].lkg_cpu;
		__entry->lkg_topo[0] = env->cl_data[0].lkg_topo;
		__entry->lkg_topo[1] = env->cl_data[1].lkg_topo;
		__entry->lkg_topo[2] = env->cl_data[2].lkg_topo;
		__entry->lkg_sram[0] = env->cl_data[0].lkg_sram;
		__entry->lkg_sram[1] = env->cl_data[1].lkg_sram;
		__entry->lkg_sram[2] = env->cl_data[2].lkg_sram;),

	TP_printk("delta_ns %llu energy_sum %lu  energy/dyn/lkg/cpu/topo/lkg cl0 %d/%d/%d/%d/%d/%d  cl1 %d/%d/%d/%d/%d/%d %d/%d/%d/%d/%d/%d",
			__entry->delta_ns, __entry->energy_sum,
			__entry->energy[0], __entry->dyn[0], __entry->lkg[0], __entry->lkg_cpu[0], __entry->lkg_topo[0], __entry->lkg_sram[0],
			__entry->energy[1], __entry->dyn[1], __entry->lkg[1], __entry->lkg_cpu[1], __entry->lkg_topo[1], __entry->lkg_sram[1],
			__entry->energy[2], __entry->dyn[2], __entry->lkg[2], __entry->lkg_cpu[2], __entry->lkg_topo[2], __entry->lkg_sram[2])
);

TRACE_EVENT(geas_task_util_translate,

	TP_PROTO(struct geas_task_struct *p, u8 em_type, int task_num, int cl_id,
		u16 ratio_cross_cluster, u16 ratio_same_cluster, u32 freq_curr_cl, int util_em_same_clu, int util_em_cross_clu, u32 util_base),

	TP_ARGS(p, em_type, task_num, cl_id, ratio_cross_cluster, ratio_same_cluster, freq_curr_cl, util_em_same_clu, util_em_cross_clu, util_base),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(u8, em_type)
		__field(int, task_num)
		__field(int, cl_id)
		__field(u16, ratio_cross_cluster)
		__field(u16, ratio_same_cluster)
		__field(u32, freq_curr_cl)
		__field(u32, util_base)
		__field(int, util_em_same_clu)
		__field(int, util_em_cross_clu)),

	TP_fast_assign(
		memcpy(__entry->comm, p != NULL ? p->comm : "null", TASK_COMM_LEN);
		__entry->pid = p != NULL ? p->pid : 0;
		__entry->em_type = em_type;
		__entry->task_num = task_num;
		__entry->cl_id = cl_id;
		__entry->ratio_cross_cluster = ratio_cross_cluster;
		__entry->ratio_same_cluster = ratio_same_cluster;
		__entry->freq_curr_cl = freq_curr_cl;
		__entry->util_em_same_clu = util_em_same_clu;
		__entry->util_em_cross_clu = util_em_cross_clu;
		__entry->util_base = util_base;),

	TP_printk("pid=%d comm=%s em_type=%u task_num %d cl_id %d "
			"ratio_cross %u ratio_same %u freq_curr_cl %u util_em_same_clu %d util_cross_clu %d util_base %d",
			__entry->pid, __entry->comm, __entry->em_type, __entry->task_num, __entry->cl_id,
			__entry->ratio_cross_cluster, __entry->ratio_same_cluster, __entry->freq_curr_cl,
			__entry->util_em_same_clu, __entry->util_em_cross_clu, __entry->util_base)
);

TRACE_EVENT(geas_cpu_update_base_info,

	TP_PROTO(int ret, struct multitask_energy_env *multitask_env, u64 delta_ns),

	TP_ARGS(ret, multitask_env, delta_ns),

	TP_STRUCT__entry(
		__field(int, ret)
		__field(int, task_num)
		__field(u16, def_em_type)
		__field(u64, delta_ns)
		__field(int, last_pipline)
		__field(unsigned long, exclusive)
		__field(unsigned long, online)
		__array(u32, pre_freq, GEAS_SCHED_CPU_NR)
		__array(u32, pre_cpu_load, GEAS_SCHED_CPU_NR)
		__array(u32, cpu_load_def_em, GEAS_SCHED_CPU_NR)
		__array(u8, prev_cpu, MAX_PIPLINE_TASK)
		__array(u16, util_em, MAX_PIPLINE_TASK * CLUSTER_NUM)
		__array(u16, task_clamp_min, MAX_PIPLINE_TASK * CLUSTER_NUM)
		__array(u16, task_clamp_max, MAX_PIPLINE_TASK * CLUSTER_NUM)
		__array(u32, soft_freq_max, CLUSTER_NUM)
		__array(u32, soft_freq_min, CLUSTER_NUM)),

	TP_fast_assign(
		__entry->ret = ret;
		__entry->delta_ns = delta_ns;
		__entry->task_num = multitask_env->task_num;
		__entry->def_em_type = multitask_env->def_em_type;
		__entry->last_pipline = multitask_env->last_pipline;
		__entry->exclusive = cpumask_bits(&(multitask_env->exclusive))[0];
		__entry->online = cpumask_bits(&(multitask_env->online))[0];
		__entry->task_num = multitask_env->task_num;
		memcpy(__entry->pre_freq, multitask_env->pre_freq, GEAS_SCHED_CPU_NR * sizeof(u32));
		memcpy(__entry->pre_cpu_load, multitask_env->pre_cpu_load, GEAS_SCHED_CPU_NR * sizeof(u32));
		memcpy(__entry->cpu_load_def_em, multitask_env->cpu_load_def_em, GEAS_SCHED_CPU_NR * sizeof(u32));
		memcpy(__entry->prev_cpu, multitask_env->prev_cpu, MAX_PIPLINE_TASK);
		memcpy(__entry->util_em, multitask_env->util_em, MAX_PIPLINE_TASK * EM_TYPE_CLUSTER_BUTTON *sizeof(u16));
		memcpy(__entry->task_clamp_min, multitask_env->task_clamp_min, MAX_PIPLINE_TASK * CLUSTER_NUM *sizeof(u16));
		memcpy(__entry->task_clamp_max, multitask_env->task_clamp_max, MAX_PIPLINE_TASK * CLUSTER_NUM *sizeof(u16));
		memcpy(__entry->soft_freq_max, multitask_env->soft_freq_max, CLUSTER_NUM *sizeof(u32));
		memcpy(__entry->soft_freq_min, multitask_env->soft_freq_min, CLUSTER_NUM *sizeof(u32));),

	TP_printk("ret=%d task_num %d def_em_type %u last_pipline %d exclusive 0x%lx online 0x%lx "
			"prev_cpu %u/%u/%u util task0 %u/%u/%u/%u task1 %u/%u/%u/%u task2 %u/%u/%u/%u "
			"soft_freq_max %u/%u/%u soft_freq_min %u/%u/%u"
			"pre_freq %u/%u/%u/%u/%u/%u/%u/%u "
			"pre_cpu_load %u/%u/%u/%u/%u/%u/%u/%u "
			"cpu_load_def_em %u/%u/%u/%u/%u/%u/%u/%u "
			"delta_ns %llu"
			,
			__entry->ret, __entry->task_num, __entry->def_em_type, __entry->last_pipline, __entry->exclusive, __entry->online,
			__entry->prev_cpu[0], __entry->prev_cpu[1], __entry->prev_cpu[2],
			__entry->util_em[0], __entry->util_em[1], __entry->util_em[2], __entry->util_em[3], __entry->util_em[4], __entry->util_em[5],
			__entry->util_em[6], __entry->util_em[7], __entry->util_em[8], __entry->util_em[9], __entry->util_em[10], __entry->util_em[11],
			__entry->soft_freq_max[0], __entry->soft_freq_max[1], __entry->soft_freq_max[2],
			__entry->soft_freq_min[0], __entry->soft_freq_min[1], __entry->soft_freq_min[2],
			__entry->pre_freq[0], __entry->pre_freq[1], __entry->pre_freq[2], __entry->pre_freq[3], __entry->pre_freq[4],
			__entry->pre_freq[5], __entry->pre_freq[6], __entry->pre_freq[7],
			__entry->pre_cpu_load[0], __entry->pre_cpu_load[1], __entry->pre_cpu_load[2], __entry->pre_cpu_load[3],
			__entry->pre_cpu_load[4], __entry->pre_cpu_load[5], __entry->pre_cpu_load[6], __entry->pre_cpu_load[7],
			__entry->cpu_load_def_em[0], __entry->cpu_load_def_em[1], __entry->cpu_load_def_em[2], __entry->cpu_load_def_em[3],
			__entry->cpu_load_def_em[4], __entry->cpu_load_def_em[5], __entry->cpu_load_def_em[6], __entry->cpu_load_def_em[7],
			__entry->delta_ns)
);

#endif /* _TRACE_GEAS_CPU_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../vendor/oplus/kernel/cpu/geas/cpu
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_geas

#include <trace/define_trace.h>
