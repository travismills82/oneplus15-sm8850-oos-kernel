// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2025 Oplus. All rights reserved.
 */

#include "internal.h"
#if IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)
#include <linux/soc/qcom/panel_event_notifier.h>
#include <linux/of.h>
#include <drm/drm_panel.h>
#elif IS_ENABLED(CONFIG_DRM_MSM) || IS_ENABLED(CONFIG_DRM_OPLUS_NOTIFY)
#include <linux/msm_drm_notify.h>
#elif IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
#include <linux/mtk_disp_notify.h>
#endif

#include <linux/sched.h>
#include "mm_osvelte/common.h"

#define PRIMARY_PANEL_ID 0
#define SECONDARY_PANEL_ID 1
#define MAX_PANEL_ID 2

#if IS_ENABLED(CONFIG_DRM_MSM) || IS_ENABLED(CONFIG_DRM_OPLUS_NOTIFY) || IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
static struct notifier_block fb_notif;
#elif IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)
static void *g_panel_cookie[MAX_PANEL_ID];
static bool g_display_off[MAX_PANEL_ID];
#endif
atomic_t display_off = ATOMIC_LONG_INIT(0);

static void set_display_off(bool set, int panel_id)
{
	bool b;

#if IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)
	g_display_off[panel_id] = set;
	b = g_display_off[PRIMARY_PANEL_ID] && g_display_off[SECONDARY_PANEL_ID];
#else /* IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER) */
	b = set;
#endif /* IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER) */
	atomic_set(&display_off, b);
	osvelte_set_scene(MM_SCENE_DISPLAY_OFF, b);
}

#if IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)
static struct drm_panel *get_active_panel(const char *panel_name)
{
	int i;
	int count;
	struct device_node *panel_node = NULL;
	struct drm_panel *panel = NULL;
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "oplus,dsi-display-dev");
	if (!np) {
		log_err("oplus,dsi-display-dev node missing\n");
		return NULL;
	}

	log_warn("oplus,dsi-display-dev node found\n");
	count = of_count_phandle_with_args(np, panel_name, NULL);
	if (count <= 0) {
		log_err("%s missing\n", panel_name);
		goto not_found;
	}

	for (i = 0; i < count; i++) {
		panel_node = of_parse_phandle(np, panel_name, i);
		panel = of_drm_find_panel(panel_node);
		of_node_put(panel_node);
		if (!IS_ERR(panel)) {
			log_warn("%s: active panel found\n", panel_name);
			goto found;
		}
	}
not_found:
	panel = NULL;
found:
	of_node_put(np);
	return panel;
}

static void bright_fb_notifier_callback(enum panel_event_notifier_tag tag,
	struct panel_event_notification *notification, void *client_data)
{
	if (!notification) {
		log_info("%s, invalid notify\n", __func__);
		return;
	}

	switch (notification->notif_type) {
	case DRM_PANEL_EVENT_BLANK:
		set_display_off(true, PRIMARY_PANEL_ID);
		break;
	case DRM_PANEL_EVENT_UNBLANK:
		set_display_off(false, PRIMARY_PANEL_ID);
		break;
	default:
		break;
	}
}

static void secondary_bright_fb_notifier_callback(enum panel_event_notifier_tag tag,
	struct panel_event_notification *notification, void *client_data)
{
	if (!notification) {
		log_info("%s, invalid notify\n", __func__);
		return;
	}
	switch (notification->notif_type) {
	case DRM_PANEL_EVENT_BLANK:
		set_display_off(true, SECONDARY_PANEL_ID);
		break;
	case DRM_PANEL_EVENT_UNBLANK:
		set_display_off(false, SECONDARY_PANEL_ID);
		break;
	default:
		break;
	}
}

#elif IS_ENABLED(CONFIG_DRM_MSM) || IS_ENABLED(CONFIG_DRM_OPLUS_NOTIFY)
static int bright_fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct msm_drm_notifier *evdata = data;
	int *blank;

	if (evdata && evdata->data) {
		blank = evdata->data;

		if (*blank ==  MSM_DRM_BLANK_POWERDOWN)
			set_display_off(true, PRIMARY_PANEL_ID);
		else if (*blank == MSM_DRM_BLANK_UNBLANK)
			set_display_off(false, PRIMARY_PANEL_ID);
	}

	return NOTIFY_OK;
}
#elif IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
static int mtk_bright_fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	int *blank = (int *)data;

	if (!blank) {
		log_err("get disp stat err, blank is NULL!\n");
		return 0;
	}

	if (*blank == MTK_DISP_BLANK_POWERDOWN)
		set_display_off(true, PRIMARY_PANEL_ID);
	else if (*blank == MTK_DISP_BLANK_UNBLANK)
		set_display_off(false, PRIMARY_PANEL_ID);
	return NOTIFY_OK;
}
#endif

void register_panel_event_notifier(void)
{
#if IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)
	struct drm_panel *primary_panel, *secondary_panel;
	void *cookie = NULL;

	primary_panel = get_active_panel("oplus,dsi-panel-primary");
	if (primary_panel)
		cookie = panel_event_notifier_register(PANEL_EVENT_NOTIFICATION_PRIMARY,
			PANEL_EVENT_NOTIFIER_CLIENT_MM, primary_panel, bright_fb_notifier_callback, NULL);

	if (primary_panel && !IS_ERR(cookie)) {
		log_warn("%s success\n", __func__);
		g_panel_cookie[PRIMARY_PANEL_ID] = cookie;
	} else {
		log_err("%s failed. need fix\n", __func__);
	}

	g_display_off[SECONDARY_PANEL_ID] = true;
	secondary_panel = get_active_panel("oplus,dsi-panel-secondary");
	if (secondary_panel)
		cookie = panel_event_notifier_register(PANEL_EVENT_NOTIFICATION_SECONDARY,
			PANEL_EVENT_NOTIFIER_CLIENT_MM_SECONDARY, secondary_panel, secondary_bright_fb_notifier_callback, NULL);

	if (secondary_panel && !IS_ERR(cookie)) {
		log_warn("%s secondary panel success\n", __func__);
		g_panel_cookie[SECONDARY_PANEL_ID] = cookie;
		/* if secondary panel is active, set primary panel display off by default */
		g_display_off[PRIMARY_PANEL_ID] = true;
	} else {
		/* set secondary panel display off by default */
		log_err("%s failed. need fix\n", __func__);
	}

#elif IS_ENABLED(CONFIG_DRM_MSM) || IS_ENABLED(CONFIG_DRM_OPLUS_NOTIFY)
	fb_notif.notifier_call = bright_fb_notifier_callback;
	if (msm_drm_register_client(&fb_notif))
		log_err("msm_drm_register_client failed\n");
#elif IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
	fb_notif.notifier_call = mtk_bright_fb_notifier_callback;
	if (mtk_disp_notifier_register("Oplus_hybridswap", &fb_notif))
		log_err("mtk_disp_notifier_register failed\n");
#endif
}

void unregister_panel_event_notifier(void)
{
#if IS_ENABLED(CONFIG_DRM_PANEL_NOTIFY) || IS_ENABLED(CONFIG_QCOM_PANEL_EVENT_NOTIFIER)

	if (g_panel_cookie[PRIMARY_PANEL_ID]) {
		panel_event_notifier_unregister(g_panel_cookie[PRIMARY_PANEL_ID]);
		g_panel_cookie[PRIMARY_PANEL_ID] = NULL;
	}
	if (g_panel_cookie[SECONDARY_PANEL_ID]) {
		panel_event_notifier_unregister(g_panel_cookie[SECONDARY_PANEL_ID]);
		g_panel_cookie[SECONDARY_PANEL_ID] = NULL;
	}
#elif IS_ENABLED(CONFIG_DRM_MSM) || IS_ENABLED(CONFIG_DRM_OPLUS_NOTIFY)
	msm_drm_unregister_client(&fb_notif);
#elif IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
	mtk_disp_notifier_unregister(&fb_notif);
#endif
}
