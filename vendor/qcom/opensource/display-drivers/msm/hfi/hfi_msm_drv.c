// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include "hfi_msm_drv.h"

#define MSM_DRV_HFI_ADAPTER 1

int hfi_msm_drv_hfi_init(struct msm_drm_private *priv)
{
	struct msm_drm_hfi_private *hfi_priv = priv->hfi_priv;
	int rc = 0;

	if (!hfi_priv)
		return -EINVAL;

	hfi_priv->hfi_adapter = hfi_adapter_init(MSM_DRV_HFI_ADAPTER);
	if (!hfi_priv->hfi_adapter) {
		rc = -EINVAL;
		DRM_ERROR("failed to initialize HFI adapter: %d\n", rc);
		return rc;
	}
	DRM_DEBUG("success to initialize HFI adapter: %d\n", rc);

	return rc;
}

int hfi_msm_drv_init(struct drm_device *ddev)
{
	struct msm_drm_hfi_private *hfi_priv;
	struct msm_drm_private *priv;

	if (!ddev)
		return -EINVAL;

	hfi_priv = kvzalloc(sizeof(*hfi_priv), GFP_KERNEL);
	if (!hfi_priv)
		return -EINVAL;

	priv = ddev->dev_private;
	hfi_priv->base = priv;
	priv->hfi_priv = hfi_priv;

	return 0;
}
