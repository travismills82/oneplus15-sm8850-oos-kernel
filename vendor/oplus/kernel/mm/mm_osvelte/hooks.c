// SPDX-License-Identifier: GPL-2.0-only
/* hooks.c
 *
 * oplus mm osvelte hooks
 *
 * Copyright (C) 2025 Oplus. All rights reserved.
 */

#define CREATE_TRACE_POINTS
#include <trace/hooks/vendor_hooks.h>
#include <linux/tracepoint.h>

#include "mm-hooks.h"

EXPORT_TRACEPOINT_SYMBOL_GPL(oplus_mm_vh_set_or_clear_scene);
