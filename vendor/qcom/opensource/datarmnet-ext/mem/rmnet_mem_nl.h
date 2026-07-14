/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_MEM_NL_H_
#define _RMNET_MEM_NL_H_

#include <net/genetlink.h>

int rmnet_mem_nl_register(void);
void rmnet_mem_nl_unregister(void);
int rmnet_mem_nl_cmd_update_mode(struct sk_buff *skb, struct genl_info *info);
int rmnet_mem_nl_cmd_update_pool_size(struct sk_buff *skb, struct genl_info *info);
int rmnet_mem_nl_cmd_peak_pool_size(struct sk_buff *skb, struct genl_info *info);
int rmnet_mem_genl_send_int_to_userspace_no_info(int val, struct genl_info *info);
int rmnet_mem_nl_get_mem_stats(struct sk_buff *skb, struct genl_info *info);
int rmnet_mem_nl_cmd_config_set(struct sk_buff *skb, struct genl_info *info);
int rmnet_mem_nl_cmd_config_get(struct sk_buff *skb, struct genl_info *info);
#endif /* _RMNET_MEM_GENL_H_ */
