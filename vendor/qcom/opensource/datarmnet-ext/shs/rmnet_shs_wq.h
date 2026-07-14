/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_SHS_WQ_H_
#define _RMNET_SHS_WQ_H_

#include "rmnet_shs_config.h"
#include "rmnet_shs.h"
#include <linux/ktime.h>

#define RMNET_SHS_DEBUG  0
#define RMNET_SHS_DEBUG1 0

#define rm_err(fmt, ...)  \
	do { if (RMNET_SHS_DEBUG) pr_err(fmt, __VA_ARGS__); } while (0)

#define rm_err1(fmt, ...)  \
	do { if (RMNET_SHS_DEBUG1) pr_err(fmt, __VA_ARGS__); } while (0)

#define MAX_SUPPORTED_FLOWS_DEBUG 32
#define RMNET_SHS_RX_BPNSEC_TO_BPSEC(x) ((x)*1000000000)
#define RMNET_SHS_SEC_TO_NSEC(x) ((x)*1000000000)
#define RMNET_SHS_NSEC_TO_SEC(x) ((x)/1000000000)
#define RMNET_SHS_BYTE_TO_BIT(x) ((x)*8)
#define RMNET_SHS_MSEC_TO_NSC(x) ((x)*1000000  )

#define RMNET_SHS_MIN_HSTAT_NODES_REQD 16
#define RMNET_SHS_WQ_INTERVAL_MS  100

#define RMNET_SHS_WQ_SUGG_MAX RMNET_SHS_WQ_SUGG_LL_PHY_CORE

struct rmnet_shs_wq_hstat_s {
	unsigned long int rmnet_shs_wq_suggs[RMNET_SHS_WQ_SUGG_MAX];
	struct list_head cpu_node_id;
	struct list_head hstat_node_id;
	struct rmnet_shs_skbn_s *node; //back pointer to node
	struct rcu_head rcu; /* Used to free struct*/
	ktime_t c_epoch; /*current epoch*/
	ktime_t l_epoch; /*last hash update epoch*/
	ktime_t inactive_duration;
	u64 rx_skb;
	u64 rx_bytes;
	u64 rx_coal_skb;
	u64 rx_pps; /*pkts per second*/
	u64 rx_bps; /*bits per second*/
	u64 last_pps;
	u64 avg_pps;
	u64 avg_segs;
	u64 hw_coal_bytes_diff; /* diff of coalescing bytes in HW */
	u64 hw_coal_bufsize_diff; /* diff of coalescing buffer size from HW */
	u64 last_hw_coal_bytes;
	u64 last_hw_coal_bufsize;
	u64 hw_coal_bytes;
	u64 hw_coal_bufsize;
	u64 last_rx_skb;
	u64 last_rx_coal_skb;
	u64 last_rx_bytes;
	u32 last_rx_ll_skb;
	u32 rx_ll_skb;
	u32 rps_config_msk; /*configured rps mask for net device*/
	u32 current_core_msk; /*mask where the current core's bit is set*/
	u32 def_core_msk; /*(little cluster) avaialble core mask*/
	u32 pri_core_msk; /* priority cores availability mask*/
	u32 available_core_msk; /* other available cores for this flow*/
	u32 hash; /*skb hash*/
	u32 bif; /* Bytes in flight */
	u32 ack_thresh; /* Quick ack threshold */
	int stat_idx; /*internal used for datatop*/
	u16 suggested_cpu; /* recommended CPU to stamp pkts*/
	u16 current_cpu; /* core where the flow is being processed*/
	u16 skb_tport_proto;
	u8 low_latency;
	u8 ll_diff;
	u8 mux_id;
	u8 in_use;
	u8 is_perm;
	u8 is_new_flow;
	u8 segs_per_skb; /* segments per skb */
};

struct rmnet_shs_wq_cpu_rx_pkt_q_s {
	struct list_head hstat_id;
	ktime_t l_epoch; /*last epoch update for this structure*/
	u64 last_rx_skbs;
	u64 last_rx_bytes;
	u64 last_rx_segs;
	u64 rx_skbs;
	u64 rx_bytes;
	u64 rx_segs;
	u64 rx_pps; /* pkts per second*/
	u64 rx_bps; /*bits per second*/
	u64 last_rx_pps; /* pkts per second*/
	u64 last_rx_bps; /* bits per second*/
	u64 avg_pps;
	u64 rx_bps_est; /*estimated bits per second*/
	u32 qhead;          /* queue head */
	u32 last_qhead;     /* last queue head */
	u32 qhead_diff; /* diff in pp in last tick*/
	u32 qhead_start; /* start mark of total pp*/
	u32 qhead_total; /* end mark of total pp*/
	int flows;
	u16 cpu_num;
};

struct rmnet_shs_wq_rx_flow_s {
	struct rmnet_shs_wq_cpu_rx_pkt_q_s cpu_list[MAX_CPUS];
	ktime_t l_epoch; /*last epoch update for this flow*/
	u64 dl_mrk_last_rx_bytes;
	u64 dl_mrk_last_rx_pkts;
	u64 dl_mrk_rx_bytes; /*rx bytes as observed in DL marker*/
	u64 dl_mrk_rx_pkts; /*rx pkts as observed in DL marker*/
	u64 dl_mrk_rx_pps; /*rx pkts per sec as observed in DL marker*/
	u64 dl_mrk_rx_bps; /*rx bits per sec as observed in DL marker*/
	u64 last_rx_skbs;
	u64 last_rx_bytes;
	u64 last_rx_segs;
	u64 last_rx_pps; /*rx pkts per sec*/
	u64 last_rx_bps; /*rx bits per sec*/
	u64 rx_skbs;
	u64 rx_bytes;
	u64 rx_segs;
	u64 rx_pps; /*rx pkts per sec*/
	u64 rx_bps; /*rx bits per sec*/
	u64 hw_coal_bytes_diff; /* diff of coalescing bytes in HW */
	u64 hw_coal_bufsize_diff; /* diff of coalescing buffer size from HW */
	u32 rps_config_msk; /*configured rps mask for net device*/
	u32 def_core_msk; /*(little cluster) avaialble core mask*/
	u32 pri_core_msk; /* priority cores availability mask*/
	u32 available_core_msk; /* other available cores for this flow*/
	int  new_lo_core[MAX_CPUS];
	int  new_hi_core[MAX_CPUS];
	int  new_lo_idx;
	int  new_hi_idx;
	int  new_lo_max;
	int  new_hi_max;
	int flows;
	u8 cpus;
};

struct rmnet_shs_delay_wq_s {
	struct delayed_work wq;
};

extern struct rmnet_shs_cpu_node_s rmnet_shs_cpu_node_tbl[MAX_CPUS];


/* Tracing Definitions */
enum rmnet_shs_wq_trace_func {
	RMNET_SHS_WQ_INIT,
	RMNET_SHS_WQ_PROCESS_WQ,
	RMNET_SHS_WQ_EXIT,
	RMNET_SHS_WQ_EP_TBL,
	RMNET_SHS_WQ_HSTAT_TBL,
	RMNET_SHS_WQ_CPU_HSTAT_TBL,
	RMNET_SHS_WQ_FLOW_STATS,
	RMNET_SHS_WQ_CPU_STATS,
	RMNET_SHS_WQ_TOTAL_STATS,
	RMNET_SHS_WQ_SHSUSR,
};

enum rmnet_shs_wq_trace_evt {
	RMNET_SHS_WQ_EP_TBL_START,
	RMNET_SHS_WQ_EP_TBL_ADD,
	RMNET_SHS_WQ_EP_TBL_DEL,
	RMNET_SHS_WQ_EP_TBL_CLEANUP,
	RMNET_SHS_WQ_EP_TBL_INIT,
	RMNET_SHS_WQ_EP_TBL_END,
	RMNET_SHS_WQ_HSTAT_TBL_START,
	RMNET_SHS_WQ_HSTAT_TBL_ADD,
	RMNET_SHS_WQ_HSTAT_TBL_DEL,
	RMNET_SHS_WQ_HSTAT_TBL_NODE_RESET,
	RMNET_SHS_WQ_HSTAT_TBL_NODE_NEW_REQ,
	RMNET_SHS_WQ_HSTAT_TBL_NODE_REUSE,
	RMNET_SHS_WQ_HSTAT_TBL_NODE_DYN_ALLOCATE,
	RMNET_SHS_WQ_HSTAT_TBL_END,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_START,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_INIT,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_ADD,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_MOVE,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_DEL,
	RMNET_SHS_WQ_CPU_HSTAT_TBL_END,
	RMNET_SHS_WQ_FLOW_STATS_START,
	RMNET_SHS_WQ_FLOW_STATS_UPDATE_MSK,
	RMNET_SHS_WQ_FLOW_STATS_UPDATE_NEW_CPU,
	RMNET_SHS_WQ_FLOW_STATS_SUGGEST_NEW_CPU,
	RMNET_SHS_WQ_FLOW_STATS_ERR,
	RMNET_SHS_WQ_FLOW_STATS_FLOW_INACTIVE,
	RMNET_SHS_WQ_FLOW_STATS_FLOW_INACTIVE_TIMEOUT,
	RMNET_SHS_WQ_FLOW_STATS_END,
	RMNET_SHS_WQ_CPU_STATS_START,
	RMNET_SHS_WQ_CPU_STATS_CURRENT_UTIL,
	RMNET_SHS_WQ_CPU_STATS_INC_CPU_FLOW,
	RMNET_SHS_WQ_CPU_STATS_DEC_CPU_FLOW,
	RMNET_SHS_WQ_CPU_STATS_GET_CPU_FLOW,
	RMNET_SHS_WQ_CPU_STATS_GET_MAX_CPU_FLOW,
	RMNET_SHS_WQ_CPU_STATS_MAX_FLOW_IN_CLUSTER,
	RMNET_SHS_WQ_CPU_STATS_UPDATE,
	RMNET_SHS_WQ_CPU_STATS_CORE2SWITCH_START,
	RMNET_SHS_WQ_CPU_STATS_CORE2SWITCH_FIND,
	RMNET_SHS_WQ_CPU_STATS_CORE2SWITCH_EVAL_CPU,
	RMNET_SHS_WQ_CPU_STATS_CORE2SWITCH_END,
	RMNET_SHS_WQ_CPU_STATS_NEW_FLOW_LIST_LO,
	RMNET_SHS_WQ_CPU_STATS_NEW_FLOW_LIST_HI,
	RMNET_SHS_WQ_CPU_STATS_END,
	RMNET_SHS_WQ_TOTAL_STATS_START,
	RMNET_SHS_WQ_TOTAL_STATS_UPDATE,
	RMNET_SHS_WQ_TOTAL_STATS_END,
	RMNET_SHS_WQ_PROCESS_WQ_START,
	RMNET_SHS_WQ_PROCESS_WQ_END,
	RMNET_SHS_WQ_PROCESS_WQ_ERR,
	RMNET_SHS_WQ_INIT_START,
	RMNET_SHS_WQ_INIT_END,
	RMNET_SHS_WQ_EXIT_START,
	RMNET_SHS_WQ_EXIT_END,
	RMNET_SHS_WQ_TRY_PASS,
	RMNET_SHS_WQ_TRY_FAIL,
	RMNET_SHS_WQ_SHSUSR_SYNC_START,
	RMNET_SHS_WQ_SHSUSR_SYNC_END,
	RMNET_SHS_WQ_FLOW_STATS_SET_FLOW_SEGMENTATION,
	RMNET_SHS_WQ_FLOW_SEG_SET_PASS,
	RMNET_SHS_WQ_FLOW_SEG_SET_FAIL,
};

void rmnet_shs_wq_init(void);
void rmnet_shs_wq_exit(void);
void rmnet_shs_wq_restart(void);
void rmnet_shs_wq_pause(void);
void rmnet_shs_wq_create_new_flow(struct rmnet_shs_skbn_s *node_p);
int rmnet_shs_wq_get_least_utilized_core(u16 core_msk);
void rmnet_shs_wq_refresh_new_flow_list(void);
int rmnet_shs_wq_try_to_move_flow(u16 cur_cpu, u16 dest_cpu, u32 hash_to_move,
				  u32 sugg_type);
int rmnet_shs_wq_set_flow_segmentation(u32 hash_to_set, u8 segs_per_skb);
int rmnet_shs_wq_set_quickack_thresh(u32 hash_to_set, u32 ack_thresh);
void rmnet_shs_ep_lock_bh(void);
void rmnet_shs_ep_unlock_bh(void);
void rmnet_shs_wq_cleanup_hash_tbl(u8 force_clean, u32 hash_to_clean);
#endif /*_RMNET_SHS_WQ_H_*/
