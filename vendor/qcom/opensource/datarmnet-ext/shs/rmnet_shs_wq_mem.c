/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "rmnet_shs_modules.h"
#include "rmnet_shs_common.h"
#include "rmnet_shs_wq_mem.h"
#include <uapi/linux/rmnet_shs.h>
#include <linux/proc_fs.h>
#include <linux/refcount.h>

MODULE_LICENSE("GPL v2");

struct proc_dir_entry *shs_proc_dir;
/* Fixed arrays to copy to userspace over netlink */
struct rmnet_shs_shared_mem_block_s rmnet_shs_wq_global_struct;
struct rmnet_shs_mmap_info *global_shared;
#define global_flow rmnet_shs_wq_global_struct.flow_entries
#define global_blk_hdr rmnet_shs_wq_global_struct.blk_hdr

static int rmnet_shs_mmap_global(struct file *filp, struct vm_area_struct *vma)
{

	int ret = 0;
	struct page *page = NULL;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	if (!global_shared || !global_shared->data) {
		pr_err("rmnet_shs: null global shared %ld", size);
		return 0;
	}

	page = virt_to_page(global_shared->data);
	ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(page), size , vma->vm_page_prot);
	if (ret != 0) {
		pr_err("rmnet_shs: failed remap pfn");
		goto out;
	}
out:
    return ret;
}

static int rmnet_shs_open_global(struct inode *inode, struct file *filp)
{
	struct rmnet_shs_mmap_info *info;

	rm_err("%s", "SHS_MEM: rmnet_shs_global - entry\n");

	rmnet_shs_ep_lock_bh();
	if (!global_shared) {
		info = kzalloc(sizeof(struct rmnet_shs_mmap_info), GFP_ATOMIC);
		if (!info)
			goto fail;

		info->data = (char *)__get_free_pages(GFP_ATOMIC | __GFP_COMP, 4);
		if (!info->data) {
			kfree(info);
			goto fail;
		}

		global_shared = info;
		refcount_set(&global_shared->refcnt, 1);
		rm_err("SHS_MEM: virt_to_phys = 0x%llx global_shared = 0x%llx\n",
		       (unsigned long long)virt_to_phys((void *)info),
		       (unsigned long long)virt_to_phys((void *)global_shared));
	} else {
		refcount_inc(&global_shared->refcnt);
	}

	filp->private_data = global_shared;
	rmnet_shs_ep_unlock_bh();

	rm_err("%s", "SHS_MEM: rmnet_shs_open - OK\n");

	return 0;

fail:
	rmnet_shs_ep_unlock_bh();
	rm_err("%s", "SHS_MEM: rmnet_shs_open - FAILED\n");
	return -ENOMEM;
}

static ssize_t rmnet_shs_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	/*
	 * Decline to expose file value and simply return benign value
	 */
	return RMNET_SHS_READ_VAL;
}

static ssize_t rmnet_shs_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	/*
	 * Returning zero here would result in echo commands hanging
	 * Instead return len and simply decline to allow echo'd values to
	 * take effect
	 */
	return len;
}
static int rmnet_shs_release_global(struct inode *inode, struct file *filp)
{
	struct rmnet_shs_mmap_info *info;

	rm_err("%s", "SHS_MEM: rmnet_shs_release - entry\n");

	rmnet_shs_ep_lock_bh();
	if (global_shared) {
		info = filp->private_data;
		if (refcount_read(&info->refcnt) <= 1) {
			free_page((unsigned long)info->data);
			kfree(info);
			global_shared = NULL;
			filp->private_data = NULL;
		} else {
			refcount_dec(&info->refcnt);
		}
	}
	rmnet_shs_ep_unlock_bh();

	return 0;
}


static const struct proc_ops rmnet_shs_global_fops = {
	.proc_mmap    = rmnet_shs_mmap_global,
	.proc_open    = rmnet_shs_open_global,
	.proc_release = rmnet_shs_release_global,
	.proc_read    = rmnet_shs_read,
	.proc_write   = rmnet_shs_write,
};


void rmnet_shs_wq_mem_update_global(void)
{
	struct rmnet_shs_wq_hstat_s *hnode = NULL;
	struct rmnet_shs_skbn_s *node_p = NULL;

	uint16_t idx = 0;
	unsigned ip_len = 0;

	if (!global_shared) {
		return;
	}
	memset(&rmnet_shs_wq_global_struct, -1, sizeof(rmnet_shs_wq_global_struct)),
	global_blk_hdr.version = SHS_SHARED_MEM_BLOCK_STRUCT_VERSION;
	global_blk_hdr.isolation_mask = rmnet_shs_halt_mask;
	global_blk_hdr.reserve_mask = rmnet_shs_reserve_mask;
	global_blk_hdr.titanium_mask = 0x0;
	global_blk_hdr.online_mask = rmnet_shs_get_online_mask();
	global_blk_hdr.cur_time = ktime_get_boottime_ns();
	if (rmnet_shs_cfg.port) {
		global_blk_hdr.pb_marker_seq = rmnet_shs_cfg.port->stats.pb_marker_seq;
	}

	global_blk_hdr.sizeof_flow_entry = sizeof(struct rmnet_shs_flow_entry_s);

	rcu_read_lock();
	/* Filter out flows with low pkt count and
	 * mark CPUS with slowstart flows
	 */
	list_for_each_entry_rcu(hnode, &rmnet_shs_wq_hstat_tbl, hstat_node_id) {
		if (idx >= MAX_SHSUSRD_FLOWS) {
			break;
		}

		global_flow[idx].hash = hnode->hash;
		global_flow[idx].cpu_num = hnode->current_cpu;
		global_flow[idx].mux_id = hnode->mux_id;
		global_flow[idx].trans_proto = hnode->skb_tport_proto;
		global_flow[idx].is_ll_flow = hnode->low_latency == RMNET_SHS_LOW_LATENCY_MATCH;
		global_flow[idx].is_ll_true_flow = hnode->low_latency == RMNET_SHS_TRUE_LOW_LATENCY;
		global_flow[idx].rx_skbs = hnode->rx_skb;
		global_flow[idx].rx_bytes = hnode->rx_bytes;
		global_flow[idx].hw_coal_bytes = hnode->hw_coal_bytes;
		global_flow[idx].hw_coal_bufsize = hnode->hw_coal_bufsize;
		global_flow[idx].ack_thresh = hnode->ack_thresh;
		node_p = rcu_dereference(hnode->node);
		if (node_p != NULL) {
			global_flow[idx].is_l4s_flow = node_p->l4s;
			global_flow[idx].ecn_capable = node_p->ecn_capable;
			global_flow[idx].ip_family = node_p->ip_fam;
			ip_len = (node_p->ip_fam == SHSUSR_IPV4 )? 4 : 16;
			if (global_flow[idx].trans_proto == IPPROTO_TCP) {
				global_flow[idx].sport = node_p->trans_hdr.tp.source;
				global_flow[idx].dport = node_p->trans_hdr.tp.dest;

			} else if (global_flow[idx].trans_proto == IPPROTO_UDP) {
				global_flow[idx].sport = node_p->trans_hdr.up.source;
				global_flow[idx].dport = node_p->trans_hdr.up.dest;
			}
			if (node_p->ip_fam == SHSUSR_IPV4 ) {

				memcpy(&global_flow[idx].ip_src,
					   &(node_p->ip_hdr.v4hdr.saddr),
					   ip_len);
				memcpy(&global_flow[idx].ip_dest,
					   &(node_p->ip_hdr.v4hdr.daddr),
					   ip_len);
			} else if (node_p->ip_fam == SHSUSR_IPV6) {
				memcpy(&global_flow[idx].ip_src,
					   &(node_p->ip_hdr.v6hdr.saddr),
					   ip_len);
				memcpy(&global_flow[idx].ip_dest,
					   &(node_p->ip_hdr.v6hdr.daddr),
					   ip_len);
			}
		}
		idx++;
	}
	global_blk_hdr.num_flow_entries = idx;
	rcu_read_unlock();
	memcpy((char *) global_shared->data,
		&rmnet_shs_wq_global_struct,
		sizeof(struct rmnet_shs_block_hdr) + idx * sizeof(struct rmnet_shs_flow_entry_s));
	return;
}

/* Creates the proc folder and files for shs shared memory */
void rmnet_shs_wq_mem_init(void)
{
	kuid_t shs_uid;
	kgid_t shs_gid;

	shs_proc_dir = proc_mkdir("shs", NULL);

	if (!shs_proc_dir) {
		rm_err("%s", "SHS_MEM_INIT: Failed to create proc dir");
		return;
	}

	shs_uid = make_kuid(&init_user_ns, 1001);
	shs_gid = make_kgid(&init_user_ns, 1001);

	if (uid_valid(shs_uid) && gid_valid(shs_gid))
		proc_set_user(shs_proc_dir, shs_uid, shs_gid);

	proc_create(RMNET_SHS_PROC_GLOBAL, 0644, shs_proc_dir, &rmnet_shs_global_fops);
	rmnet_shs_ep_lock_bh();
	global_shared = NULL;
	rmnet_shs_ep_unlock_bh();
}

/* Remove shs files and folders from proc fs */
void rmnet_shs_wq_mem_deinit(void)
{
	remove_proc_entry(RMNET_SHS_PROC_GLOBAL, shs_proc_dir);
	remove_proc_entry(RMNET_SHS_PROC_DIR, NULL);

	rmnet_shs_ep_lock_bh();
	global_shared = NULL;
	rmnet_shs_ep_unlock_bh();
}
