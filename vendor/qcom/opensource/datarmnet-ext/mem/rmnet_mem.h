/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_MEM_H_
#define _RMNET_MEM_H_

#define IPA_ID 1
#define RMNET_CORE_ID 2

#define POOL_NOTIF 3

#define RMNET_MEM_SUCCESS 0
#define RMNET_MEM_FAIL -1
#define RMNET_MEM_DOWNGRADE -2
#define RMNET_MEM_UPGRADE -3
#define NS_IN_MS 1000000
#define POWER_SAVE_NOTIF  0
/* Bitmask for client config IPA*/
#define DISABLE_STATIC_REDUCTION_F 1

int rmnet_mem_unregister_notifier(struct notifier_block *nb);
int rmnet_mem_register_notifier(struct notifier_block *nb);
void rmnet_mem_pb_ind(void);
int rmnet_mem_get_pool_size(unsigned int order);
void rmnet_mem_cb(unsigned long event, void* data);
void rmnet_mem_cache_add(unsigned int order, bool force);
uint32_t rmnet_mem_config_query(unsigned int id);
void rmnet_mem_put_page_entry(struct page *page);
void rmnet_mem_page_ref_inc_entry(struct page *page, unsigned int id);
struct page *rmnet_mem_get_pages_entry(gfp_t gfp_mask, unsigned int order, int *code, int *pageorder, unsigned int id);

#endif
