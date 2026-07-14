// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas-sharebuck: " fmt

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
#include <linux/energy_model.h>
#include "../../../kernel/sched/sched.h"
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "trace_geas.h"
#include "geas_dyn_em.h"
#include "sharebuck.h"
#include "geas_cpu_para.h"

struct geas_sched_cluster_info *get_shb_cluster(struct geas_sched_cluster_info *cl)
{
	if (cl->shb_type != TYPE_INTERCLUSTER)
		return NULL;
	return cl->shbuck_cluster;
}

unsigned long get_freq_vol_mv(unsigned long freq, struct geas_opp_table *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (table[i].frequency_khz >= freq) {
			return table[i].volt_mv;
		}
	}
	return 0;
}
static int get_vol_boost_freq_index(struct em_perf_domain *pd, struct geas_opp_table *table,
				int opp_count, unsigned long vol_mv, int cur_freq_index)
{
	int i = cur_freq_index;
	for (; i < pd->nr_perf_states; i++) {
		unsigned long freq = pd->em_table->state[i].frequency;
		unsigned long vol_temp = get_freq_vol_mv(freq,
						table, opp_count);
		if (vol_temp > vol_mv) {
			break;
		}
	}
	return  max_t(int, i - 1, 0);
}

/* hardcode for fetch dsu opp table */
static int get_dsu_geas_opp_table(struct geas_opp_table **freq_table_inp,
				struct em_perf_domain *pd)
{
	int coe, idx = 0;
	int nr_op_cpu, nr_op_mem = 0;
	struct geas_opp_table *geas_opp_table_cpu0 = NULL;
	struct geas_opp_table *freq_table = NULL;
	struct cpufreq_dsufreq_map *cpufreq_dsufreq_map = NULL;

	nr_op_cpu = get_geas_opp_table(0, &geas_opp_table_cpu0, pd, &coe);
	if (nr_op_cpu < 0) {
		pr_err("failed get cpu0 opp info ret = %d", nr_op_cpu);
		return nr_op_cpu;
	}
	nr_op_mem = get_cpufreq_dsufreq_map(0, &cpufreq_dsufreq_map);
	if (nr_op_mem < 0) {
		pr_err("failed get cpufreq_dsufreq_map info ret = %d", nr_op_mem);
		return nr_op_mem;
	}

	freq_table = kcalloc(nr_op_mem, sizeof(*freq_table), GFP_KERNEL);
	if (!freq_table)
		return -ENOMEM;

	for (; idx < nr_op_mem; idx++) {
		freq_table[idx].frequency_khz = cpufreq_dsufreq_map[idx].memfreq_khz;
		freq_table[idx].volt_mv =
			get_freq_vol_mv(cpufreq_dsufreq_map[idx].cpufreq_mhz * 1000, geas_opp_table_cpu0, nr_op_cpu); /* mV */
		pr_debug("dsu %d freq:%lu Khz cpu %d Mhz volt:%lu mv\n", idx,
				freq_table[idx].frequency_khz, cpufreq_dsufreq_map[idx].cpufreq_mhz,
				freq_table[idx].volt_mv);
	}
	*freq_table_inp = freq_table;
	return nr_op_mem;
}

struct shb_config_data **alloc_shb_config_array(int nr_op_self, int nr_op_bro)
{
	int i, j;
	struct shb_config_data **p = kcalloc(nr_op_self, sizeof(struct shb_config_data *), GFP_KERNEL);
	if (!p) {
		pr_err("cost table alloc failed\n");
		return NULL;
	}
	for (i = 0; i < nr_op_self; i++) {
			p[i] = kcalloc(nr_op_bro, sizeof(struct shb_config_data), GFP_KERNEL);
			if (!p[i]) {
				for (j = i; j >= 0; j--) {
					kfree(p[j]);
					p[j] = NULL;
				}
				kfree(p);
				return NULL;
			}
	}
	return p;
}

static void update_shb_config_data(struct geas_sched_cluster_info *cluster,
			unsigned long vol_mv, unsigned long freq, u16 power, u16 util, int index,
			int index_bro, int boost_freq_index, struct shb_config_data **datas,
			unsigned long scale_cpu)
{
	struct shb_config_data *data = &datas[index][index_bro];

	data->vol_mv = vol_mv;
	data->freq = freq;
	data->ps_index = (u16)index;
	data->util = util;
	data->power = (u16)power;
	data->cost_scaled = (power << 10) / util;
	pr_debug("update_shb_config_data: cl%u %u/%u,freq:%lu "
			"vol:%lu util:%u boost:%d @%u, power %u, cost_origin:%u,cost_scaled:%u,lkg_vol:%u",
			cluster->policy_id, index, index_bro, freq, vol_mv,
			data->util, boost_freq_index, datas[boost_freq_index][0].freq, power,
			get_cluster_util_to_cost(cluster, min_t(unsigned long, 1023L, data->util), cluster->dem.cur_enable),
			data->cost_scaled, data->lkg_vol_part);
}

/*
 * pd_x must be a core pd and none NULL.
 * pd_y can be null while create a TYPE_CLUSTER_DSU or NONE Sharebuck state
 */
static int create_shb_config(struct em_perf_domain *pd_x,
			struct em_perf_domain *pd_y, int type, int em_id)

{
	struct shb_config_data **config_x = NULL, **config_y = NULL;
	unsigned long scale_x, scale_y;
	u32 coef_x, coef_y;
	int i_x, i_y, nr_op_x, nr_op_y = 0, ret = 0;
	int cpu_x = geas_sched_get_pd_first_cpu(pd_x);
	int cpu_y = geas_sched_get_pd_first_cpu(pd_y);
	struct geas_sched_cluster_info *cluster_x = geas_sched_get_pd_cluster_info(pd_x);
	struct geas_sched_cluster_info *cluster_y = geas_sched_get_pd_cluster_info(pd_y);
	struct geas_opp_table *geas_opp_table_x = NULL, *geas_opp_table_y = NULL;

	if (cluster_x != NULL) {
		nr_op_x = get_geas_opp_table(cpu_x, &geas_opp_table_x, pd_x, &coef_x);
		if (nr_op_x <= 0) {
			pr_err("opp table not found @ cpu %d\n", cpu_x);
			return -1;
		}
	} else {
		pr_err("create_shb_config failed for first pd null\n");
		return -2;
	}

	if (cluster_y != NULL) {
		nr_op_y = get_geas_opp_table(cpu_y, &geas_opp_table_y, pd_y, &coef_y);
		if (nr_op_y <= 0) {
			pr_err("opp table not found @ cpu %d\n", cpu_y);
			ret = -3;
			goto failed;
		}
	} else if (type == TYPE_CLUSTER_DSU) {
		/* hard code for dsu, use cpu 0*/
		nr_op_y = get_dsu_geas_opp_table(&geas_opp_table_y, pd_x);
		if (nr_op_y <= 0) {
			pr_err("dsu opp table not found\n");
			ret = -4;
			goto failed;
		}
	}

	/* for none shb type */
	if (nr_op_y == 0)
		nr_op_y = 1;

	scale_x = cluster_x->dem.ems[em_id].cap_origin;
	coef_x = cluster_x->dem.ems[em_id].coeff_power;

	config_x = alloc_shb_config_array(nr_op_x, nr_op_y);
	if (!config_x) {
		pr_err("shb_config_data alloc failed @ cpu %d\n", cpu_x);
		ret = -ENOMEM;
		goto failed;
	}
	if (cluster_y != NULL) {
		config_y = alloc_shb_config_array(nr_op_y, nr_op_x);
		if (!config_y) {
			ret = -ENOMEM;
			pr_err("shb_config_data alloc failed @ cpu %d\n", cpu_y);
			goto failed;
		}

		scale_y = cluster_y->dem.ems[em_id].cap_origin;
		coef_y = cluster_y->dem.ems[em_id].coeff_power;
	}

	pr_debug("creating shb config for %d : %d, type %d, emid %d coef_x %d coef_y %d",
		cpu_x, cpu_y, type, em_id, coef_x, coef_y);
	for (i_x = 0; i_x < pd_x->nr_perf_states; i_x++) {
		for (i_y = 0; i_y < nr_op_y; i_y++) {
			int freq_boost_y, freq_boost_x;
			unsigned long freq_x = pd_x->em_table->state[i_x].frequency, freq_y = 0;
			unsigned long vol_x = get_freq_vol_mv(freq_x, geas_opp_table_x, nr_op_x);
			unsigned long vol, vol_y = 0;
			u16 power = 0, util = 0;

			if (cluster_y != NULL) {
				freq_y = pd_y->em_table->state[i_y].frequency;
				vol_y = get_freq_vol_mv(freq_y, geas_opp_table_y, nr_op_y);
			} else if (type == TYPE_CLUSTER_DSU)
				/* hardcode for dsu */
				vol_y = geas_opp_table_y[i_y].volt_mv;

			vol = max_t(unsigned long, vol_x, vol_y);
			freq_boost_x = get_vol_boost_freq_index(pd_x, geas_opp_table_x, nr_op_x, vol, i_x);

			power = cluster_x->dem.ems[em_id].states[i_x].power;
			util = cluster_x->dem.ems[em_id].states[i_x].util;

			update_shb_config_data(cluster_x, vol, freq_x, power, util, i_x, i_y, freq_boost_x, config_x, scale_x);
			if (cluster_y) {
				freq_boost_y = get_vol_boost_freq_index(pd_y, geas_opp_table_y, nr_op_y, vol, i_y);
				power = cluster_y->dem.ems[em_id].states[i_y].power;
				util = cluster_y->dem.ems[em_id].states[i_y].util;
				update_shb_config_data(cluster_y, vol, freq_y, power, util, i_y, i_x, freq_boost_y, config_y, scale_y);
			}
		}
	}

	cluster_x->dem.ems[em_id].shb_config_data = config_x;
	if (cluster_y != NULL)
		cluster_y->dem.ems[em_id].shb_config_data = config_y;

	return 0;

failed:
	if (geas_opp_table_x)
		kfree(geas_opp_table_x);
	if (geas_opp_table_y)
		kfree(geas_opp_table_y);
	if (config_x) {
		int i;
		for (i = 0; i < nr_op_x; i++)
			if (config_x[i])
				kfree(config_x[i]);
		kfree(config_x);
	}
	if (config_y) {
		int i;
		for (i = 0; i < nr_op_y; i++)
			if (config_y[i])
				kfree(config_y[i]);
		kfree(config_y);
	}

	pr_err("%s failed ret %d\n", __func__, ret);

	return ret;
}

static inline void shb_get_pd_index(struct geas_sched_cluster_info *cluster_x,
				struct geas_sched_cluster_info *cluster_y, unsigned long max_util_x,
				unsigned long max_util_y, int *index_x, int *index_y, int em_type)
{
	max_util_x = min_t(unsigned long, 1023, max_util_x);
	max_util_y = min_t(unsigned long, 1023, max_util_y);

	*index_x = geas_sched_get_cluster_util_to_data(cluster_x, max_util_x, em_type)->ps_index;
	if (cluster_y) {
		*index_y = geas_sched_get_cluster_util_to_data(cluster_y, max_util_y, em_type)->ps_index;
	}
}

int get_lkg_cpu(int cl_id, int temp, int freq, int extern_volt, u16 util)
{
	int i;

	if (util < MIN_UTIL)
		return 0;

	if (freq <= static_pwr[cl_id][0].freq)
		return static_pwr[cl_id][0].lkg_cpu;

	for (i = 0; i < MAX_FREQ_OPP_NUM; i++) {
		if (freq <= static_pwr[cl_id][i].freq)
			return static_pwr[cl_id][i].lkg_cpu;
	}

	return 0;
}
int get_lkg_sram(int cl_id, int temp, int freq, int extern_volt, u16 util)
{
	int i;

	if (freq <= static_pwr[cl_id][0].freq)
		return static_pwr[cl_id][0].lkg_sram;

	for (i = 0; i < MAX_FREQ_OPP_NUM; i++) {
		if (freq <= static_pwr[cl_id][i].freq)
			return static_pwr[cl_id][i].lkg_sram;
	}

	return 0;
}

int get_lkg_topo(int cl_id, int temp, int freq, int extern_volt, u16 util)
{
	int i;

	if (freq <= static_pwr[cl_id][0].freq)
		return static_pwr[cl_id][0].lkg_topo;

	for (i = 0; i < MAX_FREQ_OPP_NUM; i++) {
		if (freq <= static_pwr[cl_id][i].freq)
			return static_pwr[cl_id][i].lkg_topo;
	}

	return 0;
}

long shb_pd_compute_energy_multitask_by_cpu(struct shb_energy_env *env,
	struct geas_sched_cluster_info *cl, int type, u16 def_em_type)
{
	u32 cost;
	int cpu, unpipline_cal = 0;
	long dyn = 0, dyn_cpu = 0, lkg_cpu = 0, lkg_per_cpu = 0, lkg_sram = 0, lkg_topo = 0;
	int cl_id = cl->id;

	for_each_cpu(cpu, &cl->cpus) {
		u8 em_type = env->cpu_data[cpu].em_type;
		u16 util;
		if (env->cpu_data[cpu].is_pipiline == 0 && unpipline_cal == 1)
			continue;

		if (env->cpu_data[cpu].is_pipiline == 0)
			util = env->cl_data[cl_id].unpip_util;
		else
			util = env->cpu_data[cpu].cum_util;

		cost = dyn_em_freq_to_cost(env->cl_data[cl_id].freq, em_type, cl);
		env->cpu_data[cpu].cost = cost;
		dyn_cpu = (cost * util);
		env->cpu_data[cpu].energy = dyn_cpu >> 10;
		dyn += dyn_cpu >> 10;

		lkg_per_cpu = get_lkg_cpu(cl_id, 50, env->cl_data[cl_id].freq, 0, env->cpu_data[cpu].cum_util);
		if (env->cpu_data[cpu].is_pipiline == 0)
			lkg_per_cpu = lkg_per_cpu * env->cl_data[cl_id].unpipline_cpu_num;
		lkg_cpu += lkg_per_cpu;

		if (util && !lkg_sram) {
			lkg_sram = get_lkg_sram(cl_id, 50, env->cl_data[cl_id].freq, 0, env->cpu_data[cpu].cum_util);
			lkg_topo = get_lkg_topo(cl_id, 50, env->cl_data[cl_id].freq, 0, env->cpu_data[cpu].cum_util);
		}
		env->cpu_data[cpu].lkg = lkg_per_cpu;
		if (env->cpu_data[cpu].is_pipiline == 0)
			unpipline_cal = 1;

		trace_sched_shb_compute_energy_cpu(cpu, em_type, cost, util, dyn_cpu >> 10, lkg_per_cpu, env->cl_data[cl_id].freq);
	}
	env->cl_data[cl->id].dyn_energy = dyn;
	env->cl_data[cl->id].lkg_energy = lkg_cpu + lkg_sram + lkg_topo;
	env->cl_data[cl->id].lkg_cpu = lkg_cpu;
	env->cl_data[cl->id].lkg_sram = lkg_sram;
	env->cl_data[cl->id].lkg_topo = lkg_topo;
	env->cl_data[cl->id].energy = dyn + lkg_cpu + lkg_sram + lkg_topo;

	return env->cl_data[cl->id].energy;
}

int update_share_buck_config(void)
{
	struct geas_sched_cluster_info *cl;
	int cl_id;

	for (cl_id = 0; cl_id < numb_of_clusters; cl_id++) {
		cl = geas_sched_info[cl_id];
		if (!cl) {
			pr_err("%s failed geas_sched_info is null \n", __func__);
			return -1;
		}

		cl->shb_type = TYPE_NONE;
	}
	return 0;
}

int create_share_buck_freq_to_cost(void)
{
	struct geas_sched_cluster_info *cl;
	int i = 0, ret = 0, cl_id;
	struct em_perf_domain *pd_x;

	ret = update_share_buck_config();
	if (ret < 0)
		goto out;

	for (cl_id = 0; cl_id < numb_of_clusters; cl_id++) {
		cl = geas_sched_info[cl_id];
		if (!cl) {
			pr_err("%s failed geas_sched_info is null \n", __func__);
			ret = -2;
			goto out;
		}
		int cpu = cpumask_first(&cl->cpus);
		pd_x = em_cpu_get(cpu);
		if (!pd_x) {
			pr_err("%s em_cpu_get failed\n", __func__);
			ret = -1;
			goto out;
		}
		if (cl && !cl->shb_cost_inited) {
			/* bro pd is NULL for !INTER_CLUSTER sharing */
			for (i = 0; i < cl->dem.numb_of_em; i++) {
				ret = create_shb_config(pd_x, NULL,
						cl->shb_type, i);
				if (ret < 0)
					goto out;
			}
		}
	}

out:
	pr_info("create_share_buck_freq_to_cost done");
	return ret;
}

int sharebuck_init(void)
{
	return create_share_buck_freq_to_cost();
}
