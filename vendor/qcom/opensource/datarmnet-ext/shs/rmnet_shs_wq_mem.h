/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_SHS_WQ_MEM_H_
#define _RMNET_SHS_WQ_MEM_H_

struct rmnet_shs_mmap_info {
	char *data;
	refcount_t refcnt;
};

void rmnet_shs_wq_mem_init(void);
void rmnet_shs_wq_mem_deinit(void);
void rmnet_shs_wq_mem_update_global(void);
#endif /*_RMNET_SHS_WQ_GENL_H_*/
