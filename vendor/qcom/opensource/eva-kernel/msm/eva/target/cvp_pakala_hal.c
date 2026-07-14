// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "cvp_pakala_hal.h"

extern struct cvp_hal_ops hal_ops;

void __check_tensilica_in_reset_pakala(struct iris_hfi_device *device)
{
	u32 xtss_reset_ro = 1;

#ifdef CONFIG_EVA_SUN
	xtss_reset_ro = __read_register(device, CVP_WRAPPER_XTSS_SW_RESET_RO);
#endif
	dprintk(CVP_WARN, "tensilica xtss_reset_ro %#x\n", xtss_reset_ro);
}

static void __enter_cpu_noc_lpi(struct iris_hfi_device *device)
{
	u32 lpi_status, count = 0, max_count = 2000;

	/* New addition to put CPU/Tensilica NOC to low power Section 6.14
	 * (Steps 15-17)
	 *
	 * Clear CVP_iris_cpu_noc_errorLogger_ErrVld_Low SWI
	 * by writing 0x1 to CVP_NOC_ERR_ERRCLR_LOW_OFFS
	 */
	__write_register(device, CVP_NOC_ERR_ERRCLR_LOW_OFFS, 0x1);
	__write_register(device, CVP_WRAPPER_CPU_NOC_LPI_CONTROL, 0x1);
	while (count < max_count) {
		lpi_status =
		    __read_register(device, CVP_WRAPPER_CPU_NOC_LPI_STATUS);
		if ((lpi_status & BIT(1)) ||
		    ((lpi_status & BIT(2)) && (!(lpi_status & BIT(0))))) {
			/*
			 * If QDENY == true, or
			 * If QACTIVE == true && QACCEPT == false
			 * Try again
			 */
			__write_register(device,
					 CVP_WRAPPER_CPU_NOC_LPI_CONTROL, 0x0);
			usleep_range(10, 20);
			__write_register(device, CVP_NOC_ERR_ERRCLR_LOW_OFFS,
					 0x1);
			__write_register(device,
					 CVP_WRAPPER_CPU_NOC_LPI_CONTROL, 0x1);
			usleep_range(1000, 1200);
			count++;
		} else {
			break;
		}
	}

	dprintk(CVP_PWR, "%s, CPU Noc: lpi_status %x (count %d)\n", __func__,
		lpi_status, count);
	/* HPG Step-7 of section 3.7 */
	// __write_register(device, CVP_WRAPPER_CPU_NOC_LPI_CONTROL, 0x0);
	if (count == max_count) {
		u32 pc_ready, wfi_status;

		wfi_status = __read_register(device, CVP_WRAPPER_CPU_STATUS);
		pc_ready = __read_register(device, CVP_CTRL_STATUS);

		dprintk(CVP_WARN,
			"%s, CPU NOC not in qaccept status %x %x %x\n",
			__func__, lpi_status, wfi_status, pc_ready);

		/* Added for debug info purpose, not part of HPG */
		call_iris_op(device, print_sbm_regs, device);
	}
}

static void __enter_core_noc_lpi(struct iris_hfi_device *device)
{
	u32 lpi_status, value, count = 0, max_count = 2000;

	/* New addition to put CORE NOC to low power Section 6.14 (Steps 4-6)*/
	/*
	 * Clear CVP_NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRVLD_LOW SWI
	 * by writing 0x1 to CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS
	 */
	__write_register(device, CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS, 0x1);
	__write_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL, 0x1);
	while (count < max_count) {
		/* Reading the LPI status */
		lpi_status =
		    __read_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_STATUS);
		if ((lpi_status & BIT(1)) ||
		    ((lpi_status & BIT(2)) && (!(lpi_status & BIT(0))))) {
			/*
			 * If QDENY == true, or
			 * If QACTIVE == true && QACCEPT == false
			 * Try again
			 */
			__write_register(
			    device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL, 0x0);
			usleep_range(10, 20);
			__write_register(device,
					 CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS, 0x1);
			__write_register(
			    device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL, 0x1);
			usleep_range(1000, 1200);
			count++;
		} else {
			break;
		}
	}

	dprintk(CVP_PWR, "%s, CORE Noc: lpi_status %x (count %d)\n", __func__,
		lpi_status, count);
	/* HPG Step-4 of section 3.4.4 */
	// __write_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL, 0x0);
	if (count == max_count) {
		dprintk(CVP_WARN, "%s, CORE NOC not in qaccept status %x\n",
			__func__, lpi_status);

		/* Added for debug info purpose, not part of HPG */
		call_iris_op(device, print_sbm_regs, device);
	}

	/* HPG 3.4.4 Step 4-5 */
	count = 0;
	do {
		value =
		    __read_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_STATUS);
		if (value & 0x0)
			break;
		usleep_range(1000, 2000);
		count++;
	} while (count < 10);

	__write_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL, 0x0);
}

static void __enter_video_ctl_noc_lpi(struct iris_hfi_device *device)
{
	u32 lpi_status, count = 0, max_count = 2000;

	/* New addition to put CVP_VIDEO_CTL NOC to low power Section 6.14
	 * (Steps 19-21)
	 */

	__write_register(device, CVP_AON_WRAPPER_CVP_VIDEO_CTL_NOC_LPI_CONTROL,
			 0x1);
	while (count < max_count) {
		/* Reading the LPI status */
		lpi_status = __read_register(
		    device, CVP_AON_WRAPPER_CVP_VIDEO_CTL_NOC_LPI_STATUS);
		if ((lpi_status & BIT(1)) ||
		    ((lpi_status & BIT(2)) && (!(lpi_status & BIT(0))))) {
			/*
			 * If QDENY == true, or
			 * If QACTIVE == true && QACCEPT == false
			 * Try again
			 */
			__write_register(
			    device,
			    CVP_AON_WRAPPER_CVP_VIDEO_CTL_NOC_LPI_CONTROL, 0x0);
			usleep_range(10, 20);
			__write_register(
			    device,
			    CVP_AON_WRAPPER_CVP_VIDEO_CTL_NOC_LPI_CONTROL, 0x1);
			usleep_range(1000, 1200);
			count++;
		} else {
			break;
		}
	}

	dprintk(CVP_PWR, "%s, CVP_VIDEO_CTL Noc: lpi_status %x (count %d)\n",
		__func__, lpi_status, count);
	/* HPG Step-22 of section 6.14 */
	__write_register(device, CVP_AON_WRAPPER_CVP_VIDEO_CTL_NOC_LPI_CONTROL,
			 0x0);
	if (count == max_count) {
		dprintk(
		    CVP_WARN,
		    "%s, CVP_VIDEO_CTL NOC not in qaccept status %x %x %x\n",
		    __func__, lpi_status);

		/* Added for debug info purpose, not part of HPG */
		call_iris_op(device, print_sbm_regs, device);
	}
}

void setup_dsp_uc_memmap_vpu5_pakala(struct iris_hfi_device *device)
{
	/* initialize DSP QTBL & UCREGION with CPU queues */
#ifdef USE_PRESIL42
	presil42_setup_dsp_uc_memmap_vpu5(device);
	return;
#endif
	__write_register(device, HFI_DSP_QTBL_ADDR,
			 (u32)device->dsp_iface_q_table.align_device_addr);
	__write_register(device, HFI_DSP_UC_REGION_ADDR,
			 (u32)device->dsp_iface_q_table.align_device_addr);
	__write_register(device, HFI_DSP_UC_REGION_SIZE,
			 device->dsp_iface_q_table.mem_data.size);
}

void interrupt_init_iris2_pakala(struct iris_hfi_device *device)
{
	u32 mask_val = 0;

	/* All interrupts should be disabled initially 0x1F6 : Reset value */
	mask_val = __read_register(device, CVP_WRAPPER_INTR_MASK);

	/* Write 0 to unmask CPU and WD interrupts */
	mask_val &= ~(CVP_FATAL_INTR_BMSK | CVP_WRAPPER_INTR_MASK_A2HCPU_BMSK);
	__write_register(device, CVP_WRAPPER_INTR_MASK, mask_val);
	dprintk(CVP_REG, "Init irq: reg: %x, mask value %x\n",
		CVP_WRAPPER_INTR_MASK, mask_val);

	mask_val = 0;
	mask_val = __read_register(device, CVP_SS_IRQ_MASK);
	mask_val &= ~(CVP_SS_INTR_BMASK);
	__write_register(device, CVP_SS_IRQ_MASK, mask_val);
	dprintk(CVP_REG, "Init irq_wd: reg: %x, mask value %x\n",
		CVP_SS_IRQ_MASK, mask_val);
}

int __check_ctl_power_on_pakala(struct iris_hfi_device *device)
{
	u32 reg;

	reg = __read_register(device, CVP_CC_MVS0C_GDSCR);
	if (!(reg & 0x80000000))
		return -1;

	reg = __read_register(device, CVP_CC_MVS0C_CBCR);
	if (reg & 0x80000000)
		return -2;

	return 0;
}

int __check_core_power_on_pakala(struct iris_hfi_device *device)
{
	u32 reg;

	reg = __read_register(device, CVP_CC_MVS0_GDSCR);
	if (!(reg & 0x80000000))
		return -1;

	reg = __read_register(device, CVP_CC_MVS0_CBCR);
	if (reg & 0x80000000)
		return -2;

	return 0;
}

int __power_on_controller_pakala(struct iris_hfi_device *device)
{
	int rc = 0;

	CVPKERNEL_ATRACE_BEGIN("__power_on_controller_v1");

	rc = __enable_gdsc(device, "controller");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable ctrler: %d\n", rc);
		return rc;
	}

	rc = msm_cvp_prepare_enable_clk(device, "sleep_clk");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable sleep clk: %d\n", rc);
		goto fail_reset_sleep;
	}
	/*
	 *After adding AXI0C and FREERUN resets in DTSI, changing below code
	 *rc = call_iris_op(device, reset_control_assert_name, device,
	 *"cvp_axi_reset"); if (rc) dprintk(CVP_ERR, "%s: assert cvp_axi_reset
	 *failed\n", __func__);
	 *rc = call_iris_op(device, reset_control_assert_name, device,
	 *"cvp_core_reset"); if (rc) dprintk(CVP_ERR, "%s: assert cvp_core_reset
	 *failed\n", __func__);
	 *usleep_range(300, 400);
	 *rc = call_iris_op(device, reset_control_deassert_name, device,
	 *"cvp_axi_reset"); if (rc) dprintk(CVP_ERR, "%s: de-assert cvp_axi_reset
	 *failed\n", __func__); rc = call_iris_op(device,
	 *reset_control_deassert_name, device, "cvp_core_reset"); if (rc)
	 *   dprintk(CVP_ERR, "%s: de-assert cvp_core_reset failed\n", __func__);
	 */
	rc = msm_cvp_prepare_enable_clk(device, "core_axi_clock");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable axi0 clk: %d\n", rc);
		goto fail_enable_axi0;
	}

	rc = msm_cvp_prepare_enable_clk(device, "cvp_axi_clock");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable axi0c clk: %d\n", rc);
		goto fail_enable_axi0c;
	}

	rc = msm_cvp_prepare_enable_clk(device, "cvp_clk");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable cvp_clk: %d\n", rc);
		goto fail_enable_cvp;
	}

	rc = msm_cvp_prepare_enable_clk(device, "cvp_freerun_clk");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable cvp_freerun_clk: %d\n", rc);
		goto fail_enable_freerun;
	}

	dprintk(CVP_PWR, "EVA controller powered on\n");
	CVPKERNEL_ATRACE_END("__power_on_controller_v1");
	return 0;

fail_enable_freerun:
	msm_cvp_disable_unprepare_clk(device, "cvp");
fail_enable_cvp:
	msm_cvp_disable_unprepare_clk(device, "cvp_axi_clock");
fail_enable_axi0c:
	msm_cvp_disable_unprepare_clk(device, "core_axi_clock");
fail_enable_axi0:
	msm_cvp_disable_unprepare_clk(device, "sleep_clk");
fail_reset_sleep:
	__disable_gdsc(device, "controller");
	CVPKERNEL_ATRACE_END("__power_on_controller_v1");
	return rc;
}

int __power_on_core_pakala(struct iris_hfi_device *device)
{
	int rc = 0;

	CVPKERNEL_ATRACE_BEGIN("__power_on_core_v1");

	rc = __enable_gdsc(device, "core");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable core: %d\n", rc);
		return rc;
	}

	rc = msm_cvp_prepare_enable_clk(device, "eva_cc_mvs0_clk_src");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable eva_cc_mvs0_clk_src:%d\n",
			rc);
		goto fail_enable_clk_src;
	}

	rc = msm_cvp_prepare_enable_clk(device, "core_clk");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable core_clk: %d\n", rc);
		goto fail_enable_core;
	}

	rc = msm_cvp_prepare_enable_clk(device, "core_freerun_clk");
	if (rc) {
		dprintk(CVP_ERR, "Failed to enable core_freerun_clk: %d\n", rc);
		goto fail_enable_freerun;
	}

	dprintk(CVP_PWR, "EVA core powered on\n");
	CVPKERNEL_ATRACE_END("__power_on_core_v1");

	return 0;

fail_enable_freerun:
	msm_cvp_disable_unprepare_clk(device, "core_clk");
fail_enable_core:
	msm_cvp_disable_unprepare_clk(device, "eva_cc_mvs0_clk_src");
fail_enable_clk_src:
	__disable_gdsc(device, "core");
	return rc;
}

int __power_off_core_pakala(struct iris_hfi_device *device)
{
	u32 config, value = 0, count = 0;
	u32 max_count = 10;

	value = __read_register(device, CVP_CC_MVS0_GDSCR);
	if (!(value & 0x80000000)) {
		/*
		 * Core has been powered off by f/w.
		 * Check NOC reset registers to ensure
		 * NO outstanding NoC transactions
		 */
		value = __read_register(device, CVP_NOC_RESET_ACK);
		if (value) {
			dprintk(CVP_WARN,
				"Core off with NOC RESET ACK non-zero %x\n",
				value);
			call_iris_op(device, print_sbm_regs, device);
		}
		__disable_gdsc(device, "core");
		msm_cvp_disable_unprepare_clk(device, "core_clk");
		return 0;
	} else if (!(value & 0x2) && msm_cvp_fw_low_power_mode) {
		/*
		 * HW_CONTROL PC disabled, then core is powered on for
		 * CVP NoC access
		 */
		__disable_gdsc(device, "core");
		msm_cvp_disable_unprepare_clk(device, "core_clk");
		msm_cvp_disable_unprepare_clk(device, "core_freerun_clk");
		return 0;
	}

	dprintk(CVP_PWR, "Driver controls Core power off now\n");

	/* HPG 3.4.4 step 1 */
	/*
	 * check to make sure core clock branch enabled else
	 * we cannot read core idle register
	 */
	config = __read_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG);
	if (config) {
		dprintk(CVP_PWR, "core clock config not enabled\n");
		__write_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG, 0);
	}

	/*
	 * add MNoC idle check before collapsing MVS1 per HPG update
	 * poll for NoC DMA idle -> HPG 6.2.1
	 *
	 */
	do {
		value = __read_register(device, CVP_SS_IDLE_STATUS);
		if (value & 0x400000)
			goto advance;
		else
			usleep_range(1000, 2000);
		count++;
	} while (count < max_count);

	if (count == max_count)
		dprintk(CVP_WARN, "Core fail to go idle %x\n", value);

advance:
	/* New addition to put CORE NOC to low power Section 6.14 (Steps 4-6)*/
	__enter_core_noc_lpi(device);

	/* HPG 3.4.4 step 5 in sun */
	/* HPG 3.4.4 step 11 in canoe */
	/* Reset both sides of 2 ahb2ahb_bridges (TZ and non-TZ) */
	__write_register(device, CVP_AHB_BRIDGE_SYNC_RESET, 0x3);
	__write_register(device, CVP_AHB_BRIDGE_SYNC_RESET, 0x2);
	__write_register(device, CVP_AHB_BRIDGE_SYNC_RESET, 0x0);
	__write_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG, config);

	/* HPG 3.4.4 step 6-7 */
	__disable_gdsc(device, "core");
	msm_cvp_disable_unprepare_clk(device, "core_clk");
	return 0;
}

int __power_off_controller_pakala(struct iris_hfi_device *device)
{
	u32 lpi_status, count = 0, max_count = 1000;
	u32 lpi_control;
	int rc;

	/* HPG 3.7 Step 4  */
	__write_register(device, CVP_CPU_CS_X2RPMh, 0x3);

	/* New addition to put CPU/Tensilica NOC to low power Section 6.14
	 * (Steps 15-17)
	 */
	__enter_cpu_noc_lpi(device);

	/* New addition to put CVP_VIDEO_CTL NOC to low power Section 6.14
	 * (Steps 19-21)
	 */
	__enter_video_ctl_noc_lpi(device);

	/* HPG 3.7 step 11 */
	__write_register(device, CVP_WRAPPER_DEBUG_BRIDGE_LPI_CONTROL, 0x0);

	/* HPG 3.7 step 12 */
	lpi_status = 0x1;
	count = 0;
	while (lpi_status && count < max_count) {
		lpi_status = __read_register(
		    device, CVP_WRAPPER_DEBUG_BRIDGE_LPI_STATUS);
		usleep_range(50, 100);
		count++;
	}
	dprintk(CVP_PWR, "DBLP Release: lpi_status %d(count %d)\n", lpi_status,
		count);
	if (count == max_count)
		dprintk(CVP_WARN, "DBLP Release: lpi_status %x\n", lpi_status);

	lpi_control =
	    __read_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL);
	lpi_control = lpi_control | 0x10;
	__write_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL,
			 lpi_control);
	usleep_range(50, 100);
	lpi_control = lpi_control & (~0x10);
	__write_register(device, CVP_AON_WRAPPER_CVP_NOC_LPI_CONTROL,
			 lpi_control);

	/*
	 * Below sequence are missing from HPG Section 3.7.
	 * It disables EVA_CC clks in power on sequence
	 */
	rc = msm_cvp_disable_unprepare_clk(device, "core_freerun_clk");
	if (rc)
		dprintk(CVP_ERR, "Failed to disable core_freerun_clk: %d\n",
			rc);

	rc = msm_cvp_disable_unprepare_clk(device, "cvp_freerun_clk");
	if (rc)
		dprintk(CVP_ERR, "Failed to disable cvp_freerun_clk: %d\n", rc);

	rc = msm_cvp_disable_unprepare_clk(device, "cvp_clk");
	if (rc)
		dprintk(CVP_ERR, "Failed to disable cvp_clk: %d\n", rc);

	rc = msm_cvp_disable_unprepare_clk(device, "sleep_clk");
	if (rc)
		dprintk(CVP_ERR, "Failed to disable sleep clk: %d\n", rc);

	/* HPG 3.7 Step 13 and 14 */
	__disable_gdsc(device, "controller");

	/* Below sequence are missing from HPG Section 3.7.
	 * It disables GCC clks in power on sequence
	 */
	rc = msm_cvp_disable_unprepare_clk(device, "core_axi_clock");
	rc = msm_cvp_disable_unprepare_clk(device, "cvp_axi_clock");

	/****************** TODO RESET ****************************************
	 * Section 3.8.1
	 *
	 *
	rc = call_iris_op(device, reset_control_assert_name, device,
	"cvp_axi_reset"); if (rc) dprintk(CVP_ERR, "%s: assert cvp_axi_reset
	failed\n", __func__);

	rc = call_iris_op(device, reset_control_assert_name, device,
	"cvp_core_reset"); if (rc) dprintk(CVP_ERR, "%s: assert cvp_core_reset
	failed\n", __func__); usleep_range(1000, 1050);

	rc = call_iris_op(device, reset_control_deassert_name, device,
	"cvp_axi_reset"); if (rc) dprintk(CVP_ERR, "%s: de-assert cvp_axi_reset
	failed\n", __func__);

	rc = call_iris_op(device, reset_control_deassert_name, device,
	"cvp_core_reset"); if (rc) dprintk(CVP_ERR, "%s: de-assert
	cvp_core_reset failed\n", __func__);

	***********************************************************************/
	rc = msm_cvp_disable_unprepare_clk(device, "eva_cc_mvs0_clk_src");
	if (rc) {
		dprintk(CVP_ERR, "Failed to disable eva_cc_mvs0_clk_src: %d\n",
			rc);
	}
	return 0;
}

void __print_sidebandmanager_regs_pakala(struct iris_hfi_device *device)
{
	u32 sbm_ln0_low, axi_cbcr, val;
	u32 main_sbm_ln0_low = 0xdeadbeef, main_sbm_ln0_high = 0xdeadbeef;
	u32 main_sbm_ln1_high = 0xdeadbeef, cpu_cs_x2rpmh;

	sbm_ln0_low = __read_register(device, CVP_NOC_SBM_SENSELN0_LOW);

	cpu_cs_x2rpmh = __read_register(device, CVP_CPU_CS_X2RPMh);

	__write_register(device, CVP_CPU_CS_X2RPMh,
			 (cpu_cs_x2rpmh | CVP_CPU_CS_X2RPMh_SWOVERRIDE_BMSK));
	usleep_range(500, 1000);
	val = __read_register(device, CVP_CPU_CS_X2RPMh);
	dprintk(CVP_REG, "CVP_CPU_CS_X2RPMh %#x\n", val);
	val = __read_register(device, CVP_CPU_CS_X2RPMh_STATUS);
	dprintk(CVP_REG, "CVP_CPU_CS_X2RPMh_STATUS %#x\n", val);

	cpu_cs_x2rpmh = __read_register(device, CVP_CPU_CS_X2RPMh);
	if (!(cpu_cs_x2rpmh & CVP_CPU_CS_X2RPMh_SWOVERRIDE_BMSK)) {
		dprintk(CVP_WARN, "failed set CVP_CPU_CS_X2RPMH mask %x\n",
			cpu_cs_x2rpmh);
		goto exit;
	}

	axi_cbcr = __read_gcc_register(device, CVP_GCC_EVA_AXI0_CBCR);
	if (axi_cbcr & 0x80000000) {
		dprintk(CVP_WARN, "failed to turn on AXI clock %x\n", axi_cbcr);
		goto exit;
	}

	/* Added by Thomas to debug CPU NoC hang */
	val = __read_register(device, CVP_NOC_ERR_ERRVLD_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRVLD_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_SBM_FAULTINSTATUS0_LOW);
	dprintk(CVP_ERR, "CVP_NOC_SBM_FAULTINSTATUS0_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG0_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG0_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG0_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG0_HIGH %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG1_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG1_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG1_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG1_HIGH %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG2_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG2_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG2_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG2_HIGH %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG3_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG3_LOW %#x\n", val);

	val = __read_register(device, CVP_NOC_ERR_ERRLOG3_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_ERL_MAIN_ERRLOG3_HIGH %#x\n", val);

	main_sbm_ln0_low =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_LOW);
	main_sbm_ln0_high =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_HIGH);
	main_sbm_ln1_high =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN1_HIGH);

exit:
	cpu_cs_x2rpmh = cpu_cs_x2rpmh & (~CVP_CPU_CS_X2RPMh_SWOVERRIDE_BMSK);
	__write_register(device, CVP_CPU_CS_X2RPMh, cpu_cs_x2rpmh);
	dprintk(CVP_WARN, "Sidebandmanager regs %x %x %x %x %x\n", sbm_ln0_low,
		main_sbm_ln0_low, main_sbm_ln0_high, main_sbm_ln1_high,
		cpu_cs_x2rpmh);
}

int __enable_hw_power_collapse_pakala(struct iris_hfi_device *device)
{
	int rc = 0, loop = 10;
	u32 reg_gdsc;

	if (!msm_cvp_fw_low_power_mode) {
		dprintk(CVP_PWR, "Not enabling hardware power collapse\n");
		return 0;
	}

	if (device->res->gdsc_framework_type)
		rc = switch_core_gdsc_mode(device, TO_HW_CTRL);
	else
		rc = __hand_off_regulators(device);

	if (rc) {
		dprintk(CVP_WARN,
			"%s : Failed to enable HW power collapse %d\n",
			__func__, rc);
		return rc;
	}

	while (loop) {
		reg_gdsc = __read_register(device, CVP_CC_MVS0_GDSCR);
		if (reg_gdsc & 0x80000000) {
			usleep_range(100, 200);
			loop--;
		} else {
			break;
		}
	}

	if (!loop) {
		dprintk(CVP_ERR, "fail to power off CORE during resume\n");
		return -EINVAL;
	}

	return rc;
}

int __set_registers_pakala(struct iris_hfi_device *device)
{
	struct msm_cvp_core *core;
	struct msm_cvp_platform_data *pdata;
	struct reg_set *reg_set;
	int i;

	if (!device->res) {
		dprintk(CVP_ERR,
			"device resources null, cannot set registers\n");
		return -EINVAL;
	}

	core = cvp_driver->cvp_core;
	pdata = core->platform_data;

	reg_set = &device->res->reg_set;
	for (i = 0; i < reg_set->count; i++) {
		__write_register(device, reg_set->reg_tbl[i].reg,
				 reg_set->reg_tbl[i].value);
		dprintk(CVP_REG, "write_reg offset=%x, val=%x\n",
			reg_set->reg_tbl[i].reg, reg_set->reg_tbl[i].value);
	}

	__write_register(device, CVP_NOC_RCGCONTROLLER_HYSTERESIS_LOW, 0xff);
	__write_register(device, CVP_NOC_RCGCONTROLLER_WAKEUP_LOW, 0x7);
	__write_register(device, CVP_NOC_RCG_VNOC_NOC_CLK_FORCECLOCKON_LOW,
			 0x1);
	__write_register(device, CVP_NOC_RCG_VNOC_NOC_CLK_ENABLE_LOW, 0x1);
	usleep_range(5, 10);
	__write_register(device, CVP_NOC_RCG_VNOC_NOC_CLK_FORCECLOCKON_LOW,
			 0x0);
	__write_register(device, CVP_AON_WRAPPER_CVP_NOC_ARCG_CONTROL, 0x0);

	__write_register(device, CVP_CPU_CS_AXI4_QOS, pdata->noc_qos->axi_qos);
	__write_register(device, CVP_NOC_A_PRIORITYLUT_LOW,
			 pdata->noc_qos->prioritylut_low);
	__write_register(device, CVP_NOC_A_PRIORITYLUT_HIGH,
			 pdata->noc_qos->prioritylut_high);
	__write_register(device, CVP_NOC_A_URGENCY_LOW,
			 pdata->noc_qos->urgency_low);
	__write_register(device, CVP_NOC_A_DANGERLUT_LOW,
			 pdata->noc_qos->dangerlut_low);
	__write_register(device, CVP_NOC_A_SAFELUT_LOW,
			 pdata->noc_qos->safelut_low);
	__write_register(device, CVP_NOC_B_PRIORITYLUT_LOW,
			 pdata->noc_qos->prioritylut_low);
	__write_register(device, CVP_NOC_B_PRIORITYLUT_HIGH,
			 pdata->noc_qos->prioritylut_high);
	__write_register(device, CVP_NOC_B_URGENCY_LOW,
			 pdata->noc_qos->urgency_low);
	__write_register(device, CVP_NOC_B_DANGERLUT_LOW,
			 pdata->noc_qos->dangerlut_low);
	__write_register(device, CVP_NOC_B_SAFELUT_LOW,
			 pdata->noc_qos->safelut_low);
	__write_register(device, CVP_NOC_C_PRIORITYLUT_LOW,
			 pdata->noc_qos->prioritylut_low);
	__write_register(device, CVP_NOC_C_PRIORITYLUT_HIGH,
			 pdata->noc_qos->prioritylut_high);
	__write_register(device, CVP_NOC_C_URGENCY_LOW,
			 pdata->noc_qos->urgency_low_ro);
	__write_register(device, CVP_NOC_C_DANGERLUT_LOW,
			 pdata->noc_qos->dangerlut_low);
	__write_register(device, CVP_NOC_C_SAFELUT_LOW,
			 pdata->noc_qos->safelut_low);

	/* Below registers write moved from FW to SW to enable UBWC */
	__write_register(device, CVP_NOC_A_NIU_DECCTL_LOW, 0x1);
	__write_register(device, CVP_NOC_A_NIU_ENCCTL_LOW, 0x1);
	__write_register(device, CVP_NOC_B_NIU_DECCTL_LOW, 0x1);
	__write_register(device, CVP_NOC_B_NIU_ENCCTL_LOW, 0x1);
	__write_register(device, CVP_NOC_CORE_ERR_MAINCTL_LOW_OFFS, 0x3);
	__write_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_FAULTINEN0_LOW,
			 0x1);

	return 0;
}

void __print_reg_details_errlog3_low_pakala(u32 val)
{
	u32 mid, sid;

	mid = (val >> 7) & 0x1F;

	sid = (val >> 2) & 0x7;
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG3_LOW:     %#x\n", val);
	dprintk(CVP_ERR, "Sub-client:%s, SID: %d\n", mid_names_pakala[mid],
		sid);
}

void __dump_noc_regs_pakala(struct iris_hfi_device *device)
{
#ifndef USE_PRESIL42
	u32 val = 0, config;
	struct regulator_info *rinfo;
	int rc = 0;

	if (msm_cvp_fw_low_power_mode) {
		if (device->res->gdsc_framework_type) {
			rc = switch_core_gdsc_mode(device, TO_SW_CTRL);
		} else {
			iris_hfi_for_each_regulator(device, rinfo) {
				if (strcmp(rinfo->name, "cvp-core"))
					continue;
				rc = __acquire_regulator(rinfo, device);
			}
		}
		if (rc)
			dprintk(
			    CVP_WARN,
			    "%s, Failed to acquire core gdsc control to SW\n",
			    __func__);
	}
	val = __read_register(device, CVP_CC_MVS0_GDSCR);
	dprintk(CVP_ERR, "%s, CVP_CC_MVS0_GDSCR: 0x%x", __func__, val);
	config = __read_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG);
	dprintk(CVP_ERR, "%s, CVP_WRAPPER_CORE_CLOCK_CONFIG: 0x%x", __func__,
		config);
	if (config) {
		dprintk(CVP_PWR, "core clock config not enabled\n");
		__write_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG, 0);
	}

	val = __read_register(device, CVP_NOC_A_NIU_DECCTL_LOW);
	dprintk(CVP_ERR, "CVP_NOC_A_NIU_DECCTL_LOW: 0x%x", val);
	val = __read_register(device, CVP_NOC_A_NIU_ENCCTL_LOW);
	dprintk(CVP_ERR, "CVP_NOC_A_NIU_ENCCTL_LOW: 0x%x", val);
	val = __read_register(device, CVP_NOC_B_NIU_DECCTL_LOW);
	dprintk(CVP_ERR, "CVP_NOC_B_NIU_DECCTL_LOW: 0x%x", val);
	val = __read_register(device, CVP_NOC_B_NIU_ENCCTL_LOW);
	dprintk(CVP_ERR, "CVP_NOC_B_NIU_ENCCTL_LOW: 0x%x", val);
	val = __read_register(device,
			      CVP_NOC_MAIN_SIDEBANDMANAGER_FAULTINEN0_LOW);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_FAULTINEN0_LOW: 0x%x",
		val);
	val =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_LOW);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_LOW: 0x%x",
		val);
	val =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_HIGH);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN0_HIGH: 0x%x",
		val);
	val =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN1_LOW);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN1_LOW: 0x%x",
		val);
	val =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN1_HIGH);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN1_HIGH: 0x%x",
		val);
	val =
	    __read_register(device, CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN2_LOW);
	dprintk(CVP_ERR, "CVP_NOC_MAIN_SIDEBANDMANAGER_SENSELN2_LOW: 0x%x",
		val);

	dprintk(CVP_ERR, "Dumping Core NoC registers\n");
	val = __read_register(device, CVP_NOC_CORE_ERR_SWID_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC__CORE_ERL_MAIN_SWID_LOW: 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_SWID_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_SWID_HIGH 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_MAINCTL_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_MAINCTL_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRVLD_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRVLD_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRCLR_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG0_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG0_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG0_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG0_HIGH 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG1_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG1_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG1_HIGH_OFFS);
	__print_reg_details_errlog1_high(val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG2_LOW_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG2_LOW 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG2_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG2_HIGH 0x%x", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG3_LOW_OFFS);
	dprintk(CVP_ERR, "CORE ERRLOG3_LOW 0x%x, below details", val);
	__print_reg_details_errlog3_low_pakala(val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG3_HIGH_OFFS);
	dprintk(CVP_ERR, "CVP_NOC_CORE_ERL_MAIN_ERRLOG3_HIGH 0x%x", val);
	__write_register(device, CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS, 0x1);

	if (msm_cvp_fw_low_power_mode) {
		if (device->res->gdsc_framework_type) {
			rc = switch_core_gdsc_mode(device, TO_HW_CTRL);
		} else {
			iris_hfi_for_each_regulator(device, rinfo) {
				if (strcmp(rinfo->name, "cvp-core"))
					continue;
				rc = __hand_off_regulator(rinfo);
			}
		}
		if (rc)
			dprintk(
			    CVP_WARN,
			    "%s, Failed to hand off core gdsc control to HW\n",
			    __func__);
	}
	__write_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG, config);
#endif
}

void __noc_error_info_iris2_pakala(struct iris_hfi_device *device)
{
	struct msm_cvp_core *core;
	struct cvp_noc_log *noc_log;
	u32 val = 0, regi, regiii;
	bool log_required = false;
	int rc;

	core = cvp_driver->cvp_core;

	if (core->resources.max_ssr_allowed >= 1)
		log_required = true;

	noc_log = &core->kmd_trace.kmd_debug_log.log.noc_log;

	if (noc_log->used) {
		dprintk(CVP_WARN, "Data already in NoC log, skip logging\n");
		return;
	}
	noc_log->used = 1;
	rc = 0;

	__disable_hw_power_collapse(device);

	val = call_iris_op(device, check_core_power_on, device);
	regi =
	    __read_register(device, CVP_AON_WRAPPER_CVP_NOC_CORE_CLK_CONTROL);
	regiii = __read_register(device, CVP_WRAPPER_CORE_CLOCK_CONFIG);
	dprintk(CVP_ERR, "noc reg check: %#x %#x %#x\n", val, regi, regiii);

	val = __read_register(device, CVP_NOC_ERR_SWID_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_swid_low,
		  "CVP_NOC_ERL_MAIN_SWID_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_SWID_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_swid_high,
		  "CVP_NOC_ERL_MAIN_SWID_HIGH", val);
	val = __read_register(device, CVP_NOC_ERR_MAINCTL_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_mainctl_low,
		  "CVP_NOC_ERL_MAIN_MAINCTL_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRVLD_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errvld_low,
		  "CVP_NOC_ERL_MAIN_ERRVLD_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRCLR_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errclr_low,
		  "CVP_NOC_ERL_MAIN_ERRCLR_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG0_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog0_low,
		  "CVP_NOC_ERL_MAIN_ERRLOG0_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG0_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog0_high,
		  "CVP_NOC_ERL_MAIN_ERRLOG0_HIGH", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG1_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog1_low,
		  "CVP_NOC_ERL_MAIN_ERRLOG1_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG1_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog1_high,
		  "CVP_NOC_ERL_MAIN_ERRLOG1_HIGH", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG2_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog2_low,
		  "CVP_NOC_ERL_MAIN_ERRLOG2_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG2_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog2_high,
		  "CVP_NOC_ERL_MAIN_ERRLOG2_HIGH", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG3_LOW_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog3_low,
		  "CVP_NOC_ERL_MAIN_ERRLOG3_LOW", val);
	val = __read_register(device, CVP_NOC_ERR_ERRLOG3_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_ctrl_errlog3_high,
		  "CVP_NOC_ERL_MAIN_ERRLOG3_HIGH", val);

#ifdef CONFIG_EVA_PINEAPPLE
	/* Lanai HW bug workaround */
	rc = call_iris_op(device, reset_control_acquire_name, device,
			  "cvp_xo_reset");
	if (rc) {
		dprintk(CVP_WARN, "%s Fail acquire xo_reset\n", __func__);
		return;
	}
#endif

	val = __read_register(device, CVP_NOC_CORE_ERR_SWID_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_swid_low,
		  "CVP_NOC__CORE_ERL_MAIN_SWID_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_SWID_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_core_swid_high,
		  "CVP_NOC_CORE_ERL_MAIN_SWID_HIGH", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_MAINCTL_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_mainctl_low,
		  "CVP_NOC_CORE_ERL_MAIN_MAINCTL_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRVLD_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errvld_low,
		  "CVP_NOC_CORE_ERL_MAIN_ERRVLD_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errclr_low,
		  "CVP_NOC_CORE_ERL_MAIN_ERRCLR_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG0_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog0_low,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG0_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG0_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog0_high,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG0_HIGH", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG1_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog1_low,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG1_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG1_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog1_high,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG1_HIGH", val);
	__print_reg_details_errlog1_high(val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG2_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog2_low,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG2_LOW", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG2_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog2_high,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG2_HIGH", val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG3_LOW_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog3_low,
		  "CORE ERRLOG3_LOW, below details", val);
	__print_reg_details_errlog3_low_pakala(val);
	val = __read_register(device, CVP_NOC_CORE_ERR_ERRLOG3_HIGH_OFFS);
	__err_log(log_required, &noc_log->err_core_errlog3_high,
		  "CVP_NOC_CORE_ERL_MAIN_ERRLOG3_HIGH", val);
	__write_register(device, CVP_NOC_CORE_ERR_ERRCLR_LOW_OFFS, 0x1);
#ifdef CONFIG_EVA_PINEAPPLE
	/* Lanai HW bug workaround */
	call_iris_op(device, reset_control_release_name, device,
		     "cvp_xo_reset");
#endif
#define CVP_SS_CLK_HALT 0x8
#define CVP_SS_CLK_EN 0xC
#define CVP_VPU_WRAPPER_CORE_CONFIG 0xB0088
	__write_register(device, CVP_SS_CLK_HALT, 0);
	__write_register(device, CVP_SS_CLK_EN, 0x3f);
	__write_register(device, CVP_VPU_WRAPPER_CORE_CONFIG, 0);
}

int set_pakala_hal_functions(void)
{
	hal_ops.interrupt_init = interrupt_init_iris2_pakala;
	hal_ops.setup_dsp_uc_memmap = setup_dsp_uc_memmap_vpu5_pakala;
	hal_ops.power_off_controller = __power_off_controller_pakala;
	hal_ops.power_off_core = __power_off_core_pakala;
	hal_ops.power_on_controller = __power_on_controller_pakala;
	hal_ops.power_on_core = __power_on_core_pakala;
	hal_ops.noc_error_info = __noc_error_info_iris2_pakala;
	hal_ops.check_ctl_power_on = __check_ctl_power_on_pakala;
	hal_ops.check_core_power_on = __check_core_power_on_pakala;
	hal_ops.print_sbm_regs = __print_sidebandmanager_regs_pakala;
	hal_ops.enable_hw_power_collapse = __enable_hw_power_collapse_pakala;
	hal_ops.set_registers = __set_registers_pakala;
	hal_ops.dump_noc_regs = __dump_noc_regs_pakala;
	hal_ops.check_tensilica_in_reset = __check_tensilica_in_reset_pakala;
	return 0;
}
