/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */

#ifndef _CFBT_STATE_MACHINE_H
#define _CFBT_STATE_MACHINE_H

#include <linux/types.h>
#include <linux/atomic.h>

typedef enum {
	CFBT_STATE_NO_WORK,
	CFBT_STATE_PREPARE_WORK,
	CFBT_STATE_WORKING,
	CFBT_STATE_LEAVE_WORK,
	CFBT_STATE_FINISH_WORK
} cfbt_work_state_t;

void cfbt_set_no_work(void);
void cfbt_set_prepare_work(void);
void cfbt_set_working(void);
void cfbt_set_leave_work(void);
void cfbt_set_finish_work(void);

bool cfbt_is_state_invalid(void);

#endif /* _CFBT_STATE_MACHINE_H */

