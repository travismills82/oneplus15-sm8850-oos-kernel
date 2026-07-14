/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020-2024 Oplus. All rights reserved.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM cputime

#undef TRACE_INCLUDE_PATH
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
#define TRACE_INCLUDE_PATH ../../vendor/oplus/kernel/cpu/sched/sched_assist
#else
#define TRACE_INCLUDE_PATH ../../kernel_device_modules-6.12/kernel/oplus_cpu/sched/sched_assist
#endif

#if !defined(_TRACE_HOOK_CPUTIME_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_CPUTIME_H

#include <trace/hooks/vendor_hooks.h>

DECLARE_HOOK(android_vh_account_task_time,
	TP_PROTO(struct task_struct *p, struct rq *rq, int user_tick),
	TP_ARGS(p, rq, user_tick));


#endif /* _TRACE_HOOK_CPUTIME_H */
/* This part must be outside protection */
#include <trace/define_trace.h>

