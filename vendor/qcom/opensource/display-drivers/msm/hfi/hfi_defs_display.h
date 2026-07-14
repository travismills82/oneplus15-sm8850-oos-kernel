/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_DEFS_DISPLAY_H__
#define __H_HFI_DEFS_DISPLAY_H__

#include <hfi_defs_common.h>

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * enum hfi_display_blend_ops - Different blend operations
 * @HFI_BLEND_OP_NOT_DEFINED     :    No blend operation defined for the layer.
 * @HFI_BLEND_OP_OPAQUE          :    Apply a constant blend operation. The layer
 *                                    would appear opaque in case fg plane alpha is
 *                                    0xff.
 * @HFI_BLEND_OP_PREMULTIPLIED   :    Apply source over blend rule. Layer already has
 *                                    alpha pre-multiplication done. If fg plane alpha
 *                                    is less than 0xff, apply modulation as well. This
 *                                    operation is intended on layers having alpha
 *                                    channel.
 * @HFI_BLEND_OP_COVERAGE        :    Apply source over blend rule. Layer is not alpha
 *                                    pre-multiplied. Apply pre-multiplication. If fg
 *                                    plane alpha is less than 0xff, apply modulation as
 *                                    well.
 * @HFI_BLEND_OP_SKIP            :   Operation to skip blending explicitly
 * @HFI_BLEND_OP_MAX             :    Used to track maximum blend operation possible.
 */
enum hfi_display_blend_ops {
	HFI_BLEND_OP_NOT_DEFINED        = 0x0,
	HFI_BLEND_OP_OPAQUE             = 0x1,
	HFI_BLEND_OP_PREMULTIPLIED      = 0x2,
	HFI_BLEND_OP_COVERAGE           = 0x3,
	HFI_BLEND_OP_SKIP               = 0x4,
	HFI_BLEND_OP_MAX                = 0x5,
};

/*
 * enum hfi_display_power_mode - extended power modes supported by the Display
 * @HFI_MODE_DPMS_ON      :   ON
 * @HFI_MODE_DPMS_LP1     :   Low power mode 1
 * @HFI_MODE_DPMS_LP2     :   Low power mode 2
 * @HFI_MODE_DPMS_OFF     :   OFF
 */
enum hfi_display_power_mode {
	HFI_MODE_DPMS_ON        = 0x0,
	HFI_MODE_DPMS_LP1       = 0x1,
	HFI_MODE_DPMS_LP2       = 0x2,
	HFI_MODE_DPMS_OFF       = 0x3,
};

/*
 * enum hfi_display_power_control - Bitmask to control power supplies for Panel/Controller/PHY
 * @HFI_PANEL_POWER      :   Panel Power
 * @HFI_CTRL_POWER       :   Controller Power
 * @HFI_PHY_POWER        :   PHY Power
 */
enum hfi_display_power_control {
	HFI_PANEL_POWER      = 0x1,
	HFI_CTRL_POWER       = 0x2,
	HFI_PHY_POWER        = 0x4,
};

/*
 * enum hfi_display_commit_flag - commit flags
 * @HFI_VALIDATE      :   validation flag
 * @HFI_COMMIT        :   commit flag
 */
enum hfi_display_commit_flag {
	HFI_VALIDATE           = 0x1,
	HFI_COMMIT             = 0x2,
};

/*
 * struct hfi_display_roi
 * @x_pos    :  x position of the roi
 * @y_pos    :  y position of the roi
 * @width    :  width of the roi
 * @height   :  height of the roi
 */
struct hfi_display_roi {
	u32 x_pos;
	u32 y_pos;
	u32 width;
	u32 height;
};

/*
 * struct hfi_display_vsync_data - vsync data
 * @timestamp_lo    :  lower value of 64bit vsync timestamp
 * @timestamp_hi    :  higher value of 64bit vsync timestamp
 * @vsync_index     :  vsync index for the timestamp
 */
struct hfi_display_vsync_data {
	u32 timestamp_lo;
	u32 timestamp_hi;
	u32 vsync_index;
};

/*
 * struct hfi_display_frame_event_data - frame event data
 * @timestamp_lo         :  lower value of 64bit Buffer flip timestamp
 * @timestamp_hi         :  higher value of 64bit Buffer flip timestamp
 * @bufferflip_index     :  bufferflip index for the timestamp
 */
struct hfi_display_frame_event_data {
	u32 timestamp_lo;
	u32 timestamp_hi;
	u32 bufferflip_index;
};

/*
 * hfi_display_event_id - HFI event ID
 * @HFI_EVENT_VSYNC                   : Event ID for vsync
 * @HFI_EVENT_FRAME_SCAN_START        : Event ID for frame scan start
 * @HFI_EVENT_FRAME_SCAN_COMPLETE     : Event ID for frame scan complete
 * @HFI_EVENT_FRAME_IDLE              : Event ID for frame idle
 * @HFI_EVENT_DISPLAY_POWER           : Event ID for display power
 */
enum hfi_display_event_id {
	HFI_EVENT_VSYNC                     = 0x1,
	HFI_EVENT_FRAME_SCAN_START          = 0x2,
	HFI_EVENT_FRAME_SCAN_COMPLETE       = 0x3,
	HFI_EVENT_FRAME_IDLE                = 0x4,
	HFI_EVENT_DISPLAY_POWER             = 0x5,
};

/*
 * struct hfi_display_mode_info - hfi dcp mode info
 * @size            :  Size of hfi_dcs_mode_info structure.
 * @h_active        :  Active width of one frame in pixels.
 * @h_back_porch    :  Horizontal back porch in pixels.
 * @h_sync_width    :  HSYNC width in pixels.
 * @h_front_porch   :  Horizontal front porch in pixels.
 * @h_skew          :  Horizontal sync skew value
 * @h_sync_polarity :  Polarity of HSYNC (false is active low).
 * @v_active        :  Active height of one frame in lines.
 * @v_back_porch    :  Vertical back porch in lines.
 * @v_sync_width    :  VSYNC width in lines.
 * @v_front_porch   :  Vertical front porch in lines.
 * @v_sync_polarity :  Polarity of VSYNC (false is active low).
 * @clk_rate_hz_lo  :  Lower address value DSI bit clock rate per lane in Hz.
 * @clk_rate_hz_hi  :  Upper address value of DSI bit clock rate per lane in Hz.
 * @flags_lo        :  Lower address value of flags.
 * @flags_hi        :  Upper address value of flags.
 * @reserved1       :  Reserved for future use.
 * @reserved2       :  Reserved for future use.
 */
struct hfi_display_mode_info {
	u32 size;
	u32 h_active;
	u32 h_back_porch;
	u32 h_sync_width;
	u32 h_front_porch;
	u32 h_skew;
	u32 h_sync_polarity;
	u32 v_active;
	u32 v_back_porch;
	u32 v_sync_width;
	u32 v_front_porch;
	u32 v_sync_polarity;
	u32 refresh_rate;
	u32 clk_rate_hz_lo;
	u32 clk_rate_hz_hi;
	u32 flags_lo;
	u32 flags_hi;
	u32 reserved1;
	u32 reserved2;
};

/*
 * enum hfi_display_blend_stage - Defines blending stages
 * @HFI_BLEND_STAGE_BASE    :  base layer
 * @HFI_BLEND_STAGE_0       :  Blend Stage #0(One base layer + one foreground layer)
 * @HFI_BLEND_STAGE_1       :  Blend Stage #1(Output of blend stage#0 + one foreground layer)
 * @HFI_BLEND_STAGE_2       :  Blend Stage #2(Output of blend stage#1 + one foreground layer)
 * @HFI_BLEND_STAGE_3       :  Blend Stage #3(Output of blend stage#2 + one foreground layer)
 * @HFI_BLEND_STAGE_4       :  Blend Stage #4(Output of blend stage#3 + one foreground layer)
 * @HFI_BLEND_STAGE_5       :  Blend Stage #5(Output of blend stage#4 + one foreground layer)
 * @HFI_BLEND_STAGE_6       :  Blend Stage #6(Output of blend stage#5 + one foreground layer)
 * @HFI_BLEND_STAGE_7       :  Blend Stage #7(Output of blend stage#6 + one foreground layer)
 * @HFI_BLEND_STAGE_8       :  Blend Stage #8(Output of blend stage#7 + one foreground layer)
 * @HFI_BLEND_STAGE_9       :  Blend Stage #9(Output of blend stage#8 + one foreground layer)
 * @HFI_BLEND_STAGE_10      :  Blend Stage #10(Output of blend stage#9 + one foreground layer)
 */
enum hfi_display_blend_stage {
	HFI_BLEND_STAGE_BASE,
	HFI_BLEND_STAGE_0,
	HFI_BLEND_STAGE_1,
	HFI_BLEND_STAGE_2,
	HFI_BLEND_STAGE_3,
	HFI_BLEND_STAGE_4,
	HFI_BLEND_STAGE_5,
	HFI_BLEND_STAGE_6,
	HFI_BLEND_STAGE_7,
	HFI_BLEND_STAGE_8,
	HFI_BLEND_STAGE_9,
	HFI_BLEND_STAGE_10,
};

/*
 * enum hfi_layer_fetch_mode - Layer fetch modes
 * @HFI_PARALLEL_FETCH       :       Parallel Fetch mode
 * @HFI_TIME_MULTIPLEX_FETCH :       Time-Multiplexed (Serial) Fetch mode
 */
enum hfi_layer_fetch_mode {
	HFI_PARALLEL_FETCH        = 0x0,
	HFI_TIME_MULTIPLEX_FETCH  = 0x1,
};

/**
 * @def HFI_DISPLAY_ROTATION_0
 * @brief Set when layer is not rotated.
 */
#define HFI_DISPLAY_ROTATION_0   (1 << 0)

/**
 * @def HFI_DISPLAY_ROTATION_90
 * @brief Set when layer is rotated by 90 degrees.
 */
#define HFI_DISPLAY_ROTATION_90   (1 << 1)

/**
 * @def HFI_DISPLAY_ROTATION_180
 * @brief Set when layer is rotated by 180 degrees.
 */
#define HFI_DISPLAY_ROTATION_180     (1 << 2)

/**
 * @def HFI_DISPLAY_ROTATION_270
 * @brief Set when layer is rotated by 270 degrees.
 */
#define HFI_DISPLAY_ROTATION_270   (1 << 3)

/**
 * @def HFI_DISPLAY_REFLECT_X
 * @brief Set when layer is reflected along X-axis.
 */
#define HFI_DISPLAY_REFLECT_X   (1 << 4)

/**
 * @def HFI_DISPLAY_REFLECT_Y
 * @brief Set when layer is reflected along Y-axis.
 */
#define HFI_DISPLAY_REFLECT_Y   (1 << 5)

#endif // __H_HFI_DEFS_DISPLAY_H__

