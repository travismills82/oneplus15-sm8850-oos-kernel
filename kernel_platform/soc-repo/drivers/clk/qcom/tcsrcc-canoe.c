// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/clk-provider.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include <dt-bindings/clock/qcom,tcsrcc-canoe.h>

#include "clk-alpha-pll.h"
#include "clk-branch.h"
#include "clk-pll.h"
#include "clk-rcg.h"
#include "clk-regmap.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"
#include "common.h"
#include "reset.h"
#include "vdd-level.h"

static struct clk_branch tcsr_pcie_0_clkref_en = {
	.halt_reg = 0x0,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "tcsr_pcie_0_clkref_en",
			.flags = CLK_DONT_HOLD_STATE,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch tcsr_ufs_clkref_en = {
	.halt_reg = 0x10,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x10,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "tcsr_ufs_clkref_en",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch tcsr_usb2_clkref_en = {
	.halt_reg = 0x18,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x18,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "tcsr_usb2_clkref_en",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch tcsr_usb3_clkref_en = {
	.halt_reg = 0x8,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "tcsr_usb3_clkref_en",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap *tcsr_cc_canoe_clocks[] = {
	[TCSR_PCIE_0_CLKREF_EN] = &tcsr_pcie_0_clkref_en.clkr,
	[TCSR_UFS_CLKREF_EN] = &tcsr_ufs_clkref_en.clkr,
	[TCSR_USB2_CLKREF_EN] = &tcsr_usb2_clkref_en.clkr,
	[TCSR_USB3_CLKREF_EN] = &tcsr_usb3_clkref_en.clkr,
};

static const struct regmap_config tcsr_cc_canoe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x18,
	.fast_io = true,
};

static const struct qcom_cc_desc tcsr_cc_canoe_desc = {
	.config = &tcsr_cc_canoe_regmap_config,
	.clks = tcsr_cc_canoe_clocks,
	.num_clks = ARRAY_SIZE(tcsr_cc_canoe_clocks),
};

static const struct of_device_id tcsr_cc_canoe_match_table[] = {
	{ .compatible = "qcom,canoe-tcsrcc" },
	{ }
};
MODULE_DEVICE_TABLE(of, tcsr_cc_canoe_match_table);

static int tcsr_cc_canoe_probe(struct platform_device *pdev)
{
	struct regmap *regmap;
	int ret;

	regmap = qcom_cc_map(pdev, &tcsr_cc_canoe_desc);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = qcom_cc_really_probe(&pdev->dev, &tcsr_cc_canoe_desc, regmap);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "Failed to register TCSR CC clocks\n");

	dev_info(&pdev->dev, "Registered TCSR CC clocks\n");

	return ret;
}

static void tcsr_cc_canoe_sync_state(struct device *dev)
{
	qcom_cc_sync_state(dev, &tcsr_cc_canoe_desc);
}

static struct platform_driver tcsr_cc_canoe_driver = {
	.probe = tcsr_cc_canoe_probe,
	.driver = {
		.name = "tcsrcc-canoe",
		.of_match_table = tcsr_cc_canoe_match_table,
		.sync_state = tcsr_cc_canoe_sync_state,
	},
};

module_platform_driver(tcsr_cc_canoe_driver);

MODULE_DESCRIPTION("QTI TCSRCC CANOE Driver");
MODULE_LICENSE("GPL");
