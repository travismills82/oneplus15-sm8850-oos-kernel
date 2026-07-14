// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _PIPLINE_EAS_H
#define _PIPLINE_EAS_H

#include <linux/sched.h>

extern int pipline_debug_mask;
extern int pipline_mem_debug_ratio[];
extern int pipline_debug_ratio_1[];

extern u64 pipline_select_cnt[MAX_CPU_MAP_NUM];
extern struct mutex pipline_mutex;
int pipline_update_load_info_by_cpu(struct multitask_energy_env *multitask_env, struct multitask_load_info *load_info);
int pipline_find_energy_efficient_cpu(struct multitask_result *result, struct multitask_energy_env *multitask_env);

#endif /* _PIPLINE_EAS_H */
