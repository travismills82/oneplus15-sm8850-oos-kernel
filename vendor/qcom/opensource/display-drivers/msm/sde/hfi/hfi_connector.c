// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include "hfi_connector.h"
#include "hfi_kms.h"
#include "hfi_crtc.h"
#include "hfi_props.h"

#define HFI_CONNECTOR_ID(c) ((c)->sde_base->base.base.id)

#define HFI_DEBUG_CONN(c, fmt, ...) SDE_DEBUG("conn%d " fmt,\
		(c) ? HFI_CONNECTOR_ID(c) : -1, ##__VA_ARGS__)

#define HFI_ERROR_CONN(c, fmt, ...) SDE_ERROR("conn%d " fmt,\
		(c) ? HFI_CONNECTOR_ID(c) : -1, ##__VA_ARGS__)

#define to_hfi_connector(x) x->hfi_conn

#define HFI_CONNECTOR_MAX_PROPS 128
#define HFI_CONNECTOR_BASE_PROP_MAX_SIZE 1024

struct hfi_prop_map {
	u32 drm_prop;
	u32 hfi_prop;
	void (*add_hfi_prop)(u32 hfi_prop, struct sde_connector *conn,
			struct sde_connector_state *old_state,
			struct hfi_cmdbuf_t *cmd_buf);
};

/*
 * struct base_prop_lookup: tuple of drm property ID to HFI property ID
 */
struct base_prop_lookup {
	u32 drm_prop;
	u32 hfi_prop;
};

struct base_prop_lookup hfi_connector_base_props_map[] = {
	{ CONNECTOR_PROP_DYN_BIT_CLK, HFI_PROPERTY_DISPLAY_DYN_CLK_SUPPORT },
	{ CONNECTOR_PROP_QSYNC_MODE, HFI_PROPERTY_DISPLAY_QSYNC },
	{ CONNECTOR_PROP_AVR_STEP_STATE, HFI_PROPERTY_DISPLAY_AVR_STEP },

	// wb specific properties
	{ CONNECTOR_PROP_PP_CWB_DITHER, HFI_PROPERTY_DISPLAY_WB_CWB_DITHER },
	{ CONNECTOR_PROP_WB_ROT_TYPE, HFI_PROPERTY_DISPLAY_WB_LINEAR_ROTATION },
};

struct hfi_prop_map hfi_connector_kv_props_map[] = {
	{ CONNECTOR_PROP_ROI_V1, HFI_PROPERTY_LAYER_BLEND_ROI, sde_connector_add_roi_v1 },
};

void sde_connector_add_roi_v1(u32 hfi_prop, struct sde_connector *conn,
		struct sde_connector_state *old_state, struct hfi_cmdbuf_t *cmd_buf)
{
	struct msm_roi_list msm_roi = old_state->rois;
	struct hfi_connector *hfi_conn;
	u32 key;
	int ret = 0;

	if (!conn || !cmd_buf || !old_state)
		return;

	hfi_conn = to_hfi_connector(conn);
	if (!msm_property_is_dirty(&conn->property_info,
				&old_state->property_state,
				CONNECTOR_PROP_ROI_V1)) {
		HFI_DEBUG_CONN(hfi_conn, "not dirty %s : %d\n", __func__, __LINE__);
	}

	if (old_state->rois.num_rects == 0)
		HFI_DEBUG_CONN(hfi_conn, "num rects = 0 %s : %d\n", __func__, __LINE__);

	return;

	key = HFI_PACKKEY(HFI_PROPERTY_LAYER_BLEND_ROI, 0, sizeof(msm_roi));

	ret = hfi_util_kv_helper_add(hfi_conn->kv_props, key, (u32 *)&msm_roi);
	if (ret)
		HFI_ERROR_CONN(hfi_conn, "failed adding HFI KV prop:0x%x\n", hfi_prop);
}

int _hfi_connector_add_base_prop_helper(u32 hfi_prop, struct sde_connector *conn,
		struct sde_connector_state *old_cstate,
		struct hfi_util_u32_prop_helper *prop_collector)
{
	u32 val;
	struct sde_connector_state *state;
	struct hfi_connector *hfi_conn;
	int ret = 0;

	if (!conn || !old_cstate || !prop_collector || conn->base.state)
		return -EINVAL;

	state = (struct sde_connector_state *)conn->base.state;
	hfi_conn = to_hfi_connector(conn);

	switch (hfi_prop) {
	case HFI_PROPERTY_DISPLAY_DYN_CLK_SUPPORT:
		val = sde_connector_get_property(&old_cstate->base, CONNECTOR_PROP_DYN_BIT_CLK);
		break;
	case HFI_PROPERTY_DISPLAY_QSYNC:
		val = sde_connector_get_property(&old_cstate->base, CONNECTOR_PROP_QSYNC_MODE);
		break;
	case HFI_PROPERTY_DISPLAY_AVR_STEP:
		val = sde_connector_get_property(&old_cstate->base, CONNECTOR_PROP_AVR_STEP_STATE);
		break;
	case HFI_PROPERTY_DISPLAY_WB_CWB_DITHER:
		val = sde_connector_get_property(&old_cstate->base, CONNECTOR_PROP_PP_CWB_DITHER);
		break;
	case HFI_PROPERTY_DISPLAY_WB_LINEAR_ROTATION:
		val = sde_connector_get_property(&old_cstate->base, CONNECTOR_PROP_WB_ROT_TYPE);
		break;
	default:
		HFI_ERROR_CONN(hfi_conn, "failed to send HFI commands\n");
		return -EINVAL;
	}

	ret = hfi_util_u32_prop_helper_add_prop_by_obj(prop_collector,
			hfi_prop, conn->base.base.id, HFI_VAL_U32, &val, sizeof(u32));
	if (ret)
		HFI_ERROR_CONN(hfi_conn, "failed adding HFI prop:0x%x\n", hfi_prop);

	HFI_DEBUG_CONN(hfi_conn, "done adding HFI prop:0x%x\n", hfi_prop);

	return 0;
}

/**
 * hfi_connector_populate_custom_kv_setter_props:  this is for all basic payloads.
 * Collects all listed props into a linear memory and voids memcopy of value by value at adapeter
 */
static int _hfi_connector_set_props_base(struct sde_connector *conn, u32 disp_id,
		struct sde_connector_state *old_cstate, struct hfi_cmdbuf_t *cmd_buf)
{
	u32 drm_prop, hfi_prop;
	int i, ret = 0;
	struct hfi_connector *hfi_conn = to_hfi_connector(conn);

	if (!hfi_conn || !hfi_conn->base_props) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	mutex_lock(&hfi_conn->hfi_lock);
	hfi_util_u32_prop_helper_reset(hfi_conn->base_props);

	/* append msm properties */
	for (i = 0; i < ARRAY_SIZE(hfi_connector_base_props_map); i++) {
		drm_prop = hfi_connector_base_props_map[i].drm_prop;
		hfi_prop = hfi_connector_base_props_map[i].hfi_prop;
		if (!sde_connector_property_is_dirty(old_cstate, drm_prop))
			continue;

		_hfi_connector_add_base_prop_helper(hfi_prop, conn, old_cstate,
				 hfi_conn->base_props);
	}

	if (!hfi_util_u32_prop_helper_prop_count(hfi_conn->base_props))
		goto end;

	/*
	 * Once all the key value pairs of properties are collected invoke adapter api
	 * to add all these property array as a single HFI Packet
	 */
	ret = hfi_adapter_add_set_property(cmd_buf,
			HFI_COMMAND_DISPLAY_SET_PROPERTY,
			disp_id,
			HFI_PAYLOAD_TYPE_U32_ARRAY,
			hfi_util_u32_prop_helper_get_payload_addr(hfi_conn->base_props),
			hfi_util_u32_prop_helper_get_size(hfi_conn->base_props),
			HFI_HOST_FLAGS_NON_DISCARDABLE);
	if (ret) {
		HFI_ERROR_CONN(hfi_conn, "failed to send HFI commands\n");
		goto end;
	}

	HFI_DEBUG_CONN(hfi_conn, "done adding all base props\n");

end:
	mutex_unlock(&hfi_conn->hfi_lock);

	return ret;
}

/**
 * hfi_connector_populate_custom_kv_setter_props:  this is for large payloads.
 * Collects all listed props to provide as key-value pairs and adapter does memcopy
 */
int hfi_connector_populate_custom_kv_setter_props(struct sde_connector *conn, u32 disp_id,
		struct sde_connector_state *old_cstate, struct hfi_cmdbuf_t *cmd_buf)
{
	struct hfi_prop_map *setter;
	int i, ret = 0;
	struct hfi_connector *hfi_conn = to_hfi_connector(conn);
	u32 kv_count;

	if (!hfi_conn || !old_cstate || !cmd_buf) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	mutex_lock(&hfi_conn->hfi_lock);
	hfi_util_kv_helper_reset(hfi_conn->kv_props);

	for (i = 0; i < ARRAY_SIZE(hfi_connector_kv_props_map); i++) {
		setter = &hfi_connector_kv_props_map[i];
		if (!sde_connector_property_is_dirty(old_cstate, setter->drm_prop)) {
			HFI_DEBUG_CONN(hfi_conn, "conn prop is not dirty\n");
			continue;
		}

		if (setter->add_hfi_prop)
			setter->add_hfi_prop(setter->hfi_prop, conn, old_cstate, cmd_buf);
	}

	kv_count = hfi_util_kv_helper_get_count(hfi_conn->kv_props);
	if (!kv_count)
		goto end;

	ret = hfi_adapter_add_prop_array(cmd_buf,
			HFI_COMMAND_DISPLAY_SET_PROPERTY,
			disp_id,
			HFI_PAYLOAD_TYPE_U32_ARRAY,
			hfi_util_kv_helper_get_payload_addr(hfi_conn->kv_props),
			kv_count,
			kv_count * sizeof(struct hfi_kv_pairs));
	if (ret) {
		HFI_DEBUG_CONN(hfi_conn, "failed to send HFI commands\n");
		goto end;
	}

end:
	HFI_DEBUG_CONN(hfi_conn, "adding kv-props count%d\n", kv_count);
	mutex_unlock(&hfi_conn->hfi_lock);

	return ret;
}

int _hfi_connector_populate_props(struct hfi_cmdbuf_t *cmd_buf, u32 disp_id,
		struct sde_connector *conn, struct sde_connector_state *old_cstate)
{
	int ret = 0;
	struct hfi_connector *hfi_conn = to_hfi_connector(conn);

	if (!hfi_conn) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	ret = _hfi_connector_set_props_base(conn, disp_id, old_cstate, cmd_buf);
	if (ret) {
		HFI_ERROR_CONN(hfi_conn, "failed to send hfis\n");
		return ret;
	}

	ret = hfi_connector_populate_custom_kv_setter_props(conn, disp_id, old_cstate, cmd_buf);
	if (ret) {
		HFI_ERROR_CONN(hfi_conn, "failed to send hfis\n");
		return ret;
	}

	return ret;
}

void hfi_connector_destroy(struct sde_connector *conn)
{
	struct hfi_connector *conn_hfi = (struct hfi_connector *)to_hfi_connector(conn);

	if (!conn_hfi) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	kfree(conn_hfi->base_props);
	kfree(conn_hfi->kv_props);

	mutex_destroy(&conn_hfi->hfi_lock);

	kfree(conn_hfi);
}

static int hfi_conn_add_hfi_cmds(struct hfi_cmdbuf_t *cmd_buf, u32 disp_id,
		struct sde_connector *conn, struct sde_connector_state *cstate)
{
	int ret = 0;

	if (!conn || !cmd_buf || !cstate) {
		SDE_ERROR("invalid args\n");
		return -EINVAL;
	}

	ret = _hfi_connector_populate_props(cmd_buf, disp_id, conn, cstate);

	return ret;
}

int hfi_connector_prepare_commit(struct drm_connector *conn, struct sde_connector_state *cstate)
{
	int ret = 0;
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	struct hfi_cmdbuf_t *cmd_buf;
	struct sde_connector *sde_conn;
	u32 disp_id;

	if (!conn) {
		SDE_ERROR("invalid args\n");
		return -EINVAL;
	}

	sde_conn = to_sde_connector(conn);
	sde_kms = sde_connector_get_kms(conn);
	hfi_kms = to_hfi_kms(sde_kms);

	disp_id = sde_conn_get_display_obj_id(conn);
	cmd_buf = hfi_kms_get_cmd_buf(hfi_kms, disp_id, HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
	if (!cmd_buf) {
		SDE_ERROR("conn:%d failed to get cmd-buf in commit path\n", DRMID(conn));
		return -EINVAL;
	}

	ret = hfi_conn_add_hfi_cmds(cmd_buf, disp_id, sde_conn, cstate);
	if (ret) {
		SDE_ERROR("failed to add hfi cmds\n");
		return ret;
	}

	return ret;
}

static void _sde_connector_hal_funcs_install(struct sde_connector *conn)
{
	if (!conn) {
		SDE_ERROR("invalid args\n");
		return;
	}

	conn->hal_ops.destroy[MSM_DISP_OP_HFI] = hfi_connector_destroy;
	conn->hal_ops.prepare_commit[MSM_DISP_OP_HFI] = hfi_connector_prepare_commit;
}

struct hfi_cmdbuf_t *hfi_connector_get_cmd_buf(struct drm_connector *drm_conn,
		u32 cmd_buf_type)
{
	struct sde_kms *sde_kms = sde_connector_get_kms(drm_conn);
	struct sde_connector *sde_conn = to_sde_connector(drm_conn);
	struct hfi_kms *hfi_kms;

	if (!sde_kms || !sde_conn) {
		SDE_ERROR("invalid connector\n");
		return NULL;
	}

	hfi_kms = to_hfi_kms(sde_kms);

	return hfi_kms_get_cmd_buf(hfi_kms, sde_conn->conn_id, cmd_buf_type);
}

int hfi_connector_init(int connector_type, struct sde_connector *c_conn)
{
	struct hfi_connector *hfi_conn = NULL;

	hfi_conn = kvzalloc(sizeof(*hfi_conn), GFP_KERNEL);
	if (!hfi_conn) {
		SDE_ERROR("[%u] failed to allocate memory for hfi connector\n", connector_type);
		return -ENOMEM;
	}

	mutex_init(&hfi_conn->hfi_lock);

	_sde_connector_hal_funcs_install(c_conn);

	hfi_conn->kv_props = hfi_util_kv_helper_alloc(HFI_CONNECTOR_MAX_PROPS);
	if (IS_ERR(hfi_conn->kv_props)) {
		SDE_ERROR("failed to allocate memory for base property collector\n");
		goto free_conn;
	}

	hfi_conn->base_props = hfi_util_u32_prop_helper_alloc(HFI_CONNECTOR_BASE_PROP_MAX_SIZE);
	if (IS_ERR(hfi_conn->base_props)) {
		SDE_ERROR("failed to allocate memory for base prop collector\n");
		goto free_kv;
	}

	hfi_conn->sde_base = c_conn;
	c_conn->hfi_conn = hfi_conn;
	return 0;

free_kv:
	kfree(hfi_conn->base_props);
free_conn:
	mutex_destroy(&hfi_conn->hfi_lock);
	kfree(hfi_conn);

	return -ENOMEM;
}
