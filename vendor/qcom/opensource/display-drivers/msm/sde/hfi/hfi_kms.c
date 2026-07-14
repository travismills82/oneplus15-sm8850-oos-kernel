// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include "sde_connector.h"
#include "hfi_crtc.h"
#include "hfi_kms.h"
#include "hfi_msm_drv.h"
#include "hfi_catalog.h"
#include "hfi_msm_drv.h"
#include "sde_encoder.h"
#include "sde_plane.h"
#include "sde_formats.h"
#include "hfi_utils.h"

#define DWORDS_TO_BYTES(x) (x * 4)
#define BYTES_TO_DWORDS(x) (x / 4)

/*
 * minimum required size for commn caps is 1 dword for number of caps &
 * at least 2 dwords to hold each key-value pair
 */
#define MIN_BYTES_FOR_COMMON_CAPS(x) DWORDS_TO_BYTES(x * 2)

/*
 * minimum required size for pipe caps is size of formats list + size of caps
 * size of fromats is 1 dword for format count + dword each for every format
 * and in best case with 0 property 1 dword for property count
 */
#define MIN_BYTES_FOR_PIPE_CAPS(x) DWORDS_TO_BYTES(1 + x[0] +  1)

static int hfi_kms_prepare_commit(struct sde_kms *kms,
		struct drm_atomic_state *state)
{
	int i, ret = 0;
	u32 disp_id;
	u32 encoder_mask;
	struct hfi_cmdbuf_t *cmd_buf;
	struct drm_encoder *encoder;
	struct drm_crtc *crtc;
	struct sde_crtc *sde_crtc;
	struct drm_crtc_state *cstate;
	struct hfi_kms *hfi_kms;

	if (!kms)
		return -EINVAL;

	hfi_kms = to_hfi_kms(kms);

	for_each_new_crtc_in_state(state, crtc, cstate, i) {
		sde_crtc = to_sde_crtc(crtc);

		encoder_mask = cstate->encoder_mask ?
			cstate->encoder_mask : sde_crtc->cached_encoder_mask;
		SDE_DEBUG("crtc:%d encoder_mask:0x%x\n", DRMID(crtc), encoder_mask);

		drm_for_each_encoder_mask(encoder, kms->dev, encoder_mask) {
			disp_id = hfi_crtc_get_display_id(crtc, cstate);
			if (disp_id == U32_MAX) {
				SDE_DEBUG("no display for encoder%p\n", encoder);
				continue;
			}
			SDE_DEBUG("creating cmd buffer for disp_id:%d\n", disp_id);

			cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
					disp_id, HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
			if (!cmd_buf) {
				SDE_ERROR("failed to get cmd_buf for crtc:%d disp_id:%d\n",
						DRMID(crtc), disp_id);
				return -EINVAL;
			}
		}
	}

	SDE_DEBUG("done\n");

	return ret;
}

static int hfi_kms_trigger_commit(struct sde_kms *kms,
		struct drm_atomic_state *state)
{
	int i, ret;
	u32 disp_id;
	u32 payload = HFI_COMMIT;
	struct hfi_cmdbuf_t *cmd_buf;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct hfi_kms *hfi_kms;
	struct drm_encoder *encoder;
	struct drm_device *dev;
	u32 pending_commit_count;

	if (!kms || !state)
		return -EINVAL;

	hfi_kms = to_hfi_kms(kms);

	SDE_EVT32(HFI_COMMAND_DISPLAY_FRAME_TRIGGER, SDE_EVTLOG_FUNC_ENTRY);
	for_each_new_crtc_in_state(state, crtc, crtc_state, i) {
		if (crtc->state->active || crtc_state->active || crtc_state->active_changed) {
			disp_id = hfi_crtc_get_display_id(crtc, crtc_state);
			if (disp_id == U32_MAX) {
				SDE_DEBUG("no valid display for crtc:%d\n", DRMID(crtc));
				continue;
			}
			SDE_DEBUG("getting cmd buffer for disp_id:%d\n", disp_id);

			cmd_buf = hfi_kms_get_cmd_buf(hfi_kms, disp_id,
					HFI_CMDBUF_TYPE_ATOMIC_COMMIT);
			if (!cmd_buf) {
				SDE_ERROR("failed to get cmd_buf for crtc:%d disp_id:%d\n",
						DRMID(crtc), disp_id);
				return -EINVAL;
			}

			ret = hfi_adapter_add_set_property(cmd_buf,
					HFI_COMMAND_DISPLAY_FRAME_TRIGGER, MSM_DRV_HFI_ID,
					HFI_PAYLOAD_TYPE_U32, &payload, sizeof(u32), 0);

			dev = crtc->dev;
			list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
				if (encoder->crtc != crtc)
					continue;

				pending_commit_count = sde_encoder_helper_inc_pending(encoder);
				SDE_EVT32(pending_commit_count);
			}

			ret = hfi_adapter_set_cmd_buf(cmd_buf);
			SDE_EVT32(HFI_COMMAND_DISPLAY_FRAME_TRIGGER, ret, SDE_EVTLOG_FUNC_CASE1);
			if (ret) {
				SDE_ERROR("failed to send commit buffer\n");
				return ret;
			}
		}
	}

	SDE_EVT32(HFI_COMMAND_DISPLAY_FRAME_TRIGGER, SDE_EVTLOG_FUNC_EXIT);
	return ret;
}

static int hfi_kms_commit(struct sde_kms *kms,
		struct drm_atomic_state *state)
{
	int i, ret = 0;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;

	if (!kms || !state)
		return -EINVAL;

	for_each_new_crtc_in_state(state, crtc, crtc_state, i) {
		if (crtc->state->active || crtc_state->active) {
			SDE_DEBUG(" crtc:%d\n", DRMID(crtc));
			sde_crtc_commit_kickoff(crtc, crtc_state);
		}
	}

	return ret;
}

static int hfi_kms_process_cmd_buf(struct hfi_client_t *client, struct hfi_cmdbuf_t *cmd_buf)
{
	int rc;

	SDE_DEBUG("process cmd-buf called\n");

	/*
	 * If no crtcs and encoders are initialized, cannot switch context
	 * Currently adapter thread is used, need to schedule unpack to right event thread
	 */
	rc = hfi_adapter_unpack_cmd_buf(client, cmd_buf);
	if (rc)
		SDE_ERROR("[WARNING] error in response packet or unpacking buffer\n");

	rc = hfi_adapter_release_cmd_buf(cmd_buf);
	if (rc)
		SDE_ERROR("[WARNING] Failed to release command buffer\n");

	return rc;
}

static const struct sde_kms_hal_funcs hfi_hal_funcs = {
	.prepare_commit[MSM_DISP_OP_HFI] = hfi_kms_prepare_commit,
	.commit[MSM_DISP_OP_HFI] = hfi_kms_commit,
	.trigger_commit[MSM_DISP_OP_HFI] = hfi_kms_trigger_commit,
};

static int _hfi_kms_setup_hfi(struct hfi_adapter_t *adapter, struct hfi_kms *hfi_kms)
{
	int ret;

	if (!adapter || !hfi_kms)
		return -EINVAL;

	hfi_kms->hfi_adapter = adapter;
	hfi_kms->hfi_client.process_cmd_buf = hfi_kms_process_cmd_buf;

	ret = hfi_adapter_client_register(hfi_kms->hfi_adapter, &hfi_kms->hfi_client);
	if (ret) {
		SDE_ERROR("failed to register as adapter client\n");
		return ret;
	}

	return ret;
}

struct hfi_cmdbuf_t *hfi_kms_get_cmd_buf(struct hfi_kms *hfi_kms,
		u16 display_id, u32 cmd_type)
{
	struct hfi_cmdbuf_t *ret_buf = NULL;
	struct hfi_cmdbuf_t *buf;
	struct hfi_client_t *hfi_client;

	if (!hfi_kms) {
		SDE_ERROR("Invalid hfi_kms\n");
		return NULL;
	}

	hfi_client = &hfi_kms->hfi_client;

	list_for_each_entry(buf, &hfi_client->cmd_buf_list, node) {
		if (buf->cmd_type == cmd_type && buf->obj_id == display_id) {
			ret_buf = buf;
			break;
		}
	}

	return ret_buf;
}

int hfi_kms_reg_client(struct drm_device *dev)
{
	int ret;
	struct msm_drm_hfi_private *hfi_drv_priv;
	struct sde_kms *sde_kms;
	struct msm_drm_private *priv;

	if (!dev || !dev->dev_private)
		return -EINVAL;

	priv = dev->dev_private;
	if (!priv || !priv->hfi_priv) {
		SDE_ERROR("Invalid priv");
		return -HFI_ERROR;
	}

	hfi_drv_priv = priv->hfi_priv;
	if (!hfi_drv_priv || !hfi_drv_priv->hfi_adapter) {
		SDE_ERROR("Invalid hfi_drv_priv");
		return -HFI_ERROR;
	}

	sde_kms = to_sde_kms(priv->kms);
	if (!sde_kms) {
		SDE_ERROR("Invalid sde_kms");
		return -HFI_ERROR;
	}

	ret = _hfi_kms_setup_hfi(hfi_drv_priv->hfi_adapter, sde_kms->hfi_kms);
	if (ret) {
		SDE_ERROR("failed to setup HFI client ret=%d\n", ret);
		return -HFI_ERROR;
	}

	return 0;

}

static u32 _hfi_kms_read_init_caps(struct hfi_catalog_base *catalog,
		u32 hfi_prop, u32 *payload, u32 max_words)
{
	u32 read = 0;
	u32 prop_id = HFI_PROP_ID(hfi_prop);
	u32 payload_size = HFI_PROP_SZ(hfi_prop);
	u32 vig_index_count;
	u32 dma_index_count;

	if (!catalog || !payload)
		return -EINVAL;

	SDE_INFO("read prop:0x%x size:%d\n", prop_id, payload_size);
	switch (prop_id) {
	case HFI_PROPERTY_DEVICE_INIT_DCP_HW_VERSION:
		catalog->dcp_hw_rev = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_MDSS_HW_VERSION:
		catalog->hw_rev = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_DCP_FW_VERSION:
		catalog->fw_rev = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_VIG_INDICES:
		if (payload[0] > SDE_MAX_SSPP_COUNT) {
			SDE_ERROR("invalid vig indices in the packet\n");
			return read;
		}
		vig_index_count = payload[read++];
		for (int i = 0; i < vig_index_count; i++, read++)
			if (REC_ID(payload[read]) == 0) {
				catalog->vig_indices[catalog->vig_count] = payload[read];
				catalog->vig_count++;
			} else {
				catalog->vig_r1_indices[catalog->virt_vig_count] = payload[read];
				catalog->virt_vig_count++;
			}
		break;
	case HFI_PROPERTY_DEVICE_INIT_DMA_INDICES:
		if (!max_words || max_words <=  payload[0] || payload[0] > SDE_MAX_SSPP_COUNT) {
			SDE_ERROR("invalid dma indices in the packet\n");
			return read;
		}

		dma_index_count = payload[read++];
		for (int i = 0; i < dma_index_count; i++, read++)
			if (REC_ID(payload[read]) == 0) {
				catalog->dma_indices[catalog->dma_count] = payload[read];
				catalog->dma_count++;
			} else {
				catalog->dma_r1_indices[catalog->virt_dma_count] = payload[read];
				catalog->virt_dma_count++;
			}
		return read;
	case HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT:
		catalog->max_display_count = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_WB_INDICES:
		if (payload[0] > MAX_BLOCKS) {
			SDE_ERROR("invalid wb indices in the packet\n");
			return read;
		}

		catalog->wb_count = payload[read++];
		for (int i = 0; i < catalog->wb_count; i++, read++)
			catalog->wb_indices[i] = payload[read];
		break;
	case HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION:
		catalog->max_wb_linear_resolution = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION:
		catalog->max_wb_ubwc_resolution = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_DSI_INDICES:
		if (payload[0] > MAX_BLOCKS) {
			SDE_ERROR("invalid dsi indices in the packet\n");
			return read;
		}

		catalog->dsi_count = payload[read++];
		for (int i = 0; i < catalog->dsi_count; i++, read++)
			catalog->dsi_indices[i] = payload[read];
		break;
	case HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION:
		catalog->max_dsi_resolution = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_DP_INDICES:
		if (payload[0] > MAX_BLOCKS) {
			SDE_ERROR("invalid dp indices in the packet\n");
			return read;
		}

		catalog->dp_count = payload[read++];
		for (int i = 0; i < catalog->dp_count; i++, read++)
			catalog->dp_indices[i] = payload[read];
		break;
	case HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION:
		catalog->max_dp_resolution = payload[read++];
		break;
	case HFI_PROPERTY_DEVICE_INIT_DS_INDICES:
		if (payload[0] > MAX_BLOCKS) {
			SDE_ERROR("invalid ds indices in the packet\n");
			return read;
		}

		catalog->ds_count = payload[read++];
		for (int i = 0; i < catalog->ds_count; i++, read++)
			catalog->ds_indices[i] = payload[read];
		return read;
	case HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION:
		catalog->max_ds_resolution = payload[read++];
		break;
	default:
		SDE_DEBUG("Unknown device cap key (%u)\n", prop_id);
	}

	if (read > payload_size) {
		SDE_ERROR("mismatch in read vs payload_size, prop:0x%x read:%d payload_size:%d\n",
				prop_id, read, payload_size);
	}
	return payload_size;
}

static int _hfi_kms_init_device_caps(struct hfi_catalog_base *catalog,
		void *payload, u32 size)
{
	int ret;
	u32 *value_ptr;
	u32 prop, max_words, last_read = 0;
	struct hfi_util_kv_parser kv_parser;

	ret  = hfi_util_kv_parser_init(&kv_parser, size, payload);
	if (ret) {
		SDE_ERROR("failed to get int prop parser\n");
		return ret;
	}

	ret = hfi_util_kv_parser_get_next(&kv_parser, last_read, &prop, &value_ptr, &max_words);
	if (ret) {
		SDE_ERROR("failed to get next prop\n");
		return ret;
	}

	SDE_INFO("prop:0x%x, max_words:%d\n", prop, max_words);

	while (prop && payload) {
		last_read = _hfi_kms_read_init_caps(catalog, prop, value_ptr, max_words);
		if (!last_read) {
			SDE_ERROR("failed to get next prop\n");
			return ret;
		}

		ret = hfi_util_kv_parser_get_next(&kv_parser, last_read, &prop,
				&value_ptr, &max_words);
		if (ret) {
			SDE_ERROR("failed to get next prop\n");
			return ret;
		}
	}
	return ret;
}

static void hfi_kms_populate_catalog(u32 display_id, u32 cmd_id,
		void *prop_data, u32 size, struct hfi_prop_listener *hfi_listener)
{
	struct hfi_kms *hfi_kms = container_of(hfi_listener, struct hfi_kms, device_init_listener);
	struct hfi_catalog_base *catalog;

	if (!hfi_kms) {
		SDE_ERROR("invalid object or listener from FW\n");
		return;
	}

	catalog = hfi_kms->catalog;
	if (!catalog) {
		SDE_ERROR("Catalog not yet initialized\n");
		return;
	}

	switch (cmd_id) {
	case HFI_COMMAND_DEVICE_INIT:
		_hfi_kms_init_device_caps(catalog, prop_data, size);
		break;
	case HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS:
		_hfi_kms_init_device_caps(catalog, prop_data, size);
		atomic_inc(&hfi_kms->cat_init_done);
		break;
	case HFI_COMMAND_DEVICE_INIT_VIG_CAPS:
	case HFI_COMMAND_DEVICE_INIT_DMA_CAPS:
	case HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS:
	case HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS:
	case HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS:
	case HFI_COMMAND_DEVICE_RESOURCE_REGISTER:
	case HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE:
	case HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS:
	case HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS:
		break;
	default:
		SDE_ERROR("command:0x%x not supported\n", cmd_id);
	}
}

static int _send_device_init_cmd(struct hfi_kms *hfi_kms)
{
	int ret;
	struct hfi_cmdbuf_t *cmd_buf;
	bool cat_done = false;
	bool wait_count = false;

	if (!hfi_kms)
		return -EINVAL;

	SDE_EVT32(HFI_COMMAND_DEVICE_INIT, SDE_EVTLOG_FUNC_ENTRY);
	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_DEVICE_INFO);
	SDE_EVT32(HFI_COMMAND_DEVICE_INIT, SDE_EVTLOG_FUNC_CASE1);
	if (!cmd_buf) {
		SDE_ERROR("failed to get hfi command buffer\n");
		return -EINVAL;
	}

	hfi_kms->device_init_listener.hfi_prop_handler = hfi_kms_populate_catalog;
	ret = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEVICE_INIT,
			MSM_DRV_HFI_ID, HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			&hfi_kms->device_init_listener,
			HFI_HOST_FLAGS_RESPONSE_REQUIRED | HFI_HOST_FLAGS_NON_DISCARDABLE);
	if (ret) {
		SDE_ERROR("failed to add device-init command\n");
		return ret;
	}

	SDE_EVT32(HFI_COMMAND_DEVICE_INIT, SDE_EVTLOG_FUNC_CASE2);
	ret = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
	SDE_EVT32(HFI_COMMAND_DEVICE_INIT, ret, SDE_EVTLOG_FUNC_CASE3);
	if (ret) {
		SDE_ERROR("failed to send device-init command\n");
		return ret;
	}

	cat_done = atomic_read(&hfi_kms->cat_init_done);
	if (!cat_done)
		SDE_ERROR("failed to parse catalog\n");

	SDE_DEBUG("catalog wait success after :%d ms\n", wait_count);
	SDE_EVT32(HFI_COMMAND_DEVICE_INIT, SDE_EVTLOG_FUNC_EXIT);
	return ret;
}

int hfi_kms_get_plane_indices(struct hfi_kms *hfi_kms, bool vig_pipe,
		uint32_t pipe_idx, bool rect1, uint32_t *hfi_pipe_id)
{
	if (vig_pipe && !rect1) {
		if (hfi_kms->catalog->vig_count >= pipe_idx)
			*hfi_pipe_id = hfi_kms->catalog->vig_indices[pipe_idx];
		else
			goto fail;
	} else if (vig_pipe && rect1) {
		if (hfi_kms->catalog->virt_vig_count >= pipe_idx)
			*hfi_pipe_id = hfi_kms->catalog->vig_r1_indices[pipe_idx];
		else
			goto fail;
	} else if (!vig_pipe && !rect1) {
		if (hfi_kms->catalog->dma_count >= pipe_idx)
			*hfi_pipe_id = hfi_kms->catalog->dma_indices[pipe_idx];
		else
			goto fail;
	} else if (!vig_pipe && rect1) {
		if (hfi_kms->catalog->virt_dma_count >= pipe_idx)
			*hfi_pipe_id = hfi_kms->catalog->dma_r1_indices[pipe_idx];
		else
			goto fail;
	} else {
		goto fail;
	}

	return 0;

fail:
	*hfi_pipe_id = 0xffffff;
	return -EINVAL;
}

int hfi_kms_get_catalog_data(struct hfi_kms *hfi_kms)
{
	int ret = 0;

	ret = _send_device_init_cmd(hfi_kms);
	if (ret) {
		SDE_ERROR("failed to send device-init\n");
		return ret;
	}

	return ret;
}

int hfi_kms_set_reg_dma_buffer(struct hfi_kms *hfi_kms, struct sde_reg_dma_buffer *buffer)
{
	int ret;
	struct hfi_cmdbuf_t *cmd_buf;
	struct hfi_buff_dpu hfi_cfg = {};

	if (!hfi_kms || !buffer)
		return -EINVAL;

	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_DEVICE_INFO);
	if (!cmd_buf) {
		SDE_ERROR("failed to get hfi command buffer\n");
		return -EINVAL;
	}

	hfi_cfg.iova = buffer->iova;
	hfi_cfg.len = buffer->index;

	ret = hfi_adapter_add_set_property(cmd_buf, HFI_COMMAND_DEVICE_LUT_DMA_LAST_CMD,
			MSM_DRV_HFI_ID, HFI_PAYLOAD_TYPE_U32_ARRAY, &hfi_cfg, sizeof(hfi_cfg),
			HFI_HOST_FLAGS_NONE);
	if (ret) {
		SDE_ERROR("failed to set hfi property for last command buffer\n");
		return -EINVAL;
	}

	ret = hfi_adapter_set_cmd_buf(cmd_buf);
	if (ret) {
		SDE_ERROR("failed to send DEVICE_REG_DMA_LAST_CMD\n");
		return ret;
	}

	return ret;
}

int hfi_kms_init(struct sde_kms *sde_kms)
{
	struct hfi_kms *hfi_kms;
	struct hfi_catalog_base *hfi_cfg;

	if (!sde_kms)
		return -EINVAL;

	hfi_kms = kvzalloc(sizeof(*hfi_kms), GFP_KERNEL);
	if (!hfi_kms) {
		SDE_ERROR("failed to allocate hfi_kms\n");
		return -ENOMEM;
	}
	hfi_cfg = kvzalloc(sizeof(*hfi_cfg), GFP_KERNEL);
	if (!hfi_cfg)
		return -ENOMEM;

	hfi_kms->catalog = hfi_cfg;
	atomic_set(&hfi_kms->cat_init_done, 0);
	sde_kms->hfi_kms = hfi_kms;
	sde_kms->hal_ops = hfi_hal_funcs;
	hfi_kms->base = sde_kms;

	return 0;
}

void hfi_kms_resource_vote_hfi_prop_handler(u32 obj_uid, u32 CMD_ID, void *payload, u32 size,
			struct hfi_prop_listener *resource_vote_listener)
{
	struct msm_drm_private *priv;
	struct sde_kms *sde_kms;
	struct hfi_kms *hfi_kms;
	u32 bus_id, i, prop_count;
	struct hfi_kv_pairs *kv_pairs;
	struct hfi_bw_config bw_config;
	struct dss_module_power *mp;
	struct sde_power_handle *phandle;
	u32 dirty[SDE_POWER_HANDLE_DBUS_ID_MAX] = {0,};
	u32 rc;
	u32 enable;

	if (!payload || !resource_vote_listener)
		return;

	hfi_kms = container_of(resource_vote_listener, struct hfi_kms, resource_vote_listener);

	sde_kms = hfi_kms->base;
	priv = sde_kms->dev->dev_private;
	phandle = &priv->phandle;
	mp = &phandle->mp;

	if (CMD_ID == HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE) {
		prop_count = size/sizeof(struct hfi_kv_pairs);
		kv_pairs = payload;
		//HFI_UNPACK_KEY(kv_pairs[i].key, property_id, version, size);
		for (i = 0; i < prop_count; i++) {
			switch (kv_pairs[i].key) {
			case HFI_PROPERTY_DEVICE_CORE_IB:
				bw_config.ib_vote[SDE_POWER_HANDLE_DBUS_ID_MNOC] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_MNOC] = 1;
				break;
			case HFI_PROPERTY_DEVICE_CORE_AB:
				bw_config.ab_vote[SDE_POWER_HANDLE_DBUS_ID_MNOC] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_MNOC] = 1;
				break;
			case HFI_PROPERTY_DEVICE_LLCC_IB:
				bw_config.ib_vote[SDE_POWER_HANDLE_DBUS_ID_LLCC] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_LLCC] = 1;
				break;
			case HFI_PROPERTY_DEVICE_LLCC_AB:
				bw_config.ab_vote[SDE_POWER_HANDLE_DBUS_ID_LLCC] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_LLCC] = 1;
				break;
			case HFI_PROPERTY_DEVICE_DRAM_IB:
				bw_config.ib_vote[SDE_POWER_HANDLE_DBUS_ID_EBI] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_EBI] = 1;
				break;
			case HFI_PROPERTY_DEVICE_DRAM_AB:
				bw_config.ab_vote[SDE_POWER_HANDLE_DBUS_ID_EBI] =
					*(u64 *)kv_pairs[i].value_ptr;
				dirty[SDE_POWER_HANDLE_DBUS_ID_EBI] = 1;
				break;
			case HFI_PROPERTY_DEVICE_CORE_POWER_RAIL:
				enable = *(u32 *)kv_pairs[i].value_ptr;
				rc = msm_dss_enable_vreg(mp->vreg_config, mp->num_vreg,
						enable);
				break;
			}
		}

		for (bus_id = 0; bus_id < SDE_POWER_HANDLE_DBUS_ID_MAX; bus_id++) {
			if (dirty[bus_id]) {
				rc = sde_power_data_bus_set_quota(phandle,
						bus_id,
						bw_config.ab_vote[bus_id],
						bw_config.ib_vote[bus_id]);
				if (rc)
					pr_err("set quota failed\n");
			}
		}
	}
}
