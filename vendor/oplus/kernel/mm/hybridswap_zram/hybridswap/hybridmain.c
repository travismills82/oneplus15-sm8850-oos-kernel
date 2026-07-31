// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[HYB_ZRAM]" fmt

#include <linux/slab.h>
#include <linux/cpu.h>
#include <trace/hooks/mm.h>
#include <trace/hooks/vmscan.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/memcontrol.h>
#include <linux/swap.h>
#include <linux/blkdev.h>
#include <linux/cred.h>
#include <linux/mm_inline.h>

#ifdef CONFIG_HYBRIDSWAP_OPLUS_ZRAM
#include "../zram_drv.h"
#include "../zram_drv_internal.h"
#else
#include "hybridswap_zram_drv.h"
#endif
#include "internal.h"
#include "hybridswap.h"
#include "header_dup.h"

#ifdef CONFIG_OPLUS_FEATURE_UXMEM_OPT
#include <../kernel/oplus_cpu/sched/sched_assist/sa_common.h>
#endif

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_MM_OSVELTE)
#include "mm_osvelte/mm-config.h"
#include "mm_osvelte/common.h"
#endif

static const char *swapd_text[NR_EVENT_ITEMS] = {
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	"swapd_wakeup",
	"swapd_hit_refaults",
	"swapd_medium_press",
	"swapd_critical_press",
	"swapd_memcg_ratio_skip",
	"swapd_memcg_refault_skip",
	"swapd_shrink_anon",
	"swapd_swapout",
	"swapd_skip_swapout",
	"swapd_empty_round",
	"swapd_over_min_buffer_skip_times",
	"swapd_empty_round_skip_times",
	"swapd_snapshot_times",
	"swapd_skip_shrink_of_window",
	"swapd_manual_pause",
#ifdef CONFIG_OPLUS_JANK
	"swapd_cpu_busy_skip_times",
	"swapd_cpu_busy_break_times",
#endif
#endif
};

static int log_level = HYB_MAX;
static struct kmem_cache *hybridswap_cache;
static struct list_head score_head;
static DEFINE_SPINLOCK(score_list_lock);
static DEFINE_MUTEX(hybridswap_enable_lock);
static bool hybridswap_enabled = false;
struct hybridswapd_operations *hybridswapd_ops;

DEFINE_MUTEX(reclaim_para_lock);
DEFINE_PER_CPU(struct swapd_event_state, swapd_event_states);
struct zram *swapd_zram;
bool ezreclaimd_enable;

extern int folio_referenced(struct folio *folio, int is_locked,
				struct mem_cgroup *memcg, unsigned long *vm_flags);

void hybridswap_loglevel_set(int level)
{
	log_level = level;
}

int hybridswap_loglevel(void)
{
	return log_level;
}

void __put_memcg_cache(memcg_hybs_t *hybs)
{
	kmem_cache_free(hybridswap_cache, (void *)hybs);
}

static inline void sum_hybridswap_vm_events(unsigned long *ret)
{
	int cpu;
	int i;

	memset(ret, 0, NR_EVENT_ITEMS * sizeof(unsigned long));

	for_each_online_cpu(cpu) {
		struct swapd_event_state *this =
			&per_cpu(swapd_event_states, cpu);

		for (i = 0; i < NR_EVENT_ITEMS; i++)
			ret[i] += this->event[i];
	}
}

static inline void all_hybridswap_vm_events(unsigned long *ret)
{
	cpus_read_lock();
	sum_hybridswap_vm_events(ret);
	cpus_read_unlock();
}

ssize_t hybridswap_vmstat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned long *vm_buf = NULL;
	int len = 0;
	int i = 0;

	vm_buf = kzalloc(sizeof(struct swapd_event_state), GFP_KERNEL);
	if (!vm_buf)
		return -ENOMEM;
	all_hybridswap_vm_events(vm_buf);

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	len += snprintf(buf + len, PAGE_SIZE - len, "%-32s %12lu\n",
			"fault_out_pause", atomic_long_read(hybridswapd_ops->fault_out_pause));
	len += snprintf(buf + len, PAGE_SIZE - len, "%-32s %12lu\n",
			"fault_out_pause_cnt", atomic_long_read(hybridswapd_ops->fault_out_pause_cnt));
#endif

	for (;i < NR_EVENT_ITEMS; i++) {
		len += snprintf(buf + len, PAGE_SIZE - len, "%-32s %12lu\n",
				swapd_text[i], vm_buf[i]);
		if (len == PAGE_SIZE)
			break;
	}
	kfree(vm_buf);

	return len;
}

ssize_t hybridswap_loglevel_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	char *type_buf = NULL;
	unsigned long val;

	type_buf = strstrip((char *)buf);
	if (kstrtoul(type_buf, 0, &val))
		return -EINVAL;

	if (val >= HYB_MAX) {
		log_err("val %lu is not valid\n", val);
		return -EINVAL;
	}
	hybridswap_loglevel_set((int)val);

	return len;
}

ssize_t hybridswap_loglevel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;

	size += scnprintf(buf + size, PAGE_SIZE - size,
			"Hybridswap log level: %d\n", hybridswap_loglevel());

	return size;
}

/* Make sure the memcg is not NULL in caller */
memcg_hybs_t *hybridswap_cache_alloc(struct mem_cgroup *memcg, bool atomic,
				     bool css_alloc)
{
	memcg_hybs_t *hybs;
	u64 ret;
	gfp_t flags = GFP_KERNEL;

	if (memcg->android_oem_data1[0])
		BUG();

	if (atomic)
		flags &= ~__GFP_DIRECT_RECLAIM;

	hybs = (memcg_hybs_t *)kmem_cache_zalloc(hybridswap_cache, flags);
	if (unlikely(hybs == NULL)) {
		log_err("alloc memcg_hybs_t failed\n");
		return NULL;
	}

	hybs->css_alloc = css_alloc;

	INIT_LIST_HEAD(&hybs->score_node);
#ifdef CONFIG_HYBRIDSWAP_CORE
	spin_lock_init(&hybs->zram_init_lock);
#endif
	atomic64_set(&hybs->app_score, 300);
	atomic64_set(&hybs->ub_ufs2zram_ratio, 100);
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	atomic_set(&hybs->ub_mem2zram_ratio, 80);
	atomic_set(&hybs->ub_zram2ufs_ratio, 50);
	atomic_set(&hybs->refault_threshold, 50);
#endif
	hybs->memcg = memcg;
	refcount_set(&hybs->usage, 1);

	ret = atomic64_cmpxchg((atomic64_t *)&memcg->android_oem_data1[0], 0, (u64)hybs);
	if (ret != 0) {
		put_memcg_cache(hybs);
		return (memcg_hybs_t *)ret;
	}

	return hybs;
}

#ifdef CONFIG_HYBRIDSWAP_SWAPD
static void tune_scan_type_hook(void *data, enum scan_balance *s_balance)
{
	hybridswapd_ops->vh_tune_scan_type(NULL , s_balance);
}
#endif

static void mem_cgroup_alloc_hook(void *data, struct mem_cgroup *memcg)
{
	if (memcg->android_oem_data1[0])
		BUG();

	hybridswap_cache_alloc(memcg, true, true);
}

static void mem_cgroup_free_hook(void *data, struct mem_cgroup *memcg)
{
	memcg_hybs_t *hybs;

	if (!memcg->android_oem_data1[0])
		return;

	hybs = (memcg_hybs_t *)memcg->android_oem_data1[0];
	memcg->android_oem_data1[0] = 0;
	put_memcg_cache(hybs);
}

void memcg_app_score_update(struct mem_cgroup *target)
{
	struct list_head *pos = NULL;
	unsigned long flags;

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops->update_memcg_param(target);
#endif
	spin_lock_irqsave(&score_list_lock, flags);
	list_for_each(pos, &score_head) {
		memcg_hybs_t *hybs = list_entry(pos, memcg_hybs_t, score_node);
		if (atomic64_read(&hybs->app_score) <
				atomic64_read(&MEMCGRP_ITEM(target, app_score)))
			break;
	}
	list_move_tail(&MEMCGRP_ITEM(target, score_node), pos);
	spin_unlock_irqrestore(&score_list_lock, flags);
}

static void mem_cgroup_css_online_hook(void *data,
		struct cgroup_subsys_state *css, struct mem_cgroup *memcg)
{
	if (memcg->android_oem_data1[0]) {
		memcg_app_score_update(memcg);
		css_get(css);
	}
}

static void mem_cgroup_css_offline_hook(void *data,
		struct cgroup_subsys_state *css, struct mem_cgroup *memcg)
{
	unsigned long flags;
	memcg_hybs_t *hybs = MEMCGRP_ITEM_DATA(memcg);

	if (hybs) {
		spin_lock_irqsave(&score_list_lock, flags);
		list_del_init(&MEMCGRP_ITEM(memcg, score_node));
		spin_unlock_irqrestore(&score_list_lock, flags);

		/*
		 * if hybs allocated after css_alloc(), then css refcount is
		 * error without css_get(). Actually we can remove get/put in
		 * hooks, however, for compatiable, just invokes css_put()
		 * if alloc in css_alloc_hook().
		 */
		if (hybs->css_alloc)
			css_put(css);
	}
}

#define REGISTER_HOOK(name) do {\
	rc = register_trace_android_vh_##name(name##_hook, NULL);\
	if (rc) {\
		log_err("%s:%d register hook %s failed", __FILE__, __LINE__, #name);\
		goto err_out_##name;\
	}\
} while (0)

#define UNREGISTER_HOOK(name) do {\
	unregister_trace_android_vh_##name(name##_hook, NULL);\
} while (0)

#define ERROR_OUT(name) err_out_##name

static int register_all_hooks(void)
{
	int rc;

	/* mem_cgroup_alloc_hook */
	REGISTER_HOOK(mem_cgroup_alloc);
	/* mem_cgroup_free_hook */
	REGISTER_HOOK(mem_cgroup_free);
	/* mem_cgroup_css_online_hook */
	REGISTER_HOOK(mem_cgroup_css_online);
	/* mem_cgroup_css_offline_hook */
	REGISTER_HOOK(mem_cgroup_css_offline);
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	rc = register_trace_android_vh_alloc_pages_slowpath_end(hybridswapd_ops->vh_alloc_pages_slowpath, NULL);
	if (rc) {
		log_err("register alloc_pages_slowpath_hook failed\n");
		goto err_out_alloc_pages_slowpath;
	}

	/* tune_scan_type_hook */
	REGISTER_HOOK(tune_scan_type);
#endif
#ifdef CONFIG_HYBRIDSWAP_CORE
	/* mem_cgroup_id_remove_hook */
	REGISTER_HOOK(mem_cgroup_id_remove);
#endif
	return 0;

#ifdef CONFIG_HYBRIDSWAP_CORE
	UNREGISTER_HOOK(mem_cgroup_id_remove);
ERROR_OUT(mem_cgroup_id_remove):
#endif
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	UNREGISTER_HOOK(tune_scan_type);
ERROR_OUT(tune_scan_type):
	/* UNREGISTER_HOOK(rmqueue);
ERROR_OUT(rmqueue): */
	unregister_trace_android_vh_alloc_pages_slowpath_end(hybridswapd_ops->vh_alloc_pages_slowpath, NULL);
ERROR_OUT(alloc_pages_slowpath):
#endif
	UNREGISTER_HOOK(mem_cgroup_css_offline);
ERROR_OUT(mem_cgroup_css_offline):
	UNREGISTER_HOOK(mem_cgroup_css_online);
ERROR_OUT(mem_cgroup_css_online):
	UNREGISTER_HOOK(mem_cgroup_free);
ERROR_OUT(mem_cgroup_free):
	UNREGISTER_HOOK(mem_cgroup_alloc);
ERROR_OUT(mem_cgroup_alloc):
	return rc;
}

static void unregister_all_hook(void)
{
	UNREGISTER_HOOK(mem_cgroup_alloc);
	UNREGISTER_HOOK(mem_cgroup_free);
	UNREGISTER_HOOK(mem_cgroup_css_offline);
	UNREGISTER_HOOK(mem_cgroup_css_online);
#ifdef CONFIG_HYBRIDSWAP_CORE
	UNREGISTER_HOOK(mem_cgroup_id_remove);
#endif
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	/* UNREGISTER_HOOK(rmqueue); */
	UNREGISTER_HOOK(tune_scan_type);
	unregister_trace_android_vh_alloc_pages_slowpath_end(hybridswapd_ops->vh_alloc_pages_slowpath, NULL);
#endif
}

unsigned long memcg_anon_pages(struct mem_cgroup *memcg)
{
	return memcg_page_state_local(memcg, NR_INACTIVE_ANON) +
		memcg_page_state_local(memcg, NR_ACTIVE_ANON);
}

static bool inactive_file_is_low(void)
{
	return global_node_page_state(NR_INACTIVE_FILE) < (SZ_512M + SZ_256M) / PAGE_SIZE;
}

/* Shrink by free a batch of pages */
static int force_shrink_batch(struct mem_cgroup * memcg,
			      unsigned long nr_need_reclaim,
			      unsigned long *nr_reclaimed,
			      unsigned long batch,
			      unsigned int reclaim_options)
{
	int ret = 0;
	gfp_t gfp_mask = GFP_KERNEL;
	bool file = !(reclaim_options & MEMCG_RECLAIM_MAY_SWAP);
	int mglru_anon_swappiness = MAX_SWAPPINESS + 1;
	unsigned int opts = reclaim_options | MEMCG_RECLAIM_PROACTIVE;
	int *swappiness = NULL;

	/* use lru_gen_enabled instead */
	if (ezreclaimd_enable && !file)
		swappiness = &mglru_anon_swappiness;

	while (*nr_reclaimed < nr_need_reclaim) {
		unsigned long reclaimed;

		if (!free_zram_is_ok())
			break;

		reclaimed = try_to_free_mem_cgroup_pages(memcg, batch, gfp_mask,
							 opts, swappiness);

		if (reclaimed == 0)
			break;

		*nr_reclaimed += reclaimed;

		if (file && inactive_file_is_low())
			break;

		/* Abort shrink when receive SIGUSR2 */
		if (unlikely(sigismember(&current->pending.signal, SIGUSR2) ||
			sigismember(&current->signal->shared_pending.signal, SIGUSR2))) {
			log_warn("abort shrink while shrinking\n");
			ret = -EINTR;
			break;
		}
	}

	log_warn("%s try to reclaim %lu pages and reclaim %lu pages option: %d\n",
		 MEMCGRP_ITEM(memcg, name), nr_need_reclaim, *nr_reclaimed, reclaim_options);
	return ret;
}

#define	BATCH_4M	(1 << 10)
#define	RECLAIM_INACTIVE	0
#define	RECLAIM_ALL		1
unsigned long get_reclaim_pages(struct mem_cgroup *memcg, bool file,
				char *buf, unsigned long *batch,
				unsigned long *nr_reclaimed)
{
	unsigned long nr_need_reclaim = 0;
	unsigned long reclaim_flag = 0;
	unsigned long reclaim_batch = 0;
	int ret;
	int stat_item = file ? NR_INACTIVE_FILE : NR_INACTIVE_ANON;

	buf = strstrip(buf);
	ret = sscanf(buf, "%lu %lu", &reclaim_flag, &reclaim_batch);
	if (ret != 1 && ret != 2) {
		log_err("reclaim_flag %s value is error!\n",  buf);
		return 0;
	}

	if (reclaim_flag == RECLAIM_INACTIVE)
		nr_need_reclaim = memcg_page_state_local(memcg, stat_item);
	else if (reclaim_flag == RECLAIM_ALL)
		nr_need_reclaim = memcg_page_state_local(memcg, stat_item) +
			memcg_page_state_local(memcg, stat_item + LRU_ACTIVE);
	else
		nr_need_reclaim = reclaim_flag;

	if (reclaim_batch > 0 && batch)
		*batch = reclaim_batch;

	log_warn("batch:%lu file:%d nr_need_reclaim:%lu\n",
		 *batch, file, nr_need_reclaim);
	return nr_need_reclaim;
}

static unsigned long get_total_memcg_anon_pages(struct mem_cgroup *memcg,
	unsigned long *zram_anon, unsigned long *nand_anon)
{
	unsigned long base = memcg_page_state_local(memcg, NR_INACTIVE_ANON) +
		memcg_page_state_local(memcg, NR_ACTIVE_ANON);

	unsigned long zram =
		hybridswap_read_memcg_stats(memcg, MCG_ZRAM_STORED_PG_SZ);

	unsigned long nand =
		hybridswap_read_memcg_stats(memcg, MCG_DISK_STORED_PG_SZ);

	unsigned long total = base + zram + nand;

	zram_anon ? (*zram_anon = zram) : 0;
	nand_anon ? (*nand_anon = nand) : 0;

	log_info("total: %lu, base_anon: %lu, zram: %lu, nand: %lu",
		total, base, zram, nand);

	return total;
}

static ssize_t mem_cgroup_force_shrink_anon_percent(struct kernfs_open_file *of,
		char *buf, size_t nbytes, loff_t off)
{
	int ret;
	struct mem_cgroup *memcg;
	long nr_need_reclaim;
	unsigned long total_pages, nr_reclaimed = 0;
	unsigned long batch = BATCH_4M;
	unsigned int percent = 0;
	unsigned long zram, nand;

	if (!buf)
		return nbytes;

	ret = sscanf(strstrip(buf), "%u", &percent);

	if (percent <= 0)
		return nbytes;

	if (percent > 100)
		percent = 100;

	memcg = mem_cgroup_from_css(of_css(of));
	total_pages = get_total_memcg_anon_pages(memcg, &zram, &nand);

	nr_need_reclaim = total_pages * percent / 100 - zram - nand;

	log_info("%u%% are %lu pages, nr_need_reclaim: %ld",
		percent, total_pages * percent / 100, nr_need_reclaim);

	if (nr_need_reclaim <= 0)
		return nbytes;

	current->flags |= PF_SHRINK_ANON;

	ret = force_shrink_batch(memcg, nr_need_reclaim, &nr_reclaimed,
					batch, MEMCG_RECLAIM_MAY_SWAP);

	current->flags &= ~PF_SHRINK_ANON;
	return nbytes;
}

static ssize_t mem_cgroup_force_shrink(struct kernfs_open_file *of,
		char *buf, size_t nbytes, bool file)
{
	int ret;
	struct mem_cgroup *memcg;
	unsigned long nr_need_reclaim;
	unsigned long nr_reclaimed = 0;
	unsigned long batch = BATCH_4M;
	unsigned int reclaim_options = 0;

	memcg = mem_cgroup_from_css(of_css(of));
	if (file && inactive_file_is_low())
		return -EBUSY;

	nr_need_reclaim = get_reclaim_pages(memcg, file, buf, &batch, &nr_reclaimed);
	if (!file) {
		reclaim_options = MEMCG_RECLAIM_MAY_SWAP;
		/* In the hook of scan_type, only reclaim anon */
		current->flags |= PF_SHRINK_ANON;
	}

	/* Set may_swap as false to only reclaim file */
	ret = force_shrink_batch(memcg, nr_need_reclaim, &nr_reclaimed, batch, reclaim_options);
	if (ret == -EINTR)
		goto out;
out:
	if (!file)
		current->flags &= ~PF_SHRINK_ANON;
	return nbytes;
}

static ssize_t mem_cgroup_force_shrink_anon(struct kernfs_open_file *of,
		char *buf, size_t nbytes, loff_t off)
{
	return mem_cgroup_force_shrink(of, buf, nbytes, false);
}

static ssize_t mem_cgroup_force_shrink_file(struct kernfs_open_file *of,
		char *buf, size_t nbytes, loff_t off)
{
	return mem_cgroup_force_shrink(of, buf, nbytes, true);
}

static ssize_t mem_cgroup_aging_anon(struct kernfs_open_file *of,
		char *buf, size_t nbytes, loff_t off)
{
	return nbytes;
}

static int memcg_total_info_per_app_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = NULL;
	unsigned long anon_size;
	unsigned long zram_compress_size;
	unsigned long eswap_compress_size;
	unsigned long zram_page_size;
	unsigned long eswap_page_size;

	seq_printf(m, "%-8s %-8s %-8s %-8s %-8s %s \n",
			"anon", "zram_c", "zram_p", "eswap_c", "eswap_p",
			"memcg_n");
	while ((memcg = get_next_memcg(memcg))) {
		if (!MEMCGRP_ITEM_DATA(memcg))
			continue;

		anon_size = memcg_anon_pages(memcg);
		zram_compress_size = hybridswap_read_memcg_stats(memcg,
				MCG_ZRAM_STORED_SZ);
		eswap_compress_size = hybridswap_read_memcg_stats(memcg,
				MCG_DISK_STORED_SZ);
		zram_page_size = hybridswap_read_memcg_stats(memcg,
				MCG_ZRAM_STORED_PG_SZ);
		eswap_page_size = hybridswap_read_memcg_stats(memcg,
				MCG_DISK_STORED_PG_SZ);

		anon_size *= PAGE_SIZE / SZ_1K;
		zram_compress_size /= SZ_1K;
		eswap_compress_size /= SZ_1K;
		zram_page_size *= PAGE_SIZE / SZ_1K;
		eswap_page_size *= PAGE_SIZE / SZ_1K;

		seq_printf(m, "%-8lu %-8lu %-8lu %-8lu %-8lu %s \n",
				anon_size, zram_compress_size, zram_page_size,
				eswap_compress_size, eswap_page_size,
				MEMCGRP_ITEM(memcg, name));
	}

	return 0;
}

static int memcg_swap_stat_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = NULL;
	unsigned long eswap_out_cnt;
	unsigned long eswap_out_size;
	unsigned long eswap_in_size;
	unsigned long eswap_in_cnt;
	unsigned long page_fault_cnt;
	unsigned long cur_eswap_size;
	unsigned long max_eswap_size;
	unsigned long zram_compress_size, zram_page_size;
	unsigned long eswap_compress_size, eswap_page_size;

	memcg = mem_cgroup_from_css(seq_css(m));

	zram_compress_size = hybridswap_read_memcg_stats(memcg, MCG_ZRAM_STORED_SZ);
	zram_page_size = hybridswap_read_memcg_stats(memcg, MCG_ZRAM_STORED_PG_SZ);
	eswap_compress_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_STORED_SZ);
	eswap_page_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_STORED_PG_SZ);

	eswap_out_cnt = hybridswap_read_memcg_stats(memcg, MCG_ESWAPOUT_CNT);
	eswap_out_size = hybridswap_read_memcg_stats(memcg, MCG_ESWAPOUT_SZ);
	eswap_in_size = hybridswap_read_memcg_stats(memcg, MCG_ESWAPIN_SZ);
	eswap_in_cnt = hybridswap_read_memcg_stats(memcg, MCG_ESWAPIN_CNT);
	page_fault_cnt = hybridswap_read_memcg_stats(memcg, MCG_DISK_FAULT_CNT);
	cur_eswap_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_SPACE);
	max_eswap_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_SPACE_PEAK);

	seq_printf(m, "%-32s %12lu KB\n", "zramCompressedSize:",
			zram_compress_size / SZ_1K);
	seq_printf(m, "%-32s %12lu KB\n", "zramOrignalSize:",
			zram_page_size << (PAGE_SHIFT - 10));
	/* Compatible with Osense, here are placeholders */
	seq_printf(m, "%-32s %12lu KB\n", "zramCHPCompressedSize:", 0UL);
	seq_printf(m, "%-32s %12lu KB\n", "zramCHPOrignalSize:", 0UL);
	seq_printf(m, "%-32s %12lu KB\n", "eswapCompressedSize:",
			eswap_compress_size / SZ_1K);
	seq_printf(m, "%-32s %12lu KB\n", "eswapOrignalSize:",
			eswap_page_size << (PAGE_SHIFT - 10));
	seq_printf(m, "%-32s %12lu \n", "eswapOutTotal:", eswap_out_cnt);
	seq_printf(m, "%-32s %12lu KB\n", "eswapOutSize:", eswap_out_size / SZ_1K);
	seq_printf(m, "%-32s %12lu\n", "eswapInTotal:", eswap_in_cnt);
	seq_printf(m, "%-32s %12lu KB\n", "eswapInSize:", eswap_in_size / SZ_1K);
	seq_printf(m, "%-32s %12lu\n", "pageInTotal:", page_fault_cnt);
	seq_printf(m, "%-32s %12lu KB\n", "eswapSizeCur:", cur_eswap_size / SZ_1K);
	seq_printf(m, "%-32s %12lu KB\n", "eswapSizeMax:", max_eswap_size / SZ_1K);
	seq_printf(m, "%-32s %12lu\n", "pageInactiveAnon:",
		   memcg_page_state_local(memcg, NR_INACTIVE_ANON));
	seq_printf(m, "%-32s %12lu\n", "pageActiveAnon:",
		   memcg_page_state_local(memcg, NR_ACTIVE_ANON));
	seq_printf(m, "%-32s %12lu\n", "pageInactiveFile:",
		   memcg_page_state_local(memcg, NR_INACTIVE_FILE));
	seq_printf(m, "%-32s %12lu\n", "pageActiveFile:",
		   memcg_page_state_local(memcg, NR_ACTIVE_FILE));
	/* Compatible with Osense, here are placeholders */
	seq_printf(m, "%-32s %12lu\n", "pageInactiveChpAnon:", 0UL);
	seq_printf(m, "%-32s %12lu\n", "pageActiveChpAnon:", 0UL);
	seq_printf(m, "%-32s %12lu\n", "zramOrignalTotal:", zram_page_size);
	seq_printf(m, "%-32s %12lu\n", "memcgSwap:",
		   memcg_page_state(memcg, MEMCG_SWAP));

	return 0;
}

static int memcg_swap_stat_array_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = NULL;
	unsigned long eswap_out_cnt;
	unsigned long eswap_out_size;
	unsigned long eswap_in_size;
	unsigned long eswap_in_cnt;
	unsigned long page_fault_cnt;
	unsigned long cur_eswap_size;
	unsigned long max_eswap_size;
	unsigned long zram_compress_size, zram_page_size;
	unsigned long eswap_compress_size, eswap_page_size;

	memcg = mem_cgroup_from_css(seq_css(m));

	zram_compress_size = hybridswap_read_memcg_stats(memcg, MCG_ZRAM_STORED_SZ);
	zram_page_size = hybridswap_read_memcg_stats(memcg, MCG_ZRAM_STORED_PG_SZ);
	eswap_compress_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_STORED_SZ);
	eswap_page_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_STORED_PG_SZ);

	eswap_out_cnt = hybridswap_read_memcg_stats(memcg, MCG_ESWAPOUT_CNT);
	eswap_out_size = hybridswap_read_memcg_stats(memcg, MCG_ESWAPOUT_SZ);
	eswap_in_size = hybridswap_read_memcg_stats(memcg, MCG_ESWAPIN_SZ);
	eswap_in_cnt = hybridswap_read_memcg_stats(memcg, MCG_ESWAPIN_CNT);
	page_fault_cnt = hybridswap_read_memcg_stats(memcg, MCG_DISK_FAULT_CNT);
	cur_eswap_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_SPACE);
	max_eswap_size = hybridswap_read_memcg_stats(memcg, MCG_DISK_SPACE_PEAK);

	seq_printf(m, "%lu ", zram_compress_size / SZ_1K);
	seq_printf(m, "%lu ", zram_page_size << (PAGE_SHIFT - 10));
	/* Compatible with Osense, here are placeholders */
	seq_printf(m, "%lu ", 0UL);
	seq_printf(m, "%lu ", 0UL);
	seq_printf(m, "%lu ", eswap_compress_size / SZ_1K);
	seq_printf(m, "%lu ", eswap_page_size << (PAGE_SHIFT - 10));
	seq_printf(m, "%lu ", eswap_out_cnt);
	seq_printf(m, "%lu ", eswap_out_size / SZ_1K);
	seq_printf(m, "%lu ", eswap_in_cnt);
	seq_printf(m, "%lu ", eswap_in_size / SZ_1K);
	seq_printf(m, "%lu ", page_fault_cnt);
	seq_printf(m, "%lu ", cur_eswap_size / SZ_1K);
	seq_printf(m, "%lu ", max_eswap_size / SZ_1K);
	seq_printf(m, "%lu ", memcg_page_state_local(memcg, NR_INACTIVE_ANON));
	seq_printf(m, "%lu ", memcg_page_state_local(memcg, NR_ACTIVE_ANON));
	seq_printf(m, "%lu ", memcg_page_state_local(memcg, NR_INACTIVE_FILE));
	seq_printf(m, "%lu ", memcg_page_state_local(memcg, NR_ACTIVE_FILE));
	/* Compatible with Osense, here are placeholders */
	seq_printf(m, "%lu ", 0UL);
	seq_printf(m, "%lu ", 0UL);
	seq_printf(m, "%lu ", zram_page_size);
	seq_printf(m, "%lu\n", memcg_page_state(memcg, MEMCG_SWAP));

	return 0;
}

static ssize_t mem_cgroup_name_write(struct kernfs_open_file *of, char *buf,
		size_t nbytes, loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	memcg_hybs_t *hybp = MEMCGRP_ITEM_DATA(memcg);
	int len, w_len;

	if (unlikely(hybp == NULL)) {
		hybp = hybridswap_cache_alloc(memcg, false, false);
		if (!hybp)
			return -EINVAL;
	}

	buf = strstrip(buf);
	len = strlen(buf) + 1;
	if (len > MEM_CGROUP_NAME_MAX_LEN)
		len = MEM_CGROUP_NAME_MAX_LEN;

	w_len = snprintf(hybp->name, len, "%s", buf);
	if (w_len > len)
		hybp->name[len - 1] = '\0';

	return nbytes;
}

static int mem_cgroup_name_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(seq_css(m));

	if (!MEMCGRP_ITEM_DATA(memcg))
		return -EPERM;

	seq_printf(m, "%s\n", MEMCGRP_ITEM(memcg, name));

	return 0;
}

static int mem_cgroup_app_score_write(struct cgroup_subsys_state *css,
		struct cftype *cft, s64 val)
{
	struct mem_cgroup *memcg;
	memcg_hybs_t *hybs;

	if (val > MAX_APP_SCORE || val < 0)
		return -EINVAL;

	memcg = mem_cgroup_from_css(css);
	hybs = MEMCGRP_ITEM_DATA(memcg);
	if (!hybs) {
		hybs = hybridswap_cache_alloc(memcg, false, false);
		if (!hybs)
			return -EINVAL;
	}

	if (atomic64_read(&MEMCGRP_ITEM(memcg, app_score)) != val)
		atomic64_set(&MEMCGRP_ITEM(memcg, app_score), val);
	memcg_app_score_update(memcg);

	return 0;
}

static s64 mem_cgroup_app_score_read(struct cgroup_subsys_state *css,
		struct cftype *cft)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);

	if (!MEMCGRP_ITEM_DATA(memcg))
		return -EPERM;

	return atomic64_read(&MEMCGRP_ITEM(memcg, app_score));
}

int mem_cgroup_app_uid_write(struct cgroup_subsys_state *css,
		struct cftype *cft, s64 val)
{
	struct mem_cgroup *memcg;
	memcg_hybs_t *hybs;

	if (val < 0)
		return -EINVAL;

	memcg = mem_cgroup_from_css(css);
	hybs = MEMCGRP_ITEM_DATA(memcg);

	if (unlikely(hybs == NULL)) {
		hybs = hybridswap_cache_alloc(memcg, false, false);
		if (!hybs)
			return -EINVAL;
	}

	if (atomic64_read(&MEMCGRP_ITEM(memcg, app_uid)) != val)
		atomic64_set(&MEMCGRP_ITEM(memcg, app_uid), val);

	return 0;
}

static s64 mem_cgroup_app_uid_read(struct cgroup_subsys_state *css, struct cftype *cft)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);

	if (!MEMCGRP_ITEM_DATA(memcg))
		return -EPERM;

	return atomic64_read(&MEMCGRP_ITEM(memcg, app_uid));
}

static int mem_cgroup_ub_ufs2zram_ratio_write(struct cgroup_subsys_state *css,
		struct cftype *cft, s64 val)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);

	if (!MEMCGRP_ITEM_DATA(memcg))
		return -EPERM;

	if (val > MAX_RATIO || val < MIN_RATIO)
		return -EINVAL;

	atomic64_set(&MEMCGRP_ITEM(memcg, ub_ufs2zram_ratio), val);

	return 0;
}

static s64 mem_cgroup_ub_ufs2zram_ratio_read(struct cgroup_subsys_state *css,
		struct cftype *cft)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);

	if (!MEMCGRP_ITEM_DATA(memcg))
		return -EPERM;

	return atomic64_read(&MEMCGRP_ITEM(memcg, ub_ufs2zram_ratio));
}

static int mem_cgroup_force_swapin_write(struct cgroup_subsys_state *css,
		struct cftype *cft, s64 val)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);
	memcg_hybs_t *hybs;
	unsigned long size = 0;
	const unsigned int ratio = 100;

	hybs = MEMCGRP_ITEM_DATA(memcg);
	if (!hybs)
		return -EPERM;

#ifdef	CONFIG_HYBRIDSWAP_CORE
	size = atomic64_read(&hybs->hybridswap_stored_size);
#endif
	size = atomic64_read(&hybs->ub_ufs2zram_ratio) * size / ratio;
	size = EXTENT_ALIGN_UP(size);

#ifdef CONFIG_HYBRIDSWAP_CORE
	hybridswap_batch_out(memcg, size, val ? true : false);
#endif

	return 0;
}

static int mem_cgroup_force_swapout_write(struct cgroup_subsys_state *css,
		struct cftype *cft, s64 val)
{
#ifdef CONFIG_HYBRIDSWAP_CORE
	hybridswap_force_reclaim(mem_cgroup_from_css(css));
#endif
	return 0;
}

struct mem_cgroup *get_next_memcg(struct mem_cgroup *prev)
{
	memcg_hybs_t *hybs = NULL;
	struct mem_cgroup *memcg = NULL;
	struct list_head *pos = NULL;
	unsigned long flags;
	bool prev_got = true;

	spin_lock_irqsave(&score_list_lock, flags);
find_again:
	if (unlikely(!prev))
		pos = &score_head;
	else
		pos = &MEMCGRP_ITEM(prev, score_node);

	if (list_empty(pos)) /* deleted node */
		goto unlock;

	if (pos->next == &score_head)
		goto unlock;

	hybs = list_entry(pos->next, struct mem_cgroup_hybridswap, score_node);
	memcg = hybs->memcg;
	if (unlikely(!memcg))
		goto unlock;

	if (!css_tryget(&memcg->css)) {
		if (prev && prev_got)
			css_put(&prev->css);
		prev = memcg;
		prev_got = false;
		goto find_again;
	}

unlock:
	spin_unlock_irqrestore(&score_list_lock, flags);
	if (prev && prev_got)
		css_put(&prev->css);

	return memcg;
}

void get_next_memcg_break(struct mem_cgroup *memcg)
{
	if (memcg)
		css_put(&memcg->css);
}

static struct cftype mem_cgroup_hybridswap_legacy_files[] = {
	{
		.name = "force_shrink_anon_percent",
		.write = mem_cgroup_force_shrink_anon_percent,
	},
	{
		.name = "force_shrink_anon",
		.write = mem_cgroup_force_shrink_anon,
	},
	{
		.name = "force_shrink_file",
		.write = mem_cgroup_force_shrink_file,
	},
	{
		.name = "total_info_per_app",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = memcg_total_info_per_app_show,
	},
	{
		.name = "aging_anon",
		.write = mem_cgroup_aging_anon,
	},
	{
		.name = "swap_stat",
		.seq_show = memcg_swap_stat_show,
	},
	{
		.name = "swap_stat_array",
		.seq_show = memcg_swap_stat_array_show,
	},
	{
		.name = "name",
		.write = mem_cgroup_name_write,
		.seq_show = mem_cgroup_name_show,
	},
	{
		.name = "app_score",
		.write_s64 = mem_cgroup_app_score_write,
		.read_s64 = mem_cgroup_app_score_read,
	},
	{
		.name = "app_uid",
		.write_s64 = mem_cgroup_app_uid_write,
		.read_s64 = mem_cgroup_app_uid_read,
	},
	{
		.name = "ub_ufs2zram_ratio",
		.write_s64 = mem_cgroup_ub_ufs2zram_ratio_write,
		.read_s64 = mem_cgroup_ub_ufs2zram_ratio_read,
	},
	{
		.name = "force_swapin",
		.write_s64 = mem_cgroup_force_swapin_write,
	},
	{
		.name = "force_swapout",
		.write_s64 = mem_cgroup_force_swapout_write,
	},
#ifdef CONFIG_HYBRIDSWAP_CORE
	{
		.name = "psi",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = hybridswap_psi_show,
	},
	{
		.name = "stored_wm_ratio",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.write_s64 = mem_cgroup_stored_wm_ratio_write,
		.read_s64 = mem_cgroup_stored_wm_ratio_read,
	},
#endif
	{ }, /* terminate */
};

static int hybridswap_enable(struct zram *zram)
{
	int ret = 0;

	if (hybridswap_enabled) {
		log_warn("enabled is true\n");
		return ret;
	}

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	swapd_zram = zram;
	ret = hybridswapd_ops->init(zram);
	if (ret)
		return ret;
#endif

#ifdef CONFIG_HYBRIDSWAP_CORE
	ret = hybridswap_core_enable();
	if (ret)
		goto hybridswap_core_enable_fail;
#endif
	hybridswap_enabled = true;

	return 0;

#ifdef CONFIG_HYBRIDSWAP_CORE
hybridswap_core_enable_fail:
#endif
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops->deinit();
#endif
	return ret;
}

static void hybridswap_disable(struct zram *zram)
{
	if (!hybridswap_enabled) {
		log_warn("enabled is false\n");
		return;
	}

#ifdef CONFIG_HYBRIDSWAP_CORE
	hybridswap_core_disable();
#endif

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops->deinit();
#endif
	hybridswap_enabled = false;
}

ssize_t hybridswap_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int len = snprintf(buf, PAGE_SIZE, "hybridswap %s reclaim_in %s swapd %s\n",
			hybridswap_core_enabled() ? "enable" : "disable",
			hybridswap_reclaim_in_enable() ? "enable" : "disable",
			hybridswapd_ops->enabled() ? "enable" : "disable");

	return len;
}

ssize_t hybridswap_swapd_pause_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t len)
{
	char *type_buf = NULL;
	bool val;

	type_buf = strstrip((char *)buf);
	if (kstrtobool(type_buf, &val))
		return -EINVAL;
	atomic_set(hybridswapd_ops->swapd_pause, val);
	return len;
}

ssize_t hybridswap_swapd_pause_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;

	size += scnprintf(buf + size, PAGE_SIZE - size,
			  "%d\n", atomic_read(hybridswapd_ops->swapd_pause));
	return size;
}

ssize_t hybridswap_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	unsigned long val;
	char *kbuf;
	struct zram *zram;

	kbuf = strstrip((char *)buf);
	ret = kstrtoul(kbuf, 0, &val);
	if (unlikely(ret)) {
		log_err("val %s is invalid!\n", kbuf);

		return -EINVAL;
	}

	mutex_lock(&hybridswap_enable_lock);
	zram = dev_to_zram(dev);
	if (val == 0)
		hybridswap_disable(zram);
	else
		ret = hybridswap_enable(zram);
	mutex_unlock(&hybridswap_enable_lock);

	if (ret == 0)
		ret = len;
	return ret;
}

int __init hybridswap_pre_init(void)
{
	int ret;

	INIT_LIST_HEAD(&score_head);
	log_level = HS_LOG_INFO;

	hybridswap_cache = kmem_cache_create("mem_cgroup_hybridswap",
			sizeof(struct mem_cgroup_hybridswap),
			0, SLAB_PANIC, NULL);
	if (!hybridswap_cache) {
		log_err("create hybridswap_cache failed\n");
		ret = -ENOMEM;
		return ret;
	}

	ret = cgroup_add_legacy_cftypes(&memory_cgrp_subsys,
			mem_cgroup_hybridswap_legacy_files);
	if (ret) {
		log_info("add mem_cgroup_hybridswap_legacy_files failed\n");
		goto error_out;
	}

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops = kzalloc(sizeof(struct hybridswapd_operations),
				  GFP_KERNEL);
	if (!hybridswapd_ops)
		goto error_out;

	log_info("using hybridswapd\n");
	hybridswapd_ops_init(hybridswapd_ops);
	hybridswapd_ops->pre_init();
	ret = hybridswapd_ops->pre_init();
	if (ret) {
		log_err("pre init failed\n");
		goto fail_out;
	}

	ret = cgroup_add_legacy_cftypes(&memory_cgrp_subsys,
			hybridswapd_ops->memcg_legacy_files);
	if (ret) {
		log_info("add mem_cgroup_swapd_legacy_files failed!\n");
		goto fail_out;
	}
#endif
	ret = register_all_hooks();
	if (ret)
		goto fail_out;

	log_info("hybridswap inited success!\n");
	return 0;

fail_out:
#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops->pre_deinit();
	kfree(hybridswapd_ops);
#endif
error_out:
	if (hybridswap_cache) {
		kmem_cache_destroy(hybridswap_cache);
		hybridswap_cache = NULL;
	}
	return ret;
}

void __exit hybridswap_exit(void)
{
	unregister_all_hook();

#ifdef CONFIG_HYBRIDSWAP_SWAPD
	hybridswapd_ops->pre_deinit();
#endif

	if (hybridswap_cache) {
		kmem_cache_destroy(hybridswap_cache);
		hybridswap_cache = NULL;
	}
}
