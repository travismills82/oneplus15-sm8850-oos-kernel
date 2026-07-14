/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM thqspi_trace

#if !defined(_TRACE_THQSPI_CNSS_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_THQSPI_CNSS_TRACE_H

#include <linux/ktime.h>
#include <linux/tracepoint.h>
#include <linux/version.h>
#define MAX_MSG_LEN 256

TRACE_EVENT(thqspi_log_info,
			TP_PROTO(const char *name, struct va_format *vaf),
			TP_ARGS(name, vaf),
			TP_STRUCT__entry(__string(name, name)
			__dynamic_array(char, msg, MAX_MSG_LEN)),
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
			TP_fast_assign(__assign_str(name);
			#else
		        TP_fast_assign(__assign_str(name, name);
			#endif
			WARN_ON_ONCE(vsnprintf(__get_dynamic_array(msg), MAX_MSG_LEN,
			vaf->fmt, *vaf->va) >= MAX_MSG_LEN);),
			TP_printk("%s: %s", __get_str(name), __get_str(msg))
);

#endif //_TRACE_THQSPI_CNSS_TRACE_H

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE thqspi_trace
#include <trace/define_trace.h>
