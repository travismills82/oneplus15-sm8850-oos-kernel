// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/iopoll.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif
#include <linux/soc/qcom/smem.h>

#include "msm_vidc_core.h"
#include "msm_vidc_driver.h"
#include "msm_vidc_state.h"
#include "msm_vidc_debug.h"
#include "msm_vidc_variant.h"
#include "msm_vidc_platform.h"
#include "msm_vidc_events.h"
#include "venus_hfi.h"
#include "resources.h"

enum video_memory_region {
	VIDEO_REGION_SECURE_FW_REGION_ID    = 0,
	VIDEO_REGION_VM0_SECURE_NP_ID       = 1,
	VIDEO_REGION_VM1_SECURE_NP_ID       = 2,
	VIDEO_REGION_VM2_SECURE_NP_ID       = 3,
	VIDEO_REGION_VM3_SECURE_NP_ID       = 4,
	VIDEO_REGION_VM0_NONSECURE_NP_ID    = 5,
	VIDEO_REGION_VM1_NONSECURE_NP_ID    = 6,
	VIDEO_REGION_VM2_NONSECURE_NP_ID    = 7,
	VIDEO_REGION_VM3_NONSECURE_NP_ID    = 8
};

int __write_register(struct msm_vidc_core *core, u32 reg, u32 value)
{
	u32 hwiosymaddr = reg;
	u8 *base_addr;
	int rc = 0;

	rc = __strict_check(core, __func__);
	if (rc)
		return rc;

	if (!is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE)) {
		d_vpr_e("HFI Write register failed : Power is OFF\n");
		return -EINVAL;
	}

	base_addr = core->resource->register_base_addr;
	d_vpr_l("regwrite(%pK + %#x) = %#x\n", base_addr, hwiosymaddr, value);
	base_addr += hwiosymaddr;
	writel_relaxed(value, base_addr);

	/* Memory barrier to make sure value is written into the register */
	wmb();

	return rc;
}

/*
 * Argument mask is used to specify which bits to update. In case mask is 0x11,
 * only bits 0 & 4 will be updated with corresponding bits from value. To update
 * entire register with value, set mask = 0xFFFFFFFF.
 */
int __write_register_masked(struct msm_vidc_core *core, u32 reg, u32 value,
			    u32 mask)
{
	u32 prev_val, new_val;
	u8 *base_addr;
	int rc = 0;

	rc = __strict_check(core, __func__);
	if (rc)
		return rc;

	if (!is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE)) {
		d_vpr_e("%s: register write failed, power is off\n",
			__func__);
		return -EINVAL;
	}

	base_addr = core->resource->register_base_addr;
	base_addr += reg;

	prev_val = readl_relaxed(base_addr);
	/*
	 * Memory barrier to ensure register read is correct
	 */
	rmb();

	new_val = (prev_val & ~mask) | (value & mask);
	d_vpr_l(
		"Base addr: %pK, writing to: %#x, previous-value: %#x, value: %#x, mask: %#x, new-value: %#x...\n",
		base_addr, reg, prev_val, value, mask, new_val);
	writel_relaxed(new_val, base_addr);
	/*
	 * Memory barrier to make sure value is written into the register.
	 */
	wmb();

	return rc;
}

int __read_register(struct msm_vidc_core *core, u32 reg, u32 *value)
{
	int rc = 0;
	u8 *base_addr;

	if (!is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE)) {
		d_vpr_e("HFI Read register failed : Power is OFF\n");
		return -EINVAL;
	}

	base_addr = core->resource->register_base_addr;

	*value = readl_relaxed(base_addr + reg);
	/*
	 * Memory barrier to make sure value is read correctly from the
	 * register.
	 */
	rmb();
	d_vpr_l("regread(%pK + %#x) = %#x\n", base_addr, reg, *value);

	return rc;
}

int __read_register_with_poll_timeout(struct msm_vidc_core *core, u32 reg,
				      u32 mask, u32 exp_val, u32 sleep_us,
				      u32 timeout_us)
{
	int rc = 0;
	u32 val = 0;
	u8 *addr;

	if (!is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE)) {
		d_vpr_e("%s failed: Power is OFF\n", __func__);
		return -EINVAL;
	}

	addr = (u8 *)core->resource->register_base_addr + reg;

	rc = readl_relaxed_poll_timeout(addr, val, ((val & mask) == exp_val), sleep_us, timeout_us);
	/*
	 * Memory barrier to make sure value is read correctly from the
	 * register.
	 */
	rmb();
	d_vpr_l(
		"regread(%pK + %#x) = %#x. rc %d, mask %#x, exp_val %#x, cond %u, sleep %u, timeout %u\n",
		core->resource->register_base_addr, reg, val, rc, mask, exp_val,
		((val & mask) == exp_val), sleep_us, timeout_us);

	return rc;
}

int __set_registers(struct msm_vidc_core *core)
{
	const struct reg_preset_table *reg_prst;
	unsigned int prst_count;
	int cnt, rc = 0;

	reg_prst = core->platform->data.reg_prst_tbl;
	prst_count = core->platform->data.reg_prst_tbl_size;

	/* skip if there is no preset reg available */
	if (!reg_prst || !prst_count)
		return 0;

	for (cnt = 0; cnt < prst_count; cnt++) {
		rc = __write_register_masked(core, reg_prst[cnt].reg,
				reg_prst[cnt].value, reg_prst[cnt].mask);
		if (rc)
			return rc;
	}

	return rc;
}

int msm_vidc_mem_protect_video_regions_v1(struct msm_vidc_core *core)
{
	int rc = 0;
	struct context_bank_info *cb;
	u32 cp_start = 0, cp_size = 0, cp_nonpixel_start = 0, cp_nonpixel_size = 0;

	venus_hfi_for_each_context_bank(core, cb) {
		if (cb->region & MSM_VIDC_NON_SECURE) {
			cp_size = cb->addr_range.start;

			d_vpr_h("%s: cp_size: %#x\n",
				__func__, cp_size);
		}

		if (cb->region & MSM_VIDC_SECURE_NONPIXEL) {
			cp_nonpixel_start = cb->addr_range.start;
			cp_nonpixel_size = cb->addr_range.size;

			d_vpr_h("%s: cp_nonpixel_start: %#x size: %#x\n",
				__func__, cp_nonpixel_start,
				cp_nonpixel_size);
		}
	}

	rc = qcom_scm_mem_protect_video_var(cp_start, cp_size,
			cp_nonpixel_start, cp_nonpixel_size);
	if (rc) {
		d_vpr_e("Failed to protect memory(%d)\n", rc);
		return rc;
	}

	trace_venus_hfi_var_done(cp_start, cp_size, cp_nonpixel_start,
			cp_nonpixel_size);

	return rc;
}

int msm_vidc_mem_protect_video_regions_v2(struct msm_vidc_core *core)
{
	int rc = 0;
	struct context_bank_info *cb;
	int region = -1;
	u32 start = 0, size = 0;

	venus_hfi_for_each_context_bank(core, cb) {

		if (cb->region & MSM_VIDC_NON_SECURE)
			region = VIDEO_REGION_VM0_NONSECURE_NP_ID;
		else if (cb->region & MSM_VIDC_SECURE_NONPIXEL)
			region = VIDEO_REGION_VM0_SECURE_NP_ID;
		else
			continue;

		start = cb->addr_range.start;
		size = cb->addr_range.size;

		rc = qcom_scm_mem_protect_video_var(region, 0, start, size);
		if (rc) {
			d_vpr_e("%s Failed to protect memory(%d)\n", __func__, rc);
			return rc;
		}

		trace_venus_hfi_var_done(region, 0, start, size);
	}

	return rc;
}
