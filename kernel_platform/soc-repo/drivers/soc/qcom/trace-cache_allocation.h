/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#if !defined(_TRACE_CACHE_ALLOC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CACHE_ALLOC_H

#undef TRACE_SYSTEM
#define TRACE_SYSTEM cache_alloc

#include <linux/tracepoint.h>

TRACE_EVENT(cache_alloc_cpu_update,

	TP_PROTO(int cluster_num, s32 freq_prev, s32 freq_curr),

	TP_ARGS(cluster_num, freq_prev, freq_curr),

	TP_STRUCT__entry(
		__field(int, cluster_num)
		__field(s32, freq_prev)
		__field(s32, freq_curr)
	),

	TP_fast_assign(
		__entry->cluster_num = cluster_num;
		__entry->freq_prev = freq_prev;
		__entry->freq_curr = freq_curr;
	),

	TP_printk("cluster%d; cpu_freq(prev, curr)=%d, %d",
		__entry->cluster_num,
		__entry->freq_prev,
		__entry->freq_curr)
);

TRACE_EVENT(cache_alloc_gpu_update,

	TP_PROTO(unsigned long gpu_freq_prev, unsigned long gpu_freq_curr),

	TP_ARGS(gpu_freq_prev, gpu_freq_curr),

	TP_STRUCT__entry(
		__field(unsigned long, gpu_freq_prev)
		__field(unsigned long, gpu_freq_curr)
	),

	TP_fast_assign(
		__entry->gpu_freq_prev = gpu_freq_prev;
		__entry->gpu_freq_curr = gpu_freq_curr;
	),

	TP_printk("gpu_freq(prev, curr)=%lu, %lu",
		__entry->gpu_freq_prev,
		__entry->gpu_freq_curr)
);

TRACE_EVENT(cache_alloc_config_update,

	TP_PROTO(int config1, int config2),

	TP_ARGS(config1, config2),

	TP_STRUCT__entry(
		__field(int, config1)
		__field(int, config2)
	),

	TP_fast_assign(
		__entry->config1 = config1;
		__entry->config2 = config2;
	),

	TP_printk("set config, config1=%d, config2=%d",
		__entry->config1,
		__entry->config2)
);

TRACE_EVENT(cache_alloc_config_check,

	TP_PROTO(int gov, int config1, int config2),

	TP_ARGS(gov, config1, config2),

	TP_STRUCT__entry(
		__field(int, gov)
		__field(int, config1)
		__field(int, config2)
	),

	TP_fast_assign(
		__entry->gov = gov;
		__entry->config1 = config1;
		__entry->config2 = config2;
	),

	TP_printk("check config, gov=%d, config1=%d, config2=%d",
		__entry->gov,
		__entry->config1,
		__entry->config2)
);

TRACE_EVENT(cache_alloc_timestamp,

	TP_PROTO(u64 timestamp, int idx),

	TP_ARGS(timestamp, idx),

	TP_STRUCT__entry(
		__field(u64, timestamp)
		__field(int, idx)
	),

	TP_fast_assign(
		__entry->timestamp = timestamp;
		__entry->idx = idx;
	),

	TP_printk("timestamp=%llu idx=%d",
		__entry->timestamp,
		__entry->idx)
);

TRACE_EVENT(cache_alloc_byte_cnt,

	TP_PROTO(u64 byte_cnt, int idx),

	TP_ARGS(byte_cnt, idx),

	TP_STRUCT__entry(
		__field(u64, byte_cnt)
		__field(int, idx)
	),

	TP_fast_assign(
		__entry->byte_cnt = byte_cnt;
		__entry->idx = idx;
	),

	TP_printk("byte_cnt=%llu idx=%d",
		__entry->byte_cnt,
		__entry->idx)
);

TRACE_EVENT(cache_alloc_bw_ratio,

	TP_PROTO(u64 m_bw, u64 l_bw, u64 cpu_bw, u64 gpu_bw, u64 bw_ratio),

	TP_ARGS(m_bw, l_bw, cpu_bw, gpu_bw, bw_ratio),

	TP_STRUCT__entry(
		__field(u64, m_bw)
		__field(u64, l_bw)
		__field(u64, cpu_bw)
		__field(u64, gpu_bw)
		__field(u64, bw_ratio)
	),

	TP_fast_assign(
		__entry->m_bw = m_bw;
		__entry->l_bw = l_bw;
		__entry->cpu_bw = cpu_bw;
		__entry->gpu_bw = gpu_bw;
		__entry->bw_ratio = bw_ratio;
	),

	TP_printk("cpu_m_bw=%llu, cpu_l_bw=%llu, cpu_bw=%llu, gpu_bw=%llu, bw_ratio=%llu",
		__entry->m_bw,
		__entry->l_bw,
		__entry->cpu_bw,
		__entry->gpu_bw,
		__entry->bw_ratio)
);

#endif /* _TRACE_CACHE_ALLOC_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace-cache_allocation

#include <trace/define_trace.h>
