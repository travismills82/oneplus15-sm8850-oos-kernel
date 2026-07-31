/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM osvelte

#if !defined(_TRACE_HOOK_OSVELTE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_OSVELTE_H

#include <trace/hooks/vendor_hooks.h>

DECLARE_HOOK(oplus_mm_vh_set_or_clear_scene,
	TP_PROTO(bool set, unsigned long nr_bit),
	TP_ARGS(set, nr_bit));

#endif /* _TRACE_HOOK_OSVELTE_H */

#undef TRACE_INCLUDE_PATH
#if IS_ENABLED(CONFIG_OPLUS_VENDOR_QCOM)
#define TRACE_INCLUDE_PATH ../../vendor/oplus/kernel/mm/mm_osvelte
#elif IS_ENABLED(CONFIG_OPLUS_VENDOR_MTK)
/*
 * ugly code, mtk now use mm_osvelte under driver/dma-buf/heaps/mm_osvelte, but
 * compile this
 */
#define TRACE_INCLUDE_PATH ../../vendor/oplus/kernel/mm/mm_osvelte
#else
#define TRACE_INCLUDE_PATH ../../kernel_device_modules-6.12/drivers/dma-buf/heaps/mm_osvelte
#endif

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE mm-hooks

/* This part must be outside protection */
#include <trace/define_trace.h>
