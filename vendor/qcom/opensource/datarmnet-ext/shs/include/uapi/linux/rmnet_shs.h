/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _RMNET_SHS_UAPI_H_
#define _RMNET_SHS_UAPI_H_

/* Shared memory files */
#define RMNET_SHS_PROC_DIR	"shs"
#define RMNET_SHS_PROC_GLOBAL	"rmnet_shs_global"

#define RMNET_SHS_NUM_TOP_FFLOWS (30)
#define RMNET_SHS_NUM_TOP_LL_FLOWS (RMNET_SHS_NUM_TOP_FFLOWS)

#define RMNET_SHS_MAX_USRFLOWS (100)
#define RMNET_SHS_MAX_NETDEVS (40)
#define RMNET_SHS_IFNAMSIZ (16)
#define RMNET_SHS_READ_VAL (0)

#define SHS_SHARED_MEM_BLOCK_STRUCT_VERSION (1)

#define MAX_SHSUSRD_FLOWS 700

/* Contains all info needed for a flow - this is 80 bytes */
struct __attribute__((__packed__ ))  rmnet_shs_flow_entry_s {
	// basic flow info
	__u32 hash;
	__u8  cpu_num;
	__u8  mux_id;

	// 5 tuple info
	/* Address information */
	union {
		__be32 saddr4;
		__be32 saddr6[4];
	} ip_src;
	union {
		__be32 daddr4;
		__be32 daddr6[4];
	} ip_dest;
	/* Port information */
	__be16 sport;
	__be16 dport;
	__u8 trans_proto;

	// Bitfield to store smaller info like ip family and other flags
	__u8 ip_family:4;
	__u8 is_ll_flow:1; // is SW low latency matched flow
	__u8 is_l4s_flow:1; // seen packets marked with ECT(1)
	__u8 is_ll_true_flow:1; // is using LL codepath, SW LL or HW LL
	__u8 ecn_capable:1; // seen packets marked with ECN or ECT

	// packet/byte counters and some other values
	__u64 rx_skbs;
	__u64 rx_bytes;
	__u64 hw_coal_bytes;
	__u64 hw_coal_bufsize;
	__u32 ack_thresh; // quick ack threshold
};

struct __attribute__((__packed__ )) rmnet_shs_block_hdr {
	__u8 version; // struct version set to SHS_SHARED_MEM_BLOCK_STRUCT_VERSION

	// Common fields
	__u64 cur_time; // time at which values were queried
	__u64 pb_marker_seq;
	__u8 isolation_mask;
	__u8 reserve_mask;
	__u8 titanium_mask;
	__u8 online_mask;
	// Flow entry fields
	__u8 sizeof_flow_entry; // set to sizeof rmnet_shs_flow_entry_s struct
	__u16 num_flow_entries; // number of flows in flow_entries array
};
/* This structure should be less than 2^16 = 65536 bytes. Max flows is MAX_SHSUSRD_FLOWS = 500
 * Above struct is 80 bytes. This struct is 1+8+8+1+1+1+1+2+(500*80) = 56023 bytes */
struct __attribute__((__packed__ ))  rmnet_shs_shared_mem_block_s {
	struct rmnet_shs_block_hdr blk_hdr;
	struct rmnet_shs_flow_entry_s flow_entries[MAX_SHSUSRD_FLOWS];
};

/* Generic Netlink Definitions */
#define RMNET_SHS_GENL_VERSION 1
#define RMNET_SHS_GENL_FAMILY_NAME "RMNET_SHS"
#define RMNET_SHS_MOVE_FAIL_RESP_INT 720
#define RMNET_SHS_MOVE_PASS_RESP_INT 727

#define RMNET_SHS_SYNC_RESP_INT      828
#define RMNET_SHS_SYNC_RESP_INT_LPM_DISABLE      829
#define RMNET_SHS_CLEAN_PASS_INT 620
#define RMNET_SHS_CLEAN_FAIL_RESP_INT 621
#define RMNET_SHS_RMNET_MOVE_DONE_RESP_INT 300
#define RMNET_SHS_RMNET_MOVE_FAIL_RESP_INT 301

#define RMNET_SHS_BATCH_PASS_INT 500
#define RMNET_SHS_BATCH_FAIL_INT 501

#define RMNET_SHS_RMNET_DMA_RESP_INT 400

#define RMNET_SHS_SEG_FAIL_RESP_INT  920
#define RMNET_SHS_SEG_SET_RESP_INT   929

#define RMNET_SHS_BOOT_FAIL_RESP_INT  530
#define RMNET_SHS_BOOT_SET_RESP_INT   539

#define RMNET_SHS_QUICKACK_FAIL_RESP_INT  930
#define RMNET_SHS_QUICKACK_SET_RESP_INT   931
#define RMNET_SHS_SYNC_WQ_EXIT        42

#define MAXCPU 8
#define MAX_BATCH_FLOWS 50

/* These parameters can't exceed UINT8_MAX */
#define RMNET_SHS_GENL_CMD_UNSPEC 0
#define RMNET_SHS_GENL_CMD_INIT_SHSUSRD 1
#define RMNET_SHS_GENL_CMD_TRY_TO_MOVE_FLOW 2
#define RMNET_SHS_GENL_CMD_SET_FLOW_SEGMENTATION 3
#define RMNET_SHS_GENL_CMD_MEM_SYNC 4
#define RMNET_SHS_GENL_CMD_LL_FLOW 5
#define RMNET_SHS_GENL_CMD_QUICKACK 6
#define RMNET_SHS_GENL_CMD_BOOTUP 7
#define RMNET_SHS_GENL_CMD_CLEANUP 8
#define RMNET_SHS_GENL_CMD_BATCH_MOVE 9
#define __RMNET_SHS_GENL_CMD_MAX 255

/* Update RMNET_SHS_GENL_ATTR_MAX with the maximum value if a new entry is added */
#define RMNET_SHS_GENL_ATTR_UNSPEC 0
#define RMNET_SHS_GENL_ATTR_STR 1
#define RMNET_SHS_GENL_ATTR_INT 2
#define RMNET_SHS_GENL_ATTR_SUGG 3
#define RMNET_SHS_GENL_ATTR_SEG 4
#define RMNET_SHS_GENL_ATTR_FLOW 5
#define RMNET_SHS_GENL_ATTR_QUICKACK 6
#define RMNET_SHS_GENL_ATTR_BOOTUP 7
#define RMNET_SHS_GENL_ATTR_CLEAN 8
#define RMNET_SHS_GENL_ATTR_BATCH_MOVE 9

/* Update RMNET_SHS_WQ_SUGG_MAX with the maximum value if a new entry is added */
#define RMNET_SHS_WQ_SUGG_NONE 0
#define RMNET_SHS_WQ_SUGG_SILVER_TO_GOLD 1
#define RMNET_SHS_WQ_SUGG_GOLD_TO_SILVER 2
#define RMNET_SHS_WQ_SUGG_GOLD_BALANCE 3
#define RMNET_SHS_WQ_SUGG_RMNET_TO_GOLD 4
#define RMNET_SHS_WQ_SUGG_RMNET_TO_SILVER 5
#define RMNET_SHS_WQ_SUGG_LL_FLOW_CORE 6
#define RMNET_SHS_WQ_SUGG_LL_PHY_CORE 7

struct rmnet_shs_bootup_info {
	__u32 feature_mask;
	__u8 non_perf_mask;
	/*Scaled by 1k on recv */
	__u32 rx_min_pps_thresh[MAXCPU];
	__u32 rx_max_pps_thresh[MAXCPU];
	__u32 cpu_freq_boost_val;
	__u32 usr_version;
};

struct rmnet_shs_wq_sugg_info {
	__u32 hash_to_move;
	__u32 sugg_type;
	__u16 cur_cpu;
	__u16 dest_cpu;
};

struct rmnet_shs_wq_batch_sugg_info {
	__u16 num_flows;
	struct rmnet_shs_wq_sugg_info move_info[MAX_BATCH_FLOWS];
};

struct rmnet_shs_wq_seg_info {
	__u32 hash_to_set;
	__u32 segs_per_skb;
};

struct rmnet_shs_wq_quickack_info {
	__u32 hash_to_set;
	__u32 ack_thresh;
};

struct rmnet_shs_phy_change_payload {
	__u8  old_cpu;
	__u8  new_cpu;
};

/* rmnet_shs to shsusrd message channel */
#define RMNET_SHS_GENL_MSG_FAMILY_NAME "RMNET_SHS_MSG"

/* Command Types
 * These parameters can't exceed UINT8_MAX
 */
#define RMNET_SHS_GENL_MSG_CMD_UNSPEC 0
#define RMNET_SHS_GENL_MSG_WAIT_CMD 1
#define __RMNET_SHS_GENL_MSG_CMD_MAX 255

/* Attribute Types
 * These parameters can't exceed UINT8_MAX
 */
#define RMNET_SHS_GENL_MSG_ATTR_UNSPEC 0
#define RMNET_SHS_GENL_MSG_ATTR_REQ 1
#define RMNET_SHS_GENL_MSG_ATTR_RESP 2
#define __RMNET_SHS_GENL_MSG_ATTR_MAX 255

#define RMNET_SHS_MSG_PAYLOAD_SIZE (98)
#define RMNET_SHS_GENL_MSG_MAX     (1)

struct rmnet_shs_ping_boost_payload {
    __u32 perf_duration; /* Duration to acquire perf lock */
    __u8  perf_acq;      /* Set to 1 to aquire */
};

struct rmnet_shs_clean_payload {
    __u8 seq; /* Reserved */
};

struct rmnet_shs_pause_payload {
    __u8 seq; /*  Reserved */
};

#define RMNET_SHS_GENL_MSG_NOP 0
#define RMNET_SHS_GENL_PING_BOOST_MSG 1
#define RMNET_SHS_GENL_PHY_CHANGE_MSG 2
#define RMNET_SHS_GENL_TRAFFIC_PAUSE_MSG 3
#define RMNET_SHS_GENL_CLEANUP_MSG 4

struct rmnet_shs_msg_info {
	char     payload[RMNET_SHS_MSG_PAYLOAD_SIZE];
	__u16 msg_type;
};

struct rmnet_shs_msg_req {
	int valid;
};

struct rmnet_shs_msg_resp {
	struct rmnet_shs_msg_info list[RMNET_SHS_GENL_MSG_MAX];
	__u64 timestamp;
	__u16 list_len;
	__u8  valid;
};

struct rmnet_shs_wq_clean_info {
	__u32 hash_to_clean;
};

struct rmnet_shs_wq_flow_info {
	union {
		__be32	daddr;
		struct in6_addr  v6_daddr;
	} dest_ip_addr;
	union {
		__be32	saddr;
		struct in6_addr  v6_saddr;
	} src_ip_addr;
	__u16 src_port;
	__u16 src_port_valid;
	__u16 dest_port;
	__u16 dest_port_valid;
	__u8 src_addr_valid;
	__u8 dest_addr_valid;
	__u8 proto;
	__u8 proto_valid;
	__u8 ip_version;
	__u8 timeout;
	__u8 seq;
	__u8 opcode;
	union {
		__be32 mask;
		struct in6_addr v6_mask;
	} dest_ip_addr_mask;
	union {
		__be32 mask;
		struct in6_addr v6_mask;
	} src_ip_addr_mask;
	__u16 src_port_max;
	__u16 dest_port_max;
};

/* Types of suggestions made by shs wq
 * These parameters can't exceed UINT8_MAX
 */
#define RMNET_SHS_LL_OPCODE_DEL 0
#define RMNET_SHS_LL_OPCODE_ADD 1
#define RMNET_SHS_LL_OPCODE_MAX 255

#endif /*_RMNET_SHS_WQ_GENL_H_*/
