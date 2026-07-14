/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_MEM_UAPI_H_
#define _RMNET_MEM_UAPI_H_

#define POOL_LEN 4
#define RMNET_MEM_MSG_PAYLOAD_SIZE 1000
/* These parameters can't exceed 15*/
#define	RMNET_MEM_PB_IND 0
#define	RMNET_MEM_PB_TIMEOUT 1
#define	RMNET_MEM_POOL_NL 2
#define	RMNET_MEM_PEAK_POOL_NL 3
#define	RMNET_MEM_LOW_MEM_NOTIF 4
#define	RMNET_MEM_MEM_STATS_NL 5
#define	RMNET_MEM_LL_LOW 6
#define	RMNET_MEM_FREE_PAGE_SCHED 7
#define	RMNET_MEM_LL_PAGE_SCHED 8
#define	RMNET_MEM_STAT_CONFIG_SET 9
#define	RMNET_MEM_ALLOC_FAILS 10

#define	RMNET_MEM_STAT_MAX 15

/* These parameters can't exceed 15 */
#define	ERR_MALLOC_FAIL1 0
#define	ERR_GET_ORDER_ERR 1
#define	ERR_INV_ARGS 2
#define	ERR_TIMEOUT 3
#define	ERR_NL_SEND_ERR 4
#define	ERR_MAX 15

/* These parameters can't exceed UINT8_MAX */
#define	RMNET_MEM_CMD_UNSPEC 0
#define	RMNET_MEM_CMD_UPDATE_MODE 1
#define	RMNET_MEM_CMD_UPDATE_POOL_SIZE 2
#define	RMNET_MEM_CMD_UPDATE_PEAK_POOL_SIZE 3
#define	RMNET_MEM_CMD_GET_MEM_STATS 4
#define	RMNET_MEM_CMD_CONFIG_SET 5
#define	RMNET_MEM_CMD_CONFIG_GET 6
#define	RMNET_MEM_GENL_CMD_MAX 255

/* Update RMNET_SHS_GENL_ATTR_MAX with the maximum value if a new entry is added */
#define	RMNET_MEM_ATTR_UNSPEC 0
#define	RMNET_MEM_ATTR_MODE 1
#define	RMNET_MEM_ATTR_POOL_SIZE 2
#define	RMNET_MEM_ATTR_INT 3
#define	RMNET_MEM_ATTR_STATS 4
#define	RMNET_MEM_ATTR_CONFIG 5

struct rmnet_memzone_req {
	int32_t zone;
	int32_t valid;
};
struct rmnet_pool_update_req {
	uint32_t poolsize[4];
	uint32_t valid_mask;
};

struct rmnet_mem_msg_info {
	char payload[RMNET_MEM_MSG_PAYLOAD_SIZE];
	uint16_t msg_type;
};

struct rmnet_mem_nl_stats {
	int32_t max_pool_size[POOL_LEN];
	int32_t cache_pool_size[POOL_LEN];
	int32_t static_pool_size[POOL_LEN];
	int32_t target_pool_size[POOL_LEN];
	uint64_t mem_id_gaveup[POOL_LEN];
	uint64_t mem_id_req[POOL_LEN];
	uint64_t mem_id_recycled[POOL_LEN];
	uint64_t mem_order_requests[POOL_LEN];
	uint64_t mem_order_gaveup[POOL_LEN];
	uint64_t mem_order_recycled[POOL_LEN];
	uint64_t pb_ind_max[POOL_LEN];
	uint64_t cache_adds[POOL_LEN];
	uint64_t cache_add_fails[POOL_LEN];
	uint64_t mem_stats[RMNET_MEM_STAT_MAX];
	uint64_t mem_err[ERR_MAX];
};
#endif
