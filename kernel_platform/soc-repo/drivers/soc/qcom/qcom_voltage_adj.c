// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <soc/qcom/qcom_voltage_adj.h>

int qvadj_get_cur_voltage_overrides(u8 *global_rails_index, u8 *cpu_rails_index)
{
	int ret;
	u32 current_index = 0;

	if (!global_rails_index || !cpu_rails_index)
		return -EINVAL;

	mutex_lock(&pd->lock);
	ret = regmap_read(pd->regmap, SDAM2_MEM17, &current_index);
	if (ret < 0)
		pr_err("get current voltage overrides failed: %d\n", ret);
	mutex_unlock(&pd->lock);

	if (!(current_index & GLOBAL_VALID_MASK)) {
		*global_rails_index = 0;
		pr_err("get global rails overrides failed\n");
	} else {
		*global_rails_index = (u8)(current_index & GLOBAL_MASK);
	}

	if (!(current_index & CPU_VALID_MASK)) {
		*cpu_rails_index = 0;
		pr_err("get cpu rails overrides failed\n");
	} else {
		*cpu_rails_index = (u8)((current_index & CPU_MASK) >> CPU_SHFT);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(qvadj_get_cur_voltage_overrides);

int qvadj_set_next_voltage_overrides(u8 global_rails_index, u8 cpu_rails_index)
{
	int ret;
	u32 current_index = 0;

	if ((global_rails_index > MAX_IDX) || (cpu_rails_index > MAX_IDX)) {
		pr_err("set next voltage overrides failed. Invalid argument\n");
		return -EINVAL;
	}

	current_index = global_rails_index | (cpu_rails_index << CPU_SHFT);
	current_index |= VALID_MASK;

	mutex_lock(&pd->lock);
	ret = regmap_write(pd->regmap, SDAM2_MEM17, current_index);
	if (ret < 0)
		pr_err("set next voltage overrides failed: %d\n", ret);
	mutex_unlock(&pd->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(qvadj_set_next_voltage_overrides);

static int qvadj_get_overrides_show(struct seq_file *s, void *unused)
{
	u8 global_rails_index = 0;
	u8 cpu_rails_index = 0;

	qvadj_get_cur_voltage_overrides(&global_rails_index, &cpu_rails_index);

	seq_printf(s, "CPUSS rails index: %u\n", cpu_rails_index);
	seq_printf(s, "Global rails index: %u\n", global_rails_index);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(qvadj_get_overrides);

static int qvadj_set_overrides(void *data, u64 val)
{

	u8 global_rails_index = (u8)val & GLOBAL_MASK;
	u8 cpu_rails_index = ((u8)val & CPU_MASK) >> CPU_SHFT;

	qvadj_set_next_voltage_overrides(global_rails_index, cpu_rails_index);

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(qvadj_set_overrides_fops, NULL, qvadj_set_overrides, "%llu\n");

static int qvadj_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct dentry *root;

	pd = devm_kzalloc(dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	regmap = dev_get_regmap(dev->parent, NULL);
	if (!regmap)
		return -ENODEV;

	root = debugfs_create_dir("qcom_voltage_adj", NULL);

	debugfs_create_file("get_overrides", 0400, root, NULL, &qvadj_get_overrides_fops);
	debugfs_create_file("set_overrides", 0600, root, NULL, &qvadj_set_overrides_fops);

	pd->regmap = regmap;
	pd->root = root;
	mutex_init(&pd->lock);

	platform_set_drvdata(pdev, pd);

	return 0;
}

static void qvadj_remove(struct platform_device *pdev)
{
	struct qvadj_platform_data *pd = platform_get_drvdata(pdev);

	debugfs_remove_recursive(pd->root);
}

static const struct of_device_id qvadj_table[] = {
	{ .compatible = "qcom,vadj" },
	{ }
};

static struct platform_driver qvadj_driver = {
	.probe = qvadj_probe,
	.remove = qvadj_remove,
	.driver = {
		.name = "qcom_voltage_adj",
		.of_match_table = qvadj_table,
	},
};
module_platform_driver(qvadj_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Qualcomm Technologies, Inc. (QTI) QVADJ driver");
