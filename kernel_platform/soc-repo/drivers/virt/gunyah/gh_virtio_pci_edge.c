// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": %s:" fmt, __func__

#include <linux/device/bus.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>

static void gh_virtio_pci_edge_dev_note(struct pci_dev *pdev)
{
	int err;
	int irq = pdev->irq;

	if (irq) {
		err = irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
		if (err) {
			pr_err("failed to set IRQ %d to RISING edge err=%d, may miss wakeups\n",
				irq, err);
			return;
		}
		err = enable_irq_wake(irq);
		if (err) {
			pr_err("failed to set IRQ %d to wakeup capable err=%d\n",
				irq, err);
			return;
		}
		pr_info("IRQ %d made wakeup and set to RISING edge\n", irq);
	} else {
		pr_err("IRQ not present\n");
	}
}

static int gh_virtio_pci_edge_notifier_call(struct notifier_block *nb,
					    unsigned long event,
					    void *data)
{
	struct device *dev = data;
	struct pci_dev *pdev = to_pci_dev(dev);

	if (pdev->vendor != PCI_VENDOR_ID_REDHAT_QUMRANET ||
			event != BUS_NOTIFY_ADD_DEVICE)
		return NOTIFY_DONE;

	gh_virtio_pci_edge_dev_note(pdev);

	return NOTIFY_OK;
}

static struct notifier_block gh_virtio_pci_edge_notifier = {
	.notifier_call = gh_virtio_pci_edge_notifier_call,
};

static void gh_virtio_pci_edge_probe_devices(void)
{
	struct device *dev = NULL;
	struct pci_dev *pdev;

	while ((dev = bus_find_next_device(&pci_bus_type, dev))) {
		pdev = to_pci_dev(dev);
		if (pdev->vendor == PCI_VENDOR_ID_REDHAT_QUMRANET)
			gh_virtio_pci_edge_dev_note(pdev);
		put_device(dev);
	}
}

static int __init gh_virtio_pci_edge_init(void)
{
	int ret;

	gh_virtio_pci_edge_probe_devices();

	ret = bus_register_notifier(&pci_bus_type, &gh_virtio_pci_edge_notifier);
	if (ret)
		pr_err("PCI bus_register_notifier failed with %d, ignoring.\n", ret);

	return 0;
}

module_init(gh_virtio_pci_edge_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GH Virtio PCI Interrupt Edge Module");
