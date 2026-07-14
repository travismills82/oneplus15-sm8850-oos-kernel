/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __GH_RM_HEAP_MANAGER_H
#define __GH_RM_HEAP_MANAGER_H

#if IS_ENABLED(CONFIG_GH_RM_HEAP_MANAGER)

/* gets total hypervisor heap added by hlos */
uint64_t gh_rm_heap_get_total_heap_added(void);

/* gets total hypervisor free heap memory */
uint64_t gh_rm_heap_get_hyp_heap_free(void);

/* add hypervisor heap for the given dmabuf_size being assigned */
void gh_rm_heap_memlend_prealloc(size_t dmabuf_size);

/* remove all free hypervisor memory */
void gh_rm_heap_shrink(void);

#else
static inline uint64_t gh_rm_heap_get_total_heap_added(void)
{
	return 0;
}

static inline uint64_t gh_rm_heap_get_hyp_heap_free(void)
{
	return 0;
}

static inline void gh_rm_heap_memlend_prealloc(size_t dmabuf_size) { }

static inline void gh_rm_heap_shrink(void) { }

#endif //CONFIG_GH_RM_HEAP_MANAGER
#endif
