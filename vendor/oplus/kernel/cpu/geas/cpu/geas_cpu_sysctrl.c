// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas-sysctrl: " fmt

#include <linux/sched.h>
#include <linux/sysfs.h>

#include <linux/errno.h>
#include <linux/kmemleak.h>
#include <linux/energy_model.h>
#include "../../../kernel/sched/sched.h"
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "geas_task_manager.h"
#include "geas_dyn_em.h"
#include "sharebuck.h"
#include "pipline_eas.h"
#include "geas_cpu_debug.h"
#include "trace_geas.h"

struct ctl_table geas_sched_table[] = {
	{
		.procname	= "geas_cpu_inited",
		.data		= &geas_cpu_inited,
		.maxlen		= sizeof(int),
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_ONE,
	},
	{
		.procname	= "geas_targetload",
		.data		= &geas_targetload,
		.maxlen 	= sizeof(int) * GEAS_SCHED_CLUSTER_NR,
		.mode		= 0664,
		.proc_handler	= update_targetload_ctrl_values,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},
	{
		.procname	= "multitask_pid_list",
		.data		= &multitask_pid_list,
		.maxlen 	= sizeof(unsigned int) * MAX_PIPLINE_TASK * 3,
		.mode		= 0664,
		.proc_handler	= update_multitask_ctrl_values,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},
	{
		.procname	= "pipline_debug_para_list",
		.data		= &pipline_debug_para_list,
		.maxlen 	= sizeof(unsigned int) * MAX_PIPLINE_DEBUG_PARA_LEN,
		.mode		= 0664,
		.proc_handler	= update_pipline_debug_para,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},
	{
		.procname	= "geas_cpu_enable",
		.data		= &geas_cpu_enable,
		.maxlen 	= sizeof(unsigned int),
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},

	{
		.procname	= "pipline_debug_mask",
		.data		= &pipline_debug_mask,
		.maxlen 	= sizeof(unsigned int),
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},
	{
		.procname	= "pipline_mem_debug_ratio",
		.data		= &pipline_mem_debug_ratio,
		.maxlen 	= sizeof(unsigned int) * MAX_CPU_MAP_NUM,
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},
	{
		.procname	= "pipline_debug_ratio_1",
		.data		= &pipline_debug_ratio_1,
		.maxlen 	= sizeof(unsigned int) * MAX_CPU_MAP_NUM,
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},

	{
		.procname	= "multitask_inner_triger_same_count",
		.data		= &multitask_inner_triger_same_count,
		.maxlen 	= sizeof(unsigned int),
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1 	= SYSCTL_ZERO,
		.extra2 	= SYSCTL_INT_MAX,
	},

	{
		.procname	= "multitask_eas_fp_dump",
		.mode		= 0664,
		.proc_handler	= multitask_eas_fp_dump,
	},

	{
		.procname	= "geas_cpu_dbg_enabled",
		.data		= &geas_cpu_dbg_enabled,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0664,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_INT_MAX,
	}
};
int geas_sysctrl_init(void)
{
	struct ctl_table_header *hdr;

	hdr = register_sysctl("geas/cpu", geas_sched_table);
	kmemleak_not_leak(hdr);
	if (!hdr) {
		pr_err("%s faild register sysctl geas/cpu\n", __func__);
		return -1;
	}
	return 0;
}

