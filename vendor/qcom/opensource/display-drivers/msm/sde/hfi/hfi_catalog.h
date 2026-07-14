/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_CATALOG_H__
#define __HFI_CATALOG_H__

#include "sde_kms.h"
#include "sde_formats.h"
#include "hfi_defs_common.h"

/*
 * hfi_format_map - maps for drm format to hfi format
 *
 * @hfi_format		HFI base pixel format
 * @drm_format		DRM base pixel format
 * @modifier		drm format modifier
 */
struct hfi_format_map {
	u32 hfi_format;
	u32 drm_format;
	u64 modifier;
};

/**
 * hfi_get_extened_format - Get SDE format for HFI format
 * @hfi_fmt: HFI format
 * Returns: Returns sde format structure
 */
struct sde_format_extended hfi_get_extened_format(u32 hfi_fmt);

/**
 * hfi_catalog_get_hfi_format - Get HFI format for HFI format
 * @sde_fmt: sde format structure
 * Returns: Returns hfi format
 */
u32 hfi_catalog_get_hfi_format(struct sde_format_extended *sde_fmt);

#endif // __HFI_CATALOG_H__
