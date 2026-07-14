// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/types.h>
#include <linux/of_device.h>
#include <video/mipi_display.h>

#include "msm_drv.h"
#include "hfi_msm_drv.h"
#include "msm_gem.h"
#include "msm_mmu.h"
#include "sde_kms.h"
#include "hfi_kms.h"
#include "dsi_drm.h"
#include "dsi_display_hfi.h"
#include "dsi_display.h"
#include "dsi_hfi.h"
#include "dsi_pwr.h"
#include "dsi_panel.h"
#include "sde_connector.h"
#include "sde_kms.h"
#include "sde_dbg.h"

int dsi_display_hfi_panel_enable_supplies(struct dsi_display *display, bool enable)
{
	int rc = 0;

	if (!display->panel) {
		DSI_ERR("invalid panel\n");
		return -EINVAL;
	}

	mutex_lock(&display->panel->panel_lock);

	if (enable) {
		if (display->panel->powered)
			goto error;

		DSI_DEBUG("powering on panel\n");
		rc = dsi_panel_power_on(display->panel);
		if (rc) {
			DSI_ERR("dsi panel failed to enable power supplies\n");
			goto error;
		}

		display->panel->powered = true;
	} else {
		if (!display->panel->powered)
			goto error;

		DSI_DEBUG("powering off panel\n");
		rc = dsi_panel_power_off(display->panel);
		if (rc) {
			DSI_ERR("dsi panel failed to disable power supplies\n");
			goto error;
		}

		display->panel->powered = false;
	}

error:
	mutex_unlock(&display->panel->panel_lock);
	return rc;
}

static int dsi_display_hfi_set_mode(struct dsi_display *display, struct dsi_display_mode *mode)
{
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_client_t *hfi_client;
	struct hfi_display_mode_info *hfi_mode_info;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_SET_MODE;
	int rc = 0;

	sde_kms = sde_connector_get_kms(display->drm_conn);
	if (!sde_kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms)
		return -EINVAL;

	hfi_client = &hfi_kms->hfi_client;

	hfi_mode_info = kvzalloc(sizeof(struct hfi_display_mode_info), GFP_KERNEL);
	if (!hfi_mode_info)
		return -ENOMEM;

	hfi_mode_info->size =			sizeof(struct hfi_display_mode_info);
	hfi_mode_info->h_active =		mode->timing.h_active;
	hfi_mode_info->h_back_porch =		mode->timing.h_back_porch;
	hfi_mode_info->h_sync_width =		mode->timing.h_sync_width;
	hfi_mode_info->h_front_porch =		mode->timing.h_front_porch;
	hfi_mode_info->h_skew =			mode->timing.h_skew;
	hfi_mode_info->h_sync_polarity =	mode->timing.h_sync_polarity;
	hfi_mode_info->v_active =		mode->timing.v_active;
	hfi_mode_info->v_back_porch =		mode->timing.v_back_porch;
	hfi_mode_info->v_sync_width =		mode->timing.v_sync_width;
	hfi_mode_info->v_front_porch =		mode->timing.v_front_porch;
	hfi_mode_info->v_sync_polarity =	mode->timing.v_sync_polarity;
	hfi_mode_info->refresh_rate =		mode->timing.refresh_rate;
	hfi_mode_info->clk_rate_hz_lo =		HFI_VAL_L32(mode->timing.clk_rate_hz);
	hfi_mode_info->clk_rate_hz_hi =		HFI_VAL_H32(mode->timing.clk_rate_hz);
	hfi_mode_info->flags_lo =		mode->dsi_mode_flags;

	rc = dsi_display_hfi_send_cmd_buf(display, hfi_client, hfi_cmd, display->display_type,
			HFI_PAYLOAD_TYPE_U32_ARRAY, hfi_mode_info, hfi_mode_info->size,
			(HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc)
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_SET_MODE, rc=%d\n", rc);
	kfree(hfi_mode_info);

	return rc;
}

int dsi_display_hfi_prepare(struct dsi_display *display)
{
	int rc = 0;
	bool hfi_power_enable = true;

	if (!display) {
		DSI_ERR("Invalid params\n");
		goto end;
	}

	rc = dsi_display_hfi_panel_enable_supplies(display, hfi_power_enable);
	if (rc) {
		DSI_ERR("[%s] dsi panel power supply %s failed, rc=%d\n", display->name,
			hfi_power_enable ? "enable" : "disable", rc);
			goto end;
	}

	rc = dsi_display_hfi_set_mode(display, display->panel->cur_mode);
	if (rc)
		DSI_ERR("set mode failed, rc=%d\n", rc);

end:
	DSI_DEBUG("%s: DSI core power, enable=%d\n", __func__, hfi_power_enable);
	return rc;
}

int dsi_display_hfi_enable(struct dsi_display *display)
{
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_client_t *hfi_client;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_ENABLE;
	int rc = 0;

	sde_kms = sde_connector_get_kms(display->drm_conn);
	if (!sde_kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms)
		return -EINVAL;

	hfi_client = &hfi_kms->hfi_client;

	rc = dsi_display_hfi_send_cmd_buf(display, hfi_client, hfi_cmd, display->display_type,
			HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			(HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc) {
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_ENABLE, rc=%d\n", rc);
		goto error;
	}

	display->panel->panel_initialized = true;

error:
	return rc;
}

int dsi_display_hfi_post_enable(struct dsi_display *display)
{
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_client_t *hfi_client;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_POST_ENABLE;
	int rc = 0;

	sde_kms = sde_connector_get_kms(display->drm_conn);
	if (!sde_kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms)
		return -EINVAL;

	hfi_client = &hfi_kms->hfi_client;

	rc = dsi_display_hfi_send_cmd_buf(display, hfi_client, hfi_cmd, display->display_type,
			HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			(HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc)
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_POST_ENABLE, rc=%d\n", rc);

	return rc;
}

int dsi_display_hfi_pre_disable(struct dsi_display *display)
{
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_client_t *hfi_client;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_DISABLE;
	int rc = 0;

	sde_kms = sde_connector_get_kms(display->drm_conn);
	if (!sde_kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms)
		return -EINVAL;

	hfi_client = &hfi_kms->hfi_client;

	rc = dsi_display_hfi_send_cmd_buf(display, hfi_client, hfi_cmd, display->display_type,
			HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			(HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc)
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_DISABLE, rc=%d\n", rc);

	return rc;
}

int dsi_display_hfi_disable(struct dsi_display *display)
{
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_client_t *hfi_client;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_POST_DISABLE;
	int rc = 0;

	sde_kms = sde_connector_get_kms(display->drm_conn);
	if (!sde_kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms)
		return -EINVAL;

	hfi_client = &hfi_kms->hfi_client;

	rc = dsi_display_hfi_send_cmd_buf(display, hfi_client, hfi_cmd, display->display_type,
			HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			(HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc) {
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_POST_DISABLE, rc=%d\n", rc);
		goto error;
	}

	display->panel->panel_initialized = false;

error:
	return rc;
}

int dsi_display_hfi_unprepare(struct dsi_display *display)
{
	int rc = 0;
	bool hfi_power_enable = false;

	if (!display) {
		DSI_ERR("Invalid params\n");
		goto end;
	}

	rc = dsi_display_hfi_panel_enable_supplies(display, hfi_power_enable);
	if (rc) {
		DSI_ERR("[%s] dsi panel power supply %s failed, rc=%d\n", display->name,
			hfi_power_enable ? "enable" : "disable", rc);
	}

end:
	DSI_DEBUG("%s: DSI core power, enable=%d\n", __func__, hfi_power_enable);
	return rc;
}

void dsi_display_setup_ops(struct dsi_display *display)
{
	display->display_ops.display_prepare[MSM_DISP_OP_HFI] = dsi_display_hfi_prepare;
	display->display_ops.display_enable[MSM_DISP_OP_HFI] = dsi_display_hfi_enable;
	display->display_ops.post_enable[MSM_DISP_OP_HFI] = dsi_display_hfi_post_enable;
	display->display_ops.pre_disable[MSM_DISP_OP_HFI] = dsi_display_hfi_pre_disable;
	display->display_ops.display_disable[MSM_DISP_OP_HFI] = dsi_display_hfi_disable;
	display->display_ops.display_unprepare[MSM_DISP_OP_HFI] = dsi_display_hfi_unprepare;
	display->display_ops.misr_setup[MSM_DISP_OP_HFI] = dsi_hfi_misr_setup;
	display->display_ops.misr_read[MSM_DISP_OP_HFI] = dsi_hfi_misr_read;

	display->display_ops.display_prepare[MSM_DISP_OP_HWIO] = dsi_display_prepare;
	display->display_ops.display_enable[MSM_DISP_OP_HWIO] = dsi_display_enable;
	display->display_ops.post_enable[MSM_DISP_OP_HWIO] = dsi_display_post_enable;
	display->display_ops.pre_disable[MSM_DISP_OP_HWIO] = dsi_display_pre_disable;
	display->display_ops.display_disable[MSM_DISP_OP_HWIO] = dsi_display_disable;
	display->display_ops.display_unprepare[MSM_DISP_OP_HWIO] = dsi_display_unprepare;
}

static int dsi_hfi_copy_and_pad_cmd(const struct mipi_dsi_packet *packet,
				     u8 *buf,
				     u32 *size)
{
	int rc = 0;
	u32 len, i;
	u8 cmd_type = 0;

	len = packet->size;
	len += 0x3; len &= ~0x03; /* Align to 32 bits */

	if (!buf)
		return -ENOMEM;

	for (i = 0; i < len; i++) {
		if (i >= packet->size)
			buf[i] = 0xFF;
		else if (i < sizeof(packet->header))
			buf[i] = packet->header[i];
		else
			buf[i] = packet->payload[i - sizeof(packet->header)];
	}

	if (packet->payload_length > 0)
		buf[3] |= BIT(6);

	/* Swap BYTE order in the command buffer for MSM */
	buf[0] = packet->header[1];
	buf[1] = packet->header[2];
	buf[2] = packet->header[0];

	/* send embedded BTA for read commands */
	cmd_type = buf[2] & 0x3f;
	if ((cmd_type == MIPI_DSI_DCS_READ) ||
			(cmd_type == MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM) ||
			(cmd_type == MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM) ||
			(cmd_type == MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM))
		buf[3] |= BIT(5);

	*size = len;

	return rc;
}

int dsi_hfi_packetize_panel_cmd(struct dsi_cmd_desc *cmd_desc, u32 *size_of_indv_cmd, u8 *buffer)
{
	int rc = 0;
	struct mipi_dsi_packet packet;
	const struct mipi_dsi_msg *msg;
	u32 *flags;

	msg = &cmd_desc->msg;
	flags = &cmd_desc->ctrl_flags;

	rc = mipi_dsi_create_packet(&packet, msg);
	if (rc) {
		DSI_ERR("failed to create message packet, rc=%d\n", rc);
		goto error;
	}

	rc = dsi_hfi_copy_and_pad_cmd(&packet, buffer, size_of_indv_cmd);
	if (rc) {
		DSI_ERR("failed to copy message, rc=%d\n", rc);
		goto error;
	}

error:
	return rc;
}

static void dsi_display_aspace_cb_locked(void *cb_data, bool is_detach)
{
	struct dsi_display *display;
	int rc;

	if (!cb_data) {
		DSI_ERR("aspace cb called with invalid cb_data\n");
		return;
	}
	display = (struct dsi_display *)cb_data;

	/*
	 * acquire panel_lock to make sure no commands are in-progress
	 * while detaching the non-secure context banks
	 */
	dsi_panel_acquire_panel_lock(display->panel);

	if (is_detach) {
		/* invalidate the stored iova */
		display->cmd_buffer_iova = 0;

		/* return the virtual address mapping */
		msm_gem_put_vaddr(display->tx_cmd_buf);
		msm_gem_vunmap(display->tx_cmd_buf, OBJ_LOCK_NORMAL);

	} else {
		rc = msm_gem_get_iova(display->tx_cmd_buf,
				display->aspace, &(display->cmd_buffer_iova));
		if (rc) {
			DSI_ERR("failed to get the iova rc %d\n", rc);
			goto end;
		}

		display->vaddr =
			(void *) msm_gem_get_vaddr(display->tx_cmd_buf);

		if (IS_ERR_OR_NULL(display->vaddr)) {
			DSI_ERR("failed to get va rc %d\n", rc);
			goto end;
		}
	}

end:
	/* release panel_lock */
	dsi_panel_release_panel_lock(display->panel);
}

int dsi_hfi_host_alloc_cmd_tx_buffer(struct dsi_display *display)
{
	int rc = 0;

	display->tx_cmd_buf = msm_gem_new(display->drm_dev,
			SZ_4K,
			MSM_BO_UNCACHED);

	if ((display->tx_cmd_buf) == NULL) {
		DSI_ERR("Failed to allocate cmd tx buf memory\n");
		rc = -ENOMEM;
		goto error;
	}

	display->cmd_buffer_size = SZ_4K;

	display->aspace = msm_gem_smmu_address_space_get(
			display->drm_dev, MSM_SMMU_DOMAIN_UNSECURE);

	if (PTR_ERR(display->aspace) == -ENODEV) {
		display->aspace = NULL;
		DSI_DEBUG("IOMMU not present, relying on VRAM\n");
	} else if (IS_ERR_OR_NULL(display->aspace)) {
		rc = PTR_ERR(display->aspace);
		display->aspace = NULL;
		DSI_ERR("failed to get aspace %d\n", rc);
		goto free_gem;
	} else if (display->aspace) {
		/* register to aspace */
		rc = msm_gem_address_space_register_cb(display->aspace,
				dsi_display_aspace_cb_locked, (void *)display);
		if (rc) {
			DSI_ERR("failed to register callback %d\n", rc);
			goto free_gem;
		}
	}

	rc = msm_gem_get_iova(display->tx_cmd_buf, display->aspace,
				&(display->cmd_buffer_iova));
	if (rc) {
		DSI_ERR("failed to get the iova rc %d\n", rc);
		goto free_aspace_cb;
	}

	display->vaddr =
		(void *) msm_gem_get_vaddr(display->tx_cmd_buf);
	if (IS_ERR_OR_NULL(display->vaddr)) {
		DSI_ERR("failed to get va rc %d\n", rc);
		rc = -EINVAL;
		goto put_iova;
	}

	return rc;

put_iova:
	msm_gem_put_iova(display->tx_cmd_buf, display->aspace);
free_aspace_cb:
	msm_gem_address_space_unregister_cb(display->aspace,
			dsi_display_aspace_cb_locked, display);
free_gem:
	mutex_lock(&display->drm_dev->struct_mutex);
	msm_gem_free_object(display->tx_cmd_buf);
	mutex_unlock(&display->drm_dev->struct_mutex);
error:
	return rc;
}

int dsi_hfi_transition(struct dsi_display *display, enum hfi_display_power_mode lpm_state)
{
	int rc = 0;

	if (!lpm_state)
		return rc;

	rc = dsi_display_hfi_send_cmd_buf(display,
			display->dsi_hfi_info->hfi_client,
			HFI_COMMAND_DISPLAY_LP_STATE_REQ,
			display->display_type,
			HFI_PAYLOAD_TYPE_U32,
			(void *) lpm_state,
			sizeof(enum hfi_display_power_mode),
			(HFI_HOST_FLAGS_RESPONSE_REQUIRED | HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc)
		DSI_ERR("could not send hfi command, rc=%d\n", rc);

	return rc;
}
