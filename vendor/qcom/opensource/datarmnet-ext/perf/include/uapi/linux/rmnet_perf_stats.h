/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __RMNET_PERF_STATS_H__
#define __RMNET_PERF_STATS_H__

#include <linux/types.h>

#define RMNET_PERF_GENL_FAMILY_NAME "RMNET_PERF"
#define RMNET_PERF_GENL_MULTICAST_NAME_0 "RMNET_PERF_MC_0"
#define RMNET_PERF_GENL_MULTICAST_NAME_1 "RMNET_PERF_MC_1"
#define RMNET_PERF_GENL_MULTICAST_NAME_2 "RMNET_PERF_MC_2"
#define RMNET_PERF_GENL_MULTICAST_NAME_3 "RMNET_PERF_MC_3"

#define RMNET_PERF_CMD_UNSPEC 0
#define RMNET_PERF_CMD_GET_STATS 1
#define RMNET_PERF_CMD_MAP_CMD 2
#define RMNET_PERF_CMD_ECN_UPDATE 3
#define RMNET_PERF_CMD_ECN_DROP_STATS 4
#define RMNET_PERF_CMD_ECN_FLUSH 5

/* Update RMNET_PERF_ATTR_MAX with the maximum value if a new entry is added */
#define RMNET_PERF_ATTR_UNSPEC 0
#define RMNET_PERF_ATTR_STATS_REQ 1
#define RMNET_PERF_ATTR_STATS_RESP 2
#define RMNET_PERF_ATTR_MAP_CMD_REQ 3
#define RMNET_PERF_ATTR_MAP_CMD_RESP 4
#define RMNET_PERF_ATTR_MAP_CMD_IND 5
#define RMNET_PERF_ATTR_ECN_HASH 6
#define RMNET_PERF_ATTR_ECN_PROB 7
#define RMNET_PERF_ATTR_ECN_TYPE 8
#define RMNET_PERF_ATTR_ECN_DROPS 9

#define RMNET_PERF_ECN_TYPE_DROP 0
#define RMNET_PERF_ECN_TYPE_MARK 1

struct rmnet_perf_stats_req {
	__u8 mux_id;
} __attribute__((aligned(1)));

struct rmnet_perf_proto_stats {
	__u64 tcpv4_pkts;
	__u64 tcpv4_bytes;
	__u64 udpv4_pkts;
	__u64 udpv4_bytes;
	__u64 tcpv6_pkts;
	__u64 tcpv6_bytes;
	__u64 udpv6_pkts;
	__u64 udpv6_bytes;
} __attribute__((aligned(1)));

struct rmnet_perf_coal_common_stats {
	__u64 csum_error;
	__u64 pkt_recons;
	__u64 close_non_coal;
	__u64 l3_mismatch;
	__u64 l4_mismatch;
	__u64 nlo_limit;
	__u64 pkt_limit;
	__u64 byte_limit;
	__u64 time_limit;
	__u64 eviction;
	__u64 close_coal;
} __attribute__((aligned(1)));

struct downlink_stats {
	struct rmnet_perf_coal_common_stats coal_common_stats;
	struct rmnet_perf_proto_stats coal_veid_stats[16];
	__u64 non_coal_pkts;
	__u64 non_coal_bytes;
} __attribute__((aligned(1)));

struct uplink_stats {
	struct rmnet_perf_proto_stats seg_proto_stats;
} __attribute__((aligned(1)));

struct rmnet_perf_stats_store {
	struct downlink_stats dl_stats;
	struct uplink_stats ul_stats;
} __attribute__((aligned(1)));

struct rmnet_perf_stats_resp {
	__u16 error_code;
	struct rmnet_perf_stats_store stats;
} __attribute__((aligned(1)));

struct rmnet_perf_map_cmd_req {
	__u16 cmd_len;
	__u8 cmd_name;
	__u8 ack;
	__u8 cmd_content[16384];
} __attribute__((aligned(1)));

struct rmnet_perf_map_cmd_resp {
	__u8 cmd_name;
	__u16 error_code;
} __attribute__((aligned(1)));

struct rmnet_perf_map_cmd_ind {
	__u16 cmd_len;
	__u8 cmd_name;
	__u8 ack;
	__u8 cmd_content[4096];
} __attribute__((aligned(1)));

#endif
