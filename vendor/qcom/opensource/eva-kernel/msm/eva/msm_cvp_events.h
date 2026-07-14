/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#if !defined(_MSM_CVP_EVENTS_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _MSM_CVP_EVENTS_H_

#include <linux/types.h>
#include <linux/tracepoint.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM msm_cvp

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE msm_cvp_events

// #define USE_PERFETTO

#ifdef USE_PERFETTO

#define cvp_trace trace_printk

#define CVPKERNEL_ATRACE_BEGIN(name) do { \
	if ((msm_cvp_debug & CVP_TRACE) == CVP_TRACE) { \
		char buf[128]; \
		snprintf(buf, 128, "B|%d|%s\n", current->tgid, name); \
		cvp_trace(buf); \
	} \
} while (0)

#define CVPKERNEL_ATRACE_END(name) do { \
	if ((msm_cvp_debug & CVP_TRACE) == CVP_TRACE) { \
		char buf[128]; \
		snprintf(buf, 128, "E|%d\n", current->tgid); \
		cvp_trace(buf); \
	} \
} while (0)

#else  // #ifdef USE_PERFETTO

// Since Chrome supports to parse the event “tracing_mark_write” by default
// so we can re-use this to display your own events in Chrome
// enable command as below:
// adb shell "echo 1 > /sys/kernel/tracing/events/msm_cvp/tracing_mark_write/enable"

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(char trace_type, const struct task_struct *task,
		const char *name, int value),
	TP_ARGS(trace_type, task, name, value),
	TP_STRUCT__entry(
			__field(char, trace_type)
			__field(int, pid)
			__string(trace_name, name)
			__field(int, value)
	),
	TP_fast_assign(
			__entry->trace_type = trace_type;
			__entry->pid = task ? task->tgid : 0;
#if (KERNEL_VERSION(6, 10, 0) <= LINUX_VERSION_CODE)
			__assign_str(trace_name);
#else
			__assign_str(trace_name, name);
#endif
			__entry->value = value;
	),
	TP_printk("%c|%d|%s|%d", __entry->trace_type,
			__entry->pid, __get_str(trace_name), __entry->value)
)
/*
#define CVPKERNEL_ATRACE_END(name) \
		trace_tracing_mark_write(current->tgid, name, 0)
#define CVPKERNEL_ATRACE_BEGIN(name) \
		trace_tracing_mark_write(current->tgid, name, 1)
*/

#define CVPKERNEL_ATRACE_END(name) trace_tracing_mark_write('E', current, name, 0)
#define CVPKERNEL_ATRACE_BEGIN(name) trace_tracing_mark_write('B', current, name, 0)

#endif  // #ifdef USE_PERFETTO

TRACE_EVENT(tracing_eva_frame_from_sw,
	TP_PROTO(u64 aon_cycles, const char *name,
	u32 session_id, u32 stream_id,
	u32 packet_id, const char *command_name, u32 transaction_id, u64 ktid),
	TP_ARGS(aon_cycles, name, session_id, stream_id, packet_id, command_name,
	transaction_id, ktid),
	TP_STRUCT__entry(
		__field(u64, aon_cycles)
		__string(trace_name, name)
		__field(u32, session_id)
		__field(u32, stream_id)
		__field(u32, packet_id)
		__string(trace_command_name, command_name)
		__field(u32, transaction_id)
		__field(u64, ktid)
	),
	TP_fast_assign(
		__entry->aon_cycles = aon_cycles;
#if KERNEL_VERSION(6, 10, 0) <= LINUX_VERSION_CODE
		__assign_str(trace_name);
		__assign_str(trace_command_name);

#else
		__assign_str(trace_name, name);
		__assign_str(trace_command_name, command_name);

#endif
		__entry->session_id = session_id;
		__entry->stream_id  = stream_id;
		__entry->packet_id  = packet_id;
		__entry->transaction_id = transaction_id;
		__entry->ktid = ktid;
	),
	TP_printk("AON_TIMESTAMP: %llu %s session_id = 0x%08x stream_id = 0x%08x packet_id = 0x%08x command_name = %s transaction_id = 0x%016llx ktid = %llu",
		__entry->aon_cycles, __get_str(trace_name),
		__entry->session_id, __entry->stream_id,
		__entry->packet_id, __get_str(trace_command_name),
		__entry->transaction_id, __entry->ktid)
)

TRACE_EVENT(tracing_eva_frame_from_fw,

	TP_PROTO(char *trace),

	TP_ARGS(trace),

	TP_STRUCT__entry(
		__string(trace_name, trace)
	),

	TP_fast_assign(
#if KERNEL_VERSION(6, 10, 0) <= LINUX_VERSION_CODE
		__assign_str(trace_name);
#else
		__assign_str(trace_name, trace);
#endif
	),

	TP_printk("%s", __get_str(trace_name))
);

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#include <trace/define_trace.h>
