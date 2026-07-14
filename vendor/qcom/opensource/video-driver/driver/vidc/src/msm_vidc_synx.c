// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <synx_api.h>
#include <synx_hwfence.h>

#include "msm_vidc_core.h"
#include "msm_vidc_driver.h"
#include "msm_vidc_internal.h"
#include "msm_vidc_fence.h"
#include "msm_vidc_synx.h"
#include "msm_vidc_debug.h"

#define MSM_VIDC_SYNX_FENCE_CLIENT_ID         SYNX_CLIENT_HW_FENCE_VID_CTX0
#define MSM_VIDC_SYNX_CREATE_DMA_FENCE        SYNX_CREATE_DMA_FENCE
#define MSM_VIDC_SYNX_IMPORT_DMA_FENCE        SYNX_IMPORT_DMA_FENCE
#define MSM_VIDC_SYNX_IMPORT_INDV_PARAMS      SYNX_IMPORT_INDV_PARAMS
#define MSM_VIDC_SYNX_STATE_SIGNALED_SUCCESS  SYNX_STATE_SIGNALED_SUCCESS

#define MAX_SYNX_FENCE_SESSION_NAME        64

/**
 * Recommended fence flow from hwfence driver is,
 * Producer side:
 *		- dma_fence_init()
 *		- synx_create()
 *		- synx_signal() (Either from FW/HLOS)
 *		- synx_release()
 *		- dma_fence_signal()
 *		- dma_fence_put()
 *
 * Consumer side:
 *		- dma_fence_get()
 *		- synx_import()
 *		- synx_release()
 *		- dma_fence_put()
 */

static int msm_vidc_synx_fence_register(struct msm_vidc_core *core)
{
	struct synx_initialization_params params;
	struct synx_session *session = NULL;
	char synx_session_name[MAX_SYNX_FENCE_SESSION_NAME];
	struct synx_queue_desc queue_desc;
	int rc = 0;

	if (!core->capabilities[SUPPORTS_SYNX_V2_FENCE].value)
		return 0;

	/* fill synx_initialization_params */
	memset(&params, 0, sizeof(struct synx_initialization_params));
	memset(&queue_desc, 0, sizeof(struct synx_queue_desc));

	params.id = (enum synx_client_id)MSM_VIDC_SYNX_FENCE_CLIENT_ID;
	snprintf(synx_session_name, MAX_SYNX_FENCE_SESSION_NAME,
		"video synx fence");
	params.name = synx_session_name;
	params.ptr = &queue_desc;
	params.flags = SYNX_INIT_MAX; /* unused */

	session =
		(struct synx_session *)synx_initialize(&params);
	if (IS_ERR_OR_NULL(session)) {
		d_vpr_e("%s: invalid synx fence session\n", __func__);
		rc = PTR_ERR(session) ? PTR_ERR(session) : -EINVAL;
		goto error;
	}

	/* fill core synx fence data */
	core->synx_fence_data.client_id = (u32)params.id;
	core->synx_fence_data.client_flags = (u32)params.flags;
	core->synx_fence_data.session = (void *)session;
	core->synx_fence_data.queue.size = (u32)queue_desc.size;
	core->synx_fence_data.queue.kvaddr = queue_desc.vaddr;
	core->synx_fence_data.queue.phys_addr = (phys_addr_t)queue_desc.dev_addr;

	core->synx_fence_data.queue.type = MSM_VIDC_BUF_INTERFACE_QUEUE;
	core->synx_fence_data.queue.region = MSM_VIDC_NON_SECURE;
	core->synx_fence_data.queue.direction = DMA_BIDIRECTIONAL;

	d_vpr_h("%s: successfully registered synx fence\n", __func__);
	return rc;

error:
	d_vpr_e("%s: failed. Disable Synx_V2 support\n", __func__);
	core->capabilities[SUPPORTS_SYNX_V2_FENCE].value = 0;
	return rc;
}

static int msm_vidc_synx_fence_deregister(struct msm_vidc_core *core)
{
	int rc = 0;

	if (!core->capabilities[SUPPORTS_SYNX_V2_FENCE].value)
		return 0;

	rc = synx_uninitialize(
		(struct synx_session *)core->synx_fence_data.session);
	if (rc) {
		d_vpr_e("%s: failed to deregister synx fence\n", __func__);
		/* ignore error */
		rc = 0;
	} else {
		d_vpr_h("%s: successfully deregistered synx fence\n", __func__);
	}

	core->synx_fence_data.session = NULL;

	return rc;
}

static struct msm_vidc_fence *msm_vidc_synx_fence_create(
	struct msm_vidc_inst *inst, struct msm_vidc_fence_context *f_context,
	enum msm_vidc_fence_type f_type, enum msm_vidc_fence_direction f_dir, int fence_fd)
{
	struct msm_vidc_core *core = inst->core;
	struct msm_vidc_fence *fence = NULL;
	struct synx_create_params params;
	u32 fence_id = 0;
	int rc = 0;

	if (!core->synx_fence_data.session) {
		i_vpr_e(inst, "%s: invalid synx fence session\n", __func__);
		return NULL;
	}

	/* create dma fence */
	fence = msm_vidc_get_sw_fence(inst, f_context, f_type, f_dir, fence_fd);
	if (!fence) {
		i_vpr_e(inst, "%s: failed to create sw fence\n", __func__);
		return NULL;
	}

	if (is_sw_fence(inst, fence->type))
		goto exit;

	/* hande hw fence case */
	memset(&params, 0, sizeof(struct synx_create_params));
	params.name = fence->name;
	params.fence = (void *)&fence->dma_fence;
	params.h_synx = &fence_id;
	params.flags = MSM_VIDC_SYNX_CREATE_DMA_FENCE;

	/* create hw fence */
	rc = synx_create(
		(struct synx_session *)core->synx_fence_data.session,
		&params);
	if (rc) {
		i_vpr_e(inst, "%s: failed to create hw fence %s\n",
			__func__, fence->name);
		goto put_sw_fence;
	}

	fence->fence_id = (u64)(*(params.h_synx));
	fence->session = core->synx_fence_data.session;
	/* prepare hw fence name */
	populate_fence_name(inst, fence, false);

exit:
	return fence;

put_sw_fence:
	msm_vidc_fence_signal(inst, fence, true);
	msm_vidc_put_sw_fence(inst, fence);
	return NULL;
}

static struct msm_vidc_fence *msm_vidc_synx_fence_import(
	struct msm_vidc_inst *inst, struct msm_vidc_fence_context *f_context,
	enum msm_vidc_fence_type f_type, enum msm_vidc_fence_direction f_dir, int fence_fd)
{
	struct msm_vidc_core *core = inst->core;
	struct msm_vidc_fence *fence = NULL;
	struct synx_import_params params;
	u32 fence_id = 0;
	int rc = 0;

	if (!core->synx_fence_data.session) {
		i_vpr_e(inst, "%s: invalid synx fence session\n", __func__);
		return NULL;
	}

	/* import dma fence */
	fence = msm_vidc_get_sw_fence(inst, f_context, f_type, f_dir, fence_fd);
	if (!fence) {
		i_vpr_e(inst, "%s: failed to import sw fence\n", __func__);
		return NULL;
	}

	/* handle hw fence case */
	memset(&params, 0, sizeof(struct synx_import_params));
	params.type = MSM_VIDC_SYNX_IMPORT_INDV_PARAMS;
	params.indv.fence = (void *)fence->imp_fence;
	params.indv.new_h_synx = &fence_id;
	params.indv.flags = MSM_VIDC_SYNX_IMPORT_DMA_FENCE;

	/* import hw fence */
	rc = synx_import((struct synx_session *)core->synx_fence_data.session, &params);
	if (rc) {
		i_vpr_e(inst, "%s: failed to import hw fence %s\n",
			__func__, fence->name);
		goto put_sw_fence;
	}

	fence->fence_id = (u64)(*(params.indv.new_h_synx));
	fence->session = core->synx_fence_data.session;
	/* prepare hw fence name */
	populate_fence_name(inst, fence, false);

	return fence;

put_sw_fence:
	msm_vidc_put_sw_fence(inst, fence);
	return NULL;
}

static int msm_vidc_synx_fence_create_fd(struct msm_vidc_inst *inst,
	struct msm_vidc_fence *fence)
{
	int rc = 0;

	rc = msm_vidc_get_sw_fence_fd(inst, fence);
	if (rc) {
		i_vpr_e(inst, "%s: failed. %s\n", __func__, fence->name);
		return rc;
	}

	return rc;
}

static int msm_vidc_synx_fence_destroy_fd(struct msm_vidc_inst *inst,
	struct msm_vidc_fence *fence)
{
	int rc = 0;

	rc = msm_vidc_put_sw_fence_fd(inst, fence);
	if (rc) {
		i_vpr_e(inst, "%s: failed. %s\n", __func__, fence->name);
		return rc;
	}

	return rc;
}

static int msm_vidc_synx_fence_destroy(struct msm_vidc_inst *inst,
	struct msm_vidc_fence *fence, bool is_error)
{
	int rc = 0;

	/* sanity - calling fence_destroy for imp fence not expected */
	if (fence->imp_fence) {
		i_vpr_e(inst, "%s: unexpected. name %s\n", __func__, fence->name);
		return -EINVAL;
	}

	if (fence->session) {
		/* signal hw fence, incase if there any error from fw */
		if (is_error) {
			rc = synx_signal((struct synx_session *)fence->session,
				(u32)fence->fence_id, MSM_VIDC_SYNX_STATE_SIGNALED_SUCCESS);
			if (rc)
				i_vpr_e(inst, "%s: failed to signal hw fence %s\n",
					__func__, fence->name);
		}

		/* destroy hw fence */
		rc = synx_release((struct synx_session *)fence->session,
			(u32)fence->fence_id);
		if (rc)
			d_vpr_e("%s: failed to destroy hw fence %s\n",
				__func__, fence->name);
	}

	/* signal sw fence(in normal flow) */
	rc = msm_vidc_fence_signal(inst, fence, is_error);
	if (rc) {
		i_vpr_e(inst, "%s: synx signal failed. name %s\n", __func__, fence->name);
		return rc;
	}

	/* destroy sw fence */
	rc = msm_vidc_put_sw_fence(inst, fence);
	if (rc)
		return rc;

	return rc;
}

static int msm_vidc_synx_fence_enable_resources(struct msm_vidc_core *core, bool state)
{
	int rc = 0;

	if (!core->capabilities[SUPPORTS_REMOTE_PROC].value)
		return 0;

	if ((state && is_core_sub_state(core, CORE_SUBSTATE_RPROC_ENABLE)) ||
	    (!state && !is_core_sub_state(core, CORE_SUBSTATE_RPROC_ENABLE))) {
		d_vpr_e("%s: unexpected. state %d, substate %d\n", __func__,
			state, is_core_sub_state(core, CORE_SUBSTATE_RPROC_ENABLE));
		return -EINVAL;
	}

	rc = synx_enable_resources(MSM_VIDC_SYNX_FENCE_CLIENT_ID, SYNX_RESOURCE_SOCCP, state);
	if (rc) {
		d_vpr_e("%s: failed to %s\n", __func__, state ? "enable" : "disable");
		return rc;
	}

	if (state)
		msm_vidc_change_core_sub_state(core, 0, CORE_SUBSTATE_RPROC_ENABLE, __func__);
	else
		msm_vidc_change_core_sub_state(core, CORE_SUBSTATE_RPROC_ENABLE, 0, __func__);

	return rc;
}

static int msm_vidc_synx_fence_release(struct msm_vidc_inst *inst,
	struct msm_vidc_fence *fence, bool is_error)
{
	int rc = 0;

	/* sanity - calling fence_release for raw fence not expected */
	if (!fence->imp_fence) {
		i_vpr_e(inst, "%s: unexpected. name %s\n", __func__, fence->name);
		return -EINVAL;
	}

	/* destroy hw fence */
	if (fence->session) {
		rc = synx_release((struct synx_session *)fence->session,
			(u32)fence->fence_id);
		if (rc)
			d_vpr_e("%s: failed to destroy hw fence %s\n",
				__func__, fence->name);
	}

	/* destroy sw fence */
	rc = msm_vidc_put_sw_fence(inst, fence);
	if (rc)
		return rc;

	return rc;
}

static void msm_vidc_synx_fence_recover(struct msm_vidc_core *core)
{
	int rc = 0;

	rc = synx_recover((enum synx_client_id)core->synx_fence_data.client_id);
	if (rc)
		d_vpr_e("%s: failed to recover hw fences. client id: %d",
			__func__,
			(enum synx_client_id)core->synx_fence_data.client_id);
}

const struct msm_vidc_fence_ops *get_synx_fence_ops(void)
{
	static struct msm_vidc_fence_ops synx_ops;

	synx_ops.fence_register          = msm_vidc_synx_fence_register;
	synx_ops.fence_deregister        = msm_vidc_synx_fence_deregister;
	synx_ops.fence_create            = msm_vidc_synx_fence_create;
	synx_ops.fence_import            = msm_vidc_synx_fence_import;
	synx_ops.fence_create_fd         = msm_vidc_synx_fence_create_fd;
	synx_ops.fence_destroy_fd        = msm_vidc_synx_fence_destroy_fd;
	synx_ops.fence_destroy           = msm_vidc_synx_fence_destroy;
	synx_ops.fence_release           = msm_vidc_synx_fence_release;
	synx_ops.fence_recover           = msm_vidc_synx_fence_recover;
	synx_ops.fence_enable_resources  = msm_vidc_synx_fence_enable_resources;

	return &synx_ops;
}
