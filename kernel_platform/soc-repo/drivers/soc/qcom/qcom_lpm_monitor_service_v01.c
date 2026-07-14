// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/soc/qcom/qmi.h>

#include "qcom_lpm_monitor_service_v01.h"

static struct qmi_elem_info lpm_monitor_string_t_v01_ei[] = {
	{
		.data_type      = QMI_STRING,
		.elem_len       = QMI_LPM_MONITOR_RESOURCE_NAME_LENGTH_MAX_V01 + 1,
		.elem_size      = sizeof(char),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_string_t_v01,
					   qmi_string),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static struct qmi_elem_info lpm_monitor_subsystem_lpm_violator_clock_t_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_clock_t_v01,
					   count),
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_clock_t_v01,
					   last_on_timestamp),
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_clock_t_v01,
					   last_off_timestamp),
	},
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct lpm_monitor_string_t_v01),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_clock_t_v01,
					   clock_name),
		.ei_array      = lpm_monitor_string_t_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static struct qmi_elem_info lpm_monitor_subsystem_lpm_violator_resource_t_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct lpm_monitor_string_t_v01),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_resource_t_v01,
					   resource_name),
		.ei_array      = lpm_monitor_string_t_v01_ei,
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_resource_t_v01,
					   num_clk_enabled),
	},
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = QMI_LPMM_CLK_NUM_MAX_V01,
		.elem_size      = sizeof(struct lpm_monitor_subsystem_lpm_violator_clock_t_v01),
		.array_type       = STATIC_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct
					   lpm_monitor_subsystem_lpm_violator_resource_t_v01,
					   data),
		.ei_array      = lpm_monitor_subsystem_lpm_violator_clock_t_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

struct qmi_elem_info lpm_monitor_report_to_apps_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct
					   lpm_monitor_report_to_apps_req_msg_v01,
					   subsystem_id),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct
					   lpm_monitor_report_to_apps_req_msg_v01,
					   lpm_resource_count),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct
					   lpm_monitor_report_to_apps_req_msg_v01,
					   magic_number),
	},
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = QMI_LPMM_CLK_NUM_MAX_V01,
		.elem_size      = sizeof(struct lpm_monitor_subsystem_lpm_violator_resource_t_v01),
		.array_type       = STATIC_ARRAY,
		.tlv_type       = 0x04,
		.offset         = offsetof(struct
					   lpm_monitor_report_to_apps_req_msg_v01,
					   data),
		.ei_array      = lpm_monitor_subsystem_lpm_violator_resource_t_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

struct qmi_elem_info lpm_monitor_report_to_apps_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct
					   lpm_monitor_report_to_apps_resp_msg_v01,
					   resp),
		.ei_array      = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

struct qmi_elem_info lpm_monitor_handling_ind_msg_v01_ei[] = {
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};
