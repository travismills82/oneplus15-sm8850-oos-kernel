// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/errno.h>

#include "dsi_catalog.h"

/**
 * dsi_catalog_cmn_init() - catalog init for dsi controller v1.4
 */
static void dsi_catalog_cmn_init(struct dsi_ctrl_hw *ctrl,
		enum dsi_ctrl_version version)
{
	/* common functions */
	ctrl->ops.host_setup[MSM_DISP_OP_HWIO]             = dsi_ctrl_hw_cmn_host_setup;
	ctrl->ops.video_engine_en[MSM_DISP_OP_HWIO]        = dsi_ctrl_hw_cmn_video_engine_en;
	ctrl->ops.video_engine_setup[MSM_DISP_OP_HWIO]     = dsi_ctrl_hw_cmn_video_engine_setup;
	ctrl->ops.set_video_timing[MSM_DISP_OP_HWIO]       = dsi_ctrl_hw_cmn_set_video_timing;
	ctrl->ops.set_timing_db[MSM_DISP_OP_HWIO]          = dsi_ctrl_hw_cmn_set_timing_db;
	ctrl->ops.cmd_engine_setup[MSM_DISP_OP_HWIO]       = dsi_ctrl_hw_cmn_cmd_engine_setup;
	ctrl->ops.setup_cmd_stream[MSM_DISP_OP_HWIO]       = dsi_ctrl_hw_cmn_setup_cmd_stream;
	ctrl->ops.ctrl_en[MSM_DISP_OP_HWIO]                = dsi_ctrl_hw_cmn_ctrl_en;
	ctrl->ops.cmd_engine_en[MSM_DISP_OP_HWIO]          = dsi_ctrl_hw_cmn_cmd_engine_en;
	ctrl->ops.phy_sw_reset[MSM_DISP_OP_HWIO]           = dsi_ctrl_hw_cmn_phy_sw_reset;
	ctrl->ops.soft_reset[MSM_DISP_OP_HWIO]             = dsi_ctrl_hw_cmn_soft_reset;
	ctrl->ops.kickoff_command[MSM_DISP_OP_HWIO]        = dsi_ctrl_hw_cmn_kickoff_command;
	ctrl->ops.kickoff_fifo_command[MSM_DISP_OP_HWIO]   = dsi_ctrl_hw_cmn_kickoff_fifo_command;
	ctrl->ops.reset_cmd_fifo[MSM_DISP_OP_HWIO]         = dsi_ctrl_hw_cmn_reset_cmd_fifo;
	ctrl->ops.trigger_command_dma[MSM_DISP_OP_HWIO]    = dsi_ctrl_hw_cmn_trigger_command_dma;
	ctrl->ops.get_interrupt_status[MSM_DISP_OP_HWIO]   = dsi_ctrl_hw_cmn_get_interrupt_status;
	ctrl->ops.poll_dma_status[MSM_DISP_OP_HWIO]        = dsi_ctrl_hw_cmn_poll_dma_status;
	ctrl->ops.get_error_status[MSM_DISP_OP_HWIO]       = dsi_ctrl_hw_cmn_get_error_status;
	ctrl->ops.clear_error_status[MSM_DISP_OP_HWIO]     = dsi_ctrl_hw_cmn_clear_error_status;
	ctrl->ops.clear_interrupt_status[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_clear_interrupt_status;
	ctrl->ops.enable_status_interrupts[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_enable_status_interrupts;
	ctrl->ops.enable_error_interrupts[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_enable_error_interrupts;
	ctrl->ops.video_test_pattern_setup[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_video_test_pattern_setup;
	ctrl->ops.cmd_test_pattern_setup[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_cmd_test_pattern_setup;
	ctrl->ops.test_pattern_enable[MSM_DISP_OP_HWIO]    = dsi_ctrl_hw_cmn_test_pattern_enable;
	ctrl->ops.trigger_cmd_test_pattern[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_trigger_cmd_test_pattern;
	ctrl->ops.clear_phy0_ln_err[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_dln0_phy_err;
	ctrl->ops.phy_reset_config[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_phy_reset_config;
	ctrl->ops.setup_misr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_setup_misr;
	ctrl->ops.collect_misr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_collect_misr;
	ctrl->ops.get_cmd_read_data[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_get_cmd_read_data;
	ctrl->ops.clear_rdbk_register[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_clear_rdbk_reg;
	ctrl->ops.ctrl_reset[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_ctrl_reset;
	ctrl->ops.mask_error_intr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_mask_error_intr;
	ctrl->ops.error_intr_ctrl[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_error_intr_ctrl;
	ctrl->ops.get_error_mask[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_get_error_mask;
	ctrl->ops.get_hw_version[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_get_hw_version;
	ctrl->ops.wait_for_cmd_mode_mdp_idle[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_wait_for_cmd_mode_mdp_idle;
	ctrl->ops.setup_avr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_setup_avr;
	ctrl->ops.set_continuous_clk[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_set_continuous_clk;
	ctrl->ops.wait4dynamic_refresh_done[MSM_DISP_OP_HWIO] =
		dsi_ctrl_hw_cmn_wait4dynamic_refresh_done;
	ctrl->ops.hs_req_sel[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_hs_req_sel;
	ctrl->ops.vid_engine_busy[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_vid_engine_busy;
	ctrl->ops.init_cmddma_trig_ctrl[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_init_cmddma_trig_ctrl;

	switch (version) {
	case DSI_CTRL_VERSION_2_2:
	case DSI_CTRL_VERSION_2_3:
	case DSI_CTRL_VERSION_2_4:
	case DSI_CTRL_VERSION_2_5:
	case DSI_CTRL_VERSION_2_6:
	case DSI_CTRL_VERSION_2_7:
	case DSI_CTRL_VERSION_2_7_HFI:
	case DSI_CTRL_VERSION_2_8:
	case DSI_CTRL_VERSION_2_9:
	case DSI_CTRL_VERSION_2_10:
	case DSI_CTRL_VERSION_2_10_HFI:
		ctrl->ops.phy_reset_config[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_phy_reset_config;
		ctrl->ops.config_clk_gating[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_config_clk_gating;
		ctrl->ops.setup_lane_map[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_setup_lane_map;
		ctrl->ops.wait_for_lane_idle[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_22_wait_for_lane_idle;
		ctrl->ops.reg_dump_to_buffer[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_22_reg_dump_to_buffer;
		ctrl->ops.ulps_ops.ulps_request[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_ulps_request;
		ctrl->ops.ulps_ops.ulps_exit[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_cmn_ulps_exit;
		ctrl->ops.ulps_ops.get_lanes_in_ulps[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_cmn_get_lanes_in_ulps;
		ctrl->ops.clamp_enable[MSM_DISP_OP_HWIO] = NULL;
		ctrl->ops.clamp_disable[MSM_DISP_OP_HWIO] = NULL;
		ctrl->ops.schedule_dma_cmd[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_schedule_dma_cmd;
		ctrl->ops.kickoff_command_non_embedded_mode[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_kickoff_non_embedded_mode;
		ctrl->ops.configure_cmddma_window[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_22_configure_cmddma_window;
		ctrl->ops.reset_trig_ctrl[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_22_reset_trigger_controls;
		ctrl->ops.log_line_count[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_log_line_count;
		ctrl->ops.splitlink_cmd_setup[MSM_DISP_OP_HWIO] =
			dsi_ctrl_hw_22_configure_splitlink;
		ctrl->ops.setup_misr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_setup_misr;
		ctrl->ops.collect_misr[MSM_DISP_OP_HWIO] = dsi_ctrl_hw_22_collect_misr;
		break;
	default:
		break;
	}
}

/**
 * dsi_catalog_ctrl_setup() - return catalog info for dsi controller
 * @ctrl:        Pointer to DSI controller hw object.
 * @version:     DSI controller version.
 * @index:       DSI controller instance ID.
 * @phy_pll_bypass:              DSI PHY/PLL drivers bypass HW access.
 * @null_insertion_enabled:      DSI controller inserts null packet.
 *
 * This function setups the catalog information in the dsi_ctrl_hw object.
 *
 * return: error code for failure and 0 for success.
 */
int dsi_catalog_ctrl_setup(struct dsi_ctrl_hw *ctrl,
		   enum dsi_ctrl_version version, u32 index,
		   bool phy_pll_bypass, bool null_insertion_enabled)
{
	int rc = 0;

	if (version == DSI_CTRL_VERSION_UNKNOWN ||
	    version >= DSI_CTRL_VERSION_MAX) {
		DSI_ERR("Unsupported version: %d\n", version);
		return -ENOTSUPP;
	}

	ctrl->index = index;
	ctrl->null_insertion_enabled = null_insertion_enabled;
	set_bit(DSI_CTRL_VIDEO_TPG, ctrl->feature_map);
	set_bit(DSI_CTRL_CMD_TPG, ctrl->feature_map);
	set_bit(DSI_CTRL_VARIABLE_REFRESH_RATE, ctrl->feature_map);
	set_bit(DSI_CTRL_DYNAMIC_REFRESH, ctrl->feature_map);
	set_bit(DSI_CTRL_DESKEW_CALIB, ctrl->feature_map);
	set_bit(DSI_CTRL_DPHY, ctrl->feature_map);

	switch (version) {
	case DSI_CTRL_VERSION_2_2:
	case DSI_CTRL_VERSION_2_3:
	case DSI_CTRL_VERSION_2_4:
		ctrl->phy_pll_bypass = phy_pll_bypass;
		dsi_catalog_cmn_init(ctrl, version);
		break;
	case DSI_CTRL_VERSION_2_5:
	case DSI_CTRL_VERSION_2_6:
	case DSI_CTRL_VERSION_2_7:
	case DSI_CTRL_VERSION_2_7_HFI:
	case DSI_CTRL_VERSION_2_8:
	case DSI_CTRL_VERSION_2_9:
	case DSI_CTRL_VERSION_2_10:
	case DSI_CTRL_VERSION_2_10_HFI:
		ctrl->widebus_support = true;
		ctrl->phy_pll_bypass = phy_pll_bypass;
		dsi_catalog_cmn_init(ctrl, version);
		break;
	default:
		return -ENOTSUPP;
	}

	return rc;
}

/**
 * dsi_catalog_phy_3_0_init() - catalog init for DSI PHY 10nm
 */
static void dsi_catalog_phy_3_0_init(struct dsi_phy_hw *phy)
{
	phy->ops.regulator_enable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_regulator_enable;
	phy->ops.regulator_disable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_regulator_disable;
	phy->ops.enable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_enable;
	phy->ops.disable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_disable;
	phy->ops.calculate_timing_params[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_calculate_timing_params;
	phy->ops.ulps_ops.wait_for_lane_idle[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_wait_for_lane_idle;
	phy->ops.ulps_ops.ulps_request[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_ulps_request;
	phy->ops.ulps_ops.ulps_exit[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_ulps_exit;
	phy->ops.ulps_ops.get_lanes_in_ulps[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_get_lanes_in_ulps;
	phy->ops.ulps_ops.is_lanes_in_ulps[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_is_lanes_in_ulps;
	phy->ops.phy_timing_val[MSM_DISP_OP_HWIO] = dsi_phy_hw_timing_val_v3_0;
	phy->ops.clamp_ctrl[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_clamp_ctrl;
	phy->ops.phy_lane_reset[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_lane_reset;
	phy->ops.toggle_resync_fifo[MSM_DISP_OP_HWIO] = dsi_phy_hw_v3_0_toggle_resync_fifo;
	phy->ops.dyn_refresh_ops.dyn_refresh_config[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_dyn_refresh_config;
	phy->ops.dyn_refresh_ops.dyn_refresh_pipe_delay[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_dyn_refresh_pipe_delay;
	phy->ops.dyn_refresh_ops.dyn_refresh_helper[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_dyn_refresh_helper;
	phy->ops.dyn_refresh_ops.dyn_refresh_trigger_sel[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.dyn_refresh_ops.cache_phy_timings[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v3_0_cache_phy_timings;
	phy->ops.phy_idle_off[MSM_DISP_OP_HWIO] = NULL;
}

/**
 * dsi_catalog_phy_4_0_init() - catalog init for DSI PHY 7nm
 */
static void dsi_catalog_phy_4_0_init(struct dsi_phy_hw *phy)
{
	phy->ops.regulator_enable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.regulator_disable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.enable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_enable;
	phy->ops.disable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_disable;
	phy->ops.calculate_timing_params[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_calculate_timing_params;
	phy->ops.ulps_ops.wait_for_lane_idle[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_wait_for_lane_idle;
	phy->ops.ulps_ops.ulps_request[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_ulps_request;
	phy->ops.ulps_ops.ulps_exit[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_ulps_exit;
	phy->ops.ulps_ops.get_lanes_in_ulps[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_get_lanes_in_ulps;
	phy->ops.ulps_ops.is_lanes_in_ulps[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_is_lanes_in_ulps;
	phy->ops.phy_timing_val[MSM_DISP_OP_HWIO] = dsi_phy_hw_timing_val_v4_0;
	phy->ops.phy_lane_reset[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_lane_reset;
	phy->ops.toggle_resync_fifo[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_toggle_resync_fifo;
	phy->ops.reset_clk_en_sel[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_reset_clk_en_sel;

	phy->ops.dyn_refresh_ops.dyn_refresh_config[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_dyn_refresh_config;
	phy->ops.dyn_refresh_ops.dyn_refresh_pipe_delay[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_dyn_refresh_pipe_delay;
	phy->ops.dyn_refresh_ops.dyn_refresh_helper[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_dyn_refresh_helper;
	phy->ops.dyn_refresh_ops.dyn_refresh_trigger_sel[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_dyn_refresh_trigger_sel;
	phy->ops.dyn_refresh_ops.cache_phy_timings[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v4_0_cache_phy_timings;
	phy->ops.set_continuous_clk[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_set_continuous_clk;
	phy->ops.commit_phy_timing[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_commit_phy_timing;
	phy->ops.phy_idle_off[MSM_DISP_OP_HWIO] = dsi_phy_hw_v4_0_phy_idle_off;
}

/**
 * dsi_catalog_phy_5_0_init() - catalog init for DSI PHY 7nm
 */
static void dsi_catalog_phy_5_0_init(struct dsi_phy_hw *phy)
{
	phy->ops.regulator_enable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.regulator_disable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.enable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_enable;
	phy->ops.disable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_disable;
	phy->ops.calculate_timing_params[MSM_DISP_OP_HWIO] = dsi_phy_hw_calculate_timing_params;
	phy->ops.ulps_ops.wait_for_lane_idle[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_wait_for_lane_idle;
	phy->ops.ulps_ops.ulps_request[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_ulps_request;
	phy->ops.ulps_ops.ulps_exit[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_ulps_exit;
	phy->ops.ulps_ops.get_lanes_in_ulps[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_get_lanes_in_ulps;
	phy->ops.ulps_ops.is_lanes_in_ulps[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_is_lanes_in_ulps;
	phy->ops.phy_timing_val[MSM_DISP_OP_HWIO] = dsi_phy_hw_timing_val_v5_0;
	phy->ops.phy_lane_reset[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_lane_reset;
	phy->ops.toggle_resync_fifo[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_toggle_resync_fifo;
	phy->ops.reset_clk_en_sel[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_reset_clk_en_sel;

	phy->ops.dyn_refresh_ops.dyn_refresh_config[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v5_0_dyn_refresh_config;
	phy->ops.dyn_refresh_ops.dyn_refresh_pipe_delay[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v5_0_dyn_refresh_pipe_delay;
	phy->ops.dyn_refresh_ops.dyn_refresh_helper[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v5_0_dyn_refresh_helper;
	phy->ops.dyn_refresh_ops.dyn_refresh_trigger_sel[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v5_0_dyn_refresh_trigger_sel;
	phy->ops.dyn_refresh_ops.cache_phy_timings[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v5_0_cache_phy_timings;
	phy->ops.set_continuous_clk[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_set_continuous_clk;
	phy->ops.commit_phy_timing[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_commit_phy_timing;
	phy->ops.phy_idle_off[MSM_DISP_OP_HWIO] = dsi_phy_hw_v5_0_phy_idle_off;
}

/**
 * dsi_catalog_phy_7_2_init() - catalog init for DSI PHY 3nm
 */
static void dsi_catalog_phy_7_2_init(struct dsi_phy_hw *phy)
{
	phy->ops.regulator_enable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.regulator_disable[MSM_DISP_OP_HWIO] = NULL;
	phy->ops.enable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_enable;
	phy->ops.disable[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_disable;
	phy->ops.calculate_timing_params[MSM_DISP_OP_HWIO] = dsi_phy_hw_calculate_timing_params;
	phy->ops.ulps_ops.wait_for_lane_idle[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_wait_for_lane_idle;
	phy->ops.ulps_ops.ulps_request[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_ulps_request;
	phy->ops.ulps_ops.ulps_exit[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_ulps_exit;
	phy->ops.ulps_ops.get_lanes_in_ulps[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_get_lanes_in_ulps;
	phy->ops.ulps_ops.is_lanes_in_ulps[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_is_lanes_in_ulps;
	phy->ops.phy_timing_val[MSM_DISP_OP_HWIO] = dsi_phy_hw_timing_val_v7_2;
	phy->ops.phy_lane_reset[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_lane_reset;
	phy->ops.toggle_resync_fifo[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_toggle_resync_fifo;
	phy->ops.reset_clk_en_sel[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_reset_clk_en_sel;
	phy->ops.dyn_refresh_ops.dyn_refresh_config[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v7_2_dyn_refresh_config;
	phy->ops.dyn_refresh_ops.dyn_refresh_pipe_delay[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v7_2_dyn_refresh_pipe_delay;
	phy->ops.dyn_refresh_ops.dyn_refresh_helper[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v7_2_dyn_refresh_helper;
	phy->ops.dyn_refresh_ops.dyn_refresh_trigger_sel[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v7_2_dyn_refresh_trigger_sel;
	phy->ops.dyn_refresh_ops.cache_phy_timings[MSM_DISP_OP_HWIO] =
		dsi_phy_hw_v7_2_cache_phy_timings;
	phy->ops.set_continuous_clk[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_set_continuous_clk;
	phy->ops.commit_phy_timing[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_commit_phy_timing;
	phy->ops.phy_idle_off[MSM_DISP_OP_HWIO] = dsi_phy_hw_v7_2_phy_idle_off;
}

/**
 * dsi_catalog_phy_setup() - return catalog info for dsi phy hardware
 * @ctrl:        Pointer to DSI PHY hw object.
 * @version:     DSI PHY version.
 * @index:       DSI PHY instance ID.
 *
 * This function setups the catalog information in the dsi_phy_hw object.
 *
 * return: error code for failure and 0 for success.
 */
int dsi_catalog_phy_setup(struct dsi_phy_hw *phy,
			  enum dsi_phy_version version,
			  u32 index)
{
	int rc = 0;

	if (version == DSI_PHY_VERSION_UNKNOWN ||
	    version >= DSI_PHY_VERSION_MAX) {
		DSI_ERR("Unsupported version: %d\n", version);
		return -ENOTSUPP;
	}

	phy->index = index;
	phy->version = version;
	set_bit(DSI_PHY_DPHY, phy->feature_map);

	dsi_phy_timing_calc_init(phy, version);

	switch (version) {
	case DSI_PHY_VERSION_3_0:
		dsi_catalog_phy_3_0_init(phy);
		break;
	case DSI_PHY_VERSION_4_0:
	case DSI_PHY_VERSION_4_1:
	case DSI_PHY_VERSION_4_2:
	case DSI_PHY_VERSION_4_3:
	case DSI_PHY_VERSION_4_3_2:
		dsi_catalog_phy_4_0_init(phy);
		break;
	case DSI_PHY_VERSION_5_2:
		dsi_catalog_phy_5_0_init(phy);
		break;
	case DSI_PHY_VERSION_7_2:
	case DSI_PHY_VERSION_7_2_HFI:
		dsi_catalog_phy_7_2_init(phy);
		break;
	default:
		return -ENOTSUPP;
	}

	return rc;
}

int dsi_catalog_phy_pll_setup(struct dsi_phy_hw *phy, u32 pll_ver)
{
	int rc = 0;

	if (pll_ver >= DSI_PLL_VERSION_UNKNOWN) {
		DSI_ERR("Unsupported version: %d\n", pll_ver);
		return -EOPNOTSUPP;
	} else if (phy->phy_pll_bypass) {
		return 0;
	}

	switch (pll_ver) {
	case DSI_PLL_VERSION_5NM:
		phy->ops.configure[MSM_DISP_OP_HWIO] = dsi_pll_5nm_configure;
		phy->ops.pll_toggle[MSM_DISP_OP_HWIO] = dsi_pll_5nm_toggle;
		break;
	case DSI_PLL_VERSION_4NM:
		phy->ops.configure[MSM_DISP_OP_HWIO] = dsi_pll_4nm_configure;
		phy->ops.pll_toggle[MSM_DISP_OP_HWIO] = dsi_pll_4nm_toggle;
		break;
	case DSI_PLL_VERSION_3NM:
	case DSI_PLL_VERSION_3NM_HFI:
		phy->ops.configure[MSM_DISP_OP_HWIO] = dsi_pll_3nm_configure;
		phy->ops.pll_toggle[MSM_DISP_OP_HWIO] = dsi_pll_3nm_toggle;
		break;
	default:
		break;
	}

	return rc;
}
