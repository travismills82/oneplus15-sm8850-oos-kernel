// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/types.h>
#include <linux/list.h>
#include <linux/of_address.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/soc/qcom/smem.h>
#include <linux/devcoredump.h>
#include <linux/firmware.h>

#include "msm_vidc_core.h"
#include "msm_vidc_debug.h"
#include "msm_vidc_platform.h"
#include "msm_vidc_md.h"
#include "firmware.h"
#include "resources.h"

#define MAX_FIRMWARE_NAME_SIZE	128

enum tzbsp_video_state {
	TZBSP_VIDEO_STATE_SUSPEND = 0,
	TZBSP_VIDEO_STATE_RESUME = 1,
	TZBSP_VIDEO_STATE_RESTORE_THRESHOLD = 2,
};

static int __load_fw_to_memory(struct platform_device *pdev,
			const char *fw_name)
{
	int rc = 0;
	const struct firmware *firmware = NULL;
	struct msm_vidc_core *core;
	char firmware_name[MAX_FIRMWARE_NAME_SIZE] = { 0 };
	struct device_node *node = NULL;
	struct resource res = { 0 };
	phys_addr_t phys = 0;
	size_t res_size = 0;
	ssize_t fw_size = 0;
	void *virt = NULL;
	int pas_id = 0;

	if (!fw_name || !(*fw_name) || !pdev) {
		d_vpr_e("%s: Invalid inputs\n", __func__);
		return -EINVAL;
	}
	if (strlen(fw_name) >= MAX_FIRMWARE_NAME_SIZE - 4) {
		d_vpr_e("%s: Invalid fw name\n", __func__);
		return -EINVAL;
	}

	core = dev_get_drvdata(&pdev->dev);
	if (!core) {
		d_vpr_e("%s: core not found in device %s",
			__func__, dev_name(&pdev->dev));
		return -EINVAL;
	}
	scnprintf(firmware_name, ARRAY_SIZE(firmware_name), "%s.mbn", fw_name);

	pas_id = core->platform->data.pas_id;

	node = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!node) {
		d_vpr_e("%s: failed to read \"memory-region\"\n",
			__func__);
		return -EINVAL;
	}

	rc = of_address_to_resource(node, 0, &res);
	if (rc) {
		d_vpr_e("%s: failed to read \"memory-region\", error %d\n",
			__func__, rc);
		goto exit;
	}
	phys = res.start;
	res_size = (size_t)resource_size(&res);

	rc = request_firmware(&firmware, firmware_name, &pdev->dev);
	if (rc) {
		d_vpr_e("%s: failed to request fw \"%s\", error %d\n",
			__func__, firmware_name, rc);
		goto exit;
	}

	fw_size = qcom_mdt_get_size(firmware);
	if (fw_size < 0 || res_size < (size_t)fw_size) {
		rc = -EINVAL;
		d_vpr_e("%s: out of bound fw image fw size: %ld, res_size: %lu",
			__func__, fw_size, res_size);
		goto exit;
	}

	virt = memremap(phys, res_size, MEMREMAP_WC);
	if (!virt) {
		d_vpr_e("%s: failed to remap fw memory phys %pa[p]\n",
			__func__, &phys);
		return -ENOMEM;
	}

	/* prevent system suspend during fw_load */
	pm_stay_awake(pdev->dev.parent);
	rc = qcom_mdt_load(&pdev->dev, firmware, firmware_name,
			   pas_id, virt, phys, res_size, NULL);
	pm_relax(pdev->dev.parent);
	if (rc) {
		d_vpr_e("%s: error %d loading fw \"%s\"\n",
			__func__, rc, firmware_name);
		goto exit;
	}
	rc = qcom_scm_pas_auth_and_reset(pas_id);
	if (rc) {
		d_vpr_e("%s: error %d authenticating fw \"%s\"\n",
			__func__, rc, firmware_name);
		goto exit;
	}

	/* Enabling FW memory-region dump during Kernel panic */
	call_md_op(core, md_dump_fw_region, core, "vidc_core", virt, phys, res_size);
	memunmap(virt);
	release_firmware(firmware);
	d_vpr_h("%s: firmware \"%s\" loaded successfully\n",
		__func__, firmware_name);

	return pas_id;

exit:
	if (virt)
		memunmap(virt);
	if (firmware)
		release_firmware(firmware);

	return rc;
}

int fw_load(struct msm_vidc_core *core)
{
	int rc;

	if (!core->resource->fw_cookie) {
		core->resource->fw_cookie = __load_fw_to_memory(core->pdev,
								core->platform->data.fwname);
		if (core->resource->fw_cookie <= 0) {
			d_vpr_e("%s: firmware download failed %d\n",
				__func__, core->resource->fw_cookie);
			core->resource->fw_cookie = 0;
			return -ENOMEM;
		}
	}

	rc = call_venus_op(core, scm_mem_protect, core);
	if (rc) {
		d_vpr_e("%s scm_mem_protect failed\n", __func__);
		goto fail_scm_mem_protect;
	}

	return rc;

fail_scm_mem_protect:
	if (core->resource->fw_cookie)
		qcom_scm_pas_shutdown(core->resource->fw_cookie);
	core->resource->fw_cookie = 0;
	return rc;
}

int fw_unload(struct msm_vidc_core *core)
{
	int ret;

	if (!core->resource->fw_cookie)
		return -EINVAL;

	d_vpr_h("%s: unloading video firmware\n", __func__);
	ret = qcom_scm_pas_shutdown(core->resource->fw_cookie);
	if (ret)
		d_vpr_e("Firmware unload failed rc=%d\n", ret);

	core->resource->fw_cookie = 0;

	return ret;
}

int fw_suspend(struct msm_vidc_core *core)
{
	return qcom_scm_set_remote_state(TZBSP_VIDEO_STATE_SUSPEND, 0);
}

int fw_resume(struct msm_vidc_core *core)
{
	return qcom_scm_set_remote_state(TZBSP_VIDEO_STATE_RESUME, 0);
}

void fw_coredump(struct msm_vidc_core *core)
{
	int rc = 0;
	struct platform_device *pdev;
	struct device_node *node = NULL;
	struct resource res = {0};
	phys_addr_t mem_phys = 0;
	size_t res_size = 0;
	void *mem_va = NULL;
	char *data = NULL, *dump = NULL;
	u64 total_size;

	pdev = core->pdev;

	node = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!node) {
		d_vpr_e("%s: DT error getting \"memory-region\" property\n",
			__func__);
		return;
	}

	rc = of_address_to_resource(node, 0, &res);
	if (rc) {
		d_vpr_e("%s: error %d while getting \"memory-region\" resource\n",
			__func__, rc);
		return;
	}

	mem_phys = res.start;
	res_size = (size_t)resource_size(&res);

	mem_va = memremap(mem_phys, res_size, MEMREMAP_WC);
	if (!mem_va) {
		d_vpr_e("%s: unable to remap firmware memory\n", __func__);
		return;
	}
	total_size = res_size + TOTAL_QSIZE + ALIGNED_SFR_SIZE;

	data = vmalloc(total_size);
	if (!data) {
		memunmap(mem_va);
		return;
	}
	dump = data;

	/* copy firmware dump */
	memcpy(data, mem_va, res_size);
	memunmap(mem_va);

	/* copy queues(cmd, msg, dbg) dump(along with headers) */
	data += res_size;
	memcpy(data, (char *)core->iface_q_table.align_virtual_addr, TOTAL_QSIZE);

	/* copy sfr dump */
	data += TOTAL_QSIZE;
	memcpy(data, (char *)core->sfr.align_virtual_addr, ALIGNED_SFR_SIZE);

	dev_coredumpv(&pdev->dev, dump, total_size, GFP_KERNEL);
}
