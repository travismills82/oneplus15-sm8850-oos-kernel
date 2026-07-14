/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 Oplus. All rights reserved.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM freqgov

#if !defined(_TRACE_FREQGOV_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_FREQGOV_H

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

TRACE_EVENT(hmbird_freq_update,

	TP_PROTO(int cpu, unsigned int flag),

	TP_ARGS(cpu, flag),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(unsigned int, flag)),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->flag = flag;),

	TP_printk("hmbird_freq_update:cpu=%d, flag=%u",
		__entry->cpu, __entry->flag)
);

#endif /*_TRACE_FREQGOV_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ./hmbird_II

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_freqgov
/* This part must be outside protection */
#include <trace/define_trace.h>
