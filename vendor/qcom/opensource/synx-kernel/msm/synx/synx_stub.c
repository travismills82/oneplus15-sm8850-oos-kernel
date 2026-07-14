// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/module.h>
#include "synx_api.h"
#include "synx_hwfence.h"
#include "synx_private.h"
#include "synx_debugfs.h"

int synx_debug = SYNX_ERR | SYNX_WARN | SYNX_INFO;

struct synx_session *synx_internal_initialize(
	struct synx_initialization_params *params)
{
	return NULL;
}

int synx_internal_recover(enum synx_client_id id)
{
	return -SYNX_NOSUPPORT;
}

static int __init synx_init(void)
{
	return synx_hwfence_init_ops(&synx_hwfence_ops);
}

module_init(synx_init);
MODULE_DESCRIPTION("Global Synx Driver");
MODULE_LICENSE("GPL");
