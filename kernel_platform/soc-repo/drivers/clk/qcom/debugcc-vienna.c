// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "clk: %s: " fmt, __func__

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "clk-debug.h"
#include "common.h"

static struct measure_clk_data debug_mux_priv = {
	.ctl_reg = 0x62048,
	.status_reg = 0x6204C,
	.xo_div4_cbcr = 0x62008,
};

static const char *const apss_cc_debug_mux_parent_names[] = {
	"measure_only_apcs_gold_post_acd_clk",
	"measure_only_apcs_gold_pre_acd_clk",
	"measure_only_apcs_l3_post_acd_clk",
	"measure_only_apcs_l3_pre_acd_clk",
	"measure_only_apcs_silver_post_acd_clk",
	"measure_only_apcs_silver_pre_acd_clk",
};

static int apss_cc_debug_mux_sels[] = {
	0x25,		/* measure_only_apcs_gold_post_acd_clk */
	0x45,		/* measure_only_apcs_gold_pre_acd_clk */
	0x41,		/* measure_only_apcs_l3_post_acd_clk */
	0x46,		/* measure_only_apcs_l3_pre_acd_clk */
	0x21,		/* measure_only_apcs_silver_post_acd_clk */
	0x44,		/* measure_only_apcs_silver_pre_acd_clk */
};

static int apss_cc_debug_mux_pre_divs[] = {
	0x8,		/* measure_only_apcs_gold_post_acd_clk */
	0x10,		/* measure_only_apcs_gold_pre_acd_clk */
	0x4,		/* measure_only_apcs_l3_post_acd_clk */
	0x10,		/* measure_only_apcs_l3_pre_acd_clk */
	0x4,		/* measure_only_apcs_silver_post_acd_clk */
	0x10,		/* measure_only_apcs_silver_pre_acd_clk */
};

static struct clk_debug_mux apss_cc_debug_mux = {
	.priv = &debug_mux_priv,
	.debug_offset = 0x18,
	.post_div_offset = 0x18,
	.cbcr_offset = 0x0,
	.src_sel_mask = 0x7F0,
	.src_sel_shift = 4,
	.post_div_mask = 0x7800,
	.post_div_shift = 11,
	.post_div_val = 1,
	.mux_sels = apss_cc_debug_mux_sels,
	.num_mux_sels = ARRAY_SIZE(apss_cc_debug_mux_sels),
	.pre_div_vals = apss_cc_debug_mux_pre_divs,
	.hw.init = &(const struct clk_init_data){
		.name = "apss_cc_debug_mux",
		.ops = &clk_debug_mux_ops,
		.parent_names = apss_cc_debug_mux_parent_names,
		.num_parents = ARRAY_SIZE(apss_cc_debug_mux_parent_names),
	},
};

static const char *const gcc_debug_mux_parent_names[] = {
	"apss_cc_debug_mux",
	"gcc_aggre_noc_pcie_axi_clk",
	"gcc_aggre_usb3_prim_axi_clk",
	"gcc_boot_rom_ahb_clk",
	"gcc_camera_hf_axi_clk",
	"gcc_camera_sf_axi_clk",
	"gcc_camss_axi_clk",
	"gcc_camss_camnoc_atb_clk",
	"gcc_camss_camnoc_nts_xo_clk",
	"gcc_camss_cci_0_clk",
	"gcc_camss_cphy_0_clk",
	"gcc_camss_csi0phytimer_clk",
	"gcc_camss_mclk0_clk",
	"gcc_camss_mclk1_clk",
	"gcc_camss_ope_ahb_clk",
	"gcc_camss_ope_clk",
	"gcc_camss_tfe_0_clk",
	"gcc_camss_tfe_0_cphy_rx_clk",
	"gcc_camss_tfe_0_csid_clk",
	"gcc_camss_tfe_1_clk",
	"gcc_camss_tfe_1_cphy_rx_clk",
	"gcc_camss_tfe_1_csid_clk",
	"gcc_camss_top_ahb_clk",
	"gcc_camss_top_shift_clk",
	"gcc_cfg_noc_pcie_anoc_ahb_clk",
	"gcc_cfg_noc_usb3_prim_axi_clk",
	"gcc_cnoc_pcie_sf_axi_clk",
	"gcc_ddrss_gpu_axi_clk",
	"gcc_ddrss_pcie_sf_qtb_clk",
	"gcc_disp_gpll0_clk_src",
	"gcc_disp_hf_axi_clk",
	"gcc_gp1_clk",
	"gcc_gp2_clk",
	"gcc_gp3_clk",
	"gcc_gpu_gemnoc_gfx_clk",
	"gcc_gpu_gpll0_clk_src",
	"gcc_gpu_gpll0_div_clk_src",
	"gcc_pcie_0_aux_clk",
	"gcc_pcie_0_cfg_ahb_clk",
	"gcc_pcie_0_mstr_axi_clk",
	"gcc_pcie_0_phy_rchng_clk",
	"gcc_pcie_0_pipe_clk",
	"gcc_pcie_0_pipe_div2_clk",
	"gcc_pcie_0_slv_axi_clk",
	"gcc_pcie_0_slv_q2a_axi_clk",
	"gcc_pdm2_clk",
	"gcc_pdm_ahb_clk",
	"gcc_pdm_xo4_clk",
	"gcc_pwm0_xo512_clk",
	"gcc_qmip_aod_noc_ahb_clk",
	"gcc_qmip_camera_nrt_ahb_clk",
	"gcc_qmip_camera_rt_ahb_clk",
	"gcc_qmip_gpu_ahb_clk",
	"gcc_qmip_pcie_ahb_clk",
	"gcc_qmip_video_cv_cpu_ahb_clk",
	"gcc_qmip_video_cvp_ahb_clk",
	"gcc_qmip_video_v_cpu_ahb_clk",
	"gcc_qmip_video_vcodec_ahb_clk",
	"gcc_qupv3_wrap1_core_2x_clk",
	"gcc_qupv3_wrap1_core_clk",
	"gcc_qupv3_wrap1_qspi_ref_clk",
	"gcc_qupv3_wrap1_s0_clk",
	"gcc_qupv3_wrap1_s10_clk",
	"gcc_qupv3_wrap1_s1_clk",
	"gcc_qupv3_wrap1_s2_clk",
	"gcc_qupv3_wrap1_s3_clk",
	"gcc_qupv3_wrap1_s4_clk",
	"gcc_qupv3_wrap1_s5_clk",
	"gcc_qupv3_wrap1_s6_clk",
	"gcc_qupv3_wrap1_s7_clk",
	"gcc_qupv3_wrap1_s8_clk",
	"gcc_qupv3_wrap1_s9_clk",
	"gcc_qupv3_wrap_1_m_ahb_clk",
	"gcc_qupv3_wrap_1_s_ahb_clk",
	"gcc_sdcc1_ahb_clk",
	"gcc_sdcc1_apps_clk",
	"gcc_sdcc1_ice_core_clk",
	"gcc_sdcc2_ahb_clk",
	"gcc_sdcc2_apps_clk",
	"gcc_usb30_prim_master_clk",
	"gcc_usb30_prim_mock_utmi_clk",
	"gcc_usb30_prim_sleep_clk",
	"gcc_usb3_prim_phy_aux_clk",
	"gcc_usb3_prim_phy_com_aux_clk",
	"gcc_usb3_prim_phy_pipe_clk",
	"gcc_vcodec0_axi_clk",
	"gcc_venus_ctl_axi_clk",
	"gcc_video_vcodec0_sys_clk",
	"gcc_video_venus_ctl_clk",
	"gpu_cc_debug_mux",
	"mc_cc_debug_mux",
	"measure_only_cnoc_clk",
	"measure_only_gcc_camera_ahb_clk",
	"measure_only_gcc_camera_xo_clk",
	"measure_only_gcc_disp_ahb_clk",
	"measure_only_gcc_gpu_cfg_ahb_clk",
	"measure_only_gcc_sys_noc_cpuss_ahb_clk",
	"measure_only_gcc_video_ahb_clk",
	"measure_only_gcc_video_xo_clk",
	"measure_only_ipa_2x_clk",
	"measure_only_pcie_0_pipe_clk",
	"measure_only_snoc_clk",
	"measure_only_usb3_phy_wrapper_gcc_usb30_pipe_clk",
};

static int gcc_debug_mux_sels[] = {
	0x149,		/* apss_cc_debug_mux */
	0x48,		/* gcc_aggre_noc_pcie_axi_clk */
	0x49,		/* gcc_aggre_usb3_prim_axi_clk */
	0xF5,		/* gcc_boot_rom_ahb_clk */
	0x85,		/* gcc_camera_hf_axi_clk */
	0x86,		/* gcc_camera_sf_axi_clk */
	0x7F,		/* gcc_camss_axi_clk */
	0x81,		/* gcc_camss_camnoc_atb_clk */
	0x84,		/* gcc_camss_camnoc_nts_xo_clk */
	0x7D,		/* gcc_camss_cci_0_clk */
	0x74,		/* gcc_camss_cphy_0_clk */
	0x6B,		/* gcc_camss_csi0phytimer_clk */
	0x6C,		/* gcc_camss_mclk0_clk */
	0x6D,		/* gcc_camss_mclk1_clk */
	0x7C,		/* gcc_camss_ope_ahb_clk */
	0x7A,		/* gcc_camss_ope_clk */
	0x6E,		/* gcc_camss_tfe_0_clk */
	0x72,		/* gcc_camss_tfe_0_cphy_rx_clk */
	0x75,		/* gcc_camss_tfe_0_csid_clk */
	0x70,		/* gcc_camss_tfe_1_clk */
	0x73,		/* gcc_camss_tfe_1_cphy_rx_clk */
	0x77,		/* gcc_camss_tfe_1_csid_clk */
	0x7E,		/* gcc_camss_top_ahb_clk */
	0x83,		/* gcc_camss_top_shift_clk */
	0x36,		/* gcc_cfg_noc_pcie_anoc_ahb_clk */
	0x26,		/* gcc_cfg_noc_usb3_prim_axi_clk */
	0x20,		/* gcc_cnoc_pcie_sf_axi_clk */
	0x117,		/* gcc_ddrss_gpu_axi_clk */
	0x112,		/* gcc_ddrss_pcie_sf_qtb_clk */
	0x8B,		/* gcc_disp_gpll0_clk_src */
	0x8A,		/* gcc_disp_hf_axi_clk */
	0x156,		/* gcc_gp1_clk */
	0x157,		/* gcc_gp2_clk */
	0x158,		/* gcc_gp3_clk */
	0x18A,		/* gcc_gpu_gemnoc_gfx_clk */
	0x18C,		/* gcc_gpu_gpll0_clk_src */
	0x18D,		/* gcc_gpu_gpll0_div_clk_src */
	0x15E,		/* gcc_pcie_0_aux_clk */
	0x15D,		/* gcc_pcie_0_cfg_ahb_clk */
	0x15C,		/* gcc_pcie_0_mstr_axi_clk */
	0x160,		/* gcc_pcie_0_phy_rchng_clk */
	0x15F,		/* gcc_pcie_0_pipe_clk */
	0x161,		/* gcc_pcie_0_pipe_div2_clk */
	0x15B,		/* gcc_pcie_0_slv_axi_clk */
	0x15A,		/* gcc_pcie_0_slv_q2a_axi_clk */
	0xE5,		/* gcc_pdm2_clk */
	0xE3,		/* gcc_pdm_ahb_clk */
	0xE4,		/* gcc_pdm_xo4_clk */
	0xE6,		/* gcc_pwm0_xo512_clk */
	0x9B,		/* gcc_qmip_aod_noc_ahb_clk */
	0x99,		/* gcc_qmip_camera_nrt_ahb_clk */
	0x9A,		/* gcc_qmip_camera_rt_ahb_clk */
	0x187,		/* gcc_qmip_gpu_ahb_clk */
	0x159,		/* gcc_qmip_pcie_ahb_clk */
	0x98,		/* gcc_qmip_video_cv_cpu_ahb_clk */
	0x95,		/* gcc_qmip_video_cvp_ahb_clk */
	0x97,		/* gcc_qmip_video_v_cpu_ahb_clk */
	0x96,		/* gcc_qmip_video_vcodec_ahb_clk */
	0xD6,		/* gcc_qupv3_wrap1_core_2x_clk */
	0xD5,		/* gcc_qupv3_wrap1_core_clk */
	0xE2,		/* gcc_qupv3_wrap1_qspi_ref_clk */
	0xD7,		/* gcc_qupv3_wrap1_s0_clk */
	0xE1,		/* gcc_qupv3_wrap1_s10_clk */
	0xD8,		/* gcc_qupv3_wrap1_s1_clk */
	0xD9,		/* gcc_qupv3_wrap1_s2_clk */
	0xDA,		/* gcc_qupv3_wrap1_s3_clk */
	0xDB,		/* gcc_qupv3_wrap1_s4_clk */
	0xDC,		/* gcc_qupv3_wrap1_s5_clk */
	0xDD,		/* gcc_qupv3_wrap1_s6_clk */
	0xDE,		/* gcc_qupv3_wrap1_s7_clk */
	0xDF,		/* gcc_qupv3_wrap1_s8_clk */
	0xE0,		/* gcc_qupv3_wrap1_s9_clk */
	0xD3,		/* gcc_qupv3_wrap_1_m_ahb_clk */
	0xD4,		/* gcc_qupv3_wrap_1_s_ahb_clk */
	0xCD,		/* gcc_sdcc1_ahb_clk */
	0xCC,		/* gcc_sdcc1_apps_clk */
	0xCF,		/* gcc_sdcc1_ice_core_clk */
	0xD1,		/* gcc_sdcc2_ahb_clk */
	0xD0,		/* gcc_sdcc2_apps_clk */
	0xC0,		/* gcc_usb30_prim_master_clk */
	0xC2,		/* gcc_usb30_prim_mock_utmi_clk */
	0xC1,		/* gcc_usb30_prim_sleep_clk */
	0xC3,		/* gcc_usb3_prim_phy_aux_clk */
	0xC4,		/* gcc_usb3_prim_phy_com_aux_clk */
	0xC5,		/* gcc_usb3_prim_phy_pipe_clk */
	0x9D,		/* gcc_vcodec0_axi_clk */
	0x9C,		/* gcc_venus_ctl_axi_clk */
	0x91,		/* gcc_video_vcodec0_sys_clk */
	0x8F,		/* gcc_video_venus_ctl_clk */
	0x189,		/* gpu_cc_debug_mux */
	0x11E,		/* mc_cc_debug_mux */
	0x21,		/* measure_only_cnoc_clk */
	0x94,		/* measure_only_gcc_camera_ahb_clk */
	0x9F,		/* measure_only_gcc_camera_xo_clk */
	0x89,		/* measure_only_gcc_disp_ahb_clk */
	0x186,		/* measure_only_gcc_gpu_cfg_ahb_clk */
	0xB,		/* measure_only_gcc_sys_noc_cpuss_ahb_clk */
	0x93,		/* measure_only_gcc_video_ahb_clk */
	0x9E,		/* measure_only_gcc_video_xo_clk */
	0x173,		/* measure_only_ipa_2x_clk */
	0x162,		/* measure_only_pcie_0_pipe_clk */
	0xE,		/* measure_only_snoc_clk */
	0xC9,		/* measure_only_usb3_phy_wrapper_gcc_usb30_pipe_clk */
};

static struct clk_debug_mux gcc_debug_mux = {
	.priv = &debug_mux_priv,
	.debug_offset = 0x72024,
	.post_div_offset = 0x62000,
	.cbcr_offset = 0x62004,
	.src_sel_mask = 0x3FF,
	.src_sel_shift = 0,
	.post_div_mask = 0xF,
	.post_div_shift = 0,
	.post_div_val = 2,
	.mux_sels = gcc_debug_mux_sels,
	.num_mux_sels = ARRAY_SIZE(gcc_debug_mux_sels),
	.hw.init = &(const struct clk_init_data){
		.name = "gcc_debug_mux",
		.ops = &clk_debug_mux_ops,
		.parent_names = gcc_debug_mux_parent_names,
		.num_parents = ARRAY_SIZE(gcc_debug_mux_parent_names),
	},
};

static const char *const gpu_cc_debug_mux_parent_names[] = {
	"gpu_cc_ahb_clk",
	"gpu_cc_crc_ahb_clk",
	"gpu_cc_cx_accu_shift_clk",
	"gpu_cc_cx_ff_clk",
	"gpu_cc_cx_gmu_clk",
	"gpu_cc_cxo_clk",
	"gpu_cc_freq_measure_clk",
	"gpu_cc_gx_accu_shift_clk",
	"gpu_cc_gx_gmu_clk",
	"gpu_cc_gx_vsense_clk",
	"gpu_cc_hub_aon_clk",
	"gpu_cc_hub_cx_int_clk",
	"gpu_cc_memnoc_gfx_clk",
	"gpu_cc_mnd1x_0_gfx3d_clk",
	"gpu_cc_mnd1x_1_gfx3d_clk",
	"measure_only_gpu_cc_cb_clk",
	"measure_only_gpu_cc_cx_gfx3d_clk",
	"measure_only_gpu_cc_cx_gfx3d_slv_clk",
	"measure_only_gpu_cc_cxo_aon_clk",
	"measure_only_gpu_cc_demet_clk",
	"measure_only_gpu_cc_gx_gfx3d_clk",
	"measure_only_gpu_cc_sleep_clk",
};

static int gpu_cc_debug_mux_sels[] = {
	0x15,		/* gpu_cc_ahb_clk */
	0x16,		/* gpu_cc_crc_ahb_clk */
	0x2F,		/* gpu_cc_cx_accu_shift_clk */
	0x1F,		/* gpu_cc_cx_ff_clk */
	0x1C,		/* gpu_cc_cx_gmu_clk */
	0x1D,		/* gpu_cc_cxo_clk */
	0xB,		/* gpu_cc_freq_measure_clk */
	0x2E,		/* gpu_cc_gx_accu_shift_clk */
	0x11,		/* gpu_cc_gx_gmu_clk */
	0xE,		/* gpu_cc_gx_vsense_clk */
	0x2C,		/* gpu_cc_hub_aon_clk */
	0x1E,		/* gpu_cc_hub_cx_int_clk */
	0x20,		/* gpu_cc_memnoc_gfx_clk */
	0x27,		/* gpu_cc_mnd1x_0_gfx3d_clk */
	0x28,		/* gpu_cc_mnd1x_1_gfx3d_clk */
	0x2B,		/* measure_only_gpu_cc_cb_clk */
	0x23,		/* measure_only_gpu_cc_cx_gfx3d_clk */
	0x24,		/* measure_only_gpu_cc_cx_gfx3d_slv_clk */
	0xA,		/* measure_only_gpu_cc_cxo_aon_clk */
	0xC,		/* measure_only_gpu_cc_demet_clk */
	0xD,		/* measure_only_gpu_cc_gx_gfx3d_clk */
	0x1A,		/* measure_only_gpu_cc_sleep_clk */
};

static struct clk_debug_mux gpu_cc_debug_mux = {
	.priv = &debug_mux_priv,
	.debug_offset = 0x9564,
	.post_div_offset = 0x928C,
	.cbcr_offset = 0x9290,
	.src_sel_mask = 0xFF,
	.src_sel_shift = 0,
	.post_div_mask = 0xF,
	.post_div_shift = 0,
	.post_div_val = 2,
	.mux_sels = gpu_cc_debug_mux_sels,
	.num_mux_sels = ARRAY_SIZE(gpu_cc_debug_mux_sels),
	.hw.init = &(const struct clk_init_data){
		.name = "gpu_cc_debug_mux",
		.ops = &clk_debug_mux_ops,
		.parent_names = gpu_cc_debug_mux_parent_names,
		.num_parents = ARRAY_SIZE(gpu_cc_debug_mux_parent_names),
	},
};

static const char *const mc_cc_debug_mux_parent_names[] = {
	"measure_only_mccc_clk",
};

static struct clk_debug_mux mc_cc_debug_mux = {
	.period_offset = 0x50,
	.hw.init = &(struct clk_init_data){
		.name = "mc_cc_debug_mux",
		.ops = &clk_debug_mux_ops,
		.parent_names = mc_cc_debug_mux_parent_names,
		.num_parents = ARRAY_SIZE(mc_cc_debug_mux_parent_names),
	},
};

static struct mux_regmap_names mux_list[] = {
	{ .mux = &mc_cc_debug_mux, .regmap_name = "qcom,mccc" },
	{ .mux = &gpu_cc_debug_mux, .regmap_name = "qcom,gpucc" },
	{ .mux = &apss_cc_debug_mux, .regmap_name = "qcom,apsscc" },
	{ .mux = &gcc_debug_mux, .regmap_name = "qcom,gcc" },
};

static struct clk_dummy measure_only_apcs_gold_post_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_gold_post_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_apcs_gold_pre_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_gold_pre_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_apcs_l3_post_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_l3_post_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_apcs_l3_pre_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_l3_pre_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_apcs_silver_post_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_silver_post_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_apcs_silver_pre_acd_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_apcs_silver_pre_acd_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_cnoc_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_cnoc_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_camera_ahb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_camera_ahb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_camera_xo_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_camera_xo_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_disp_ahb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_disp_ahb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_gpu_cfg_ahb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_gpu_cfg_ahb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_sys_noc_cpuss_ahb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_sys_noc_cpuss_ahb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_video_ahb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_video_ahb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gcc_video_xo_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gcc_video_xo_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_cb_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_cb_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_cx_gfx3d_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_cx_gfx3d_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_cx_gfx3d_slv_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_cx_gfx3d_slv_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_cxo_aon_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_cxo_aon_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_demet_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_demet_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_gx_gfx3d_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_gx_gfx3d_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_gpu_cc_sleep_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_gpu_cc_sleep_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_ipa_2x_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_ipa_2x_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_mccc_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_mccc_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_pcie_0_pipe_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_pcie_0_pipe_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_snoc_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_snoc_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_dummy measure_only_usb3_phy_wrapper_gcc_usb30_pipe_clk = {
	.rrate = 1000,
	.hw.init = &(const struct clk_init_data){
		.name = "measure_only_usb3_phy_wrapper_gcc_usb30_pipe_clk",
		.ops = &clk_dummy_ops,
	},
};

static struct clk_hw *debugcc_vienna_hws[] = {
	&measure_only_apcs_gold_post_acd_clk.hw,
	&measure_only_apcs_gold_pre_acd_clk.hw,
	&measure_only_apcs_l3_post_acd_clk.hw,
	&measure_only_apcs_l3_pre_acd_clk.hw,
	&measure_only_apcs_silver_post_acd_clk.hw,
	&measure_only_apcs_silver_pre_acd_clk.hw,
	&measure_only_cnoc_clk.hw,
	&measure_only_gcc_camera_ahb_clk.hw,
	&measure_only_gcc_camera_xo_clk.hw,
	&measure_only_gcc_disp_ahb_clk.hw,
	&measure_only_gcc_gpu_cfg_ahb_clk.hw,
	&measure_only_gcc_sys_noc_cpuss_ahb_clk.hw,
	&measure_only_gcc_video_ahb_clk.hw,
	&measure_only_gcc_video_xo_clk.hw,
	&measure_only_gpu_cc_cb_clk.hw,
	&measure_only_gpu_cc_cx_gfx3d_clk.hw,
	&measure_only_gpu_cc_cx_gfx3d_slv_clk.hw,
	&measure_only_gpu_cc_cxo_aon_clk.hw,
	&measure_only_gpu_cc_demet_clk.hw,
	&measure_only_gpu_cc_gx_gfx3d_clk.hw,
	&measure_only_gpu_cc_sleep_clk.hw,
	&measure_only_ipa_2x_clk.hw,
	&measure_only_mccc_clk.hw,
	&measure_only_pcie_0_pipe_clk.hw,
	&measure_only_snoc_clk.hw,
	&measure_only_usb3_phy_wrapper_gcc_usb30_pipe_clk.hw,
};

static const struct of_device_id clk_debug_match_table[] = {
	{ .compatible = "qcom,vienna-debugcc" },
	{ }
};

static int clk_debug_vienna_probe(struct platform_device *pdev)
{
	struct clk *clk;
	int ret = 0, i;

	BUILD_BUG_ON(ARRAY_SIZE(apss_cc_debug_mux_parent_names) !=
		ARRAY_SIZE(apss_cc_debug_mux_sels));
	BUILD_BUG_ON(ARRAY_SIZE(gcc_debug_mux_parent_names) != ARRAY_SIZE(gcc_debug_mux_sels));
	BUILD_BUG_ON(ARRAY_SIZE(gpu_cc_debug_mux_parent_names) !=
		ARRAY_SIZE(gpu_cc_debug_mux_sels));

	clk = devm_clk_get(&pdev->dev, "xo_clk_src");
	if (IS_ERR(clk)) {
		if (PTR_ERR(clk) != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Unable to get xo clock\n");
		return PTR_ERR(clk);
	}

	debug_mux_priv.cxo = clk;

	for (i = 0; i < ARRAY_SIZE(mux_list); i++) {
		if (IS_ERR_OR_NULL(mux_list[i].mux->regmap)) {
			ret = map_debug_bases(pdev, mux_list[i].regmap_name,
					      mux_list[i].mux);
			if (ret == -EBADR)
				continue;
			else if (ret)
				return ret;
		}
	}

	for (i = 0; i < ARRAY_SIZE(debugcc_vienna_hws); i++) {
		clk = devm_clk_register(&pdev->dev, debugcc_vienna_hws[i]);
		if (IS_ERR(clk)) {
			dev_err(&pdev->dev, "Unable to register %s, err:(%ld)\n",
				qcom_clk_hw_get_name(debugcc_vienna_hws[i]),
				PTR_ERR(clk));
			return PTR_ERR(clk);
		}
	}

	for (i = 0; i < ARRAY_SIZE(mux_list); i++) {
		ret = devm_clk_register_debug_mux(&pdev->dev, mux_list[i].mux);
		if (ret) {
			dev_err(&pdev->dev, "Unable to register mux clk %s, err:(%d)\n",
				qcom_clk_hw_get_name(&mux_list[i].mux->hw),
				ret);
			return ret;
		}
	}

	ret = clk_debug_measure_register(&gcc_debug_mux.hw);
	if (ret) {
		dev_err(&pdev->dev, "Could not register Measure clocks\n");
		return ret;
	}

	dev_info(&pdev->dev, "Registered debug measure clocks\n");

	return ret;
}

static struct platform_driver clk_debug_driver = {
	.probe = clk_debug_vienna_probe,
	.driver = {
		.name = "vienna-debugcc",
		.of_match_table = clk_debug_match_table,
	},
};

static int __init clk_debug_vienna_init(void)
{
	return platform_driver_register(&clk_debug_driver);
}
fs_initcall(clk_debug_vienna_init);

MODULE_DESCRIPTION("QTI DEBUG CC VIENNA Driver");
MODULE_LICENSE("GPL");
