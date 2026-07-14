// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <trace/hooks/dtask.h>
#include "kernel/locking/mutex.h"
#include "walt.h"

static void android_vh_alter_mutex_list_add(void *unused, struct mutex *lock,
				struct mutex_waiter *waiter, struct list_head *list,
				bool *already_on_list)
{
	struct walt_task_struct *wts_waiter =
		(struct walt_task_struct *)android_task_vendor_data(current);
	struct mutex_waiter *pos = NULL;
	struct mutex_waiter *n = NULL;
	struct list_head *head = list;
	struct walt_task_struct *wts;

	if (unlikely(walt_disabled))
		return;

	if (!lock || !waiter || !list)
		return;

	if (!is_mvp(wts_waiter))
		return;

	list_for_each_entry_safe(pos, n, head, list) {
		wts = (struct walt_task_struct *)
			android_task_vendor_data((struct task_struct *)(pos->task));
		if (!is_mvp(wts)) {
			list_add(&waiter->list, pos->list.prev);
			*already_on_list = true;
			break;
		}
	}
}

void walt_mvp_lock_ordering_init(void)
{
	register_trace_android_vh_alter_mutex_list_add(android_vh_alter_mutex_list_add, NULL);
}
