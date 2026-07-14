// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "rmnet_mem_priv.h"

#define RMNET_MEM_NL_SUCCESS 400
#define RMNET_MEM_NL_FAIL 401

int rmnet_mem_nl_cmd_update_mode(struct sk_buff *skb, struct genl_info *info)
{
	u8 mode = 0;
	struct rmnet_memzone_req mem_info;
	struct nlattr *na;

	if (info->attrs[RMNET_MEM_ATTR_MODE]) {
		na = info->attrs[RMNET_MEM_ATTR_MODE];
		if (nla_memcpy(&mem_info, na, sizeof(mem_info)) > 0)
			rm_err("%s(): modeinfo %u\n", __func__, mem_info.zone);

		rm_err("%s(): mode %u\n", __func__, mode);

		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}
	return 0;
}

int rmnet_mem_nl_cmd_update_pool_size(struct sk_buff *skb, struct genl_info *info)
{
	struct rmnet_pool_update_req mem_info;
	struct nlattr *na;
	int i;
	unsigned long jiffies;
	u8 update_flag = 0;
	u8 increase = 0;

	rmnet_mem_stats[RMNET_MEM_POOL_NL]++;

	if (info->attrs[RMNET_MEM_ATTR_POOL_SIZE]) {
		na = info->attrs[RMNET_MEM_ATTR_POOL_SIZE];
		if (nla_memcpy(&mem_info, na, sizeof(mem_info)) > 0)
			rm_err("%s(): modeinfo %u\n", __func__, mem_info.valid_mask);


		for (i = 0; i < POOL_LEN; i++) {
			if (mem_info.valid_mask & 1 << i &&
			    mem_info.poolsize[i] > 0 &&
			    mem_info.poolsize[i] <= MAX_STATIC_POOL) {
				/* Sets next adjust work trigger to alloc new target memory.
				 * Updates grow cap for new pages we alloc.
				 */
				target_pool_size[i] = mem_info.poolsize[i];
				max_pool_size[i] = mem_info.poolsize[i];
				update_flag = 1;
				/* If greater mem demands grab mem immediately */
				if (!increase && mem_info.poolsize[i] > static_pool_size[i])
					increase = 1;
			}
		}
		rm_err(" poolsize %d %d\n", mem_info.poolsize[2], mem_info.poolsize[3]);

		if (update_flag && mem_wq) {
			jiffies = msecs_to_jiffies(RAMP_DOWN_DELAY);
			cancel_delayed_work_sync(&pool_adjust_work);
			queue_delayed_work(mem_wq, &pool_adjust_work, (increase) ? 0 : jiffies);
		}

		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}

	return 0;
}

/* Update peak Mem pool size for Pb Ind usage */
int rmnet_mem_nl_cmd_peak_pool_size(struct sk_buff *skb, struct genl_info *info)
{
	struct rmnet_pool_update_req mem_info;
	struct nlattr *na;
	int i;

	rmnet_mem_stats[RMNET_MEM_PEAK_POOL_NL]++;

	if (info->attrs[RMNET_MEM_ATTR_POOL_SIZE]) {
		na = info->attrs[RMNET_MEM_ATTR_POOL_SIZE];
		if (nla_memcpy(&mem_info, na, sizeof(mem_info)) > 0)
			rm_err("%s(): modeinfo %u\n", __func__, mem_info.valid_mask);


		rm_err("%s(): pbind pool_size %u\n", __func__, mem_info.poolsize[3]);

		for (i = 0; i < POOL_LEN; i++) {
			if (mem_info.valid_mask & 1 << i) {
				if (mem_info.poolsize[i] > 0 && mem_info.poolsize[i] <= MAX_STATIC_POOL)
					rmnet_mem_pb_ind_max[i] = mem_info.poolsize[i];
			}
		}

		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}

	return 0;
}

int rmnet_mem_nl_cmd_config_set(struct sk_buff *skb, struct genl_info *info)
{
	uint32_t config;
	struct nlattr *na;
	int prev_config;

	rmnet_mem_stats[RMNET_MEM_STAT_CONFIG_SET]++;
	if (info->attrs[RMNET_MEM_ATTR_CONFIG]) {
		na = info->attrs[RMNET_MEM_ATTR_CONFIG];
		if (nla_memcpy(&config, na, sizeof(config)) > 0)
			rm_err("%s(): config %u\n", __func__, config);

		rm_err("%s(): Setting ipa config %u\n", __func__, config);
		prev_config = ipa_config;

		if (DISABLE_STATIC_REDUCTION_F & config &&
		    !(DISABLE_STATIC_REDUCTION_F & prev_config)) {
			ipa_config = config;
			/* Disable new feature and expect bigger static pool */
			if(static_pool_size[STATIC_F_O3] == MID_POOL_O3 || !static_pool_size[STATIC_F_O3] ) {
				target_pool_size[STATIC_F_O3] = OLD_MID_POOL_O3;
				max_pool_size[STATIC_F_O3] = OLD_MID_POOL_O3;
				cancel_delayed_work_sync(&pool_adjust_work);
				queue_delayed_work(mem_wq, &pool_adjust_work, 0);
			}
			else if(target_pool_size[STATIC_F_O3] == MID_POOL_O3) {
				target_pool_size[STATIC_F_O3] = OLD_MID_POOL_O3;
				max_pool_size[STATIC_F_O3] = OLD_MID_POOL_O3;
			}
			rm_err("setting config 2 %x max pool %d target max %d", 1, max_pool_size[STATIC_F_O3],  target_pool_size[STATIC_F_O3]);
		}
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}

	return 0;
}


int rmnet_mem_nl_cmd_config_get(struct sk_buff *skb, struct genl_info *info)
{
	rmnet_mem_genl_send_int_to_userspace_no_info(ipa_config, info);
	return 0;
}
