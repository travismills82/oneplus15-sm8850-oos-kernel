// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _GEAS_CPU_DEBUG_H
#define _GEAS_CPU_DEBUG_H
#define MAX_PIPLINE_DEBUG_PARA_LEN 60
extern int pipline_debug_para_list[MAX_PIPLINE_DEBUG_PARA_LEN];
int update_pipline_debug_para(const struct ctl_table *table, int write, void __user *buffer,  unsigned long *lenp, long long *ppos);

#endif  /* _GEAS_CPU_DEBUG_H */
