/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _QCOM_SLC_PERFMON_SCMI_H_
#define _QCOM_SLC_PERFMON_SCMI_H_

/* Filter selection index */
enum fltr_idx {
	FILTER_0,
	FILTER_1,
	FILTER_2,
	FILTER_3,
	FILTER_4,
	FILTER_5,
	FILTER_6,
	FILTER_7,
};

/* FILTER types */
enum filter_type {
	UNKNOWN_FILTER = -1,
	SCID,
	MID,
	PROFTAG,
	OPCODE,
	CACHEALLOC,
	MEMTAGOPS,
	MAX_FILTER_TYPE,
	MULTISCID,
};

/* Perfmon SCMI SET attributes */
enum slc_perfmon_set_attr {
	PERFMON_FILTER_CONFIG,
	PERFMON_COUNTER_CONFIG,
	PERFMON_TIMER_CONFIG,
	PERFMON_START,
	PERFMON_COUNTER_REMOVE,
	PERFMON_FILTER_REMOVE,
};

/* Perfmon SCMI GET attributes */
enum slc_perfmon_get_attr {
	PERFMON_SLC_COMMON_INFO,
	PERFMON_CONFIG_INFO,
};

/*
 * struct slc_version		SLC version.
 *
 * @major:	SLC version major.
 * @branch:	SLC version branch.
 * @minor:	SLC version minor.
 * @eco:	SLC version eco.
 */
struct slc_version {
	u8 major;
	u8 branch;
	u8 minor;
	u8 eco;
} __packed;

/*
 * struct mem_cntrl_channel	memory controller and channel inforation.
 *
 * @channels:			No. of channels.
 * @num_mc:			No. of memory controller.
 */
struct mem_cntrl_channel {
	uint8_t channels : 6;
	uint8_t num_mc : 2;
} __packed;

/*
 * struct slc_info_attr		PERFMON_SLC_COMMON_INFO SCMI interface.
 *
 * @version:			SLC version.
 * @max_fltr_idx:		Maximum filter supported.
 * @max_cntr:			Maximum counters supported.
 * @num_mc:			No. of memory controller.
 * @cacheline_size:		Cache line size.
 * @port_registered:		Registered ports (bit-mapped).
 */
struct slc_info_attr {
	struct slc_version version;
	u8 max_fltr_idx;
	u8 max_cntr;
	struct mem_cntrl_channel mc_ch;
	u8 cacheline_size;
	u16 port_registered;
} __packed;

/*
 * struct slc_info_attr		PERFMON_CONFIG_INFO SCMI interface.
 *
 * @filter_max_match:			SLC version.
 * @filter_port_support:		Maximum filter supported.
 */
struct slc_perfmon_info_attr {
	u16 filter_max_match[MAX_FILTER_TYPE];
	u8 filter_port_support[];
} __packed;

/*
 * struct slc_info_attr:		PERFMON_FILTER_CONFIG SCMI SCMI interface.
 *
 * @fltr_type:			Filter type.
 * @fltr_idx:			Filter index (FILTER_0/FILTER_1).
 * @fltr_ports:			Ports on which filter is applied (bit-mapped).
 * @fltr_match:			Filter match value.
 * @fltr_mask:			Filter mask value.
 */
struct filter_config {
	u8 fltr_type;
	u8 fltr_idx;
	u16 fltr_ports;
	u64 fltr_match;
	u64 fltr_mask;
} __packed;

/*
 * struct filter:		Filter information.
 *
 * @fltr_idx:			Filter index (FILTER_0/FILTER_1).
 * @fltr_en:			Filter enable flag.
 */
struct filter {
	u8 fltr_idx : 7;
	u8 fltr_en : 1;
} __packed;

/*
 * struct port_event_config  SLC sub-block port and event configuration.
 *
 * @port:    SLC sub-block port number.
 * @event:    SLC sub-block event number..
 */
struct port_event_config {
	u8 port;
	u16 event;
} __packed;

/*
 * struct counter_config		PERFMON_COUNTER_CONFIG SCMI interface.
 *
 * @fltr:		Filter information.
 * @prt_evnt_map	port event mapping.
 */
struct counter_config {
	struct filter fltr;
	struct port_event_config prt_evnt_map;
} __packed;

/*
 * struct timer_config	PERFMON_TIMER_CONFIG SCMI interface
 *
 * @timer:		Timer value in ms.
 */
struct timer_config {
	u32 timer;
} __packed;

/*
 * struct perfmon_start_config	PERFMON_START SCMI interface
 *
 * @start:		Perfmon start/stop flag.
 */
struct perfmon_start_config {
	u8 start;
} __packed;

/*
 * struct perfmon_return_status	SCMI return status for SET PARAM
 *
 * @status:		Return status
 */
struct perfmon_return_status {
	u32 status;
} __packed;

/*
 * struct qcom_slc_perfmon_mem		Perfmon shared memory template.
 *
 * @last_captured_time:		Last capture time for counter dumps.
 * @match_seq:			Match sequence for synchroization.
 * @dump:			Counter dumps.
 */
struct qcom_slc_perfmon_mem {
	u64 last_captured_time;
	u64 match_seq;
	u64 dump[];
} __packed;

#endif /* _QCOM_SLC_PERFMON_SCMI_H_ */
