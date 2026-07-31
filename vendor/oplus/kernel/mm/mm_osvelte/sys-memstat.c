// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#include <linux/fdtable.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <trace/hooks/mm.h>
#include <trace/hooks/vmscan.h>

#include "common.h"
#include "internal.h"
#include "memstat.h"
#include "sys-memstat.h"
#include "mm-config.h"
#include "mm-trace.h"

static struct proc_dir_entry *mtrack_procs[MTRACK_MAX];
static struct mtrack_debugger *mtrack_debugger[MTRACK_MAX];
static atomic64_t mtrack_vh_pages[MTRACK_VH_MAX];
static unsigned long kswapd_dump_meminfo_jiffies;
static struct osvelte_reclaim_stat kwapd_reclaim_stat;
static struct tracepoint *tracepoint_mm_vmscan_lru_shrink_inactive_dup;
static struct tracepoint *tracepoint_mm_vmscan_wakeup_kswap_dup;
static bool ezr_enabled;

static void show_val_kb(struct seq_file *m, const char *s, unsigned long num)
{
	seq_put_decimal_ull_width_dup(m, s, num << (PAGE_SHIFT - 10), 8);
	seq_write(m, " kB\n", 4);
}

void unregister_mtrack_debugger(enum mtrack_type type,
				struct mtrack_debugger *debugger)
{
	mtrack_debugger[type] = NULL;
}
EXPORT_SYMBOL_GPL(unregister_mtrack_debugger);

int register_mtrack_debugger(enum mtrack_type type,
			     struct mtrack_debugger *debugger)
{
	if (!debugger)
		return -EINVAL;

	if (mtrack_debugger[type])
		return -EEXIST;

	mtrack_debugger[type] = debugger;
	return 0;
}
EXPORT_SYMBOL_GPL(register_mtrack_debugger);

int register_mtrack_procfs(enum mtrack_type t, const char *name, umode_t mode,
			   const struct proc_ops *proc_ops, void *data)
{
	struct proc_dir_entry *entry;

	if (!mtrack_procs[t])
		return -EBUSY;

	entry = proc_create_data(name, mode, mtrack_procs[t], proc_ops, data);
	if (!entry)
		return -ENOMEM;

	return 0;
}
EXPORT_SYMBOL_GPL(register_mtrack_procfs);

void unregister_mtrack_procfs(enum mtrack_type t, const char *name)
{
	if (!unlikely(mtrack_procs[t]))
		return;

	remove_proc_subtree(name, mtrack_procs[t]);
}
EXPORT_SYMBOL_GPL(unregister_mtrack_procfs);

long read_mtrack_vh_mem_usage(enum mtrack_vh_type t)
{
	long val = atomic64_read(&mtrack_vh_pages[t]);

	/*
	 * osvelte module might loaded after other modules which use shmem or
	 * cma, so the val mey smaller than 0, return 0 instead.
	 */
	return val > 0 ? val : 0;
}

long read_mtrack_mem_usage(enum mtrack_type t, enum mtrack_subtype s)
{
	struct mtrack_debugger *d = mtrack_debugger[t];

	if (d && d->mem_usage)
		return d->mem_usage(s);
	return 0;
}
EXPORT_SYMBOL_GPL(read_mtrack_mem_usage);

long read_pid_mtrack_mem_usage(enum mtrack_type t,
				      enum mtrack_subtype s, pid_t pid)
{
	struct mtrack_debugger *d = mtrack_debugger[t];

	if (d && d->pid_mem_usage)
		return d->pid_mem_usage(s, pid);
	return 0;
}

void dump_mtrack_usage_stat(enum mtrack_type t, bool verbose)
{
	struct mtrack_debugger *d = mtrack_debugger[t];

	if (d && d->dump_usage_stat) {
		osvelte_info("======= dump_%s\n", mtrack_text[t]);
		return d->dump_usage_stat(verbose);
	}
}

static void extra_meminfo_proc_show(void *data, struct seq_file *m)
{
	show_val_kb(m, "IonTotalCache:  ",
			read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_POOL));
	show_val_kb(m, "IonTotalUsed:   ",
			read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_SYSTEM_HEAP));
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_MM_BOOSTPOOL)
	show_val_kb(m, "RsvPool:        ",
			read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_BOOST_POOL));
#endif
	show_val_kb(m, "GPUTotalUsed:   ",
			read_mtrack_mem_usage(MTRACK_GPU, MTRACK_GPU_TOTAL));
	show_val_kb(m, "ShmemSwapped:   ",
		    read_mtrack_vh_mem_usage(MTRACK_VH_SHMEM_SWAPED));
	show_val_kb(m, "UxPool:         ",
		    read_mtrack_mem_usage(MTRACK_UXMEM_POOL, MTRACK_UXMEM_POOL_TOTAL));
	if (!ezr_enabled)
		return;
	show_val_kb(m, "Ezr:            ",
			read_mtrack_mem_usage(MTRACK_ERM, MTRACK_ERM_LRU));
	show_val_kb(m, "EzrFree:        ",
			read_mtrack_mem_usage(MTRACK_ERM, MTRACK_ERM_FREE));
}

static void osvelte_vh_vmscan_wakeup_kswapd(void *data, int nid, int zid, int order, gfp_t gfp_flags)
{
	if (order < 2)
		return;
	mm_trace_fmt_instant_body("%d:%d@%d", OMTE_KWAPD_WAKEUP_HIGH_ORDER, nid, zid, order);
}

static void osvelte_vh_vmscan_lru_shrink_inactive(void *data, int nid,
						  unsigned long nr_scanned,
						  unsigned long nr_reclaimed,
						  struct reclaim_stat *stat,
						  int priority, int file)
{
	struct osvelte_reclaim_stat *rc;
	int index;

	if (!current_is_kswapd() || current->comm[2] != 'w')
		return;

	rc = &kwapd_reclaim_stat;
	index = file ? 1 : 0;
	rc->pgscan[index] += nr_scanned;
	rc->pgsteal[index] += nr_reclaimed;
}

static void dump_kwapd_reclaim_stat(void)
{
	struct osvelte_reclaim_stat *rc = &kwapd_reclaim_stat;

	mm_trace_fmt_int64(rc->pgscan[0], "%d:A1_pgscan_anon", OMTE_KWAPD_RECLAIM);
	mm_trace_fmt_int64(rc->pgsteal[0], "%d:A2_pgsteal_anon", OMTE_KWAPD_RECLAIM);
	mm_trace_fmt_int64(rc->pgscan[1], "%d:B1_pgscan_file", OMTE_KWAPD_RECLAIM);
	mm_trace_fmt_int64(rc->pgsteal[1], "%d:B2_pgsteal_file", OMTE_KWAPD_RECLAIM);
}

static void osvelte_vh_vmscan_kswapd_done(void *data, int node_id, unsigned int highest_zoneidx, unsigned int alloc_order, unsigned int reclaim_order)
{
	mm_trace_fmt_instant_body("%d:%d@%d", OMTE_KWAPD_RECLAIM, highest_zoneidx, alloc_order);
	dump_kwapd_reclaim_stat();
}

static void osvelte_vh_tune_swappiness(void *data, int *swappiness)
{
	struct sysinfo si;

	/* kshrink_slabd also set this PF_KSWAPD. */
	if (!current_is_kswapd() || current->comm[2] != 'w')
		return;

	if (time_before(jiffies, kswapd_dump_meminfo_jiffies + msecs_to_jiffies(100)))
		return;

	si_swapinfo(&si);
	mm_trace_int64(OMTE_COMMON_STRING"AA_available", K(si_mem_available()));
	mm_trace_int64(OMTE_COMMON_STRING"AB_free", K(sys_freeram()));
	mm_trace_int64(OMTE_COMMON_STRING"AC_active_file", K(sys_active_file()));
	mm_trace_int64(OMTE_COMMON_STRING"AC_inactive_file", K(sys_inactive_file()));
	mm_trace_int64(OMTE_COMMON_STRING"AD_active_anon", K(sys_active_anon()));
	mm_trace_int64(OMTE_COMMON_STRING"AD_inactive_anon", K(sys_inactive_anon()));
	mm_trace_int64(OMTE_COMMON_STRING"AD_swap_used", K(si.totalswap - si.freeswap));
	mm_trace_int64(OMTE_COMMON_STRING"AE1_dmabuf_pool", K(read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_POOL)));
	mm_trace_int64(OMTE_COMMON_STRING"AE2_dmabuf_boost_pool", K(read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_BOOST_POOL)));
	mm_trace_int64(OMTE_COMMON_STRING"AE3_dmabuf", K(read_mtrack_mem_usage(MTRACK_DMABUF, MTRACK_DMABUF_SYSTEM_HEAP)));
	mm_trace_int64(OMTE_COMMON_STRING"AF1_ezr_free", K(read_mtrack_mem_usage(MTRACK_ERM, MTRACK_ERM_FREE)));
	mm_trace_int64(OMTE_COMMON_STRING"AF2_ezr", K(read_mtrack_mem_usage(MTRACK_ERM, MTRACK_ERM_LRU)));
	mm_trace_int64(OMTE_COMMON_STRING"AG_gpu_total", K(read_mtrack_mem_usage(MTRACK_GPU, MTRACK_GPU_TOTAL)));
	mm_trace_int64(OMTE_COMMON_STRING"AH_uxpool", K(read_mtrack_mem_usage(MTRACK_UXMEM_POOL, MTRACK_UXMEM_POOL_TOTAL)));
	dump_kwapd_reclaim_stat();

	kswapd_dump_meminfo_jiffies = jiffies;
}

static void osvelte_vh_shmem_mod_swapped(void *data, struct address_space *mapping,
					 long nr_pages)
{
	atomic64_add(nr_pages, &mtrack_vh_pages[MTRACK_VH_SHMEM_SWAPED]);
}

int sys_memstat_init(struct proc_dir_entry *root)
{
	struct proc_dir_entry *dir_entry;
	struct config_ezreclaimd *config;
	int i;

	if (register_trace_android_vh_meminfo_proc_show(extra_meminfo_proc_show, NULL)) {
		pr_err("register extra meminfo proc failed.\n");
		return -EINVAL;
	}

	/* if failed, not fatal error */
	tracepoint_mm_vmscan_lru_shrink_inactive_dup = osvelte_kallsyms_lookup_name("__tracepoint_mm_vmscan_lru_shrink_inactive");
	if (tracepoint_mm_vmscan_lru_shrink_inactive_dup)
		tracepoint_probe_register(tracepoint_mm_vmscan_lru_shrink_inactive_dup, osvelte_vh_vmscan_lru_shrink_inactive, NULL);
	tracepoint_mm_vmscan_wakeup_kswap_dup = osvelte_kallsyms_lookup_name("__tracepoint_mm_vmscan_wakeup_kswap");
	if (tracepoint_mm_vmscan_wakeup_kswap_dup)
		tracepoint_probe_register(tracepoint_mm_vmscan_wakeup_kswap_dup, osvelte_vh_vmscan_wakeup_kswapd, NULL);

	register_trace_android_vh_vmscan_kswapd_done(osvelte_vh_vmscan_kswapd_done, NULL);
	register_trace_android_vh_tune_swappiness(osvelte_vh_tune_swappiness, NULL);
	register_trace_android_vh_shmem_mod_swapped(osvelte_vh_shmem_mod_swapped, NULL);

	/* create mtrack dir here */
	for (i = 0; i < MTRACK_MAX; i++) {
		mtrack_procs[i] = proc_mkdir(mtrack_text[i], root);
		if (!mtrack_procs[i]) {
			osvelte_err("proc_fs: create %s failed\n",
				    mtrack_text[i]);
		}
	}

	dir_entry = mtrack_procs[MTRACK_DMABUF];
	if (dir_entry)
		create_dmabuf_procfs(dir_entry);

	dir_entry = mtrack_procs[MTRACK_ASHMEM];
	if (dir_entry)
		create_ashmem_procfs(dir_entry);

	config = oplus_read_mm_config(module_name_ezreclaimd);
	if (config)
		ezr_enabled = config->enable;
	return 0;
}

int sys_memstat_exit(void)
{
	remove_proc_subtree(DEV_NAME, NULL);
	unregister_trace_android_vh_meminfo_proc_show(extra_meminfo_proc_show, NULL);
	return 0;
}
