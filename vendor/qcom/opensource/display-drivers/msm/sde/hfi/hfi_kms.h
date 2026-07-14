/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_KMS_H_
#define _HFI_KMS_H_

#include "sde_kms.h"
#include "hfi_adapter.h"
#include "hfi_msm_drv.h"
#include "linux/completion.h"

#define SDE_MAX_SSPP_COUNT  16

#define REC_ID(x)  ((x & 0xFF000000) >> 24)

/*
 * hfi_catalog_base - base struct for sde HW information
 *
 * @hw_rev	    HW version
 * @dcp_hw_rev  dcp HW version
 * @fw_rev		FW version
 * @vig_count		Number of VIG layers
 * @vig_indices		VIG layer indices
 * @dma_count		Number of DMA layers
 * @dma_indices		DMA layer indices
 * @max_display_count	Max display count
 * @wb_count		Number of writeback blocks
 * @wb_indices		Writeback block indices
 * @max_wb_linear_resolution	Max writeback linear resolution
 * @max_wb_ubwc_resolution	Max writeback UBWC resolution
 * @dsi_count		Number of DSI blocks
 * @dsi_indices		DSI block indices
 * @max_dsi_resolution	DSI maximum resolution
 * @max_cwb		Number of WB that support CWB concurrently
 * @dp_count		Number of DP blocks
 * @dp_indices		DP block indices
 * @max_dp_resolution	DP maximum resolution
 * @ds_count		count of destination scaler blocks
 * @ds_indices		DS block indices
 * @max_ds_resolution	Max resolution support of DS
 */
struct hfi_catalog_base {
	u32 dcp_hw_rev;
	u32 hw_rev;
	u32 fw_rev;
	u32 vig_count;
	u32 vig_indices[SDE_MAX_SSPP_COUNT];
	u32 virt_vig_count;
	u32 vig_r1_indices[SDE_MAX_SSPP_COUNT];
	u32 dma_count;
	u32 dma_indices[SDE_MAX_SSPP_COUNT];
	u32 virt_dma_count;
	u32 dma_r1_indices[SDE_MAX_SSPP_COUNT];
	u32 max_display_count;
	u32 wb_count;
	u32 wb_indices[MAX_BLOCKS];
	u32 max_wb_linear_resolution;
	u32 max_wb_ubwc_resolution;
	u32 dsi_count;
	u32 dsi_indices[MAX_BLOCKS];
	u32 max_dsi_resolution;
	u32 dp_count;
	u32 dp_indices[MAX_BLOCKS];
	u32 max_dp_resolution;
	u32 ds_count;
	u32 ds_indices[MAX_BLOCKS];
	u32 max_ds_resolution;
};

/**
 * struct hfi_kms - virtualized hfi kms structure
 * @base: Pointer to base sde kms structure
 * @hfi_client: hfi client structure
 * @hfi_adapter: hfi adapter structure
 * @device_init_listener: HFI listener object for catalog parsing
 * @resource_vote_listener: HFI listener object for resource vote
 * @cat_init_done: atomic variable tracking catalog parse status
 */
struct hfi_kms {
	struct sde_kms *base;
	struct hfi_client_t hfi_client;
	struct hfi_adapter_t *hfi_adapter;
	struct hfi_prop_listener device_init_listener;
	struct hfi_prop_listener resource_vote_listener;
	atomic_t cat_init_done;
	struct hfi_catalog_base *catalog;
};

/**
 * struct kms_hfi_cb - hfi extension of hfi callback structure
 * @client: hfi client structure
 * @cmd_buf: pointer to hfi_adapter command buffer
 * @cb_work: hfi callback work structure
 */
struct kms_hfi_cb {
	struct hfi_client_t *client;
	struct hfi_cmdbuf_t *cmd_buf;
	struct kthread_work cb_work;
};

#if IS_ENABLED(CONFIG_MDSS_HFI)
/**
 * hfi_kms_reg_client - Register SDE as a client with HFI
 * @dev:        Pointer to drm device structure
 * Returns:     0 on success, or error code on failure
 */
int hfi_kms_reg_client(struct drm_device *dev);
#else
int hfi_kms_reg_client(struct drm_device *dev);
#endif // IS_ENABLED(CONFIG_MDSS_HFI)

#if IS_ENABLED(CONFIG_MDSS_HFI)
/**
 * hfi_kms_init - initialize virtual hfi kms object
 * @sde_kms:        Pointer to sde kms structure
 * Returns:     0 on success, or error code on failure
 */
int hfi_kms_init(struct sde_kms *sde_kms);
#else
int hfi_kms_init(struct sde_kms *sde_kms);
#endif // IS_ENABLED(CONFIG_MDSS_HFI)

/**
 * hfi_kms_resource_vote_hfi_prop_handler - listener function for resource voting
 * @UNIQUE_DISP_OR_OBJ_ID: Unique ID for display or object
 * @CMD_ID: HFI Command ID for which callback received
 * @payload: Pointer to the payload data
 * @size: Size of the payload
 * @resource_vote_listener: Pointer to the resource vote listener structure
 * Returns: This function does not return a value.
 */
void hfi_kms_resource_vote_hfi_prop_handler(u32 UNIQUE_DISP_OR_OBJ_ID, u32 CMD_ID, void *payload,
		u32 size, struct hfi_prop_listener *resource_vote_listener);

/**
 * to_hfi_kms - convert sde_kms pointer to hfi kms pointer
 * @X: Pointer to sde_kms structure
 * Returns: Pointer to hfi_kms structure
 */
#define to_hfi_kms(x) x->hfi_kms

/**
 * hfi_kms_get_cmd_buf - retrieve a command buffer for a specific display and cmd_type
 * @hfi_kms: Pointer to hfi_kms structure
 * @display_id: ID of display for which buffer is requested
 * @cmd_type: type of command buffer needed
 * Returns: pointer to the command buffer structure on success,
 *          or NULL on failure
 */
struct hfi_cmdbuf_t *hfi_kms_get_cmd_buf(struct hfi_kms *hfi_kms,
		u16 display_id, u32 cmd_type);

/**
 * hfi_kms_get_catalog_data - get hfi catalog data
 * @hfi_kms: Pointer to hfi_kms structure
 * Returns: 0 on success, or error code on failure
 */
int hfi_kms_get_catalog_data(struct hfi_kms *hfi_kms);

/**
 * hfi_kms_get_plane_indices - get hfi plane indices
 * @hfi_kms: Pointer to hfi_kms structure
 * @vig_pipe: True if VIG pipe
 * @pipe_idx: sde pipe id
 * @rect1: true if rect1
 * @hfi_pipe_id: func updates hfi pipe idx for pipe_idx
 * Returns: 0 on success, or error code on failure
 */
int hfi_kms_get_plane_indices(struct hfi_kms *hfi_kms, bool vig_pipe, uint32_t pipe_idx,
		bool rect1, uint32_t *hfi_pipe_id);

/**
 * hfi_kms_set_reg_dma_buffer - send LUT DMA last command buffer to FW
 * @hfi_kms: Pointer to hfi_kms structure
 * @buffer: Pointer to LUT DMA last command buffer
 * Returns: 0 on success, or error code on failure
 */
int hfi_kms_set_reg_dma_buffer(struct hfi_kms *hfi_kms, struct sde_reg_dma_buffer *buffer);

#endif // _HFI_KMS_H_
