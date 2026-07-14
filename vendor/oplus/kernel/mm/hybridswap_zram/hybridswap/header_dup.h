// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2025 Oplus. All rights reserved.
 */

#ifndef HYBRIDSWAP_HEADER_DUP_H
#define HYBRIDSWAP_HEADER_DUP_H

/* copy from mm/memcontrol.c */
static const unsigned int memcg_node_stat_items[] = {
	NR_INACTIVE_ANON,
	NR_ACTIVE_ANON,
	NR_INACTIVE_FILE,
	NR_ACTIVE_FILE,
	NR_UNEVICTABLE,
	NR_SLAB_RECLAIMABLE_B,
	NR_SLAB_UNRECLAIMABLE_B,
	WORKINGSET_REFAULT_ANON,
	WORKINGSET_REFAULT_FILE,
	WORKINGSET_ACTIVATE_ANON,
	WORKINGSET_ACTIVATE_FILE,
	WORKINGSET_RESTORE_ANON,
	WORKINGSET_RESTORE_FILE,
	WORKINGSET_NODERECLAIM,
	NR_ANON_MAPPED,
	NR_FILE_MAPPED,
	NR_FILE_PAGES,
	NR_FILE_DIRTY,
	NR_WRITEBACK,
	NR_SHMEM,
	NR_SHMEM_THPS,
	NR_FILE_THPS,
	NR_ANON_THPS,
	NR_KERNEL_STACK_KB,
	NR_PAGETABLE,
	NR_SECONDARY_PAGETABLE,
#ifdef CONFIG_SWAP
	NR_SWAPCACHE,
#endif
#ifdef CONFIG_NUMA_BALANCING
	PGPROMOTE_SUCCESS,
#endif
	PGDEMOTE_KSWAPD,
	PGDEMOTE_DIRECT,
	PGDEMOTE_KHUGEPAGED,
};

static const unsigned int memcg_stat_items[] = {
	MEMCG_SWAP,
	MEMCG_SOCK,
	MEMCG_PERCPU_B,
	MEMCG_VMALLOC,
	MEMCG_KMEM,
	MEMCG_ZSWAP_B,
	MEMCG_ZSWAPPED,
};

#define NR_MEMCG_NODE_STAT_ITEMS ARRAY_SIZE(memcg_node_stat_items)
#define MEMCG_VMSTAT_SIZE (NR_MEMCG_NODE_STAT_ITEMS + \
			   ARRAY_SIZE(memcg_stat_items))
#define BAD_STAT_IDX(index) ((u32)(index) >= U8_MAX)

/* Subset of vm_event_item to report for memcg event stats */
static const unsigned int memcg_vm_event_stat[] = {
#ifdef CONFIG_MEMCG_V1
	PGPGIN,
	PGPGOUT,
#endif
	PGSCAN_KSWAPD,
	PGSCAN_DIRECT,
	PGSCAN_KHUGEPAGED,
	PGSTEAL_KSWAPD,
	PGSTEAL_DIRECT,
	PGSTEAL_KHUGEPAGED,
	PGFAULT,
	PGMAJFAULT,
	PGREFILL,
	PGACTIVATE,
	PGDEACTIVATE,
	PGLAZYFREE,
	PGLAZYFREED,
#ifdef CONFIG_SWAP
	SWPIN_ZERO,
	SWPOUT_ZERO,
#endif
#ifdef CONFIG_ZSWAP
	ZSWPIN,
	ZSWPOUT,
	ZSWPWB,
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	THP_FAULT_ALLOC,
	THP_COLLAPSE_ALLOC,
	THP_SWPOUT,
	THP_SWPOUT_FALLBACK,
#endif
#ifdef CONFIG_NUMA_BALANCING
	NUMA_PAGE_MIGRATE,
	NUMA_PTE_UPDATES,
	NUMA_HINT_FAULTS,
#endif
};

#define NR_MEMCG_EVENTS ARRAY_SIZE(memcg_vm_event_stat)

struct memcg_vmstats_percpu {
	/* Stats updates since the last flush */
	unsigned int			stats_updates;

	/* Cached pointers for fast iteration in memcg_rstat_updated() */
	struct memcg_vmstats_percpu	*parent;
	struct memcg_vmstats		*vmstats;

	/* The above should fit a single cacheline for memcg_rstat_updated() */

	/* Local (CPU and cgroup) page state & events */
	long			state[MEMCG_VMSTAT_SIZE];
	unsigned long		events[NR_MEMCG_EVENTS];

	/* Delta calculation for lockless upward propagation */
	long			state_prev[MEMCG_VMSTAT_SIZE];
	unsigned long		events_prev[NR_MEMCG_EVENTS];
} ____cacheline_aligned;
#endif /* HYBRIDSWAP_HEADER_DUP_H */
