/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM lmh_stats

#if !defined(_TRACE_LMH_STATS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_LMH_STATS

#include <linux/tracepoint.h>

TRACE_EVENT(thermal_hw_freq_limit,

	TP_PROTO(unsigned int cpu_id, unsigned long throttled_freq),

	TP_ARGS(cpu_id, throttled_freq),

	TP_STRUCT__entry(
		__field(unsigned int, cpu_id)
		__field(unsigned long, throttled_freq)
	),

	TP_fast_assign(
		__entry->cpu_id = cpu_id;
		__entry->throttled_freq = throttled_freq;
	),

	TP_printk("cpu%u thermal limit frequency %lu KHz", __entry->cpu_id, __entry->throttled_freq)
);

#endif /* _TRACE_CLOCK_GDSC */

/* This part must be outside protection */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_lmh_stats

#include <trace/define_trace.h>
