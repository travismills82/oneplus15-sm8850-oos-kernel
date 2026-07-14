// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Oplus. All rights reserved.
 */

#ifndef __WAKER_IDENTIFY_H__
#define __WAKER_IDENTIFY_H__

int set_ttwu_callback(void (*entry_func)(struct task_struct *task));

#endif /*__WAKER_IDENTIFY_H__*/
