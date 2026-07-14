// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/iommu.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 5, 0) <= LINUX_VERSION_CODE)
#include <linux/remoteproc/qcom_rproc.h>
#endif
#include <linux/remoteproc.h>
#include "hfi_interface.h"
#include "hfi_core_debug.h"
#include "hfi_core.h"
#include "hfi_smmu.h"

#define DCP_TRACE_EVENTS_ADDR_OFFSET                                   0x410000
/* max size in bytes supported by alloc_pages_exact() */
#define DCP_MAX_PAGE_ALLOC_SIZE                                         4000000

struct hfi_smmu_info {
	struct rproc *soccp_rproc;
	unsigned long soccp_map_iova_index;
	struct iommu_domain *domain;
};

static int get_drv_domain(struct hfi_core_drv_data *drv_data)
{
	struct hfi_smmu_info *smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->dev) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	smmu->domain = iommu_get_domain_for_dev(drv_data->dev);
	if (IS_ERR_OR_NULL(smmu->domain)) {
		HFI_CORE_ERR("failed to get iommu domain for device ret:%ld\n",
			PTR_ERR(smmu->domain));
		return PTR_ERR(smmu->domain);
	}

	HFI_CORE_DBG_H("-\n");
	return 0;
}

static int parse_dt_props(struct hfi_core_drv_data *drv_data, enum hfi_core_client_id client)
{
	int ret;
	phandle ph;
	struct device_node *node;
	struct device *dev = NULL;
	unsigned int reg_config[2];
	struct hfi_smmu_info *smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;
	struct hfi_core_resource_info *res_info = &drv_data->client_data[client].resource_info;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->dev) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}
	dev = (struct device *)drv_data->dev;
	node = dev->of_node;

	/* check presence of soccp */
	ret = of_property_read_u32(node, "soccp_controller", &ph);
	if (ret) {
		HFI_CORE_DBG_INFO("failed to get soccp controller: %u\n", ph);
	} else {
		smmu->soccp_rproc = rproc_get_by_phandle(ph);
		if (IS_ERR_OR_NULL(smmu->soccp_rproc)) {
			HFI_CORE_DBG_INFO("failed to find rproc for phandle:%u\n", ph);
			ret = -EPROBE_DEFER;
			goto exit;
		}
	}

	ret = of_property_read_u32_array(dev->of_node, "qcom,device-map-addr-reg", reg_config, 2);
	if (ret) {
		HFI_CORE_ERR("failed to read swi reg, ret: %d\n", ret);
		goto exit;
	}

	res_info->dcp_map_addr = reg_config[0];
	res_info->dcp_map_addr_max_size = reg_config[1];
exit:
	HFI_CORE_DBG_H("-\n");
	return ret;
}

/* soccp power vote */
int set_power_vote(struct hfi_core_drv_data *drv_data, bool state)
{
	int ret = 0;
	struct hfi_smmu_info *smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;

	HFI_CORE_DBG_H("+\n");

	if (!smmu->soccp_rproc) {
		HFI_CORE_DBG_INFO("smmu soccp proc is null. skipping power vote\n");
		goto exit;
	}

#if (KERNEL_VERSION(6, 5, 0) <= LINUX_VERSION_CODE)
		ret = rproc_set_state(smmu->soccp_rproc, state);
#else
		ret = -EINVAL;
#endif

exit:
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int smmu_alloc_and_map_for_drv(struct hfi_core_drv_data *drv_data,
	phys_addr_t *addr, size_t size, void **__iomem cpu_va, enum hfi_core_dma_alloc_type type)
{
	u32 dma_flags = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->dev || !addr || !cpu_va) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}

	if (size > DCP_MAX_PAGE_ALLOC_SIZE) {
		HFI_CORE_ERR("invalid size to allocate: %zx, max supported: %x\n", size,
			DCP_MAX_PAGE_ALLOC_SIZE);
		return -EINVAL;
	}

	if (type == HFI_CORE_DMA_ALLOC_UNCACHE) {
		dma_flags = DMA_ATTR_NO_KERNEL_MAPPING | DMA_ATTR_WRITE_COMBINE;
	} else {
		HFI_CORE_ERR("unsupported dma alloc type %d requested\n", type);
		return -EINVAL;
	}

	*cpu_va = alloc_pages_exact(size, GFP_KERNEL);
	if (!(*cpu_va))
		return -ENOMEM;

	*addr = virt_to_phys(*cpu_va);
	memset_io(*cpu_va, 0x0, size);

	HFI_CORE_DBG_H("mapped allocated:0x%llx size:%zx cpu_va: 0x%llx\n", *addr, size,
		(u64)*cpu_va);

	HFI_CORE_DBG_H("-\n");
	return 0;
}

void smmu_unmap_for_drv(void *__iomem cpu_va, size_t size)
{
	HFI_CORE_DBG_H("+\n");

	if (cpu_va)
		free_pages_exact(cpu_va, size);

	HFI_CORE_DBG_H("-\n");
}

int smmu_mmap_for_fw(struct hfi_core_drv_data *drv_data, phys_addr_t addr,
	unsigned long *iova, size_t size, u32 flags)
{
	int ret = 0;
	u32 iommu_flags = 0;
	struct hfi_smmu_info *smmu = NULL;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->smmu_info.data || !iova) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}
	smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;
	if (!smmu->domain) {
		HFI_CORE_ERR("smmu domain is null\n");
		return -EINVAL;
	}

	if (flags & HFI_CORE_MMAP_READ)
		iommu_flags |= IOMMU_READ;

	if (flags & HFI_CORE_MMAP_WRITE)
		iommu_flags |= IOMMU_WRITE;

	if (flags & HFI_CORE_MMAP_CACHE)
		iommu_flags |= IOMMU_CACHE;

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	ret = iommu_map(smmu->domain, smmu->soccp_map_iova_index, addr, size, iommu_flags,
		GFP_KERNEL);
#else
	ret = iommu_map(smmu->domain, smmu->soccp_map_iova_index, addr, size, iommu_flags);
#endif

	if (ret) {
		HFI_CORE_ERR("iommu map failed for addr: 0x%llx size: %zx to addr: 0x%lx\n",
			addr, size, smmu->soccp_map_iova_index);
		return ret;
	}
	*iova = smmu->soccp_map_iova_index;

	HFI_CORE_DBG_INIT("mapped memory:0x%llx size:%zx to addr:0x%lx with iommu_flags : 0x%x\n",
		addr, size, smmu->soccp_map_iova_index, iommu_flags);

	/* update soccp memory map addr index */
	smmu->soccp_map_iova_index += size;

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int smmu_unmmap_for_fw(struct hfi_core_drv_data *drv_data, unsigned long iova, size_t size)
{
	struct hfi_smmu_info *smmu = NULL;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->smmu_info.data) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}
	smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;
	if (!smmu->domain) {
		HFI_CORE_ERR("smmu domain is null\n");
		return -EINVAL;
	}

	iommu_unmap(smmu->domain, iova, size);

	HFI_CORE_DBG_H("unmapped addr:0x%lx size: %zx\n", iova, size);

	HFI_CORE_DBG_H("-\n");
	return 0;
}

int smmu_mmap_debug_trace_mem_for_fw(struct hfi_core_drv_data *drv_data, phys_addr_t addr,
	unsigned long *iova, size_t size)
{
	int ret = 0;
	struct hfi_smmu_info *smmu = NULL;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->smmu_info.data || !iova) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}
	smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;
	if (!smmu->domain) {
		HFI_CORE_ERR("smmu domain is null\n");
		return -EINVAL;
	}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	ret = iommu_map(smmu->domain, (smmu->soccp_map_iova_index + DCP_TRACE_EVENTS_ADDR_OFFSET),
		addr, size, IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
#else
	ret = iommu_map(smmu->domain, (smmu->soccp_map_iova_index + DCP_TRACE_EVENTS_ADDR_OFFSET),
		addr, size, IOMMU_READ | IOMMU_WRITE);
#endif
	if (ret) {
		HFI_CORE_ERR("iommu map failed for addr: 0x%llx size: %zx to addr: 0x%lx\n",
			addr, size,
			(smmu->soccp_map_iova_index + DCP_TRACE_EVENTS_ADDR_OFFSET));
		return ret;
	}
	*iova = smmu->soccp_map_iova_index + DCP_TRACE_EVENTS_ADDR_OFFSET;

	HFI_CORE_DBG_H("mapped memory: 0x%llx size: %zx to addr: 0x%lx\n",
		addr, size,
		(smmu->soccp_map_iova_index + DCP_TRACE_EVENTS_ADDR_OFFSET));

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int hfi_init_fw_trace_mem(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_memory_alloc_info *alloc_info;
	u32 req_size = 0;

	HFI_CORE_DBG_H("+\n");

	alloc_info = kzalloc(sizeof(struct hfi_memory_alloc_info), GFP_KERNEL);
	if (!alloc_info) {
		HFI_CORE_ERR("failed to allocate fw trace memory\n");
		return -ENOMEM;
	}

	/* calculate size */
	req_size = sizeof(struct hfi_core_trace_event) * HFI_CORE_MAX_TRACE_EVENTS;

	alloc_info->size_wr = req_size;
	alloc_info->size_allocated = PAGE_ALIGN(req_size);
	/* allocate memory */
	ret = smmu_alloc_and_map_for_drv(drv_data, &alloc_info->phy_addr,
		alloc_info->size_allocated, &alloc_info->cpu_va, HFI_CORE_DMA_ALLOC_UNCACHE);
	if (ret) {
		HFI_CORE_ERR("failed to alloc, ret: %d\n", ret);
		goto alloc_fail;
	}
	memset_io(alloc_info->cpu_va, 0x0, alloc_info->size_allocated);

	/* map memory for fw */
	ret = smmu_mmap_debug_trace_mem_for_fw(drv_data, alloc_info->phy_addr,
		&alloc_info->mapped_iova, alloc_info->size_allocated);
	if (ret) {
		HFI_CORE_ERR("failed to map to fw, ret: %d\n", ret);
		goto mmap_fail;
	}

	drv_data->fw_trace_mem = alloc_info;

	HFI_CORE_DBG_H("allocated: cpu_va: 0x%llx, iova: 0x%lx, salign: 0x%zx, max_events: %d\n",
		(u64)alloc_info->cpu_va, alloc_info->mapped_iova,
		alloc_info->size_allocated, HFI_CORE_MAX_TRACE_EVENTS);

	HFI_CORE_DBG_H("-\n");
	return ret;

mmap_fail:
	/* unmap for drv */
	if (alloc_info->cpu_va)
		smmu_unmap_for_drv(alloc_info->cpu_va, alloc_info->size_allocated);
	alloc_info->cpu_va = NULL;
alloc_fail:
	alloc_info->size_allocated = 0;
	kfree(alloc_info);

	return ret;
}

static int hfi_deinit_fw_trace_mem(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data->fw_trace_mem || !drv_data->fw_trace_mem->size_allocated) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* unmap for fw */
	ret = smmu_unmmap_for_fw(drv_data, drv_data->fw_trace_mem->mapped_iova,
		drv_data->fw_trace_mem->size_allocated);
	if (ret) {
		HFI_CORE_ERR("unmap failed\n");
		return ret;
	}
	/* unmap for drv */
	if (drv_data->fw_trace_mem->cpu_va)
		smmu_unmap_for_drv(drv_data->fw_trace_mem->cpu_va,
			drv_data->fw_trace_mem->size_allocated);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int init_smmu(struct hfi_core_drv_data *drv_data)
{
	int ret;
	struct hfi_smmu_info *smmu = NULL;
	struct hfi_core_resource_info *res_info;
	enum hfi_core_client_id client = HFI_CORE_CLIENT_ID_0;

	HFI_CORE_DBG_H("+\n");

	if (client >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client id: %u\n", client);
		return -EINVAL;
	}

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	res_info = &drv_data->client_data[client].resource_info;

	smmu = kzalloc(sizeof(*smmu), GFP_KERNEL);
	if (!smmu) {
		ret = -ENOMEM;
		goto exit;
	}
	drv_data->smmu_info.data = (void *)smmu;

	ret = get_drv_domain(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to get domain\n");
		goto exit;
	}

	ret = parse_dt_props(drv_data, client);
	if (ret) {
		HFI_CORE_ERR("failed to set dt properties\n");
		goto exit;
	}

	smmu->soccp_map_iova_index = res_info->dcp_map_addr;

	ret = hfi_init_fw_trace_mem(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to init fw trace mem\n");
		goto exit;
	}

exit:
	HFI_CORE_DBG_H("-\n");
	return ret;
}

int deinit_smmu(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct hfi_smmu_info *smmu = NULL;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->smmu_info.data) {
		HFI_CORE_ERR("invalid params drv_data\n");
		return -EINVAL;
	}
	smmu = (struct hfi_smmu_info *)drv_data->smmu_info.data;

	ret = hfi_deinit_fw_trace_mem(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit fw trace mem\n");
		return ret;
	}

	if (smmu->soccp_rproc)
		rproc_put(smmu->soccp_rproc);

	HFI_CORE_DBG_H("-\n");
	return 0;
}
