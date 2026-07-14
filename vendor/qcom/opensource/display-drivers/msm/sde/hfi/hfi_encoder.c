// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include "hfi_encoder.h"
#include "hfi_props.h"
#include "sde_crtc.h"
#include "sde_encoder_phys.h"
#include "hfi_crtc.h"
#include "hfi_kms.h"
#include "sde_kms.h"
#include "hfi_msm_dbg.h"

#define TIMEOUT_MAX	80

static ktime_t hfi_enc_unpack_frame_event(void *payload, u32 *idx, struct sde_encoder_virt *sde_enc)
{
	u32 read = 0;
	u32 fps;
	u64 ts_high, ts_low;
	ktime_t ts = 0;
	u32 *data = payload;
	struct hfi_encoder *hfi_enc;
	struct drm_encoder *drm_enc;

	if (!payload) {
		SDE_ERROR("No payload specified\n");
		return 0;
	}

	hfi_enc = to_hfi_encoder(sde_enc);
	if (!hfi_enc)
		return -EINVAL;

	drm_enc = &sde_enc->base;
	fps = sde_encoder_get_fps(drm_enc);

	ts_low =  data[read++];
	ts_high =  data[read++];
	atomic_set(&hfi_enc->hfi_frame_done_cnt, (u32) data[read++]);

	if (idx)
		*idx = data[read++];

	ts = (ts_high << 32);
	ts =  ts | (ts_low);

	/* convert into qtimer hw ticks & adjust */
	ts = (ts * 192) / (10 * 1000);
	ts = sde_encoder_event_timestamp_adjust(DRMID(drm_enc), fps, ts);

	SDE_EVT32(DRMID(drm_enc), atomic_read(&hfi_enc->hfi_commit_cnt),
			atomic_read(&hfi_enc->hfi_frame_done_cnt), ts);

	return ts;
}

static ktime_t hfi_enc_unpack_vsync_event(void *payload, u32 *idx, struct sde_encoder_virt *sde_enc)
{
	u32 read = 0;
	u32 fps;
	u64 ts_high, ts_low;
	ktime_t ts = 0;
	u32 *data = payload;
	u32 hw_vsync_count;
	struct hfi_encoder *hfi_enc;
	struct drm_encoder *drm_enc;

	if (!payload) {
		SDE_ERROR("No payload specified\n");
		return 0;
	}

	hfi_enc = to_hfi_encoder(sde_enc);
	if (!hfi_enc)
		return -EINVAL;

	drm_enc = &sde_enc->base;
	fps = sde_encoder_get_fps(drm_enc);

	ts_low =  data[read++];
	ts_high =  data[read++];

	hw_vsync_count = data[read++];
	if (idx)
		*idx = data[read++];

	ts = (ts_high << 32);
	ts =  ts | (ts_low);

	/* convert into qtimer hw ticks & adjust */
	ts = (ts * 192) / (10 * 1000);
	ts = sde_encoder_event_timestamp_adjust(DRMID(drm_enc), fps, ts);

	atomic_inc_return(&hfi_enc->hfi_vsync_cnt);
	hfi_enc->vblank_ts = ts;

	SDE_EVT32(DRMID(drm_enc), atomic_read(&hfi_enc->hfi_vsync_cnt), ts);

	return ts;
}

static void hfi_encoder_frame_event_callback(struct sde_encoder_virt *sde_enc,
		void *payload, u32 event)
{
	struct sde_kms *sde_kms = sde_encoder_get_kms(&sde_enc->base);
	struct hfi_encoder *hfi_enc = to_hfi_encoder(sde_enc);
	ktime_t ts = 0;
	unsigned long lock_flags;
	int new_cnt = -1;

	if (!sde_kms || !hfi_enc) {
		SDE_ERROR("invalid param: sde_kms %pK\n", sde_kms);
		return;
	}

	if (!sde_enc->cur_master) {
		SDE_ERROR("invalid param: sde_enc\n");
		return;
	}

	ts = hfi_enc_unpack_frame_event(payload, NULL, sde_enc);

	spin_lock_irqsave(&sde_enc->enc_spinlock, lock_flags);
	new_cnt = atomic_add_unless(&sde_enc->pending_commit_cnt, -1, 0);
	spin_unlock_irqrestore(&sde_enc->enc_spinlock, lock_flags);

	SDE_EVT32(atomic_read(&sde_enc->pending_commit_cnt));

	sde_enc->crtc_frame_event_cb_data.connector = sde_enc->cur_master->connector;

	if (sde_enc->crtc_frame_event_cb)
		sde_enc->crtc_frame_event_cb(&sde_enc->crtc_frame_event_cb_data, event, ts);

	/* if vsync is not enabled by client, increment local vsync counter on scan-start */
	if (!sde_enc->crtc_vblank_cb) {
		atomic_inc_return(&hfi_enc->hfi_vsync_cnt);
		hfi_enc->vblank_ts = ts;
		SDE_EVT32(atomic_read(&hfi_enc->hfi_vsync_cnt), ts);
	}

	wake_up_all(&hfi_enc->pending_kickoff_wq);
}

static void hfi_encoder_vblank_callback(struct hfi_encoder *hfi_enc, void *payload)
{
	struct sde_encoder_virt *sde_enc;
	ktime_t ts = 0;

	if (!hfi_enc || !payload)
		return;

	sde_enc = hfi_enc->sde_base;

	ts = hfi_enc_unpack_vsync_event(payload, NULL, sde_enc);

	if (sde_enc->crtc_vblank_cb)
		sde_enc->crtc_vblank_cb(sde_enc->crtc_vblank_cb_data, ts);
}

static void hfi_enc_hfi_prop_handler(u32 obj_id, u32 cmd_id,
		void *payload, u32 size, struct hfi_prop_listener *listener)
{
	struct hfi_encoder *hfi_enc = container_of(listener,
			struct hfi_encoder, hfi_cb_obj);
	struct sde_encoder_virt *sde_enc = hfi_enc->sde_base;
	struct drm_connector *conn;
	struct drm_encoder *drm_enc;
	u32 event = 0;
	bool recovery_events;
	u32 *data = payload;

	if (!hfi_enc) {
		SDE_ERROR("invalid object or listener from FW\n");
		return;
	}

	switch (cmd_id) {
	case HFI_COMMAND_DISPLAY_EVENT_FRAME_SCAN_START:
		event = SDE_ENCODER_FRAME_EVENT_DONE |
			SDE_ENCODER_FRAME_EVENT_SIGNAL_RETIRE_FENCE |
			SDE_ENCODER_FRAME_EVENT_SIGNAL_RELEASE_FENCE;
		sde_encoder_update_pending_kickoff_cnt(hfi_enc->sde_base);
		hfi_encoder_frame_event_callback(hfi_enc->sde_base,
				payload, event);
		break;
	case HFI_COMMAND_DISPLAY_EVENT_FRAME_SCAN_COMPLETE:
		event = SDE_ENCODER_FRAME_EVENT_DONE;
		hfi_encoder_frame_event_callback(hfi_enc->sde_base,
				payload, event);
		break;
	case HFI_COMMAND_DISPLAY_EVENT_VSYNC:
		hfi_encoder_vblank_callback(hfi_enc, payload);
		break;
	case HFI_COMMAND_DEBUG_PANIC_EVENT:
		recovery_events = sde_encoder_recovery_events_enabled(&sde_enc->base);

		drm_enc = &sde_enc->base;
		conn = sde_encoder_get_connector(drm_enc->dev, drm_enc);

		if (!conn) {
			SDE_ERROR("Invalid connector\n");
			return;
		}

		if (sde_conn_get_display_obj_id(conn) != (u32) data[0]) {
			SDE_ERROR("Invalid display ID\n");
			return;
		}

		if (recovery_events) {
			sde_connector_event_notify(conn, DRM_EVENT_SDE_HW_RECOVERY,
				sizeof(uint8_t), SDE_RECOVERY_CAPTURE);
		} else {
			event = (u32) data[1];
			if (HFI_DEBUG_EVENT_UNDERRUN & event) {
				MDSS_DBG_CTRL(0x0, "stop_ftrace");
				MDSS_DBG_CTRL(0x0, "panic_underrun");
			} else {
				MDSS_DBG_DUMP(0x0, "panic");
			}
		}
		break;
	case HFI_COMMAND_DISPLAY_EVENT_REGISTER:
	case HFI_COMMAND_DISPLAY_EVENT_DEREGISTER:
		break;
	default:
		SDE_ERROR("invalid hfi command 0x%x\n", cmd_id);
	}
}

static int _hfi_enc_hw_event_set_buff(struct sde_encoder_virt *enc, u32 payload,
		bool enable, bool defer_to_commit)
{
	struct drm_connector *conn;
	struct hfi_encoder *hfi_enc = to_hfi_encoder(enc);
	struct hfi_kms *hfi_kms = to_hfi_kms(sde_encoder_get_kms(&enc->base));
	struct hfi_cmdbuf_t *cmd_buf;
	u32 cmd, display_id = 0;
	int ret = 0;

	if (!hfi_enc || !hfi_kms) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	conn = sde_encoder_get_connector(enc->base.dev, &enc->base);
	if (!conn) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	display_id = sde_conn_get_display_obj_id(conn);
	if (defer_to_commit) {
		cmd_buf = hfi_kms_get_cmd_buf(hfi_kms, display_id, HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
	} else {
		cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
				display_id, HFI_CMDBUF_TYPE_DISPLAY_INFO_BLOCKING);
	}

	if (!cmd_buf) {
		SDE_ERROR("enc:%d failed to get cmd buf in events enable:%d for display:%d\n",
				enc->base.base.id, enable, display_id);
		return -EINVAL;
	}

	cmd = enable ? HFI_COMMAND_DISPLAY_EVENT_REGISTER : HFI_COMMAND_DISPLAY_EVENT_DEREGISTER;
	ret = hfi_adapter_add_get_property(cmd_buf, cmd, display_id, HFI_PAYLOAD_TYPE_U32,
			&payload, sizeof(payload), &hfi_enc->hfi_cb_obj,
			HFI_HOST_FLAGS_NON_DISCARDABLE);
	if (ret) {
		SDE_ERROR("failed to update event: 0x%x\n", payload);
		return ret;
	}

	SDE_DEBUG("sending events enable:%d for display:%d\n", enable, display_id);
	if (!defer_to_commit) {
		ret = hfi_adapter_set_cmd_buf(cmd_buf);
		SDE_EVT32(enc->base.base.id, display_id, cmd, ret, SDE_EVTLOG_FUNC_CASE1);
		if (ret) {
			SDE_ERROR("failed to send event register command\n");
			return ret;
		}
	}

	return 0;
}

static int _hfi_enc_register_hw_event(struct sde_encoder_virt *enc,
		u32 event, bool enable, bool defer_to_commit)
{
	int ret = 0;

	switch (event) {
	case MSM_ENC_COMMIT_DONE:
		ret = _hfi_enc_hw_event_set_buff(enc, HFI_EVENT_FRAME_SCAN_START,
			enable, defer_to_commit);
		break;
	case MSM_ENC_TX_COMPLETE:
		SDE_ERROR("unsupported tx complete wait %d\n", event);
		break;
	case MSM_ENC_VBLANK:
		ret = _hfi_enc_hw_event_set_buff(enc, HFI_EVENT_VSYNC,
				enable, defer_to_commit);
		break;
	default:
		SDE_ERROR("Invalid SDE event %d\n", event);
		return -EINVAL;
	}

	return ret;
}

static int hfi_enc_set_panic_events(struct sde_encoder_virt *enc, bool enable)
{
	struct hfi_encoder *hfi_enc = to_hfi_encoder(enc);
	struct hfi_kms *hfi_kms = to_hfi_kms(sde_encoder_get_kms(&enc->base));
	struct sde_encoder_virt *sde_enc = hfi_enc->sde_base;
	struct hfi_cmdbuf_t *cmd_buf;
	struct drm_connector *conn;
	struct drm_encoder *drm_enc;
	u32 payload[3];
	u32 disp_id;
	int ret;

	if (!enc || !hfi_enc || !hfi_kms) {
		SDE_ERROR("Invalid params\n");
		return -EINVAL;
	}

	drm_enc = &sde_enc->base;
	conn = sde_encoder_get_connector(drm_enc->dev, drm_enc);

	if (!conn) {
		SDE_ERROR("Invalid connector\n");
		return -EINVAL;
	}

	disp_id = sde_conn_get_display_obj_id(conn);

	cmd_buf = hfi_kms_get_cmd_buf(hfi_kms, disp_id, HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
	if (!cmd_buf) {
		SDE_ERROR("failed to get hfi command buffer\n");
		return -EINVAL;
	}

	payload[0] = disp_id;
	payload[1] = (HFI_DEBUG_EVENT_UNDERRUN | HFI_DEBUG_EVENT_HW_RESET |
				HFI_DEBUG_EVENT_PP_TIMEOUT);
	if (enable)
		payload[2] = HFI_TRUE;
	else
		payload[2] = HFI_FALSE;

	ret = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE,
			MSM_DRV_HFI_ID, HFI_PAYLOAD_TYPE_U32_ARRAY, &payload,
			sizeof(payload), &hfi_enc->hfi_cb_obj, HFI_HOST_FLAGS_NONE);
	if (ret) {
		SDE_ERROR("panic subscribe command failed\n");
		return ret;
	}

	SDE_EVT32(drm_enc->base.id, MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE, ret);
	return ret;
}

static int hfi_encoder_helper_wait_for_event(struct hfi_encoder *hfi_enc,
		struct sde_encoder_wait_info *info, u32 event)
{
	int rc = 0;
	s64 timeout_ms = info->timeout_ms;
	s64 wait_time_jiffies = msecs_to_jiffies(timeout_ms);
	ktime_t cur_ktime;
	ktime_t exp_ktime = ktime_add_ms(ktime_get(), timeout_ms);
	u32 curr_atomic_cnt = atomic_read(info->atomic_cnt);

	do {
		rc = wait_event_timeout(*(info->wq),
				atomic_read(info->atomic_cnt) == info->count_check,
				wait_time_jiffies);
		cur_ktime = ktime_get();

		SDE_EVT32(rc, ktime_to_ms(cur_ktime), timeout_ms, event,
				atomic_read(info->atomic_cnt), info->count_check);

		/* Make an early exit if the condition is already satisfied */
		if ((atomic_read(info->atomic_cnt) <= info->count_check) &&
			(info->count_check < curr_atomic_cnt)) {
			rc = true;
			break;
		}
	} while ((atomic_read(info->atomic_cnt) != info->count_check) &&
				(rc == 0) &&
				(ktime_compare_safe(exp_ktime, cur_ktime) > 0));

	return rc;
}

static int _hfi_enc_wait_for_commit_done(struct hfi_encoder *hfi_enc)
{
	int ret;
	struct sde_encoder_wait_info wait_info = {0};
	struct sde_encoder_virt *sde_enc;

	sde_enc = hfi_enc->sde_base;

	wait_info.wq = &hfi_enc->pending_kickoff_wq;
	wait_info.atomic_cnt = &sde_enc->pending_commit_cnt;
	wait_info.timeout_ms = TIMEOUT_MAX;

	ret = hfi_encoder_helper_wait_for_event(hfi_enc, &wait_info, HFI_EVENT_FRAME_SCAN_START);
	return ret;
}

static int hfi_enc_wait_for_event(struct sde_encoder_virt *enc, u32 event)
{
	int (*fn_wait)(struct hfi_encoder *hfi_enc) = NULL;
	int ret = 0;
	struct hfi_encoder *hfi_enc = to_hfi_encoder(enc);

	switch (event) {
	case MSM_ENC_COMMIT_DONE:
		fn_wait = _hfi_enc_wait_for_commit_done;
		break;
	case MSM_ENC_TX_COMPLETE:
		fn_wait = NULL;
		ret = 1;
		break;
	default:
		SDE_ERROR("unknown wait event %d\n", event);
		return -EINVAL;
	}

	if (fn_wait)
		ret = fn_wait(hfi_enc);

	return ret;
}

static int hfi_enc_enable_hw_event(struct sde_encoder_virt *enc, u32 event, bool enable)
{
	int ret = 0;
	struct hfi_encoder *hfi_enc = to_hfi_encoder(enc);

	if (!hfi_enc || event >= MSM_ENC_EVENT_MAX)
		return -EINVAL;

	if (event == MSM_ENC_VBLANK || event == MSM_ENC_COMMIT_DONE) {
		ret = _hfi_enc_register_hw_event(enc, event, enable, false);
		if (ret) {
			SDE_ERROR("failed to send event register ret:%d\n", ret);
			return ret;
		}

		hfi_enc->hw_events_state[event].state = enable;
		hfi_enc->hw_events_state[event].pending = false;
	} else if (hfi_enc->hw_events_state[event].state != enable) {
		hfi_enc->hw_events_state[event].state = enable;
		hfi_enc->hw_events_state[event].pending = true;
	}

	return ret;
}

static int hfi_enc_kickoff(struct sde_encoder_virt *enc, bool cfg_changed)
{
	int ret;
	u32 display_id;
	struct hfi_encoder *hfi_enc;
	struct hfi_cmdbuf_t *cmd_buf;
	struct drm_connector *conn;
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	u32 scan_id_prop[3] = {0,};
	u32 num_props = 1;

	if (!enc)
		return -EINVAL;

	sde_kms = sde_encoder_get_kms(&enc->base);
	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms) {
		SDE_ERROR("Failed to get hfi_kms\n");
		return -EINVAL;
	}

	hfi_enc = to_hfi_encoder(enc);
	conn = sde_encoder_get_connector(enc->base.dev, &enc->base);
	if (!conn) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	display_id = sde_conn_get_display_obj_id(conn);
	cmd_buf = hfi_kms_get_cmd_buf(hfi_kms, display_id, HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
	if (!cmd_buf) {
		SDE_ERROR("failed to find command buf for display_id:%d\n", display_id);
		return -EINVAL;
	}

	scan_id_prop[0] = num_props;
	scan_id_prop[1] = HFI_PROPERTY_DISPLAY_SCAN_SEQUENCE_ID;
	scan_id_prop[2] = atomic_inc_return(&hfi_enc->hfi_commit_cnt);

	ret = hfi_adapter_add_set_property(cmd_buf,
			HFI_COMMAND_DISPLAY_SET_PROPERTY,
			display_id,
			HFI_PAYLOAD_TYPE_U32_ARRAY,
			&scan_id_prop,
			sizeof(scan_id_prop),
			HFI_HOST_FLAGS_NON_DISCARDABLE);
	if (ret) {
		SDE_ERROR("failed to send scan id HFI property\n");
		return ret;
	}

	SDE_EVT32(atomic_read(&hfi_enc->hfi_commit_cnt));

	return ret;
}

static int hfi_enc_encoder_enable(struct sde_encoder_virt *enc)
{
	int ret;

	if (!enc) {
		SDE_ERROR("Invalid params\n");
		return -EINVAL;
	}

	ret = hfi_enc_set_panic_events(enc, true);
	if (ret) {
		SDE_ERROR("failed to send debug-init command\n");
		return ret;
	}

	ret = hfi_enc_enable_hw_event(enc, MSM_ENC_COMMIT_DONE, true);
	if (ret) {
		SDE_ERROR("failed to send commit wait command\n");
		return ret;
	}

	return 0;
}

static int hfi_enc_encoder_disable(struct sde_encoder_virt *enc)
{
	int ret;

	if (!enc) {
		SDE_ERROR("Invalid params\n");
		return -EINVAL;
	}

	ret = hfi_enc_enable_hw_event(enc, MSM_ENC_COMMIT_DONE, false);
	if (ret) {
		SDE_ERROR("failed to send commit wait command\n");
		return ret;
	}

	ret = hfi_enc_set_panic_events(enc, false);
	if (ret) {
		SDE_ERROR("failed to send debug-init command\n");
		return ret;
	}

	return 0;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int hfi_enc_debugfs_dump_status(struct sde_encoder_virt *sde_enc, struct seq_file *s)
{
	struct hfi_encoder *hfi_enc;
	struct sde_encoder_phys *phys;

	if (!s || !s->private || !sde_enc || (sde_enc != s->private))
		return -EINVAL;

	hfi_enc = to_hfi_encoder(sde_enc);
	if (!hfi_enc)
		return -EINVAL;

	phys = sde_enc->phys_encs[0];
	if (!phys)
		return -EINVAL;

	seq_printf(s, "intf:%d    vsync:%8d     underrun:%8d    ",
		phys->intf_idx - INTF_0,  atomic_read(&hfi_enc->hfi_frame_done_cnt), 0);

	switch (phys->intf_mode) {
	case INTF_MODE_VIDEO:
		seq_puts(s, "mode: video\n");
		break;
	case INTF_MODE_CMD:
		seq_puts(s, "mode: command\n");
		break;
	case INTF_MODE_WB_BLOCK:
		seq_puts(s, "mode: wb block\n");
		break;
	case INTF_MODE_WB_LINE:
		seq_puts(s, "mode: wb line\n");
		break;
	case INTF_MODE_NONE:
		seq_puts(s, "mode: none\n");
		break;
	default:
		seq_puts(s, "mode: ???\n");
		break;
	}

	return 0;
}

static int hfi_enc_debugfs_misr_setup(struct sde_encoder_virt *enc)
{
	int rc = 0;
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	struct drm_crtc *crtc;
	struct drm_encoder *drm_enc;
	struct msm_drm_private *priv;
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct drm_encoder *drm_encoder;
	struct drm_connector *drm_connector;
	struct misr_setup_data misr_data;
	u32 disp_id;

	crtc = enc->crtc;
	if (!crtc)
		SDE_ERROR("Failed to get crtc\n");

	drm_enc = &enc->base;
	priv = drm_enc->dev->dev_private;
	sde_kms = to_sde_kms(priv->kms);
	hfi_kms = to_hfi_kms(sde_kms);
	if (!hfi_kms) {
		SDE_ERROR("Failed to get hfi_kms\n");
		return -EINVAL;
	}

	/* Find connector to get disp_id */
	drm_encoder = &enc->base;
	drm_connector = sde_encoder_get_connector(drm_encoder->dev, drm_encoder);
	if (!drm_connector) {
		SDE_ERROR("Failed to get connector\n");
		return -EINVAL;
	}

	disp_id = sde_conn_get_display_obj_id(drm_connector);
	if (disp_id == U32_MAX) {
		SDE_ERROR("invalid display id\n");
		return -EINVAL;
	}

	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		SDE_ERROR("Failed to get valid command buffer\n");
		return -EINVAL;
	}

	misr_data.display_id = disp_id;
	misr_data.enable = atomic_read(&enc->misr_enable);
	misr_data.frame_count = enc->misr_frame_count;
	misr_data.module_type = HFI_DEBUG_MISR_INTF;

	rc = hfi_adapter_add_set_property(cmd_buf, HFI_COMMAND_DEBUG_MISR_SETUP,
			disp_id, HFI_PAYLOAD_TYPE_U32_ARRAY, &misr_data,
			sizeof(misr_data), HFI_HOST_FLAGS_NONE);
	if (rc) {
		SDE_ERROR("Failed to add property\n");
		return rc;
	}

	SDE_DEBUG("%s misr_setup: sending cmd buf\n", __func__);
	rc = hfi_adapter_set_cmd_buf(cmd_buf);
	SDE_EVT32(drm_enc->base.id, disp_id, HFI_COMMAND_DEBUG_MISR_SETUP, rc,
			SDE_EVTLOG_FUNC_CASE1);
	if (rc) {
		SDE_ERROR("Failed to send misr_setup command\n");
		return rc;
	}

	return rc;
}

void hfi_enc_misr_read_hfi_prop_handler(u32 obj_uid, u32 CMD_ID, void *payload, u32 size,
			struct hfi_prop_listener *hfi_listener)
{
	struct hfi_encoder *hfi_enc = container_of(hfi_listener,
		struct hfi_encoder, misr_read_listener);
	struct misr_read_data_ret *misr_data;
	struct sde_misr_values *misr_read_values;
	u32 max_count = 0;
	u32 module_type = 0;
	u32 *misr_values;

	if (!hfi_enc) {
		SDE_ERROR("Invalid object or listener from FW\n");
		return;
	}

	/* Parse MISR values */
	if (!payload || !size) {
		SDE_ERROR("Invalid payload received from FW\n");
		return;
	}

	SDE_DEBUG("About to read MISR values from %s\n", __func__);

	misr_read_values = &hfi_enc->sde_base->misr_vals;
	misr_data = (struct misr_read_data_ret *)payload;

	max_count = misr_data->num_misr;
	module_type = misr_data->module_type;
	SDE_DEBUG("Module_type:%d, Max_count:%d\n", module_type, max_count);
	misr_values = (u32 *)(payload + sizeof(u32) * 2);

	memset(&misr_read_values->misr_values, 0, sizeof(u32) * MAX_MISR_MODULES);

	for (int i = 0; i < max_count; i++)
		misr_read_values->misr_values[i] = misr_values[i];

	misr_read_values->count = max_count;

}

static int hfi_enc_debugfs_misr_read(struct sde_encoder_virt *enc)
{
	int rc = 0;
	struct hfi_cmdbuf_t *cmd_buf = NULL;
	struct drm_crtc *crtc;
	struct drm_encoder *drm_encoder;
	struct drm_connector *drm_connector;
	struct hfi_encoder *hfi_enc;
	struct hfi_kms *hfi_kms;
	struct misr_read_data misr_read;
	u32 disp_id;

	if (!enc) {
		SDE_ERROR("Invalid encoder\n");
		return -EINVAL;
	}

	crtc = enc->crtc;
	if (!crtc) {
		SDE_ERROR("Failed to get crtc\n");
		//return -EINVAL;
	}

	hfi_enc = to_hfi_encoder(enc);
	if (!hfi_enc)
		return -EINVAL;

	hfi_kms = to_hfi_kms(sde_encoder_get_kms(&enc->base));
	if (!hfi_kms) {
		SDE_ERROR("Failed to get hfi_kms\n");
		return -EINVAL;
	}

	/* Find connector to get disp_id */
	drm_encoder = &enc->base;
	drm_connector = sde_encoder_get_connector(drm_encoder->dev, drm_encoder);
	if (!drm_connector) {
		SDE_ERROR("Failed to get connector\n");
		return -EINVAL;
	}

	disp_id = sde_conn_get_display_obj_id(drm_connector);
	if (disp_id == U32_MAX) {
		SDE_ERROR("invalid display id\n");
		return -EINVAL;
	}

	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		SDE_ERROR("Failed to get valid command buffer\n");
		return -EINVAL;
	}

	misr_read.display_id = disp_id;
	misr_read.module_type = HFI_DEBUG_MISR_INTF;

	/* Listener init */
	hfi_enc->misr_read_listener.hfi_prop_handler = &hfi_enc_misr_read_hfi_prop_handler;

	rc = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEBUG_MISR_READ, disp_id,
			HFI_PAYLOAD_TYPE_U32_ARRAY, &misr_read, sizeof(misr_read),
			&hfi_enc->misr_read_listener, (HFI_HOST_FLAGS_RESPONSE_REQUIRED |
			HFI_HOST_FLAGS_NON_DISCARDABLE));

	SDE_EVT32(drm_encoder->base.id, disp_id, HFI_COMMAND_DEBUG_MISR_READ,
			SDE_EVTLOG_FUNC_CASE1);
	rc = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
	SDE_EVT32(drm_encoder->base.id, disp_id, HFI_COMMAND_DEBUG_MISR_READ, rc,
			SDE_EVTLOG_FUNC_CASE2);

	return rc;
}

u32 hfi_enc_get_vblank_count(struct sde_encoder_virt *enc)
{
	int cnt = 0;
	struct hfi_encoder *hfi_enc;

	if (!enc)
		return cnt;

	hfi_enc = to_hfi_encoder(enc);
	cnt = atomic_read(&hfi_enc->hfi_vsync_cnt);

	return cnt;
}

ktime_t hfi_enc_get_vblank_timestamp(struct sde_encoder_virt *enc)
{
	ktime_t ts = 0;
	struct hfi_encoder *hfi_enc;

	if (!enc)
		return ts;

	hfi_enc = to_hfi_encoder(enc);
	ts = hfi_enc->vblank_ts;

	return ts;
}

#else
static int hfi_enc_debugfs_dump_status(struct sde_encoder_virt *sde_enc, struct seq_file *s)
{
	return 0;
}

static int hfi_enc_debugfs_misr_setup(struct sde_encoder_virt *enc)
{
	return 0;
}

static int hfi_enc_debugfs_misr_read(struct sde_encoder_virt *enc)
{
	return 0;
}

u32 hfi_enc_get_vblank_count(struct sde_encoder_virt *enc)
{
	return 0;
}

ktime_t hfi_enc_get_vblank_timestamp(struct sde_encoder_virt *enc)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

static void _hfi_encoder_setup_ops(struct sde_encoder_virt *sde_enc)
{
	sde_enc->hal_ops.kickoff[MSM_DISP_OP_HFI] = hfi_enc_kickoff;
	sde_enc->hal_ops.encoder_enable[MSM_DISP_OP_HFI] = hfi_enc_encoder_enable;
	sde_enc->hal_ops.encoder_disable[MSM_DISP_OP_HFI] = hfi_enc_encoder_disable;
	sde_enc->hal_ops.wait_for_event[MSM_DISP_OP_HFI] = hfi_enc_wait_for_event;
	sde_enc->hal_ops.enable_hw_event[MSM_DISP_OP_HFI] = hfi_enc_enable_hw_event;
	sde_enc->hal_ops.debugfs_misr_setup[MSM_DISP_OP_HFI] = hfi_enc_debugfs_misr_setup;
	sde_enc->hal_ops.debugfs_misr_read[MSM_DISP_OP_HFI] = hfi_enc_debugfs_misr_read;
	sde_enc->hal_ops.debugfs_dump_status[MSM_DISP_OP_HFI] = hfi_enc_debugfs_dump_status;
	sde_enc->hal_ops.get_vblank_count[MSM_DISP_OP_HFI] = hfi_enc_get_vblank_count;
	sde_enc->hal_ops.get_vblank_timestamp[MSM_DISP_OP_HFI] = hfi_enc_get_vblank_timestamp;
}

int hfi_encoder_init(struct drm_device *dev, struct sde_encoder_virt *sde_enc)
{
	struct hfi_encoder *hfi_enc;

	hfi_enc = kvzalloc(sizeof(*hfi_enc), GFP_KERNEL);
	if (!hfi_enc) {
		SDE_ERROR("failed to allocate memory for hfi encoder\n");
		return -ENOMEM;
	}

	_hfi_encoder_setup_ops(sde_enc);
	hfi_enc->hfi_cb_obj.hfi_prop_handler = hfi_enc_hfi_prop_handler;
	init_waitqueue_head(&hfi_enc->pending_kickoff_wq);

	sde_enc->hfi_encoder = hfi_enc;
	hfi_enc->sde_base = sde_enc;

	return 0;
}
