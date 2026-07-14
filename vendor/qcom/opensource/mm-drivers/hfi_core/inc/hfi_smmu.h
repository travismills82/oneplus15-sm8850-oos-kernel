/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_SMMU_H__
#define __HFI_SMMU_H__

#include "hfi_core.h"
#include <linux/iommu.h>
#include <linux/dma-mapping.h>

/**
 * init_smmu() - SMMU initialization.
 *
 * This call initializes the DCP context bank required for the device
 * to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int init_smmu(struct hfi_core_drv_data *drv_data);

/**
 * deinit_smmu() - SMMU deinitialization.
 *
 * This call deinitializes the DCP context bank required for the device
 * to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int deinit_smmu(struct hfi_core_drv_data *drv_data);

/**
 * smmu_alloc_and_map_for_drv() - Allocate and map memory for hfi core
 *
 * This API allocates dynamic memory of requested size and type and maps it
 * for hfi core access. Output of this API is physical and virtual addresses
 * of allocated memory.
 *
 * Return: 0 on success or negative errno
 */
int smmu_alloc_and_map_for_drv(struct hfi_core_drv_data *drv_data,
	phys_addr_t *addr, size_t size, void **__iomem cpu_va,
	enum hfi_core_dma_alloc_type type);

/**
 * smmu_unmap_for_drv() - Unmap memory for hfi core
 *
 * This API unmpas dynamic memory allocated by smmu_alloc_and_map_for_drv()
 * and frees it.
 *
 * Return: 0 on success or negative errno
 */
void smmu_unmap_for_drv(void *__iomem cpu_va, size_t size);

/**
 * smmu_mmap_for_fw() - map memory for firmware access
 *
 * This API maps the memory allocated by smmu_alloc_and_map_for_drv() to the
 * to device address region for firmware access of this memory.
 * Input of this API is the physical address of memory region to be mapped
 * that is returned from smmu_alloc_and_map_for_drv() API call and the size of
 * this memory to map. Output of this API is iova (device address) of the
 * of memory requested for mapping.
 *
 * Return: 0 on success or negative errno
 */
int smmu_mmap_for_fw(struct hfi_core_drv_data *drv_data, phys_addr_t addr,
	unsigned long *iova, size_t size, enum hfi_core_mmap_flags flags);

/**
 * smmu_unmmap_for_fw() - unmap memory to remove firmware access
 *
 * This API unmpas the memory region mapped for firmware access.
 *
 * Return: 0 on success or negative errno
 */
int smmu_unmmap_for_fw(struct hfi_core_drv_data *drv_data, unsigned long iova,
	size_t size);

/**
 * set_power_vote() - soccp power vote
 *
 * This API power votes soccp to move it to D3 (active) state.
 *
 * Return: 0 on success or negative errno
 */
int set_power_vote(struct hfi_core_drv_data *drv_data, bool state);

#endif // __HFI_SMMU_H
