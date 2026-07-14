/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _GH_CTRL_H
#define _GH_CTRL_H

struct gh_virt_time_offset {
	u64 offset;
	u64 ns;
};

#if IS_ENABLED(CONFIG_GUNYAH_GH_CTRL)
static inline void
gh_get_virt_time_offset(struct gh_virt_time_offset *virt_time_offset)
{
	return 0;
}
#else
void gh_get_virt_time_offset(struct gh_virt_time_offset *virt_time_offset);
#endif

#endif
