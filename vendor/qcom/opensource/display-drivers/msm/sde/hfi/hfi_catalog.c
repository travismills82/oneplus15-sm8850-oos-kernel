// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include <linux/types.h>
#include "hfi_catalog.h"

static const struct hfi_format_map sde_linear_fmt_map[] = {
	{HFI_COLOR_FORMAT_RGB565, DRM_FORMAT_BGR565, 0},
	{HFI_COLOR_FORMAT_RGB888, DRM_FORMAT_BGR888, 0},
	{HFI_COLOR_FORMAT_ARGB8888, DRM_FORMAT_BGRA8888, 0},
	{HFI_COLOR_FORMAT_RGBA8888, DRM_FORMAT_ABGR8888, 0},
	{HFI_COLOR_FORMAT_XRGB8888, DRM_FORMAT_BGRX8888, 0},
	{HFI_COLOR_FORMAT_RGBX8888, DRM_FORMAT_XBGR8888, 0},
	{HFI_COLOR_FORMAT_ARGB1555, DRM_FORMAT_BGRA5551, 0},
	{HFI_COLOR_FORMAT_RGBA5551, DRM_FORMAT_ABGR1555, 0},
	{HFI_COLOR_FORMAT_XRGB1555, DRM_FORMAT_BGRX5551, 0},
	{HFI_COLOR_FORMAT_RGBX5551, DRM_FORMAT_XBGR1555, 0},
	{HFI_COLOR_FORMAT_ARGB4444, DRM_FORMAT_BGRA4444, 0},
	{HFI_COLOR_FORMAT_RGBA4444, DRM_FORMAT_ABGR4444, 0},
	{HFI_COLOR_FORMAT_RGBX4444, DRM_FORMAT_XBGR4444, 0},
	{HFI_COLOR_FORMAT_XRGB4444, DRM_FORMAT_BGRX4444, 0},
	{HFI_COLOR_FORMAT_ARGB2_10_10_10, DRM_FORMAT_BGRA1010102, 0},
	{HFI_COLOR_FORMAT_XRGB2_10_10_10, DRM_FORMAT_BGRX1010102, 0},
	{HFI_COLOR_FORMAT_RGBA10_10_10_2, DRM_FORMAT_ABGR2101010, 0},
	{HFI_COLOR_FORMAT_RGBX10_10_10_2, DRM_FORMAT_XBGR2101010, 0},
	{HFI_COLOR_FORMAT_BGR565, DRM_FORMAT_RGB565, 0},
	{HFI_COLOR_FORMAT_BGR888, DRM_FORMAT_RGB888, 0},
	{HFI_COLOR_FORMAT_ABGR8888, DRM_FORMAT_RGBA8888, 0},
	{HFI_COLOR_FORMAT_BGRA8888, DRM_FORMAT_ARGB8888, 0},
	{HFI_COLOR_FORMAT_BGRX8888, DRM_FORMAT_XRGB8888, 0},
	{HFI_COLOR_FORMAT_XBGR8888, DRM_FORMAT_RGBX8888, 0},
	{HFI_COLOR_FORMAT_ABGR1555, DRM_FORMAT_RGBA5551, 0},
	{HFI_COLOR_FORMAT_BGRA5551, DRM_FORMAT_ARGB1555, 0},
	{HFI_COLOR_FORMAT_XBGR1555, DRM_FORMAT_RGBX5551, 0},
	{HFI_COLOR_FORMAT_BGRX5551, DRM_FORMAT_XRGB1555, 0},
	{HFI_COLOR_FORMAT_ABGR4444, DRM_FORMAT_RGBA4444, 0},
	{HFI_COLOR_FORMAT_BGRA4444, DRM_FORMAT_ARGB4444, 0},
	{HFI_COLOR_FORMAT_BGRX4444, DRM_FORMAT_XRGB4444, 0},
	{HFI_COLOR_FORMAT_XBGR4444, DRM_FORMAT_RGBX4444, 0},
	{HFI_COLOR_FORMAT_ABGR2_10_10_10, DRM_FORMAT_RGBA1010102, 0},
	{HFI_COLOR_FORMAT_XBGR2_10_10_10, DRM_FORMAT_RGBX1010102, 0},
	{HFI_COLOR_FORMAT_BGRA10_10_10_2, DRM_FORMAT_ARGB2101010, 0},
	{HFI_COLOR_FORMAT_BGRX10_10_10_2, DRM_FORMAT_XRGB2101010, 0},
	{HFI_COLOR_FORMAT_RGBA_FP_16, DRM_FORMAT_ABGR16161616F, 0},
	{HFI_COLOR_FORMAT_BGRA16_FP_16, DRM_FORMAT_ARGB16161616F, 0},
	{HFI_COLOR_FORMAT_YU12, DRM_FORMAT_YUV420, 0},
	{HFI_COLOR_FORMAT_YV12, DRM_FORMAT_YVU420, 0},
	{HFI_COLOR_FORMAT_NV12, DRM_FORMAT_NV12, 0},
	{HFI_COLOR_FORMAT_NV21, DRM_FORMAT_NV21, 0},
	{HFI_COLOR_FORMAT_YUYV, DRM_FORMAT_YUYV, 0},
	{HFI_COLOR_FORMAT_YVYU, DRM_FORMAT_YVYU, 0},
	{HFI_COLOR_FORMAT_UYVY, DRM_FORMAT_UYVY, 0},
	{HFI_COLOR_FORMAT_VYUY, DRM_FORMAT_VYUY, 0},
};

static const struct hfi_format_map sde_non_linear_fmt_map[] = {
	{HFI_COLOR_FORMAT_RGBA8888_TILED, DRM_FORMAT_ABGR8888,
		DRM_FORMAT_MOD_QCOM_TILE},
	{HFI_COLOR_FORMAT_RGBX8888_TILED, DRM_FORMAT_XBGR8888,
		DRM_FORMAT_MOD_QCOM_TILE},
	{HFI_COLOR_FORMAT_RGBA10_10_10_2_TILED, DRM_FORMAT_ABGR2101010,
		DRM_FORMAT_MOD_QCOM_TILE},
	{HFI_COLOR_FORMAT_RGBX10_10_10_2_TILED, DRM_FORMAT_XBGR2101010,
		DRM_FORMAT_MOD_QCOM_TILE},
	{HFI_COLOR_FORMAT_ABGR16_FP_16, DRM_FORMAT_ARGB16161616F,
		DRM_FORMAT_MOD_QCOM_ALPHA_SWAP},
	{HFI_COLOR_FORMAT_ARGB_FP_16, DRM_FORMAT_ABGR16161616F,
		DRM_FORMAT_MOD_QCOM_ALPHA_SWAP},
	{HFI_COLOR_FORMAT_UBWC_RGBA8888, DRM_FORMAT_ABGR8888,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGBA10_10_10_2, DRM_FORMAT_ABGR2101010,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGBA16_16_16_16_F, DRM_FORMAT_ABGR16161616F,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGB565, DRM_FORMAT_BGR565,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGBX8888, DRM_FORMAT_XBGR8888,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGBX10_10_10_2, DRM_FORMAT_XBGR2101010,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_RGBA8888_L_8_5, DRM_FORMAT_ABGR8888,
		DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_8_5},
	{HFI_COLOR_FORMAT_UBWC_RGBA8888_L_2_1, DRM_FORMAT_ABGR8888,
		DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_2_1},
	{HFI_COLOR_FORMAT_P010, DRM_FORMAT_NV12,
		DRM_FORMAT_MOD_QCOM_DX},
	{HFI_COLOR_FORMAT_UBWC_NV12, DRM_FORMAT_NV12,
		DRM_FORMAT_MOD_QCOM_COMPRESSED},
	{HFI_COLOR_FORMAT_UBWC_TP10, DRM_FORMAT_NV12,
		DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_DX |
		DRM_FORMAT_MOD_QCOM_TIGHT},
	{HFI_COLOR_FORMAT_UBWC_P010, DRM_FORMAT_NV12,
		DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_DX},
};

static inline bool sde_format_lookup(u32 hfi_fmt,
	const struct hfi_format_map *sde_fmt_map, u32 fmt_map_size,
	struct sde_format_extended *sde_fmt)
{
	bool found = false;

	for (int i = 0; i < fmt_map_size; i++) {
		if (sde_fmt_map[i].hfi_format == hfi_fmt) {
			sde_fmt->fourcc_format = sde_fmt_map[i].drm_format;
			sde_fmt->modifier = sde_fmt_map[i].modifier;
			found = true;
			break;
		}
	}

	return found;
}

struct sde_format_extended hfi_get_extened_format(u32 hfi_fmt)
{
	struct sde_format_extended sde_fmt = {0};
	bool fmt_found = false;

	fmt_found = sde_format_lookup(hfi_fmt, sde_linear_fmt_map,
			ARRAY_SIZE(sde_linear_fmt_map), &sde_fmt);
	if (fmt_found && sde_fmt.fourcc_format != 0)
		goto end;

	fmt_found = sde_format_lookup(hfi_fmt, sde_non_linear_fmt_map,
			ARRAY_SIZE(sde_non_linear_fmt_map), &sde_fmt);
	if (!fmt_found || !sde_fmt.fourcc_format)
		SDE_ERROR("No valid SDE format for given HFI format");

end:
	return sde_fmt;
}

static inline u32 hfi_format_lookup(struct sde_format_extended *sde_fmt,
		const struct hfi_format_map *sde_fmt_map, u32 fmt_map_size)
{
	for (int i = 0; i < fmt_map_size; i++) {
		if (sde_fmt_map[i].drm_format == sde_fmt->fourcc_format &&
				sde_fmt_map[i].modifier == sde_fmt->modifier) {
			return sde_fmt_map[i].hfi_format;
		}
	}

	return U32_MAX;
}

u32 hfi_catalog_get_hfi_format(struct sde_format_extended *sde_fmt)
{
	u32 hfi_fmt = U32_MAX;

	if (!sde_fmt) {
		SDE_ERROR("No valid SDE format for given HFI format");
		return hfi_fmt;
	}

	switch (sde_fmt->modifier) {
	case 0:
		hfi_fmt = hfi_format_lookup(sde_fmt, sde_linear_fmt_map,
					ARRAY_SIZE(sde_linear_fmt_map));
		break;

	default:
		hfi_fmt = hfi_format_lookup(sde_fmt, sde_non_linear_fmt_map,
					ARRAY_SIZE(sde_non_linear_fmt_map));
	}

	if (hfi_fmt == U32_MAX)
		SDE_ERROR("No valid HFI format for given SDE format 0x%x\t, fmt modifier: 0x%llx\n",
				sde_fmt->fourcc_format, sde_fmt->modifier);

	return hfi_fmt;
}
