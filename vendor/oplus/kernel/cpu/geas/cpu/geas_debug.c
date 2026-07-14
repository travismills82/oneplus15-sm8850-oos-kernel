// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas-debug: " fmt
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/sched/cpufreq.h>
#include <linux/syscore_ops.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
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
#include "geas_task_manager.h"
#include "sharebuck.h"
#include "geas_dyn_em.h"
#include "pipline_eas.h"
#include "geas_cpu_debug.h"

int pipline_debug_para_list[MAX_PIPLINE_DEBUG_PARA_LEN];
void show_dyn_em(void)
{
	struct dynamic_em_manager *dem;
	struct geas_sched_cluster_info *info = NULL;
	int index = 0;
	int i, j;
	int util;

	for (i = 0; i < geas_cpu_num; i++) {
		info = geas_sched_cpu_cluster_info(i);
		pr_err("---- cpu %d  map_size %d----\n", i, freq_to_index_max[info->id]);
		for (j = 0; j < freq_to_index_max[info->id]; j++)
			pr_err("    %d  %u\n", i, freq_to_target[i][j]);
	}

	for_each_cluster_info(info, index) {
		dem = &info->dem;
		pr_err("---- clusterID %d ----\n", dem->cluster->id);
		for (i = 0; i < info->dem.numb_of_em; i++) {
			pr_err("---- em %d ----\n", i);
			pr_err("clu  freq  index  util  vol_mv  power  cost  util_ratio freq_to_ratio\n");
			for (j = 0; j < info->dem.nr_freq; j++) {
				u32 freq = dem->ems[i].states[j].frequency;
				struct geas_perf_state *states = &dem->ems[i].states[j];
				pr_err(" %d    %u    %u    %u    %u    %u    %u    %u    %u\n",
					info->id, freq, freq >> 15, states->util, states->vol_mv,
					states->power, dyn_em_freq_to_cost(freq, i, info), states->util_ratio, dyn_em_freq_to_ratio(freq, i, info));
			}
			pr_err("    clu util util_to_cost freq ps_index target_load\n");
			for (util = 0; util < 1024; util++) {
				pr_err("    %d    %d    %u    %u    %u     %u\n",
						dem->cluster->id, util, dem->ems[i].util_to_data[util].cost, dem->ems[i].util_to_data[util].freq,
						dem->ems[i].util_to_data[util].ps_index,  dem->ems[i].util_to_data[util].target_load);
			}
		}
	}
}

int update_pipline_debug_para(const struct ctl_table *table, int write, void __user *buffer,  unsigned long *lenp, long long *ppos)
{
	int ret, i;
	int num = 0;
	static DEFINE_MUTEX(mutex);
	struct multitask_load_info load_info;
	struct multitask_result result;

	mutex_lock(&mutex);
	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write) {
		int ret_val;
		load_info.cpu_mask.ec = pipline_debug_para_list[0] & 0xff;
		load_info.cpu_mask.ex = (pipline_debug_para_list[0] >> 8) & 0xff;
		load_info.cpu_mask.pf = (pipline_debug_para_list[0] >> 16) & 0xff;
		load_info.cpu_mask.pt = (pipline_debug_para_list[0] >> 24) & 0xff;
		num = 2;
		for (i = 0; i < GEAS_SCHED_CPU_NR; i++)
			load_info.pre_freq[i] = pipline_debug_para_list[num++];
		for (i = 0; i < GEAS_SCHED_CPU_NR; i++)
			load_info.pre_cpu_load[i] = pipline_debug_para_list[num++];
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			load_info.prev_cpu[i] = pipline_debug_para_list[num++];
		load_info.last_pipline = pipline_debug_para_list[num++];
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			load_info.task_util[i] = pipline_debug_para_list[num++];
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			load_info.task_clamp_min[i] = pipline_debug_para_list[num++];
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			load_info.task_clamp_max[i] = pipline_debug_para_list[num++];

		for (i = 0; i < GEAS_SCHED_CLUSTER_NR; i++)
			load_info.soft_freq_max[i] = pipline_debug_para_list[num++];
		for (i = 0; i < GEAS_SCHED_CLUSTER_NR; i++)
			load_info.soft_freq_min[i] = pipline_debug_para_list[num++];
		if (GEAS_PRINT_ENABLE)
			pr_err("%s %d num %d last_pipline %u cpu_mask %x/%x/%x/%x  prev_cpu %u/%u/%u prev_freq %u/%u/%u/%u/%u/%u/%u/%u cpu_load %u/%u/%u/%u/%u/%u/%u/%u",
				__func__, __LINE__, num, load_info.last_pipline,
				load_info.cpu_mask.ec, load_info.cpu_mask.ex, load_info.cpu_mask.pf, load_info.cpu_mask.pt,
				load_info.prev_cpu[0], load_info.prev_cpu[1], load_info.prev_cpu[2],
				load_info.pre_freq[0], load_info.pre_freq[1], load_info.pre_freq[2], load_info.pre_freq[3],
				load_info.pre_freq[4], load_info.pre_freq[5], load_info.pre_freq[6], load_info.pre_freq[7],
				load_info.pre_cpu_load[0], load_info.pre_cpu_load[1], load_info.pre_cpu_load[2], load_info.pre_cpu_load[3],
				load_info.pre_cpu_load[4], load_info.pre_cpu_load[5], load_info.pre_cpu_load[6], load_info.pre_cpu_load[7]);

		ret_val = geas_update_load_info(&load_info);
		num += CLUSTER_NUM;
		for (i = 0; i < MAX_PIPLINE_TASK; i++)
			pipline_debug_para_list[num++] = result.dst_cpu[i];

		pipline_debug_para_list[num++] = ret_val;
	} else {
		if (GEAS_PRINT_ENABLE)
			show_dyn_em();
	}
	mutex_unlock(&mutex);

	return ret;
}


