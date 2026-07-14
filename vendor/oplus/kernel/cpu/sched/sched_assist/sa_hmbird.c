// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */

#include <linux/sched.h>
#include <kernel/sched/sched.h>
#include "sa_hmbird.h"

#ifdef CONFIG_HMBIRD_SCHED_BPF
bool is_hmbird_enabled(void)
{
	return scx_enabled();
}
#else
bool is_hmbird_enabled(void)
{
	return false;
}
#endif

