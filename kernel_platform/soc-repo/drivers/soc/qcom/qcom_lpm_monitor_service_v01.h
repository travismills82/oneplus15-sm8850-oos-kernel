/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef QCOM_LPM_MONITOR_SERVICE_V01_H
#define QCOM_LPM_MONITOR_SERVICE_V01_H

#define LPM_MONITOR_SERVICE_ID_V01 0x450
#define LPM_MONITOR_SERVICE_VERS_V01 0x01

#define QMI_LPM_MONITOR_REPORT_STATS_RESP_V01 0x0003
#define QMI_LPM_MONITOR_REPORT_STATS_REQ_V01 0x0003
#define QMI_LPM_MONITOR_INITIATE_REPORT_IND_V01 0x0004

#define QMI_LPMM_CLK_NUM_MAX_V01 40
#define QMI_LPMM_SS_NUM_MAX_V01 1
#define QMI_LPM_MONITOR_RESOURCE_NAME_LENGTH_MAX_V01 40
#define QMI_LPM_MONITOR_MAGIC_NUMBER_V01 0xabcddcba

enum lpm_monitor_subsystem_id_type_enum_t_v01 {
	LPM_MONITOR_SUBSYSTEM_ID_TYPE_ENUM_T_MIN_VAL_V01 = INT_MIN,
	SUBSYSTEM_ADSP_V01 = 0,
	SUBSYSTEM_ID_MAX_V01 = 1,
	LPM_MONITOR_SUBSYSTEM_ID_TYPE_ENUM_T_MAX_VAL_V01 = INT_MAX,
};

enum lpm_monitor_subsystem_lpr_type_enum_t_v01 {
	LPM_MONITOR_SUBSYSTEM_LPR_TYPE_ENUM_T_MIN_VAL_V01 = INT_MIN,
	SUBSYSTEM_LPM_XO_V01 = 0,
	SUBSYSTEM_LPM_NUM_MAX_V01 = 1,
	LPM_MONITOR_SUBSYSTEM_LPR_TYPE_ENUM_T_MAX_VAL_V01 = INT_MAX,
};

struct lpm_monitor_string_t_v01 {
	char qmi_string[QMI_LPM_MONITOR_RESOURCE_NAME_LENGTH_MAX_V01 + 1];
};

struct lpm_monitor_subsystem_lpm_violator_clock_t_v01 {
	u32 count;
	u64 last_on_timestamp;
	u64 last_off_timestamp;
	struct lpm_monitor_string_t_v01 clock_name;
};

struct lpm_monitor_subsystem_lpm_violator_resource_t_v01 {
	struct lpm_monitor_string_t_v01 resource_name;
	u32 num_clk_enabled;
	struct lpm_monitor_subsystem_lpm_violator_clock_t_v01 data[QMI_LPMM_CLK_NUM_MAX_V01];
};

struct lpm_monitor_report_to_apps_req_msg_v01 {
	u32 subsystem_id;
	u32 lpm_resource_count;
	u32 magic_number;
	struct lpm_monitor_subsystem_lpm_violator_resource_t_v01 data[QMI_LPMM_SS_NUM_MAX_V01];
};
#define LPM_MONITOR_REPORT_TO_APPS_REQ_MSG_V01_MAX_MSG_LEN 2509
extern struct qmi_elem_info lpm_monitor_report_to_apps_req_msg_v01_ei[];

struct lpm_monitor_report_to_apps_resp_msg_v01 {
	struct qmi_response_type_v01 resp;
};
#define LPM_MONITOR_REPORT_TO_APPS_RESP_MSG_V01_MAX_MSG_LEN 7
extern struct qmi_elem_info lpm_monitor_report_to_apps_resp_msg_v01_ei[];

struct lpm_monitor_handling_ind_msg_v01 {
	char placeholder;
};
#define LPM_MONITOR_HANDLING_IND_MSG_V01_MAX_MSG_LEN 0
extern struct qmi_elem_info lpm_monitor_handling_ind_msg_v01_ei[];

#endif
