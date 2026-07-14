// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "pipline_eas: " fmt
#include <trace/hooks/sched.h>
#include <trace/hooks/cpufreq.h>
#include <linux/syscore_ops.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <trace/events/power.h>
#include <linux/sysfs.h>
#include <linux/kmemleak.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/circ_buf.h>
#include <linux/ktime.h>
#include <linux/perf_event.h>
#include <linux/errno.h>
#include <linux/topology.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/list_sort.h>
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "trace_geas.h"
#include "geas_task_manager.h"
#include "sharebuck.h"
#include "geas_dyn_em.h"
#include "pipline_eas.h"
#include "geas_cpu_para.h"
int pipline_debug_mask = 0;
int pipline_mem_debug_ratio[MAX_CPU_MAP_NUM] = {110, 100, 120, 120, 100, 105, 105};
int pipline_debug_ratio_1[MAX_CPU_MAP_NUM] = {2, 2, 3, 3, 5, 5, 5};

bool pipline_task_ready = false;

void pipline_update_shb_energy_env_base(struct multitask_energy_env *multitask_env)
{
	int cpu = 0, index = 0, i;
	struct geas_sched_cluster_info *cluster;
	struct shb_energy_env *env = &multitask_env->shb_env;
	struct cpumask cpus = multitask_env->online;
	int task_num = multitask_env->task_num;
	u8 freq_policy_type = key_thread_info.freq_policy_type;
	u64 start = 0;

	if (trace_sched_multitask_energy_env_enabled())
		start = geas_sched_ktime_get_ns();

	memset(env, 0, sizeof(struct shb_energy_env));
	for (i = 0; i < task_num; i++) {
		env->dst_cpu[i] = 0;
		cpumask_clear_cpu(multitask_env->prev_cpu[i], &cpus);
	}

	for_each_cpu(cpu, &cpus) {
		int cl_id;
		cluster = geas_sched_cpu_cluster_info(cpu);
		cl_id = cluster->id;
		env->cl_data[cl_id].sum_util += multitask_env->cpu_load_def_em[cpu];
		env->cl_data[cl_id].unpip_util += multitask_env->cpu_load_def_em[cpu];
		env->cpu_data[cpu].cum_util = multitask_env->cpu_load_def_em[cpu];
		env->cpu_data[cpu].em_type = multitask_env->def_em_type;
		env->cpu_data[cpu].is_pipiline = 0;
		if (freq_policy_type == FREQ_POLICY_MAX)
			env->cl_data[cl_id].max_util = max(env->cl_data[cl_id].max_util, env->cpu_data[cpu].cum_util);

		if (env->cpu_data[cpu].cum_util > MIN_UTIL)
			multitask_env->unpipline_cpu_num[cl_id]++;
	}

	index = 0;
	for_each_cluster_info(cluster, index) {
		int cl_id = cluster->id;
		int num;
		env->cl_data[cl_id].unpipline_cpu_num = multitask_env->unpipline_cpu_num[cl_id];
		num = env->cl_data[cl_id].unpipline_cpu_num;
		if ((freq_policy_type == FREQ_POLICY_MEAN) && num)
			env->cl_data[cl_id].max_util = env->cl_data[cl_id].unpip_util / num;
	}

	trace_sched_multitask_energy_env(env, multitask_env, 1, geas_sched_ktime_get_ns() - start);
}

static inline void clear_shb_energy_env(struct shb_energy_env *env, int task_num, u16 def_em_type)
{
	int cpu = 0, index = 0, i;
	struct geas_sched_cluster_info *cluster;

	for_each_cluster_info(cluster, index) {
		env->cl_data[index].energy = 0;
		env->cl_data[index].sum_util = 0;
		env->cl_data[index].freq = 0;
	}
	for (cpu = 0; cpu < MAX_CPU_NR; cpu++) {
		env->cpu_data[cpu].energy = 0;
		env->cpu_data[cpu].is_pipiline = 0;
		env->cpu_data[cpu].em_type = def_em_type;
		env->cpu_data[cpu].freq = 0;
		env->cpu_data[cpu].target_load = 0;
	}
	for (i = 0; i < task_num; i++) {
		if (env->dst_cpu[i] != 0) {
			env->cpu_data[env->dst_cpu[i]].cum_util = 0;
		}
	}
}

int pipline_update_shb_energy_env_by_cpu(struct multitask_energy_env *multitask_env, u8 *cpu_map, enum em_type_cluster type)
{
	int cpu = 0, index = 0;
	int i, cl_id, ret = 0;
	struct geas_sched_cluster_info *cluster;
	struct shb_energy_env *env = &multitask_env->shb_env;
	int task_num = multitask_env->task_num;
	int cl_pip_num[CLUSTER_NUM] = {0, 0, 0, 0};
	u16 def_em_type = multitask_env->def_em_type;
	u64 start = 0;
	u8 freq_policy_type = key_thread_info.freq_policy_type;

	if (trace_sched_multitask_energy_env_enabled())
		start = geas_sched_ktime_get_ns();

	clear_shb_energy_env(env, task_num, def_em_type);

	for (i = 0; i < task_num; i++) {
		cpu = cpu_map[i];
		cpu = min(cpu, geas_cpu_num);
		cluster = geas_sched_cpu_cluster_info(cpu);
		cl_id = cluster->id;
		cl_pip_num[cl_id]++;
		env->cpu_data[cpu].cum_util = multitask_env->util_em[i][type];
		env->cpu_data[cpu].em_type = get_em_type(multitask_env->em_type[i], type);
		env->cpu_data[cpu].is_pipiline = 1;
		env->dst_cpu[i] = cpu;
	}

	index = 0;
	for_each_cluster_info(cluster, index) {
		int cl_id = cluster->id;
		u16 target_load = 0;
		u8 cpu_num = cluster->cpu_num;
		u8 num = multitask_env->unpipline_cpu_num[cl_id];
		u16 max_util;
		u32 freq_temp;

		/* pipline cpu may task place of unpipline cpu */
		env->cl_data[cl_id].unpipline_cpu_num = min(num, cpu_num - cl_pip_num[cl_id]);
		env->cl_data[cl_id].pipline_cpu_num = cl_pip_num[cl_id];

		if (env->cl_data[cl_id].unpipline_cpu_num) {
			int first_cpu = cpumask_first(&cluster->cpus);
			if (freq_policy_type == FREQ_POLICY_MEAN)
				max_util = env->cl_data[cl_id].unpip_util / env->cl_data[cl_id].unpipline_cpu_num;
			else
				max_util = env->cl_data[cl_id].max_util;
			freq_temp = geas_sched_get_cluster_util_to_data(cluster, max_util, def_em_type)->freq;
			target_load = geas_sched_util_to_target_load_inner(freq_temp, first_cpu, cl_id);
			max_util = max_util * target_load >> SCHED_CAPACITY_SHIFT;
			env->cl_data[cl_id].freq = geas_sched_get_cluster_util_to_data(cluster, max_util, def_em_type)->freq;
			env->cpu_data[first_cpu].freq = env->cl_data[cl_id].freq;
			env->cpu_data[first_cpu].target_load = target_load;
		}

		cpu = 0;
		for_each_cpu(cpu, &cluster->cpus) {
			struct shb_cpu_loading *cpu_loading = &env->cpu_data[cpu];
			if (cpu_loading->em_type != def_em_type) {
				u16 util;
				u8 em_type;
				u32 freq;

				/* if is same last_pipline we use base em cal freq */
				if (type == multitask_env->last_pipline) {
					util = multitask_env->pre_cpu_load[cpu];
					em_type = EM_TYPE_ORIGIN;
				} else {
					util = cpu_loading->cum_util;
					em_type =  cpu_loading->em_type;
				}
				freq_temp = geas_sched_get_cluster_util_to_data(cluster, util, em_type)->freq;
				target_load = geas_sched_util_to_target_load_inner(freq_temp, cpu, cl_id);

				util = util * target_load >> SCHED_CAPACITY_SHIFT;
				freq = geas_sched_get_cluster_util_to_data(cluster, util, em_type)->freq;

				cpu_loading->freq = freq;
				cpu_loading->target_load = target_load;
				env->cl_data[cl_id].freq = max((u32)env->cl_data[cl_id].freq, freq);
			}
		}

		env->cl_data[cl_id].freq = min(multitask_env->soft_freq_max[cl_id], (u32)env->cl_data[cl_id].freq);
		env->cl_data[cl_id].freq = max(multitask_env->soft_freq_min[cl_id], (u32)env->cl_data[cl_id].freq);
	}

	trace_sched_multitask_energy_env(env, multitask_env, ret, geas_sched_ktime_get_ns() - start);
	return ret;
}

long pipline_shb_compute_energy_by_cpu(struct multitask_energy_env *multitask_env, u8 *cpu_map, enum em_type_cluster type)
{
	long energy = 0;
	struct geas_sched_cluster_info *cl;
	struct shb_energy_env *env = &multitask_env->shb_env;
	int index = 0, ret;
	u64 start;
	if (trace_sched_shb_compute_energy_cl_enabled())
		start = geas_sched_ktime_get_ns();

	ret = pipline_update_shb_energy_env_by_cpu(multitask_env, cpu_map, type);
	if (ret < 0)
		return UINT_MAX;

	for_each_cluster_info(cl, index) {
		energy += shb_pd_compute_energy_multitask_by_cpu(env, cl, cl->shb_type, multitask_env->def_em_type);
	}
	trace_sched_shb_compute_energy_cl(env, energy, geas_sched_ktime_get_ns() - start);

	return energy;
}

u32 get_freq_by_util(struct geas_sched_cluster_info *cl, u8 em_type, u16 util)
{
	u32 freq = get_cluster_util_to_raito_by_emtype(cl, em_type, util);
	return freq;
}

int get_last_cpu(int task_id, int task_num, int last_pipline)
{
	return pipline_cpu_map[task_num].cpu_map[last_pipline][task_id];
}

void geas_log_c_printk(const char *msg, unsigned long val);

void geas_log_printk(const char *msg, int cpu, unsigned long val)
{
	char buf[128];
	snprintf(buf, sizeof(buf), "%s_%d", msg, cpu);
	geas_log_c_printk(buf, val);
}

void geas_log_id_printk(const char *msg, int id, int cpu, unsigned long val)
{
	char buf[128];
	snprintf(buf, sizeof(buf), "%s_%d_%d", msg, id, cpu);
	geas_log_c_printk(buf, val);
}

#define DEF_CAL_MASK 0xff
static inline u16 get_pipline_cal_mask(void)
{
	u16 pipline_cal_mask = key_thread_info.pipline_cal_mask;
	if (pipline_cal_mask == 0)
		return DEF_CAL_MASK;
	return pipline_cal_mask;
}

int pipline_update_load_info_by_cpu(struct multitask_energy_env *multitask_env, struct multitask_load_info *load_info)
{
	int i = 0, ret = 0, task_num = 0, cl_id, last_cpu;
	struct cpumask cpus = *cpu_possible_mask;
	u16 def_em_type = key_thread_info.def_em_type;
	u16 pipline_cal_mask = get_pipline_cal_mask();
	u64 start;

	if (trace_geas_cpu_update_base_info_enabled())
		start = geas_sched_ktime_get_ns();

	multitask_env->online = *cpu_possible_mask;
	multitask_env->def_em_type = def_em_type;
	multitask_env->last_pipline = load_info->last_pipline;
	if (multitask_env->last_pipline >= MAX_CPU_MAP_NUM || multitask_env->last_pipline < 0) {
		ret = -4;
		goto done;
	}

	for_each_cpu(i, &cpus) {
		u16 ratio_same_cluster = 0;
		u16 pre_cpu_load = load_info->pre_cpu_load[i];
		struct geas_sched_cluster_info *cl_pre = geas_sched_cpu_cluster_info(i);
		multitask_env->pre_freq[i] = load_info->pre_freq[i];

		if (def_em_type != EM_TYPE_ORIGIN) {
			ratio_same_cluster = dyn_em_freq_to_ratio(multitask_env->pre_freq[i], multitask_env->def_em_type, cl_pre);
			if (ratio_same_cluster)
				pre_cpu_load = (pre_cpu_load << 10) / ratio_same_cluster;
			else {
				ret = -5;
				goto done;
			}
			trace_geas_task_util_translate(NULL, def_em_type, 0, i, 0,
				ratio_same_cluster, multitask_env->pre_freq[i], 0,
				pre_cpu_load, load_info->pre_cpu_load[i]);
		}

		multitask_env->cpu_load_def_em[i] = pre_cpu_load;
		multitask_env->pre_cpu_load[i] = load_info->pre_cpu_load[i];
		if (GEAS_TRACE_ENABLE) {
			geas_log_printk("pre_freq", i, multitask_env->pre_freq[i]);
			geas_log_printk("pre_load", i, multitask_env->pre_cpu_load[i]);
		}
	}

	for (i = 0; i < GEAS_SCHED_CLUSTER_NR; i++) {
		multitask_env->soft_freq_max[i] = load_info->soft_freq_max[i];
		multitask_env->soft_freq_min[i] = load_info->soft_freq_min[i];
	}
	multitask_env->soft_freq_min[1] = 883200;

	for (i = 0; i < MAX_PIPLINE_TASK; i++)
		multitask_env->em_type[i] = key_thread_info.task_list[i].em_type;

	for (task_num = 0; task_num < min(key_thread_info.thread_num, MAX_PIPLINE_TASK); task_num++) {
		struct geas_task_struct *geas_task = &key_thread_info.task_list[task_num];
		u32 util, util_base;
		int type, util_em_same_clu;
		u8 em_type;
		u16 ratio_same_cluster;
		struct geas_sched_cluster_info *cl_pre;

		last_cpu = load_info->prev_cpu[task_num];
		if (last_cpu >= geas_cpu_num) {
			ret = -2;
			goto done;
		}
		cl_pre = geas_sched_cpu_cluster_info(last_cpu);
		cl_id = cl_pre->id;
		util_base = load_info->task_util[task_num];
		multitask_env->prev_cpu[task_num] = last_cpu;

		util = util_base;
		for (type = 0; type < EM_TYPE_CLUSTER_BUTTON; type++) {
			if (!(pipline_cal_mask & (1 << type)))
				continue;

			em_type = get_em_type(geas_task->em_type, type);
			ratio_same_cluster = dyn_em_freq_to_ratio(multitask_env->pre_freq[last_cpu], em_type, cl_pre);
			if (!ratio_same_cluster) {
				ret = -3;
				goto done;
			}

			util_em_same_clu = (util << 10) / ratio_same_cluster;
			multitask_env->util_em[task_num][type] = util_em_same_clu;

			trace_geas_task_util_translate(geas_task, em_type, task_num, cl_id, type,
					ratio_same_cluster, multitask_env->pre_freq[last_cpu], util_em_same_clu, util, util_base);
		}
	}

	multitask_env->task_num = task_num;

	if (task_num <= 0)
		ret = -1;

done:
	trace_geas_cpu_update_base_info(ret, multitask_env, geas_sched_ktime_get_ns() - start);
	return ret;
}

u64 pipline_select_cnt[MAX_CPU_MAP_NUM];
int get_share_mem_power(struct multitask_energy_env *multitask_env, long energy, int i, u8 mem_ratio)
{
	long energy_temp = energy;

	if ((pipline_debug_mask & 1) == 0)
		return energy;

	if (pipline_mem_debug_ratio[i] != 0)
		energy_temp = energy * pipline_mem_debug_ratio[i] / 100;
	else
		energy_temp = energy * mem_ratio / 100;
	/* to do judge mem bound and get ratio */
	return energy_temp;
}

int pipline_find_energy_efficient_cpu(struct multitask_result *result, struct multitask_energy_env *multitask_env)
{
	struct pipline_cpu_map_info *map;
	int i, j;
	long energy[MAX_CPU_MAP_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	long energy_mem[MAX_CPU_MAP_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	long best_energy = LONG_MAX, last_energy = LONG_MAX;
	int ratio;
	int task_num = multitask_env->task_num;
	int last_pipline = multitask_env->last_pipline;
	int best_pipline = last_pipline;
	int ret_pipline = last_pipline;

	u16 pipline_cal_mask = get_pipline_cal_mask();
	u64 start;

	task_num = min(task_num, MAX_PIPLINE_TASK);
	map = &pipline_cpu_map[max(0, task_num - 1)];
	if (trace_pipline_find_efficient_cpu_enabled())
		start = geas_sched_ktime_get_ns();

	pipline_update_shb_energy_env_base(multitask_env);
	for (i = 0; i < map->map_num; i++) {
		if (!(pipline_cal_mask & (1 << i)))
			continue;

		energy[i] = pipline_shb_compute_energy_by_cpu(multitask_env, map->cpu_map[i], i);
		energy_mem[i] = get_share_mem_power(multitask_env, energy[i], i, map->mem_ratio[i]);
		for (j = 0; j < CLUSTER_NUM; j++)
			result->freq_pip[i][j] = multitask_env->shb_env.cl_data[j].freq;

		if (energy_mem[i] < best_energy) {
			best_energy = energy_mem[i];
			best_pipline = i;
		}
		if (last_pipline == i)
			last_energy = energy_mem[i];
	}

	ratio = pipline_debug_ratio_1[best_pipline];
	if (best_energy && ((last_energy - best_energy) * 100 / best_energy < ratio)) {
		ret_pipline = last_pipline;
	} else {
		ret_pipline = best_pipline;
	}

	for (i = 0; i < task_num; i++) {
		result->dst_cpu[i] = map->cpu_map[ret_pipline][i];
		result->util[i] = multitask_env->pre_cpu_load[multitask_env->prev_cpu[i]];
	}

	/* for demo */
	pipline_select_cnt[ret_pipline]++;

	trace_pipline_find_efficient_cpu(ret_pipline, best_pipline, best_energy,
		last_pipline, last_energy, energy, energy_mem, geas_sched_ktime_get_ns() - start);
	if (GEAS_TRACE_ENABLE) {
		for (i = 0; i < map->map_num; i++) {
			if (!(pipline_cal_mask & (1 << i)))
				continue;
			for (j = 0; j < min(numb_of_clusters, CLUSTER_NUM); j++)
				geas_log_id_printk("pipfreq", i, j, result->freq_pip[i][j]);
			geas_log_printk("energy", i, energy_mem[i]);
		}

		geas_log_c_printk("ret_pipline", ret_pipline);
		geas_log_c_printk("best_pipline", best_pipline);
		geas_log_c_printk("last_pipline", last_pipline);
	}

	return ret_pipline;
}

DEFINE_MUTEX(pipline_mutex);
int geas_update_load_info(struct multitask_load_info *load_info)
{
	struct multitask_energy_env multitask_env;
	struct multitask_result result;
	int ret = 0, val = 0, i;

	if (GEAS_PRINT_ENABLE)
		pr_info("%s geas_cpu_enable %d geas_cpu_inited %d pipline_task_ready %u cpu_mask %x %x %x %x "
			"task_util %u/%u/%u clamp_min  %u/%u/%u clamp_max %u/%u/%u prev_cpu %u %u %u soft_min %u %u soft_max %u %u ",
			__func__, geas_cpu_enable, geas_cpu_inited, pipline_task_ready,
			load_info->cpu_mask.ec, load_info->cpu_mask.pf, load_info->cpu_mask.pt, load_info->cpu_mask.ex,
			load_info->task_util[0], load_info->task_util[1], load_info->task_util[2],
			load_info->task_clamp_min[0], load_info->task_clamp_min[1], load_info->task_clamp_min[2],
			load_info->task_clamp_max[0], load_info->task_clamp_max[1], load_info->task_clamp_max[2],
			load_info->prev_cpu[0], load_info->prev_cpu[1], load_info->prev_cpu[2],
			load_info->soft_freq_min[0], load_info->soft_freq_min[1],
			load_info->soft_freq_max[0], load_info->soft_freq_max[1]);

	if (unlikely(!geas_cpu_inited || !geas_cpu_enable))
		return -1;

	if (pipline_task_ready == false)
		return -1;

	mutex_lock(&pipline_mutex);
	ret = pipline_update_load_info_by_cpu(&multitask_env, load_info);
	if (ret == 0) {
		val = pipline_find_energy_efficient_cpu(&result, &multitask_env);
		load_info->out.pipline = val;
		for (i = 0; i < GEAS_SCHED_CLUSTER_NR; i++)
			load_info->out.freq_cal[i] = result.freq_pip[val][i];
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			load_info->out.pipline_cpu[i] = result.dst_cpu[i];
	}

	mutex_unlock(&pipline_mutex);

	if (pipline_debug_mask & 4)
		return -1;

	return ret;
}

static inline void print_base_info(struct multitask_ofb_base_info *base_info)
{
	int i;
	struct pipline_cpu_map_info *cpu_map_info = &base_info->pipline_cpu_map;
	pr_info("%s thread_num %u em_mask %d dyn_enable %d em_type %x %x %x def_em_type %u pipline_cal_mask 0x%x ctr_mode %u %u",
		__func__, base_info->thread_num, base_info->em_mask, base_info->dyn_enable,
		base_info->em_type[0], base_info->em_type[1], base_info->em_type[2], base_info->def_em_type,
		base_info->pipline_cal_mask, base_info->ctr_mode[0], base_info->ctr_mode[1]);

	for (i = 0; i < GEAS_SCHED_CPU_NR; i++)
		pr_info("%s cpu %d targetload %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
			__func__, i,
			base_info->targetload[i][0].util, base_info->targetload[i][0].val, base_info->targetload[i][0].size,
			base_info->targetload[i][1].util, base_info->targetload[i][1].val, base_info->targetload[i][1].size,
			base_info->targetload[i][2].util, base_info->targetload[i][2].val, base_info->targetload[i][2].size,
			base_info->targetload[i][3].util, base_info->targetload[i][3].val, base_info->targetload[i][3].size,
			base_info->targetload[i][4].util, base_info->targetload[i][4].val, base_info->targetload[i][4].size);

	if (!base_info->dyn_enable)
		pr_info("%s map_num %u mem_ratio %u %u %u %u cpu_map %u/%u  %u/%u  %u/%u  %u/%u	%u/%u",
			__func__, cpu_map_info[base_info->thread_num].map_num,
			cpu_map_info[base_info->thread_num].mem_ratio[0], cpu_map_info[base_info->thread_num].mem_ratio[1],
			cpu_map_info[base_info->thread_num].mem_ratio[2], cpu_map_info[base_info->thread_num].mem_ratio[3],
			cpu_map_info[base_info->thread_num].cpu_map[0][0], cpu_map_info[base_info->thread_num].cpu_map[0][1],
			cpu_map_info[base_info->thread_num].cpu_map[1][0], cpu_map_info[base_info->thread_num].cpu_map[1][1],
			cpu_map_info[base_info->thread_num].cpu_map[2][0], cpu_map_info[base_info->thread_num].cpu_map[2][1],
			cpu_map_info[base_info->thread_num].cpu_map[3][0], cpu_map_info[base_info->thread_num].cpu_map[3][1],
			cpu_map_info[base_info->thread_num].cpu_map[4][0], cpu_map_info[base_info->thread_num].cpu_map[4][1]);
}

int geas_update_ofb_base_info(struct multitask_ofb_base_info *base_info)
{
	int thread_num, i, ret;
	if (GEAS_PRINT_ENABLE)
		print_base_info(base_info);

	if (unlikely(!geas_cpu_inited || !geas_cpu_enable))
		return 0;

	thread_num = base_info->thread_num;
	if (thread_num > MAX_PIPLINE_TASK || thread_num == 0) {
		pr_err("%s: failed base_info->thread_num  %u > MAX_PIPELINE_TASK_NUM %d \n", __func__, thread_num, MAX_PIPLINE_TASK);
		ret = -2;
		goto out;
	}

	mutex_lock(&pipline_mutex);

	/* update  em */
	if (!base_info->dyn_enable && ((pipline_debug_mask & 0x2) == 0x2)) {
			for (i = 0; i < MAX_EM_NUM; i++) {
				if (base_info->em_mask & (1 << i))
					ret = geas_dyn_em_update(i, base_info->em_para[i]);
			}

		/* update pipline map */
		pipline_cpu_map[thread_num - 1] = base_info->pipline_cpu_map;
		memcpy(geas_targetload_list, base_info->targetload, sizeof(geas_targetload_list));
		ret = update_freq_to_target();
		if (ret < 0) {
			mutex_lock(&pipline_mutex);
			return ret;
		}
	}

	if (thread_num > MAX_PIPLINE_TASK)
		thread_num = MAX_PIPLINE_TASK;

	key_thread_info.thread_num = 0;
	key_thread_info.def_em_type = base_info->def_em_type;
	key_thread_info.freq_policy_type = FREQ_POLICY_MAX;

	key_thread_info.pipline_cal_mask = base_info->pipline_cal_mask;
	if (key_thread_info.pipline_cal_mask == 0)
		key_thread_info.pipline_cal_mask = 0x3;

	for (i = 0; i < thread_num; i++)
		ret = geas_add_task_to_group(base_info->tid[i], 1, base_info->em_type[i]);

	mutex_unlock(&pipline_mutex);

out:
	if (ret < 0)
		pipline_task_ready = false;
	else
		pipline_task_ready = true;
	return ret;
}

