/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#if IS_ENABLED(CONFIG_SCHED_WALT_STATS)
/* debug counter */
extern atomic64_t walt_active_balance_migration_counter[WALT_NR_CPUS];
extern atomic64_t walt_idle_balance_migration_counter[WALT_NR_CPUS];

extern void assign_reasons_counter(struct waltgov_policy *wg_policy);
extern void rollover_freq_ns_stats(u64 value);
extern void update_min_max_freq_ns(struct cpufreq_policy *policy);

/* cpufreq  stats */
static inline void walt_inc_active_balance_migration_counter(int cpu)
{
	atomic64_inc(&walt_active_balance_migration_counter[cpu]);
}

static inline void walt_inc_idle_balance_migration_counter(int cpu)
{
	atomic64_inc(&walt_idle_balance_migration_counter[cpu]);
}

extern void walt_stats_sysctl_init(void);
extern void walt_stats_freq_counter_init(void);
#else
static inline void assign_reasons_counter(struct waltgov_policy *wg_policy) { }
static inline void rollover_freq_ns_stats(u64 value) { }
static inline void update_min_max_freq_ns(struct cpufreq_policy *policy) { }
static inline void update_ns_stats(void) { }
static inline void walt_inc_active_balance_migration_counter(int cpu) { }
static inline void walt_inc_idle_balance_migration_counter(int cpu) { }
static inline void walt_stats_sysctl_init(void) { }
#endif
