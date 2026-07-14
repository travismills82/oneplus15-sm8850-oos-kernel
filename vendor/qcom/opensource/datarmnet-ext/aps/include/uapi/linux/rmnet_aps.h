/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2021-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __UAPI_LINUX_RMNET_APS_H__
#define __UAPI_LINUX_RMNET_APS_H__

#include <linux/types.h>

/* Generic Netlink Definitions */
#define RMNET_APS_GENL_VERSION 1
#define RMNET_APS_GENL_FAMILY_NAME "RMNET_APS"


/* UAPI checker wines about enums not being backwards compatible because
 * "waaaah LaSt ValUe ChaNges!! whaaaaah~" if you add a new command.
 * Never mind that is how the ENTIRETY of the kernel handles netlink
 * and adding numerous tacky defines is asinine.
 *
 * We must adhere to all rigid rules our omniscient and faultless overlords
 * have laid down. THEY CAN DO NO WRONG!
 */

/* GENL Commands */
#define RMNET_APS_GENL_CMD_UNSPEC	0
#define RMNET_APS_GENL_CMD_FLOW		1
#define RMNET_APS_GENL_CMD_PDN_CONFIG	2
#define RMNET_APS_GENL_CMD_FILTER	3
#define RMNET_APS_GENL_CMD_DATA_REPORT	4

/* GENL Attributes */
#define RMNET_APS_GENL_ATTR_UNSPEC		0
#define RMNET_APS_GENL_ATTR_FLOW_REQ		1
#define RMNET_APS_GENL_ATTR_FLOW_RESP		2
#define RMNET_APS_GENL_ATTR_PDN_CONFIG_REQ	3
#define RMNET_APS_GENL_ATTR_PDN_CONFIG_RESP	4
#define RMNET_APS_GENL_ATTR_FILTER_REQ		5
#define RMNET_APS_GENL_ATTR_FILTER_RESP		6
#define RMNET_APS_GENL_ATTR_DATA_REPORT		7

/* Sub Command Types for CMD_FLOW and CMD_FILTER */
#define RMNET_APS_SUBCMD_INIT		1
#define RMNET_APS_SUBCMD_ADD_FLOW	2
#define RMNET_APS_SUBCMD_DEL_FLOW	3
#define RMNET_APS_SUBCMD_UPD_FLOW	4
#define RMNET_APS_SUBCMD_FLOW_REMOVED	5
#define RMNET_APS_SUBCMD_ADD_FILTER	6

struct rmnet_aps_flow_req {
	__u32 cmd;
	__u32 label;
	__u32 duration;
	__u32 ifindex;
	__u8 aps_prio;
	__u8 use_llc;
	__u8 use_llb;
	__u8 reserved;
};

/* Flow response cmd_data values */
#define RMNET_APS_FLOW_REMOVED_EXPIRED		1
#define RMNET_APS_FLOW_REMOVED_NO_LONGER_VALID	2
#define RMNET_APS_FLOW_REMOVED_RESET		3

struct rmnet_aps_flow_resp {
	__u32 cmd;
	__u32 cmd_data;
	__u32 label;
};

#define RMNET_APS_FILTER_MASK_SADDR	1
#define RMNET_APS_FILTER_MASK_DADDR	2
struct rmnet_aps_filter_req {
	__u32 cmd;
	__u32 label;
	__u32 ifindex;
	__s32 ip_type;
	__be32 saddr[4];
	__be32 daddr[4];
	__u16 sport;
	__u16 dport;
	__u32 flow_label;
	__u8 tos;
	__u8 tos_mask;
	__u8 l4_proto;
	__u8 filter_masks;
	__be32 saddr_mask[4];
	__be32 daddr_mask[4];
	__u16 sport_max;
	__u16 dport_max;
	__u8 reserved[32];
};

struct rmnet_aps_filter_resp {
	__u32 cmd;
	__u32 cmd_data;
	__u32 label;
};

struct rmnet_aps_pdn_config_req {
	__u32 ifindex;
	__u64 apn_mask;
	__u32 expire_ms;
	__u32 reserved[8];
};

struct rmnet_aps_pdn_config_resp {
	__u32 ifindex;
	__u32 reserved[7];
};

struct rmnet_aps_data_report {
	__u8 mux_id;
	__u8 type;
	__u8 sum_all_bearers;
	__u8 len;
	__u32 value[8];
};

#endif
