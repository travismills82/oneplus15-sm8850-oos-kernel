/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _HFI_MSM_DRV_H_
#define _HFI_MSM_DRV_H_

#include "msm_drv.h"
#include "hfi_props.h"
#include "hfi_adapter.h"

#define MAX_NUM_HFI_RESOURCES 8

#define MSM_DRV_HFI_ID 0

#define HFI_CMD_BUFF_DEVICE 0

struct hfi_bw_config {
	u64	ab_vote[SDE_POWER_HANDLE_DBUS_ID_MAX];
	u64	ib_vote[SDE_POWER_HANDLE_DBUS_ID_MAX];
};

static const u32 bw_resources_hfi_props[MAX_NUM_HFI_RESOURCES] = {
	HFI_PROPERTY_DEVICE_CORE_IB,
	HFI_PROPERTY_DEVICE_CORE_AB,
	HFI_PROPERTY_DEVICE_LLCC_IB,
	HFI_PROPERTY_DEVICE_LLCC_AB,
	HFI_PROPERTY_DEVICE_DRAM_IB,
	HFI_PROPERTY_DEVICE_DRAM_AB,
};

struct msm_drm_hfi_private {
	struct msm_drm_private *base;
	struct hfi_adapter_t *hfi_adapter;
	struct hfi_client_t *msm_drv_hfi_client;
};

struct hfi_resources_register {
	u32 num_of_resources;
	u32 resource_property_id[MAX_NUM_HFI_RESOURCES];
};

/**
 * hfi_msm_drv_hfi_init
 */
int hfi_msm_drv_hfi_init(struct msm_drm_private *priv);

/**
 * hfi_msm_drv_init
 */
int hfi_msm_drv_init(struct drm_device *dev);

/**
 * msm_drv_client_cmdbuf_handler
 */
int msm_drv_client_cmdbuf_handler(struct hfi_client_t *client, struct hfi_cmdbuf_t *cmd_buf);

#endif  // _HFI_MSM_DRV_H_
