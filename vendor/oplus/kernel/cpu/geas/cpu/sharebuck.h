// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _GEAS_SHAREBUCK_H
#define _GEAS_SHAREBUCK_H

enum share_buck_type {
	TYPE_NONE,
	TYPE_INTERCLUSTER,
	TYPE_CLUSTER_DSU,
};

unsigned long get_freq_vol_mv(unsigned long freq, struct geas_opp_table *table, int count);
long shb_pd_compute_energy_multitask_by_cpu(struct shb_energy_env *env,
	struct geas_sched_cluster_info *cl, int type, u16 def_em_type);
int sharebuck_init(void);
#endif /* _GEAS_SHAREBUCK_H */
