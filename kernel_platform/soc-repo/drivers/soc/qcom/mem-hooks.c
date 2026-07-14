// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Took is_el1_instruction_abort() from arch/arm64/mm/fault.c
 * Copyright (C) 2012 ARM Ltd
 */

#include <linux/module.h>
#include <asm/esr.h>
#include <asm/ptrace.h>
#include <linux/cma.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>

#define CREATE_TRACE_POINTS
#include <trace/events/cma.h>
#undef CREATE_TRACE_POINTS
#include <trace/hooks/mm.h>
#include <trace/hooks/fault.h>
#include "mm/cma.h"

static bool is_el1_instruction_abort(unsigned long esr)
{
	return ESR_ELx_EC(esr) == ESR_ELx_EC_IABT_CUR;
}

static void can_fixup_sea(void *unused, unsigned long addr, unsigned long esr,
			  struct pt_regs *regs, bool *can_fixup)
{
	if (!user_mode(regs) && !is_el1_instruction_abort(esr))
		*can_fixup = true;
	else
		*can_fixup = false;
}

static void bitmap_find_best_next_zero_area_off(void *data,
						unsigned long *bitmap,
						unsigned long bitmap_maxno,
						unsigned long start,
						unsigned int bitmap_count,
						unsigned long mask,
						unsigned long offset,
						unsigned long *bitmap_no,
						bool best_fit)
{
	if (!best_fit || !IS_ENABLED(CONFIG_CMA_BEST_FIT))
		return;

	unsigned long start_bit;
	unsigned long bitmap_index;
	unsigned long bitmap_len;
	unsigned long next_bit;

	start_bit = bitmap_maxno;
	bitmap_len = bitmap_maxno + 1;
	bitmap_index = bitmap_find_next_zero_area_off(bitmap,
			bitmap_maxno, start, bitmap_count, mask,
			offset);

	while (bitmap_index < bitmap_maxno) {
		next_bit = find_next_bit(bitmap, bitmap_maxno, bitmap_index + bitmap_count);
		if ((next_bit - bitmap_index) < bitmap_len) {
			bitmap_len = next_bit - bitmap_index;
			start_bit = bitmap_index;
			if (bitmap_len == bitmap_maxno)
				goto end;
			if (bitmap_len == bitmap_count) {
				bitmap_no = &start_bit;
				return;
			}
		}
		bitmap_index = bitmap_find_next_zero_area_off(bitmap,
				bitmap_maxno, next_bit + 1, bitmap_count, mask, offset);
	}
end:
	*bitmap_no = start_bit;
}

#ifdef CONFIG_CMA_BEST_FIT
static int cma_best_fit_setup(struct cma *cma, void *data)
{
	struct device_node *rmem_node, *rmem;

	rmem_node = of_find_node_by_path("/reserved-memory");
	if (!rmem_node) {
		pr_err("Failed to find reserved-memory node\n");
		return -ENODEV;
	}

	for_each_child_of_node(rmem_node, rmem) {
		if (!strcmp(rmem->name, cma->name)) {
			spin_lock_irq(&cma->lock);
			cma->android_vendor_data1 =
				(of_get_property(rmem, "cma-best-fit", NULL) ? true : false);
			spin_unlock_irq(&cma->lock);
		}
	}

	return 0;
}
#else
static int cma_best_fit_setup(struct cma *cma, void *data)
{
	return 0;
}
#endif

static void register_cma_hooks(void)
{
	register_trace_android_rvh_bitmap_find_best_next_area
			(bitmap_find_best_next_zero_area_off, NULL);
	cma_for_each_area(cma_best_fit_setup, NULL);
}

static int __init init_mem_hooks(void)
{
	int ret;

	ret = register_trace_android_vh_try_fixup_sea(can_fixup_sea, NULL);
	if (ret) {
		pr_err("Failed to register try_fixup_sea\n");
		return ret;
	}
	register_cma_hooks();

	return 0;
}

module_init(init_mem_hooks);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. Memory Trace Hook Call-Back Registration");
MODULE_LICENSE("GPL");
