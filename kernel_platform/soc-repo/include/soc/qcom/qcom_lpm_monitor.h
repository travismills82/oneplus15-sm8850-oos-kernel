/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef QLPMM_H
#define QLPMM_H

#define QLPMM_MAX_NAME_LEN 41
#define QLPMM_MAX_CLOCK_NUM 40
#define QLPMM_IND_TOUT msecs_to_jiffies(500)

struct qlpmm_sslv_clock {
	u32 count;
	u64 last_on_ts;
	u64 last_off_ts;
	char clock_name[QLPMM_MAX_NAME_LEN];
};

struct qlpmm_ss_clks_info {
	u32 lpm_resource_count;
	u32 num_clk_enabled;
	char resource_name[QLPMM_MAX_NAME_LEN];
	struct qlpmm_sslv_clock data[QLPMM_MAX_CLOCK_NUM];
};

struct qlpmm_ss_info {
	struct list_head node;
	struct sockaddr_qrtr client_sq;
	bool client_connected;
	bool rw_flag;
	u32 magic_num;
	u32 subsystem_id;
	struct qlpmm_ss_clks_info ci;
};

struct qlpmm_platform_data {
	int connected_client_num;
	struct dentry *root;
	struct completion comp;
	struct qmi_handle handle;
	struct mutex lock;
	struct list_head subsystem_list;
};

static struct qlpmm_platform_data *qlpmm_pd;

#if IS_ENABLED(CONFIG_QCOM_LPM_MONITOR)

int qlpmm_get_subsystem_lpm_data(u32 subsystem_id,
			       struct qlpmm_ss_clks_info *ss_clks_info);

#else

int qlpmm_get_subsystem_lpm_data(u32 subsystem_id,
			       struct qlpmm_ss_clks_info *ss_clks_info)
{ return -ENODEV; }

#endif

#endif /* QLPMM_H */
