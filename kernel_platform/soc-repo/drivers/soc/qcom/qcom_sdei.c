// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "qcom_sdei: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/arm_sdei.h>
#include <linux/sched/debug.h>
#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <uapi/linux/psci.h>

#define SDEI_EVENT_STANDARD		0x0
#define SDEI_EVENT_WDG_BITE		0x40000000

#define SDEI_1_0_FN_SDEI_EVENT_SIGNAL	0xC400002F

static DEFINE_PER_CPU(u64, cpu_mpidr);

static int sdei_standard_cb(u32 event, struct pt_regs *regs, void *arg)
{
	int cpu = smp_processor_id();

	pr_crit("SDEI event 0x%x triggered on cpu %d\n", event, cpu);

	show_regs(regs);

	return 0;
}

static int psci_system_reset(void)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_1_1_hvc(PSCI_0_2_FN_SYSTEM_RESET, &res);

	return res.a0;
}

static int sdei_shared_reset(void)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_1_1_hvc(SDEI_1_0_FN_SDEI_SHARED_RESET, &res);

	return res.a0;
}

static int sdei_event_signal(u64 target_pe)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_1_1_hvc(SDEI_1_0_FN_SDEI_EVENT_SIGNAL, 0, target_pe, &res);

	return res.a0;
}

int qcom_sdei_shared_reset(void)
{
	sdei_shared_reset();

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_sdei_shared_reset);

static int sdei_wdg_bite_cb(u32 event, struct pt_regs *regs, void *arg)
{
	int cpu;

	pr_crit("SDEI event 0x%x triggered on cpu %d\n", event, smp_processor_id());

	show_regs(regs);

	for_each_possible_cpu(cpu) {
		if (cpu == smp_processor_id())
			continue;
		sdei_event_signal(per_cpu(cpu_mpidr, cpu));
		mdelay(1);
	}

	psci_system_reset();

	return 0;
}

static void register_cpu_mpidr(void *info)
{
	per_cpu(cpu_mpidr, smp_processor_id()) = read_cpuid_mpidr();
}

static int qcom_sdei_driver_probe(struct platform_device *pdev)
{
	int ret;

	on_each_cpu(register_cpu_mpidr, NULL, 1);

	ret = sdei_event_register(SDEI_EVENT_STANDARD, sdei_standard_cb, NULL);
	if (ret) {
		pr_err("Failed to register SDEI event 0x%x\n", SDEI_EVENT_STANDARD);
		return ret;
	}

	ret = sdei_event_enable(SDEI_EVENT_STANDARD);
	if (ret) {
		sdei_event_unregister(SDEI_EVENT_STANDARD);
		pr_err("Failed to enable SDEI event 0x%x\n", SDEI_EVENT_STANDARD);
		return ret;
	}

	ret = sdei_event_register(SDEI_EVENT_WDG_BITE, sdei_wdg_bite_cb, NULL);
	if (ret) {
		sdei_event_disable(SDEI_EVENT_STANDARD);
		sdei_event_unregister(SDEI_EVENT_STANDARD);
		pr_err("Failed to register SDEI event 0x%x\n", SDEI_EVENT_WDG_BITE);
		return ret;
	}

	ret |= sdei_event_enable(SDEI_EVENT_WDG_BITE);
	if (ret) {
		sdei_event_unregister(SDEI_EVENT_WDG_BITE);
		sdei_event_disable(SDEI_EVENT_STANDARD);
		sdei_event_unregister(SDEI_EVENT_STANDARD);
		pr_err("Failed to enable SDEI event 0x%x\n", SDEI_EVENT_WDG_BITE);
		return ret;
	}

	return 0;
}

static void qcom_sdei_driver_remove(struct platform_device *pdev)
{
	sdei_event_disable(SDEI_EVENT_STANDARD);
	sdei_event_unregister(SDEI_EVENT_STANDARD);

	sdei_event_disable(SDEI_EVENT_WDG_BITE);
	sdei_event_unregister(SDEI_EVENT_WDG_BITE);
}

static const struct of_device_id qcom_sdei_of_match[] = {
	{ .compatible = "qcom,sdei" },
	{ }
};
MODULE_DEVICE_TABLE(of, qcom_sdei_of_match);

static struct platform_driver qcom_sdei_driver = {
	.driver = {
		.name = "qcom_sdei",
		.of_match_table = qcom_sdei_of_match,
	},
	.probe = qcom_sdei_driver_probe,
	.remove = qcom_sdei_driver_remove,
};
module_platform_driver(qcom_sdei_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Qualcomm Technologies, Inc. SDEI driver");
