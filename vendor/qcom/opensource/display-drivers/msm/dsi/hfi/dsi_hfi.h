/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _DSI_HFI_HEADER_H_
#define _DSI_HFI_HEADER_H_

#include <linux/types.h>

#include "msm_drv.h"
#include "dsi_defs.h"
#include "dsi_display.h"
#include "dsi_display_hfi.h"
#include "hfi_utils.h"
#include "hfi_defs_panel.h"

#define MAX_NUM_CTRLS_AND_LENGTH 3
#define MAX_NUM_PHYS_AND_LENGTH 3
#define MIN_NUM_OF_GEN_CAPS 15
#define NUM_PANEL_CMD_TYPES_SUPPORTED 4
#define CLK_RATE_SIZE 2
#define JITTER_SIZE 2
#define NUM_VARIABLE_DPHY_TIMINGS 14

/**
 * struct dsi_value_to_prop_lookup - contains map with hfi properties and
 *                                   corresponding values
 * @value:               value
 * @hfi_prop:            hfi property
 */
struct dsi_value_to_prop_lookup {
	u32 value;
	u32 hfi_prop;
};

/**
 * struct dsi_panel_init_caps - contains properties to be sent as part of
 * HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 * @num_timing_modes:               HFI_PROPERTY_PANEL_TIMING_MODE_COUNT
 */
struct dsi_panel_init_caps {
	u32 num_timing_modes;
};

/**
 * struct dsi_hfi_panel_cmd_info - contains information per command
 * @cmd_offset:         offset in dpu mapped buffer
 * @size:               size of command
 * @delay:              delay for command
 * @ctrl_flags:         panel ctrl flags
 * @mode:               panel sending mode (LP/HS)
 */
struct dsi_hfi_panel_cmd_info {
	u32 cmd_offset;
	u32 size;
	u32 delay;
	u32 ctrl_flags;
	u32 mode;
	u32 reserved1;
	u32 reserved2;
};

/**
 * struct dsi_hfi_panel_per_cmd_type - contains information per command type
 * @cmd_offset:              offset in dpu mapped buffer for commands
 * @cmd_type:                type of command
 * @count_cmds:              count of command
 * @hfi_buff_struct_offset:  offset in dcp mapped buffer for structs
 */
struct dsi_hfi_panel_per_cmd_type {
	u32 sde_buff_type_offset;
	enum hfi_panel_dcs_command_type cmd_type;
	u32 count_cmds;
	u32 hfi_buff_struct_offset;
	u32 reserved_key;
};

/**
 * struct dsi_hfi_per_cmd_type_payload - payload with all DCS command types
 *
 * @count:                         count
 * @hfi_per_type_array:            payload per type
 */
struct dsi_hfi_per_cmd_type_payload  {
	u32 count;
	struct dsi_hfi_panel_per_cmd_type hfi_per_type_array[NUM_PANEL_CMD_TYPES_SUPPORTED];
};

/**
 * struct dsi_hfi_topology_payload - payload with topology info
 *
 * @count:                          count
 * @hfi_topology:                   hfi topology payload
 */
struct dsi_hfi_topology_payload {
	u32 count;
	struct hfi_panel_topology hfi_topology;
};

/**
 * struct dsi_hfi_phy_timings_payload - payload with DSI PHY Panel Timings from DT
 *
 * @count:                          count
 * @dphy_timings:                   DSI PHY Panel Timings
 */
struct dsi_hfi_phy_timings_payload {
	u32 count;
	u32 dphy_timings[NUM_VARIABLE_DPHY_TIMINGS];
};

/**
 * struct dsi_panel_timing_caps - contains properties to be sent as part of
 * HFI_COMMAND_PANEL_INIT_TIMING_CAPS
 * @panel_index:                    HFI_PROPERTY_PANEL_INDEX
 * @clockrate:                      HFI_PROPERTY_PANEL_CLOCKRATE lsb/msb
 * @framerate:                      HFI_PROPERTY_PANEL_FRAMERATE
 * @panel_jitter:                   HFI_PROPERTY_PANEL_JITTER numerator/denominator
 * @hsync_pulse:                    HFI_PROPERTY_PANEL_H_SYNC_PULSE
 * @res_data:                       HFI_PROPERTY_PANEL_RESOLUTION_DATA
 * @compression_params:             HFI_PROPERTY_PANEL_COMPRESSION_DATA
 * @topology:                       topology information
 * top_index:                       index of default topology
 * running_hfi_offset:              offset of pointer in hfi mapped buffer
 * payload:                         panel cmd information
 * phy_timings_payload:             DSI PHY panel tmgs info
 */
struct dsi_panel_timing_caps {
	u32 panel_index;
	u32 clockrate[CLK_RATE_SIZE];
	u32 framerate;
	u32 panel_jitter[JITTER_SIZE];
	u32 hsync_pulse;
	struct hfi_panel_res_data res_data;
	struct hfi_panel_compression_params compression_params;
	struct dsi_hfi_topology_payload topology;
	u32 top_index;
	u32 running_hfi_offset;
	struct dsi_hfi_per_cmd_type_payload payload;
	struct dsi_hfi_phy_timings_payload phy_timings_payload;
};

/**
 * struct dsi_panel_generic_caps - contains properties to be sent as part of
 * HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 * @valid_gen_caps_cnt:             number of caps with valid parameters
 * @panel_name:                     HFI_PROPERTY_PANEL_NAME
 * @panel_type:                     HFI_PROPERTY_PANEL_PHYSICAL_TYPE
 * @panel_bpp:                      HFI_PROPERTY_PANEL_BPP
 * @panels_lanes_state:             HFI_PROPERTY_PANEL_LANES_STATE
 * @panel_lane_map:                 HFI_PROPERTY_PANEL_LANE_MAP
 * @color_order_type:               HFI_PROPERTY_PANEL_COLOR_ORDER
 * @dma_trigger_type:               HFI_PROPERTY_PANEL_DMA_TRIGGER
 * @mdp_trigger_type:               HFI_PROPERTY_PANEL_STREAM_TRIGGER
 * @te_mode:                        HFI_PROPERTY_PANEL_TE_MODE
 * @tx_eot_append:                  HFI_PROPERTY_PANEL_TX_EOT_APPEND
 * @eof_power_mode:                 HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE
 * @bllp_power_mode:                HFI_PROPERTY_PANEL_BLLP_POWER_MODE
 * @traffic_mode:                   HFI_PROPERTY_PANEL_TRAFFIC_MODE
 * @virtual_channel_id:             HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID
 * @wr_mem_start:                   HFI_PROPERTY_PANEL_WR_MEM_START
 * @wr_mem_continue:                HFI_PROPERTY_PANEL_WR_MEM_CONTINUE
 * @te_dcs_command:                 HFI_PROPERTY_PANEL_TE_DCS_COMMAND
 * @panel_op_mode:                  HFI_PROPERTY_PANEL_PANEL_OPERATING_MODE
 * @min_backlight_level:            HFI_PROPERTY_PANEL_BL_MIN_LEVEL
 * @max_backlight_level:            HFI_PROPERTY_PANEL_BL_MAX_LEVEL
 * @max_brightness_level:           HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL
 * @backlight_ctrl_prim:            HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE
 * @backlight_ctrl_sec:             HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE
 * @is_bl_inverted:                 HFI_PROPERTY_PANEL_BL_INVERTED_DBV
 * @vsync_src:                      HFI_PROPERTY_PANEL_VSYNC_SOURCE
 * @ctrl_nums:                      HFI_PROPERTY_PANEL_CTRL_NUM
 * @phy_nums:                       HFI_PROPERTY_PANEL_PHY_NUM
 */
struct dsi_panel_generic_caps {
	int valid_gen_caps_cnt;

	u32 panel_name;
	enum hfi_panel_phy_type panel_type;
	enum hfi_panel_bpp panel_bpp;
	enum hfi_panel_lane_enable panels_lanes_state;
	enum hfi_panel_lane_map panel_lane_map;
	enum hfi_panel_color_order_type color_order_type;
	enum hfi_panel_trigger_type dma_trigger_type;
	enum hfi_panel_trigger_type mdp_trigger_type;
	u32 te_mode;
	u32 tx_eot_append;
	u32 eof_power_mode;
	u32 bllp_power_mode;
	enum hfi_panel_fps_traffic_mode traffic_mode;
	u32 virtual_channel_id;
	u32 wr_mem_start;
	u32 wr_mem_continue;
	u32 te_dcs_command;
	enum hfi_panel_modes panel_op_mode;
	u32 min_backlight_level;
	u32 max_backlight_level;
	u32 max_brightness_level;
	enum hfi_panel_backlight_ctrl backlight_ctrl_prim;
	enum hfi_panel_backlight_ctrl backlight_ctrl_sec;
	u32 is_bl_inverted;
	enum hfi_panel_vsync_source vsync_src;
	u32 ctrl_nums[MAX_NUM_CTRLS_AND_LENGTH];
	u32 phy_nums[MAX_NUM_PHYS_AND_LENGTH];
};

/**
 * struct dsi_hfi_cb - dsi hfi callback
 * @client:                             hfi client
 * @cmd_buf:                            hfi cmd buff
 * @cmd_buff_work:                      cmd buff worker
 */
struct dsi_hfi_cb {
	struct hfi_client_t *client;
	struct hfi_cmdbuf_t *cmd_buf;
	struct kthread_work cmd_buff_work;
};

/**
 * dsi_hfi_process_cmd_buf() - process hfi command buffer
 * @hfi_client:	handle to hfi client
 * @cmd_buf: handle to cmd buffer
 *
 * Return: error code.
 */
int dsi_hfi_process_cmd_buf(struct hfi_client_t *hfi_client, struct hfi_cmdbuf_t *cmd_buf);

/**
 * dsi_hfi_prop_handler() - handle/parse hfi properties
 * @UNIQUE_DISP_OR_OBJ_ID:	display/object ID
 * @CMD_ID: command ID
 * @payload: handle to payload
 * @size: size of payload
 * @listener: handle to hfi property listener
 */
void dsi_hfi_prop_handler(u32 UNIQUE_DISP_OR_OBJ_ID, u32 CMD_ID, void *payload, u32 size,
						struct hfi_prop_listener *listener);

/**
 * dsi_display_hfi_setup_hfi() - setup dsi as a client of hfi
 * @display: handle to dsi display structure
 * @hfi_host: handle to hfi host
 *
 * Return: error code.
 */
int dsi_display_hfi_setup_hfi(struct dsi_display *display, struct hfi_adapter_t *hfi_host);

/**
 * dsi_display_hfi_send_cmd_buf() - dsi wrapper for sending hfi cmd buffer
 * @display: handle to dsi display structure
 * @hfi_client: handle to hfi client
 * @hfi_cmd: hfi command
 * @display_type: display type string
 * @hfi_payload_type: hfi payload type
 * @payload: handle to payload
 * @payload_size: payload size
 * @flags: flags
 *
 * Return: error code.
 */
int dsi_display_hfi_send_cmd_buf(struct dsi_display *display,
					struct hfi_client_t *hfi_client, u32 hfi_cmd,
					const char *display_type, u32 hfi_payload_type,
					void *payload, u32 payload_size, u32 flags);

/**
 * dsi_hfi_panel_init() - submit dsi dtsi panel information to hfi
 * @display: handle to dsi display structure
 * @panel: handle to dsi panel structure
 *
 * Return: error code.
 */
int dsi_hfi_panel_init(struct dsi_display *display, struct dsi_panel *panel);

/**
 * dsi_display_hfi_register_pwr_supplies() - register power supplies with hfi
 * @display: handle to dsi display structure
 *
 * Return: error code.
 */
int dsi_display_hfi_register_pwr_supplies(struct dsi_display *display);

/**
 * dsi_hfi_misr_setup() - setup dsi hfi misr
 * @display: handle to dsi display structure
 *
 * Return: error code.
 */
int dsi_hfi_misr_setup(struct dsi_display *display);

/**
 * dsi_hfi_misr_read() - read dsi misr values from hfi
 * @display: handle to dsi display structure
 *
 * Return: error code.
 */
int dsi_hfi_misr_read(struct dsi_display *display);

#endif
