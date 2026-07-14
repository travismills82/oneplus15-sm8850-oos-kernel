/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "rmnet_shs.h"

#ifndef _RMNET_SHS_WQ_GENL_H_
#define _RMNET_SHS_WQ_GENL_H_

#include <net/genetlink.h>


extern int rmnet_shs_userspace_connected;


struct rmnet_shs_wq_flow_node {
	struct list_head filter_head;
	struct hlist_node list;
	struct rmnet_shs_wq_flow_info info;
};

/* Function Prototypes */
int rmnet_shs_genl_dma_init(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_try_to_move_flow(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_set_flow_segmentation(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_mem_sync(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_set_flow_ll(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_set_quickack_thresh(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_set_bootup_config(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_cleanup(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_shs_genl_batch_move_flow(struct sk_buff *skb_2, struct genl_info *info);

int rmnet_shs_genl_send_int_to_userspace(struct genl_info *info, int val);
int rmnet_shs_genl_send_int_to_userspace_no_info(int val);
int rmnet_shs_genl_send_msg_to_userspace(void);
int rmnet_shs_genl_msg_direct_send_to_userspace(struct rmnet_shs_msg_resp *msg_ptr);

/* rmnet_shs to shsusrd messaging functionality */
void rmnet_shs_create_ping_boost_msg_resp(uint32_t perf_duration,
					  struct rmnet_shs_msg_resp *msg_resp);
void rmnet_shs_create_pause_msg_resp(uint8_t seq,
				     struct rmnet_shs_msg_resp *msg_resp);
void rmnet_shs_create_phy_msg_resp(struct rmnet_shs_msg_resp *msg_resp,
                                   uint8_t ocpu, uint8_t ncpu);

void rmnet_shs_create_cleanup_msg_resp(struct rmnet_shs_msg_resp *msg_resp);

/* Handler for message channel to shsusrd */
int rmnet_shs_genl_msg_req_hdlr(struct sk_buff *skb_2,
				struct genl_info *info);

int rmnet_shs_wq_genl_init(void);
int rmnet_shs_wq_genl_deinit(void);

#endif /*_RMNET_SHS_WQ_GENL_H_*/
