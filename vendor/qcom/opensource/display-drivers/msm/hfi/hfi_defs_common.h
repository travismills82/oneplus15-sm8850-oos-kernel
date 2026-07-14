/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_DEFS_COMMON_H__
#define __H_HFI_DEFS_COMMON_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * HFI_FALSE    (Disables / do not support the property)
 * HFI_TRUE     (Enables / supports the property)
 */
#define HFI_FALSE		0
#define HFI_TRUE		1

/*
 * Note: 1st MSB byte determines the type of color format
 * // Interleaved RGB (MSB byte = 0x01)
 * @HFI_COLOR_FORMAT_RGB565                : Color format RGB565
 * @HFI_COLOR_FORMAT_RGB888                : Color format RGB888
 * @HFI_COLOR_FORMAT_ARGB8888              : Color format ARGB8888
 * @HFI_COLOR_FORMAT_RGBA8888              : Color format RGBA8888
 * @HFI_COLOR_FORMAT_XRGB8888              : Color format XRGB8888
 * @HFI_COLOR_FORMAT_RGBX8888              : Color format RGBX8888
 * @HFI_COLOR_FORMAT_ARGB1555              : Color format ARGB1555
 * @HFI_COLOR_FORMAT_RGBA5551              : Color format RGBA5551
 * @HFI_COLOR_FORMAT_XRGB1555              : Color format XRGB1555
 * @HFI_COLOR_FORMAT_RGBX5551              : Color format RGBX5551
 * @HFI_COLOR_FORMAT_ARGB4444              : Color format ARGB4444
 * @HFI_COLOR_FORMAT_RGBA4444              : Color format RGBA4444
 * @HFI_COLOR_FORMAT_RGBX4444              : Color format RGBX4444
 * @HFI_COLOR_FORMAT_XRGB4444              : Color format XRGB4444
 * @HFI_COLOR_FORMAT_ARGB2_10_10_10        : Color format ARGB2 10 10 10
 * @HFI_COLOR_FORMAT_XRGB2_10_10_10        : Color format XRGB2 10 10 10
 * @HFI_COLOR_FORMAT_RGBA10_10_10_2        : Color format RGBA10 10 10 2
 * @HFI_COLOR_FORMAT_RGBX10_10_10_2        : Color format RGBX10 10 10 2
 * @HFI_COLOR_FORMAT_ARGB_FP_16            : Color format ARGB16 16 16 16 Float
 * @HFI_COLOR_FORMAT_RGBA_FP_16            : Color format RGBA16 16 16 16 Float
 * @HFI_COLOR_FORMAT_RGBA8888_TILED        : Color format ABGR8888 Tiled
 * @HFI_COLOR_FORMAT_RGBX8888_TILED        : Color format XBGR8888 Tiled
 * @HFI_COLOR_FORMAT_RGBA10_10_10_2_TILED  : Color format ABGR2 10 10 10 Tiled
 * @HFI_COLOR_FORMAT_RGBX10_10_10_2_TILED  : Color format XBGR2 10 10 10 Tiled
 * // Interleaved BGR (MSB byte = 0x02)
 * @HFI_COLOR_FORMAT_BGR565                : Color format BGR565
 * @HFI_COLOR_FORMAT_BGR888                : Color format BGR888
 * @HFI_COLOR_FORMAT_ABGR8888              : Color format ABGR8888
 * @HFI_COLOR_FORMAT_BGRA8888              : Color format BGRA8888
 * @HFI_COLOR_FORMAT_BGRX8888              : Color format BGRX8888
 * @HFI_COLOR_FORMAT_XBGR8888              : Color format XBGR8888
 * @HFI_COLOR_FORMAT_ABGR1555              : Color format ABGR1555
 * @HFI_COLOR_FORMAT_BGRA5551              : Color format BGRA5551
 * @HFI_COLOR_FORMAT_XBGR1555              : Color format XBGR1555
 * @HFI_COLOR_FORMAT_BGRX5551              : Color format BGRX5551
 * @HFI_COLOR_FORMAT_ABGR4444              : Color format ABGR4444
 * @HFI_COLOR_FORMAT_BGRA4444              : Color format BGRA4444
 * @HFI_COLOR_FORMAT_BGRX4444              : Color format BGRX4444
 * @HFI_COLOR_FORMAT_XBGR4444              : Color format XBGR4444
 * @HFI_COLOR_FORMAT_ABGR2_10_10_10        : Color format ABGR2 10 10 10
 * @HFI_COLOR_FORMAT_XBGR2_10_10_10        : Color format XBGR2 10 10 10
 * @HFI_COLOR_FORMAT_BGRA10_10_10_2        : Color format BGRA10 10 10 2
 * @HFI_COLOR_FORMAT_BGRX10_10_10_2        : Color format BGRX10 10 10 2
 * @HFI_COLOR_FORMAT_ABGR_FP_16            : Color format ABGR16 16 16 16 Float
 * @HFI_COLOR_FORMAT_BGRA_FP_16            : Color format BGRA16 16 16 16 Float
 * // Compressed RGBA UBWC 3.0/ 4.x / 5.0 Lossless  (MSB byte = 0x03)
 * @HFI_COLOR_FORMAT_UBWC_RGBA8888         : Color format UBWC RGBA8888
 * @HFI_COLOR_FORMAT_UBWC_RGBA10_10_10_2   : Color format UBWC RGBA10 10 10 2
 * @HFI_COLOR_FORMAT_UBWC_RGBA16_16_16_16_F: Color format UBWC RGBA16_16 16 16
 * @HFI_COLOR_FORMAT_UBWC_RGB565           : Color format UBWC RGB565
 * @HFI_COLOR_FORMAT_UBWC_RGBX8888         : Color format UBWC RGBX8888
 * @HFI_COLOR_FORMAT_UBWC_RGBX10_10_10_2   : Color format UBWC RGBX10_10_10_2
 * // Compressed RGBA UBWC 5.0 Lossy  (MSB byte = 0x04)
 * @HFI_COLOR_FORMAT_UBWC_RGBA8888_L_8_5   : Color format UBWC RGBA8888 8:5
 * @HFI_COLOR_FORMAT_UBWC_RGBA8888_L_2_1   : Color format UBWC RGBA8888 2:1
 * // Linear / Uncompressed YUV420 (MSB byte = 0x05)
 * @HFI_COLOR_FORMAT_YU12                  : Color format YU12
 * @HFI_COLOR_FORMAT_YV12                  : Color format YV12
 * @HFI_COLOR_FORMAT_NV12                  : Color format NV12
 * @HFI_COLOR_FORMAT_NV21                  : Color format NV21
 * @HFI_COLOR_FORMAT_P010                  : Color format P010
 * // Compressed UBWC 3.0 / 4.x / 5.0 Lossless (MSB byte = 0x06)
 * @HFI_COLOR_FORMAT_UBWC_NV12             : Color format UBWC NV12
 * @HFI_COLOR_FORMAT_UBWC_NV12_Interlace   : Color format UBWC NV12 Interlace
 * @HFI_COLOR_FORMAT_UBWC_TP10             : Color format UBWC TP10
 * @HFI_COLOR_FORMAT_UBWC_P010             : Color format UBWC P010
 * // Compressed UBWC 3.0 / 4.x / 5.0 Lossy (MSB byte = 0x07)
 * @HFI_COLOR_FORMAT_UBWC_NV12_LOSSY       : Color format UBWC NV12 Lossy
 * @HFI_COLOR_FORMAT_UBWC_TP10_LOSSY       : Color format UBWC TP10 Lossy
 */
enum hfi_color_formats {
	/* Interleaved RGB */
	HFI_COLOR_FORMAT_INTERLEAVED_RGB_MIN        = 0x01000000,
	HFI_COLOR_FORMAT_RGB565                     = 0x01000001,
	HFI_COLOR_FORMAT_RGB888                     = 0x01000002,
	HFI_COLOR_FORMAT_ARGB8888                   = 0x01000003,
	HFI_COLOR_FORMAT_RGBA8888                   = 0x01000004,
	HFI_COLOR_FORMAT_XRGB8888                   = 0x01000005,
	HFI_COLOR_FORMAT_RGBX8888                   = 0x01000006,
	HFI_COLOR_FORMAT_ARGB1555                   = 0x01000007,
	HFI_COLOR_FORMAT_RGBA5551                   = 0x01000008,
	HFI_COLOR_FORMAT_XRGB1555                   = 0x01000009,
	HFI_COLOR_FORMAT_RGBX5551                   = 0x0100000A,
	HFI_COLOR_FORMAT_ARGB4444                   = 0x0100000B,
	HFI_COLOR_FORMAT_RGBA4444                   = 0x0100000C,
	HFI_COLOR_FORMAT_RGBX4444                   = 0x0100000D,
	HFI_COLOR_FORMAT_XRGB4444                   = 0x0100000E,
	HFI_COLOR_FORMAT_ARGB2_10_10_10             = 0x0100000F,
	HFI_COLOR_FORMAT_XRGB2_10_10_10             = 0x01000010,
	HFI_COLOR_FORMAT_RGBA10_10_10_2             = 0x01000011,
	HFI_COLOR_FORMAT_RGBX10_10_10_2             = 0x01000012,
	HFI_COLOR_FORMAT_ARGB_FP_16                 = 0x01000013,
	HFI_COLOR_FORMAT_RGBA_FP_16                 = 0x01000014,
	HFI_COLOR_FORMAT_RGBA8888_TILED             = 0x01000015,
	HFI_COLOR_FORMAT_RGBX8888_TILED             = 0x01000016,
	HFI_COLOR_FORMAT_RGBA10_10_10_2_TILED       = 0x01000017,
	HFI_COLOR_FORMAT_RGBX10_10_10_2_TILED       = 0x01000018,
	HFI_COLOR_FORMAT_INTERLEAVED_RGB_MAX        = 0x01FFFFFF,
	/* Interleaved BGR */
	HFI_COLOR_FORMAT_INTERLEAVED_BGR_MIN        = 0x02000000,
	HFI_COLOR_FORMAT_BGR565                     = 0x02000001,
	HFI_COLOR_FORMAT_BGR888                     = 0x02000002,
	HFI_COLOR_FORMAT_ABGR8888                   = 0x02000003,
	HFI_COLOR_FORMAT_BGRA8888                   = 0x02000004,
	HFI_COLOR_FORMAT_BGRX8888                   = 0x02000005,
	HFI_COLOR_FORMAT_XBGR8888                   = 0x02000006,
	HFI_COLOR_FORMAT_ABGR1555                   = 0x02000007,
	HFI_COLOR_FORMAT_BGRA5551                   = 0x02000008,
	HFI_COLOR_FORMAT_XBGR1555                   = 0x02000009,
	HFI_COLOR_FORMAT_BGRX5551                   = 0x0200000A,
	HFI_COLOR_FORMAT_ABGR4444                   = 0x0200000B,
	HFI_COLOR_FORMAT_BGRA4444                   = 0x0200000C,
	HFI_COLOR_FORMAT_BGRX4444                   = 0x0200000D,
	HFI_COLOR_FORMAT_XBGR4444                   = 0x0200000E,
	HFI_COLOR_FORMAT_ABGR2_10_10_10             = 0x0200000F,
	HFI_COLOR_FORMAT_XBGR2_10_10_10             = 0x02000010,
	HFI_COLOR_FORMAT_BGRA10_10_10_2             = 0x02000011,
	HFI_COLOR_FORMAT_BGRX10_10_10_2             = 0x02000012,
	HFI_COLOR_FORMAT_ABGR16_FP_16               = 0x02000013,
	HFI_COLOR_FORMAT_BGRA16_FP_16               = 0x02000014,
	HFI_COLOR_FORMAT_INTERLEAVED_BGR_MAX        = 0x02FFFFFF,
	/* Compressed RGBA UBWC 3.0/ 4.x / 5.0 Lossless */
	HFI_COLOR_FORMAT_RGBA_UBWC_LOSSLESS_MIN     = 0x03000000,
	HFI_COLOR_FORMAT_UBWC_RGBA8888              = 0x03000001,
	HFI_COLOR_FORMAT_UBWC_RGBA10_10_10_2        = 0x03000002,
	HFI_COLOR_FORMAT_UBWC_RGBA16_16_16_16_F     = 0x03000003,
	HFI_COLOR_FORMAT_UBWC_RGB565                = 0x03000004,
	HFI_COLOR_FORMAT_UBWC_RGBX8888              = 0x03000005,
	HFI_COLOR_FORMAT_UBWC_RGBX10_10_10_2        = 0x03000006,
	HFI_COLOR_FORMAT_RGBA_UBWC_LOSSLESS_MAX     = 0x03FFFFFF,
	/* Compressed RGBA UBWC 5.0 Lossy */
	HFI_COLOR_FORMAT_RGBA_UBWC_LOSSY_MIN        = 0x04000000,
	HFI_COLOR_FORMAT_UBWC_RGBA8888_L_8_5        = 0x04000001,
	HFI_COLOR_FORMAT_UBWC_RGBA8888_L_2_1        = 0x04000002,
	HFI_COLOR_FORMAT_RGBA_UBWC_LOSSY_MAX        = 0x04FFFFFF,
	/* Linear / Uncompressed YUV420 */
	HFI_COLOR_FORMAT_LINEAR_MIN                 = 0x05000000,
	HFI_COLOR_FORMAT_YU12                       = 0x05000001,
	HFI_COLOR_FORMAT_YV12                       = 0x05000002,
	HFI_COLOR_FORMAT_NV12                       = 0x05000003,
	HFI_COLOR_FORMAT_NV21                       = 0x05000004,
	HFI_COLOR_FORMAT_P010                       = 0x05000005,
	HFI_COLOR_FORMAT_YUYV                       = 0x05000006,
	HFI_COLOR_FORMAT_YVYU                       = 0x05000007,
	HFI_COLOR_FORMAT_UYVY                       = 0x05000008,
	HFI_COLOR_FORMAT_VYUY                       = 0x05000009,
	HFI_COLOR_FORMAT_LINEAR_MAX                 = 0x05FFFFFF,
	/* Compressed UBWC 3.0 / 4.x / 5.0 Lossless */
	HFI_COLOR_FORMAT_UBWC_LOSSLESS_MIN          = 0x06000000,
	HFI_COLOR_FORMAT_UBWC_NV12                  = 0x06000001,
	HFI_COLOR_FORMAT_UBWC_NV12_Interlace        = 0x06000002,
	HFI_COLOR_FORMAT_UBWC_TP10                  = 0x06000003,
	HFI_COLOR_FORMAT_UBWC_P010                  = 0x06000004,
	HFI_COLOR_FORMAT_UBWC_LOSSLESS_MAX          = 0x06FFFFFF,
	/* Compressed UBWC 3.0 / 4.x / 5.0 Lossy */
	HFI_COLOR_FORMAT_UBWC_LOSSY_MIN             = 0x07000000,
	HFI_COLOR_FORMAT_UBWC_NV12_LOSSY            = 0x07000001,
	HFI_COLOR_FORMAT_UBWC_TP10_LOSSY            = 0x07000002,
	HFI_COLOR_FORMAT_UBWC_LOSSY_MAX             = 0x07FFFFFF,
};

/**
 * \brief Typedefs for unsigned integer types
 *
 */
#ifndef u8
typedef uint8_t u8;   /**< Unsigned 8-bit integer */
#endif
#ifndef u16
typedef uint16_t u16; /**< Unsigned 16-bit integer */
#endif
#ifndef u32
typedef uint32_t u32; /**< Unsigned 32-bit integer */
#endif
#ifndef u64
typedef uint64_t u64; /**< Unsigned 64-bit integer */
#endif

/*
 * struct hfi_buff - hfi buffer
 * @addr_l    :  lower memory address of buffer
 * @addr_h    :  upper memory address of buffer
 * @size      :  size of buffer
 * @version   :  version of buffer
 * @flags     :  flags
 */
struct hfi_buff {
	u32 addr_l;
	u32 addr_h;
	u32 size;
	u32 version;
	u32 flags;
};

/*
 * struct hfi_prop_u64 - hfi 64 bit prop
 * @val_lo    :  lower value of 64 bit property
 * @val_hi    :  higher value of 64 bit property
 */
struct hfi_prop_u64 {
	u32 val_lo;
	u32 val_hi;
};

#define HFI_BUFF_FEATURE_ENABLE         (1 << 0)
#define HFI_BUFF_FEATURE_BROADCAST      (1 << 1)
#define HFI_BUFF_FEATURE_HW_BLK_IDX_0   (1 << 2)
#define HFI_BUFF_FEATURE_HW_BLK_IDX_1   (1 << 3)
#define HFI_BUFF_FEATURE_HW_BLK_IDX_2   (1 << 4)
#define HFI_BUFF_FEATURE_HW_BLK_IDX_3   (1 << 5)

/*
 * struct hfi_buff_dpu - hfi buffer accessible by dpu
 * @flags    :  flags
 * @iova     :  input and output virtual address
 * @len      :  length of buffer
 */
struct hfi_buff_dpu {
	u32 flags;
	u32 iova;
	u32 len;
};

#endif // __H_HFI_DEFS_COMMON_H__

