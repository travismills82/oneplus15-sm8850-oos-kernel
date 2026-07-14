// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <soc/qcom/minidump.h>
#include "msm_vidc_core.h"
#include "msm_vidc_debug.h"
#include "msm_vidc_md.h"

extern struct msm_vidc_core *g_core;

static int __md_dump_fw_region(struct msm_vidc_core *core,
				const char *name, void *virt, u64 phys, u64 size)
{
	struct md_region md_entry = {0};
	int ret;

	if (!core->capabilities[SUPPORTS_MINIDUMP].value ||
	    !msm_minidump_enabled()) {
		d_vpr_e("%s: minidump is not enabled!\n", __func__);
		return 0;
	}

	strscpy(md_entry.name, name, sizeof(md_entry.name));
	md_entry.virt_addr = (uintptr_t)virt;
	md_entry.phys_addr = phys;
	md_entry.size = size;
	ret = msm_minidump_add_region(&md_entry);
	if (ret < 0) {
		d_vpr_e("%s: failed to add FW mem-region in minidump: %d\n", __func__, ret);
		return ret;
	}
	d_vpr_h("%s: added FW mem-region, virt_addr %#llx size %llu\n",
			__func__, md_entry.virt_addr, size);

	return ret;
}

static int __md_dump_hfi_queues(struct msm_vidc_core *core)
{
	struct va_md_entry entry = {0};
	int ret = 0;

	/* copy queues(cmd, msg, dbg) dump(along with headers) */
	entry.size = TOTAL_QSIZE;
	strscpy(entry.owner, "msm_vidc", sizeof(entry.owner));
	entry.cb = NULL;
	entry.vaddr = (unsigned long)core->iface_q_table.align_virtual_addr;
	/* Client needs to run below command in their boot-up script
	 * To collect queues dump and sfr buffer dump during kernel panic
	 *
	 * "echo 1 > /sys/kernel/va-minidump/msm_vidc/enable"
	 */
	ret = qcom_va_md_add_region(&entry);
	if (ret) {
		d_vpr_e("%s: failed to dump queues in minidump: %d\n", __func__, ret);
		return -EINVAL;
	}
	d_vpr_h("%s: added hfi queues memory region, virt_addr %#lx size %u\n",
			__func__, entry.vaddr, entry.size);

	/* copy sfr dump */
	entry.size = ALIGNED_SFR_SIZE;
	strscpy(entry.owner, "msm_vidc", sizeof(entry.owner));
	entry.cb = NULL;
	entry.vaddr = (unsigned long)core->sfr.align_virtual_addr;
	ret = qcom_va_md_add_region(&entry);
	if (ret) {
		d_vpr_e("%s: failed to dump SFR buffer in minidump: %d\n", __func__, ret);
		return -EINVAL;
	}
	d_vpr_h("%s: added SFR memory region, virt_addr %#lx size %u\n",
			__func__, entry.vaddr, entry.size);

	return ret;
}

/* This is the call back function invoked from mini dump driver
 * During kernel panic
 */
static int __md_callback(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct msm_vidc_core *core = g_core;

	if (!core)
		return -ENODEV;

	return __md_dump_hfi_queues(core);
}

static struct notifier_block __md_nb = {
	.priority = INT_MAX,
	.notifier_call = __md_callback,
};

static int __md_register(struct msm_vidc_core *core)
{
	int ret;

	if (!core->capabilities[SUPPORTS_MINIDUMP].value || !qcom_va_md_enabled())
		return 0;

	ret = qcom_va_md_register("msm_vidc", &__md_nb);
	if (ret) {
		d_vpr_e("%s: failed to register notifier with va_minidump: %d\n", __func__, ret);
		return -EINVAL;
	}

	return ret;
}

static int __md_unregister(struct msm_vidc_core *core)
{
	int ret;

	if (!core->capabilities[SUPPORTS_MINIDUMP].value || !qcom_va_md_enabled())
		return 0;

	ret = qcom_va_md_unregister("msm_vidc", &__md_nb);
	if (ret) {
		d_vpr_e("%s: failed to unregister notifier with va_minidump: %d\n", __func__, ret);
		return -EINVAL;
	}

	return ret;
}

static const struct msm_vidc_md_ops msm_md_ops = {
	.md_register        = __md_register,
	.md_unregister      = __md_unregister,
	.md_dump_fw_region  = __md_dump_fw_region,
	.md_dump_hfi_queues = __md_dump_hfi_queues,
};

const struct msm_vidc_md_ops *get_md_ops(void)
{
	return &msm_md_ops;
}
