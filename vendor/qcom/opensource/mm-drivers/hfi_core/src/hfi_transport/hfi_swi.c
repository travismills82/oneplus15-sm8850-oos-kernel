// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/iommu.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include "hfi_core_debug.h"
#include "hfi_interface.h"
#include "hfi_core.h"
#include "hfi_smmu.h"
#include "hfi_swi.h"

#define REG_WRITE(_val, _addr)                      writel_relaxed(_val, _addr)
#define REG_READ(_addr)                                    readl_relaxed(_addr)

#define HFI_DEV_SWI_CTRL_INIT(base)                                (base + 0x8)
#define HFI_DEV_SWI_RES_TBL_INFO(base)                             (base + 0xc)
#define HFI_DEV_SWI_RES_TBL_ADDR_H(base)                          (base + 0x10)
#define HFI_DEV_SWI_RES_TBL_ADDR_L(base)                          (base + 0x14)

#define HFI_DEV_CTRL_POWER_OFF(val)                          ((val & 0x1) << 1)
#define HFI_RES_TBL_HOSTID(val)                            ((val & 0xFF) << 24)
#define HFI_TBL_STATUS(val)                                      ((val & 0xFF))

static int map_mdss_register(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	unsigned int reg_config[2];
	unsigned long mapped_iova = 0;
	struct device *dev = (struct device *)drv_data->dev;

	HFI_CORE_DBG_H("+\n");

	ret = of_property_read_u32_array(dev->of_node, "qcom,mdss-reg", reg_config, 2);
	if (ret) {
		HFI_CORE_DBG_INFO("mdss reg is unavailable, ret: %d. skip mapping\n", ret);
		return 0;
	}

	drv_data->mdss_info.reg_base = reg_config[0];
	drv_data->mdss_info.size = reg_config[1];

	ret = smmu_mmap_for_fw(drv_data, drv_data->mdss_info.reg_base, &mapped_iova,
		drv_data->mdss_info.size, HFI_CORE_MMAP_READ | HFI_CORE_MMAP_WRITE);
	if (ret) {
		HFI_CORE_ERR("failed to map mdss registers, ret: %d\n", ret);
		return ret;
	}
	drv_data->mdss_info.iova = mapped_iova;

	HFI_CORE_DBG_H("mapped memory: 0x%llx size: 0x%x to addr:0x%lx\n",
		drv_data->mdss_info.reg_base, drv_data->mdss_info.size, mapped_iova);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static void unmap_mdss_register(struct hfi_core_drv_data *drv_data)
{
	HFI_CORE_DBG_H("+\n");

	if (!drv_data->mdss_info.reg_base)
		return;

	smmu_unmmap_for_fw(drv_data, drv_data->mdss_info.iova, drv_data->mdss_info.size);
	drv_data->mdss_info.reg_base = 0x0;
	drv_data->mdss_info.size = 0;
	drv_data->mdss_info.iova = 0;

	HFI_CORE_ERR("unmap mdss registers\n");

	HFI_CORE_DBG_H("-\n");
}

static int map_swi_register(struct hfi_core_drv_data *drv_data, u32 client_id)
{
	int ret = 0;
	void *__iomem ptr;
	struct device *dev = (struct device *)drv_data->dev;
	struct platform_device *pdev = NULL;
	char *dt_string = NULL;
	struct resource *res;
	unsigned long res_size;

	HFI_CORE_DBG_H("+\n");

	if (client_id == HFI_CORE_CLIENT_ID_0) {
		dt_string = "swi_dev0";
	} else {
		HFI_CORE_ERR("client id: %u is not supported\n", client_id);
		return -EINVAL;
	}

	pdev = to_platform_device(dev);
	if (!pdev) {
		HFI_CORE_ERR("invalid platform device for client: %u\n", client_id);
		return -EINVAL;
	}
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, dt_string);
	if (!res) {
		HFI_CORE_DBG_INFO("swi resource: %s is unavailable\n. skip mapping", dt_string);
		return 0;
	}
	res_size = resource_size(res);

	drv_data->client_data[client_id].swi_info.reg_base = res->start;
	drv_data->client_data[client_id].swi_info.size = res_size;

	ptr = memremap(drv_data->client_data[client_id].swi_info.reg_base,
		drv_data->client_data[client_id].swi_info.size, MEMREMAP_WB);
	if (!ptr) {
		HFI_CORE_ERR("failed to ioremap swi reg\n");
		return -ENOMEM;
	}
	drv_data->client_data[client_id].swi_info.io_mem = ptr;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static void unmap_swi_register(struct hfi_core_drv_data *drv_data, u32 client_id)
{
	HFI_CORE_DBG_H("+\n");

	if (!drv_data->client_data[client_id].swi_info.io_mem) {
		HFI_CORE_ERR("swi regs io mem addr not available\n");
		return;
	}

	memunmap(drv_data->client_data[client_id].swi_info.io_mem);

	HFI_CORE_DBG_H("-\n");
}

int init_swi(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	enum hfi_core_client_id client = HFI_CORE_CLIENT_ID_0;

	HFI_CORE_DBG_H("+\n");

	if (client >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client);
		return -EINVAL;
	}

	if (!drv_data || !drv_data->dev) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	ret = map_mdss_register(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to map mdss regs\n");
		goto exit;
	}

	ret = map_swi_register(drv_data, client);
	if (ret) {
		HFI_CORE_ERR("failed to map swi regs\n");
		goto exit;
	}

exit:
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int deinit_swi(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	enum hfi_core_client_id client = HFI_CORE_CLIENT_ID_0;

	HFI_CORE_DBG_H("+\n");

	if (client >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client);
		return -EINVAL;
	}

	if (!drv_data || !drv_data->dev) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	unmap_mdss_register(drv_data);
	unmap_swi_register(drv_data, client);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int swi_setup_resources(u32 client_id, struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_core_resource_info *res_info;
	u32 reg_val, val_to_write;
	void __iomem *reg_io_mem_base;

	HFI_CORE_DBG_H("+\n");

	if (client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client_id);
		return -EINVAL;
	}

	if (!drv_data) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	if (!drv_data->client_data[client_id].swi_info.io_mem) {
		HFI_CORE_DBG_INFO("swi io mem unavailable. skipping configuration\n");
		return 0;
	}

	res_info = &drv_data->client_data[client_id].resource_info;
	if (!res_info->dcp_map_addr) {
		HFI_CORE_ERR("client: %u dcp map addr[%x] is invalid\n", client_id,
			(u32)res_info->dcp_map_addr);
		return -EINVAL;
	}
	reg_io_mem_base = drv_data->client_data[client_id].swi_info.io_mem;

	/* configure resource table info register */
	reg_val = REG_READ(HFI_DEV_SWI_RES_TBL_INFO(reg_io_mem_base));
	val_to_write = reg_val | HFI_RES_TBL_HOSTID(0x1) | HFI_TBL_STATUS(0x1);
	REG_WRITE(val_to_write,  HFI_DEV_SWI_RES_TBL_INFO(reg_io_mem_base));

	/* configure resource table addr high register */
	REG_WRITE(0x0,  HFI_DEV_SWI_RES_TBL_ADDR_H(reg_io_mem_base));

	/* configure resource table addr low register */
	REG_WRITE(res_info->dcp_map_addr,  HFI_DEV_SWI_RES_TBL_ADDR_L(reg_io_mem_base));

	HFI_CORE_DBG_L("configured client[%u] swi reg phy[%llx] with dcp map addr[%x]\n",
		client_id,
		drv_data->client_data[client_id].swi_info.reg_base,
		(u32)res_info->dcp_map_addr);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int swi_reg_power_off(u32 client_id, struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_core_resource_info *res_info;
	u32 reg_val, val_to_write;
	void __iomem *reg_io_mem_base;

	HFI_CORE_DBG_H("+\n");

	if (client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client_id);
		return -EINVAL;
	}

	if (!drv_data) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	if (!drv_data->client_data[client_id].swi_info.io_mem) {
		HFI_CORE_DBG_INFO("swi io mem unavailable. skipping reg power off\n");
		return 0;
	}

	res_info = &drv_data->client_data[client_id].resource_info;
	if (!res_info->dcp_map_addr) {
		HFI_CORE_ERR("client: %u dcp map addr[%x] is invalid\n", client_id,
			(u32)res_info->dcp_map_addr);
		return -EINVAL;
	}
	reg_io_mem_base = drv_data->client_data[client_id].swi_info.io_mem;

	/* configure swi ctrl init register power off bit to 1 */
	reg_val = REG_READ(HFI_DEV_SWI_CTRL_INIT(reg_io_mem_base));
	val_to_write = reg_val | HFI_DEV_CTRL_POWER_OFF(1);
	REG_WRITE(val_to_write, HFI_DEV_SWI_CTRL_INIT(reg_io_mem_base));

	HFI_CORE_DBG_H("-\n");
	return ret;
}
