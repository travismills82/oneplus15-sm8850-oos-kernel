/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */
#ifndef _HMBIRD_KFUNC_H_
#define _HMBIRD_KFUNC_H_

#define LK_PROTECT_ENABLE	(1 << 5)

void hmbird_kfunc_register(void);
void pre_hmbird_kfunc_register(void);

void locking_state_systrace_c(unsigned int cpu, struct task_struct *p);
extern unsigned int g_opt_enable;
extern atomic_t __hb_ops_enabled;

#define TASK_UNKNOWN_CLASS	0
#define TASK_STOP_CLASS		1
#define TASK_DL_CLASS		2
#define TASK_RT_CLASS		3
#define TASK_FAIR_CLASS		4
#define TASK_EXT_CLASS		5
#define TASK_IDLE_CLASS		6
extern struct sched_class *addr_stop_sched_class;
extern struct sched_class *addr_dl_sched_class;
extern struct sched_class *addr_rt_sched_class;
extern struct sched_class *addr_fair_sched_class;
extern struct sched_class *addr_ext_sched_class;

#endif /* _HMBIRD_KFUNC_H_ */
