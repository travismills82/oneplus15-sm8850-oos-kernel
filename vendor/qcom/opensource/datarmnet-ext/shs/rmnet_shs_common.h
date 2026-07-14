/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _RMNET_SHS_COMMON_H_
#define _RMNET_SHS_COMMON_H_
#undef TRACE_INCLUDE_PATH

#include <trace/hooks/sched.h>
#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>
#if (KERNEL_VERSION(6, 9, 0) <= LINUX_VERSION_CODE)
#include <net/rps.h>
#endif

#define VH_MAGIC_HASH 0x77777
#define SHSUSR_IPV4 1
#define SHSUSR_IPV6 2

void rmnet_shs_switch_disable(void);
void rmnet_shs_switch_enable(void);
void rmnet_shs_skb_entry_disable(void);
void rmnet_shs_skb_entry_enable(void);

/* Helper functions to add and remove entries to the table
 * that maintains a list of all nodes that maintain statistics per flow
 */
void rmnet_shs_hstat_tbl_add(struct rmnet_shs_wq_hstat_s *hnode);
void rmnet_shs_hstat_tbl_remove(struct rmnet_shs_wq_hstat_s *hnode);

/* We maintain a list of all flow nodes processed by a cpu.
 * Below helper functions are used to maintain flow<=>cpu
 * association.*
 */
void rmnet_shs_cpu_list_add(struct rmnet_shs_wq_hstat_s *hnode,
			    struct list_head *head);
void rmnet_shs_cpu_list_move(struct rmnet_shs_wq_hstat_s *hnode,
			     struct list_head *head);

void rmnet_shs_ep_lock_bh(void);
void rmnet_shs_ep_unlock_bh(void);

void rmnet_shs_update_cfg_mask(void);

void rmnet_shs_cpu_node_remove(struct rmnet_shs_skbn_s *node);
void rmnet_shs_cpu_node_add(struct rmnet_shs_skbn_s *node,
			    struct list_head *hd);
void rmnet_shs_cpu_node_move(struct rmnet_shs_skbn_s *node,
			     struct list_head *hd, int oldcpu);

void rmnet_shs_cpu_ooo(u8 cpu, int count);
inline int rmnet_shs_is_lpwr_cpu(u16 cpu);

u64 rmnet_shs_wq_get_max_allowed_pps(u16 cpu);
u32 rmnet_shs_get_cpu_qtail(u8 cpu_num);
u32 rmnet_shs_get_cpu_qdiff(u8 cpu_num);
u8 rmnet_shs_mask_from_map(struct rps_map *map);

/* Return Invalid core if only pri core available*/
void rmnet_shs_ps_on_hdlr(void *port);
void rmnet_shs_ps_off_hdlr(void *port);
int rmnet_shs_get_mask_len(u8 mask);
void rmnet_shs_cpu_list_remove(struct rmnet_shs_wq_hstat_s *hnode);
void rmnet_shs_cpu_list_add(struct rmnet_shs_wq_hstat_s *hnode,
			    struct list_head *head);
void rmnet_shs_cpu_list_move(struct rmnet_shs_wq_hstat_s *hnode,
			     struct list_head *head);

int rmnet_shs_idx_from_cpu(u8 cpu, u8 mask);
void rmnet_shs_get_update_skb_hdr_info(struct sk_buff *skb,
				       struct rmnet_shs_skbn_s *node_p);

void *rmnet_shs_header_ptr(struct sk_buff *skb, u32 offset, u32 hlen,
				  void *buf);
int rmnet_shs_is_skb_l4s(struct sk_buff *skb);
int rmnet_shs_is_skb_ecn_capable(struct sk_buff *skb);
u32 rmnet_shs_form_hash(u32 index, u32 maplen, u32 hash, u8 setasync);
void rmnet_shs_deliver_skb(struct sk_buff *skb);
void rmnet_shs_deliver_skb_wq(struct sk_buff *skb);
extern struct list_head rmnet_shs_wq_hstat_tbl;
u8 rmnet_shs_get_online_mask(void);

static inline void rmnet_vh_do_wake_up_sync(void *unused, struct wait_queue_head *wq_head, int *done, struct sock* sk)
{

	if ((sk->sk_protocol == IPPROTO_TCP || sk->sk_protocol == IPPROTO_UDP) &&
	    (sk->sk_rxhash & 0xFFFFF) == VH_MAGIC_HASH) {
		(*done) = 1;
		/* Non sync poll is done here as above flag disables sync poll */
		wake_up_interruptible_poll(wq_head, EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND);
	}
}

static inline int rmnet_shs_vh_set(void)
{
	int rc = 0;

	rc = register_trace_android_vh_do_wake_up_sync(rmnet_vh_do_wake_up_sync, NULL);

	return rc;
}

static inline int rmnet_shs_vh_unset(void)
{
	int rc = 0;

	rc = unregister_trace_android_vh_do_wake_up_sync(rmnet_vh_do_wake_up_sync, NULL);

	return rc;
}

#undef TRACE_INCLUDE_PATH
#endif
