// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "rmnet_shs.h"
#include "rmnet_shs_wq_genl.h"
#include "rmnet_shs_wq_mem.h"
#include <uapi/linux/rmnet_shs.h>
#include <linux/workqueue.h>
#include <linux/list_sort.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#if IS_ENABLED(CONFIG_SCHED_WALT)
#include <linux/sched/walt.h>
#endif /* CONFIG_SCHED_WALT */
#include "rmnet_shs_modules.h"
#include "rmnet_shs_common.h"
#include <linux/pm_wakeup.h>
#include "rmnet_module.h"
#if (KERNEL_VERSION(6, 6, 0) <= LINUX_VERSION_CODE)
#include <net/netdev_rx_queue.h>
#endif

MODULE_LICENSE("GPL v2");
/* Local Macros */
#define RMNET_SHS_FILTER_PKT_LIMIT 200
#define RMNET_SHS_FILTER_FLOW_RATE 100

#define PERIODIC_CLEAN 0
/* FORCE_CLEAN should only used during module de-init.*/
#define FORCE_CLEAN 1
#define MAX_RESERVE_CPU 2
#define TITANIUM_CPU1 5
#define TITANIUM_CPU2 6

#define SYNC_TIME 0x7F
#define ASYNC_LOWTHRESH 15000
#define ASYNC_UPTHRESH 34000
#define BIT_TEST(X,Y) ((1<<Y) & X)
/* Local Definitions and Declarations */
#define PRIO_BACKOFF ((!rmnet_shs_cpu_prio_dur) ? 2 : rmnet_shs_cpu_prio_dur)

#define RMNET_SHS_SEGS_PER_SKB_DEFAULT (2)

DEFINE_SPINLOCK(rmnet_shs_hstat_tbl_lock);
DEFINE_SPINLOCK(rmnet_shs_ep_lock);

static ktime_t rmnet_shs_wq_tnsec;
struct workqueue_struct *rmnet_shs_wq;
static struct rmnet_shs_delay_wq_s *rmnet_shs_delayed_wq;

struct list_head rmnet_shs_ll_hstat_tbl =
				LIST_HEAD_INIT(rmnet_shs_ll_hstat_tbl);

struct list_head rmnet_shs_wq_hstat_tbl =
				LIST_HEAD_INIT(rmnet_shs_wq_hstat_tbl);
static int rmnet_shs_flow_dbg_stats_idx_cnt;

static int is_reserved(int cpu)
{
#if IS_ENABLED(CONFIG_SCHED_WALT)
	cpumask_t rmask = walt_get_cpus_taken();
	return cpumask_test_cpu(cpu, &rmask);
#else
	return 0;
#endif /* CONFIG_SCHED_WALT */
}

static void rmnet_shs_phy_sync(void)
{
	struct rmnet_shs_msg_resp chg_msg;

	rmnet_shs_create_phy_msg_resp(&chg_msg, rmnet_shs_cfg.phy_acpu, rmnet_shs_cfg.phy_acpu);
	rmnet_shs_genl_msg_direct_send_to_userspace(&chg_msg);
}

static void rmnet_shs_get_state(void)
{
#if IS_ENABLED(CONFIG_SCHED_WALT)
	cpumask_t rmask;
	int j;
	unsigned int dest_mask = 0;

	/* Feature needs to be enabled on target for support*/
	if (!(rmnet_shs_cfg.feature_mask & TITANIUM_FEAT))
		return;

	walt_get_cpus_in_state1(&rmask);


	for (j = 0; j < MAX_CPUS; j++) {
			if (cpumask_test_cpu(j, &rmask)) {
			dest_mask |= 1 << j;
		}
	}

	if ((dest_mask & (1 << TITANIUM_CPU1)) &&
	    (dest_mask & (1 << TITANIUM_CPU2)))
	{
		if (rmnet_shs_cfg.max_s_cores != 4) {
			rmnet_shs_cfg.max_s_cores = 4;
			rmnet_shs_cfg.perf_mask = 0x9C;
			rmnet_shs_cfg.non_perf_mask = 0x63;
			rmnet_shs_cpu_rx_min_pps_thresh[TITANIUM_CPU1] = RMNET_SHS_UDP_PPS_LPWR_CPU_LTHRESH;
			rmnet_shs_cpu_rx_min_pps_thresh[TITANIUM_CPU2] = RMNET_SHS_UDP_PPS_LPWR_CPU_LTHRESH;

			rmnet_shs_cpu_rx_max_pps_thresh[TITANIUM_CPU1] = RMNET_SHS_UDP_PPS_LPWR_CPU_UTHRESH;
			rmnet_shs_cpu_rx_max_pps_thresh[TITANIUM_CPU2] = RMNET_SHS_UDP_PPS_LPWR_CPU_UTHRESH;
			trace_rmnet_shs_wq_low(RMNET_SHS_WALT, RMNET_SHS_WALT_TRANSITION,
						rmnet_shs_cfg.perf_mask, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
			rmnet_shs_switch_reason[RMNET_SHS_WALT_SWITCH1]++;
		}
	} else {
		if (rmnet_shs_cfg.max_s_cores != 2) {
			rmnet_shs_cfg.max_s_cores = 2;
			rmnet_shs_cfg.perf_mask = 0xFC;
			rmnet_shs_cfg.non_perf_mask = 0x03;

			rmnet_shs_cpu_rx_min_pps_thresh[TITANIUM_CPU1] = RMNET_SHS_UDP_PPS_PERF_CPU_LTHRESH;
			rmnet_shs_cpu_rx_min_pps_thresh[TITANIUM_CPU2] = RMNET_SHS_UDP_PPS_PERF_CPU_LTHRESH;

			rmnet_shs_cpu_rx_max_pps_thresh[TITANIUM_CPU1] = RMNET_SHS_UDP_PPS_PERF_CPU_UTHRESH;
			rmnet_shs_cpu_rx_max_pps_thresh[TITANIUM_CPU2] = RMNET_SHS_UDP_PPS_PERF_CPU_UTHRESH;
			trace_rmnet_shs_wq_low(RMNET_SHS_WALT, RMNET_SHS_WALT_TRANSITION,
						rmnet_shs_cfg.perf_mask, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
			rmnet_shs_switch_reason[RMNET_SHS_WALT_SWITCH2]++;
		}
	}
#endif /* CONFIG_SCHED_WALT */
}

static void rmnet_update_reserve_mask(void)
{
#if IS_ENABLED(CONFIG_SCHED_WALT)
	int j = 0, res_cpus = 0, update_mask = 0, new_cpu = 0;
	int new_phy = 1, aud_res_mask = 0, cluster;
	int old_mask = rmnet_shs_halt_mask;
	cpumask_t rmask = walt_get_cpus_taken();
	cpumask_t halt_mask = walt_get_halted_cpus();

	if (!rmnet_shs_reserve_on)
		return;

	for (j = 0; j < MAX_CPUS; j++) {
		if (BIT_TEST(rmnet_shs_cfg.map_mask, j) && cpumask_test_cpu(j, &halt_mask)) {
			res_cpus++;
			if (res_cpus > MAX_RESERVE_CPU) {
				rmnet_shs_crit_err[RMNET_SHS_RESERVE_LIMIT]++;
				break;
			}
			update_mask |= 1<< j;
		}
	}

	for (j = 0; j < MAX_CPUS && res_cpus < MAX_RESERVE_CPU; j++) {
		if (BIT_TEST(rmnet_shs_cfg.map_mask, j) && cpumask_test_cpu(j, &rmask)) {
			res_cpus++;
			if (res_cpus > MAX_RESERVE_CPU) {
				rmnet_shs_crit_err[RMNET_SHS_RESERVE_LIMIT]++;
				break;
			}
			aud_res_mask |= 1<< j;
			update_mask |= 1<< j;
		}
	}
	/* Halt mask is halted cpus + audio reserved cpus we are honoring up to Max reserve cpus we allow
	 * reserve_mask is a subset of halted mask that shows which audio cpus are honored.
	 */
	if (old_mask != update_mask)
		rmnet_shs_switch_reason[RMNET_SHS_HALT_MASK_CHANGE]++;

	rmnet_shs_reserve_mask = aud_res_mask;
	rmnet_shs_halt_mask = update_mask;

	/* Move to Perf core if silver phy is taken, move to another Perf core if perf phy is taken */
	if ((1 << rmnet_shs_cfg.phy_acpu) & rmnet_shs_halt_mask) {
		cluster = (rmnet_shs_cfg.feature_mask & PHY_GOLD_SWITCH_FEAT)? PERF_MASK : NONPERF_MASK;
		new_cpu = rmnet_shs_wq_get_least_utilized_core(rmnet_shs_cfg.map_mask & cluster &
							       ~rmnet_shs_cfg.ban_mask & ~rmnet_shs_halt_mask);
		if (new_cpu > 0) {
			new_phy = new_cpu;
			rmnet_shs_cfg.phy_tcpu = new_phy;
			rmnet_shs_switch_reason[RMNET_SHS_HALT_PHY]++;
			rmnet_shs_switch_enable();
		}
	}
#endif /* CONFIG_SCHED_WALT */
}

/* Resets all the parameters used to maintain hash statistics */
static void rmnet_shs_wq_hstat_reset_node(struct rmnet_shs_wq_hstat_s *hnode)
{
	hnode->c_epoch = 0;
	hnode->l_epoch = 0;
	hnode->node = NULL;
	hnode->inactive_duration = 0;
	hnode->rx_skb = 0;
	hnode->rx_coal_skb = 0;
	hnode->rx_bytes = 0;
	hnode->rx_pps = 0;
	hnode->rx_bps = 0;
	hnode->hw_coal_bytes_diff = 0;
	hnode->hw_coal_bufsize_diff = 0;
	hnode->last_hw_coal_bytes = 0;
	hnode->last_hw_coal_bufsize = 0;
	hnode->hw_coal_bytes = 0;
	hnode->hw_coal_bufsize = 0;
	hnode->last_rx_skb = 0;
	hnode->rx_ll_skb = 0;
	hnode->last_rx_ll_skb = 0;
	hnode->ll_diff = 0;
	hnode->low_latency = 0;
	hnode->last_rx_bytes = 0;
	hnode->rps_config_msk = 0;
	hnode->current_core_msk = 0;
	hnode->def_core_msk = 0;
	hnode->pri_core_msk = 0;
	hnode->available_core_msk = 0;
	hnode->hash = 0;
	hnode->suggested_cpu = 0;
	hnode->current_cpu = 0;
	hnode->segs_per_skb = 0;
	hnode->skb_tport_proto = 0;
	hnode->stat_idx = (-1);
	hnode->bif = 0;
	hnode->ack_thresh = 0;
	INIT_LIST_HEAD(&hnode->cpu_node_id);
	hnode->is_new_flow = 0;
	/* clear in use flag as a last action. This is required to ensure
	 * the same node does not get allocated until all the paramaeters
	 * are cleared.
	 */
	hnode->in_use = 0;
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_HSTAT_TBL,
			    RMNET_SHS_WQ_HSTAT_TBL_NODE_RESET,
			    hnode->is_perm, 0xDEF, 0xDEF, 0xDEF, hnode, NULL);
}

/* If there is an already pre-allocated node available and not in use,
 * we will try to re-use them.
 */
struct rmnet_shs_wq_hstat_s *rmnet_shs_wq_get_new_hstat_node(void)
{
	struct rmnet_shs_wq_hstat_s *hnode = NULL;
	struct rmnet_shs_wq_hstat_s *ret_node = NULL;


	/* We have reached a point where all pre-allocated nodes are in use
	 * Allocating memory to maintain the flow level stats for new flow.
	 * However, this newly allocated memory will be released as soon as we
	 * realize that this flow is inactive
	 */
	ret_node = kzalloc(sizeof(*hnode), GFP_ATOMIC);

	if (!ret_node) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_ALLOC_HSTAT_ERR]++;
		return NULL;
	}

	rmnet_shs_wq_hstat_reset_node(ret_node);
	ret_node->is_perm = 0;
	ret_node->in_use = 1;
	ret_node->is_new_flow = 1;
	INIT_LIST_HEAD(&ret_node->hstat_node_id);
	INIT_LIST_HEAD(&ret_node->cpu_node_id);

	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_HSTAT_TBL,
			    RMNET_SHS_WQ_HSTAT_TBL_NODE_DYN_ALLOCATE,
			    ret_node->is_perm, 0xDEF, 0xDEF, 0xDEF,
			    ret_node, NULL);

	rmnet_shs_hstat_tbl_add(ret_node);

	return ret_node;
}

void rmnet_shs_wq_create_new_flow(struct rmnet_shs_skbn_s *node_p)
{
	struct timespec64 time;

	if (!node_p) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return;
	}

	node_p->hstats = rmnet_shs_wq_get_new_hstat_node();
	if (node_p->hstats != NULL) {
		(void)ktime_get_boottime_ts64(&time);

		node_p->hstats->hash = node_p->hash;
		node_p->hstats->skb_tport_proto = node_p->skb_tport_proto;
		node_p->hstats->current_cpu = node_p->map_cpu;
		node_p->hstats->suggested_cpu = node_p->map_cpu;
		/* Default 0 for segmentation */
		node_p->hstats->segs_per_skb = 0;

		/* Start TCP flows with segmentation if userspace connected */
		if (rmnet_shs_userspace_connected &&
		    node_p->hstats->skb_tport_proto == IPPROTO_TCP)
			node_p->hstats->segs_per_skb = RMNET_SHS_SEGS_PER_SKB_DEFAULT;

		node_p->hstats->node = node_p;
		node_p->hstats->c_epoch = RMNET_SHS_SEC_TO_NSEC(time.tv_sec) +
		   time.tv_nsec;
		node_p->hstats->l_epoch = RMNET_SHS_SEC_TO_NSEC(time.tv_sec) +
		   time.tv_nsec;
	}

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_HSTAT_TBL,
				RMNET_SHS_WQ_HSTAT_TBL_NODE_NEW_REQ,
				0xDEF, 0xDEF, 0xDEF, 0xDEF,
				node_p, node_p->hstats);
}

/* Compute the average pps for a flow based on tuning param
 * Often when we decide to switch from a small cluster core,
 * it is because of the heavy traffic on that core. In such
 * circumstances, we want to switch to a big cluster
 * core as soon as possible. Therefore, we will provide a
 * greater weightage to the most recent sample compared to
 * the previous samples.
 *
 * On the other hand, when a flow which is on a big cluster
 * cpu suddenly starts to receive low traffic we move to a
 * small cluster core after observing low traffic for some
 * more samples. This approach avoids switching back and forth
 * to small cluster cpus due to momentary decrease in data
 * traffic.
 */
static u64 rmnet_shs_wq_get_flow_avg_pps(struct rmnet_shs_wq_hstat_s *hnode)
{
	u64 avg_pps, mov_avg_pps;
	u16 new_weight, old_weight;

	if (!hnode) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return 0;
	}

	old_weight = rmnet_shs_wq_tuning;
	new_weight = 100 - rmnet_shs_wq_tuning;

	/* computing weighted average per flow, if the flow has just started,
	 * there is no past values, so we use the current pps as the avg
	 */
	if (hnode->last_pps == 0) {
		avg_pps = hnode->rx_pps;
	} else {
		mov_avg_pps = (hnode->last_pps + hnode->avg_pps) / 2;
		avg_pps = (((new_weight * hnode->rx_pps) +
			    (old_weight * mov_avg_pps)) /
			    (new_weight + old_weight));
	}

	return avg_pps;
}

void rmnet_shs_wq_update_hash_stats_debug(struct rmnet_shs_wq_hstat_s *hstats_p,
					  struct rmnet_shs_skbn_s *node_p)
{
	int idx = rmnet_shs_flow_dbg_stats_idx_cnt;

	if (!rmnet_shs_stats_enabled)
		return;

	if (!hstats_p || !node_p) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return;
	}

	if (hstats_p->stat_idx < 0) {
		idx = idx % MAX_SUPPORTED_FLOWS_DEBUG;
		hstats_p->stat_idx = idx;
		rmnet_shs_flow_dbg_stats_idx_cnt++;
	}

	rmnet_shs_flow_hash[hstats_p->stat_idx] = hstats_p->hash;
	rmnet_shs_flow_proto[hstats_p->stat_idx] = node_p->skb_tport_proto;
	rmnet_shs_flow_inactive_tsec[hstats_p->stat_idx] =
			RMNET_SHS_NSEC_TO_SEC(hstats_p->inactive_duration);
	rmnet_shs_flow_rx_bps[hstats_p->stat_idx] = hstats_p->rx_bps;
	rmnet_shs_flow_rx_pps[hstats_p->stat_idx] = hstats_p->rx_pps;
	rmnet_shs_flow_rx_bytes[hstats_p->stat_idx] = hstats_p->rx_bytes;
	rmnet_shs_flow_rx_pkts[hstats_p->stat_idx] = hstats_p->rx_skb;
	rmnet_shs_flow_cpu[hstats_p->stat_idx] = hstats_p->current_cpu;
	rmnet_shs_flow_cpu_recommended[hstats_p->stat_idx] =
						hstats_p->suggested_cpu;
	rmnet_shs_flow_silver_to_gold[hstats_p->stat_idx] =
		hstats_p->rmnet_shs_wq_suggs[RMNET_SHS_WQ_SUGG_SILVER_TO_GOLD];
	rmnet_shs_flow_gold_to_silver[hstats_p->stat_idx] =
		hstats_p->rmnet_shs_wq_suggs[RMNET_SHS_WQ_SUGG_GOLD_TO_SILVER];
	rmnet_shs_flow_gold_balance[hstats_p->stat_idx] =
		hstats_p->rmnet_shs_wq_suggs[RMNET_SHS_WQ_SUGG_GOLD_BALANCE];

}

/* Returns TRUE if this flow received a new packet
 *         FALSE otherwise
 */
inline u8 rmnet_shs_wq_is_hash_rx_new_pkt(struct rmnet_shs_wq_hstat_s *hstats_p,
				   struct rmnet_shs_skbn_s *node_p)
{
	if (!hstats_p || !node_p) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return 0;
	}

	if (node_p->num_skb == hstats_p->rx_skb)
		return 0;

	return 1;
}

static void rmnet_shs_wq_update_hash_tinactive(struct rmnet_shs_wq_hstat_s *hstats_p,
					struct rmnet_shs_skbn_s *node_p)
{
	ktime_t tdiff;

	if (!hstats_p || !node_p) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return;
	}

	tdiff = rmnet_shs_wq_tnsec - hstats_p->c_epoch;
	hstats_p->inactive_duration = tdiff;

	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_FLOW_STATS,
				RMNET_SHS_WQ_FLOW_STATS_FLOW_INACTIVE,
				hstats_p->hash, tdiff, 0xDEF, 0xDEF,
				hstats_p, NULL);
}

static void rmnet_shs_wq_update_hash_stats(struct rmnet_shs_wq_hstat_s *hstats_p)
{
	ktime_t tdiff;
	u64 skb_diff, coal_skb_diff, bytes_diff;
	struct rmnet_shs_skbn_s *node_p;

	if (!hstats_p) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_PTR_ERR]++;
		return;
	}

	node_p = hstats_p->node;

	if (!rmnet_shs_wq_is_hash_rx_new_pkt(hstats_p, node_p)) {
		hstats_p->rx_pps = 0;
		hstats_p->avg_pps = 0;
		hstats_p->rx_bps = 0;
		rmnet_shs_wq_update_hash_tinactive(hstats_p, node_p);
		rmnet_shs_wq_update_hash_stats_debug(hstats_p, node_p);
		return;
	}

	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_FLOW_STATS,
				RMNET_SHS_WQ_FLOW_STATS_START,
				hstats_p->hash, 0xDEF, hstats_p->rx_pps,
				hstats_p->rx_bps, hstats_p, NULL);

	hstats_p->inactive_duration = 0;
	hstats_p->l_epoch = node_p->hstats->c_epoch;
	hstats_p->last_rx_skb = node_p->hstats->rx_skb;
	hstats_p->last_rx_ll_skb = node_p->hstats->rx_ll_skb;

	hstats_p->last_rx_coal_skb = node_p->hstats->rx_coal_skb;
	hstats_p->last_hw_coal_bytes = node_p->hstats->hw_coal_bytes;
	hstats_p->last_hw_coal_bufsize = node_p->hstats->hw_coal_bufsize;
	hstats_p->last_rx_bytes = node_p->hstats->rx_bytes;

	hstats_p->c_epoch = rmnet_shs_wq_tnsec;
	hstats_p->rx_skb = node_p->num_skb;
	hstats_p->rx_ll_skb = node_p->num_ll_skb;
	hstats_p->ll_diff = hstats_p->rx_ll_skb !=  hstats_p->last_rx_ll_skb;
	hstats_p->low_latency = node_p->low_latency;

	hstats_p->rx_coal_skb = node_p->num_coal_skb;
	hstats_p->hw_coal_bytes = node_p->hw_coal_bytes;
	hstats_p->hw_coal_bufsize = node_p->hw_coal_bufsize;
	hstats_p->rx_bytes = node_p->num_skb_bytes;
	tdiff = (hstats_p->c_epoch - hstats_p->l_epoch);

	/* Under-cap tdff to be 100ms and check wq_interval_ms > 0*/
	tdiff = (tdiff > RMNET_SHS_MSEC_TO_NSC(rmnet_shs_wq_interval_ms) &&
		rmnet_shs_wq_interval_ms > 0)? tdiff : RMNET_SHS_MSEC_TO_NSC(100);
	skb_diff = hstats_p->rx_skb - hstats_p->last_rx_skb;
	coal_skb_diff = hstats_p->rx_coal_skb - hstats_p->last_rx_coal_skb;
	bytes_diff = hstats_p->rx_bytes - hstats_p->last_rx_bytes;

	rm_err1("SHS_SEGS: hash 0x%x coal skb = %llu | last coal skb = %llu | rx skb = %llu | last rx skb %llu",
	       hstats_p->hash,
	       hstats_p->rx_coal_skb,
	       hstats_p->last_rx_coal_skb,
	       hstats_p->rx_skb,
	       hstats_p->last_rx_skb);

	hstats_p->rx_pps = RMNET_SHS_RX_BPNSEC_TO_BPSEC(skb_diff)/(tdiff);
	hstats_p->rx_bps = RMNET_SHS_RX_BPNSEC_TO_BPSEC(bytes_diff)/(tdiff);
	hstats_p->rx_bps = RMNET_SHS_BYTE_TO_BIT(hstats_p->rx_bps);
	hstats_p->avg_pps = rmnet_shs_wq_get_flow_avg_pps(hstats_p);
	if (coal_skb_diff > 0) {
		hstats_p->avg_segs = skb_diff / coal_skb_diff;
		rm_err1("SHS_SEGS: avg segs = %llu skb_diff = %llu coal_skb_diff = %llu",
		       hstats_p->avg_segs, skb_diff, coal_skb_diff);

	} else {
		hstats_p->avg_segs = 0;
	}
	hstats_p->hw_coal_bytes_diff = hstats_p->hw_coal_bytes - hstats_p->last_hw_coal_bytes;
	hstats_p->hw_coal_bufsize_diff = hstats_p->hw_coal_bufsize - hstats_p->last_hw_coal_bufsize;
	rm_err1("SHS_HW_COAL: hw coal bytes = %llu hw coal bufsize = %llu",
		node_p->hw_coal_bytes, node_p->hw_coal_bufsize);
	rm_err1("SHS_HW_COAL: LAST: hw coal bytes = %llu hw coal bufsize = %llu",
		hstats_p->last_hw_coal_bytes, hstats_p->last_hw_coal_bufsize);
	rm_err1("SHS_HW_COAL: hw coal bytes diff = %llu hw coal bufsize diff = %llu",
		hstats_p->hw_coal_bytes_diff, hstats_p->hw_coal_bufsize_diff);

	hstats_p->last_pps = hstats_p->rx_pps;
	hstats_p->current_cpu = node_p->map_cpu;
	rmnet_shs_wq_update_hash_stats_debug(hstats_p, node_p);

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_FLOW_STATS,
				RMNET_SHS_WQ_FLOW_STATS_END,
				hstats_p->hash, hstats_p->rx_pps,
				hstats_p->rx_bps, (tdiff/1000000),
				hstats_p, NULL);

	hstats_p->bif = node_p->bif;
	hstats_p->ack_thresh = node_p->ack_thresh;
	rm_err1("SHS_QUICKACK: bif = %u ack_thresh = %u",
		node_p->bif, node_p->ack_thresh);
}

/* Increment the per-flow counter for suggestion type */
static void rmnet_shs_wq_inc_sugg_type(u32 sugg_type,
				       struct rmnet_shs_wq_hstat_s *hstat_p)
{
	if (sugg_type >= RMNET_SHS_WQ_SUGG_MAX || hstat_p == NULL)
		return;

	hstat_p->rmnet_shs_wq_suggs[sugg_type] += 1;
}

/* Change suggested cpu, return 1 if suggestion was made, 0 otherwise */
static int rmnet_shs_wq_chng_flow_cpu(u16 old_cpu, u16 new_cpu,
				      u32 hash_to_move, u32 sugg_type)
{
	struct rmnet_shs_skbn_s *node_p;
	struct rmnet_shs_wq_hstat_s *hstat_p;
	int rc = 0;
	u16 bkt;

	if (old_cpu >= MAX_CPUS || new_cpu >= MAX_CPUS) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_CPU_ERR]++;
		return 0;
	}
	spin_lock_bh(&rmnet_shs_ht_splock);
	rcu_read_lock();
	hash_for_each_rcu(RMNET_SHS_HT, bkt, node_p, list) {
		if (!node_p)
			continue;

		if (!node_p->hstats)
			continue;

		hstat_p = rcu_dereference(node_p->hstats);

		if (hash_to_move != 0) {
			/* If hash_to_move is given, only move that flow,
			 * otherwise move all the flows on that cpu
			 */
			if (hstat_p->hash != hash_to_move)
				continue;
		}

		rm_err("SHS_HT: >>  sugg cpu %d | old cpu %d | new_cpu %d | "
		       "map_cpu = %d | flow 0x%x",
		       hstat_p->suggested_cpu, old_cpu, new_cpu,
		       node_p->map_cpu, hash_to_move);

		if (hstat_p->current_cpu == old_cpu) {

			trace_rmnet_shs_wq_high(RMNET_SHS_WQ_FLOW_STATS,
				RMNET_SHS_WQ_FLOW_STATS_SUGGEST_NEW_CPU,
				hstat_p->hash, hstat_p->suggested_cpu,
				new_cpu, 0xDEF, hstat_p, NULL);

			node_p->hstats->suggested_cpu = new_cpu;
			rmnet_shs_wq_inc_sugg_type(sugg_type, hstat_p);
			if (hash_to_move) { /* Stop after moving one flow */
				rm_err("SHS_CHNG: moving single flow: flow 0x%x "
				       "sugg_cpu changed from %d to %d",
				       hstat_p->hash, old_cpu,
				       node_p->hstats->suggested_cpu);
				rc = 1;
				break;
			}
			rm_err("SHS_CHNG: moving all flows: flow 0x%x "
			       "sugg_cpu changed from %d to %d",
			       hstat_p->hash, old_cpu,
			       node_p->hstats->suggested_cpu);
			rc |= 1;
		}
	}
	rcu_read_unlock();
	spin_unlock_bh(&rmnet_shs_ht_splock);

	return rc;
}


/* Returns the least utilized core from a core mask
 * In order of priority
 *    1) Returns rightmost core with no flows (Fully Idle)
 *    2) Returns the core with least flows with no pps (Semi Idle)
 *    3) Returns the core with the least pps (Non-Idle)
 */
int rmnet_shs_wq_get_least_utilized_core(u16 core_msk)
{
	struct rmnet_shs_wq_rx_flow_s *rx_flow_tbl_p = NULL;
	struct rmnet_shs_wq_cpu_rx_pkt_q_s *list_p;
	u64 min_pps = U64_MAX;
	u32 min_flows = U32_MAX;
	int ret_val = -1;
	int semi_idle_ret = -1;
	int full_idle_ret = -1;
	int cpu_num = 0;
	u16 is_cpu_in_msk;

	for (cpu_num = MAX_CPUS-1; cpu_num >= 0; cpu_num--) {

		is_cpu_in_msk = ((1 << cpu_num) & core_msk) && cpu_active(cpu_num) && !is_reserved(cpu_num);
		if (!is_cpu_in_msk)
			continue;

		list_p = &rx_flow_tbl_p->cpu_list[cpu_num];
		trace_rmnet_shs_wq_low(RMNET_SHS_WQ_CPU_STATS,
				       RMNET_SHS_WQ_CPU_STATS_CURRENT_UTIL,
				       cpu_num, list_p->rx_pps, min_pps,
				       0, NULL, NULL);

		/* When there are multiple free CPUs the first free CPU will
		 * be returned
		 */
		if (list_p->flows == 0) {
			full_idle_ret = cpu_num;
			break;
		}
		/* When there are semi-idle CPUs the CPU w/ least flows will
		 * be returned
		 */
		if (list_p->rx_pps == 0 && list_p->flows < min_flows) {
			min_flows = list_p->flows;
			semi_idle_ret = cpu_num;
		}

		/* Found a core that is processing even lower packets */
		if (list_p->rx_pps <= min_pps) {
			min_pps = list_p->rx_pps;
			ret_val = cpu_num;
		}
	}

	if (full_idle_ret >= 0)
		ret_val = full_idle_ret;
	else if (semi_idle_ret >= 0)
		ret_val = semi_idle_ret;

	return ret_val;
}

/* Return 1 if we can move a flow to dest_cpu for this endpoint,
 * otherwise return 0. Basically check rps mask and cpu is online
 */
static int rmnet_shs_wq_check_cpu_move_for_ep(u16 current_cpu, u16 dest_cpu)
{
	u16 cpu_in_rps_mask = 0;


	if (current_cpu >= MAX_CPUS || dest_cpu >= MAX_CPUS) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_CPU_ERR]++;
		return 0;
	}

	cpu_in_rps_mask = (((1 << dest_cpu)  & ~rmnet_shs_cfg.ban_mask &
			  ~rmnet_shs_halt_mask)) && cpu_active(dest_cpu);

	rm_err("SHS_MASK:  cur cpu [%d] | dest_cpu [%d] | "
	       "cpu_active(dest) = %d"
	       "cpu_in_rps_mask = %d, halt_mask %x\n",
	       current_cpu, dest_cpu, cpu_active(dest_cpu),
		   cpu_in_rps_mask, rmnet_shs_halt_mask);

	/* We cannot move to dest cpu if the cur cpu is the same,
	 * the dest cpu is offline, dest cpu is not in the rps mask
	 */
	if (!cpu_in_rps_mask) {
		rmnet_shs_mid_err[RMNET_SHS_SUGG_FAIL1]++;
		return 0;
	}


	if (current_cpu == dest_cpu || !cpu_active(dest_cpu)) {
		rmnet_shs_mid_err[RMNET_SHS_SUGG_FAIL2]++;
		return 0;
	}

	return 1;
}
/* rmnet_shs_wq_try_to_move_flow - try to make a flow suggestion
 * return 1 if flow move was suggested, otherwise return 0
 */
int rmnet_shs_wq_try_to_move_flow(u16 cur_cpu, u16 dest_cpu, u32 hash_to_move,
				  u32 sugg_type)
{

	if (cur_cpu >= MAX_CPUS || dest_cpu >= MAX_CPUS) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_CPU_ERR]++;
		return 0;
	}

	/* Traverse end-point list, check if cpu can be used, based
	 * on it if is online, rps mask, etc. then make
	 * suggestion to change the cpu for the flow by passing its hash
	 */
	if (!rmnet_shs_wq_check_cpu_move_for_ep(cur_cpu,
						dest_cpu)) {
		rm_err("SHS_FDESC: >> Cannot move flow 0x%x "
				" from cpu[%d] to cpu[%d]",
				hash_to_move, cur_cpu, dest_cpu);
		return 0;
	}

	if (rmnet_shs_wq_chng_flow_cpu(cur_cpu, dest_cpu, hash_to_move, sugg_type)) {
		rm_err("SHS_FDESC: >> flow 0x%x was suggested to"
				" move from cpu[%d] to cpu[%d] sugg_type [%d]",
				hash_to_move, cur_cpu, dest_cpu, sugg_type);

		return 1;
	}

	return 0;
}

/* Change flow segmentation, return 1 if set, 0 otherwise */
int rmnet_shs_wq_set_flow_segmentation(u32 hash_to_set, u8 segs_per_skb)
{
	struct rmnet_shs_skbn_s *node_p;
	struct rmnet_shs_wq_hstat_s *hstat_p;
	u16 bkt;

	spin_lock_bh(&rmnet_shs_ht_splock);
	rcu_read_lock();
	hash_for_each_rcu(RMNET_SHS_HT, bkt, node_p, list) {
		if (!node_p)
			continue;

		hstat_p = rcu_dereference(node_p->hstats);
		if (!hstat_p)
			continue;

		if (hstat_p->hash != hash_to_set)
			continue;

		rm_err("SHS_HT: >> segmentation on hash 0x%x segs_per_skb %u",
		       hash_to_set, segs_per_skb);

		trace_rmnet_shs_wq_high(RMNET_SHS_WQ_FLOW_STATS,
				RMNET_SHS_WQ_FLOW_STATS_SET_FLOW_SEGMENTATION,
				hstat_p->hash, segs_per_skb,
				0xDEF, 0xDEF, hstat_p, NULL);

		hstat_p->segs_per_skb = segs_per_skb;
		rcu_read_unlock();
		spin_unlock_bh(&rmnet_shs_ht_splock);
		return 1;
	}
	rcu_read_unlock();
	spin_unlock_bh(&rmnet_shs_ht_splock);

	rm_err("SHS_HT: >> segmentation on hash 0x%x segs_per_skb %u not set - hash not found",
	       hash_to_set, segs_per_skb);
	return 0;
}

/* Change quickack threshold, return 1 if set, 0 otherwise */
int rmnet_shs_wq_set_quickack_thresh(u32 hash_to_set, u32 ack_thresh)
{
	/* Call the hoook in rmnet_perf to set the quickack thresh */
	rm_err("Calling quickack thresh hook in rmnet_perf w/ hash 0x%x and thresh %u",
		hash_to_set, ack_thresh);
	if (rmnet_module_hook_perf_set_thresh(hash_to_set, ack_thresh)) {
		rm_err("Successfully changed ack_thresh to %u", ack_thresh);
		return 1;
	}

	rm_err("Failed to change ack_thresh to %u", ack_thresh);
	return 0;
}

static int rmnet_shs_wq_time_check(ktime_t time, atomic_long_t num_flows)
{

	int ret = false;
	if (time > rmnet_shs_max_flow_inactivity_sec)
		ret = true;

	return ret;
}

noinline void rmnet_shs_wq_filter(void)
{
	int cpu, cur_cpu;
	int temp;
	struct rmnet_shs_wq_hstat_s *hnode = NULL;

	for (cpu = 0; cpu < MAX_CPUS; cpu++) {
		rmnet_shs_cpu_rx_filter_flows[cpu] = 0;
		rmnet_shs_cpu_node_tbl[cpu].seg = 0;
	}

	rcu_read_lock();
	/* Filter out flows with low pkt count and
	 * mark CPUS with slowstart flows
	 */
	list_for_each_entry_rcu(hnode, &rmnet_shs_wq_hstat_tbl, hstat_node_id) {

		if (hnode->in_use == 0)
			continue;
		if (hnode->avg_pps > RMNET_SHS_FILTER_FLOW_RATE &&
		    hnode->rx_skb > RMNET_SHS_FILTER_PKT_LIMIT)
			if (hnode->current_cpu < MAX_CPUS) {
				temp = hnode->current_cpu;
				rmnet_shs_cpu_rx_filter_flows[temp]++;
			}
		cur_cpu = hnode->current_cpu;
		if (cur_cpu >= MAX_CPUS || cur_cpu < 0) {
			continue;
		}

		if (hnode->segs_per_skb > 0) {
			rmnet_shs_cpu_node_tbl[cur_cpu].seg++;
		}
	}
	rcu_read_unlock();

}

void rmnet_shs_wq_update_stats(void)
{
	struct timespec64 time;
	struct rmnet_shs_wq_hstat_s *hnode = NULL;

	(void) ktime_get_boottime_ts64(&time);
	rmnet_shs_wq_tnsec = RMNET_SHS_SEC_TO_NSEC(time.tv_sec) + time.tv_nsec;
	rmnet_update_reserve_mask();
	rmnet_shs_get_state();


	if ((rmnet_shs_wq_tick & SYNC_TIME) == SYNC_TIME)
		rmnet_shs_phy_sync();

	rcu_read_lock();
	list_for_each_entry_rcu(hnode, &rmnet_shs_wq_hstat_tbl, hstat_node_id) {

		if (hnode->in_use == 0)
			continue;

		if (hnode->node) {
			rmnet_shs_wq_update_hash_stats(hnode);
		}
	}
	rcu_read_unlock();

	if (rmnet_shs_userspace_connected) {
		rm_err("%s", "SHS_UPDATE: Userspace connected, relying on userspace evaluation");
		rmnet_shs_wq_mem_update_global();
		rmnet_shs_genl_send_int_to_userspace_no_info(RMNET_SHS_SYNC_RESP_INT);
	}
	rmnet_shs_wq_filter();
}

void rmnet_shs_wq_process_wq(struct work_struct *work)
{
	unsigned long jiffies;

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_PROCESS_WQ,
				RMNET_SHS_WQ_PROCESS_WQ_START,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);

	rmnet_shs_wq_tick++;
	spin_lock_bh(&rmnet_shs_ep_lock);
	rmnet_shs_wq_update_stats();
	spin_unlock_bh(&rmnet_shs_ep_lock);


	jiffies = msecs_to_jiffies(rmnet_shs_wq_interval_ms);

	queue_delayed_work(rmnet_shs_wq, &rmnet_shs_delayed_wq->wq,
			   jiffies);

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_PROCESS_WQ,
				RMNET_SHS_WQ_PROCESS_WQ_END,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
}

static void rmnet_shs_hnode_free(struct rcu_head *head)
{
       struct rmnet_shs_wq_hstat_s *hnode;

       hnode = container_of(head, struct rmnet_shs_wq_hstat_s, rcu);
       kfree(hnode->node);
       kfree(hnode);
}

void rmnet_shs_wq_cleanup_hash_tbl(u8 force_clean, u32 hash_to_clean)
{
	struct rmnet_shs_skbn_s *node_p = NULL;
	ktime_t tns2s;
	struct rmnet_shs_wq_hstat_s *hnode = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	int lock_flag = 0;

	spin_lock_bh(&rmnet_shs_ht_splock);
	list_for_each_safe(ptr, next, &rmnet_shs_wq_hstat_tbl) {
		hnode = list_entry(ptr, struct rmnet_shs_wq_hstat_s, hstat_node_id);

		if (hnode->node == NULL)
			continue;

		/* If hash is passed skip all non-matching hashes and only evict match */
		if (hash_to_clean && hnode->hash != hash_to_clean)
			continue;

		/* If shs just is calling  rmnet_rx_handler prevent cleanup of nodes */
		if (rmnet_shs_cfg.kfree_stop && !force_clean)
			continue;

		node_p = hnode->node;
		tns2s = RMNET_SHS_NSEC_TO_SEC(hnode->inactive_duration);

		/* Flows are cleanup from book keeping faster if
		 * there are a lot of active flows already in memory
		 * Only clear phy node if shs_switch is off.
		 */
		if ((rmnet_shs_wq_time_check(tns2s, rmnet_shs_cfg.num_flows) &&
		    ((node_p->phy && !rmnet_module_hook_is_set(RMNET_MODULE_HOOK_SHS_SWITCH)) || !node_p->phy)) ||
		    force_clean) {
			trace_rmnet_shs_wq_low(RMNET_SHS_WQ_FLOW_STATS,
					       RMNET_SHS_WQ_FLOW_STATS_FLOW_INACTIVE_TIMEOUT,
					       node_p->hash, tns2s, 0xDEF, 0xDEF, node_p, hnode);

			if (unlikely(!node_p)) {
				rmnet_shs_crit_err[RMNET_SHS_INVALID_HNODE]++;
				continue;
			}
			/* Low latency nodes need to be cleared from LL ht list with LL locking */
			if (node_p->low_latency == RMNET_SHS_TRUE_LOW_LATENCY) {
				lock_flag = 1;
			}
			if (lock_flag)
				spin_lock_bh(&rmnet_shs_ll_ht_splock);

			rm_err("SHS_FLOW: removing flow 0x%x on cpu[%d] "
				   "pps: %llu avg_pps: %llu",
				   hnode->hash, hnode->current_cpu,
				   hnode->rx_pps, hnode->avg_pps);
			/* Shouldn't be needed for LL flows as no parking is done*/
			rmnet_shs_clear_node(node_p, RMNET_WQ_CTXT);
			rmnet_shs_cpu_list_remove(hnode);
			rmnet_shs_cpu_node_remove(node_p);
			rmnet_shs_hstat_tbl_remove(hnode);
			hash_del_rcu(&node_p->list);
			atomic_long_dec(&rmnet_shs_cfg.num_flows);
			/* Unlocking temporarily to call synchronize RCU, can sync + be in bh*/
			if (lock_flag)
				spin_unlock_bh(&rmnet_shs_ll_ht_splock);
			call_rcu(&hnode->rcu, rmnet_shs_hnode_free);
		}

	}
	spin_unlock_bh(&rmnet_shs_ht_splock);
}

void rmnet_shs_wq_pause(void)
{
	int cpu;
	struct rmnet_shs_msg_resp msg;


	rmnet_shs_pause_count++;

	if (rmnet_shs_wq && rmnet_shs_delayed_wq)
		cancel_delayed_work_sync(&rmnet_shs_delayed_wq->wq);


	for (cpu = 0; cpu < MAX_CPUS; cpu++)
		rmnet_shs_cpu_rx_filter_flows[cpu] = 0;

	/* If shsusrd fails to reset phy do it on a pause
	 * Reset switch pointer if set for some reason
	 * There is a brief window where follwing can happen.
	 * SHS could go flat when suggestion is made tcpu = x, rss = high
	 * If rps isn't moved i.e rps = 1, tcpu = prio, rss = high
         * We can just reset tcpu to 1 and NULL, rss.
	 * If rss is low then change took place and we need to set rss high again.
	 * This should cover the case when that occurs and correct phy and pointer.
	 *
	 * Check core 1 is not in the reserve mask
	 */
	rcu_read_lock();
	if ((rmnet_shs_cfg.phy_acpu != DEF_PHY_CPU) && ((1 << DEF_PHY_CPU ) & ~rmnet_shs_halt_mask)) {
		rmnet_shs_cfg.phy_tcpu = DEF_PHY_CPU;
		rmnet_shs_switch_enable();
		rmnet_shs_switch_reason[RMNET_SHS_WQ_FAIL_PHY_DROP]++;
	 }
	rcu_read_unlock();

	/* Create boost msg and deliver using direct msg channel to shsusrd */
	rmnet_shs_create_pause_msg_resp(0, &msg);
	rmnet_shs_genl_msg_direct_send_to_userspace(&msg);
}

void rmnet_shs_wq_restart(void)
{
	rmnet_shs_restart_count++;

	/* Estimation is off if restart is immediate */
	if (rmnet_shs_wq && rmnet_shs_delayed_wq)
		queue_delayed_work(rmnet_shs_wq, &rmnet_shs_delayed_wq->wq, 0);
}

void rmnet_shs_wq_exit(void)
{
	struct rmnet_shs_msg_resp chg_msg;

	/*If Wq is not initialized, nothing to cleanup */
	if (!rmnet_shs_wq || !rmnet_shs_delayed_wq)
		return;

	rmnet_shs_create_cleanup_msg_resp(&chg_msg);
	rmnet_shs_genl_msg_direct_send_to_userspace(&chg_msg);
	rmnet_shs_genl_send_int_to_userspace_no_info(RMNET_SHS_SYNC_WQ_EXIT);
	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_EXIT, RMNET_SHS_WQ_EXIT_START,
				   0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);

	rmnet_shs_wq_pause();

	cancel_delayed_work_sync(&rmnet_shs_delayed_wq->wq);
	drain_workqueue(rmnet_shs_wq);
	destroy_workqueue(rmnet_shs_wq);
	kfree(rmnet_shs_delayed_wq);

	rmnet_shs_delayed_wq = NULL;
	rmnet_shs_wq = NULL;
	rmnet_shs_wq_cleanup_hash_tbl(FORCE_CLEAN, 0);
	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_EXIT, RMNET_SHS_WQ_EXIT_END,
				   0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
}

void rmnet_shs_wq_init(void)
{
	/*If the workqueue is already initialized we should not be
	 *initializing again
	 */
	if (rmnet_shs_wq)
		return;

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_INIT, RMNET_SHS_WQ_INIT_START,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
	rmnet_shs_wq = alloc_workqueue("rmnet_shs_wq", WQ_UNBOUND, 1);
	if (!rmnet_shs_wq) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_ALLOC_WQ_ERR]++;
		return;
	}

	rmnet_shs_delayed_wq = kmalloc(sizeof(struct rmnet_shs_delay_wq_s),
				       GFP_ATOMIC);

	if (!rmnet_shs_delayed_wq) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_ALLOC_DEL_WQ_ERR]++;
		rmnet_shs_wq_exit();
		return;
	}


	INIT_DELAYED_WORK(&rmnet_shs_delayed_wq->wq,
			     rmnet_shs_wq_process_wq);

	trace_rmnet_shs_wq_high(RMNET_SHS_WQ_INIT, RMNET_SHS_WQ_INIT_END,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);
}
