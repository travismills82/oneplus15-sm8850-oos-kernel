// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "rmnet_shs.h"
#include "rmnet_shs_wq.h"
#include "rmnet_shs_modules.h"
#include "rmnet_module.h"
#include <net/ip.h>
#include <linux/cpu.h>
#include <linux/bitmap.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/ipv6.h>
#include <net/dsfield.h>


#define INCREMENT 1
#define DECREMENT 0

static int rmnet_shs_switch_hook_entry(struct sk_buff *skb,
				       struct rmnet_shs_clnt_s *cfg)
{
	struct rmnet_skb_cb *cb = RMNET_SKB_CB(skb);

	if (!cb->qmap_steer && skb->priority != 0xda1a) {
		cb->qmap_steer = 1;
		rmnet_shs_assign(skb, cfg);
		return 1;
	}

	return 0;
}

static const struct rmnet_module_hook_register_info
rmnet_shs_switch_hook = {
	.hooknum = RMNET_MODULE_HOOK_SHS_SWITCH,
	.func = rmnet_shs_switch_hook_entry,
};

void rmnet_shs_switch_disable(void)
{
	rmnet_module_hook_unregister_no_sync(&rmnet_shs_switch_hook, 1);
}

void rmnet_shs_switch_enable(void)
{
	rmnet_module_hook_register(&rmnet_shs_switch_hook, 1);
}

static const struct rmnet_module_hook_register_info
rmnet_shs_skb_entry_hook = {
	.hooknum = RMNET_MODULE_HOOK_SHS_SKB_ENTRY,
	.func = rmnet_shs_assign,
};

void rmnet_shs_skb_entry_disable(void)
{
	rmnet_module_hook_unregister_no_sync(&rmnet_shs_skb_entry_hook, 1);
}

void rmnet_shs_skb_entry_enable(void)
{
	rmnet_module_hook_register(&rmnet_shs_skb_entry_hook, 1);
}

/* Helper functions to add and remove entries to the table
 * that maintains a list of all nodes that maintain statistics per flow
 */
void rmnet_shs_hstat_tbl_add(struct rmnet_shs_wq_hstat_s *hnode)
{
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_HSTAT_TBL,
			       RMNET_SHS_WQ_HSTAT_TBL_ADD,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, hnode, NULL);
    spin_lock_bh(&rmnet_shs_hstat_tbl_lock);
    list_add_rcu(&hnode->hstat_node_id, &rmnet_shs_wq_hstat_tbl);
    spin_unlock_bh(&rmnet_shs_hstat_tbl_lock);
}

void rmnet_shs_hstat_tbl_remove(struct rmnet_shs_wq_hstat_s *hnode)
{
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_HSTAT_TBL,
			       RMNET_SHS_WQ_HSTAT_TBL_DEL,
				0xDEF, 0xDEF, 0xDEF, 0xDEF, hnode, NULL);

    spin_lock_bh(&rmnet_shs_hstat_tbl_lock);
    list_del_rcu(&hnode->hstat_node_id);
    spin_unlock_bh(&rmnet_shs_hstat_tbl_lock);
}

/* We maintain a list of all flow nodes processed by a cpu.
 * Below helper functions are used to maintain flow<=>cpu
 * association.*
 */
void rmnet_shs_cpu_list_remove(struct rmnet_shs_wq_hstat_s *hnode)
{
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_CPU_HSTAT_TBL,
			    RMNET_SHS_WQ_CPU_HSTAT_TBL_DEL,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, hnode, NULL);
    spin_lock_bh(&rmnet_shs_hstat_tbl_lock);
    list_del_rcu(&hnode->cpu_node_id);
    spin_unlock_bh(&rmnet_shs_hstat_tbl_lock);
}

void rmnet_shs_cpu_list_add(struct rmnet_shs_wq_hstat_s *hnode,
			    struct list_head *head)
{
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_CPU_HSTAT_TBL,
			    RMNET_SHS_WQ_CPU_HSTAT_TBL_ADD,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, hnode, NULL);

    spin_lock_bh(&rmnet_shs_hstat_tbl_lock);
    list_add_rcu(&hnode->cpu_node_id, head);
    spin_unlock_bh(&rmnet_shs_hstat_tbl_lock);
}

void rmnet_shs_cpu_list_move(struct rmnet_shs_wq_hstat_s *hnode,
			     struct list_head *head)
{
	trace_rmnet_shs_wq_low(RMNET_SHS_WQ_CPU_HSTAT_TBL,
			    RMNET_SHS_WQ_CPU_HSTAT_TBL_MOVE,
			    hnode->current_cpu,
			    0xDEF, 0xDEF, 0xDEF, hnode, NULL);
    spin_lock_bh(&rmnet_shs_hstat_tbl_lock);
    list_move(&hnode->cpu_node_id, head);
    spin_unlock_bh(&rmnet_shs_hstat_tbl_lock);
}

void rmnet_shs_ep_lock_bh(void)
{
	spin_lock_bh(&rmnet_shs_ep_lock);
}

void rmnet_shs_ep_unlock_bh(void)
{
	spin_unlock_bh(&rmnet_shs_ep_lock);
}

void rmnet_shs_cpu_node_remove(struct rmnet_shs_skbn_s *node)
{
	SHS_TRACE_LOW(RMNET_SHS_CPU_NODE, RMNET_SHS_CPU_NODE_FUNC_REMOVE,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);

	list_del_rcu(&node->node_id);
	rmnet_shs_change_cpu_num_flows(node->map_cpu, DECREMENT);

}

void rmnet_shs_cpu_node_add(struct rmnet_shs_skbn_s *node,
			    struct list_head *hd)
{
	SHS_TRACE_LOW(RMNET_SHS_CPU_NODE, RMNET_SHS_CPU_NODE_FUNC_ADD,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);

	list_add_rcu(&node->node_id, hd);
	rmnet_shs_change_cpu_num_flows(node->map_cpu, INCREMENT);
}

void rmnet_shs_cpu_node_move(struct rmnet_shs_skbn_s *node,
			     struct list_head *hd, int oldcpu)
{
	SHS_TRACE_LOW(RMNET_SHS_CPU_NODE, RMNET_SHS_CPU_NODE_FUNC_MOVE,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, NULL, NULL);

	list_move(&node->node_id, hd);
	rmnet_shs_change_cpu_num_flows(node->map_cpu, INCREMENT);
	rmnet_shs_change_cpu_num_flows((u16) oldcpu, DECREMENT);
}

void rmnet_shs_cpu_ooo(u8 cpu, int count)
{
	if (cpu < MAX_CPUS)
	{
		rmnet_shs_cpu_ooo_count[cpu]+=count;
	}
}

u64 rmnet_shs_wq_get_max_allowed_pps(u16 cpu)
{

	if (cpu >= MAX_CPUS) {
		rmnet_shs_crit_err[RMNET_SHS_WQ_INVALID_CPU_ERR]++;
		return 0;
	}

	return rmnet_shs_cpu_rx_max_pps_thresh[cpu];
}

inline int rmnet_shs_is_lpwr_cpu(u16 cpu)
{
	return !((1 << cpu) & PERF_MASK);
}

u32 rmnet_shs_get_cpu_qhead(u8 cpu_num)
{
	u32 ret = 0;

	if (cpu_num < MAX_CPUS)
		ret = rmnet_shs_cpu_node_tbl[cpu_num].qhead;

	SHS_TRACE_LOW(RMNET_SHS_CORE_CFG, RMNET_SHS_CORE_CFG_GET_QHEAD,
			    cpu_num, ret, 0xDEF, 0xDEF, NULL, NULL);
	return ret;
}

u32 rmnet_shs_get_cpu_qtail(u8 cpu_num)
{
	u32 ret = 0;

	if (cpu_num < MAX_CPUS)
		ret =  rmnet_shs_cpu_node_tbl[cpu_num].qtail;

	SHS_TRACE_LOW(RMNET_SHS_CORE_CFG, RMNET_SHS_CORE_CFG_GET_QTAIL,
			    cpu_num, ret, 0xDEF, 0xDEF, NULL, NULL);

	return ret;
}

u32 rmnet_shs_get_cpu_qdiff(u8 cpu_num)
{
	u32 ret = 0;

	if (cpu_num < MAX_CPUS)
		ret =  rmnet_shs_cpu_node_tbl[cpu_num].qdiff;

	SHS_TRACE_LOW(RMNET_SHS_CORE_CFG, RMNET_SHS_CORE_CFG_GET_QTAIL,
			    cpu_num, ret, 0xDEF, 0xDEF, NULL, NULL);

	return ret;
}

void rmnet_shs_ps_on_hdlr(void *port)
{
	rmnet_shs_wq_pause();
}

void rmnet_shs_ps_off_hdlr(void *port)
{
	rmnet_shs_wq_restart();
}

u8 rmnet_shs_get_online_mask(void)
{
	u8 mask = 0;
	int i;

	/* Find idx by counting all other configed CPUs*/
	for (i = 0; i < MAX_CPUS; i++) {
		if (cpu_online(i))
			mask |= 1 << i;
	}
	return mask;
}

u8 rmnet_shs_mask_from_map(struct rps_map *map)
{
	u8 mask = 0;
	u8 i;

	for (i = 0; i < map->len; i++)
		mask |= 1 << map->cpus[i];

	return mask;
}

int rmnet_shs_get_mask_len(u8 mask)
{
	u8 i;
	u8 sum = 0;

	for (i = 0; i < MAX_CPUS; i++) {
		if (mask & (1 << i))
			sum++;
	}
	return sum;
}

/* Takes a CPU and a CPU mask and computes what index of configured
 * the CPU is in. Returns INVALID_CPU if CPU is not enabled in the mask.
 */
int rmnet_shs_idx_from_cpu(u8 cpu, u8 mask)
{
	int ret = INVALID_CPU;
	u8 idx = 0;
	u8 i;

	/* If not in mask return invalid*/
	if (!(mask & 1 << cpu))
		return ret;

	/* Find idx by counting all other configed CPUs*/
	for (i = 0; i < MAX_CPUS; i++) {
		if (i == cpu  && (mask & (1 << i))) {
			ret = idx;
			break;
		}
		if (mask & (1 << i))
			idx++;
	}
	return ret;
}

void *rmnet_shs_header_ptr(struct sk_buff *skb, u32 offset, u32 hlen,
				  void *buf)
{
	struct skb_shared_info *shinfo = skb_shinfo(skb);
	skb_frag_t *frag;
	u32 offset_orig = offset;
	int i;

	if (offset > skb->len || hlen > skb->len || offset + hlen > skb->len)
		return NULL;

	/* Linear packets or packets with headers in linear portion */
	if (skb_headlen(skb) >= offset + hlen)
		return skb->data + offset;

	offset -= skb_headlen(skb);
	/* Return pointer to page if contiguous */
	for (i = 0; i < shinfo->nr_frags; i++) {
		u32 frag_size;

		frag = &shinfo->frags[i];
		frag_size = skb_frag_size(frag);
		if (offset >= frag_size) {
			/* Next frag */
			offset -= frag_size;
			continue;
		}

		if (frag_size >= offset + hlen)
			return skb_frag_address(frag) + offset;
	}

	/* The data is split across pages. Use the linear buffer */
	if (skb_copy_bits(skb, (int)offset_orig, buf, (int)hlen))
		return NULL;

	return buf;
}

void rmnet_shs_get_update_skb_hdr_info(struct sk_buff *skb,
				       struct rmnet_shs_skbn_s *node_p)
{
	struct iphdr *ip4h, __ip4h;
	struct ipv6hdr *ip6h, __ip6h;

	struct tcphdr *tp, __tp;
	struct udphdr *up, __up;

	int len = 0;
	u16 ip_len = 0;

	__be16 frag_off;
	u8 protocol;

	switch (skb->protocol) {
	case htons(ETH_P_IP):
		ip4h = rmnet_shs_header_ptr(skb, 0, sizeof(*ip4h), &__ip4h);
		if (!ip4h)
			return;

		node_p->skb_tport_proto = ip4h->protocol;
		node_p->ip_fam = SHSUSR_IPV4;
		memcpy(&(node_p->ip_hdr.v4hdr), ip4h, sizeof(*ip4h));

		ip_len = ip4h->ihl * 4;

		break;
	case htons(ETH_P_IPV6):
		ip6h = rmnet_shs_header_ptr(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return;

		node_p->skb_tport_proto = ip6h->nexthdr;
		node_p->ip_fam = SHSUSR_IPV6;
		memcpy(&(node_p->ip_hdr.v6hdr), ip6h, sizeof(*ip6h));

		protocol = ip6h->nexthdr;

		len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &protocol,
				       &frag_off);
		if (len < 0) {
			/* Cant find transport header */
			return;
		}

		ip_len = (u16)len;

		break;
	default:
		break;
	}

	if (node_p->skb_tport_proto == IPPROTO_TCP) {
		tp = rmnet_shs_header_ptr(skb, ip_len, sizeof(*tp), &__tp);
		if (!tp)
			return;

		memcpy(&(node_p->trans_hdr.tp),
		       tp,
		       sizeof(struct tcphdr));
	} else if (node_p->skb_tport_proto == IPPROTO_UDP) {
		up = rmnet_shs_header_ptr(skb, ip_len, sizeof(*up), &__up);
		if (!up)
			return;

		memcpy(&(node_p->trans_hdr.up),
		       up,
		       sizeof(struct udphdr));
	} else {
		/* Non TCP or UDP proto, dont copy transport header */
	}

}

static u8 rmnet_shs_get_skb_dsfield(struct sk_buff *skb)
{
	/* It's tempting to use the inet_ecn helpers for this, but as those
	 * rely on skb->network_header being set and stuff being linear
	 * (which might not be the case depending on the path this SKB took...)
	 * we err on the safe side.
	 */
	switch (skb->protocol) {
	case __cpu_to_be16(ETH_P_IP):
	{
		struct iphdr *iph, __iph;

		iph = rmnet_shs_header_ptr(skb, 0, sizeof(*iph), &__iph);
		if (!iph)
			return 0;

		return ipv4_get_dsfield(iph);
	}
	case __cpu_to_be16(ETH_P_IPV6):
	{
		struct ipv6hdr *ip6h, __ip6h;

		ip6h = rmnet_shs_header_ptr(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return 0;

		return ipv6_get_dsfield(ip6h);
	}
	default:
		return 0;
	}
}

int rmnet_shs_is_skb_l4s(struct sk_buff *skb)
{
	u8 dsfield = rmnet_shs_get_skb_dsfield(skb);

	/* L4S sets ECT(1) */
	return !!((dsfield & 0x3) == 0x1);
}

int rmnet_shs_is_skb_ecn_capable(struct sk_buff *skb)
{
	u8 dsfield = rmnet_shs_get_skb_dsfield(skb);

	/* Any non-zero value in the lower order bits means yes */
	return !!(dsfield & 0x3);
}

/* Forms a new hash from the incoming hash based on the number of cores
 * available for processing. This new hash will be stamped by
 * SHS module (for all the packets arriving with same incoming hash)
 * before delivering them to next layer.
 */
u32 rmnet_shs_form_hash(u32 index, u32 maplen, u32 hash, u8 async)
{
	int offsetmap[MAX_CPUS / 2] = {8, 4, 3, 2};
	u32 ret = 0;

	if (!maplen) {
		rmnet_shs_crit_err[RMNET_SHS_MAIN_MAP_LEN_INVALID]++;
		return ret;
	}

	/* Override MSB of skb hash to steer. Save most of Hash bits
	 * Leave some as 0 to allow for easy debugging.
	 */
	if (maplen < MAX_CPUS)
		ret = ((((index + ((maplen % 2) ? 1 : 0))) << 28)
			* offsetmap[(maplen - 1) >> 1]) | (hash & 0xFFFFFF);
	/*Wipe last 4 bytes and set to magic number if async set*/
	if (async)
		ret = (ret & ~0xFFFFF) | VH_MAGIC_HASH;

	SHS_TRACE_LOW(RMNET_SHS_HASH_MAP, RMNET_SHS_HASH_MAP_FORM_HASH,
			    ret, hash, index, maplen, NULL, NULL);

	return ret;
}

/* Delivers skb's to the next module */
void rmnet_shs_deliver_skb(struct sk_buff *skb)
{
	SHS_TRACE_LOW(RMNET_SHS_DELIVER_SKB, RMNET_SHS_DELIVER_SKB_START,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, skb, NULL);
	netif_receive_skb(skb);
}

void rmnet_shs_deliver_skb_wq(struct sk_buff *skb)
{

	SHS_TRACE_LOW(RMNET_SHS_DELIVER_SKB, RMNET_SHS_DELIVER_SKB_START,
			    0xDEF, 0xDEF, 0xDEF, 0xDEF, skb, NULL);
	netif_rx(skb);
}
