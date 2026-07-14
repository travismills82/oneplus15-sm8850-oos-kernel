/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_DEFS_PANEL_H__
#define __H_HFI_DEFS_PANEL_H__

#include <hfi_defs_common.h>

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * hfi_panel_phy_type - Physical type of the panel
 * HFI_PANEL_PHY_OLED : Indicates the panel is an OLED panel.
 * HFI_PANEL_PHY_LCD  : Indicates the panel is LCD panel.
 */
enum hfi_panel_phy_type {
	HFI_PANEL_PHY_OLED      = 0x1,
	HFI_PANEL_PHY_LCD       = 0x2,
};

/*
 * hfi_panel_bpp - Panel Bits per pixel supported values
 * HFI_PANEL_BPP_3 : 3 BPP for rgb111
 * HFI_PANEL_BPP_8 : 8 BPP for rgb332
 * HFI_PANEL_BPP_12 : 12 BPP for rgb444
 * HFI_PANEL_BPP_16 : 16 BPP for rgb565
 * HFI_PANEL_BPP_18 : 18 BPP for rgb666
 * HFI_PANEL_BPP_24 : 24 BPP for rgb888
 * HFI_PANEL_BPP_30 : 30 BPP for rgb101010
 */
enum hfi_panel_bpp {
	HFI_PANEL_BPP_3      = 1,
	HFI_PANEL_BPP_8      = 2,
	HFI_PANEL_BPP_12     = 3,
	HFI_PANEL_BPP_16     = 4,
	HFI_PANEL_BPP_18     = 5,
	HFI_PANEL_BPP_24     = 6,
	HFI_PANEL_BPP_30     = 7,
};

/*
 * hfi_panel_lane_enable - This is bitmask values to enable required panel
 * data lanes
 * HFI_PANEL_LANE_0 : Enables panel data lane 0
 * HFI_PANEL_LANE_1 : Enables panel data lane 1
 * HFI_PANEL_LANE_2 : Enables panel data lane 2
 * HFI_PANEL_LANE_3 : Enables panel data lane 3
 */
enum hfi_panel_lane_enable {
	HFI_PANEL_LANE_0      = 0x00000001,
	HFI_PANEL_LANE_1      = 0x00000002,
	HFI_PANEL_LANE_2      = 0x00000004,
	HFI_PANEL_LANE_3      = 0x00000008,
};

/*
 * hfi_panel_lane_map - Panel lane map
 * HFI_PANEL_LANE_MAP_0123 : Lane map 0123
 */
enum hfi_panel_lane_map {
	HFI_PANEL_LANE_MAP_0123 = 1,
};

/*
 * hfi_panel_color_order_type - R, G and B channel ordering types
 * HFI_PANEL_COLOR_ORDER_RGB_SWAP_RGB : Indicates DSI_RGB_SWAP_RGB
 * HFI_PANEL_COLOR_ORDER_RGB_SWAP_RBG : Indicates DSI_RGB_SWAP_RBG
 * HFI_PANEL_COLOR_ORDER_RGB_SWAP_BRG : Indicates DSI_RGB_SWAP_BRG
 * HFI_PANEL_COLOR_ORDER_RGB_SWAP_GRB : Indicates DSI_RGB_SWAP_GRB
 * HFI_PANEL_COLOR_ORDER_RGB_SWAP_GBR : Indicates DSI_RGB_SWAP_GBR
 */
enum hfi_panel_color_order_type {
	HFI_PANEL_COLOR_ORDER_RGB_SWAP_RGB   = 0x1,
	HFI_PANEL_COLOR_ORDER_RGB_SWAP_RBG   = 0x2,
	HFI_PANEL_COLOR_ORDER_RGB_SWAP_BRG   = 0x3,
	HFI_PANEL_COLOR_ORDER_RGB_SWAP_GRB   = 0x4,
	HFI_PANEL_COLOR_ORDER_RGB_SWAP_GBR   = 0x5,
};

/*
 * hfi_panel_vsync_source - panel vsync source
 * HFI_PANEL_VSYNC_SOURCE_GPIO_0 : Vsync source from GPIO 0
 * HFI_PANEL_VSYNC_SOURCE_GPIO_1 : Vsync source from GPIO 1
 * HFI_PANEL_VSYNC_SOURCE_GPIO_2 : Vsync source from GPIO 2
 * HFI_PANEL_VSYNC_SOURCE_GPIO_3 : Vsync source from GPIO 3
 * HFI_PANEL_VSYNC_SOURCE_GPIO_4 : Vsync source from GPIO 4
 * HFI_PANEL_VSYNC_SOURCE_GPIO_5 : Vsync source from GPIO 5
 * HFI_PANEL_VSYNC_SOURCE_WD     : Vsync source from Watchdog timer
 */
enum hfi_panel_vsync_source {
	HFI_PANEL_VSYNC_SOURCE_GPIO_0  = 0x0,
	HFI_PANEL_VSYNC_SOURCE_GPIO_1  = 0x1,
	HFI_PANEL_VSYNC_SOURCE_GPIO_2  = 0x2,
	HFI_PANEL_VSYNC_SOURCE_GPIO_3  = 0x3,
	HFI_PANEL_VSYNC_SOURCE_GPIO_4  = 0x4,
	HFI_PANEL_VSYNC_SOURCE_GPIO_5  = 0x5,
	HFI_PANEL_VSYNC_SOURCE_WD      = 0xf,
};

/*
 * hfi_panel_trigger_type - Trigger mechanism types
 * HFI_PANEL_TRIGGER_NONE : No trigger
 * HFI_PANEL_TRIGGER_TE : Tear check signal line used for trigger
 * HFI_PANEL_TRIGGER_SW : Triggered by software
 * HFI_PANEL_TRIGGER_SW_TE : Software trigger and TE
 * HFI_PANEL_TRIGGER_SW_SEOF : Software trigger and start/end of frame trigger.
 * HFI_PANEL_TRIGGER_SEOF : Start/end of frame used for trigger
 */
enum hfi_panel_trigger_type {
	HFI_PANEL_TRIGGER_NONE      = 1,
	HFI_PANEL_TRIGGER_TE        = 2,
	HFI_PANEL_TRIGGER_SW        = 3,
	HFI_PANEL_TRIGGER_SW_TE     = 4,
	HFI_PANEL_TRIGGER_SW_SEOF   = 5,
	HFI_PANEL_TRIGGER_SEOF      = 6,
};

/*
 * hfi_panel_fps_traffic_mode - Panel traffic modes
 * HFI_PANEL_TRAFFIC_NON_BURST_SYNC_PULSE_MODE : Non burst with sync pulses
 * HFI_PANEL_TRAFFIC_NON_BURST_SYNC_EVENT_MODE : Non burst with sync start
 *                                               event
 * HFI_PANEL_TRAFFIC_BURST_MODE : Burst mode
 */
enum hfi_panel_fps_traffic_mode {
	HFI_PANEL_TRAFFIC_NON_BURST_SYNC_PULSE_MODE     = 1,
	HFI_PANEL_TRAFFIC_NON_BURST_SYNC_EVENT_MODE     = 2,
	HFI_PANEL_TRAFFIC_BURST_MODE                    = 3,
};

/*
 * hfi_panel_modes - Bitmasks to enable panel modes
 * HFI_PANEL_VIDEO_MODE_8_BIT : Indicates the panel works in video mode
 *                              for 8 bit.
 * HFI_PANEL_VIDEO_MODE_10_BIT : Indicates the panel works in video mode
 *                               for 10 bit.
 * HFI_PANEL_CMD_MODE_8_BIT : Indicates the panel works in cmd mode
 *                             for 8 bit.
 * HFI_PANEL_CMD_MODE_10_BIT : Indicates the panel works in cmd mode
 *                             for 10 bit.
 */
enum hfi_panel_modes {
	HFI_PANEL_VIDEO_MODE_8_BIT      = 0x1,
	HFI_PANEL_VIDEO_MODE_10_BIT     = 0x2,
	HFI_PANEL_CMD_MODE_8_BIT        = 0x4,
	HFI_PANEL_CMD_MODE_10_BIT       = 0x8,
};

/*
 * hfi_panel_backlight_ctrl - Backlight control implementation type
 * data lanes
 * HFI_PANEL_BACKLIGHT_CTRL_UNKNOWN : Unknown backlight control
 * HFI_PANEL_BACKLIGHT_CTRL_PWM     : Backlight controlled by PWM
 * HFI_PANEL_BACKLIGHT_CTRL_WLED    : Backlight controlled by WLED
 * HFI_PANEL_BACKLIGHT_CTRL_DCS     : Backlight controlled by DCS commands
 * HFI_PANEL_BACKLIGHT_CTRL_EXTERNAL : Backlight controlled externally
 */
enum hfi_panel_backlight_ctrl {
	HFI_PANEL_BACKLIGHT_CTRL_UNKNOWN    = 0x0,
	HFI_PANEL_BACKLIGHT_CTRL_PWM        = 0x1,
	HFI_PANEL_BACKLIGHT_CTRL_WLED       = 0x2,
	HFI_PANEL_BACKLIGHT_CTRL_DCS        = 0x3,
	HFI_PANEL_BACKLIGHT_CTRL_EXTERNAL   = 0x4,
};

/*
 * hfi_panel_compression_mode - Panel compression modes
 * HFI_PANEL_COMPRESSION_NONE: No Compression
 * HFI_PANEL_COMPRESSION_DSC : Display stream compression
 */
enum hfi_panel_compression_mode {
	HFI_PANEL_COMPRESSION_NONE  = 0,
	HFI_PANEL_COMPRESSION_DSC   = 1,
};

/*
 * hfi_panel_chroma_format_type - Source chroma format types
 * HFI_PANEL_CHROMA_FORMAT_444 : 444 chroma format
 * HFI_PANEL_CHROMA_FORMAT_422 : 422 chroma format
 * HFI_PANEL_CHROMA_FORMAT_420 : 420 chroma format
 */
enum hfi_panel_chroma_format_type {
	HFI_PANEL_CHROMA_FORMAT_444     = 1,
	HFI_PANEL_CHROMA_FORMAT_422     = 2,
	HFI_PANEL_CHROMA_FORMAT_420     = 3,
};

/*
 * hfi_panel_color_space_type - Source color space types
 * HFI_PANEL_COLOR_SPACE_RGB : RGB color space
 * HFI_PANEL_COLOR_SPACE_YUV : YUV color space
 */
enum hfi_panel_color_space_type {
	HFI_PANEL_COLOR_SPACE_RGB     = 1,
	HFI_PANEL_COLOR_SPACE_YUV     = 2,
};

/*
 * struct hfi_panel_topology
 * @mixer_count   :  number of mixers
 * @enc_count     :  number of compression encoders
 * @display_count :  number of displays
 */
struct hfi_panel_topology {
	u32 mixer_count;
	u32 enc_count;
	u32 display_count;
};

/*
 * hfi_panel_dcs_cmd_state - DCS command state
 * HFI_PANEL_DSI_HS_MODE : DSI high speed mode
 * HFI_PANEL_DSI_LP_MODE : DSI low power mode
 */
enum hfi_panel_dcs_cmd_state {
	HFI_PANEL_DSI_HS_MODE   = 0x1,
	HFI_PANEL_DSI_LP_MODE   = 0x2,
};

/*
 * hfi_dcs_command_type - DCS command buffer type
 * HFI_DCS_CMD_PRE_ON                   : DSI pre on command.
 * HFI_DCS_CMD_ON                       : DSI on command.
 * HFI_DCS_CMD_VIDEO_ON                 : DSI on command for video mode.
 * HFI_DCS_CMD_COMMAND_ON                : DSI on command for command mode.
 * HFI_DCS_CMD_POST_PANEL_ON            : DSI on command to send after
 *                                        displaying an image.
 * HFI_DCS_CMD_PRE_OFF                  : DSI pre off command.
 * HFI_DCS_CMD_OFF                      : DSI off command.
 * HFI_DCS_CMD_POST_OFF                 : DSI post off command.
 * HFI_DCS_CMD_PRE_RES_SWITCH           : DSI pre resolution switch command.
 * HFI_DCS_CMD_RES_SWITCH               : DSI resolution switch command.
 * HFI_DCS_CMD_POST_RES_SWITCH          : DSI post resolution switch command.
 * HFI_DCS_CMD_VIDEO_MODE_SWITCH_INS    : DSI Video mode switch in commands.
 * HFI_DCS_CMD_VIDEO_MODE_SWITCH_OUTS   : DSI Video mode switch out commands.
 * HFI_DCS_CMD_COMMAND_MODE_SWITCH_INS  : DSI Command mode switch in commands.
 * HFI_DCS_CMD_COMMAND_MODE_SWITCH_OUTS : DSI Command mode switch out commands.
 * HFI_DCS_CMD_PANEL_STATUS             : DSI Panel status command used to kick
 *                                        in ESD recovery.
 * HFI_DCS_CMD_LP1                      : DSI low power panel request command.
 * HFI_DCS_CMD_LP2                      : DSI ultra low power panel request command.
 * HFI_DCS_CMD_NOLP                     : DSI low power panel and ultra low power
 *                                        disable command.
 * HFI_DCS_CMD_PPS                      : DSI DSC PPS command.
 * HFI_DCS_CMD_ROI                      : DSI Panel ROI update.
 * HFI_DCS_CMD_TIMING_SWITCH            : DSI resolution / timing dynamic
 *                                        switch command.
 * HFI_DCS_CMD_POST_MODE_SWITCH_ON      : DSI panel on command after panel has
 *                                        switch modes.
 * HFI_DCS_CMD_QSYNC_ONS                : DSI QSYNC feature on command.
 * HFI_DCS_CMD_QSYNC_OFFS               : DSI QSYNC feature off command.
 */
enum hfi_panel_dcs_command_type {
	HFI_DCS_CMD_PRE_ON                      = 0x00000000,
	HFI_DCS_CMD_ON                          = 0x00000001,
	HFI_DCS_CMD_VIDEO_ON                    = 0x00000002,
	HFI_DCS_CMD_COMMAND_ON                  = 0x00000003,
	HFI_DCS_CMD_POST_PANEL_ON               = 0x00000004,
	HFI_DCS_CMD_PRE_OFF                     = 0x00000005,
	HFI_DCS_CMD_OFF                         = 0x00000006,
	HFI_DCS_CMD_POST_OFF                    = 0x00000007,
	HFI_DCS_CMD_PRE_RES_SWITCH              = 0x00000008,
	HFI_DCS_CMD_RES_SWITCH                  = 0x0000009,
	HFI_DCS_CMD_POST_RES_SWITCH             = 0x00000010,
	HFI_DCS_CMD_VIDEO_MODE_SWITCH_INS       = 0x00000011,
	HFI_DCS_CMD_VIDEO_MODE_SWITCH_OUTS      = 0x00000012,
	HFI_DCS_CMD_COMMAND_MODE_SWITCH_INS     = 0x00000013,
	HFI_DCS_CMD_COMMAND_MODE_SWITCH_OUTS    = 0x00000014,
	HFI_DCS_CMD_PANEL_STATUS                = 0x00000015,
	HFI_DCS_CMD_LP1                         = 0x00000016,
	HFI_DCS_CMD_LP2                         = 0x00000017,
	HFI_DCS_CMD_NOLP                        = 0x00000018,
	HFI_DCS_CMD_PPS                         = 0x00000019,
	HFI_DCS_CMD_ROI                         = 0x00000020,
	HFI_DCS_CMD_TIMING_SWITCH               = 0x00000021,
	HFI_DCS_CMD_POST_MODE_SWITCH_ON         = 0x00000022,
	HFI_DCS_CMD_QSYNC_ONS                   = 0x00000023,
	HFI_DCS_CMD_QSYNC_OFFS                  = 0x00000024,
	HFI_DCS_CMD_MAX                         = 0x00000025
};
/**
 * struct hfi_panel_res_data - Panel resolution data
 * @active_width: Panel active width
 * @active_height: Panel active height
 * @h_front_porch: Panel horizontal front porch
 * @h_back_porch: Panel horizontal back porch
 * @h_left_border: Panel horizontal left border
 * @h_right_border: Panel horizontal right border
 * @h_pulse_width: Panel horizontal pulse width
 * @h_sync_skew: Panel horizontal sync skew
 * @h_sync_pulse: Panel pulse mode option
 * @v_front_porch: Panel vertical front porch
 * @v_back_porch: Panel vertical back porch
 * @v_top_border: Panel vertical top border
 * @v_bottom_border: Panel vertical bottom border
 * @v_pulse_width: Panel vertical pulse width
 */
struct hfi_panel_res_data {
	u32 active_width;
	u32 active_height;
	u32 h_front_porch;
	u32 h_back_porch;
	u32 h_left_border;
	u32 h_right_border;
	u32 h_pulse_width;
	u32 h_sync_skew;
	u32 h_sync_pulse;
	u32 v_front_porch;
	u32 v_back_porch;
	u32 v_top_border;
	u32 v_bottom_border;
	u32 v_pulse_width;
};

/**
 * struct compression_params - Panel compression parameters
 * @mode: Panel compression modes
 * @version: Panel compression mode version
 * @scr_version: Panel compression SCR version
 * @slice_height: Panel compression slice height
 * @slice_width: Panel compression slice width
 * @slices_per_pkt: Panel compression slice per packet
 * @bits_per_component: Panel bits per component before compression
 * @pps_delay_ms: Panel post PPS command delay in milliseconds
 * @bits_per_pixel: Panel bits per pixel after compression
 * @chroma_format: Source chroma format
 * @color_space: Source color space
 * @block_prediction_enable: Enable / Disable block prediction at decoder
 */
struct hfi_panel_compression_params {
	enum hfi_panel_compression_mode	mode;
	u32 version;
	u32 scr_version;
	u32 slice_height;
	u32 slice_width;
	u32 slices_per_pkt;
	u32 bits_per_component;
	u32 pps_delay_ms;
	u32 bits_per_pixel;
	enum hfi_panel_chroma_format_type chroma_format;
	enum hfi_panel_color_space_type color_space;
	u32 block_prediction_enable;
};

/**
 * struct hfi_panel_dcs_cmd_buffer_info - DCS command buffer data
 * @offset_l: Lower address of the offset.
 * @offset_h: Higher address of the offset.
 * @size: Size of buffer including padded data.
 * @delay: Wait for specified ms after dcs command transmitted.
 * @ctrl_flags: Control flags like Broadcast/Unicast etc.
 * @mode: Type of mode (LPM/HSM)
 * @reserve1: Reserved for future use.
 * @reserve2: Reserved for future use.
 */
struct hfi_panel_dcs_cmd_buffer_info {
	u32 offset_l;
	u32 offset_h;
	u32 size;
	u32 delay;
	u32 ctrl_flags;
	enum hfi_panel_dcs_cmd_state mode;
	u32 reserve1;
	u32 reserve2;
};

#endif // __H_HFI_DEFS_PANEL_H__
