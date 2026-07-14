/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PROPERTIES_PANEL_H__
#define __H_HFI_PROPERTIES_PANEL_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All panel property IDs begin here.
 * For panel properties: 16 - 19 bits of the property id = 0x4
 */
#define HFI_PROPERTY_PANEL_BEGIN                                     0x00040000

/*
 * HFI_PROPERTY_PANEL_TIMING_MODE_COUNT - Sets the total number of timing modes
 *                                        available for the panel.
 *                                        This property is sent to DCP as part
 *                                        of HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 *                                        command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_TIMING_MODE_COUNT
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_TIMING_MODE_COUNT |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : count
 */
#define HFI_PROPERTY_PANEL_TIMING_MODE_COUNT                         0x00040001

/*
 * HFI_PROPERTY_PANEL_INDEX - Sets the index of the timing mode.
 *                            This property is sent to DCP as part of
 *                            HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS command
 *                            packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_INDEX
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_INDEX |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : index
 */

#define HFI_PROPERTY_PANEL_INDEX                                     0x00040002

/*
 * HFI_PROPERTY_PANEL_CLOCKRATE - Sets the panel clock speed in Hz.
 *                                This property is sent to DCP as part of
 *                                HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_CLOCKRATE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_CLOCKRATE |
 *                               (version=0 << 20) | (dsize=2 << 24 )
 *   (u32_value) payload[1]    : clock_rate - Least Significant 32-bits
 *   (u32_value) payload[2]    : clock_rate - Most Significant 32-bits
 */
#define HFI_PROPERTY_PANEL_CLOCKRATE                                 0x00040003

/*
 * HFI_PROPERTY_PANEL_FRAMERATE - Sets the frame rate for the panel.
 *                                This property is sent to DCP as part of
 *                                HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_FRAMERATE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_FRAMERATE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : framerate
 */
#define HFI_PROPERTY_PANEL_FRAMERATE                                 0x00040004

/*
 * HFI_PROPERTY_PANEL_RESOLUTION_DATA - Sets panel active width / height,
 *                                      horizontal / vertical front / back
 *                                      porch in pixels, pulse width, sync
 *                                      skew data.
 *                                      This property is sent to DCP as part of
 *                                      HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                      command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_RESOLUTION_DATA
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_RESOLUTION_DATA |
 *                               (version=0 << 20) | (dsize=14 << 24 )
 *   (u32_value) payload[1-14] : struct hfi_panel_res_data
 */
#define HFI_PROPERTY_PANEL_RESOLUTION_DATA                           0x00040005

/*
 * HFI_PROPERTY_PANEL_JITTER - Sets panel jitter value is expressed in terms
 *                             of numerator denominator. It contains two u32
 *                             values – numerator followed by denominator.
 *                             The jitter configuration causes the early wakeup
 *                             if panel needs to adjust before VSYNC.
 *                             This property is sent to DCP as part of
 *                             HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                             command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_JITTER
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_JITTER |
 *                               (version=0 << 20) | (dsize=2 << 24 )
 *   (u32_value) payload[1]    : jitter_numerator
 *   (u32_value) payload[2]    : jitter_denominator
 */
#define HFI_PROPERTY_PANEL_JITTER                                    0x00040006

/*
 * HFI_PROPERTY_PANEL_COMPRESSION_DATA - Sets all the compression data for
 *                                      VDC / DSC.
 *                                      This property is sent to DCP as part of
 *                                      HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                      command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_COMPRESSION_DATA
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_COMPRESSION_DATA |
 *                               (version=0 << 20) | (dsize=12 << 24 )
 *   (u32_value) payload[1-12] : struct hfi_panel_compression_params
 */
#define HFI_PROPERTY_PANEL_COMPRESSION_DATA                          0x00040007

/*
 * HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY - Sets u32 values which specifies
 *                                   the list of topologies available for the
 *                                   display. A display topology is defined
 *                                   by a set of 3 values in the order:
 *                                   - number of mixers
 *                                   - number of compression encoders
 *                                   - number of interfaces
 *                                   This property is sent to DCP as part of
 *                                   HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                   command packet payload.
 * Data layout:
 *  struct hfi_panel_topology topology[n];
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY |
 *                               (version=0 << 20) | (dsize=(n*3)+1 << 24 )
 *   (u32_value) payload[1]    : value of 'n'
 *   (u32_value) payload[2..]  : topology[n]
 */
#define HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY                          0x00040008

/*
 * HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX - Sets the default topology index.
 *                                     This is one of the index among the
 *                                     indices specified by
 *                                     HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : index
 */
#define HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX                    0x00040009

/*
 * HFI_PROPERTY_PANEL_NAME - Panel name available for current timing
 *                                 mode.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_NAME
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_NAME |
 *                               (version=0 << 20) | (dsize=64 << 24 )
 *   (u32_value) payload[1-64] : u32 array of ASCII values of panel name
 */
#define HFI_PROPERTY_PANEL_NAME                                      0x0004000A

/*
 * HFI_PROPERTY_PANEL_PHYSICAL_TYPE - Specifies the type of the panel,
 *                                 OLED / LCD. If this property is not set,
 *                                 the panel is considered to be LCD panel by
 *                                 default.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_PHYSICAL_TYPE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_PHYSICAL_TYPE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of enum hfi_panel_phy_type
 */
#define HFI_PROPERTY_PANEL_PHYSICAL_TYPE                             0x0004000B

/*
 * HFI_PROPERTY_PANEL_BPP - Sets the panel bits per pixel.
 *                          This property is sent to DCP as part of
 *                          HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                          command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BPP
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BPP |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of enum hfi_panel_bpp
 */
#define HFI_PROPERTY_PANEL_BPP                                       0x0004000C

/*
 * HFI_PROPERTY_PANEL_LANES_STATE - Specifies whether lane data is enabled or
 *                                 not for lane 0, lane 1, lane 2 and lane 3.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_LANES_STATE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_LANES_STATE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : bitmask of enum hfi_panel_lane_enable
 */
#define HFI_PROPERTY_PANEL_LANES_STATE                               0x0004000D

/*
 * HFI_PROPERTY_PANEL_LANE_MAP - Specifies the panel lane map.
 *                               This property is sent to DCP as part of
 *                               HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                               command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_LANE_MAP
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_LANE_MAP |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of enum hfi_panel_lane_map
 */
#define HFI_PROPERTY_PANEL_LANE_MAP                                  0x0004000E

/*
 * HFI_PROPERTY_PANEL_COLOR_ORDER - Specifies the R, G and B channel
 *                                 ordering.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_COLOR_ORDER
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_COLOR_ORDER |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : One of enum hfi_panel_color_order_type
 */
#define HFI_PROPERTY_PANEL_COLOR_ORDER                               0x0004000F

/*
 * HFI_PROPERTY_PANEL_DMA_TRIGGER - Specifies the trigger mechanism to be used
 *                                 for DMA path.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DMA_TRIGGER
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DMA_TRIGGER |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : One of enum hfi_panel_trigger_type
 */
#define HFI_PROPERTY_PANEL_DMA_TRIGGER                               0x00040010

/*
 * HFI_PROPERTY_PANEL_TX_EOT_APPEND - Property used to enable appending
 *                                 end of transmission packets.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_TX_EOT_APPEND
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_TX_EOT_APPEND |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : HFI_TRUE / HFI_FALSE
 */
#define HFI_PROPERTY_PANEL_TX_EOT_APPEND                             0x00040011

/*
 * HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE - Boolean to determine DSI lane state
 *                                 during blanking low power period (BLLP) EOF
 *                                 mode.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : HFI_TRUE / HFI_FALSE
 */
#define HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE                       0x00040012

/*
 * HFI_PROPERTY_PANEL_BLLP_POWER_MODE - Boolean to determine DSI lane state
 *                                 during blanking low power period (BLLP)
 *                                 mode.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BLLP_POWER_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BLLP_POWER_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : HFI_TRUE / HFI_FALSE
 */
#define HFI_PROPERTY_PANEL_BLLP_POWER_MODE                           0x00040013

/*
 * HFI_PROPERTY_PANEL_TRAFFIC_MODE - Specifies the panel traffic mode.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_TRAFFIC_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_TRAFFIC_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : One of enum hfi_panel_fps_traffic_mode
 */
#define HFI_PROPERTY_PANEL_TRAFFIC_MODE                              0x00040014

/*
 * HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID - Specifies the virtual channel
 *                                 identifier.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : value
 */
#define HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID                        0x00040015

/*
 * HFI_PROPERTY_PANEL_WR_MEM_START - DCS command for write_memory_start.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_WR_MEM_START
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_WR_MEM_START |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : value
 */
#define HFI_PROPERTY_PANEL_WR_MEM_START                              0x00040016

/*
 * HFI_PROPERTY_PANEL_WR_MEM_CONTINUE - DCS command for write_memory_continue.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                 command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_WR_MEM_CONTINUE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_WR_MEM_CONTINUE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : value
 */
#define HFI_PROPERTY_PANEL_WR_MEM_CONTINUE                           0x00040017

/*
 * HFI_PROPERTY_PANEL_TE_DCS_COMMAND - Inserts the DCS command.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_TE_DCS_COMMAND
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_TE_DCS_COMMAND |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : value
 */
#define HFI_PROPERTY_PANEL_TE_DCS_COMMAND                            0x00040018

/*
 * HFI_PROPERTY_PANEL_OPERATING_MODE - Specifies the panel operating
 *                                     mode.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_OPERATING_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_OPERATING_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : One of enum hfi_panel_modes
 */
#define HFI_PROPERTY_PANEL_OPERATING_MODE                            0x00040019

/*
 * HFI_PROPERTY_PANEL_BL_MIN_LEVEL - Specifies the min backlight level
 *                                   supported by the panel.
 *                                   This property is sent to DCP as part of
 *                                   HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                   command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BL_MIN_LEVEL
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BL_MIN_LEVEL |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : min_backlight_level (least possible: 0)
 */
#define HFI_PROPERTY_PANEL_BL_MIN_LEVEL                              0x0004001A

/*
 * HFI_PROPERTY_PANEL_BL_MAX_LEVEL - Specifies the max backlight level
 *                                   supported by the panel.
 *                                   This property is sent to DCP as part of
 *                                   HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                   command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BL_MAX_LEVEL
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BL_MAX_LEVEL |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : max_backlight_level
 */
#define HFI_PROPERTY_PANEL_BL_MAX_LEVEL                              0x0004001B

/*
 * HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL - Specifies the max brightness
 *                                     level supported.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : max_brightness_level
 */
#define HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL                      0x0004001C

/*
 * HFI_PROPERTY_PANEL_CTRL_NUM - Specifies the DSI controllers to use
 *                               for primary panel.
 *                               This property is sent to DCP as part of
 *                               HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                               command packet payload.
 * Data layout:
 *  struct property_array ctrl_nums;
 *  ctrl_nums.count = 'n'
 *  ctrl_nums.values = {.., nth}
 *
 * @PanelInit - HFI_PROPERTY_PANEL_CTRL_NUM
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_CTRL_NUM |
 *                               (version=0 << 20) | (dsize=n+1 << 24 )
 *   (u32_value) payload[1]    : ctrl_nums.count
 *   (u32_value) payload[2..]  : ctrl_nums.values
 */
#define HFI_PROPERTY_PANEL_CTRL_NUM                                  0x0004001D

/*
 * HFI_PROPERTY_PANEL_PHY_NUM - Specifies the DSI PHYs to use for
 *                              primary panel.
 *                              This property is sent to DCP as part of
 *                              HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                              command packet payload.
 * Data layout:
 *  struct property_array phy_nums;
 *  phy_nums.count = 'n'
 *  phy_nums.values = {.., nth}
 *
 * @PanelInit - HFI_PROPERTY_PANEL_PHY_NUM
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_PHY_NUM |
 *                               (version=0 << 20) | (dsize=n+1 << 24 )
 *   (u32_value) payload[1]    : phy_nums.count
 *   (u32_value) payload[2..]  : phy_nums.values
 */
#define HFI_PROPERTY_PANEL_PHY_NUM                                   0x0004001E

/*
 * HFI_PROPERTY_PANEL_RESET_SEQUENCE - An array that lists the sequence of
 * reset GPIO values and sleeps.
 * This property is sent to DCP as part of HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 * command packet payload.
 *
 * Data layout:
 *  struct property_array reset_seq;
 *  reset_seq.count = 'n'
 *  reset_seq.values = {.., nth}
 *
 * @PanelInit - HFI_PROPERTY_PANEL_RESET_SEQUENCE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_RESET_SEQUENCE |
 *                               (version=0 << 20) | (dsize=n+1 << 24 )
 *   (u32_value) payload[1]    : reset_seq.count
 *   (u32_value) payload[2..]  : reset_seq.values
 */
#define HFI_PROPERTY_PANEL_RESET_SEQUENCE                            0x0004001F

/*
 * HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE - Specifies the implementation of
 *                                     backlight control for this panel.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of enum hfi_panel_backlight_ctrl
 */
#define HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE                      0x00040020

/*
 * HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE - Specifies the implementation
 *                                     of backlight control for secondary
 *                                     panel.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of enum hfi_panel_backlight_ctrl
 */
#define HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE                  0x00040021

/*
 * HFI_PROPERTY_PANEL_BL_INVERTED_DBV - Specifies whether to invert the
 *                                     display brightness value.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_BL_INVERTED_DBV
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_BL_INVERTED_DBV |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : 1 / 0 (1 = Invert, 0 = Uninverted)
 */
#define HFI_PROPERTY_PANEL_BL_INVERTED_DBV                           0x00040022

/*
 * HFI_PROPERTY_PANEL_DCS_CMD_INFO -  Sets DCS command buffer data for
 *                                    different types of DCS commands.
 *                                    This property is sent to DCP as part of
 *                                    HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                    command packet payload.
 * Data Layout:
 *  struct dsi_dcp_panel_per_cmd_type {
 *	    u32 dpu_buf_type_offset; // Offset in DPU buffer of all commands of respective type
 *	    enum hfi_panel_dcs_command_type cmd_type; // Type of command
 *	    u32 count_cmds;                           // Number of commands of particular type
 *	    u32 dcp_buff_struct_offset; // Offset in DCP buffer of hfi_dcs_cmd_buffer
 *	    u32 reserved_key;
 *  };
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DCS_CMD_INFO
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DCS_CMD_INFO |
 *                               (version=0 << 20) | (dsize=(5*count)+1 << 24 )
 *   (u32_value) payload[1]    : count of types being passed in
 *   (u32_value) payload[2..]  : dsi_dcp_panel_per_cmd_type[count]
 */
#define HFI_PROPERTY_PANEL_DCS_CMD_INFO                              0x00040024

/*
 * HFI_PROPERTY_PANEL_DPU_ADDRESS    - Provides iova of DPU Mapped buffer of
 *                                     HW understandable panel commands.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DPU_ADDRESS
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DPU_ADDRESS |
 *                               (version=0 << 20) | (dsize=3 << 24 )
 *   (u32_value) payload[1]    : reserved_key
 *   (u32_value) payload[2]    : u32 dpu_va LSB
 *   (u32_value) payload[3]    : u32 dpu_va MSB
 */
#define HFI_PROPERTY_PANEL_DPU_ADDRESS                              0x00040025

/*
 * HFI_PROPERTY_PANEL_DCP_ADDRESS    - Provides dva of DCP Mapped
 *                                     buffer of structures of cmd descriptions.
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DCP_ADDRESS
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DPU_ADDRESS |
 *                               (version=0 << 20) | (dsize=3 << 24 )
 *   (u32_value) payload[1]    : reserved_key
 *   (u32_value) payload[2]    : u32 dcp_va LSB
 *   (u32_value) payload[3]    : u32 dcp_va MSB
 */
#define HFI_PROPERTY_PANEL_DCP_ADDRESS                              0x00040026

/*
 * HFI_PROPERTY_PANEL_DPHY_TIMINGS    - Provides phy timing values needed
 *                                     for DPHY timing params configurations
 *                                     This property is sent to DCP as part of
 *                                     HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                     command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_DPHY_TIMINGS
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_DPHY_TIMINGS |
 *                               (version=0 << 20) | (dsize = n+1 << 24 )
 *   (u32_value) payload[1]       : count
 *   (u32_value) payload[2..n]    : property_array_u32[count]
 */
#define HFI_PROPERTY_PANEL_DPHY_TIMINGS                              0x00040027

/*
 * HFI_PROPERTY_PANEL_VSYNC_SOURCE - Entry to select vsync source from GPIO's/Watchdog timer.
 *                                 This property is sent to DCP as part of
 *                                 HFI_COMMAND_PANEL_INIT_GENERIC_CAPS command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_VSYNC_SOURCE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_VSYNC_SOURCE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : one of the values from enum hfi_panel_vsync_source
 */
#define HFI_PROPERTY_PANEL_VSYNC_SOURCE                              0x00040028

/*
 * HFI_PROPERTY_PANEL_STREAM_TRIGGER - Specifies the trigger mechanism to be used for pixel
 *                                  stream transfer in a command mode display processor path.
 *                                  This property is sent to DCP as part of
 *                                  HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                  command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_STREAM_TRIGGER
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_STREAM_TRIGGER |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : One of enum hfi_panel_trigger_type
 */
#define HFI_PROPERTY_PANEL_STREAM_TRIGGER                            0x00040029

/*
 * HFI_PROPERTY_PANEL_TE_MODE - Specifies the TE Path for command mode panels.
 *                              This property is sent to DCP as part of
 *                              HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                              command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_TE_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_TE_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : 1 / 0 (1 = TE returned from dedicated pin,
 *                                      0 = TE returned from data link)
 */
#define HFI_PROPERTY_PANEL_TE_MODE                                   0x0004002A

/*
 * HFI_PROPERTY_PANEL_CPHY_MODE - Specifies whether panel is using CPHY
 *                                This property is sent to DCP as part of
 *                                HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                command packet payload.
 *
 * @PanelInit - HFI_PROPERTY_PANEL_CPHY_MODE
 *     (u32_key) payload[0]    : HFI_PROPERTY_PANEL_CPHY_MODE |
 *                               (version=0 << 20) | (dsize=1 << 24 )
 *   (u32_value) payload[1]    : HFI_TRUE / HFI_FALSE
 */
#define HFI_PROPERTY_PANEL_CPHY_MODE                                 0x0004002B

/*
 * All panel property IDs end here
 */
#define HFI_PROPERTY_PANEL_END                                       0x0004FFFF

#endif // __H_HFI_PROPERTIES_PANEL_H__
