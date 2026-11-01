/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _KGSL_OSVELTE_LOG_H
#define _KGSL_OSVELTE_LOG_H

/* Preserve the OPlus log prefix without importing unrelated task flags. */
#define osvelte_info(fmt, ...) pr_info("osvelte: " fmt, ##__VA_ARGS__)

#endif
