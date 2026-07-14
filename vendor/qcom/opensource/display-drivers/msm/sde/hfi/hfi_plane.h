/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_PLANE_H_
#define _HFI_PLANE_H_

#include "hfi_adapter.h"
#include "sde_plane.h"
#include "hfi_utils.h"

#define HFI_POPULATE_RECT(rect, a, b, c, d, Q16_flag) \
	do {						\
		(rect)->x_pos = (Q16_flag) ? (a) >> 16 : (a);    \
		(rect)->y_pos = (Q16_flag) ? (b) >> 16 : (b);    \
		(rect)->width = (Q16_flag) ? (c) >> 16 : (c);    \
		(rect)->height = (Q16_flag) ? (d) >> 16 : (d);    \
	} while (0)

/**
 * struct hfi_plane - hfi extension of sde plane structure
 * @sde_base: Poniter to base sde plane object
 * @hfi_pipe_id: pipe id from hfi catalog data
 * @hfi_lock: Mutex to protect hfi plane specific data
 * @base_props: prop helper object for intermediate property collection
 * @color_props: color prop helper object for intermediate property collection
 * @kv_props: kv pair helper object for intermediate property collection
 */
struct hfi_plane {
	struct sde_plane *sde_base;

	uint32_t hfi_pipe_id;
	struct mutex hfi_lock;
	struct hfi_util_u32_prop_helper *base_props;
	struct hfi_util_u32_prop_helper *color_props;
	struct hfi_util_kv_helper *kv_props;
};

/**
 * struct hfi_plane_state - hfi extension of sde plane state object
 * @sde_base: Pointer to base sde plane state object
 */
struct hfi_plane_state {
	struct sde_plane_state *sde_base;
};

#if IS_ENABLED(CONFIG_MDSS_HFI)
/**
 * hfi_plane_init - create hfi plane object
 * @pipe_id:  sde hardware pipe identifier
 * @pdpu: Pointer to sde plane struct
 * @Returns: 0 on success, or error code on failure
 */
int hfi_plane_init(uint32_t pipe_id, struct sde_plane *pdpu);

/**
 * hfi_plane_disable - disable plane
 * @cmd_buf: command buffer to use to set the disable property hfi
 * @disp_id: display id
 * @plane: plane to disable within the given display
 * @use_lock: boolean to indicate if api should use hfi mutex lock
 */
void hfi_plane_disable(struct hfi_cmdbuf_t *cmd_buf, u32 disp_id, struct sde_plane *plane,
	bool use_lock);
#else
int hfi_plane_init(uint32_t pipe_id, struct sde_plane *pdpu)
{
	return -HFI_ERROR;
}

void hfi_plane_disable(struct hfi_cmdbuf_t *cmd_buf, u32 disp_id, struct sde_plane *plane,
	bool use_lock);
{
	return -HFI_ERROR;
}
#endif // CONFIG_MDSS_HFI

#endif  // _SDE_PLANE_HFI_H_
