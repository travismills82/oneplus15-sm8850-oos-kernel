// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/types.h>

#include "msm_drv.h"
#include "msm_mmu.h"
#include "hfi_msm_drv.h"
#include "hfi_connector.h"
#include "dsi_drm.h"
#include "dsi_defs.h"
#include "dsi_display_hfi.h"
#include "dsi_display.h"
#include "dsi_hfi.h"
#include "dsi_parser.h"
#include "hfi_adapter.h"
#include "hfi_props.h"
#include "hfi_kms.h"

static int dsi_display_hfi_power_supplies(struct dsi_display *display,
					  u32 hfi_power_control, bool hfi_power_enable)
{
	int rc = 0;

	if (!display) {
		DSI_ERR("Invalid dsi_display\n");
		goto end;
	}

	if (hfi_power_control & HFI_PANEL_POWER) {
		rc = dsi_display_hfi_panel_enable_supplies(display, hfi_power_enable);
		if (rc) {
			DSI_ERR("[%s] dsi panel power supply %s failed, rc=%d\n", display->name,
				hfi_power_enable ? "enable" : "disable", rc);
			if (hfi_power_enable)
				goto error_panel_disable;
		}
	}

	goto end;

error_panel_disable:
	if (hfi_power_control & HFI_PANEL_POWER)
		(void)dsi_display_hfi_panel_enable_supplies(display, false);
end:
	DSI_DEBUG("%s: DSI core power, hfi_pwr_mask=%d, enable=%d\n", __func__,
		hfi_power_control, hfi_power_enable);
	return rc;
}

int dsi_hfi_process_cmd_buf(struct hfi_client_t *hfi_client, struct hfi_cmdbuf_t *cmd_buf)
{
	int rc = 0;

	if (!hfi_client || !cmd_buf) {
		DSI_ERR("Invalid client or buffer\n");
		return -EINVAL;
	}

	rc = hfi_adapter_unpack_cmd_buf(hfi_client, cmd_buf);
	if (rc) {
		DSI_ERR("[WARNING] Error in response packet or unpacking buffer\n");
		return rc;
	}

	rc = hfi_adapter_release_cmd_buf(cmd_buf);
	if (rc)
		DSI_ERR("[WARNING] Failed to release command buffer\n");

	return rc;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
int dsi_hfi_misr_setup(struct dsi_display *display)
{
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	struct dsi_display_hfi *display_hfi;
	struct misr_setup_data misr_data;
	u32 obj_id;
	int rc = 0;

	if (!display)
		return -EINVAL;

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	struct drm_connector *drm_conn = display->drm_conn;

	if (!drm_conn)
		return -EINVAL;

	/* TODO! Check if we want to use this as display_id or the display->display_type*/
	obj_id = sde_conn_get_display_obj_id(drm_conn);

	cmd_buf = hfi_adapter_get_cmd_buf(display_hfi->hfi_client, obj_id,
			HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		DSI_ERR("Failed to get valid command buffer\n");
		return -EINVAL;
	}

	misr_data.display_id = obj_id;
	misr_data.enable = display->misr_enable;
	misr_data.frame_count = display->misr_frame_count;
	misr_data.module_type = HFI_DEBUG_MISR_DSI;

	rc = hfi_adapter_add_set_property(cmd_buf, HFI_COMMAND_DEBUG_MISR_SETUP, obj_id,
					HFI_PAYLOAD_TYPE_U32_ARRAY, &misr_data,
					sizeof(misr_data), HFI_HOST_FLAGS_NONE);
	if (rc) {
		DSI_ERR("Failed to add property\n");
		return rc;
	}

	DSI_DEBUG("misr_setup: sending cmd buf\n");
	rc = hfi_adapter_set_cmd_buf(cmd_buf);
	SDE_EVT32(obj_id, HFI_COMMAND_DEBUG_MISR_SETUP, rc, SDE_EVTLOG_FUNC_CASE1);
	if (rc)
		DSI_ERR("Failed to send misr_setup command\n");

	return rc;
}

void dsi_hfi_process_misr_read(struct dsi_display *display, void *payload, u32 size)
{
	struct misr_read_data_ret *misr_data;
	struct dsi_misr_values *misr_read_values;
	u32 max_count = 0;
	u32 module_type = 0;
	u32 *misr_values;

	if (!payload || !size) {
		DSI_ERR("Invalid payload received from FW\n");
		return;
	}

	DSI_DEBUG("About to read MISR values from %s\n", __func__);

	misr_read_values = &display->misr_vals;
	misr_data = (struct misr_read_data_ret *)payload;

	max_count = misr_data->num_misr;
	module_type = misr_data->module_type;
	DSI_DEBUG("Module type:%d, Max_count:%d\n",
		module_type, max_count);
	misr_values = (u32 *)(payload + sizeof(u32) * 2);

	memset(&misr_read_values->misr_values, 0, sizeof(u32) * MAX_MISR_MODULES);

	for (int i = 0; i < max_count; i++)
		misr_read_values->misr_values[i] = misr_values[i];

	misr_read_values->count = max_count;

}

int dsi_hfi_misr_read(struct dsi_display *display)
{
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	struct dsi_display_hfi *display_hfi;
	struct misr_read_data misr_read;
	struct drm_connector *drm_conn;
	u32 obj_id;
	int rc = 0;

	if (!display)
		return -EINVAL;

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	drm_conn = display->drm_conn;

	obj_id = sde_conn_get_display_obj_id(drm_conn);

	cmd_buf = hfi_adapter_get_cmd_buf(display_hfi->hfi_client, obj_id,
			HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		DSI_ERR("Failed to get valid command buffer\n");
		return -EINVAL;
	}

	misr_read.display_id = obj_id;
	misr_read.module_type = HFI_DEBUG_MISR_DSI;

	rc = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEBUG_MISR_READ, obj_id,
			HFI_PAYLOAD_TYPE_U32_ARRAY, &misr_read, sizeof(misr_read),
			&display->hfi_cb_obj, (HFI_HOST_FLAGS_RESPONSE_REQUIRED |
			HFI_HOST_FLAGS_NON_DISCARDABLE));
	if (rc)
		DSI_ERR("Failed to add MISR read command\n");

	SDE_EVT32(drm_conn->base.id, obj_id, HFI_COMMAND_DEBUG_MISR_READ, SDE_EVTLOG_FUNC_CASE1);
	rc = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
	SDE_EVT32(drm_conn->base.id, obj_id, HFI_COMMAND_DEBUG_MISR_READ, rc,
			SDE_EVTLOG_FUNC_CASE2);

	return rc;
}

#else
int dsi_hfi_misr_setup(struct dsi_display *display)
{
	return 0;
}

void dsi_hfi_process_misr_read(struct dsi_display *display, void *payload, u32 size)
{

}

int dsi_hfi_misr_read(struct dsi_display *display)
{
	return 0;
}

#endif /* CONFIG_DEBUG_FS */

void dsi_hfi_prop_handler(u32 hfi_uid, u32 prop, void *payload, u32 size,
			  struct hfi_prop_listener *listener)
{
	struct dsi_display_hfi *display_hfi;
	struct dsi_display *display;
	u32 dsi_display_obj_id;
	int rc = 0;

	if (!listener) {
		DSI_ERR("invalid listener\n");
		return;
	}

	display = container_of(listener, struct dsi_display,
						hfi_cb_obj);
	if (!display) {
		DSI_ERR("invalid object or listener from FW\n");
		return;
	}

	display_hfi = display->dsi_hfi_info;
	dsi_display_obj_id = strcmp(display->display_type, "primary");

	if (dsi_display_obj_id != hfi_uid) {
		DSI_ERR("Component and HFI ID mismatch (%d != %d)\n",
				dsi_display_obj_id, hfi_uid);
		return;
	}

	switch (prop) {
	case HFI_COMMAND_DISPLAY_MODE_VALIDATE:
		if (payload)
			display_hfi->mode_valid = true;
		break;
	case HFI_COMMAND_DISPLAY_POWER_CONTROL:
		rc = dsi_display_hfi_power_supplies(display,
								((u32 *)payload)[0],
								((bool *)payload)[1]);
		if (rc)
			DSI_ERR("Could not power on supplies\n");
		break;
	case HFI_COMMAND_DISPLAY_DISABLE:
		msleep(20);
		break;
	case HFI_COMMAND_DISPLAY_POST_DISABLE:
	case HFI_COMMAND_DISPLAY_ENABLE:
	case HFI_COMMAND_DISPLAY_POST_ENABLE:
	case HFI_COMMAND_DISPLAY_SET_MODE:
	case HFI_COMMAND_DISPLAY_POWER_REGISTER:
		break;
	case HFI_COMMAND_DEBUG_MISR_READ:
		dsi_hfi_process_misr_read(display, payload, size);
		break;
	default:
		DSI_ERR("Invalid HFI property 0x%x\n", prop);
	}

}

int dsi_display_hfi_setup_hfi(struct dsi_display *display, struct hfi_adapter_t *hfi_host)
{
	struct dsi_display_hfi *display_hfi;
	int rc = 0;

	if (!display) {
		DSI_ERR("invalid display\n");
		return -EINVAL;
	}

	if (!hfi_host) {
		DSI_ERR("invalid hfi_host\n");
		return -EINVAL;
	}

	display_hfi = kvzalloc(sizeof(struct dsi_display_hfi), GFP_KERNEL);
	display->dsi_hfi_info = display_hfi;

	display->hfi_cb_obj.hfi_prop_handler = dsi_hfi_prop_handler;
	display_hfi->hfi_adapter = hfi_host;

	display_hfi->hfi_client = kmalloc(sizeof(struct hfi_client_t), GFP_KERNEL);
	if (!display_hfi->hfi_client)
		return -ENOMEM;

	display_hfi->hfi_client->process_cmd_buf = dsi_hfi_process_cmd_buf;

	rc = hfi_adapter_client_register(hfi_host, display_hfi->hfi_client);
	if (rc) {
		DSI_ERR("unable to register hfi client\n");
		kfree(display_hfi->hfi_client);
		return -ENODEV;
	}

	return 0;
}

int dsi_display_hfi_send_cmd_buf(struct dsi_display *display,
					struct hfi_client_t *hfi_client, u32 hfi_cmd,
					const char *display_type, u32 hfi_payload_type,
					void *payload, u32 payload_size, u32 flags)
{
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	struct drm_connector *drm_conn;
	int rc = 0;
	u32 obj_id;

	if (!display) {
		DSI_ERR("invalid display\n");
		return -EINVAL;
	}

	drm_conn = display->drm_conn;

	switch (hfi_cmd) {
	case HFI_COMMAND_DISPLAY_MODE_VALIDATE:
		flags = HFI_HOST_FLAGS_NONE;
		break;
	case HFI_COMMAND_DISPLAY_SET_MODE:
	case HFI_COMMAND_DISPLAY_ENABLE:
	case HFI_COMMAND_DISPLAY_POST_ENABLE:
	case HFI_COMMAND_DISPLAY_POST_DISABLE:
	case HFI_COMMAND_DISPLAY_DISABLE:
		flags |= HFI_HOST_FLAGS_RESPONSE_REQUIRED;
		break;
	default:
		break;
	}

	obj_id = sde_conn_get_display_obj_id(drm_conn);

	cmd_buf = hfi_adapter_get_cmd_buf(hfi_client, obj_id,
			HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING);
	if (!cmd_buf) {
		DSI_ERR("could not get cmd_buf for hfi_cmd 0x%x\n", hfi_cmd);
		return -ENODEV;
	}

	if (flags & HFI_HOST_FLAGS_RESPONSE_REQUIRED) {
		rc = hfi_adapter_add_get_property(cmd_buf, hfi_cmd, obj_id, hfi_payload_type,
			payload, payload_size, &display->hfi_cb_obj, flags);
		if (rc)
			DSI_ERR("could not set property for hfi_cmd 0x%x\n", hfi_cmd);

		SDE_EVT32(obj_id, hfi_cmd, SDE_EVTLOG_FUNC_CASE1);
		rc = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
		SDE_EVT32(obj_id, hfi_cmd, rc, SDE_EVTLOG_FUNC_CASE2);
	} else {
		rc = hfi_adapter_add_set_property(cmd_buf, hfi_cmd, obj_id, hfi_payload_type,
			payload, payload_size, flags);
		if (rc)
			DSI_ERR("could not set property for hfi_cmd 0x%x\n", hfi_cmd);

		rc = hfi_adapter_set_cmd_buf(cmd_buf);
		SDE_EVT32(obj_id, hfi_cmd, rc, SDE_EVTLOG_FUNC_CASE3);
	}

	if (rc) {
		SDE_ERROR("failed to send hfi_cmd 0x%x\n", hfi_cmd);
		return rc;
	}

	return rc;
}

int dsi_display_hfi_register_pwr_supplies(struct dsi_display *display)
{
	struct dsi_display_hfi *display_hfi;
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	u32 obj_id;
	u32 hfi_cmd = HFI_COMMAND_DISPLAY_POWER_REGISTER;
	int rc = 0;

	if (!display) {
		DSI_ERR("invalid display\n");
		return -EINVAL;
	}

	display_hfi = display->dsi_hfi_info;

	obj_id = strcmp(display->display_type, "primary");

	cmd_buf = hfi_adapter_get_cmd_buf(display_hfi->hfi_client, obj_id,
					  HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING);

	rc = hfi_adapter_add_get_property(cmd_buf, hfi_cmd, obj_id, HFI_PAYLOAD_TYPE_NONE,
			NULL, 0, &display->hfi_cb_obj,
			HFI_HOST_FLAGS_RESPONSE_REQUIRED | HFI_HOST_FLAGS_NON_DISCARDABLE);

	rc = hfi_adapter_set_cmd_buf(cmd_buf);
	SDE_EVT32(obj_id, hfi_cmd, rc, SDE_EVTLOG_FUNC_CASE1);

	if (rc)
		DSI_ERR("Could not send HFI_COMMAND_DISPLAY_POWER_REGISTER, rc=%d\n", rc);

	return rc;
}

static void hfi_panel_get_mode_res_data(struct dsi_display_mode *mode,
					struct dsi_panel_timing_caps *timing_caps)
{
	timing_caps->res_data.active_width = mode->timing.h_active;
	timing_caps->res_data.active_height = mode->timing.v_active;
	timing_caps->res_data.h_front_porch = mode->timing.h_front_porch;
	timing_caps->res_data.h_back_porch = mode->timing.h_back_porch;
	timing_caps->res_data.h_sync_skew = mode->timing.h_skew;
	timing_caps->res_data.h_sync_pulse = mode->timing.h_sync_width;
	timing_caps->res_data.v_front_porch = mode->timing.v_front_porch;
	timing_caps->res_data.v_back_porch = mode->timing.v_back_porch;
	timing_caps->res_data.v_pulse_width = mode->timing.v_sync_width;
}

static void hfi_panel_get_mode_compression_params(struct dsi_display_mode *mode,
						  struct dsi_panel_timing_caps *timing_caps)
{
	u32 v_major = mode->priv_info->dsc.config.dsc_version_major;
	u32 v_minor = mode->priv_info->dsc.config.dsc_version_minor;

	timing_caps->compression_params.mode = HFI_PANEL_COMPRESSION_DSC;
	timing_caps->compression_params.version = ((v_major & 0x0F) << 4) | (v_minor & 0x0F);
	timing_caps->compression_params.scr_version = mode->priv_info->dsc.scr_rev;
	timing_caps->compression_params.slice_height = mode->priv_info->dsc.config.slice_height;
	timing_caps->compression_params.slice_width = mode->priv_info->dsc.config.slice_width;
	timing_caps->compression_params.slices_per_pkt = mode->priv_info->dsc.slice_per_pkt;
	timing_caps->compression_params.bits_per_component =
						mode->priv_info->dsc.config.bits_per_component;
	timing_caps->compression_params.pps_delay_ms = mode->priv_info->dsc.pps_delay_ms;
	timing_caps->compression_params.bits_per_pixel = mode->priv_info->dsc.config.bits_per_pixel;
	timing_caps->compression_params.chroma_format = mode->priv_info->dsc.chroma_format;
	timing_caps->compression_params.color_space = mode->priv_info->dsc.source_color_space;
	timing_caps->compression_params.block_prediction_enable =
						mode->priv_info->dsc.config.block_pred_enable;
}

static enum hfi_panel_phy_type dsi_get_panel_type_helper(struct dsi_panel *panel)
{
	switch (panel->panel_type) {
	case DSI_DISPLAY_PANEL_TYPE_LCD:
		return HFI_PANEL_PHY_LCD;
	case DSI_DISPLAY_PANEL_TYPE_OLED:
		return HFI_PANEL_PHY_OLED;
	default:
		return HFI_PANEL_PHY_OLED;
	}
}

static enum hfi_panel_bpp dsi_get_panel_bpp_helper(struct dsi_panel *panel)
{
	switch (panel->host_config.dst_format) {
	case DSI_PIXEL_FORMAT_RGB111:
		return HFI_PANEL_BPP_3;
	case DSI_PIXEL_FORMAT_RGB332:
		return HFI_PANEL_BPP_8;
	case DSI_PIXEL_FORMAT_RGB444:
		return HFI_PANEL_BPP_12;
	case DSI_PIXEL_FORMAT_RGB565:
		return HFI_PANEL_BPP_16;
	case DSI_PIXEL_FORMAT_RGB666:
		return HFI_PANEL_BPP_18;
	case DSI_PIXEL_FORMAT_RGB888:
		return HFI_PANEL_BPP_24;
	case DSI_PIXEL_FORMAT_RGB101010:
		return HFI_PANEL_BPP_30;
	default:
		return HFI_PANEL_BPP_24;
	}
}

static enum hfi_panel_lane_enable dsi_get_panel_lane_state_helper(struct dsi_panel *panel)
{
	enum hfi_panel_lane_enable state = 0;

	if (panel->host_config.data_lanes & DSI_DATA_LANE_0)
		state |= HFI_PANEL_LANE_0;
	if (panel->host_config.data_lanes & DSI_DATA_LANE_1)
		state |= HFI_PANEL_LANE_1;
	if (panel->host_config.data_lanes & DSI_DATA_LANE_2)
		state |= HFI_PANEL_LANE_2;
	if (panel->host_config.data_lanes & DSI_DATA_LANE_3)
		state |= HFI_PANEL_LANE_3;

	return state;
}

static enum hfi_panel_color_order_type dsi_get_panel_color_order_type(struct dsi_panel *panel)
{
	switch (panel->host_config.swap_mode) {
	case DSI_COLOR_SWAP_RGB:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_RGB;
	case DSI_COLOR_SWAP_RBG:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_RBG;
	case DSI_COLOR_SWAP_BRG:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_BRG;
	case DSI_COLOR_SWAP_GRB:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_GRB;
	case DSI_COLOR_SWAP_GBR:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_GBR;
	default:
		return HFI_PANEL_COLOR_ORDER_RGB_SWAP_RGB;
	}
}

static enum hfi_panel_trigger_type dsi_get_panel_trigger_type_helper(enum dsi_trigger_type type)
{
	switch (type) {
	case DSI_TRIGGER_NONE:
		return HFI_PANEL_TRIGGER_NONE;
	case DSI_TRIGGER_TE:
		return HFI_PANEL_TRIGGER_TE;
	case DSI_TRIGGER_SEOF:
		return HFI_PANEL_TRIGGER_SEOF;
	case DSI_TRIGGER_SW:
		return HFI_PANEL_TRIGGER_SW;
	case DSI_TRIGGER_SW_SEOF:
		return HFI_PANEL_TRIGGER_SW_SEOF;
	case DSI_TRIGGER_SW_TE:
		return HFI_PANEL_TRIGGER_SW_TE;
	default:
		return HFI_PANEL_TRIGGER_SW;
	}
}

static enum hfi_panel_fps_traffic_mode dsi_get_panel_traffic_mode_helper(struct dsi_panel *panel)
{
	switch (panel->video_config.traffic_mode) {
	case DSI_VIDEO_TRAFFIC_SYNC_PULSES:
		return HFI_PANEL_TRAFFIC_NON_BURST_SYNC_PULSE_MODE;
	case DSI_VIDEO_TRAFFIC_SYNC_START_EVENTS:
		return HFI_PANEL_TRAFFIC_NON_BURST_SYNC_EVENT_MODE;
	case DSI_VIDEO_TRAFFIC_BURST_MODE:
		return HFI_PANEL_TRAFFIC_BURST_MODE;
	default:
		return HFI_PANEL_TRAFFIC_NON_BURST_SYNC_PULSE_MODE;
	}
}

static enum hfi_panel_modes dsi_get_panel_op_mode_helper(struct dsi_panel *panel)
{
	switch (panel->panel_mode) {
	case DSI_OP_VIDEO_MODE:
		return HFI_PANEL_VIDEO_MODE_8_BIT;
	case DSI_OP_CMD_MODE:
		return HFI_PANEL_CMD_MODE_8_BIT;
	default:
		return HFI_PANEL_VIDEO_MODE_8_BIT;
	}
}

static enum hfi_panel_vsync_source dsi_get_panel_vsync_src(struct dsi_display *display)
{
	if (display->panel->te_using_watchdog_timer)
		return HFI_PANEL_VSYNC_SOURCE_WD;
	else
		return (enum hfi_panel_vsync_source)display->te_source;
}

static enum hfi_panel_lane_map dsi_get_panel_lane_map_helper(struct dsi_panel *panel)
{
	return HFI_PANEL_LANE_MAP_0123;
}

static void dsi_get_panel_ctrl_nums_helper(struct dsi_display *display, u32 *ctrl_array)
{
	char *dsi_ctrl_name = "qcom,dsi-ctrl-num";
	int cnt, i;

	cnt = dsi_display_get_phandle_count(display,
					dsi_ctrl_name);
	ctrl_array[0] = cnt;

	for (i = 0; i < cnt; i++)
		ctrl_array[i+1] = i;
}

static void dsi_get_panel_phy_nums_helper(struct dsi_display *display, u32 *phy_array)
{
	char *dsi_phy_name = "qcom,dsi-phy-num";
	int cnt, i;

	cnt = dsi_display_get_phandle_count(display,
					dsi_phy_name);
	phy_array[0] = cnt;

	for (i = 0; i < cnt; i++)
		phy_array[i+1] = i;
}

static enum hfi_panel_backlight_ctrl dsi_get_panel_backlight_type(struct dsi_panel *panel,
									char *panel_type)
{
	char *bl_name = NULL;
	const char *bl_type = NULL;
	struct dsi_parser_utils *utils = &panel->utils;

	if (!strcmp(panel_type, "primary"))
		bl_name = "qcom,mdss-dsi-bl-pmic-control-type";
	else
		bl_name = "qcom,mdss-dsi-sec-bl-pmic-control-type";

	bl_type = utils->get_property(utils->data, bl_name, NULL);
	if (!bl_type)
		return HFI_PANEL_BACKLIGHT_CTRL_UNKNOWN;
	else if (!strcmp(bl_type, "bl_ctrl_pwm"))
		return HFI_PANEL_BACKLIGHT_CTRL_PWM;
	else if (!strcmp(bl_type, "bl_ctrl_wled"))
		return HFI_PANEL_BACKLIGHT_CTRL_WLED;
	else if (!strcmp(bl_type, "bl_ctrl_dcs"))
		return HFI_PANEL_BACKLIGHT_CTRL_DCS;
	else if (!strcmp(bl_type, "bl_ctrl_external"))
		return HFI_PANEL_BACKLIGHT_CTRL_EXTERNAL;
	else
		return HFI_PANEL_BACKLIGHT_CTRL_UNKNOWN;
}

int hfi_panel_fill_dcs_cmds_sub(struct dsi_display *display,
				struct dsi_panel_timing_caps *panel_timing_caps,
				struct dsi_hfi_panel_per_cmd_type *per_type,
				struct dsi_panel_cmd_set *cmd_set,
				u32 *size_of_cmds_by_type, void *sde_vaddr, void *hfi_vaddr)
{
	struct dsi_hfi_panel_cmd_info *panel_cmd_info = hfi_vaddr;
	int i, rc = 0;
	u32 size_of_indv_cmd;

	for (i = 0; i < per_type->count_cmds; i++) {
		size_of_indv_cmd = 0;

		panel_cmd_info->delay = cmd_set->cmds[i].post_wait_ms;
		panel_cmd_info->ctrl_flags = cmd_set->cmds[i].ctrl_flags;
		panel_cmd_info->mode = cmd_set->state;

		rc = dsi_hfi_packetize_panel_cmd(&cmd_set->cmds[i], &size_of_indv_cmd, sde_vaddr);
		if (rc) {
			DSI_ERR("failed to packetize command %d, rc=%d\n", i, rc);
			goto error;
		}

		panel_cmd_info->size = size_of_indv_cmd;

		size_of_indv_cmd = (((size_of_indv_cmd+7)>>3)<<3);
		sde_vaddr += size_of_indv_cmd;

		panel_cmd_info->cmd_offset = *size_of_cmds_by_type + display->cmd_buffer_iova;

		*size_of_cmds_by_type += size_of_indv_cmd;

		panel_cmd_info++;
		panel_timing_caps->running_hfi_offset += sizeof(struct dsi_hfi_panel_cmd_info);
	}

error:
	return rc;
}

int hfi_panel_fill_dcs_cmds(struct dsi_display *display,
			    struct dsi_display_mode_priv_info *priv_info,
			    struct dsi_panel_timing_caps *panel_timing_caps,
			    void *sde_vaddr, void *hfi_vaddr)
{
	int i;
	int j = 0;
	int rc = 0;
	u32 offset = 0;

	for (i = 0; i < DSI_CMD_SET_MAX; i++) {
		if (j == NUM_PANEL_CMD_TYPES_SUPPORTED)
			break;

		if (i == DSI_CMD_SET_PPS || i == DSI_CMD_SET_ROI)
			continue;

		if (!priv_info->cmd_sets[i].count)
			continue;

		panel_timing_caps->payload.hfi_per_type_array[j].hfi_buff_struct_offset =
							panel_timing_caps->running_hfi_offset;
		panel_timing_caps->payload.hfi_per_type_array[j].sde_buff_type_offset =
							offset;
		panel_timing_caps->payload.hfi_per_type_array[j].cmd_type = i;
		panel_timing_caps->payload.hfi_per_type_array[j].count_cmds =
							priv_info->cmd_sets[i].count;

		rc = hfi_panel_fill_dcs_cmds_sub(display, panel_timing_caps,
				&panel_timing_caps->payload.hfi_per_type_array[j],
				&priv_info->cmd_sets[i], &offset, sde_vaddr, hfi_vaddr);
		if (rc)
			DSI_ERR("Failed to fill panel cmds into memory for cmd type %d", i);

		sde_vaddr += offset;
		hfi_vaddr += (sizeof(struct dsi_hfi_panel_cmd_info) * priv_info->cmd_sets[i].count);
		j++;
	}

	panel_timing_caps->payload.count = j;

	return 0;
}

static void dsi_hfi_populate_panel_generic_caps(struct dsi_display *display,
					struct dsi_panel *panel,
					struct dsi_panel_generic_caps *panel_generic_caps)
{
	panel_generic_caps->valid_gen_caps_cnt = MIN_NUM_OF_GEN_CAPS;
	panel_generic_caps->panel_type = dsi_get_panel_type_helper(panel);
	panel_generic_caps->color_order_type = dsi_get_panel_color_order_type(panel);
	panel_generic_caps->dma_trigger_type =
		dsi_get_panel_trigger_type_helper(panel->host_config.dma_cmd_trigger);
	panel_generic_caps->mdp_trigger_type =
		dsi_get_panel_trigger_type_helper(panel->host_config.mdp_cmd_trigger);
	panel_generic_caps->te_mode = panel->host_config.te_mode;
	panel_generic_caps->traffic_mode = dsi_get_panel_traffic_mode_helper(panel);
	panel_generic_caps->virtual_channel_id = panel->video_config.vc_id;
	panel_generic_caps->wr_mem_start = panel->cmd_config.wr_mem_start;
	panel_generic_caps->wr_mem_continue = panel->cmd_config.wr_mem_continue;
	panel_generic_caps->te_dcs_command = panel->cmd_config.insert_dcs_command;
	panel_generic_caps->panel_op_mode = dsi_get_panel_op_mode_helper(panel);
	panel_generic_caps->min_backlight_level = panel->bl_config.bl_min_level;
	panel_generic_caps->max_backlight_level = panel->bl_config.bl_max_level;
	panel_generic_caps->max_brightness_level = panel->hdr_props.peak_brightness;
	panel_generic_caps->vsync_src = dsi_get_panel_vsync_src(display);

	panel_generic_caps->panel_name = (*(u32 *)panel->name);
	if (panel_generic_caps->panel_name)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->panel_bpp = dsi_get_panel_bpp_helper(panel);
	if (panel_generic_caps->panel_bpp)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->panels_lanes_state = dsi_get_panel_lane_state_helper(panel);
	if (panel_generic_caps->panels_lanes_state)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->panel_lane_map = dsi_get_panel_lane_map_helper(panel);
	if (panel_generic_caps->panel_lane_map)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->tx_eot_append = (u32)(panel->host_config.append_tx_eot);
	if (panel_generic_caps->tx_eot_append)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->eof_power_mode = panel->video_config.eof_bllp_lp11_en;
	if (panel_generic_caps->eof_power_mode)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->bllp_power_mode = panel->video_config.bllp_lp11_en;
	if (panel_generic_caps->bllp_power_mode)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->backlight_ctrl_prim = dsi_get_panel_backlight_type(panel, "primary");
	if (panel_generic_caps->backlight_ctrl_prim)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->backlight_ctrl_sec = dsi_get_panel_backlight_type(panel, "secondary");
	if (panel_generic_caps->backlight_ctrl_sec)
		panel_generic_caps->valid_gen_caps_cnt++;

	panel_generic_caps->is_bl_inverted = panel->bl_config.bl_inverted_dbv;
	if (panel_generic_caps->is_bl_inverted)
		panel_generic_caps->valid_gen_caps_cnt++;

	dsi_get_panel_ctrl_nums_helper(display, panel_generic_caps->ctrl_nums);
	if (panel_generic_caps->ctrl_nums[0])
		panel_generic_caps->valid_gen_caps_cnt++;

	dsi_get_panel_phy_nums_helper(display, panel_generic_caps->phy_nums);
	if (panel_generic_caps->phy_nums[0])
		panel_generic_caps->valid_gen_caps_cnt++;

}

static void dsi_hfi_populate_panel_timing_caps(struct dsi_display *display,
					struct dsi_display_mode *mode,
					struct dsi_panel_timing_caps *panel_timing_caps,
					void *sde_vaddr, void *hfi_vaddr)
{
	int i;

	panel_timing_caps->panel_index = mode->mode_idx;
	panel_timing_caps->clockrate[0] = HFI_VAL_L32(mode->timing.clk_rate_hz);
	panel_timing_caps->clockrate[1] = HFI_VAL_H32(mode->timing.clk_rate_hz);
	panel_timing_caps->framerate = (mode->timing.refresh_rate);
	panel_timing_caps->panel_jitter[0] = mode->priv_info->panel_jitter_numer;
	panel_timing_caps->panel_jitter[1] = mode->priv_info->panel_jitter_denom;
	panel_timing_caps->hsync_pulse = mode->timing.h_sync_width;
	hfi_panel_get_mode_res_data(mode, panel_timing_caps);
	if (mode->timing.dsc_enabled)
		hfi_panel_get_mode_compression_params(mode, panel_timing_caps);
	else
		panel_timing_caps->compression_params.mode = HFI_PANEL_COMPRESSION_NONE;
	panel_timing_caps->topology.count = 1;
	panel_timing_caps->topology.hfi_topology.mixer_count = mode->priv_info->topology.num_lm;
	panel_timing_caps->topology.hfi_topology.enc_count = mode->priv_info->topology.num_enc;
	panel_timing_caps->topology.hfi_topology.display_count = mode->priv_info->topology.num_intf;
	panel_timing_caps->top_index = 0;
	hfi_panel_fill_dcs_cmds(display, mode->priv_info, panel_timing_caps, sde_vaddr, hfi_vaddr);
	panel_timing_caps->phy_timings_payload.count = mode->priv_info->phy_timing_len;
	for (i = 0; i < NUM_VARIABLE_DPHY_TIMINGS; i++)
		panel_timing_caps->phy_timings_payload.dphy_timings[i] =
			mode->priv_info->phy_timing_val[i];
}

static int dsi_hfi_append_panel_init_caps(struct hfi_cmdbuf_t *buffer,
					struct dsi_display *display,
					struct dsi_panel_init_caps panel_init_caps,
					struct hfi_shared_addr_map *addr_map)
{
	int rc = 0;
	u32 object_id = 0x0;
	struct dsi_display_hfi *display_hfi;
	u32 kv_count;
	u32 reserved_key = 0;
	u32 kv_size = 0;
	u32 payload_size = 0;
	u32 sde_addr[3];
	u32 hfi_addr[3];
	u64 rem_prop_val = (u64) addr_map->remote_addr;

	if (!display)
		return -EINVAL;

	sde_addr[0] = reserved_key;
	sde_addr[1] = HFI_VAL_L32(display->cmd_buffer_iova);
	sde_addr[2] = HFI_VAL_H32(display->cmd_buffer_iova);

	hfi_addr[0] = reserved_key;
	hfi_addr[1] = HFI_VAL_L32(rem_prop_val);
	hfi_addr[2] = HFI_VAL_H32(rem_prop_val);

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	hfi_util_kv_helper_reset(display_hfi->kv_props);

	hfi_util_kv_helper_add(display_hfi->kv_props,
			HFI_PACKKEY(HFI_PROPERTY_PANEL_TIMING_MODE_COUNT, 0,
			(sizeof(panel_init_caps.num_timing_modes) / sizeof(u32))),
			(void *)&panel_init_caps.num_timing_modes);
	kv_size += sizeof(panel_init_caps.num_timing_modes);

	hfi_util_kv_helper_add(display_hfi->kv_props,
			HFI_PACKKEY(HFI_PROPERTY_PANEL_DPU_ADDRESS, 0,
			((ARRAY_SIZE(sde_addr) * sizeof(sde_addr[0])) / sizeof(u32))),
			(void *)sde_addr);
	kv_size += sizeof(sde_addr);

	hfi_util_kv_helper_add(display_hfi->kv_props,
			HFI_PACKKEY(HFI_PROPERTY_PANEL_DCP_ADDRESS, 0,
			((ARRAY_SIZE(hfi_addr) * sizeof(hfi_addr[0])) / sizeof(u32))),
			(void *)hfi_addr);
	kv_size += sizeof(hfi_addr);

	kv_count = hfi_util_kv_helper_get_count(display_hfi->kv_props);

	payload_size = (kv_count * sizeof(u32)) + kv_size;

	rc = hfi_adapter_add_prop_array(buffer,
				HFI_COMMAND_PANEL_INIT_PANEL_CAPS,
				object_id,
				HFI_PAYLOAD_TYPE_U32,
				hfi_util_kv_helper_get_payload_addr(display_hfi->kv_props),
				kv_count,
				payload_size);
	if (rc)
		DSI_ERR("Failed to add caps to buffer, rc = %d", rc);

	return rc;
}

static int dsi_hfi_append_panel_generic_caps(struct hfi_cmdbuf_t *buffer,
				struct dsi_display *display,
				struct dsi_panel_generic_caps panel_generic_caps)
{
	int rc = 0;
	int i = 0;
	u32 kv_count;
	u32 kv_size = 0;
	u32 payload_size = 0;
	u32 object_id = 0x0;
	int num_caps = panel_generic_caps.valid_gen_caps_cnt;
	struct dsi_display_hfi *display_hfi;

	if (!display)
		return -EINVAL;

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	hfi_util_kv_helper_reset(display_hfi->kv_props);

	struct dsi_value_to_prop_lookup dsi_hfi_gen_props_map[] = {
		{panel_generic_caps.panel_type, HFI_PROPERTY_PANEL_PHYSICAL_TYPE},
		{panel_generic_caps.color_order_type, HFI_PROPERTY_PANEL_COLOR_ORDER},
		{panel_generic_caps.dma_trigger_type, HFI_PROPERTY_PANEL_DMA_TRIGGER},
		{panel_generic_caps.mdp_trigger_type, HFI_PROPERTY_PANEL_STREAM_TRIGGER},
		{panel_generic_caps.te_mode, HFI_PROPERTY_PANEL_TE_MODE},
		{panel_generic_caps.traffic_mode, HFI_PROPERTY_PANEL_TRAFFIC_MODE},
		{panel_generic_caps.virtual_channel_id, HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID},
		{panel_generic_caps.wr_mem_start, HFI_PROPERTY_PANEL_WR_MEM_START},
		{panel_generic_caps.wr_mem_continue, HFI_PROPERTY_PANEL_WR_MEM_CONTINUE},
		{panel_generic_caps.te_dcs_command, HFI_PROPERTY_PANEL_TE_DCS_COMMAND},
		{panel_generic_caps.panel_op_mode, HFI_PROPERTY_PANEL_OPERATING_MODE},
		{panel_generic_caps.min_backlight_level, HFI_PROPERTY_PANEL_BL_MIN_LEVEL},
		{panel_generic_caps.max_backlight_level, HFI_PROPERTY_PANEL_BL_MAX_LEVEL},
		{panel_generic_caps.vsync_src, HFI_PROPERTY_PANEL_VSYNC_SOURCE},
		{panel_generic_caps.max_brightness_level, HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL},
		/*Cutoff for properties that take on default value*/
		{panel_generic_caps.panel_name, HFI_PROPERTY_PANEL_NAME},
		{panel_generic_caps.panel_bpp, HFI_PROPERTY_PANEL_BPP},
		{panel_generic_caps.panels_lanes_state, HFI_PROPERTY_PANEL_LANES_STATE},
		{panel_generic_caps.panel_lane_map, HFI_PROPERTY_PANEL_LANE_MAP},
		{panel_generic_caps.tx_eot_append, HFI_PROPERTY_PANEL_TX_EOT_APPEND},
		{panel_generic_caps.eof_power_mode, HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE},
		{panel_generic_caps.bllp_power_mode, HFI_PROPERTY_PANEL_BLLP_POWER_MODE},
		{panel_generic_caps.backlight_ctrl_prim, HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE},
		{panel_generic_caps.backlight_ctrl_sec,
						HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE},
		{panel_generic_caps.is_bl_inverted, HFI_PROPERTY_PANEL_BL_INVERTED_DBV},
	};

	/* Populate properties that will take on a default value, even if not present */
	for (i = 0; i < MIN_NUM_OF_GEN_CAPS; i++) {
		hfi_util_kv_helper_add(display_hfi->kv_props,
					HFI_PACKKEY(dsi_hfi_gen_props_map[i].hfi_prop, 0,
					(sizeof(dsi_hfi_gen_props_map[i].value) / sizeof(u32))),
					(void *)&dsi_hfi_gen_props_map[i].value);
		kv_size += sizeof(dsi_hfi_gen_props_map[i].value);
	}

	/* Populate properties that need to be checked for presence */
	for (i = MIN_NUM_OF_GEN_CAPS; i < (num_caps-2); i++) {
		if (dsi_hfi_gen_props_map[i].value) {
			hfi_util_kv_helper_add(display_hfi->kv_props,
					HFI_PACKKEY(dsi_hfi_gen_props_map[i].hfi_prop, 0,
					(sizeof(dsi_hfi_gen_props_map[i].value) / sizeof(u32))),
					(void *)&dsi_hfi_gen_props_map[i].value);
			kv_size += sizeof(dsi_hfi_gen_props_map[i].value);
		}
	}

	/* Special case */
	if (panel_generic_caps.ctrl_nums[0]) {
		hfi_util_kv_helper_add(display_hfi->kv_props,
					HFI_PACKKEY(HFI_PROPERTY_PANEL_CTRL_NUM, 0,
					(sizeof(panel_generic_caps.ctrl_nums) / sizeof(u32))),
					(void *)&panel_generic_caps.ctrl_nums);
		kv_size += sizeof(panel_generic_caps.ctrl_nums);
	}

	if (panel_generic_caps.phy_nums[0]) {
		hfi_util_kv_helper_add(display_hfi->kv_props,
					HFI_PACKKEY(HFI_PROPERTY_PANEL_PHY_NUM, 0,
					(sizeof(panel_generic_caps.phy_nums) / sizeof(u32))),
					(void *)&panel_generic_caps.phy_nums);
		kv_size += sizeof(panel_generic_caps.phy_nums);
	}

	kv_count = hfi_util_kv_helper_get_count(display_hfi->kv_props);

	payload_size = (kv_count * sizeof(u32)) + kv_size;

	rc = hfi_adapter_add_prop_array(buffer,
				HFI_COMMAND_PANEL_INIT_GENERIC_CAPS,
				object_id,
				HFI_PAYLOAD_TYPE_U32_ARRAY,
				hfi_util_kv_helper_get_payload_addr(display_hfi->kv_props),
				kv_count,
				payload_size);

	if (rc)
		DSI_ERR("Failed to add caps to buffer, rc = %d", rc);

	return rc;
}

static int dsi_hfi_append_panel_timing_caps(struct hfi_cmdbuf_t *buffer,
						struct dsi_display *display,
						struct dsi_panel_timing_caps *timing_caps_array,
						void *sde_vaddr)
{
	int rc = 0;
	int i;
	u32 object_id = 0x0;
	struct dsi_display_hfi *display_hfi;

	if (!display)
		return -EINVAL;

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	for (i = 0; i < display->panel->num_timing_nodes; i++) {
		u32 kv_count;
		u32 kv_size = 0;
		u32 payload_size = 0;

		hfi_util_kv_helper_reset(display_hfi->kv_props);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_INDEX, 0,
				(sizeof(timing_caps_array[i].panel_index) / sizeof(u32))),
				(void *)&timing_caps_array[i].panel_index);
		kv_size += sizeof(timing_caps_array[i].panel_index);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_CLOCKRATE, 0,
				(sizeof(timing_caps_array[i].clockrate) / sizeof(u32))),
				(void *)&timing_caps_array[i].clockrate);
		kv_size += sizeof(timing_caps_array[i].clockrate);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_FRAMERATE, 0,
				(sizeof(timing_caps_array[i].framerate) / sizeof(u32))),
				(void *)&timing_caps_array[i].framerate);
		kv_size += sizeof(timing_caps_array[i].framerate);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_JITTER, 0,
				(sizeof(timing_caps_array[i].panel_jitter) / sizeof(u32))),
				(void *)&timing_caps_array[i].panel_jitter);
		kv_size += sizeof(timing_caps_array[i].panel_jitter);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_RESOLUTION_DATA, 0,
				(sizeof(timing_caps_array[i].res_data) / sizeof(u32))),
				(void *)&timing_caps_array[i].res_data);
		kv_size += sizeof(timing_caps_array[i].res_data);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_COMPRESSION_DATA, 0,
				(sizeof(timing_caps_array[i].compression_params) / sizeof(u32))),
				(void *)&timing_caps_array[i].compression_params);
		kv_size += sizeof(timing_caps_array[i].compression_params);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY, 0,
				(sizeof(timing_caps_array[i].topology) / sizeof(u32))),
				(void *)&timing_caps_array[i].topology);
		kv_size += sizeof(timing_caps_array[i].topology);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX, 0,
				(sizeof(timing_caps_array[i].top_index) / sizeof(u32))),
				(void *)&timing_caps_array[i].top_index);
		kv_size += sizeof(timing_caps_array[i].top_index);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_DCS_CMD_INFO, 0,
				(sizeof(timing_caps_array[i].payload) / sizeof(u32))),
				(void *)&timing_caps_array[i].payload);
		kv_size += sizeof(timing_caps_array[i].payload);

		hfi_util_kv_helper_add(display_hfi->kv_props,
				HFI_PACKKEY(HFI_PROPERTY_PANEL_DPHY_TIMINGS, 0,
				(sizeof(timing_caps_array[i].phy_timings_payload) / sizeof(u32))),
				(void *)&timing_caps_array[i].phy_timings_payload);
		kv_size += sizeof(timing_caps_array[i].phy_timings_payload);

		kv_count = hfi_util_kv_helper_get_count(display_hfi->kv_props);

		payload_size = (kv_count * sizeof(u32)) + kv_size;

		rc = hfi_adapter_add_prop_array(buffer,
				HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS,
				object_id,
				HFI_PAYLOAD_TYPE_U32_ARRAY,
				hfi_util_kv_helper_get_payload_addr(display_hfi->kv_props),
				kv_count,
				payload_size);

		if (rc)
			DSI_ERR("Failed to add caps for timing node:%d, rc = %d",
					i, rc);
	}
	return rc;
}

int dsi_hfi_panel_init(struct dsi_display *display, struct dsi_panel *panel)
{
	struct dsi_panel_init_caps panel_init_caps;
	struct dsi_panel_generic_caps panel_generic_caps;
	struct dsi_panel_timing_caps *timing_caps_array;
	int i;
	int rc = 0;
	u32 obj_id;
	struct hfi_shared_addr_map *addr_map;
	struct dsi_display_hfi *display_hfi;

	if (!display)
		return -EINVAL;

	obj_id = strcmp(display->display_type, "primary");

	display_hfi = display->dsi_hfi_info;
	if (!display_hfi)
		return -EINVAL;

	struct hfi_cmdbuf_t *buffer = hfi_adapter_get_cmd_buf(display_hfi->hfi_client,
							obj_id,
							HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING);

	panel_init_caps.num_timing_modes = panel->num_timing_nodes;
	if (!panel_init_caps.num_timing_modes) {
		DSI_ERR("No timing modes - panel init failed");
		goto error_buff;
	}

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_hfi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			DSI_ERR("failed to allocate sde mapped buffer\n");
			goto error_buff;
		}
	}

	addr_map = kvzalloc(sizeof(struct hfi_shared_addr_map), GFP_KERNEL);
	if (!addr_map) {
		DSI_ERR("failed to allocate addr_map");
		goto error_buff;
	}

	addr_map->size = SZ_4K;

	hfi_adapter_buffer_alloc(addr_map);
	if (!addr_map->remote_addr || !addr_map->local_addr)
		goto error_addr_map;

	timing_caps_array = kcalloc(panel_init_caps.num_timing_modes,
					sizeof(struct dsi_panel_timing_caps),
					GFP_KERNEL);
	if (!timing_caps_array)
		goto error_array;

	dsi_hfi_populate_panel_generic_caps(display, panel, &panel_generic_caps);

	for (i = 0; i < panel_init_caps.num_timing_modes; i++)
		dsi_hfi_populate_panel_timing_caps(display,
								&display->modes[i],
								&timing_caps_array[i],
								display->vaddr,
								addr_map->local_addr);

	display_hfi->kv_props = hfi_util_kv_helper_alloc(HFI_UTIL_MAX_ALLOC);

	SDE_EVT32(HFI_COMMAND_PANEL_INIT_PANEL_CAPS, SDE_EVTLOG_FUNC_CASE1);
	rc = dsi_hfi_append_panel_init_caps(buffer, display, panel_init_caps, addr_map);
	if (rc) {
		DSI_ERR("failed to append HFI_COMMAND_PANEL_INIT_PANEL_CAPS: rc = %d", rc);
		goto error_array;
	}

	SDE_EVT32(HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS, SDE_EVTLOG_FUNC_CASE2);
	rc = dsi_hfi_append_panel_timing_caps(buffer, display, timing_caps_array, display->vaddr);
	if (rc) {
		DSI_ERR("failed to append HFI_COMMAND_PANEL_INIT_TIMING_CAPS: rc = %d", rc);
		goto error_array;
	}

	SDE_EVT32(HFI_COMMAND_PANEL_INIT_GENERIC_CAPS, SDE_EVTLOG_FUNC_CASE3);
	rc = dsi_hfi_append_panel_generic_caps(buffer, display, panel_generic_caps);
	if (rc) {
		DSI_ERR("failed to append HFI_COMMAND_PANEL_INIT_GENERIC_CAPS: rc = %d", rc);
		goto error_array;
	}

	rc = hfi_adapter_set_cmd_buf(buffer);
	SDE_EVT32(HFI_COMMAND_PANEL_INIT_PANEL_CAPS, HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS,
			HFI_COMMAND_PANEL_INIT_GENERIC_CAPS, rc, SDE_EVTLOG_FUNC_CASE4);
	if (rc) {
		DSI_ERR("failed to send panel init: rc = %d", rc);
		goto error_array;
	}

	return rc;

error_array:
	kfree(timing_caps_array);
error_addr_map:
	kfree(addr_map);
error_buff:
	rc = hfi_adapter_release_cmd_buf(buffer);
	if (rc)
		DSI_ERR("failed to release command buffer\n");

	return rc;
}
