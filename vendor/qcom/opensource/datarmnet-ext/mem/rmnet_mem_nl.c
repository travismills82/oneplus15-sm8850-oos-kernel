// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#include "rmnet_mem_priv.h"

#define RMNET_MEM_GENL_FAMILY_NAME "RMNET_MEM"
#define RMNET_MEM_GENL_VERSION 1
#define RMNET_MEM_ATTR_MAX (RMNET_MEM_ATTR_CONFIG)

uint32_t rmnet_mem_genl_seqnum;

static struct nla_policy rmnet_mem_nl_policy[RMNET_MEM_ATTR_MAX + 1] = {
	[RMNET_MEM_ATTR_MODE] =			NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_memzone_req)),
	[RMNET_MEM_ATTR_POOL_SIZE] =	NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_pool_update_req)),
	[RMNET_MEM_ATTR_STATS] =		NLA_POLICY_EXACT_LEN(sizeof(struct rmnet_mem_msg_info)),
	[RMNET_MEM_ATTR_CONFIG] =		NLA_POLICY_EXACT_LEN(sizeof(uint32_t)),
	/* Update the MAX when adding a new policy*/
};

static const struct genl_ops rmnet_mem_nl_ops[] = {
	{
		/* Deprecated, not used*/
		.cmd = RMNET_MEM_CMD_UPDATE_MODE,
		.doit = rmnet_mem_nl_cmd_update_mode,
	},
	{
		/* Adjust static pool size on the fly, set target_pool_size & start wq */
		.cmd = RMNET_MEM_CMD_UPDATE_POOL_SIZE,
		.doit = rmnet_mem_nl_cmd_update_pool_size,
	},
	{
		/* Set PB ind vote for what pool size will be adjusted to
		 * during active PB IND. Max(target_pool_size, pb_ind_max)
		 */
		.cmd = RMNET_MEM_CMD_UPDATE_PEAK_POOL_SIZE,
		.doit = rmnet_mem_nl_cmd_peak_pool_size,
	},
	{
		/* Return internal stats to requester */
		.cmd = RMNET_MEM_CMD_GET_MEM_STATS,
		.doit = rmnet_mem_nl_get_mem_stats,
	},
	{
		/* Set config of requester */
		.cmd = RMNET_MEM_CMD_CONFIG_SET,
		.doit = rmnet_mem_nl_cmd_config_set,
	},
	{
		/* Set config of requester */
		.cmd = RMNET_MEM_CMD_CONFIG_GET,
		.doit = rmnet_mem_nl_cmd_config_get,
	},
};

struct genl_family rmnet_mem_nl_family __ro_after_init = {
	.hdrsize = 0,
	.name = RMNET_MEM_GENL_FAMILY_NAME,
	.version = RMNET_MEM_GENL_VERSION,
	.maxattr = RMNET_MEM_ATTR_MAX,
	.policy = rmnet_mem_nl_policy,
	.ops = rmnet_mem_nl_ops,
	.n_ops = ARRAY_SIZE(rmnet_mem_nl_ops),
};

int rmnet_mem_send_msg_to_userspace(struct genl_info *info, struct rmnet_mem_msg_info *msg_ptr)
{
	struct sk_buff *skb;
	void *msg_head;
	int rc;

	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (skb == NULL)
		goto out;

	msg_head = genlmsg_put(skb, 0, 0, &rmnet_mem_nl_family,
			       0, RMNET_MEM_CMD_UPDATE_MODE);
	if (msg_head == NULL) {
		rc = -ENOMEM;
		kfree(skb);
		goto out;
	}

	rc = nla_put(skb, RMNET_MEM_ATTR_STATS, sizeof(struct rmnet_mem_msg_info),
				 msg_ptr);

	if (rc != 0) {
		kfree(skb);
		goto out;
	}

	genlmsg_end(skb, msg_head);

	rc = genlmsg_reply(skb, info);
	if (rc != 0)
		goto out;

	return 0;

out:
	return -1;
}

int rmnet_mem_genl_send_int_to_userspace_no_info(int val, struct genl_info *info)
{
	struct sk_buff *skb;
	void *msg_head;
	int rc;

	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (skb == NULL)
		goto out;

	msg_head = genlmsg_put(skb, 0, 0, &rmnet_mem_nl_family,
			       0, RMNET_MEM_CMD_UPDATE_MODE);
	if (msg_head == NULL) {
		rc = -ENOMEM;
		rm_err("MEM_GNL: FAILED to msg_head %d\n", rc);
		kfree(skb);
		goto out;
	}
	rc = nla_put_u32(skb, RMNET_MEM_ATTR_INT, val);
	if (rc != 0) {
		rm_err("MEM_GNL: FAILED nla_put %d\n", rc);
		kfree(skb);
		goto out;
	}

	genlmsg_end(skb, msg_head);

	rc = genlmsg_reply(skb, info);
	if (rc != 0)
		goto out;

	rm_err("MEM_GNL: Successfully sent int %d\n", val);
	return 0;

out:
	rm_err("MEM_GNL: FAILED to send int %d\n", val);
	return -1;
}

/* Update peak Mem pool size for Pb Ind usage */
int rmnet_mem_nl_get_mem_stats(struct sk_buff *skb, struct genl_info *info)
{
	struct rmnet_mem_msg_info msg;
	struct rmnet_mem_nl_stats *stats = (struct rmnet_mem_nl_stats *)&msg.payload;

	rmnet_mem_stats[RMNET_MEM_MEM_STATS_NL]++;
	memset(&msg, 0, sizeof(msg));

	memcpy(&stats->mem_id_gaveup, &rmnet_mem_id_gaveup, sizeof(rmnet_mem_id_gaveup));
	memcpy(&stats->mem_id_req, &rmnet_mem_id_req, sizeof(rmnet_mem_id_req));
	memcpy(&stats->mem_id_recycled, &rmnet_mem_id_recycled, sizeof(rmnet_mem_id_recycled));
	memcpy(&stats->mem_order_gaveup, &rmnet_mem_order_gaveup, sizeof(rmnet_mem_order_gaveup));
	memcpy(&stats->mem_order_recycled, &rmnet_mem_order_recycled,
		   sizeof(rmnet_mem_order_recycled));
	memcpy(&stats->mem_order_requests, &rmnet_mem_order_requests,
		   sizeof(rmnet_mem_order_requests));
	memcpy(&stats->max_pool_size, &max_pool_size, sizeof(max_pool_size));
	memcpy(&stats->cache_pool_size, &cache_pool_size, sizeof(cache_pool_size));
	memcpy(&stats->static_pool_size, &static_pool_size, sizeof(static_pool_size));
	memcpy(&stats->target_pool_size, &target_pool_size, sizeof(target_pool_size));
	memcpy(&stats->pb_ind_max, &rmnet_mem_pb_ind_max, sizeof(rmnet_mem_pb_ind_max));
	memcpy(&stats->cache_adds, &rmnet_mem_cache_adds, sizeof(rmnet_mem_cache_adds));
	memcpy(&stats->cache_add_fails, &rmnet_mem_cache_add_fails,
		   sizeof(rmnet_mem_cache_add_fails));
	memcpy(&stats->mem_stats, &rmnet_mem_stats, sizeof(rmnet_mem_stats));
	memcpy(&stats->mem_err, &rmnet_mem_err, sizeof(rmnet_mem_err));
	msg.msg_type = RMNET_MEM_STAT_TYPE;

	if (rmnet_mem_send_msg_to_userspace(info, &msg) < 0)
		rmnet_mem_err[ERR_NL_SEND_ERR]++;
	return 0;
}

int rmnet_mem_nl_register(void)
{
	return genl_register_family(&rmnet_mem_nl_family);
}

void rmnet_mem_nl_unregister(void)
{
	genl_unregister_family(&rmnet_mem_nl_family);
}
