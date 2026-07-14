/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_MEM_PRIV_H_
#define _RMNET_MEM_PRIV_H_

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/mm.h>

#include "rmnet_mem_nl.h"
#include "rmnet_mem.h"
#include "rmnet_mem_uapi.h"

#define POOL_LEN 4

#define MAX_STATIC_POOL 1000
#define MAX_POOL_O3 675
#define MAX_POOL_O2 224

#define MID_POOL_O3 603
#define MID_POOL_O2 190

#define STATIC_F_O3 3
#define OLD_MID_POOL_O3 600

#define RAMP_DOWN_DELAY 3000
#define PB_IND_DUR 105
#define MAX_VOTE(a, b)    ((a) > (b) ? (a) : (b))
#define RMNET_MEM_MSG_PAYLOAD_SIZE 1000
#define RMNET_MEM_STAT_TYPE 1

typedef struct {
	struct page *addr;
	struct list_head mem_head;
	struct list_head cache_head;
	u8 order;
} mem_info_s;

#define BUFF_ABOVE_HIGH_THRESHOLD_FOR_DEFAULT_PIPE        1
#define BUFF_ABOVE_HIGH_THRESHOLD_FOR_COAL_PIPE           2
#define BUFF_BELOW_LOW_THRESHOLD_FOR_DEFAULT_PIPE         3
#define BUFF_BELOW_LOW_THRESHOLD_FOR_COAL_PIPE            4
#define BUFF_ABOVE_HIGH_THRESHOLD_FOR_LL_PIPE             5
#define BUFF_BELOW_LOW_THRESHOLD_FOR_LL_PIPE              6
#define FREE_PAGE_TASK_SCHEDULED                          7
#define FREE_PAGE_TASK_SCHEDULED_LL                       8

void rmnet_mem_adjust(unsigned int perm_size, u8 order);

#define rm_err(fmt, ...)  \
	do { if (0) pr_err(fmt, __VA_ARGS__); } while (0)

extern struct rmnet_mem_notif_s rmnet_mem_notifier;
extern struct delayed_work pool_adjust_work;
extern struct workqueue_struct *mem_wq;
extern uint32_t ipa_config;
extern unsigned int rmnet_mem_debug;
extern unsigned int rmnet_mem_pb_enable;
extern int max_pool_size[POOL_LEN];
extern int cache_pool_size[POOL_LEN];
extern int static_pool_size[POOL_LEN];
extern int target_pool_size[POOL_LEN];
extern unsigned int pool_unbound_feature[POOL_LEN];
extern unsigned long long rmnet_mem_order_requests[POOL_LEN];
extern unsigned long long rmnet_mem_id_req[POOL_LEN];
extern unsigned long long rmnet_mem_id_recycled[POOL_LEN];
extern unsigned long long rmnet_mem_order_recycled[POOL_LEN];
extern unsigned long long rmnet_mem_id_gaveup[POOL_LEN];
extern unsigned long long rmnet_mem_order_gaveup[POOL_LEN];
extern unsigned long long rmnet_mem_stats[RMNET_MEM_STAT_MAX];
extern unsigned long long rmnet_mem_err[ERR_MAX];
extern unsigned long long rmnet_mem_pb_ind_max[POOL_LEN];
extern unsigned long long rmnet_mem_cache_adds[POOL_LEN];
extern unsigned long long rmnet_mem_cache_add_fails[POOL_LEN];

#endif
