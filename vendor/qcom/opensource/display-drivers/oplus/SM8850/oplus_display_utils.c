/***************************************************************
** Copyright (C), 2024, OPLUS Mobile Comm Corp., Ltd
**
** File : oplus_display_utils.c
** Description : display driver private utils
** Version : 1.1
** Date : 2024/05/09
** Author : Display
******************************************************************/
#include "oplus_display_utils.h"
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/system/oplus_project.h>
#include <soc/oplus/device_info.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include "dsi_display.h"
#include "oplus_debug.h"
#include "oplus_display_panel_cmd.h"
#include "oplus_onscreenfingerprint.h"
#include "oplus_display_ext.h"
#include "oplus_display_device_ioctl.h"
#ifdef OPLUS_TRACKPOINT_REPORT
#include "oplus_trackpoint_report.h"
#endif /* OPLUS_TRACKPOINT_REPORT */

#ifdef OPLUS_FEATURE_DISPLAY_ADFR
#include "oplus_adfr.h"
#endif /* OPLUS_FEATURE_DISPLAY_ADFR */

#ifdef OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT
#include "oplus_onscreenfingerprint.h"
#endif /* OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT */

#define GAMMA_COMPENSATION_READ_RETRY_MAX 5
#define GAMMA_COMPENSATION_PERCENTAGE1 82/100
#define GAMMA_COMPENSATION_PERCENTAGE2 87/100
#define REG_SIZE 256
#define OPLUS_DSI_CMD_PRINT_BUF_SIZE 1024
#define GAMMA_COMPENSATION_REG_CNT 3
#define GAMMA_COMPENSATION_READ_LENGTH 6
#define GAMMA_COMPENSATION_READ_REG_11 0x85
#define GAMMA_COMPENSATION_READ_REG_8 0x84
#define GAMMA_COMPENSATION_READ_REG_5 0x83
#define GAMMA_COMPENSATION_READ_REG_3 0x82
#define GAMMA_COMPENSATION_READ_REG_1 0x81
#define GAMMA_COMPENSATION_READ_REG_0 0x80
#define GAMMA_COMPENSATION_BAND_REG 0x99
#define GAMMA_COMPENSATION_BAND_120HZ 0x8B
#define GAMMA_COMPENSATION_BAND_90HZ 0xBB
#define GAMMA_COMPENSATION_120HZ_GREY8_R 97/100
#define GAMMA_COMPENSATION_120HZ_GREY8_G 95/100
#define GAMMA_COMPENSATION_120HZ_GREY8_B 99/100
#define GAMMA_COMPENSATION_90HZ_GREY8_R 97/100
#define GAMMA_COMPENSATION_90HZ_GREY8_G 92/100
#define GAMMA_COMPENSATION_90HZ_GREY8_B 98/100
#define GAMMA_COMPENSATION_120HZ_GREY5_R 94/100
#define GAMMA_COMPENSATION_120HZ_GREY5_G 86/100
#define GAMMA_COMPENSATION_120HZ_GREY5_B 97/100
#define GAMMA_COMPENSATION_90HZ_GREY5_R 92/100
#define GAMMA_COMPENSATION_90HZ_GREY5_G 81/100
#define GAMMA_COMPENSATION_90HZ_GREY5_B 96/100
#define GAMMA_COMPENSATION_120HZ_GREY3_R 90/100
#define GAMMA_COMPENSATION_120HZ_GREY3_G 79/100
#define GAMMA_COMPENSATION_120HZ_GREY3_B 96/100
#define GAMMA_COMPENSATION_90HZ_GREY3_R 86/100
#define GAMMA_COMPENSATION_90HZ_GREY3_G 72/100
#define GAMMA_COMPENSATION_90HZ_GREY3_B 93/100
#define GAMMA_COMPENSATION_120HZ_GREY1_R 85/100
#define GAMMA_COMPENSATION_120HZ_GREY1_G 73/100
#define GAMMA_COMPENSATION_120HZ_GREY1_B 95/100
#define GAMMA_COMPENSATION_90HZ_GREY1_R 82/100
#define GAMMA_COMPENSATION_90HZ_GREY1_G 65/100
#define GAMMA_COMPENSATION_90HZ_GREY1_B 91/100
#define GAMMA_COMPENSATION_120HZ_GREY0_R 27/100
#define GAMMA_COMPENSATION_120HZ_GREY0_G 316/1000
#define GAMMA_COMPENSATION_120HZ_GREY0_B 384/1000
#define GAMMA_COMPENSATION_90HZ_GREY0_R 258/1000
#define GAMMA_COMPENSATION_90HZ_GREY0_G 288/1000
#define GAMMA_COMPENSATION_90HZ_GREY0_B 372/1000

bool g_gamma_regs_read_done = false;
bool g_gamma_inverse = false;
EXPORT_SYMBOL(g_gamma_regs_read_done);
EXPORT_SYMBOL(g_gamma_inverse);

#define ROUND_FIXED_POINT(num) \
    ((int)(((num) >= 0) ? ((num) + 1000000/2) / 1000000 : ((num) - 1000000/2) / 1000000))
#define GAMMA_SEND_CHECK(_rc) \
	if ((_rc)) { \
		OPLUS_DSI_ERR("[%s:%d] rc=%d\n", __func__, __LINE__, (_rc)); \
		retry_count++; \
		continue; \
	}

#define GAMMA_READ_CHECK(_rc) \
    if ((_rc) < 0) { \
        OPLUS_DSI_ERR("[%s:%d] rc=%d\n", __func__, __LINE__, (_rc)); \
        retry_count++; \
        continue; \
    }

u32 gamma120_ratio[25][3] = {
	{ 1000363,    1000387,    1000329 },
	{ 1000726,    1000388,    1000329 },
	{ 1001110,    1000789,    1001005 },
	{ 1001132,    1001610,    1001024 },
	{ 1001145,    1000813,    1000690 },
	{ 1000300,    1000424,    1000359 },
	{ 1000839,    1000892,    1001506 },
	{ 1000446,    1000474,    1001195 },
	{ 1000000,    1000492,    1000823 },
	{ 1000000,    1000000,    1000427 },
	{ 999492,     998923,     1000000 },
	{ 999459,     998282,     999530 },
	{ 997654,     996278,     997988 },
	{ 996289,     994125,     995807 },
	{ 995533,     992603,     995704 },
	{ 992079,     990972,     993943 },
	{ 990385,     987761,     992059 },
	{ 986360,     982784,     988297 },
	{ 979669,     975078,     983686 },
	{ 974537,     970635,     981572 },
	{ 969219,     966855,     978166 },
	{ 964545,     962171,     977186 },
	{ 961349,     958961,     977434 },
	{ 957983,     955594,     978332 },
	{ 1000000,    1000000,	  1000000 },
};
u32 gamma60_ratio[5][3] = {
	{ 1000100,    1001000,    1000500 },
	{ 1000000 ,   1000400 ,   1000500 },
	{ 1000000 ,   1000400 ,   1000500 },
	{ 1000000 ,   1000400 ,   1000500 },
	{ 1000000 ,   1000400 ,   1000500 },
};
u32 gammaregs200nitL80_ratio[3][2] = {
	{ 998500,    998500 },
	{ 998500 ,   998500 },
	{ 999500 ,   999500 },
};

/* log level config */
unsigned int oplus_display_log_level = OPLUS_LOG_LEVEL_INFO;
unsigned int oplus_display_trace_enable = OPLUS_DISPLAY_DISABLE_TRACE;
unsigned int oplus_display_log_type = OPLUS_DEBUG_LOG_DISABLED;

static enum oplus_display_support_list  oplus_display_vendor =
		OPLUS_DISPLAY_UNKNOW;
static BLOCKING_NOTIFIER_HEAD(oplus_display_notifier_list);

static struct dsi_display *primary_display = NULL;
static struct dsi_display *secondary_display = NULL;
/* add for dual panel */
static struct dsi_display *current_display = NULL;

bool refresh_rate_change = false;

int oplus_sync_power_state = 0;

struct panel_ae174_gamma ae174_gamma_regs = {
	.l_r_gamma = 0xCA3,
	.l_g_gamma = 0xB47,
	.l_b_gamma = 0xD0A,
	.a_r_gamma = 0xC9B,
	.a_g_gamma = 0xB42,
	.a_b_gamma = 0xD05,
	.elvss_reg = 0x38,
};

struct dsi_display *get_main_display(void) {
		return primary_display;
}
EXPORT_SYMBOL(get_main_display);

struct dsi_display *get_sec_display(void) {
		return secondary_display;
}
EXPORT_SYMBOL(get_sec_display);

struct dsi_display *oplus_display_get_current_display(void)
{
	return current_display;
}

int oplus_display_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&oplus_display_notifier_list, nb);
}
EXPORT_SYMBOL(oplus_display_register_client);


int oplus_display_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&oplus_display_notifier_list,
			nb);
}
EXPORT_SYMBOL(oplus_display_unregister_client);

bool oplus_is_correct_display(enum oplus_display_support_list lcd_name)
{
	return (oplus_display_vendor == lcd_name ? true : false);
}

bool oplus_is_silence_reboot(void)
{
	OPLUS_DSI_INFO("get_boot_mode = %d\n", get_boot_mode());
	if ((MSM_BOOT_MODE__SILENCE == get_boot_mode())
			|| (MSM_BOOT_MODE__SAU == get_boot_mode())) {
		return true;

	} else {
		return false;
	}
	return false;
}
EXPORT_SYMBOL(oplus_is_silence_reboot);

bool oplus_is_factory_boot(void)
{
	OPLUS_DSI_INFO("get_boot_mode = %d\n", get_boot_mode());
	if ((MSM_BOOT_MODE__FACTORY == get_boot_mode())
			|| (MSM_BOOT_MODE__RF == get_boot_mode())
			|| (MSM_BOOT_MODE__WLAN == get_boot_mode())
			|| (MSM_BOOT_MODE__MOS == get_boot_mode())) {
		return true;
	} else {
		return false;
	}
	return false;
}
EXPORT_SYMBOL(oplus_is_factory_boot);

int oplus_panel_event_data_notifier_trigger(struct dsi_panel *panel,
		enum panel_event_notification_type notif_type,
		u32 data,
		bool early_trigger)
{
	struct panel_event_notification notifier;
	enum panel_event_notifier_tag panel_type;
	char tag_name[256];

	if (!panel) {
		OPLUS_DSI_ERR("Oplus Features config No panel device\n");
		return -ENODEV;
	}

	if (!strcmp(panel->type, "secondary")) {
		panel_type = PANEL_EVENT_NOTIFICATION_SECONDARY;
	} else {
		panel_type = PANEL_EVENT_NOTIFICATION_PRIMARY;
	}

	snprintf(tag_name, sizeof(tag_name),
		"oplus_panel_event_data_notifier_trigger : [%s] type=0x%X, data=%d, early_trigger=%d",
		panel->type, notif_type, data, early_trigger);
	OPLUS_DSI_TRACE_BEGIN(tag_name);

	OPLUS_DSI_DEBUG("[%s] type=0x%X, data=%d, early_trigger=%d\n",
			panel->type, notif_type, data, early_trigger);

	memset(&notifier, 0, sizeof(notifier));

	notifier.panel = &panel->drm_panel;
	notifier.notif_type = notif_type;
	notifier.notif_data.data = data;
	notifier.notif_data.early_trigger = early_trigger;

	panel_event_notification_trigger(panel_type, &notifier);

	OPLUS_DSI_TRACE_END(tag_name);
	return 0;
}
EXPORT_SYMBOL(oplus_panel_event_data_notifier_trigger);

int oplus_event_data_notifier_trigger(
		enum panel_event_notification_type notif_type,
		u32 data,
		bool early_trigger)
{
	struct dsi_display *display = oplus_display_get_current_display();

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("Oplus Features config No display device\n");
		return -ENODEV;
	}

	oplus_panel_event_data_notifier_trigger(display->panel,
			notif_type, data, early_trigger);

	return 0;
}
EXPORT_SYMBOL(oplus_event_data_notifier_trigger);

int oplus_panel_backlight_notifier(struct dsi_panel *panel, u32 bl_lvl)
{
	u32 threshold = panel->oplus_panel.bl_cfg.dc_backlight_threshold;
	bool dc_mode = panel->oplus_panel.bl_cfg.oplus_dc_mode;

	if (dc_mode && (bl_lvl > 1 && bl_lvl < threshold)) {
		dc_mode = false;
		oplus_panel_event_data_notifier_trigger(panel,
				DRM_PANEL_EVENT_DC_MODE, dc_mode, true);
	} else if (!dc_mode && bl_lvl >= threshold) {
		dc_mode = true;
		oplus_panel_event_data_notifier_trigger(panel,
				DRM_PANEL_EVENT_DC_MODE, dc_mode, true);
	}

	oplus_panel_event_data_notifier_trigger(panel,
			DRM_PANEL_EVENT_BACKLIGHT, bl_lvl, true);

	return 0;
}
EXPORT_SYMBOL(oplus_panel_backlight_notifier);

/* add for dual panel */
void oplus_display_set_current_display(void *dsi_display)
{
	struct dsi_display *display = dsi_display;
	current_display = display;
}

/* update current display when panel is enabled and disabled */
void oplus_display_update_current_display(void)
{
	struct dsi_display *primary_display = get_main_display();
	struct dsi_display *secondary_display = get_sec_display();

	OPLUS_DSI_DEBUG("start\n");

	if ((!primary_display && !secondary_display) || (!primary_display->panel && !secondary_display->panel)) {
		current_display = NULL;
	} else if ((primary_display && !secondary_display) || (primary_display->panel && !secondary_display->panel)) {
		current_display = primary_display;
	} else if ((!primary_display && secondary_display) || (!primary_display->panel && secondary_display->panel)) {
		current_display = secondary_display;
	} else if (primary_display->panel->panel_initialized && !secondary_display->panel->panel_initialized) {
		current_display = primary_display;
	} else if (!primary_display->panel->panel_initialized && secondary_display->panel->panel_initialized) {
		current_display = secondary_display;
	} else if (primary_display->panel->panel_initialized && secondary_display->panel->panel_initialized) {
		current_display = primary_display;
	}

#ifdef OPLUS_FEATURE_DISPLAY_ADFR
	oplus_adfr_update_display_id();
#endif /* OPLUS_FEATURE_DISPLAY_ADFR */

#ifdef OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT
	if (oplus_ofp_is_supported()) {
		oplus_ofp_update_display_id();
	}
#endif /* OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT */

	OPLUS_DSI_DEBUG("end\n");

	return;
}

void oplus_check_refresh_rate(const int old_rate, const int new_rate)
{
	if (old_rate != new_rate)
		refresh_rate_change = true;
	else
		refresh_rate_change = false;
}

int oplus_display_set_power(struct drm_connector *connector,
		int power_mode, void *disp)
{
	struct dsi_display *display = disp;
	int rc = 0;

	if (!display || !display->panel) {
		OPLUS_DSI_ERR("display is null\n");
		return -EINVAL;
	}

	display->panel->oplus_panel.power_mode_early = power_mode;

	oplus_sync_power_state = power_mode;
	switch (power_mode) {
	case SDE_MODE_DPMS_LP1:
	case SDE_MODE_DPMS_LP2:
		OPLUS_DSI_INFO("SDE_MODE_DPMS_LP%d\n", power_mode == SDE_MODE_DPMS_LP1 ? 1 : 2);
#ifdef OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT
		if (oplus_ofp_is_supported()) {
			oplus_ofp_power_mode_handle(display, power_mode);
		}
#endif /* OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT */
		oplus_panel_event_data_notifier_trigger(display->panel, DRM_PANEL_EVENT_BLANK_LP, power_mode, true);
		break;

	case SDE_MODE_DPMS_ON:
		OPLUS_DSI_INFO("SDE_MODE_DPMS_ON\n");
		atomic_set(&display->panel->oplus_panel.esd_pending, 0);
#ifdef OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT
		if (oplus_ofp_is_supported()) {
			oplus_ofp_power_mode_handle(display, SDE_MODE_DPMS_ON);
		}
#endif /* OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT */
		break;

	case SDE_MODE_DPMS_OFF:
		OPLUS_DSI_INFO("SDE_MODE_DPMS_OFF\n");
		atomic_set(&display->panel->oplus_panel.esd_pending, 1);
#ifdef OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT
		if (oplus_ofp_is_supported()) {
			oplus_ofp_power_mode_handle(display, SDE_MODE_DPMS_OFF);
		}
#endif /* OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT */
		break;

	default:
		return rc;
	}

	OPLUS_DSI_DEBUG("Power mode transition from %d to %d %s\n",
			display->panel->power_mode, power_mode,
			rc ? "failed" : "successful");

	if (!rc) {
		display->panel->power_mode = power_mode;
	}

	return rc;
}
EXPORT_SYMBOL(oplus_display_set_power);

void oplus_display_set_display(void *display)
{
	struct dsi_display *dsi_display = display;

	if (!strcmp(dsi_display->display_type, "primary")) {
		primary_display = dsi_display;
		oplus_display_set_current_display(primary_display);
	} else {
		secondary_display = dsi_display;
	}

	return;
}

int oplus_panel_regs_validate(struct oplus_panel_regs_check_config *config)
{
	int i = 0, tmp = 0;
	u32 len = 0, cmd_count = 0;
	u8 *lenp = NULL;
	u32 data_offset = 0, group_offset = 0, value_offset = 0;
	u32 cmd_index = 0, data_index = 0, group_index = 0;
	u32 match_modes = 0, mode = 0;
	bool matched, group_mode0_matched, group_mode1_matched, group_matched;
	char payload[1024] = "";
	u32 cnt = 0;

	if (!(config->config & OPLUS_REGS_CHECK_ENABLE)) {
		OPLUS_DSI_DEBUG("regs check is disable, return\n");
		return -EINVAL;
	}

	match_modes = config->match_modes;
	lenp = config->check_regs_rlen;
	cmd_count = config->reg_count;

	for (i = 0; i < cmd_count; i++)
		len += lenp[i];

	group_matched = false;
	group_mode1_matched = true;
	for (group_index = 0, group_offset = 0; group_index < config->groups; ++group_index) {
		group_mode0_matched = true;

		for (cmd_index = 0, data_offset = 0; cmd_index < cmd_count; ++cmd_index) {
			mode = (match_modes >> cmd_index) & 0x01;
			tmp = 0;

			for (data_index = 0; data_index < lenp[cmd_index]; ++data_index) {
				matched = true;
				value_offset = group_offset + data_offset + data_index;

				if (!mode && config->return_buf[data_offset + data_index] !=
						config->check_value[value_offset]) {
					matched = false;
					group_mode0_matched = false;
				}
				else if (mode && config->return_buf[data_offset + data_index] ==
						config->check_value[value_offset]) {
					matched = false;
					tmp++;
				}

				OPLUS_DSI_DEBUG("check at index/group:[%d/%d] exp:[0x%02X] ret:[0x%02X] mode:[%u] matched:[%d]\n",
						data_offset + data_index,
						group_index,
						config->check_value[value_offset],
						config->return_buf[data_offset + data_index],
						mode,
						matched);
			}

			if (tmp == lenp[cmd_index])
					group_mode1_matched = false;

			data_offset += lenp[cmd_index];
		}
		group_matched = (group_matched || group_mode0_matched) && group_mode1_matched;

		OPLUS_DSI_DEBUG("check matching: group:[%d] mode0/mode1/matched:[%d/%d/%d]\n",
				group_index,
				group_mode0_matched,
				group_mode1_matched,
				group_matched);

		group_offset += len;
	}

	cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, "reg check result:");
	for (i = 0; i < len; ++i)
	cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, " [0x%02X]", config->return_buf[i]);

	OPLUS_DSI_INFO("%s; matched [%d]\n", payload, group_matched);

	if (group_matched)
		return 0;

	return -EINVAL;
}

int oplus_panel_regs_read(struct dsi_display_ctrl *ctrl,
		struct dsi_display *display, struct oplus_panel_regs_check_config *config)
{
	int i, rc = 0, ret = 0, start = 0;
	u32 enter_cmd = DSI_CMD_SET_MAX;
	u32 exit_cmd = DSI_CMD_SET_MAX;
	u8 *lenp = NULL;
	struct dsi_panel *panel;

	if (!display->panel || !ctrl || !ctrl->ctrl)
		return -EINVAL;

	panel = display->panel;

	/*
	 * When DSI controller is not in initialized state, we do not want to
	 * report a false MIPI ERR failure and hence we defer until next read
	 * happen.
	 */
	if (!dsi_ctrl_validate_host_state(ctrl->ctrl)) {
		OPLUS_DSI_INFO("dsi ctrl validate state!\n");
		return -EINVAL;
	}

	if (phy_pll_bypass(display)) {
		OPLUS_DSI_INFO("phy pll bypass failed!\n");
		return -EINVAL;
	}

	enter_cmd = config->enter_cmd;
	exit_cmd = config->exit_cmd;
	/* switch to enter cmd */
	if ((config->config & OPLUS_REGS_CHECK_PAGE_SWITCH) && (enter_cmd < DSI_CMD_SET_MAX)) {
		ret = dsi_panel_tx_cmd_set(panel, enter_cmd, false);
		if (ret) {
			OPLUS_DSI_ERR("[%s] failed to send enter cmd, rc=%d\n", panel->name, rc);
			return -EINVAL;
		}
	}
	lenp = config->check_regs_rlen;

	for (i = 0; i < config->reg_count; ++i) {
		rc = dsi_panel_read_panel_reg_unlock(ctrl, panel, config->check_regs[i],
				config->check_buf, config->check_regs_rlen[i]);
		if (rc < 0) {
			OPLUS_DSI_ERR("rx cmd transfer failed rc=%d\n", rc);
			return -EINVAL;
		} else {
			memcpy(config->return_buf + start,
				config->check_buf, lenp[i]);
			start += lenp[i];
		}
	}
	/* switch to exit cmd */
	if ((config->config & OPLUS_REGS_CHECK_PAGE_SWITCH) && (exit_cmd < DSI_CMD_SET_MAX)) {
		ret = dsi_panel_tx_cmd_set(panel, exit_cmd, false);
		if (ret) {
			OPLUS_DSI_ERR("[%s] failed to send exit cmd, rc=%d\n", panel->name, rc);
			return -EINVAL;
		}
	}

	return rc;
}

int oplus_panel_mipi_err_check(struct dsi_panel *panel)
{
	int rc = 0, i;
	struct dsi_display_ctrl *m_ctrl, *ctrl;
	struct dsi_display *display = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params");
		return -EINVAL;
	}

	if(!strcmp(panel->type, "primary")) {
		display = get_main_display();
	} else if (!strcmp(panel->type, "secondary")) {
		display = get_sec_display();
	} else {
		OPLUS_DSI_ERR("[DISP][ERR][%s:%d]dsi_display error\n", __func__, __LINE__);
		return -EINVAL;
	}

	m_ctrl = &display->ctrl[display->cmd_master_idx];

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			OPLUS_DSI_ERR("failed to allocate cmd tx buffer memory\n");
			rc = -EINVAL;
			goto done;
		}
	}

	rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.mipi_err_config);
	if (rc <= 0) {
		OPLUS_DSI_ERR("[%s] read status failed on master,rc=%d\n",
		       panel->name, rc);
		rc = -EINVAL;
		goto done;
	} else {
		rc = oplus_panel_regs_validate(&panel->oplus_panel.mipi_err_config);
	}

	if (!display->panel->sync_broadcast_en)
		goto done;

	display_for_each_ctrl(i, display) {
		ctrl = &display->ctrl[i];
		if (ctrl == m_ctrl)
			continue;

		rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.mipi_err_config);
		if (rc <= 0) {
			OPLUS_DSI_ERR("[%s] read status failed on slave,rc=%d\n",
				panel->name, rc);
			rc = -EINVAL;
			goto done;
		} else {
			rc = oplus_panel_regs_validate(&panel->oplus_panel.mipi_err_config);
		}
	}

done:
	return rc;
}

int oplus_panel_parse_mipi_err_reg_read_config(struct dsi_panel *panel)
{
	struct oplus_panel_regs_check_config *mipi_err_config;
	int rc = 0;
	u32 tmp = 0;
	u32 count = 0;
	u32 match_mode = 0;
	u32 i = 0, test_len = 0;
	u8 *lenp = NULL;
	struct property *data = NULL;
	struct dsi_parser_utils *utils = NULL;
	const u32 *arr = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	mipi_err_config = &panel->oplus_panel.mipi_err_config;

	if (!mipi_err_config) {
		OPLUS_DSI_ERR("Invalid mipi_err_config params\n");
		return -EINVAL;
	}

	arr = utils->get_property(utils->data, "oplus,mipi-err-check-reg", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,mipi-err-check-reg parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(mipi_err_config->check_regs, arr, count);
	mipi_err_config->reg_count = count;


	arr = utils->get_property(utils->data, "oplus,mipi-err-check-count", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,mipi-err-check-count parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(mipi_err_config->check_regs_rlen, arr, count);

	/*
	* oplus,mipi-err-check-match-modes is a 32-bit
	* binary flag. Bit value identified how to match the return
	* value of each register. The value 0(default) means equal,
	* and the value 1 means not equal.
	*/
	rc = utils->read_u32(utils->data, "oplus,mipi-err-check-match-modes", &match_mode);
	if (!rc) {
		mipi_err_config->match_modes = match_mode;
		OPLUS_DSI_INFO("Successed to read oplus,mipi-err-check-match-modes=0x%08X\n",
				mipi_err_config->match_modes);
	} else {
		mipi_err_config->match_modes = 0x0;
		OPLUS_DSI_ERR("Failed to read oplus,mipi-err-check-match-modes, set default modes=0x%08X\n",
				mipi_err_config->match_modes);
	}

	test_len = 0;
	lenp = mipi_err_config->check_regs_rlen;
	for (i = 0; i < count; ++i) {
		test_len += lenp[i];
	}
	if (!test_len) {
		rc = -EINVAL;
		goto error;
	}

	/*
	* Some panel may need multiple read commands to properly
	* check panel status. Do a sanity check for proper status
	* value which will be compared with the value read by dsi
	* controller during MIPI ERR check. Also check if multiple read
	* commands are there then, there should be corresponding
	* status check values for each read command.
	*/

	data = utils->find_property(utils->data,
			"oplus,mipi-err-check-value", &tmp);
	tmp /= sizeof(u8);
	if (!IS_ERR_OR_NULL(data) && tmp != 0 && (tmp % test_len) == 0) {
		mipi_err_config->groups = tmp / test_len;
	} else {
		OPLUS_DSI_ERR("error parse mipi_check_value!\n");
		rc = -EINVAL;
		goto error;
	}

	mipi_err_config->check_value =
		kzalloc(sizeof(u32) * test_len * mipi_err_config->groups,
			GFP_KERNEL);
	if (!mipi_err_config->check_value) {
		rc = -ENOMEM;
		goto error1;
	}

	mipi_err_config->return_buf = kcalloc(test_len * mipi_err_config->groups,
			sizeof(unsigned char), GFP_KERNEL);
	if (!mipi_err_config->return_buf) {
		rc = -ENOMEM;
		goto error2;
	}

	mipi_err_config->check_buf = kzalloc(SZ_4K, GFP_KERNEL);
	if (!mipi_err_config->check_buf) {
		rc = -ENOMEM;
		goto error3;
	}

	arr = utils->get_property(utils->data, "oplus,mipi-err-check-value", &count);
	if (!arr || (count != mipi_err_config->groups * test_len)) {
		OPLUS_DSI_ERR("error reading oplus,mipi-err-check-value\n");
		memset(mipi_err_config->check_value, 0, mipi_err_config->groups * test_len);
	}
	memcpy(mipi_err_config->check_value, arr, count);

	for (i = 0; i < mipi_err_config->groups * test_len; i++) {
		OPLUS_DSI_INFO("check_value[%d] == %d\n", i, (mipi_err_config->check_value)[i]);
	}

	return 0;

error3:
	kfree(mipi_err_config->check_buf);
error2:
	kfree(mipi_err_config->return_buf);
error1:
	kfree(mipi_err_config->check_value);
error:
	return rc;
}

int oplus_dsi_panel_parse_mipi_err(struct dsi_panel *panel)
{
	int rc = 0;
	struct oplus_panel_regs_check_config *mipi_err_config;
	struct dsi_parser_utils *utils = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	mipi_err_config = &panel->oplus_panel.mipi_err_config;
	rc = utils->read_u32(utils->data, "oplus,mipi-err-check-config",
				&mipi_err_config->config);
	if (rc) {
		OPLUS_DSI_ERR("failed to get oplus,mipi-err-check-config\n");
		/* default disable mipi err check */
		mipi_err_config->config = 0;
	}

	if (!(mipi_err_config->config & OPLUS_REGS_CHECK_ENABLE)) {
		OPLUS_DSI_INFO("mipi err check is disable!\n");
		goto error;
	}

	rc = oplus_panel_parse_mipi_err_reg_read_config(panel);
	if (rc) {
		OPLUS_DSI_ERR("failed to parse mipi err check reg params, rc = %d\n", rc);
		goto error;
	}
	mipi_err_config->enter_cmd = DSI_CMD_MIPI_ERR_CHECK_PAGE;
	mipi_err_config->exit_cmd = DSI_CMD_DEFAULT_SWITCH_PAGE;

	return 0;
error:
	panel->oplus_panel.mipi_err_config.config = false;
	mipi_err_config->enter_cmd = DSI_CMD_SET_MAX;
	mipi_err_config->exit_cmd = DSI_CMD_SET_MAX;

	return rc;
}

static int oplus_panel_gamma_compensation_read_reg(struct dsi_panel *panel, struct dsi_display_ctrl *m_ctrl,
				char regs[][GAMMA_COMPENSATION_READ_LENGTH], u8 value)
{
	int rc = 0;
	u32 cnt = 0;
	int index = 0;
	u8 cmd = GAMMA_COMPENSATION_BAND_REG;
	size_t replace_reg_len = 1;
	char replace_reg[REG_SIZE] = {0};
	char print_buf[OPLUS_DSI_CMD_PRINT_BUF_SIZE] = {0};

	memset(replace_reg, 0, sizeof(replace_reg));
	replace_reg[0] = value;
	rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, cmd, replace_reg, replace_reg_len);
	if (rc) {
		OPLUS_DSI_ERR("oplus panel cmd reg replace failed, retry\n");
		return rc;
	}
	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
	if (rc) {
		OPLUS_DSI_ERR("send DSI_CMD_GAMMA_COMPENSATION_PAGE1 failed, retry\n");
		return rc;
	}

	memset(regs[0], 0, GAMMA_COMPENSATION_READ_LENGTH);
	rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, GAMMA_COMPENSATION_READ_REG_11,
			regs[0], GAMMA_COMPENSATION_READ_LENGTH);
	if (rc < 0) {
		OPLUS_DSI_ERR("failed to read GAMMA_COMPENSATION_READ_REG rc=%d\n", rc);
		return rc;
	}
	cnt = 0;
	memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
	for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
		cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs[0][index]);
	}
	OPLUS_DSI_INFO("read regs0x%02X len=%d, buf=[%s]\n", value, GAMMA_COMPENSATION_READ_LENGTH, print_buf);

	memset(regs[1], 0, GAMMA_COMPENSATION_READ_LENGTH);
	rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, GAMMA_COMPENSATION_READ_REG_5,
			regs[1], GAMMA_COMPENSATION_READ_LENGTH);
	if (rc < 0) {
		OPLUS_DSI_ERR("failed to read GAMMA_COMPENSATION_READ_REG rc=%d\n", rc);
		return rc;
	}
	cnt = 0;
	memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
	for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
		cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs[1][index]);
	}
	OPLUS_DSI_INFO("read regs0x%02X len=%d, buf=[%s]\n", value, GAMMA_COMPENSATION_READ_LENGTH, print_buf);

	memset(regs[2], 0, GAMMA_COMPENSATION_READ_LENGTH);
	rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, GAMMA_COMPENSATION_READ_REG_3,
			regs[2], GAMMA_COMPENSATION_READ_LENGTH);
	if (rc < 0) {
		OPLUS_DSI_ERR("failed to read GAMMA_COMPENSATION_READ_REG rc=%d\n", rc);
		return rc;
	}
	cnt = 0;
	memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
	for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
		cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs[2][index]);
	}
	OPLUS_DSI_INFO("read regs0x%02X len=%d, buf=[%s]\n", value, GAMMA_COMPENSATION_READ_LENGTH, print_buf);

	return 0;
}

int oplus_display_panel_gamma_compensation(struct dsi_display *display)
{
	u32 retry_count = 0;
	u32 index = 0;
	int rc = 0;
	u32 cnt = 0;
	struct dsi_display_mode *mode = NULL;
	char print_buf[OPLUS_DSI_CMD_PRINT_BUF_SIZE] = {0};
	struct dsi_display_ctrl *m_ctrl = NULL;
	struct dsi_panel *panel = display->panel;
	char regs120[GAMMA_COMPENSATION_REG_CNT][GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90[GAMMA_COMPENSATION_REG_CNT][GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs120_last[GAMMA_COMPENSATION_REG_CNT][GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_last[GAMMA_COMPENSATION_REG_CNT][GAMMA_COMPENSATION_READ_LENGTH] = {0};
	const char reg_base[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	u32 gamma120_grey11[3];
	u32 gamma120_grey8[3];
	u32 gamma120_grey5[3];
	u32 gamma120_grey3[3];
	u32 gamma120_grey1[3];
	u32 gamma120_grey0[3];
	u32 gamma90_grey11[3];
	u32 gamma90_grey8[3];
	u32 gamma90_grey5[3];
	u32 gamma90_grey3[3];
	u32 gamma90_grey1[3];
	u32 gamma90_grey0[3];
	char regs120_grey8[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs120_grey5[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs120_grey3[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs120_grey1[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs120_grey0[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_grey8[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_grey5[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_grey3[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_grey1[GAMMA_COMPENSATION_READ_LENGTH] = {0};
	char regs90_grey0[GAMMA_COMPENSATION_READ_LENGTH] = {0};

	if (!panel) {
		OPLUS_DSI_ERR("panel is null\n");
		return  -EINVAL;
	}

	m_ctrl = &display->ctrl[display->cmd_master_idx];

	if (!m_ctrl) {
		OPLUS_DSI_ERR("ctrl is null\n");
		return -EINVAL;
	}

	if (!panel->oplus_panel.gamma_compensation_support) {
		OPLUS_DSI_INFO("panel gamma compensation isn't supported\n");
		return rc;
	}

	if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_ERR("display panel in off status\n");
		return -EINVAL;
	}
	if (!display->panel->panel_initialized) {
		OPLUS_DSI_ERR("panel initialized = false\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);
	while(!g_gamma_regs_read_done && retry_count < GAMMA_COMPENSATION_READ_RETRY_MAX) {
		OPLUS_DSI_INFO("read gamma compensation regs, retry_count=%d\n", retry_count);

		rc = oplus_panel_gamma_compensation_read_reg(panel, m_ctrl, regs120, GAMMA_COMPENSATION_BAND_120HZ);
		if (rc) {
			OPLUS_DSI_ERR("panel read regs120 failed\n");
			retry_count++;
			continue;
		}
		rc = oplus_panel_gamma_compensation_read_reg(panel, m_ctrl, regs90, GAMMA_COMPENSATION_BAND_90HZ);
		if (rc) {
			OPLUS_DSI_ERR("panel read regs90 failed\n");
			retry_count++;
			continue;
		}
		if (!memcmp(regs120[0], reg_base, sizeof(reg_base)) || !memcmp(regs90[0], reg_base, sizeof(reg_base)) ||
			 memcmp(regs120[0], regs120_last[0], sizeof(regs120_last[0])) || memcmp(regs90[0], regs90_last[0], sizeof(regs90_last[0])) ||
			 !memcmp(regs120[1], reg_base, sizeof(reg_base)) || !memcmp(regs90[1], reg_base, sizeof(reg_base)) ||
			 memcmp(regs120[1], regs120_last[1], sizeof(regs120_last[1])) || memcmp(regs90[1], regs90_last[1], sizeof(regs90_last[1])) ||
			 !memcmp(regs120[2], reg_base, sizeof(reg_base)) || !memcmp(regs90[2], reg_base, sizeof(reg_base)) ||
			 memcmp(regs120[2], regs120_last[2], sizeof(regs120_last[2])) || memcmp(regs90[2], regs90_last[2], sizeof(regs90_last[2]))) {
			OPLUS_DSI_WARN("gamma compensation regs is invalid, retry\n");
			memcpy(regs120_last[0], regs120[0], GAMMA_COMPENSATION_READ_LENGTH);
			memcpy(regs90_last[0], regs90[0], GAMMA_COMPENSATION_READ_LENGTH);
			memcpy(regs120_last[1], regs120[1], GAMMA_COMPENSATION_READ_LENGTH);
			memcpy(regs90_last[1], regs90[1], GAMMA_COMPENSATION_READ_LENGTH);
			memcpy(regs120_last[2], regs120[2], GAMMA_COMPENSATION_READ_LENGTH);
			memcpy(regs90_last[2], regs90[2], GAMMA_COMPENSATION_READ_LENGTH);
			retry_count++;
			continue;
		}

		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
		if (rc) {
			OPLUS_DSI_ERR("send DSI_CMD_GAMMA_COMPENSATION_PAGE0 failed\n");
		}

		g_gamma_regs_read_done = true;
		OPLUS_DSI_INFO("gamma compensation read success");
		break;
	}
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	if (!g_gamma_regs_read_done) {
		return -EFAULT;
	}

	for (index = 0; index < (GAMMA_COMPENSATION_READ_LENGTH - 1); index = index+2) {
		gamma120_grey11[index / 2] = regs120[0][index] << 8 | regs120[0][index+1];
		gamma120_grey5[index / 2] = regs120[1][index] << 8 | regs120[1][index+1];
		gamma120_grey3[index / 2] = regs120[2][index] << 8 | regs120[2][index+1];
		gamma90_grey11[index / 2] = regs90[0][index] << 8 | regs90[0][index+1];
		gamma90_grey5[index / 2] = regs90[1][index] << 8 | regs90[1][index+1];
		gamma90_grey3[index / 2] = regs90[2][index] << 8 | regs90[2][index+1];
	}

	OPLUS_DSI_INFO("gamma120_grey11 = [%3d %3d %3d]", gamma120_grey11[0], gamma120_grey11[1], gamma120_grey11[2]);
	OPLUS_DSI_INFO("gamma120_grey5 = [%3d %3d %3d]", gamma120_grey5[0], gamma120_grey5[1], gamma120_grey5[2]);
	OPLUS_DSI_INFO("gamma120_grey3 = [%3d %3d %3d]", gamma120_grey3[0], gamma120_grey3[1], gamma120_grey3[2]);
	OPLUS_DSI_INFO("gamma90_grey11 = [%3d %3d %3d]", gamma90_grey11[0], gamma90_grey11[1], gamma90_grey11[2]);
	OPLUS_DSI_INFO("gamma90_grey5 = [%3d %3d %3d]", gamma90_grey5[0], gamma90_grey5[1], gamma90_grey5[2]);
	OPLUS_DSI_INFO("gamma90_grey3 = [%3d %3d %3d]", gamma90_grey3[0], gamma90_grey3[1], gamma90_grey3[2]);

	if ((gamma120_grey5[0] < gamma120_grey3[0]) || (gamma120_grey5[1] < gamma120_grey3[1]) || (gamma120_grey5[2] < gamma120_grey3[2]) ||
		(gamma90_grey5[0] < gamma90_grey3[0]) || (gamma90_grey5[1] < gamma90_grey3[1]) || (gamma90_grey5[2] < gamma90_grey3[2])) {
		g_gamma_inverse = true;
		gamma120_grey8[0] = gamma120_grey11[0] * GAMMA_COMPENSATION_120HZ_GREY8_R;
		gamma120_grey8[1] = gamma120_grey11[1] * GAMMA_COMPENSATION_120HZ_GREY8_G;
		gamma120_grey8[2] = gamma120_grey11[2] * GAMMA_COMPENSATION_120HZ_GREY8_B;
		gamma120_grey5[0] = gamma120_grey11[0] * GAMMA_COMPENSATION_120HZ_GREY5_R;
		gamma120_grey5[1] = gamma120_grey11[1] * GAMMA_COMPENSATION_120HZ_GREY5_G;
		gamma120_grey5[2] = gamma120_grey11[2] * GAMMA_COMPENSATION_120HZ_GREY5_B;
		gamma120_grey3[0] = gamma120_grey11[0] * GAMMA_COMPENSATION_120HZ_GREY3_R;
		gamma120_grey3[1] = gamma120_grey11[1] * GAMMA_COMPENSATION_120HZ_GREY3_G;
		gamma120_grey3[2] = gamma120_grey11[2] * GAMMA_COMPENSATION_120HZ_GREY3_B;
		gamma120_grey1[0] = gamma120_grey11[0] * GAMMA_COMPENSATION_120HZ_GREY1_R;
		gamma120_grey1[1] = gamma120_grey11[1] * GAMMA_COMPENSATION_120HZ_GREY1_G;
		gamma120_grey1[2] = gamma120_grey11[2] * GAMMA_COMPENSATION_120HZ_GREY1_B;
		gamma120_grey0[0] = gamma120_grey11[0] * GAMMA_COMPENSATION_120HZ_GREY0_R;
		gamma120_grey0[1] = gamma120_grey11[1] * GAMMA_COMPENSATION_120HZ_GREY0_G;
		gamma120_grey0[2] = gamma120_grey11[2] * GAMMA_COMPENSATION_120HZ_GREY0_B;
		gamma90_grey8[0] = gamma90_grey11[0] * GAMMA_COMPENSATION_90HZ_GREY8_R;
		gamma90_grey8[1] = gamma90_grey11[1] * GAMMA_COMPENSATION_90HZ_GREY8_G;
		gamma90_grey8[2] = gamma90_grey11[2] * GAMMA_COMPENSATION_90HZ_GREY8_B;
		gamma90_grey5[0] = gamma90_grey11[0] * GAMMA_COMPENSATION_90HZ_GREY5_R;
		gamma90_grey5[1] = gamma90_grey11[1] * GAMMA_COMPENSATION_90HZ_GREY5_G;
		gamma90_grey5[2] = gamma90_grey11[2] * GAMMA_COMPENSATION_90HZ_GREY5_B;
		gamma90_grey3[0] = gamma90_grey11[0] * GAMMA_COMPENSATION_90HZ_GREY3_R;
		gamma90_grey3[1] = gamma90_grey11[1] * GAMMA_COMPENSATION_90HZ_GREY3_G;
		gamma90_grey3[2] = gamma90_grey11[2] * GAMMA_COMPENSATION_90HZ_GREY3_B;
		gamma90_grey1[0] = gamma90_grey11[0] * GAMMA_COMPENSATION_90HZ_GREY1_R;
		gamma90_grey1[1] = gamma90_grey11[1] * GAMMA_COMPENSATION_90HZ_GREY1_G;
		gamma90_grey1[2] = gamma90_grey11[2] * GAMMA_COMPENSATION_90HZ_GREY1_B;
		gamma90_grey0[0] = gamma90_grey11[0] * GAMMA_COMPENSATION_90HZ_GREY0_R;
		gamma90_grey0[1] = gamma90_grey11[1] * GAMMA_COMPENSATION_90HZ_GREY0_G;
		gamma90_grey0[2] = gamma90_grey11[2] * GAMMA_COMPENSATION_90HZ_GREY0_B;
		OPLUS_DSI_INFO("g_gamma_inverse = %d", g_gamma_inverse);
		OPLUS_DSI_INFO("gamma120_grey8 = [%3d %3d %3d]", gamma120_grey8[0], gamma120_grey8[1], gamma120_grey8[2]);
		OPLUS_DSI_INFO("gamma120_grey5 = [%3d %3d %3d]", gamma120_grey5[0], gamma120_grey5[1], gamma120_grey5[2]);
		OPLUS_DSI_INFO("gamma120_grey3 = [%3d %3d %3d]", gamma120_grey3[0], gamma120_grey3[1], gamma120_grey3[2]);
		OPLUS_DSI_INFO("gamma120_grey1 = [%3d %3d %3d]", gamma120_grey1[0], gamma120_grey1[1], gamma120_grey1[2]);
		OPLUS_DSI_INFO("gamma120_grey0 = [%3d %3d %3d]", gamma120_grey0[0], gamma120_grey0[1], gamma120_grey0[2]);
		OPLUS_DSI_INFO("gamma90_grey8 = [%3d %3d %3d]", gamma90_grey8[0], gamma90_grey8[1], gamma90_grey8[2]);
		OPLUS_DSI_INFO("gamma90_grey5 = [%3d %3d %3d]", gamma90_grey5[0], gamma90_grey5[1], gamma90_grey5[2]);
		OPLUS_DSI_INFO("gamma90_grey3 = [%3d %3d %3d]", gamma90_grey3[0], gamma90_grey3[1], gamma90_grey3[2]);
		OPLUS_DSI_INFO("gamma90_grey1 = [%3d %3d %3d]", gamma90_grey1[0], gamma90_grey1[1], gamma90_grey1[2]);
		OPLUS_DSI_INFO("gamma90_grey0 = [%3d %3d %3d]", gamma90_grey0[0], gamma90_grey0[1], gamma90_grey0[2]);

		for (index = 0; index < (GAMMA_COMPENSATION_READ_LENGTH - 1); index = index+2) {
			regs120_grey8[index] = gamma120_grey8[index / 2] >> 8 & 0xFF;
			regs120_grey8[index + 1] = gamma120_grey8[index / 2] & 0xFF;
			regs120_grey5[index] = gamma120_grey5[index / 2] >> 8 & 0xFF;
			regs120_grey5[index + 1] = gamma120_grey5[index / 2] & 0xFF;
			regs120_grey3[index] = gamma120_grey3[index / 2] >> 8 & 0xFF;
			regs120_grey3[index + 1] = gamma120_grey3[index / 2] & 0xFF;
			regs120_grey1[index] = gamma120_grey1[index / 2] >> 8 & 0xFF;
			regs120_grey1[index + 1] = gamma120_grey1[index / 2] & 0xFF;
			regs120_grey0[index] = gamma120_grey0[index / 2] >> 8 & 0xFF;
			regs120_grey0[index + 1] = gamma120_grey0[index / 2] & 0xFF;
			regs90_grey8[index] = gamma90_grey8[index / 2] >> 8 & 0xFF;
			regs90_grey8[index + 1] = gamma90_grey8[index / 2] & 0xFF;
			regs90_grey5[index] = gamma90_grey5[index / 2] >> 8 & 0xFF;
			regs90_grey5[index + 1] = gamma90_grey5[index / 2] & 0xFF;
			regs90_grey3[index] = gamma90_grey3[index / 2] >> 8 & 0xFF;
			regs90_grey3[index + 1] = gamma90_grey3[index / 2] & 0xFF;
			regs90_grey1[index] = gamma90_grey1[index / 2] >> 8 & 0xFF;
			regs90_grey1[index + 1] = gamma90_grey1[index / 2] & 0xFF;
			regs90_grey0[index] = gamma90_grey0[index / 2] >> 8 & 0xFF;
			regs90_grey0[index + 1] = gamma90_grey0[index / 2] & 0xFF;
		}

		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs120_grey8[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_8,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);

		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs120_grey5[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_5,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);
		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs120_grey3[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_3,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);

		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs120_grey1[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_1,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);
		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs120_grey0[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_0,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);
		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs90_grey8[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_8,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);

		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs90_grey5[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_5,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);
		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs90_grey3[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_3,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);

		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs90_grey1[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_1,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);
		cnt = 0;
		memset(print_buf, 0, OPLUS_DSI_CMD_PRINT_BUF_SIZE);
		for (index = 0; index < GAMMA_COMPENSATION_READ_LENGTH; index++) {
			cnt += snprintf(print_buf + cnt, OPLUS_DSI_CMD_PRINT_BUF_SIZE - cnt, "%02X ", regs90_grey0[index]);
		}
		OPLUS_DSI_INFO("compensation regs0x%02X len=%d, buf=[%s]\n", GAMMA_COMPENSATION_READ_REG_0,
				GAMMA_COMPENSATION_READ_LENGTH, print_buf);

		mutex_lock(&display->display_lock);
		mutex_lock(&display->panel->panel_lock);
		for (index = 0; index < display->panel->num_display_modes; index++) {
			mode = &display->modes[index];
			if (!mode) {
				OPLUS_DSI_INFO("mode is null\n");
				continue;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs120_grey8,
				GAMMA_COMPENSATION_READ_LENGTH, 3);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey8 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs120_grey5,
					GAMMA_COMPENSATION_READ_LENGTH, 4);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey5 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
		    rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs120_grey3,
				GAMMA_COMPENSATION_READ_LENGTH, 5);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey3 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs120_grey1,
					GAMMA_COMPENSATION_READ_LENGTH, 6);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey1 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
		    rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs120_grey0,
					GAMMA_COMPENSATION_READ_LENGTH, 7);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey0 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs90_grey8,
				GAMMA_COMPENSATION_READ_LENGTH, 10);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey8 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs90_grey5,
					GAMMA_COMPENSATION_READ_LENGTH, 11);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey5 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs90_grey3,
				GAMMA_COMPENSATION_READ_LENGTH, 12);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey3 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs90_grey1,
					GAMMA_COMPENSATION_READ_LENGTH, 13);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey1 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION, regs90_grey0,
					GAMMA_COMPENSATION_READ_LENGTH, 14);
			if (rc) {
				OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION gamma120_grey0 replace failed\n");
				g_gamma_regs_read_done = false;
				return -EFAULT;
			}
			OPLUS_DSI_INFO("display mode%d had completed gamma compensation\n", index);
		}
		mutex_unlock(&display->panel->panel_lock);
		mutex_unlock(&display->display_lock);
	}

	return rc;
}

int oplus_panel_pcd_check(struct dsi_panel *panel)
{
	int rc = 0, i;
	struct dsi_display_ctrl *m_ctrl, *ctrl;
	struct dsi_display *display = NULL;
	struct oplus_panel_regs_check_config *config;
	u32 len = 0, reg_count = 0;
	char payload[OPLUS_DSI_CMD_PRINT_BUF_SIZE] = "";
	u8 *lenp;
	u32 cnt = 0;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params");
		return -EINVAL;
	}

	if(!strcmp(panel->type, "primary")) {
		display = get_main_display();
	} else if (!strcmp(panel->type, "secondary")) {
		display = get_sec_display();
	} else {
		OPLUS_DSI_ERR("[DISP][ERR][%s:%d]dsi_display error\n", __func__, __LINE__);
		return -EINVAL;
	}

	m_ctrl = &display->ctrl[display->cmd_master_idx];
	config = &panel->oplus_panel.pcd_config;

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			OPLUS_DSI_ERR("failed to allocate cmd tx buffer memory\n");
			rc = -EINVAL;
			goto done;
		}
	}

	rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.pcd_config);
	if (rc <= 0) {
		OPLUS_DSI_ERR("[%s] read status failed on master,rc=%d\n",
		       panel->name, rc);
		rc = -EINVAL;
		goto done;
	} else {
		rc = oplus_panel_regs_validate(&panel->oplus_panel.pcd_config);
		if (rc) {
			OPLUS_DSI_ERR("regs validate failed, rc=%d\n", rc);
			lenp = config->check_regs_rlen;
			reg_count = config->reg_count;
			for (i = 0; i < reg_count; i++) {
				len += lenp[i];
			}
			for (i = 0; i < len; ++i) {
				cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, "%02X ", config->return_buf[i]);
			}
#ifdef OPLUS_TRACKPOINT_REPORT
			EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$ panel pcd error rc:%s\n",
					OPLUS_DISP_Q_ERROR_PCD_CHECK_FAIL, payload);
#endif /* OPLUS_TRACKPOINT_REPORT */
			return rc;
		}
	}

	if (!display->panel->sync_broadcast_en)
		goto done;

	display_for_each_ctrl(i, display) {
		ctrl = &display->ctrl[i];
		if (ctrl == m_ctrl)
			continue;

		rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.pcd_config);
		if (rc <= 0) {
			OPLUS_DSI_ERR("[%s] read status failed on slave,rc=%d\n",
				panel->name, rc);
			rc = -EINVAL;
			goto done;
		} else {
			rc = oplus_panel_regs_validate(&panel->oplus_panel.pcd_config);
			if (rc) {
				OPLUS_DSI_ERR("regs validate failed, rc=%d\n", rc);
				lenp = config->check_regs_rlen;
				reg_count = config->reg_count;
				for (i = 0; i < reg_count; i++) {
					len += lenp[i];
				}
				for (i = 0; i < len; ++i) {
					cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, "%02X ", config->return_buf[i]);
				}
#ifdef OPLUS_TRACKPOINT_REPORT
				EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$ panel pcd error rc:%s\n",
						OPLUS_DISP_Q_ERROR_PCD_CHECK_FAIL, payload);
#endif /* OPLUS_TRACKPOINT_REPORT */
				return rc;
			}
		}
	}

done:
	return rc;
}

int oplus_panel_lvd_check(struct dsi_panel *panel)
{
	int rc = 0, i;
	struct dsi_display_ctrl *m_ctrl, *ctrl;
	struct dsi_display *display = NULL;
	struct oplus_panel_regs_check_config *config;
	u32 len = 0, reg_count = 0;
	char payload[OPLUS_DSI_CMD_PRINT_BUF_SIZE] = "";
	u8 *lenp;
	u32 cnt = 0;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params");
		return -EINVAL;
	}

	if(!strcmp(panel->type, "primary")) {
		display = get_main_display();
	} else if (!strcmp(panel->type, "secondary")) {
		display = get_sec_display();
	} else {
		OPLUS_DSI_ERR("[DISP][ERR][%s:%d]dsi_display error\n", __func__, __LINE__);
		return -EINVAL;
	}

	config = &panel->oplus_panel.lvd_config;
	m_ctrl = &display->ctrl[display->cmd_master_idx];

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			OPLUS_DSI_ERR("failed to allocate cmd tx buffer memory\n");
			rc = -EINVAL;
			goto done;
		}
	}

	rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.lvd_config);
	if (rc <= 0) {
		OPLUS_DSI_ERR("[%s] read status failed on master,rc=%d\n",
		       panel->name, rc);
		rc = -EINVAL;
		goto done;
	} else {
		rc = oplus_panel_regs_validate(&panel->oplus_panel.lvd_config);
		if (rc) {
			OPLUS_DSI_ERR("regs validate failed, rc=%d\n", rc);
			lenp = config->check_regs_rlen;
			reg_count = config->reg_count;
			for (i = 0; i < reg_count; i++) {
				len += lenp[i];
			}
			for (i = 0; i < len; ++i) {
				cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, "%02X ", config->return_buf[i]);
			}
#ifdef OPLUS_TRACKPOINT_REPORT
			EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$ panel lvd error rc:%s\n",
					OPLUS_DISP_Q_ERROR_LVD_CHECK_FAIL, payload);
#endif /* OPLUS_TRACKPOINT_REPORT */
			return rc;
		}
	}

	if (!display->panel->sync_broadcast_en)
		goto done;

	display_for_each_ctrl(i, display) {
		ctrl = &display->ctrl[i];
		if (ctrl == m_ctrl)
			continue;

		rc = oplus_panel_regs_read(m_ctrl, display, &panel->oplus_panel.lvd_config);
		if (rc <= 0) {
			OPLUS_DSI_ERR("[%s] read status failed on slave,rc=%d\n",
				panel->name, rc);
			rc = -EINVAL;
			goto done;
		} else {
			rc = oplus_panel_regs_validate(&panel->oplus_panel.lvd_config);
			if (rc) {
				OPLUS_DSI_ERR("regs validate failed, rc=%d\n", rc);
				lenp = config->check_regs_rlen;
				reg_count = config->reg_count;
				for (i = 0; i < reg_count; i++) {
					len += lenp[i];
				}
				for (i = 0; i < len; ++i) {
					cnt += scnprintf(payload + cnt, sizeof(payload) - cnt, "%02X ", config->return_buf[i]);
				}
#ifdef OPLUS_TRACKPOINT_REPORT
				EXCEPTION_TRACKPOINT_REPORT("DisplayDriverID@@%d$$ panel lvd error rc:%s\n",
						OPLUS_DISP_Q_ERROR_LVD_CHECK_FAIL, payload);
#endif /* OPLUS_TRACKPOINT_REPORT */
				return rc;
			}
		}
	}

done:
	return rc;
}

void oplus_panel_pl_check_enable(struct dsi_panel *panel)
{
	ktime_t time_gap = 0;
	u64 time_gap_hours;
	static ktime_t panel_off_time = 0;

	time_gap = ktime_to_ms(ktime_sub(ktime_get(), panel_off_time));
	time_gap_hours = time_gap / (3600LL * 1000LL);
	if (time_gap_hours >= panel->oplus_panel.pl_check_time_gap) {
		panel->oplus_panel.pl_check_flag = true;
	}

	if (panel->oplus_panel.pl_check_flag) {
		OPLUS_DSI_INFO("panel pcd and lvd check\n");
		mutex_lock(&panel->panel_lock);
		oplus_panel_pcd_check(panel);
		oplus_panel_lvd_check(panel);
		mutex_unlock(&panel->panel_lock);
		panel_off_time = ktime_get();
		panel->oplus_panel.pl_check_flag = false;
	}

	return;
}

int oplus_panel_pl_check_state(struct dsi_panel *panel)
{
	if (!panel->oplus_panel.pl_check_enable) {
		OPLUS_DSI_DEBUG("panel pcd and lvd check isn't supported\n");
		return 0;
	}

	if (oplus_ofp_get_aod_state()) {
		OPLUS_DSI_INFO("in aod doze mode, skip pcd and lvd check\n");
		return 0;
	}

	oplus_panel_pl_check_enable(panel);

	return 0;
}

int oplus_panel_parse_pcd_reg_read_config(struct dsi_panel *panel)
{
	struct oplus_panel_regs_check_config *pcd_config;
	int rc = 0;
	u32 tmp = 0;
	u32 count = 0;
	u32 match_mode = 0;
	u32 i = 0, test_len = 0;
	u8 *lenp = NULL;
	struct property *data = NULL;
	struct dsi_parser_utils *utils = NULL;
	const u32 *arr = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	pcd_config = &panel->oplus_panel.pcd_config;

	if (!pcd_config) {
		OPLUS_DSI_ERR("Invalid pcd_config params\n");
		return -EINVAL;
	}

	arr = utils->get_property(utils->data, "oplus,pcd-check-reg", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,pcd-check-reg parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(pcd_config->check_regs, arr, count);
	pcd_config->reg_count = count;


	arr = utils->get_property(utils->data, "oplus,pcd-check-count", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,pcd-check-count parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(pcd_config->check_regs_rlen, arr, count);

	rc = utils->read_u32(utils->data, "oplus,pcd-check-match-modes", &match_mode);
	if (!rc) {
		pcd_config->match_modes = match_mode;
		OPLUS_DSI_INFO("Successed to read oplus,pcd-check-match-modes=0x%08X\n",
				pcd_config->match_modes);
	} else {
		pcd_config->match_modes = 0x0;
		OPLUS_DSI_ERR("Failed to read oplus,pcd-check-match-modes, set default modes=0x%08X\n",
				pcd_config->match_modes);
	}

	test_len = 0;
	lenp = pcd_config->check_regs_rlen;
	for (i = 0; i < count; ++i) {
		test_len += lenp[i];
	}
	if (!test_len) {
		rc = -EINVAL;
		goto error;
	}

	data = utils->find_property(utils->data,
			"oplus,pcd-check-value", &tmp);
	tmp /= sizeof(u8);
	if (!IS_ERR_OR_NULL(data) && tmp != 0 && (tmp % test_len) == 0) {
		pcd_config->groups = tmp / test_len;
	} else {
		OPLUS_DSI_ERR("error parse oplus,pcd-check-value!\n");
		rc = -EINVAL;
		goto error;
	}

	pcd_config->check_value =
		kzalloc(sizeof(u32) * test_len * pcd_config->groups,
			GFP_KERNEL);
	if (!pcd_config->check_value) {
		rc = -ENOMEM;
		goto error1;
	}

	pcd_config->return_buf = kcalloc(test_len * pcd_config->groups,
			sizeof(unsigned char), GFP_KERNEL);
	if (!pcd_config->return_buf) {
		rc = -ENOMEM;
		goto error2;
	}

	pcd_config->check_buf = kzalloc(SZ_4K, GFP_KERNEL);
	if (!pcd_config->check_buf) {
		rc = -ENOMEM;
		goto error3;
	}

	arr = utils->get_property(utils->data, "oplus,pcd-check-value", &count);
	if (!arr || (count != pcd_config->groups * test_len)) {
		OPLUS_DSI_ERR("error reading oplus,pcd-check-value\n");
		memset(pcd_config->check_value, 0, pcd_config->groups * test_len);
	}
	memcpy(pcd_config->check_value, arr, count);

	for (i = 0; i < pcd_config->groups * test_len; i++) {
		OPLUS_DSI_INFO("check_value[%d] == %d\n", i, (pcd_config->check_value)[i]);
	}

	return 0;

error3:
	kfree(pcd_config->check_buf);
error2:
	kfree(pcd_config->return_buf);
error1:
	kfree(pcd_config->check_value);
error:
	return rc;
}

int oplus_dsi_panel_parse_pcd(struct dsi_panel *panel)
{
	int rc = 0;
	struct oplus_panel_regs_check_config *pcd_config;
	struct dsi_parser_utils *utils = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	pcd_config = &panel->oplus_panel.pcd_config;
	rc = utils->read_u32(utils->data, "oplus,pcd-check-config",
				&pcd_config->config);
	if (rc) {
		OPLUS_DSI_ERR("failed to get oplus,pcd-check-config\n");
		/* default disable pcd check */
		pcd_config->config = 0;
	}

	if (!(pcd_config->config & OPLUS_REGS_CHECK_ENABLE)) {
		OPLUS_DSI_INFO("pcd check is disable!\n");
		goto error;
	}

	rc = oplus_panel_parse_pcd_reg_read_config(panel);
	if (rc) {
		OPLUS_DSI_ERR("failed to parse pcd check reg params, rc = %d\n", rc);
		goto error;
	}
	pcd_config->enter_cmd = DSI_CMD_PCD_CHECK_ENTER;
	pcd_config->exit_cmd = DSI_CMD_PCD_CHECK_EXIT;

	return 0;
error:
	panel->oplus_panel.pcd_config.config = false;
	pcd_config->enter_cmd = DSI_CMD_SET_MAX;
	pcd_config->exit_cmd = DSI_CMD_SET_MAX;

	return rc;
}

int oplus_panel_parse_lvd_reg_read_config(struct dsi_panel *panel)
{
	struct oplus_panel_regs_check_config *lvd_config;
	int rc = 0;
	u32 tmp = 0;
	u32 count = 0;
	u32 match_mode = 0;
	u32 i = 0, test_len = 0;
	u8 *lenp = NULL;
	struct property *data = NULL;
	struct dsi_parser_utils *utils = NULL;
	const u32 *arr = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	lvd_config = &panel->oplus_panel.lvd_config;

	if (!lvd_config) {
		OPLUS_DSI_ERR("Invalid lvd_config params\n");
		return -EINVAL;
	}

	arr = utils->get_property(utils->data, "oplus,lvd-check-reg", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,lvd-check-reg parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(lvd_config->check_regs, arr, count);
	lvd_config->reg_count = count;

	arr = utils->get_property(utils->data, "oplus,lvd-check-count", &count);
	if (!arr) {
		OPLUS_DSI_ERR("oplus,pcd-check-count parsing failed!\n");
		rc = -EINVAL;
		goto error;
	}
	memcpy(lvd_config->check_regs_rlen, arr, count);

	rc = utils->read_u32(utils->data, "oplus,lvd-check-match-modes", &match_mode);
	if (!rc) {
		lvd_config->match_modes = match_mode;
		OPLUS_DSI_INFO("Successed to read oplus,lvd-check-match-modes=0x%08X\n",
				lvd_config->match_modes);
	} else {
		lvd_config->match_modes = 0x0;
		OPLUS_DSI_ERR("Failed to read oplus,lvd-check-match-modes, set default modes=0x%08X\n",
				lvd_config->match_modes);
	}

	test_len = 0;
	lenp = lvd_config->check_regs_rlen;
	for (i = 0; i < count; ++i) {
		test_len += lenp[i];
	}
	if (!test_len) {
		rc = -EINVAL;
		goto error;
	}

	data = utils->find_property(utils->data,
			"oplus,lvd-check-value", &tmp);
	tmp /= sizeof(u8);
	if (!IS_ERR_OR_NULL(data) && tmp != 0 && (tmp % test_len) == 0) {
		lvd_config->groups = tmp / test_len;
	} else {
		OPLUS_DSI_ERR("error parse oplus,lvd-check-value!\n");
		rc = -EINVAL;
		goto error;
	}

	lvd_config->check_value =
		kzalloc(sizeof(u32) * test_len * lvd_config->groups,
			GFP_KERNEL);
	if (!lvd_config->check_value) {
		rc = -ENOMEM;
		goto error1;
	}

	lvd_config->return_buf = kcalloc(test_len * lvd_config->groups,
			sizeof(unsigned char), GFP_KERNEL);
	if (!lvd_config->return_buf) {
		rc = -ENOMEM;
		goto error2;
	}

	lvd_config->check_buf = kzalloc(SZ_4K, GFP_KERNEL);
	if (!lvd_config->check_buf) {
		rc = -ENOMEM;
		goto error3;
	}

	arr = utils->get_property(utils->data, "oplus,lvd-check-value", &count);
	if (!arr || (count != lvd_config->groups * test_len)) {
		OPLUS_DSI_ERR("error reading oplus,lvd-check-value\n");
		memset(lvd_config->check_value, 0, lvd_config->groups * test_len);
	}
	memcpy(lvd_config->check_value, arr, count);

	for (i = 0; i < lvd_config->groups * test_len; i++) {
		OPLUS_DSI_INFO("check_value[%d] == %d\n", i, (lvd_config->check_value)[i]);
	}

	return 0;

error3:
	kfree(lvd_config->check_buf);
error2:
	kfree(lvd_config->return_buf);
error1:
	kfree(lvd_config->check_value);
error:
	return rc;
}

int oplus_dsi_panel_parse_lvd(struct dsi_panel *panel)
{
	int rc = 0;
	struct oplus_panel_regs_check_config *lvd_config;
	struct dsi_parser_utils *utils = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	lvd_config = &panel->oplus_panel.lvd_config;
	rc = utils->read_u32(utils->data, "oplus,lvd-check-config",
				&lvd_config->config);
	if (rc) {
		OPLUS_DSI_ERR("failed to get oplus,lvd-check-config\n");
		/* default disable lvd check */
		lvd_config->config = 0;
	}

	if (!(lvd_config->config & OPLUS_REGS_CHECK_ENABLE)) {
		OPLUS_DSI_INFO("lvd check is disable!\n");
		goto error;
	}

	rc = oplus_panel_parse_lvd_reg_read_config(panel);
	if (rc) {
		OPLUS_DSI_ERR("failed to parse lvd check reg params, rc = %d\n", rc);
		goto error;
	}
	lvd_config->enter_cmd = DSI_CMD_LVD_CHECK_ENTER;
	lvd_config->exit_cmd = DSI_CMD_LVD_CHECK_EXIT;

	return 0;
error:
	panel->oplus_panel.lvd_config.config = false;
	lvd_config->enter_cmd = DSI_CMD_SET_MAX;
	lvd_config->exit_cmd = DSI_CMD_SET_MAX;

	return rc;
}

extern int sde_encoder_resource_control(struct drm_encoder *drm_enc, u32 sw_event);

int oplus_sde_early_wakeup(struct dsi_panel *panel)
{
	struct dsi_display *d_display = get_main_display();
	struct drm_encoder *drm_enc;

	if(!strcmp(panel->type, "secondary")) {
		d_display = get_sec_display();
	}

	if (!d_display || !d_display->bridge) {
		DSI_ERR("invalid display params\n");
		return -EINVAL;
	}
	drm_enc = d_display->bridge->base.encoder;
	if (!drm_enc) {
		DSI_ERR("invalid encoder params\n");
		return -EINVAL;
	}
	sde_encoder_resource_control(drm_enc,
			7 /*SDE_ENC_RC_EVENT_EARLY_WAKEUP*/);
	return 0;
}

int oplus_display_wait_for_event(struct dsi_display *display,
		enum msm_event_wait event)
{
	int rc = 0;
	char tag_name[64] = {0};
	struct drm_encoder *drm_enc = NULL;

	if (!display || !display->bridge) {
		OPLUS_DSI_INFO("invalid display params\n");
		return -ENODEV;
	}

	if (display->panel->power_mode != SDE_MODE_DPMS_ON || !display->panel->panel_initialized) {
		OPLUS_DSI_INFO("display panel in off status\n");
		return -ENODEV;
	}
	drm_enc = display->bridge->base.encoder;

	if (!drm_enc) {
		OPLUS_DSI_INFO("invalid encoder params\n");
		return -ENODEV;
	}

	if (sde_encoder_is_disabled(drm_enc)) {
		OPLUS_DSI_INFO("%s encoder is disabled\n", __func__);
		return -ENODEV;
	}

	sde_encoder_resource_control(drm_enc,
			7 /* SDE_ENC_RC_EVENT_EARLY_WAKEUP */);

	snprintf(tag_name, sizeof(tag_name), "oplus_display_wait_for_event_%d", event);
	SDE_ATRACE_BEGIN(tag_name);
	sde_encoder_wait_for_event(drm_enc, event);
	SDE_ATRACE_END(tag_name);

	return rc;
}
EXPORT_SYMBOL(oplus_display_wait_for_event);

void oplus_save_te_timestamp(struct sde_connector *c_conn, ktime_t timestamp)
{
	struct dsi_display *display = c_conn->display;
	if (!display || !display->panel)
		return;
	display->panel->oplus_panel.te_timestamp = timestamp;
}

void oplus_need_to_sync_te(struct dsi_panel *panel)
{
	s64 us_per_frame;
	u32 vsync_width;
	ktime_t last_te_timestamp;
	int delay;
	int left_time;

	us_per_frame = panel->cur_mode->priv_info->oplus_priv_info.vsync_period;
	vsync_width = panel->cur_mode->priv_info->oplus_priv_info.vsync_width;
	/* add 700us to vsync width(first half of frame time) to
	 * 1. avoid command sent in the middle of TE cycle
	 * 2. compensate the TE shift period */
	vsync_width += 700;
	last_te_timestamp = panel->oplus_panel.te_timestamp;

	SDE_ATRACE_BEGIN("oplus_need_to_sync_te");
	delay = vsync_width - (ktime_to_us(ktime_sub(ktime_get(), last_te_timestamp)) % us_per_frame);
	if (delay > 0) {
		SDE_EVT32(us_per_frame, last_te_timestamp, delay);
		usleep_range(delay, delay + 100);
	} else {
		/* detect the left time for command sending in current frame,
		 * if it isn't enough for completing cmd sending,
		 * defer sending the command until the second half of the next frame. */
		left_time = us_per_frame - (vsync_width - delay);
		if (left_time < 2000) {
			delay = left_time + vsync_width;
			usleep_range(delay, delay + 100);
		}
	}
	SDE_ATRACE_END("oplus_need_to_sync_te");

	return;
}

int oplus_wait_for_vsync(struct dsi_panel *panel)
{
	int rc = 0;
	struct dsi_display *d_display = get_main_display();
	struct drm_encoder *drm_enc = NULL;

	if (!panel || !panel->cur_mode) {
		DSI_ERR("Oplus Features config No panel device\n");
		return -ENODEV;
	}

	if (panel->power_mode == SDE_MODE_DPMS_OFF || !panel->panel_initialized) {
		OPLUS_DSI_WARN("display panel in off status\n");
		return -ENODEV;
	}

	if(!strcmp(panel->type, "secondary")) {
		d_display = get_sec_display();
	}

	if (!d_display || !d_display->bridge) {
		DSI_ERR("invalid display params\n");
		return -ENODEV;
	}

	drm_enc = d_display->bridge->base.encoder;

	if (!drm_enc) {
		DSI_ERR("invalid encoder params\n");
		return -ENODEV;
	}

	sde_encoder_wait_for_event(drm_enc, MSM_ENC_VBLANK);

	return rc;
}

void oplus_panel_frame_delay(struct dsi_panel *panel, u32 per_frame_us, u32 frame_delay_us)
{
	struct sde_encoder_virt *sde_enc = NULL;
	struct dsi_display *display = NULL;
	s64 duration;
	u32 debounce_time = 3000;
	u32 frame_end;
	int delay;
	char tag_name[128];
	ktime_t last_te_timestamp;

	display = to_dsi_display(panel->host);
	if (!display) {
		OPLUS_DSI_ERR("invalid display params\n");
		return;
	}

	sde_enc = to_sde_encoder_virt(display->bridge->base.encoder);
	if (!sde_enc) {
		OPLUS_DSI_ERR("invalid encoder params\n");
		return;
	}

	/* add 700us to vsync width(first half of frame time) to
	 * 1. avoid command sent in the middle of TE cycle
	 * 2. compensate the TE shift period */
	frame_delay_us += 700;

	last_te_timestamp = panel->oplus_panel.te_timestamp;
	duration = ktime_to_us(ktime_sub(ktime_get(), last_te_timestamp));
	if(duration > 3 * per_frame_us || sde_enc->rc_state == 4) {
		SDE_ATRACE_BEGIN("frame_delay_prepare");
		oplus_sde_early_wakeup(panel);
		if (duration > 16 * per_frame_us) {
			oplus_wait_for_vsync(panel);
		}
		SDE_ATRACE_END("frame_delay_prepare");
	}

	last_te_timestamp = panel->oplus_panel.te_timestamp;
	duration = ktime_to_us(ktime_sub(ktime_get(), last_te_timestamp));
	delay = frame_delay_us - (duration % per_frame_us);
	snprintf(tag_name, sizeof(tag_name), "frame_delay: delay %d us, last te: %lld", delay, ktime_to_us(last_te_timestamp));

	if (delay > 0) {
		SDE_ATRACE_BEGIN(tag_name);
		SDE_EVT32(per_frame_us, last_te_timestamp, delay);
		usleep_range(delay, delay + 100);
		SDE_ATRACE_END(tag_name);
	}

	frame_end = per_frame_us - (ktime_to_us(ktime_sub(ktime_get(), last_te_timestamp)) % per_frame_us);

	if (frame_end < debounce_time) {
		delay = frame_end + frame_delay_us;
		snprintf(tag_name, sizeof(tag_name), "frame_delay: delay %d us to next frame, last te: %lld", delay, ktime_to_us(last_te_timestamp));
		SDE_ATRACE_BEGIN(tag_name);
		usleep_range(delay, delay + 100);
		SDE_ATRACE_END(tag_name);
	}

	return;
}

void oplus_panel_timing_switch_frame_delay(struct dsi_panel *panel)
{
	u32 per_frame_us;
	u32 timing_switch_frame_delay;

	if (!panel->oplus_panel.timing_switch_frame_delay) {
		return;
	}

	if (panel->oplus_panel.last_refresh_rate != 60) {
		return;
	}
	per_frame_us = panel->oplus_panel.last_us_per_frame;
	timing_switch_frame_delay = panel->oplus_panel.last_vsync_width;
	if (timing_switch_frame_delay) {
		oplus_panel_frame_delay(panel, per_frame_us, timing_switch_frame_delay);
		OPLUS_DSI_INFO("timing_switch cmd will be sent in the second half of this frame\n");
	}

	return;
}

void oplus_panel_all_timing_switch_frame_delay(struct dsi_panel *panel)
{
	u32 per_frame_us;
	u32 timing_switch_frame_delay;

	if (!panel->oplus_panel.all_timing_switch_frame_delay) {
		return;
	}

	per_frame_us = panel->oplus_panel.last_us_per_frame;
	timing_switch_frame_delay = panel->oplus_panel.last_vsync_width;
	if (timing_switch_frame_delay) {
		oplus_panel_frame_delay(panel, per_frame_us, timing_switch_frame_delay);
		OPLUS_DSI_INFO("timing_switch cmd will be sent in the second half of this frame\n");
	}

	return;
}

int oplus_dsi_panel_parse_lut(struct dsi_panel *panel)
{
	struct dsi_parser_utils *utils = NULL;

	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return  -EINVAL;
	}

	utils = &panel->utils;
	panel->oplus_panel.lut_enabled = utils->read_bool(utils->data, "oplus,dsi-lut-set-enabled");
	OPLUS_DSI_INFO("oplus,dsi-lut-set-enabled: %s", panel->oplus_panel.lut_enabled ? "true" : "false");
	panel->oplus_panel.lut_refresh_rate = 120;

	return 0;
}

void oplus_panel_timing_switch_lut_set(struct dsi_panel *panel)
{
	int rc = 0;
	unsigned int refresh_rate = 0;
	unsigned int last_refresh_rate = 0;

	if (panel->oplus_panel.lut_enabled == true) {
		refresh_rate = panel->cur_mode->timing.refresh_rate;
		last_refresh_rate = panel->oplus_panel.lut_refresh_rate;
		OPLUS_DSI_INFO("refresh_rate %d last_refresh_rate %d\n", refresh_rate, last_refresh_rate);
		if (last_refresh_rate == 120 && refresh_rate == 60) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_120_TO_60, false);
		} else if (last_refresh_rate == 60 && refresh_rate == 120) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_60_TO_120, false);
		} else if (last_refresh_rate == 120 && refresh_rate == 144) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_120_TO_144, false);
		} else if (last_refresh_rate == 144 && refresh_rate == 120) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_144_TO_120, false);
		} else if (last_refresh_rate == 120 && refresh_rate == 90) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_120_TO_90, false);
		} else if (last_refresh_rate == 90 && refresh_rate == 120) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_90_TO_120, false);
		} else if (last_refresh_rate == 144 && refresh_rate == 60) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_144_TO_60, false);
		} else if (last_refresh_rate == 60 && refresh_rate == 144) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_60_TO_144, false);
		} else if (last_refresh_rate == 90 && refresh_rate == 60) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_90_TO_60, false);
		} else if (last_refresh_rate == 60 && refresh_rate == 90) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_60_TO_90, false);
		} else if (last_refresh_rate == 144 && refresh_rate == 90) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_144_TO_90, false);
		} else if (last_refresh_rate == 90 && refresh_rate == 144) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_90_TO_144, false);
		} else if (last_refresh_rate == 165 && refresh_rate == 120) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_165_TO_120, false);
		} else if (last_refresh_rate == 165 && refresh_rate == 60) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_165_TO_60, false);
		} else if (last_refresh_rate == 165 && refresh_rate == 90) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_165_TO_90, false);
		} else if (last_refresh_rate == 165 && refresh_rate == 144) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_165_TO_144, false);
		} else if (last_refresh_rate == 60 && refresh_rate == 165) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_60_TO_165, false);
		} else if (last_refresh_rate == 90 && refresh_rate == 165) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_90_TO_165, false);
		} else if (last_refresh_rate == 120 && refresh_rate == 165) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_120_TO_165, false);
		} else if (last_refresh_rate == 144 && refresh_rate == 165) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_FPS_SWITCH_144_TO_165, false);
		} else {
			OPLUS_DSI_INFO("no associated LUT SEL\n");
		}
		if (rc) {
			OPLUS_DSI_ERR("failed to send DSI_CMD_ESD_SWITCH_PAGE, rc=%d\n", rc);
		}
	}

	return;
}

void oplus_panel_timing_switch_lut_post(struct dsi_panel *panel)
{
	panel->oplus_panel.lut_refresh_rate = panel->cur_mode->timing.refresh_rate;
	return;
}

void oplus_panel_timing_switch_wait_te(struct dsi_panel *panel)
{
	if (!panel) {
		OPLUS_DSI_ERR("Invalid Params\n");
		return;
	}

	if (panel->oplus_panel.wait_te_config & BIT(1)) {
		if (panel->cur_mode->timing.refresh_rate == 90)
			oplus_need_to_sync_te(panel);
	}
	if (panel->oplus_panel.wait_te_config & BIT(2)) {
		if (panel->oplus_panel.lut_refresh_rate == 165) {
			if(panel->cur_mode->timing.refresh_rate == 60) {
				usleep_range(6000, 6100);
			} else {
				usleep_range(3000, 3100);
			}
			OPLUS_DSI_INFO("timing_switch cmd after sending there will be a delay\n");
		}
	}

	return;
}

struct panel_ae174_gamma* get_panel_ae174_gamma(void)
{
	return &ae174_gamma_regs;
}

int oplus_panel_ae174_gamma_compensation(void *dsi_display)
{
	int rc = 0;
	int i = 0;
	int index = 0;
	unsigned int retry_count = 0;
	unsigned char l_r_reg[2] = {0};
	unsigned char l_g_reg[2] = {0};
	unsigned char l_b_reg[2] = {0};
	unsigned char a_r_reg[2][2] = {0};
	unsigned char a_g_reg[2][2] = {0};
	unsigned char a_b_reg[2][2] = {0};
	unsigned char elvss_reg[2] = {0};
	unsigned char check_flag = 0;
	unsigned int read_l_r_gamma = 0;
	unsigned int read_l_g_gamma = 0;
	unsigned int read_l_b_gamma = 0;
	long long temp = 0;
	char gammaregs120[3][50] = {0};
	char gammaregs60[5][6] = {0};
	char gammaregs200nitL80[3][4] = {0};
	char gammaregs120_last[3][50] = {0};
	char gammaregs60_last[5][6] = {0};
	char gammaregs200nitL80_last[3][4] = {0};
	unsigned int gamma120_compensation[3][25] = {0};
	unsigned int gamma60_compensation[5][3] = {0};
	unsigned int gammaregs200nitL80_compensation[3][2] = {0};
	char regs[50] = {0};
	char read_band[9][2] = {
		{0xB0, 18},
		{0xB1, 18},
		{0xB2, 14},
		{0xB3, 18},
		{0xB4, 18},
		{0xB5, 14},
		{0xB6, 18},
		{0xB7, 18},
		{0xB8, 14},
	};
	struct dsi_display *display = dsi_display;
	struct dsi_panel *panel = display->panel;
	struct dsi_display_ctrl *m_ctrl = NULL;
	struct dsi_display_mode *mode = panel->cur_mode;
	struct panel_ae174_gamma *ae174_gamma = get_panel_ae174_gamma();
	char replace_reg[1] = {0};

	OPLUS_DSI_INFO("%s +\n", __func__);

	if (!panel) {
		OPLUS_DSI_ERR("panel is null\n");
		return -EINVAL;
	}

	if (!panel->oplus_panel.gamma_ae174_compensation_support) {
		OPLUS_DSI_INFO("panel gamma compensation isn't supported\n");
		return rc;
	}

	if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
		OPLUS_DSI_ERR("display panel in off status\n");
		return -EINVAL;
	}

	if (!display->panel->panel_initialized) {
		OPLUS_DSI_ERR("panel initialized = false\n");
		return -EINVAL;
	}

	if (display->oplus_display.panel_id1 != 0x2A) {
		OPLUS_DSI_ERR("no panel id was read\n");
		return -EINVAL;
	}

	m_ctrl = &display->ctrl[display->cmd_master_idx];
	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);
	while (!g_gamma_regs_read_done && (retry_count < 10)) {
		if (display->panel->power_mode != SDE_MODE_DPMS_ON) {
			OPLUS_DSI_ERR("display panel in off status\n");
			g_gamma_regs_read_done = false;
			rc = -EINVAL;
			break;
		}
		if (!display->panel->panel_initialized) {
			OPLUS_DSI_ERR("panel initialized = false\n");
			g_gamma_regs_read_done = false;
			rc = -EINVAL;
			break;
		}
		check_flag = 0;
		replace_reg[0] = 0x02;
		rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, 0x6F, replace_reg, 1);
		GAMMA_SEND_CHECK(rc);
		/* read apl gamma */
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE3, false);
		GAMMA_SEND_CHECK(rc);
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
		GAMMA_SEND_CHECK(rc);
		rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB2, &a_r_reg[0], 2);
		GAMMA_READ_CHECK(rc);
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
		GAMMA_SEND_CHECK(rc);
		rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, &a_g_reg[0], 2);
		GAMMA_READ_CHECK(rc);
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
		GAMMA_SEND_CHECK(rc);
		rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB8, &a_b_reg[0], 2);
		GAMMA_READ_CHECK(rc);
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE2, false);
		GAMMA_SEND_CHECK(rc);
		rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, &elvss_reg[0], 1);
		GAMMA_READ_CHECK(rc);

		OPLUS_DSI_INFO("a_r_reg=[%02X %02X] a_g_reg=[%02X %02X] a_b_reg=[%02X %02X] elvss_reg=[%02X]\n",
				a_r_reg[0][0], a_r_reg[0][1], a_g_reg[0][0], a_g_reg[0][1], a_b_reg[0][0], a_b_reg[0][1], elvss_reg[0]);

		if (memcmp(a_r_reg[0], a_r_reg[1], 2) || memcmp(a_g_reg[0], a_g_reg[1], 2) ||
			memcmp(a_b_reg[0], a_b_reg[1], 2) || memcmp(&elvss_reg[0], &elvss_reg[1], 1)) {
			check_flag |= 1 << 0;
			memcpy(a_r_reg[1], a_r_reg[0], 2);
			memcpy(a_g_reg[1], a_g_reg[0], 2);
			memcpy(a_b_reg[1], a_b_reg[0], 2);
			memcpy(&elvss_reg[1], &elvss_reg[0], 1);
		} else {
			ae174_gamma->a_r_gamma = ((a_r_reg[0][0] & 0xFF) << 8) | a_r_reg[0][1];
			ae174_gamma->a_g_gamma = ((a_g_reg[0][0] & 0xFF) << 8) | a_g_reg[0][1];
			ae174_gamma->a_b_gamma = ((a_b_reg[0][0] & 0xFF) << 8) | a_b_reg[0][1];
			ae174_gamma->elvss_reg = elvss_reg[0];
		}
		/* read lhbm gamma */
		if (display->oplus_display.panel_id1 == 0x1D && display->oplus_display.panel_id2 == 0x01) {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB2, l_r_reg, 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, l_g_reg, 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB8, l_b_reg, 2);
			GAMMA_READ_CHECK(rc);
			OPLUS_DSI_INFO("l_r_reg=[%02X %02X] l_g_reg=[%02X %02X] l_b_reg=[%02X %02X]\n",
						   l_r_reg[0], l_r_reg[1], l_g_reg[0], l_g_reg[1], l_b_reg[0], l_b_reg[1]);
			read_l_r_gamma = ((l_r_reg[0] & 0xFF) << 8) | l_r_reg[1];
			read_l_g_gamma = ((l_g_reg[0] & 0xFF) << 8) | l_g_reg[1];
			read_l_b_gamma = ((l_b_reg[0] & 0xFF) << 8) | l_b_reg[1];
			if ((read_l_r_gamma > 0 && read_l_r_gamma < 0xFFF) &&
				(read_l_g_gamma > 0 && read_l_g_gamma < 0xFFF) &&
				(read_l_b_gamma > 0 && read_l_b_gamma < 0xFFF)) {
				ae174_gamma->a_r_gamma = ((a_r_reg[0][0] & 0xFF) << 8) | a_r_reg[0][1];
				ae174_gamma->a_g_gamma = ((a_g_reg[0][0] & 0xFF) << 8) | a_g_reg[0][1];
				ae174_gamma->a_b_gamma = ((a_b_reg[0][0] & 0xFF) << 8) | a_b_reg[0][1];
				ae174_gamma->elvss_reg = elvss_reg[0];
				ae174_gamma->l_r_gamma = read_l_r_gamma;
				ae174_gamma->l_g_gamma = read_l_g_gamma;
				ae174_gamma->l_b_gamma = read_l_b_gamma;
				g_gamma_regs_read_done = true;
				OPLUS_DSI_INFO("update panel ae174 gamma successfully\n");
				break;
			}
			OPLUS_DSI_ERR("gamma compensation regs is invalid, retry=%d\n", retry_count);
			retry_count++;
		} else {
			memset(gammaregs120, 0, sizeof(gammaregs120));
			memset(gammaregs60, 0, sizeof(gammaregs60));
			memset(gammaregs200nitL80, 0, sizeof(gammaregs200nitL80));
			/* 1.1 read 120hz gamma band */
			replace_reg[0] = 0x08;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, 0xBF, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			replace_reg[0] = 0x02;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, 0x6F, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			for (i = 0; i < 3; i++) {
				for (index = 0; index < 3; index++) {
					rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, read_band[(i * 3) + index][0],
							&gammaregs120[i][(index * 18)], read_band[(i * 3) + index][1]);
					if (rc < 0) {
						OPLUS_DSI_ERR("failed to read GAMMA_COMPENSATION_READ_REG rc=%d\n", rc);
						break;
					}
				}
				if (rc < 0) break;
			}
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB2, &gammaregs60[0][0], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, &gammaregs60[0][2], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB8, &gammaregs60[0][4], 2);
			GAMMA_READ_CHECK(rc);
			/* 1.3 read 120Hz 70nit gamma */
			replace_reg[0] = 0x07;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, 0xBF, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB2, &gammaregs60[1][0], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, &gammaregs60[1][2], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB8, &gammaregs60[1][4], 2);
			GAMMA_READ_CHECK(rc);
			/* 1.4 read 120Hz 40nit gamma */
			replace_reg[0] = 0x06;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, 0xBF, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB2, &gammaregs60[2][0], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB5, &gammaregs60[2][2], 2);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB8, &gammaregs60[2][4], 2);
			GAMMA_READ_CHECK(rc);

			/* 1.5 read 120Hz 200nit L80 gamma */
			replace_reg[0] = 0x08;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, 0xBF, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			replace_reg[0] = 0x0A;
			rc = oplus_panel_cmd_reg_replace(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, 0x6F, replace_reg, 1);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE0, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB1, &gammaregs200nitL80[0][0], 4);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB4, &gammaregs200nitL80[1][0], 4);
			GAMMA_READ_CHECK(rc);
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION_PAGE1, false);
			GAMMA_SEND_CHECK(rc);
			rc = dsi_panel_read_panel_reg_unlock(m_ctrl, panel, 0xB7, &gammaregs200nitL80[2][0], 4);
			GAMMA_READ_CHECK(rc);
			for (i = 0; i < 3; i++)
				if (!memcmp(gammaregs120[i], regs, 50))
					check_flag |= 1 << 1;
			for (i = 0; i < 3; i++)
				if (!memcmp(gammaregs60[i], regs, 6))
					check_flag |= 1 << 2;
			for (i = 0; i < 3; i++)
				if (!memcmp(gammaregs200nitL80[i], regs, 4))
					check_flag |= 1 << 3;

			for (i = 0; i < 3; i++)
				if (memcmp(gammaregs120[i], gammaregs120_last[i], 50))
					check_flag |= 1 << 4;
			for (i = 0; i < 3; i++)
				if (memcmp(gammaregs60[i], gammaregs60_last[i], 6))
					check_flag |= 1 << 5;
			for (i = 0; i < 3; i++)
				if (memcmp(gammaregs200nitL80[i], gammaregs200nitL80_last[i], 4))
					check_flag |= 1 << 6;

			OPLUS_DSI_INFO("check_flag %X\n", check_flag);
			if (check_flag == 0) {
				g_gamma_regs_read_done = true;
				OPLUS_DSI_INFO("gamma compensation read success\n");
				break;
			} else {
				memcpy(gammaregs120_last, gammaregs120, sizeof(gammaregs120));
				memcpy(gammaregs60_last, gammaregs60, sizeof(gammaregs60));
				memcpy(gammaregs200nitL80_last, gammaregs200nitL80, sizeof(gammaregs200nitL80));
				retry_count++;
				OPLUS_DSI_ERR("gamma compensation regs is invalid, retry=%d\n", retry_count);
			}
		}
	}
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	OPLUS_DSI_INFO("panel id ID1=0x%02X, ID2=0x%02X, ID3=0x%02X\n",
		display->oplus_display.panel_id1,
		display->oplus_display.panel_id2,
		display->oplus_display.panel_id3);

	if (display->oplus_display.panel_id2 != 0x05) {
		g_gamma_regs_read_done = false;
		return rc;
	}

	if (!g_gamma_regs_read_done) {
		return -EFAULT;
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);
	/* 2.1 120hz gamma compensation */
	for (i = 0; i < 3; i++) {
		for (index = 0; index < 25; index++) {
			gamma120_compensation[i][index] = gammaregs120[i][(index * 2)] << 8 | gammaregs120[i][(index * 2) + 1];
		}
		for (index = 0; index < 25; index++) {
			temp = gamma120_compensation[i][index] * gamma120_ratio[(24 - index)][i];
			gamma120_compensation[i][index] = ROUND_FIXED_POINT(temp);
		}
		for (index = 0; index < 25; index++) {
			gammaregs120[i][(index * 2)] = gamma120_compensation[i][index] >> 8;
			gammaregs120[i][(index * 2) + 1] = gamma120_compensation[i][index] & 0xFF;
		}
	}
	/* 2.2 60Hz/200nit 60Hz/70nit 60Hz/40nit 90Hz/70nit 90Hz/40nit gamma compensation */
	for (i = 0; i < 6; i++) {
		gammaregs60[3][i] = gammaregs60[1][i];
	}
	for (i = 0; i < 6; i++) {
		gammaregs60[4][i] = gammaregs60[2][i];
	}
	for (i = 0; i < 5; i++) {
		for (index = 0; index < 3; index++) {
			gamma60_compensation[i][index] = gammaregs60[i][(index * 2)] << 8 | gammaregs60[i][(index * 2) + 1];
		}
		for (index = 0; index < 3; index++) {
			temp = gamma60_compensation[i][index] * gamma60_ratio[i][index];
			gamma60_compensation[i][index] = ROUND_FIXED_POINT(temp);
		}
		for (index = 0; index < 3; index++) {
			gammaregs60[i][(index * 2)] = gamma60_compensation[i][index] >> 8;
			gammaregs60[i][(index * 2) + 1] = gamma60_compensation[i][index] & 0xFF;
		}
	}
	/* 2.3 200nit 80L gamma compensation */
	for (i = 0; i < 3; i++) {
		for (index = 0; index < 2; index++) {
			gammaregs200nitL80_compensation[i][index] = gammaregs200nitL80[i][(index * 2)] << 8 | gammaregs200nitL80[i][(index * 2) + 1];
		}
		for (index = 0; index < 2; index++) {
			temp = gammaregs200nitL80_compensation[i][index] * gammaregs200nitL80_ratio[i][index];
			gammaregs200nitL80_compensation[i][index] = ROUND_FIXED_POINT(temp);
		}
		for (index = 0; index < 2; index++) {
			gammaregs200nitL80[i][(index * 2)] = gammaregs200nitL80_compensation[i][index] >> 8;
			gammaregs200nitL80[i][(index * 2) + 1] = gammaregs200nitL80_compensation[i][index] & 0xFF;
		}
	}
	/* 3.1 set gamma compensation cmd */
	rc = 0;
	for (index = 0; index < display->panel->num_display_modes; index++) {
		mode = &display->modes[index];
		if (!mode) {
			OPLUS_DSI_INFO("mode is null\n");
			continue;
		}
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[0][0], 2, 5);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[0][2], 2, 7);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[0][4], 2, 9);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[1][0], 2, 12);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[1][2], 2, 14);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[1][4], 2, 16);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[2][0], 2, 19);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[2][2], 2, 21);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[2][4], 2, 23);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[0][0], 18, 25);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[0][18], 18, 26);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[0][36], 14, 27);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[1][0], 18, 28);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[1][18], 18, 29);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[1][36], 14, 30);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[2][0], 18, 31);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[2][18], 18, 32);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs120[2][36], 14, 33);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[3][0], 2, 36);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[3][2], 2, 38);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[3][4], 2, 40);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[4][0], 2, 43);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[4][2], 2, 45);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs60[4][4], 2, 47);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs200nitL80[0][0], 4, 50);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs200nitL80[1][0], 4, 52);
		if (rc) break;
		rc = oplus_panel_cmd_reg_replace_specific_row(panel, mode, DSI_CMD_GAMMA_COMPENSATION,
													  &gammaregs200nitL80[2][0], 4, 54);
		if (rc) break;
	}

	if (rc) {
		g_gamma_regs_read_done = false;
		OPLUS_DSI_ERR("DSI_CMD_GAMMA_COMPENSATION replace failed\n");
	} else {
		g_gamma_inverse = true;
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_GAMMA_COMPENSATION, false);
		if (rc) {
			OPLUS_DSI_ERR("send DSI_CMD_GAMMA_COMPENSATION failed\n");
		}
	}
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
	OPLUS_DSI_INFO("%s -\n", __func__);
	return rc;
}
