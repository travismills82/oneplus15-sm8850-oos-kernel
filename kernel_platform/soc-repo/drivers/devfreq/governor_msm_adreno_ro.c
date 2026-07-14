// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/devfreq.h>
#include <linux/slab.h>

#include "drivers/devfreq/governor.h"

static int adreno_passive_get_target_freq(struct devfreq *devfreq,
					   unsigned long *freq)
{
	return 0;
}

static int adreno_passive_event_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	return 0;
}

static struct devfreq_governor msm_adreno_ro = {
	.name = "msm-adreno-ro",
	.flags = DEVFREQ_GOV_FLAG_IMMUTABLE,
	.get_target_freq = adreno_passive_get_target_freq,
	.event_handler = adreno_passive_event_handler,
};

int msm_adreno_passive_init(void)
{
	return devfreq_add_governor(&msm_adreno_ro);
}
subsys_initcall(msm_adreno_passive_init);

void msm_adreno_passive_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&msm_adreno_ro);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);
}
module_exit(msm_adreno_passive_exit);

MODULE_LICENSE("GPL");
