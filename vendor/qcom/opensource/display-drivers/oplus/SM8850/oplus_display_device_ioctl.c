/***************************************************************
** Copyright (C), 2024, OPLUS Mobile Comm Corp., Ltd
**
** File : oplus_display_device_ioctl.c
** Description : oplus display panel device ioctl
** Version : 1.0
** Date : 2024/05/09
** Author : Display
******************************************************************/
#include "oplus_display_device_ioctl.h"
#include "oplus_display_device.h"
#include "oplus_display_effect.h"
#include <linux/notifier.h>
#include <linux/soc/qcom/panel_event_notifier.h>
#include "oplus_display_sysfs_attrs.h"
#include "oplus_display_interface.h"
#include "oplus_display_ext.h"
#include "oplus_display_bl.h"
#include "sde_trace.h"
#include "sde_dbg.h"
#include "oplus_debug.h"

#if defined(CONFIG_PXLW_IRIS)
#include "pw_iris_loop_back.h"
#endif

#define DSI_PANEL_OPLUS_DUMMY_VENDOR_NAME  "PanelVendorDummy"
#define DSI_PANEL_OPLUS_DUMMY_MANUFACTURE_NAME  "dummy1024"

int oplus_debug_max_brightness = 0;
int oplus_dither_enable = 0;
int oplus_dre_status = 0;
int oplus_cabc_status = OPLUS_DISPLAY_CABC_UI;
extern int lcd_closebl_flag;
extern int oplus_display_audio_ready;
char oplus_rx_reg[PANEL_TX_MAX_BUF] = {0x0};
char oplus_rx_len = 0;
extern int spr_mode;
extern int dynamic_osc_clock;
extern int oplus_hw_partial_round;
int mca_mode = 1;
int dcc_flags = 0;

extern int dither_enable;
extern int seed_mode;
char brightness_time[32];
EXPORT_SYMBOL(brightness_time);
EXPORT_SYMBOL(oplus_debug_max_brightness);
EXPORT_SYMBOL(oplus_dither_enable);
EXPORT_SYMBOL(dcc_flags);

extern int dsi_display_read_panel_reg(struct dsi_display *display, u8 cmd,
		void *data, size_t len);
extern int __oplus_display_set_spr(int mode);
extern int dsi_display_spr_mode(struct dsi_display *display, int mode);
extern int dsi_panel_spr_mode(struct dsi_panel *panel, int mode);
extern int __oplus_display_set_dither(int mode);
extern unsigned int is_project(int project);

enum {
	REG_WRITE = 0,
	REG_READ,
	REG_X,
};

struct LCM_setting_table {
	unsigned int count;
	u8 *para_list;
};

int oplus_display_panel_get_id(void *buf)
{
	struct panel_id *panel_id = buf;
	int display_id = panel_id->DA;
	struct dsi_display *display = get_main_display();

	if (display_id == 1)
		display = get_sec_display();

	if (!display) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	panel_id->DA = display->oplus_display.panel_id1;
	panel_id->DB = display->oplus_display.panel_id2;
	panel_id->DC = display->oplus_display.panel_id3;

	return 0;
}

int oplus_display_panel_get_oplus_max_brightness(void *buf)
{
	uint32_t *max_brightness = buf;
	int panel_id = (*max_brightness >> 12);
	struct dsi_display *display = get_main_display();
	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	(*max_brightness) = display->panel->oplus_panel.bl_cfg.bl_normal_max_level;

	return 0;
}

int oplus_display_panel_get_max_brightness(void *buf)
{
	uint32_t *max_brightness = buf;
	int panel_id = (*max_brightness >> 12);
	struct dsi_display *display = get_main_display();
	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	if (oplus_debug_max_brightness == 0) {
		(*max_brightness) = display->panel->oplus_panel.bl_cfg.bl_normal_max_level;
	} else {
		(*max_brightness) = oplus_debug_max_brightness;
	}

	return 0;
}

int oplus_display_panel_set_max_brightness(void *buf)
{
	uint32_t *max_brightness = buf;

	oplus_debug_max_brightness = (*max_brightness);

	return 0;
}

int oplus_display_panel_get_lcd_max_brightness(void *buf)
{
	uint32_t *lcd_max_backlight = buf;
	int panel_id = (*lcd_max_backlight >> 12);
	struct dsi_display *display = get_main_display();
	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	(*lcd_max_backlight) = display->panel->bl_config.bl_max_level;

	OPLUS_DSI_INFO("[%s] get lcd max backlight: %d\n",
			display->panel->oplus_panel.vendor_name,
			*lcd_max_backlight);

	return 0;
}

int oplus_display_panel_get_brightness(void *buf)
{
	uint32_t *brightness = buf;
	int panel_id = (*brightness >> 12);
	struct dsi_display *display = get_main_display();
	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	(*brightness) = display->panel->bl_config.bl_level;

	return 0;
}

int oplus_display_panel_set_brightness(void *buf)
{
	int rc = 0;
	struct dsi_display *display = oplus_display_get_current_display();
	struct dsi_panel *panel = NULL;
	uint32_t *backlight = buf;

	if (!display || !display->drm_conn || !display->panel) {
		OPLUS_DSI_ERR("Invalid display params\n");
		return -EINVAL;
	}
	panel = display->panel;

	if (*backlight > panel->bl_config.bl_max_level ||
			*backlight < 0) {
		OPLUS_DSI_WARN("falied to set backlight: %d, it is out of range\n",
				*backlight);
		return -EFAULT;
	}

	OPLUS_DSI_INFO("[%s] set backlight: %d\n", panel->oplus_panel.vendor_name, *backlight);

	rc = dsi_display_set_backlight(display->drm_conn, display, *backlight);

	return rc;
}

int oplus_display_panel_get_vendor(void *buf)
{
	struct panel_info *p_info = buf;
	struct dsi_display *display = NULL;
	char *vendor = NULL;
	char *manu_name = NULL;
	int panel_id = p_info->version[0];

	display = get_main_display();
	if (1 == panel_id)
		display = get_sec_display();

	if (!display || !display->panel ||
			!display->panel->oplus_panel.vendor_name ||
			!display->panel->oplus_panel.manufacture_name) {
		OPLUS_DSI_ERR("failed to config lcd proc device\n");
		return -EINVAL;
	}

	vendor = (char *)display->panel->oplus_panel.vendor_name;
	manu_name = (char *)display->panel->oplus_panel.manufacture_name;

	strncpy(p_info->version, vendor, sizeof(p_info->version));
	strncpy(p_info->manufacture, manu_name, sizeof(p_info->manufacture));

	return 0;
}

int oplus_display_get_brightness_time(void *data) {
	struct panel_brightness_time *p_time = data;

	strncpy(p_time->brightness_time, brightness_time, sizeof(p_time->brightness_time));

	OPLUS_DSI_INFO("get brightness time: brightness_time[%s] l\n", p_time->brightness_time);
	return 0;
}

int oplus_display_panel_get_panel_name(void *buf)
{
	struct panel_name *p_name = buf;
	struct dsi_display *display = NULL;
	int panel_id = p_name->name[0];

	display = get_main_display();
	if (1 == panel_id) {
		display = get_sec_display();
	}

	if (!display || !display->panel || !display->panel->name) {
		OPLUS_DSI_ERR("failed to config lcd panel name\n");
		return -EINVAL;
	}

	strncpy(p_name->name, (char *)display->panel->name, sizeof(p_name->name));
	p_name->name[sizeof(p_name->name) - 1] = '\0';

	return 0;
}

int oplus_display_panel_get_panel_bpp(void *buf)
{
	uint32_t *panel_bpp = buf;
	int bpp = 0;
	int rc = 0;
	int panel_id = (*panel_bpp >> 12);
	struct dsi_display *display = get_main_display();
	struct dsi_parser_utils *utils = NULL;

	if (panel_id == 1) {
		display = get_sec_display();
	}

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display or panel is null\n");
		return -EINVAL;
	}

	utils = &display->panel->utils;
	if (!utils) {
		OPLUS_DSI_ERR("utils is null\n");
		return -EINVAL;
	}

	rc = utils->read_u32(utils->data, "qcom,mdss-dsi-bpp", &bpp);

	if (rc) {
		OPLUS_DSI_INFO("failed to read qcom,mdss-dsi-bpp, rc=%d\n", rc);
		return -EINVAL;
	}

	*panel_bpp = bpp / 3;

	return 0;
}

int oplus_display_panel_get_ccd_check(void *buf)
{
	struct dsi_display *display = get_main_display();
	struct mipi_dsi_device *mipi_device;
	int rc = 0;
	unsigned int *ccd_check = buf;
	char value3[] = { 0x5A, 0x5A };
	char value4[] = { 0x02 };
	char value5[] = { 0x44, 0x50 };
	char value6[] = { 0x05 };
	char value7[] = { 0xA5, 0xA5 };
	unsigned char read1[10];

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel in off status\n");
		return -EFAULT;
	}

	if (display->panel->panel_mode != DSI_OP_CMD_MODE) {
		OPLUS_DSI_ERR("only supported for command mode\n");
		return -EFAULT;
	}

	mipi_device = &display->panel->mipi_device;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);

	if (rc) {
		OPLUS_DSI_ERR("cmd engine enable failed\n");
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value3, sizeof(value3));
	rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value4, sizeof(value4));
	rc = mipi_dsi_dcs_write(mipi_device, 0xCC, value5, sizeof(value5));
	usleep_range(1000, 1100);
	rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value6, sizeof(value6));

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);

	mutex_unlock(&display->panel->panel_lock);
	rc = dsi_display_read_panel_reg(display, 0xCC, read1, 1);
	OPLUS_DSI_ERR("read ccd_check value = 0x%x rc=%d\n", read1[0], rc);
	(*ccd_check) = read1[0];
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);

	if (rc) {
		OPLUS_DSI_ERR("cmd engine enable failed\n");
		goto unlock;
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value7, sizeof(value7));

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);
unlock:

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
	OPLUS_DSI_ERR("[%s] ccd_check = %d\n",  display->panel->oplus_panel.vendor_name,
			(*ccd_check));
	return 0;
}

int oplus_display_panel_get_serial_number(void *buf)
{
	int ret = 0;
	struct panel_serial_number *panel_rnum = buf;
	struct dsi_display *display = get_main_display();
	int panel_id = panel_rnum->serial_number[0];

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	if (1 == panel_id) {
		display = get_sec_display();
		if (!display || !display->panel) {
			OPLUS_DSI_ERR("display is null\n");
			return -EINVAL;
		}
	}

	if (!display->panel->oplus_panel.serial_number.serial_number_support) {
		OPLUS_DSI_INFO("display panel serial number not support\n");
		return ret;
	}

	ret = scnprintf(panel_rnum->serial_number, sizeof(panel_rnum->serial_number),
			"Get panel serial number: %016lX", display->oplus_display.panel_sn);
	OPLUS_DSI_INFO("serial_number = [%s]", panel_rnum->serial_number);

	return ret;
}

#ifdef OPLUS_FEATURE_APDMR
extern void sde_dump_evtlog_and_reg(void);
#endif

extern unsigned int oplus_display_log_type;
int oplus_display_panel_set_qcom_loglevel(void *data)
{
	struct kernel_loglevel *k_loginfo = data;
	struct dsi_display *display = oplus_display_get_current_display();

	if (k_loginfo == NULL) {
		OPLUS_DSI_ERR("k_loginfo is null pointer\n");
		return -EINVAL;
	}

	if (k_loginfo->enable) {
		oplus_display_log_type |= OPLUS_DEBUG_LOG_DSI;
		oplus_display_trace_enable |= OPLUS_DISPLAY_TRACE_ALL;

		OPLUS_DSI_INFO("on screenshot to dump evtlog and register\n");
		sde_dump_evtlog_and_reg();

		mutex_lock(&display->display_lock);
		mutex_lock(&display->panel->panel_lock);
		oplus_panel_backlight_check(display->panel);
		mutex_unlock(&display->panel->panel_lock);
		mutex_unlock(&display->display_lock);
	} else {
		oplus_display_log_type &= ~OPLUS_DEBUG_LOG_DSI;
		oplus_display_trace_enable &= ~OPLUS_DISPLAY_TRACE_ALL;
	}

	OPLUS_DSI_INFO("Set qcom kernel log, enable:0x%X, level:0x%X, current:0x%X\n",
			k_loginfo->enable,
			k_loginfo->log_level,
			oplus_display_log_type);
	return 0;
}


int oplus_display_panel_print_xlog(void *data)
{
	OPLUS_DSI_INFO("hwc_trackpoint to trigger dump evtlog\n");
	oplus_sde_evtlog_dump_all();
	return 0;
}

int oplus_big_endian_copy(void *dest, void *src, int count)
{
	int index = 0, knum = 0, rc = 0;
	uint32_t *u_dest = (uint32_t*) dest;
	char *u_src = (char*) src;

	if (dest == NULL || src == NULL) {
		OPLUS_DSI_ERR("null pointer\n");
		return -EINVAL;
	}

	if (dest == src) {
		return rc;
	}

	while (count > 0) {
		u_dest[index] = ((u_src[knum] << 24) | (u_src[knum+1] << 16) | (u_src[knum+2] << 8) | u_src[knum+3]);
		index += 1;
		knum += 4;
		count = count - 1;
	}

	return rc;
}

int oplus_display_panel_get_softiris_color_status(void *data)
{
	struct softiris_color *iris_color_status = data;
	bool color_vivid_status = false;
	bool color_srgb_status = false;
	bool color_softiris_status = false;
	bool color_dual_panel_status = false;
	bool color_dual_brightness_status = false;
	bool color_oplus_calibrate_status = false;
	bool color_samsung_status = false;
	bool color_loading_status = false;
	bool color_2nit_status = false;
	bool color_nature_profession_status = false;
	struct dsi_parser_utils *utils = NULL;
	struct dsi_panel *panel = NULL;
	int display_id = iris_color_status->color_dual_panel_status;

	struct dsi_display *display = get_main_display();
	if (1 == display_id)
		display = get_sec_display();
	if (!display) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	panel = display->panel;
	if (!panel) {
		OPLUS_DSI_ERR("panel is null\n");
		return -EINVAL;
	}

	utils = &panel->utils;
	if (!utils) {
		OPLUS_DSI_ERR("utils is null\n");
		return -EINVAL;
	}

	color_vivid_status = utils->read_bool(utils->data, "oplus,color_vivid_status");
	OPLUS_DSI_INFO("oplus,color_vivid_status: %s\n", color_vivid_status ? "true" : "false");

	color_srgb_status = utils->read_bool(utils->data, "oplus,color_srgb_status");
	OPLUS_DSI_INFO("oplus,color_srgb_status: %s\n", color_srgb_status ? "true" : "false");

	color_softiris_status = utils->read_bool(utils->data, "oplus,color_softiris_status");
	OPLUS_DSI_INFO("oplus,color_softiris_status: %s\n", color_softiris_status ? "true" : "false");

	color_dual_panel_status = utils->read_bool(utils->data, "oplus,color_dual_panel_status");
	OPLUS_DSI_INFO("oplus,color_dual_panel_status: %s\n", color_dual_panel_status ? "true" : "false");

	color_dual_brightness_status = utils->read_bool(utils->data, "oplus,color_dual_brightness_status");
	OPLUS_DSI_INFO("oplus,color_dual_brightness_status: %s\n", color_dual_brightness_status ? "true" : "false");

	color_oplus_calibrate_status = utils->read_bool(utils->data, "oplus,color_oplus_calibrate_status");
	OPLUS_DSI_INFO("oplus,color_oplus_calibrate_status: %s\n", color_oplus_calibrate_status ? "true" : "false");

	color_samsung_status = utils->read_bool(utils->data, "oplus,color_samsung_status");
	OPLUS_DSI_INFO("oplus,color_samsung_status: %s\n", color_samsung_status ? "true" : "false");

	color_loading_status = utils->read_bool(utils->data, "oplus,color_loading_status");
	OPLUS_DSI_INFO("oplus,color_loading_status: %s\n", color_loading_status ? "true" : "false");

	color_2nit_status = utils->read_bool(utils->data, "oplus,color_2nit_status");
	OPLUS_DSI_INFO("oplus,color_2nit_status: %s\n", color_2nit_status ? "true" : "false");

	color_nature_profession_status = utils->read_bool(utils->data, "oplus,color_nature_profession_status");
	OPLUS_DSI_INFO("oplus,color_nature_profession_status: %s\n", color_nature_profession_status ? "true" : "false");

	iris_color_status->color_vivid_status = (uint32_t)color_vivid_status;
	iris_color_status->color_srgb_status = (uint32_t)color_srgb_status;
	iris_color_status->color_softiris_status = (uint32_t)color_softiris_status;
	iris_color_status->color_dual_panel_status = (uint32_t)color_dual_panel_status;
	iris_color_status->color_dual_brightness_status = (uint32_t)color_dual_brightness_status;
	iris_color_status->color_oplus_calibrate_status = (uint32_t)color_oplus_calibrate_status;
	iris_color_status->color_samsung_status = (uint32_t)color_samsung_status;
	iris_color_status->color_loading_status = (uint32_t)color_loading_status;
	iris_color_status->color_2nit_status = (uint32_t)color_2nit_status;
	iris_color_status->color_nature_profession_status = (uint32_t)color_nature_profession_status;

	return 0;
}

int oplus_display_panel_get_panel_type(void *data)
{
	int ret = 0;
	uint32_t *temp_save = data;
	uint32_t panel_id = (*temp_save >> 12);
	uint32_t panel_type = 0;

	struct dsi_panel *panel = NULL;
	struct dsi_parser_utils *utils = NULL;
	struct dsi_display *display = get_main_display();
	if (1 == panel_id) {
		display = get_sec_display();
	}

	if (!display) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}
	panel = display->panel;
	if (!panel) {
		OPLUS_DSI_ERR("panel is null\n");
		return -EINVAL;
	}

	utils = &panel->utils;
	if (!utils) {
		OPLUS_DSI_ERR("utils is null\n");
		return -EINVAL;
	}

	ret = utils->read_u32(utils->data, "oplus,mdss-dsi-panel-type", &panel_type);
	OPLUS_DSI_INFO("oplus,mdss-dsi-panel-type: %d\n", panel_type);

	*temp_save = panel_type;

	return ret;
}

int oplus_display_panel_hbm_lightspot_check(void)
{
	int rc = 0;
	char value[] = { 0xE0 };
	char value1[] = { 0x0F, 0xFF };
	struct dsi_display *display = get_main_display();
	struct mipi_dsi_device *mipi_device;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel in off status\n");
		return -EFAULT;
	}

	mipi_device = &display->panel->mipi_device;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		OPLUS_DSI_ERR("dsi_panel_initialized failed\n");
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);

	if (rc) {
		OPLUS_DSI_ERR("cmd engine enable failed\n");
		rc = -EINVAL;
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = mipi_dsi_dcs_write(mipi_device, 0x53, value, sizeof(value));
	usleep_range(1000, 1100);
	rc = mipi_dsi_dcs_write(mipi_device, 0x51, value1, sizeof(value1));
	usleep_range(1000, 1100);
	OPLUS_DSI_ERR("[%s] hbm_lightspot_check successfully\n",  display->panel->oplus_panel.vendor_name);

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);

unlock:

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
	return 0;
}

int oplus_display_panel_get_dp_support(void *buf)
{
	struct dsi_display *display = NULL;
	struct dsi_panel *d_panel = NULL;
	uint32_t *dp_support = buf;

	if (!dp_support) {
		OPLUS_DSI_ERR("oplus_display_panel_get_dp_support error dp_support is null\n");
		return -EINVAL;
	}

	display = get_main_display();
	if (!display) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	d_panel = display->panel;
	if (!d_panel) {
		OPLUS_DSI_ERR("panel is null\n");
		return -EINVAL;
	}

	*dp_support = d_panel->oplus_panel.dp_support;

	return 0;
}

int oplus_display_panel_set_audio_ready(void *data) {
	uint32_t *audio_ready = data;

	oplus_display_audio_ready = (*audio_ready);
	OPLUS_DSI_INFO("oplus_display_audio_ready = %d\n", oplus_display_audio_ready);

	return 0;
}

int oplus_display_panel_dump_info(void *data) {
	int ret = 0;
	struct dsi_display * temp_display;
	struct display_timing_info *timing_info = data;

	temp_display = get_main_display();

	if (temp_display == NULL) {
		OPLUS_DSI_ERR("display is null\n");
		ret = -1;
		return ret;
	}

	if(temp_display->modes == NULL) {
		OPLUS_DSI_ERR("display modes is null\n");
		ret = -1;
		return ret;
	}

	timing_info->h_active = temp_display->modes->timing.h_active;
	timing_info->v_active = temp_display->modes->timing.v_active;
	timing_info->refresh_rate = temp_display->modes->timing.refresh_rate;
	timing_info->clk_rate_hz_l32 = (uint32_t)(temp_display->modes->timing.clk_rate_hz & 0x00000000FFFFFFFF);
	timing_info->clk_rate_hz_h32 = (uint32_t)(temp_display->modes->timing.clk_rate_hz >> 32);

	return 0;
}

int oplus_display_panel_get_dsc(void *data) {
	int ret = 0;
	uint32_t *reg_read = data;
	unsigned char read[30];
	struct dsi_display *display = get_main_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	if (display->panel->power_mode == SDE_MODE_DPMS_ON) {
		mutex_lock(&display->display_lock);
		ret = dsi_display_read_panel_reg(get_main_display(), 0x03, read, 1);
		mutex_unlock(&display->display_lock);
		if (ret < 0) {
			OPLUS_DSI_ERR("read panel dsc reg error = %d\n", ret);
			ret = -1;
		} else {
			(*reg_read) = read[0];
			ret = 0;
		}
	} else {
		OPLUS_DSI_WARN("display panel status is not on\n");
		ret = -1;
	}

	return ret;
}

int oplus_display_panel_get_closebl_flag(void *data)
{
	uint32_t *closebl_flag = data;

	(*closebl_flag) = lcd_closebl_flag;
	OPLUS_DSI_INFO("oplus_display_get_closebl_flag = %d\n", lcd_closebl_flag);

	return 0;
}

int oplus_display_panel_set_closebl_flag(void *data)
{
	uint32_t *closebl = data;

	OPLUS_DSI_INFO("lcd_closebl_flag = %d\n", (*closebl));
	if (1 != (*closebl))
		lcd_closebl_flag = 0;
	OPLUS_DSI_INFO("oplus_display_set_closebl_flag = %d\n", lcd_closebl_flag);

	return 0;
}

int oplus_display_panel_get_reg(void *data)
{
	struct dsi_display *display = get_main_display();
	struct panel_reg_get *panel_reg = data;
	uint32_t u32_bytes = sizeof(uint32_t)/sizeof(char);

	if (!display) {
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	u32_bytes = oplus_rx_len%u32_bytes ? (oplus_rx_len/u32_bytes + 1) : oplus_rx_len/u32_bytes;
	oplus_big_endian_copy(panel_reg->reg_rw, oplus_rx_reg, u32_bytes);
	panel_reg->lens = oplus_rx_len;

	mutex_unlock(&display->display_lock);

	return 0;
}

int oplus_display_panel_set_reg(void *data)
{
	char reg[PANEL_TX_MAX_BUF] = {0x0};
	char payload[PANEL_TX_MAX_BUF] = {0x0};
	u32 index = 0, value = 0;
	int ret = 0;
	int len = 0;
	struct dsi_display *display = get_main_display();
	struct panel_reg_rw *reg_rw = data;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EFAULT;
	}

	if (reg_rw->lens > PANEL_IOCTL_BUF_MAX) {
		OPLUS_DSI_ERR("error: wrong input reg len\n");
		return -EINVAL;
	}

	if (reg_rw->rw_flags == REG_READ) {
		value = reg_rw->cmd;
		len = reg_rw->lens;
		mutex_lock(&display->display_lock);
		dsi_display_read_panel_reg(get_main_display(), value, reg, len);
		mutex_unlock(&display->display_lock);

		for (index=0; index < len; index++) {
			OPLUS_DSI_INFO("reg[%d] = %x\n", index, reg[index]);
		}
		mutex_lock(&display->display_lock);
		memcpy(oplus_rx_reg, reg, PANEL_TX_MAX_BUF);
		oplus_rx_len = len;
		mutex_unlock(&display->display_lock);
		return 0;
	}

	if (reg_rw->rw_flags == REG_WRITE) {
		memcpy(payload, reg_rw->value, reg_rw->lens);
		reg[0] = reg_rw->cmd;
		len = reg_rw->lens;
		for (index=0; index < len; index++) {
			reg[index + 1] = payload[index];
		}

		if (display->panel->power_mode != SDE_MODE_DPMS_OFF) {
				/* enable the clk vote for CMD mode panels */
			mutex_lock(&display->display_lock);
			mutex_lock(&display->panel->panel_lock);

			if (display->panel->panel_initialized) {
				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_CORE_CLK | DSI_LINK_CLK, DSI_CLK_ON);
				}
				ret = mipi_dsi_dcs_write(&display->panel->mipi_device, reg[0],
						payload, len);
				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_CORE_CLK | DSI_LINK_CLK, DSI_CLK_OFF);
				}
			}

			mutex_unlock(&display->panel->panel_lock);
			mutex_unlock(&display->display_lock);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}
	OPLUS_DSI_ERR("error: please check the args\n");
	return -1;
}

int oplus_display_panel_notify_blank(void *data)
{
	uint32_t *temp_save_user = data;
	int temp_save = (*temp_save_user);

	OPLUS_DSI_INFO("oplus_display_notify_panel_blank = %d\n", temp_save);

	if(temp_save == 1) {
		oplus_event_data_notifier_trigger(DRM_PANEL_EVENT_UNBLANK, 0, true);
	} else if (temp_save == 0) {
		oplus_event_data_notifier_trigger(DRM_PANEL_EVENT_BLANK, 0, true);
	}
	return 0;
}

int oplus_display_panel_get_spr(void *data)
{
	uint32_t *spr_mode_user = data;

	OPLUS_DSI_INFO("oplus_display_get_spr = %d\n", spr_mode);
	*spr_mode_user = spr_mode;

	return 0;
}

int oplus_display_panel_set_spr(void *data)
{
	uint32_t *temp_save_user = data;
	int temp_save = (*temp_save_user);
	struct dsi_display *display = get_main_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	OPLUS_DSI_INFO("oplus_display_set_spr = %d\n", temp_save);

	__oplus_display_set_spr(temp_save);

	if (display->panel->power_mode == SDE_MODE_DPMS_ON) {
		if(get_main_display() == NULL) {
			OPLUS_DSI_ERR("display is null\n");
			return 0;
		}

		dsi_display_spr_mode(get_main_display(), spr_mode);
	} else {
		OPLUS_DSI_WARN("oplus_display_set_spr = %d, but now display panel status is not on\n",
				temp_save);
	}
	return 0;
}

int oplus_display_panel_get_dither(void *data)
{
	uint32_t *dither_mode_user = data;
	OPLUS_DSI_ERR("oplus_display_get_dither = %d\n", dither_enable);
	*dither_mode_user = dither_enable;
	return 0;
}

int oplus_display_panel_set_dither(void *data)
{
	uint32_t *temp_save_user = data;
	int temp_save = (*temp_save_user);
	OPLUS_DSI_INFO("oplus_display_set_dither = %d\n", temp_save);
	__oplus_display_set_dither(temp_save);
	return 0;
}

int oplus_display_panel_get_roundcorner(void *data)
{
	uint32_t *round_corner = data;
	bool roundcorner = true;

	*round_corner = roundcorner;

	return 0;
}
int oplus_display_panel_set_osc_track(u32 osc_status)
{
	struct dsi_display *display = get_main_display();
	int rc = 0;

	if (!display||!display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

#ifdef OPLUS_FEATURE_DISPLAY_OSC
	if (osc_status) {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_TRACK_ON, false);
	} else {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_TRACK_OFF, false);
	}
#endif
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

unlock:
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}

int oplus_display_panel_get_dynamic_osc_clock(void *data)
{
	int rc = 0;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;
	uint32_t *osc_rate = data;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if (!display->panel->oplus_panel.ffc_enabled) {
		OPLUS_DSI_WARN("FFC is disabled, failed to get osc rate\n");
		rc = -EFAULT;
		return rc;
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	*osc_rate = panel->oplus_panel.osc_rate_cur;

	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);
	OPLUS_DSI_INFO("Get osc rate=%d\n", *osc_rate);

	return rc;
}

int oplus_display_panel_set_dynamic_osc_clock(void *data)
{
	int rc = 0;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;
	uint32_t *osc_rate = data;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if (!display->panel->oplus_panel.ffc_enabled) {
		OPLUS_DSI_WARN("FFC is disabled, failed to set osc rate\n");
		rc = -EFAULT;
		return rc;
	}

	if(display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel is not on\n");
		rc = -EFAULT;
		return rc;
	}

	OPLUS_DSI_INFO("Set osc rate=%d\n", *osc_rate);
	mutex_lock(&display->display_lock);

	rc = oplus_display_update_osc_ffc(display, *osc_rate);
	if (!rc) {
		mutex_lock(&panel->panel_lock);
		rc = oplus_panel_set_ffc_mode_unlock(panel);
		mutex_unlock(&panel->panel_lock);
	}

	mutex_unlock(&display->display_lock);

	return rc;
}

int oplus_display_panel_get_cabc_status(void *buf)
{
	int rc = 0;
	uint32_t *cabc_status = buf;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	if(panel->oplus_panel.cabc_enabled) {
		*cabc_status = oplus_cabc_status;
	} else {
		*cabc_status = OPLUS_DISPLAY_CABC_OFF;
	}

	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);
	OPLUS_DSI_INFO("Get cabc status: %d\n", *cabc_status);

	return rc;
}

int oplus_display_panel_set_cabc_status(void *buf)
{
	int rc = 0;
	uint32_t *cabc_status = buf;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;
	u32 cmd_index = 0;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if (!panel->oplus_panel.cabc_enabled) {
		OPLUS_DSI_WARN("This project don't support cabc\n");
		rc = -EFAULT;
		return rc;
	}

	if (*cabc_status >= OPLUS_DISPLAY_CABC_UNKNOW) {
		OPLUS_DSI_ERR("Unknow cabc status: %d\n", *cabc_status);
		rc = -EINVAL;
		return rc;
	}

	if(display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel is not on, buf=[%s]\n", (char *)buf);
		rc = -EFAULT;
		return rc;
	}

	OPLUS_DSI_INFO("Set cabc status: %d, buf=[%s]\n", *cabc_status, (char *)buf);
	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	cmd_index = DSI_CMD_CABC_OFF + *cabc_status;
	rc = dsi_panel_tx_cmd_set(panel, cmd_index, false);
	oplus_cabc_status = *cabc_status;

	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}

int oplus_display_panel_get_dre_status(void *buf)
{
	int rc = 0;
	uint32_t *dre_status = buf;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if(panel->oplus_panel.dre_enabled) {
		*dre_status = oplus_dre_status;
	} else {
		*dre_status = OPLUS_DISPLAY_DRE_OFF;
	}

	return rc;
}

int oplus_display_panel_set_dre_status(void *buf)
{
	int rc = 0;
	uint32_t *dre_status = buf;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if(!panel->oplus_panel.dre_enabled) {
		OPLUS_DSI_ERR("This project don't support dre\n");
		return -EFAULT;
	}

	if (*dre_status >= OPLUS_DISPLAY_DRE_UNKNOW) {
		OPLUS_DSI_ERR("Unknow DRE status = [%d]\n", *dre_status);
		return -EINVAL;
	}

	if(panel->power_mode == SDE_MODE_DPMS_ON) {
		if (*dre_status == OPLUS_DISPLAY_DRE_ON) {
			/* if(mtk)  */
			/*	disp_aal_set_dre_en(0);   MTK AAL api */
		} else {
			/* if(mtk) */
			/*	disp_aal_set_dre_en(1);  MTK AAL api */
		}
		oplus_dre_status = *dre_status;
		OPLUS_DSI_INFO("buf = [%s], oplus_dre_status = %d\n",
				(char *)buf, oplus_dre_status);
	} else {
		OPLUS_DSI_WARN("buf = [%s], but display panel status is not on\n",
				(char *)buf);
	}

	return rc;
}

int oplus_display_panel_get_dither_status(void *buf)
{
	uint32_t *dither_enable = buf;
	*dither_enable = oplus_dither_enable;

	return 0;
}

int oplus_display_panel_set_dither_status(void *buf)
{
	uint32_t *dither_enable = buf;
	oplus_dither_enable = *dither_enable;
	OPLUS_DSI_INFO("buf = [%s], oplus_dither_enable = %d\n",
			(char *)buf, oplus_dither_enable);

	return 0;
}

int oplus_panel_set_ffc_mode_unlock(struct dsi_panel *panel)
{
	int rc = 0;
	u32 cmd_index = DSI_CMD_SET_MAX;

	if (panel->oplus_panel.ffc_mode_index >= FFC_MODE_MAX_COUNT) {
		OPLUS_DSI_ERR("Invalid ffc_mode_index=%d\n",
				panel->oplus_panel.ffc_mode_index);
		rc = -EINVAL;
		return rc;
	}

#ifdef OPLUS_FEATURE_DISPLAY_FFC
	cmd_index = DSI_CMD_FFC_MODE0 + panel->oplus_panel.ffc_mode_index;
#endif
	rc = dsi_panel_tx_cmd_set(panel, cmd_index, false);

	return rc;
}

int oplus_panel_set_ffc_kickoff_lock(struct dsi_panel *panel)
{
	int rc = 0;

	mutex_lock(&panel->oplus_panel.oplus_ffc_lock);
	panel->oplus_panel.ffc_delay_frames--;
	if (panel->oplus_panel.ffc_delay_frames) {
		mutex_unlock(&panel->oplus_panel.oplus_ffc_lock);
		return rc;
	}

	mutex_lock(&panel->panel_lock);
	rc = oplus_panel_set_ffc_mode_unlock(panel);
	mutex_unlock(&panel->panel_lock);

	mutex_unlock(&panel->oplus_panel.oplus_ffc_lock);

	return rc;
}

int oplus_panel_check_ffc_config(struct dsi_panel *panel,
		struct oplus_clk_osc *clk_osc_pending)
{
	int rc = 0;
	int index;
	struct oplus_clk_osc *seq = panel->oplus_panel.clk_osc_seq;
	u32 count = panel->oplus_panel.ffc_mode_count;
	u32 last_index = panel->oplus_panel.ffc_mode_index;

	if (!seq || !count) {
		OPLUS_DSI_ERR("Invalid clk_osc_seq or ffc_mode_count\n");
		rc = -EINVAL;
		return rc;
	}

	for (index = 0; index < count; index++) {
		if (seq->clk_rate == clk_osc_pending->clk_rate &&
				seq->osc_rate == clk_osc_pending->osc_rate) {
			break;
		}
		seq++;
	}

	if (index < count) {
		OPLUS_DSI_INFO("Update ffc config: index:[%d -> %d], clk=%d, osc=%d\n",
				last_index,
				index,
				clk_osc_pending->clk_rate,
				clk_osc_pending->osc_rate);

		panel->oplus_panel.ffc_mode_index = index;
		panel->oplus_panel.clk_rate_cur = clk_osc_pending->clk_rate;
		panel->oplus_panel.osc_rate_cur = clk_osc_pending->osc_rate;
	} else {
		rc = -EINVAL;
	}

	return rc;
}

int oplus_display_update_clk_ffc(struct dsi_display *display,
		struct dsi_display_mode *cur_mode, struct dsi_display_mode *adj_mode)
{
	int rc = 0;
	struct dsi_panel *panel = display->panel;
	struct oplus_clk_osc clk_osc_pending;

	INFO_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Switching ffc mode, clk:[%d -> %d]",
			OPLUS_DISP_Q_INFO_DYN_MIPI,
			display->cached_clk_rate,
			display->dyn_bit_clk);

	if (display->cached_clk_rate == display->dyn_bit_clk) {
		INFO_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Ignore duplicated clk ffc setting, clk=%d",
				OPLUS_DISP_Q_INFO_DYN_MIPI_INVALID,
				display->dyn_bit_clk);
		return rc;
	}

	mutex_lock(&panel->oplus_panel.oplus_ffc_lock);

	clk_osc_pending.clk_rate = display->dyn_bit_clk;
	clk_osc_pending.osc_rate = panel->oplus_panel.osc_rate_cur;

	rc = oplus_panel_check_ffc_config(panel, &clk_osc_pending);
	if (!rc) {
		panel->oplus_panel.ffc_delay_frames = FFC_DELAY_MAX_FRAMES;
	} else {
		EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Failed to find ffc mode index, clk=%d, osc=%d",
				OPLUS_DISP_Q_INFO_DYN_MIPI_INVALID,
				clk_osc_pending.clk_rate,
				clk_osc_pending.osc_rate);
	}

	mutex_unlock(&panel->oplus_panel.oplus_ffc_lock);

	return rc;
}

int oplus_display_update_osc_ffc(struct dsi_display *display,
		u32 osc_rate)
{
	int rc = 0;
	struct dsi_panel *panel = display->panel;
	struct oplus_clk_osc clk_osc_pending;

	INFO_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Switching ffc mode, osc:[%d -> %d]",
			OPLUS_DISP_Q_INFO_DYN_OSC,
			panel->oplus_panel.osc_rate_cur,
			osc_rate);

	if (osc_rate == panel->oplus_panel.osc_rate_cur) {
		INFO_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Ignore duplicated osc ffc setting, osc=%d",
				OPLUS_DISP_Q_INFO_DYN_OSC_INVALID,
				panel->oplus_panel.osc_rate_cur);
		return rc;
	}

	mutex_lock(&panel->oplus_panel.oplus_ffc_lock);

	clk_osc_pending.clk_rate = panel->oplus_panel.clk_rate_cur;
	clk_osc_pending.osc_rate = osc_rate;
	rc = oplus_panel_check_ffc_config(panel, &clk_osc_pending);
	if (rc) {
		EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$Failed to find ffc mode index, clk=%d, osc=%d",
				OPLUS_DISP_Q_INFO_DYN_OSC_INVALID,
				clk_osc_pending.clk_rate,
				clk_osc_pending.osc_rate);
	}

	mutex_unlock(&panel->oplus_panel.oplus_ffc_lock);

	return rc;
}

int oplus_display_tx_cmd_set_lock(struct dsi_display *display, enum dsi_cmd_set_type type)
{
	int rc = 0;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);
	rc = dsi_panel_tx_cmd_set(display->panel, type, false);
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}

int oplus_display_panel_get_iris_loopback_status(void *buf)
{
#if defined(CONFIG_PXLW_IRIS)
	uint32_t *status = buf;

	*status = iris_loop_back_validate();
#endif

	return 0;
}

int oplus_display_panel_set_hbm_max(void *data)
{
	int rc = 0;
	static u32 last_bl = 0;
	u32 *buf = data;
	u32 hbm_max_state = *buf & 0xF;
	int panel_id = (*buf >> 12);
	struct dsi_panel *panel = NULL;
	struct dsi_display *display = get_main_display();

	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}

	panel = display->panel;

	if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel is not on\n");
		rc = -EFAULT;
		return rc;
	}

	OPLUS_DSI_INFO("Set hbm max state=%d\n", hbm_max_state);

	mutex_lock(&display->display_lock);

	last_bl = oplus_last_backlight;
	if (hbm_max_state) {
		if (panel->cur_mode->priv_info->cmd_sets[DSI_CMD_HBM_MAX].count) {
			mutex_lock(&panel->panel_lock);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_HBM_MAX, false);
			mutex_unlock(&panel->panel_lock);
		}
		else {
			OPLUS_DSI_WARN("DSI_CMD_HBM_MAX is undefined, set max backlight: %d\n",
					panel->bl_config.bl_max_level);
			rc = dsi_display_set_backlight(display->drm_conn,
					display, panel->bl_config.bl_max_level);
		}
	}
	else {
		if (panel->cur_mode->priv_info->cmd_sets[DSI_CMD_EXIT_HBM_MAX].count) {
			oplus_ae174_apl_gamma_update(display, last_bl);
			mutex_lock(&panel->panel_lock);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_EXIT_HBM_MAX, false);
			mutex_unlock(&panel->panel_lock);
		} else {
			rc = dsi_display_set_backlight(display->drm_conn,
					display, last_bl);
		}
	}
	panel->oplus_panel.hbm_max_state = hbm_max_state;

	mutex_unlock(&display->display_lock);

	if (!hbm_max_state) {
		if (panel->oplus_panel.bl_cfg.hbm_max_exit_restore_gir) {
			dsi_display_seed_mode_lock(get_main_display(), seed_mode);
		}
	}

	return rc;
}

int oplus_display_panel_get_hbm_max(void *data)
{
	int rc = 0;
	u32 *hbm_max_state = data;
	int panel_id = (*hbm_max_state >> 12);
	struct dsi_panel *panel = NULL;
	struct dsi_display *display = get_main_display();

	if (panel_id == 1)
		display = get_sec_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}

	panel = display->panel;

	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	*hbm_max_state = panel->oplus_panel.hbm_max_state;

	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);
	OPLUS_DSI_INFO("Get hbm max state: %d\n", *hbm_max_state);

	return rc;
}

int oplus_display_panel_set_dc_compensate(void *data)
{
	uint32_t *dc_compensate = data;
	dcc_flags = (int)(*dc_compensate);
	OPLUS_DSI_INFO("DCCompensate set dc %d\n", dcc_flags);
	if (dcc_flags == FILE_DESTROY) {
		EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@431$$DCCompensate file destroied!!");
	}
	return 0;
}

int oplus_display_panel_set_mipi_err_check(void *data)
{
	u32 *check_result = data;
	struct dsi_panel *panel = NULL;
	struct dsi_display *display = oplus_display_get_current_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		*check_result = -EINVAL;
		return -EINVAL;
	}

	panel = display->panel;

	if (display->panel->power_mode != SDE_MODE_DPMS_ON ||
			!(panel->oplus_panel.mipi_err_config.config & OPLUS_REGS_CHECK_ENABLE)) {
		OPLUS_DSI_ERR("power mode not SDE_MODE_DPMS_ON or mipi err check is disable!\n");
		*check_result = -EINVAL;
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	*check_result = oplus_panel_mipi_err_check(panel);
	OPLUS_DSI_INFO("mipi err check result: %d\n", *check_result);
	mutex_unlock(&panel->panel_lock);

	return *check_result;
}

int oplus_display_panel_get_mipi_err_check(void *data)
{
	int rc = 0;
	u32 *check_result = data;
	struct dsi_panel *panel = NULL;
	struct dsi_display *display = oplus_display_get_current_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		return -EINVAL;
	}

	panel = display->panel;

	if (panel->oplus_panel.mipi_err_config.config & OPLUS_REGS_CHECK_ENABLE) {
		OPLUS_DSI_INFO("mipi err check is enable\n");
		*check_result = 1;
	} else {
		OPLUS_DSI_INFO("mipi err check is disable\n");
		*check_result = 0;
	}

	return rc;
}
int oplus_display_panel_set_white_point_status(void *data)
{
	int rc = 0;
	uint32_t *flag = data;

	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = NULL;
	u32 cmd_index = 0;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Invalid display or panel\n");
		rc = -EINVAL;
		return rc;
	}
	panel = display->panel;

	if (!panel->oplus_panel.white_point_compensation_enabled) {
		OPLUS_DSI_WARN("This project don't support white point compensation\n");
		rc = -EFAULT;
		return rc;
	}

	if(display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_WARN("display panel is not on, data=[%s]\n", (char *)data);
		rc = -EFAULT;
		return rc;
	}

	if (*flag > 1) {
		OPLUS_DSI_ERR("Unknow switch status: %d\n", *flag);
		rc = -EINVAL;
		return rc;
	}

	OPLUS_DSI_INFO("Set white point compensation state: %d, data=[%s]\n", *flag, (char *)data);
	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	cmd_index = DSI_CMD_REDUCE_WHITE_POINT_OFF + *flag;
	rc = dsi_panel_tx_cmd_set(panel, cmd_index, false);

	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}

int oplus_display_ioctl_get_panel_btbsn(void *buf)
{
	int rc = 0;
	struct panel_btbsn *btbsn = buf;
	char btbsn_buf[PANEL_IOCTL_BUF_MAX] = {0};
	struct dsi_display *display = NULL;
	int panel_id = btbsn->sn[0];

	display = get_main_display();
	if (1 == panel_id) {
		display = get_sec_display();
	}

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return rc;
	}

	if(display->enabled == false) {
		OPLUS_DSI_WARN("primary display is disable, try sec display\n");
		display = get_sec_display();
		if (!display) {
			OPLUS_DSI_WARN("second display is null\n");
			return rc;
		}
		if (display->enabled == false) {
			OPLUS_DSI_WARN("second panel is disabled\n");
			return rc;
		}
	}

	if (!oplus_display_get_panel_btbsn_sub(display, btbsn_buf)) {
		strncpy(btbsn->sn, btbsn_buf + display->panel->oplus_panel.btb_sn.btb_sn_index, sizeof(btbsn->sn));
		btbsn->sn[sizeof(btbsn->sn) - 1] = '\0';
		OPLUS_DSI_INFO("Get panel btb sn: %s", btbsn->sn);
	} else {
		OPLUS_DSI_ERR("Get panel btb sn: read failed\n");
	}

	return rc;
}
