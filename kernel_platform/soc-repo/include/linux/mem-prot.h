/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/qti-lcp-ppddr.h>
#include <linux/mutex.h>
#include <linux/list.h>

#ifndef _MEM_PROT_H
#define _MEM_PROT_H

#if IS_ENABLED(CONFIG_MEM_PROT)
int mem_prot_pool_alloc(size_t size, phys_addr_t *phys, enum cfg_phys_ddr_protection_cmd prot_type);
int mem_prot_pool_free(phys_addr_t phys, size_t size, enum cfg_phys_ddr_protection_cmd prot_type);
#else
int mem_prot_pool_alloc(size_t size, phys_addr_t *phys, enum cfg_phys_ddr_protection_cmd prot_type)
{
	return -EINVAL;
}
int mem_prot_pool_free(phys_addr_t phys, size_t size, enum cfg_phys_ddr_protection_cmd prot_type)
{
	return -EINVAL;
}
#endif /* CONFIG_MEM_PROT */
#endif /* _MEM_PROT_H */


