/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CPUSS_TELEMETRY_H_
#define _CPUSS_TELEMETRY_H_

#define MAX_NAME_LENGTH (16)

enum telemetry_set_param_ids {
	SET_PARAM_REQUEST_UPDATE = 1,
	SET_PARAM_REQUEST_RESET,
};

struct telemetry_global_param_t {
	u32 signature;
	u16 revision;
	u16 attributes;
	u64 match_sequence;
	u64 reserved;
	u16 num_max_core;
	u16 num_max_cluster;
	u32 num_max_counters;
	uintptr_t pname;
	uintptr_t pvalue;
} __packed;

struct telemetry_counter_attributes_name_t {
	const char name[MAX_NAME_LENGTH];
};

struct telemetry_counter_attributes_value_t {
	int64_t value;
};

#if IS_ENABLED(CONFIG_QCOM_CPUSS_TELEMETRY)
const char *get_telemetry_counter_name(u32 counter_id);
int64_t get_telemetry_counter_value(u32 counter_id);
#else
static inline const char *get_telemetry_counter_name(u32 counter_id) { return NULL; }
static inline int64_t get_telemetry_counter_value(u32 counter_id) { return -ENODEV; }
#endif
#endif /* CPUSS_TELEMETRY_H */
