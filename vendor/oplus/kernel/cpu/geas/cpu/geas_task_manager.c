// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas_task_manager: " fmt
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
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "trace_geas.h"
#include "sharebuck.h"
#include "pipline_eas.h"

unsigned int multitask_pid_list[MAX_PIPLINE_TASK * 3];
unsigned int multitask_inner_triger_same_count = 2;

struct multitask_key_thread_info key_thread_info;
static char *multitask_eas_debug_info_buffer;
#define MAX_FP_DUMP_BUFFER 4096

static inline int is_em_type_avaliable(u16 em_type)
{
	int type;
	u8 val;
	for (type = 0; type < EM_TYPE_CLUSTER_BUTTON; type++) {
		val = em_type >> (type * EM_TYPE_WIDTH);
		val = val & ((1 << EM_TYPE_WIDTH) - 1);
		if (val >= MAX_EM_NUM) {
			pr_err("%s err em_type %x is invalid type %d val %x", __func__, em_type, type, val);
			return -1;
		}
	}

	return 0;
}

int geas_add_task_to_group(int tid, u8 exclusive, u16 em_type)
{
	int ret = 0;
	ret = is_em_type_avaliable(em_type);
	if (ret < 0)
		goto out;

	struct geas_task_struct *geas_task = NULL;
	if (key_thread_info.thread_num >= MAX_PIPLINE_TASK) {
		pr_err("multitask_add_task_to_group add exceed %d", key_thread_info.thread_num);
		goto out;
	}
	geas_task = &key_thread_info.task_list[key_thread_info.thread_num];
	geas_task->pid = tid;
	geas_task->exclusive_flag = exclusive;
	geas_task->em_type = em_type;

	key_thread_info.thread_num++;

out:
	if (GEAS_PRINT_ENABLE || (ret < 0))
		pr_err("%s success %d tid %d exclusive %u em_type 0x%x thread_num %d\n",
			__func__, ret, tid, exclusive, em_type, key_thread_info.thread_num);

	return ret;
}

void geas_add_task_to_group_debug(int num)
{
	int i;
	key_thread_info.thread_num = 0;

	mutex_lock(&pipline_mutex);
	for (i = 0; i < num * 3; i = i + 3) {
		int tid = multitask_pid_list[i];
		int exclusive = multitask_pid_list[i + 1];
		int em_type = multitask_pid_list[i + 2];
		if (tid > 0)
			geas_add_task_to_group(tid, (u8)exclusive, (u16)em_type);
	}
	mutex_unlock(&pipline_mutex);
}

int update_multitask_ctrl_values(const struct ctl_table *table, int write,
						void __user *buffer,  unsigned long *lenp, long long *ppos)
{
	int ret;
	int num;
	static DEFINE_MUTEX(mutex);

	mutex_lock(&mutex);
	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write) {
		num = table->maxlen / (sizeof(unsigned int) * 3);
		num = min(num, MAX_PIPLINE_TASK);
		pr_err("table->maxlen %d len %d num %d\n", table->maxlen, (int)*lenp, num);
		geas_add_task_to_group_debug(num);
	}
	mutex_unlock(&mutex);
	return ret;
}

static ssize_t dump_multitask_eas_fp_debug_table(char * buf, size_t size)
{
	ssize_t ret = 0;
	int pipline_select_ratio[MAX_CPU_MAP_NUM];
	u64 sum = 0;
	int i;

	if (unlikely(!geas_cpu_inited || !geas_cpu_enable))
		return 0;

	ret += scnprintf(buf + ret, size - ret, "thread_num %d def_em_type %u pipline_cal_mask %u freq_policy_type %u\n",
		key_thread_info.thread_num, key_thread_info.def_em_type,
		key_thread_info.pipline_cal_mask, key_thread_info.freq_policy_type);

	for (i = 0; i < min(key_thread_info.thread_num, MAX_PIPLINE_TASK); i++) {
		struct geas_task_struct *geas_task = &key_thread_info.task_list[i];
		ret += scnprintf(buf + ret, size - ret,
				"comm=%-16s  pid=%-6d exclusive_flag=%d em_type 0x%x\n",
				geas_task->comm, geas_task->pid, geas_task->exclusive_flag, geas_task->em_type);
	}

	for (i = 0; i < MAX_CPU_MAP_NUM; i++) {
		pipline_select_ratio[i] = 0;
		sum+= pipline_select_cnt[i];
	}

	if (sum) {
		for (i = 0; i < MAX_CPU_MAP_NUM; i++) {
			pipline_select_ratio[i] = pipline_select_cnt[i] * 10000 / sum;
		}
	}

	ret += scnprintf(buf + ret, size - ret,
		"cnt %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu \n"
		"ratio %d %d %d %d %d %d %d %d %d %d\n",
		pipline_select_cnt[0], pipline_select_cnt[1], pipline_select_cnt[2], pipline_select_cnt[3], pipline_select_cnt[4],
		pipline_select_cnt[5], pipline_select_cnt[6], pipline_select_cnt[7], pipline_select_cnt[8], pipline_select_cnt[9],
		pipline_select_ratio[0], pipline_select_ratio[1], pipline_select_ratio[2], pipline_select_ratio[3], pipline_select_ratio[4],
		pipline_select_ratio[5], pipline_select_ratio[6], pipline_select_ratio[7], pipline_select_ratio[8], pipline_select_ratio[9]);
	for (i = 0; i < MAX_CPU_MAP_NUM; i++)
		pipline_select_cnt[i] = 0;

	return ret;
}

static int multitask_fp_dump_show_inner(const struct ctl_table *ro_table, int write,
			void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table table;

	if (write)
		return 0;

	memset(multitask_eas_debug_info_buffer, 0, sizeof(char) * MAX_FP_DUMP_BUFFER);
	if (!dump_multitask_eas_fp_debug_table(multitask_eas_debug_info_buffer, sizeof(char) * MAX_FP_DUMP_BUFFER))
		return -EINVAL;

	table = *ro_table;
	table.data = multitask_eas_debug_info_buffer;
	table.maxlen = sizeof(char) * MAX_FP_DUMP_BUFFER;
	return proc_dostring(&table, 0, buffer, lenp, ppos);
}

int multitask_eas_fp_dump(const struct ctl_table *ro_table, int write,
		void *buffer,  unsigned long *lenp, long long *ppos)
{
	int ret;
	static DEFINE_MUTEX(mutex);

	mutex_lock(&mutex);
	ret = multitask_fp_dump_show_inner(ro_table, write, buffer, lenp, ppos);
	mutex_unlock(&mutex);
	return ret;
}

int task_manager_init(void)
{
	multitask_eas_debug_info_buffer = kcalloc(MAX_FP_DUMP_BUFFER, sizeof(char), GFP_KERNEL);
	if (!multitask_eas_debug_info_buffer) {
		pr_err("mutitask_eas_init alloc multitask_eas_debug_info_buffer failed \n");
		return -ENOMEM;
	}

	key_thread_info.thread_num = 0;
	key_thread_info.def_em_type = 0;
	key_thread_info.freq_policy_type = FREQ_POLICY_MAX;
	key_thread_info.pipline_cal_mask = 0xff;

	pr_info("task_manager_initt done");
	return 0;
}

