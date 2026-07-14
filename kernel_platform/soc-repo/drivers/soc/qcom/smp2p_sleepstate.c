// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/pm_wakeup.h>
#include <asm/arch_timer.h>
#include <linux/soc/qcom/smem.h>

#define PROC_AWAKE_ID 12 /* 12th bit */
#define AWAKE_BIT BIT(PROC_AWAKE_ID)

static struct wakeup_source *notify_ws;

#define QCOM_POWER_STATE_SMEM_ID	512
#define QCOM_POWER_STATE_SHUTDOWN       0
#define QCOM_POWER_STATE_RESUME         1
#define QCOM_POWER_STATE_SUSPEND        2

/**
 * struct qcom_power_state - Structure to represent the power state of a subsystem
 * @version: Version of the power state structure
 * @subsystem: Identifier for the subsystem whose power state is being managed
 * @state: Current power state of the subsystem
 * @reserve: Reserved field
 * @resume_time: Timestamp indicating when the subsystem last resumed
 * @suspend_time: Timestamp indicating when the subsystem last entered into suspend
 */
struct qcom_power_state {
	__le32 version;
	__le32 subsystem;
	__le32 state;
	__le32 reserve;
	__le64 resume_time;
	__le64 suspend_time;
} __packed;

struct qcom_states {
	struct qcom_power_state *power_state;
	struct qcom_smem_state *smem_state;
};

static struct qcom_states state;

/**
 * sleepstate_pm_notifier() - PM notifier callback function.
 * @nb:		Pointer to the notifier block.
 * @event:	Suspend state event from PM module.
 * @unused:	Null pointer from PM module.
 *
 * This function is register as callback function to get notifications
 * from the PM module on the system suspend state.
 */
static int sleepstate_pm_notifier(struct notifier_block *nb,
				  unsigned long event, void *unused)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		if (state.power_state) {
			state.power_state->state = QCOM_POWER_STATE_SUSPEND;
			state.power_state->suspend_time = __arch_counter_get_cntvct();

			/* Update the power states and add a memory barrier to ensure
			 * consistent read/write between APPS and the remote.
			 */
			mb();
		}
		qcom_smem_state_update_bits(state.smem_state, AWAKE_BIT, 0);
		break;

	case PM_POST_SUSPEND:
		if (state.power_state) {
			state.power_state->state = QCOM_POWER_STATE_RESUME;
			state.power_state->resume_time = __arch_counter_get_cntvct();
			/* Update the power states and add a memory barrier to ensure
			 * consistent read/write between APPS and the remote.
			 */
			mb();
		}
		qcom_smem_state_update_bits(state.smem_state, AWAKE_BIT, AWAKE_BIT);
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block sleepstate_pm_nb = {
	.notifier_call = sleepstate_pm_notifier,
	.priority = INT_MAX,
};

static irqreturn_t smp2p_sleepstate_handler(int irq, void *ctxt)
{
	__pm_wakeup_event(notify_ws, 200);
	return IRQ_HANDLED;
}

static int smp2p_sleepstate_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	state.smem_state = qcom_smem_state_get(&pdev->dev, 0, &ret);
	if (IS_ERR(state.smem_state))
		return PTR_ERR(state.smem_state);
	qcom_smem_state_update_bits(state.smem_state, AWAKE_BIT, AWAKE_BIT);

	ret = qcom_smem_alloc(QCOM_SMEM_HOST_ANY,
			      QCOM_POWER_STATE_SMEM_ID,
			      sizeof(struct qcom_power_state));

	if (ret < 0 && ret != -EEXIST) {
		pr_err("Unable to allocate memory for power state notif err %d\n", ret);
	} else {
		state.power_state = qcom_smem_get(QCOM_SMEM_HOST_ANY,
						  QCOM_POWER_STATE_SMEM_ID,
						  NULL);
		if (IS_ERR(state.power_state)) {
			state.power_state = NULL;
			pr_err("Unable to acquire shared memory power state notif\n");
		} else {
			state.power_state->version = 1;
			state.power_state->subsystem = 0;
			state.power_state->state = QCOM_POWER_STATE_RESUME;
			state.power_state->resume_time = __arch_counter_get_cntvct();
			/* Update the power states and add a memory barrier to ensure
			 * consistent read/write between APPS and the remote.
			 */
			mb();
		}
		pr_info("%s: Allocated shared memory for power state notif\n", __func__);
	}

	ret = register_pm_notifier(&sleepstate_pm_nb);
	if (ret) {
		dev_err(dev, "%s: power state notif error %d\n", __func__, ret);
		return ret;
	}

	notify_ws = wakeup_source_register(&pdev->dev, "smp2p-sleepstate");
	if (!notify_ws) {
		ret = -ENOMEM;
		goto err_ws;
	}

	irq = of_irq_get_byname(node, "smp2p-sleepstate-in");
	if (irq <= 0) {
		dev_err(dev, "failed to get irq for smp2p_sleep_state\n");
		ret = -EPROBE_DEFER;
		goto err;
	}
	dev_dbg(dev, "got smp2p-sleepstate-in irq %d\n", irq);
	ret = devm_request_threaded_irq(dev, irq, NULL,
					smp2p_sleepstate_handler,
					IRQF_ONESHOT | IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
					"smp2p_sleepstate", dev);
	if (ret) {
		dev_err(dev, "fail to register smp2p threaded_irq=%d\n", irq);
		goto err;
	}
	return 0;
err:
	wakeup_source_unregister(notify_ws);
err_ws:
	unregister_pm_notifier(&sleepstate_pm_nb);
	return ret;
}

static const struct of_device_id smp2p_slst_match_table[] = {
	{.compatible = "qcom,smp2p-sleepstate"},
	{},
};

static struct platform_driver smp2p_sleepstate_driver = {
	.probe = smp2p_sleepstate_probe,
	.driver = {
		.name = "smp2p_sleepstate",
		.of_match_table = smp2p_slst_match_table,
	},
};

static int __init smp2p_sleepstate_init(void)
{
	int ret;

	ret = platform_driver_register(&smp2p_sleepstate_driver);
	if (ret) {
		pr_err("%s: register failed %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

module_init(smp2p_sleepstate_init);
MODULE_DESCRIPTION("SMP2P SLEEP STATE");
MODULE_LICENSE("GPL");
