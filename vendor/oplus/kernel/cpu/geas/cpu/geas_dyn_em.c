// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas-dyn-em: " fmt

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
#include <trace/hooks/topology.h>
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
#include <linux/energy_model.h>
#include "../../../kernel/sched/sched.h"
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "sharebuck.h"
#include "geas_task_manager.h"
#include "pipline_eas.h"
#include "geas_cpu_para.h"

int dynamic_em_cur_enable = -1;

#define MAX_DYN_EM_COUNT 5
/* id, coeff_power, coeff_i, coeff_a,coeff_b */
#define DYN_COEFF_COUNT 5

/*	GPA target load */
struct geas_target_load geas_targetload_list[GEAS_SCHED_CPU_NR][MAX_TL_SIZE] = {
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
	{
		{200, 85}, {300, 90}, {1024, 95}
	},
};

/*
 *    shift granu size max_util_err
 *    15    30mhz 92    10
 *    14    15mhz 186   5
*/
#define INDEX_HASH_SHIFT 15

static inline int freq_to_index(unsigned long freq)
{
	return freq >> INDEX_HASH_SHIFT;
}
static inline u32 index_to_freq(int index)
{
	return index << INDEX_HASH_SHIFT;
}

struct dynamic_energy_model *get_em_by_emtype(struct geas_sched_cluster_info *info, u8 em_type)
{
	if (geas_sched_dynamic_energy_model_valid(info)) {
		return &info->dem.ems[em_type];
	}
	return NULL;
}

unsigned long dyn_em_freq_to_util_by_emtype(unsigned long freq, struct geas_sched_cluster_info *info, int em_id)
{
	int index = freq_to_index(freq);
	unsigned long util = 0;
	struct dynamic_energy_model *em = &info->dem.ems[em_id];

	if (em) {
		if (index > em->size) {
			index = em->size - 1;
		}
		util = em->freq_to_util[index];
		if (util != 0) {
			return util;
		}
	}
	return 0;
}

int geas_sched_freq_to_util_by_emtype(struct geas_sched_cluster_info *cluster, unsigned long freq, int em_id)
{
	int cpu = cpumask_first(&cluster->cpus);

	if (geas_sched_dynamic_energy_model_valid(cluster)) {
		return dyn_em_freq_to_util_by_emtype(freq, cluster, em_id);
	}
	return mult_frac(arch_scale_cpu_capacity(cpu), freq,
			 cluster->max_possible_freq);
}

u16 dyn_em_freq_to_ratio(unsigned long freq, u8 em_type, struct geas_sched_cluster_info *info)
{
	int index = freq_to_index(freq);
	struct dynamic_energy_model *em = get_em_by_emtype(info, em_type);
	if (em) {
		if (index > em->size) {
			index = em->size - 1;
		}
		return em->freq_to_ratio[index];
	}
	return 0;
}

u16 dyn_em_freq_to_cost(unsigned long freq, u8 em_type, struct geas_sched_cluster_info *info)
{
	int index = freq_to_index(freq);
	struct dynamic_energy_model *em = get_em_by_emtype(info, em_type);
	if (em) {
		if (index > em->size) {
			index = em->size - 1;
		}
		return em->freq_to_cost[index];
	}
	return 0;
}
static void create_util_to_freq_for_em(struct dynamic_energy_model *em, struct dynamic_em_manager *dem, int em_id)
{
	int j, util;

	for (util = 0; util < 1024; util++) {
		u32 power;
		for (j = 0; j < dem->nr_freq; j++) {
			if (em->states[j].util >= util)
				break;
		}

		j = min((int)dem->nr_freq - 1, j);
		j = max(0, j);

		if ((j == 0) || (j == dem->nr_freq - 1)) {
			em->util_to_data[util].freq = em->states[j].frequency;
			em->util_to_data[util].ps_index = j;
			power = em->states[j].power;
			if (em->states[j].util)
				em->util_to_data[util].cost = (power << 10) / em->states[j].util;
		} else {
			u64 a = 0;
			u16 util_delta;
			util_delta = em->states[j].util - em->states[j - 1].util;
			if (util_delta != 0)
				a = ((u32)(util - em->states[j-1].util) << 10) / util_delta;
			power = (em->states[j - 1].power << 10) + (a * (em->states[j].power - em->states[j - 1].power));
			if (util)
				em->util_to_data[util].cost = power / util;
			em->util_to_data[util].freq = em->states[j - 1].frequency + ((a * (em->states[j].frequency - em->states[j - 1].frequency)) >> 10);
			em->util_to_data[util].ps_index = j;
		}
		if (GEAS_PRINT_ENABLE)
			pr_err("create_util_to_freq_for_em cluster%d em_id %d type %d, util:%d freq %u index %d power=%u %u cost %u freq %u ps_index %u\n",
					dem->cluster->id, em_id, em->type, util, em->states[j].frequency, j, em->states[j].power, power,
					em->util_to_data[util].cost, em->util_to_data[util].freq, em->util_to_data[util].ps_index);
	}
}

int geas_dyn_em_update(int em_id, struct geas_perf_state em_para_cl[CLUSTER_NUM][MAX_FREQ_OPP_NUM])
{
	struct dynamic_em_manager *dem;
	struct geas_sched_cluster_info *info = NULL;
	int cpu = 0;
	int j, index_start;
	int id = 0;
	if (em_id >= MAX_EM_NUM)
		return -1;

	for_each_cluster_info(info, id) {
		struct dynamic_energy_model *em;
		u32 max_freq = info->max_possible_freq;
		if (!geas_sched_dynamic_energy_model_valid(info)) {
			pr_err("%s id %d failed \n", __func__, id);
			return -2;
		}

		dem = &info->dem;
		cpu = cpumask_first(&info->cpus);
		if (GEAS_PRINT_ENABLE)
			pr_err("geas_dyn_em_update cpu %d em_id %d\n", cpu, em_id);
		em = &dem->ems[em_id];
		em->type = 0;  /* to do */
		index_start = 0;
		for (j = 0; j < dem->nr_freq; j++) {
			int index, index_end;
			em->states[j] = em_para_cl[info->id][j];
			em->states[j].frequency = min(em->states[j].frequency, max_freq);
			index_end = freq_to_index(em->states[j].frequency);
			index_end = min(max(em->size - 1, 0), index_end);
			for (index = index_start; index <= index_end; index++) {
				if (j == 0) {
					em->freq_to_util[index] = em->states[j].util;
					em->freq_to_ratio[index] = em->states[j].util_ratio;
					if (em->states[j].util)
						em->freq_to_cost[index] = ((u32)(em->states[j].power) << 10) / em->states[j].util;
				} else {
					int freq_delta, power;
					u64 a = 0;
					freq_delta = em->states[j].frequency - em->states[j - 1].frequency;
					if (freq_delta != 0)
						a = ((u64)(index_to_freq(index) - em->states[j - 1].frequency) << 10) / freq_delta;
					em->freq_to_util[index] = a * (em->states[j].util - em->states[j - 1].util) / 1024 + em->states[j - 1].util;
					em->freq_to_ratio[index] = (a * (em->states[j].util_ratio - em->states[j - 1].util_ratio)) / 1024 + em->states[j - 1].util_ratio;
					power = (a * (em->states[j].power - em->states[j - 1].power)) + (em->states[j - 1].power << 10);
					if (em->freq_to_util[index])
						em->freq_to_cost[index] = power / em->freq_to_util[index];
				}

				if (GEAS_PRINT_ENABLE)
					pr_err("cluster: %d, em_id %d freq: %u, index: %d, index_to_freq %d, util %u, util_ratio %u freq_to_cost %u\n",
						info->id, em_id, em->states[j].frequency, index, index_to_freq(index),
						em->freq_to_util[index], em->freq_to_ratio[index], em->freq_to_cost[index]);
			}
			if (GEAS_PRINT_ENABLE)
				pr_err("cluster: %d, em_id %d freq: %u, index: %d, util %u, util_ratio %u, vol_mv %u,"
					"power %u index_start %d index_end %d \n",
					info->id, em_id, em->states[j].frequency,
					freq_to_index(em->states[j].frequency),
					em->states[j].util, em->states[j].util_ratio, em->states[j].vol_mv,
					em->states[j].power, index_start, index_end);
			index_start = index_end + 1;
		}

		em->cap_origin = em->states[info->dem.nr_freq - 1].util;
		create_util_to_freq_for_em(em, dem, em_id);
	}

	return 0;
}

int geas_dyn_em_create(void)
{
	struct dynamic_em_manager *dem;
	struct geas_sched_cluster_info *cl = NULL;
	unsigned long fmax;
	int map_size = 0;
	u32 count = MAX_EM_NUM;
	u32 coef;
	int cpu = 0;
	int nr_opp = 0;
	int i, ret = 0, cl_id;
	struct geas_opp_table *geas_opp_table = NULL;
	struct em_perf_domain *em_pd;

	for (cl_id = 0; cl_id < numb_of_clusters; cl_id++) {
		cl = geas_sched_info[cl_id];
		if (!cl) {
			pr_err("%s failed geas_sched_info is null \n", __func__);
			ret = -7;
			goto out;
		}
		cpu = cpumask_first(&cl->cpus);
		em_pd = em_cpu_get(cpu);
		if (!em_pd) {
			pr_err("%s em_cpu_get failed\n", __func__);
			ret = -6;
			goto out;
		}
		dem = &cl->dem;
		pr_info("geas_dyn_em_create cpu %d count %d\n", cpu, count);
		nr_opp = get_geas_opp_table(cpu, &geas_opp_table, em_pd, &coef);
		if (nr_opp <= 0) {
			pr_err("failed to get opp table for cpu %d", cpu);
			ret = -2;
			goto out;
		}
		dem->ems = kcalloc(count, sizeof(struct dynamic_energy_model), GFP_ATOMIC);
		if (!dem->ems) {
			ret = -3;
			goto out;
		}

		dem->numb_of_em = count;
		dem->nr_freq = em_pd->nr_perf_states;
		for (i = 0; i < count; i++) {
			struct dynamic_energy_model *em = &dem->ems[i];
			em->states = kcalloc(em_pd->nr_perf_states,
				sizeof(struct geas_perf_state), GFP_KERNEL);
			if (!em->states) {
				pr_err("alloc failed states or freq_to_ratio\n");
				ret = -4;
				goto out;
			}
			fmax = geas_sched_get_pd_fmax(em_pd);
			map_size = freq_to_index(fmax) + 1;
			em->freq_to_util = kcalloc(map_size * 3, sizeof(u16), GFP_ATOMIC);
			em->freq_to_ratio = em->freq_to_util + map_size;
			em->freq_to_cost = em->freq_to_util + map_size * 2;

			em->size = map_size;
			if (!em->freq_to_util) {
				pr_err("alloc failed freq_to_util or freq_to_ratio\n");
				ret = -5;
				goto out;
			}
			em->cluster_id = cl->id;
			em->type = 0;  /*  to do  */
		}
		dem->valid = 1;
	}

out:
	pr_info("%s ret = %d\n", __func__, ret);
	return ret;
}

u16 freq_to_index_max[GEAS_SCHED_CLUSTER_NR];
u16 *freq_to_target[GEAS_SCHED_CPU_NR];
u16 geas_sched_util_to_target_load_inner(u32 freq, int cpu, int cl_id)
{
	int index = freq_to_index(freq);
	index = min(index, (int)(freq_to_index_max[cl_id]));
	return freq_to_target[cpu][index];
}

static int create_freq_to_target(void)
{
	int index = 0, cpu;
	struct geas_sched_cluster_info *info;
	int size = 0, map_size, offset = 0;
	int fmax;

	for_each_cluster_info(info, index) {
		fmax = info->max_possible_freq;
		map_size = freq_to_index(fmax) + 1;
		for_each_cpu(cpu, &info->cpus)
			size += map_size;
	}

	pr_info("%s size %d\n", __func__, size);
	freq_to_target[0] = kcalloc(size, sizeof(u16), GFP_ATOMIC);
	if (!freq_to_target[0]) {
		pr_err("%s failed alloc mem size %d\n", __func__, size);
		return -1;
	}

	index = 0;
	for_each_cluster_info(info, index) {
		int fmax = info->max_possible_freq;
		map_size = freq_to_index(fmax) + 1;
		freq_to_index_max[index] = map_size;
		for_each_cpu(cpu, &info->cpus) {
			freq_to_target[cpu] = freq_to_target[0] + offset;
			offset += map_size;
			pr_info("%s cpu %d offset %d map_size %d\n", __func__, cpu, offset, map_size);
		}
	}

	return 0;
}

int update_freq_to_target(void)
{
	int i, cpu, index;
	u16 start_util = 0, end_util;
	u32 start_freq, end_freq;
	int start_freq_index, end_freq_index;
	struct geas_sched_cluster_info *cluster;

	for (cpu = 0; cpu < geas_cpu_num; cpu++) {
		cluster = geas_sched_cpu_cluster_info(cpu);
		start_util = 0;
		for (i = 0; i < MAX_TL_SIZE; i++) {
			struct geas_target_load load = geas_targetload_list[cpu][i];
			u16 val = load.val;
			if (val == 0) {
				pr_err("%s cpu %d i %d\n", __func__, cpu, i);
				return -11;
			}

			end_util = load.util;
			start_freq = geas_sched_get_cluster_util_to_data(cluster, start_util, EM_TYPE_ORIGIN)->freq;
			end_freq = geas_sched_get_cluster_util_to_data(cluster, end_util, EM_TYPE_ORIGIN)->freq;
			start_freq_index = freq_to_index(start_freq);
			end_freq_index = freq_to_index(end_freq);
			for (index = start_freq_index; index <= end_freq_index; index++)
				freq_to_target[cpu][index] = 100 * 1024 / val;

			if (GEAS_PRINT_ENABLE)
				pr_info("%s cpu %d i %d util %u %u freq %u %u index %d %d val %u\n",
					__func__, cpu, i, start_util, end_util, start_freq, end_freq,
					start_freq_index, end_freq_index, load.val);

			if (load.util == 1024 || end_util <= start_util)
				break;
			start_util = end_util;
		}
	}

	return 0;
}

int geas_dyn_em_init(void)
{
	int i, ret;
	ret = geas_dyn_em_create();
	if (ret < 0)
		return ret;

	for (i = 0; i < MAX_EM_NUM; i++) {
		ret = geas_dyn_em_update(i, em_para[i]);
	}

	if (ret < 0)
		return ret;

	ret = create_freq_to_target();
	if (ret < 0)
		return ret;

	update_freq_to_target();
	return ret;
}

