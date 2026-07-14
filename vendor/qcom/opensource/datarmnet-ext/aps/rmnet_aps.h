/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RMNET_APS_H_
#define _RMNET_APS_H_

#include <linux/skbuff.h>
#include <net/genetlink.h>
#include <uapi/linux/rmnet_aps.h>

#ifdef RMNET_APS_DEBUG
#define aps_log(...) pr_err(__VA_ARGS__)
#else
#define aps_log(...)
#endif

int rmnet_aps_genl_flow_hdlr(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_aps_genl_pdn_config_hdlr(struct sk_buff *skb_2,
				   struct genl_info *info);
int rmnet_aps_genl_filter_hdlr(struct sk_buff *skb_2, struct genl_info *info);
int rmnet_aps_genl_data_report_hdlr(struct sk_buff *skb_2,
				    struct genl_info *info);

#endif /* _RMNET_APS_H_ */
