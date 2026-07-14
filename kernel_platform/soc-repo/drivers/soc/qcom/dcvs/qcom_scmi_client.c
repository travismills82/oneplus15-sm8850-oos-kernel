// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/scmi_protocol.h>
#include <linux/qcom_scmi_vendor.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

static const char * const qcom_scmi_plat_names[NUM_QCOM_SCMI_PLATFORMS] = {
	[QCOM_PLAT_CPUCP]	= "CPUCP",
	[QCOM_PLAT_PDP0]	= "PDP0",
	[QCOM_PLAT_PDP1]	= "PDP1",
	[QCOM_PLAT_PDP2]	= "PDP2",
};

struct qcom_scmi_dev_node {
	struct scmi_device	*scmi_dev;
	int			inited;
};

static struct qcom_scmi_dev_node qcom_scmi_devices[NUM_QCOM_SCMI_PLATFORMS];

struct scmi_device *platform_get_qcom_scmi_device(enum qcom_scmi_platforms plat)
{
	struct qcom_scmi_dev_node *node;

	if (plat > NUM_QCOM_SCMI_PLATFORMS)
		return ERR_PTR(-EINVAL);

	node = &qcom_scmi_devices[plat];
	if (!node || !node->inited)
		return ERR_PTR(-EPROBE_DEFER);

	if (!node->scmi_dev || !node->scmi_dev->handle)
		return ERR_PTR(-ENODEV);

	return node->scmi_dev;
}
EXPORT_SYMBOL_GPL(platform_get_qcom_scmi_device);

struct scmi_device *get_qcom_scmi_device(void)
{
	return platform_get_qcom_scmi_device(QCOM_PLAT_CPUCP);
}
EXPORT_SYMBOL_GPL(get_qcom_scmi_device);

static int scmi_client_probe(struct scmi_device *sdev)
{
	struct scmi_handle *handle;
	int i;

	if (!sdev)
		return -ENODEV;

	handle = sdev->handle;
	if (!handle || !handle->version) {
		dev_err(&sdev->dev, "Error: qcom vendor scmi handle is missing\n");
		return -ENODEV;
	}

	/* Use sub_vendor_id to identify platform */
	for (i = 0; i < NUM_QCOM_SCMI_PLATFORMS; i++) {
		if (!strcmp(qcom_scmi_plat_names[i],
				handle->version->sub_vendor_id)) {
			qcom_scmi_devices[i].scmi_dev = sdev;
			qcom_scmi_devices[i].inited = 1;
			break;
		}
	}
	/* Assume CPUCP if no platform found */
	if (i == NUM_QCOM_SCMI_PLATFORMS) {
		if (qcom_scmi_devices[QCOM_PLAT_CPUCP].scmi_dev) {
			dev_err(&sdev->dev, "Missing sub_vendor_id and CPUCP scmi_dev already set\n");
			return -EINVAL;
		}
		pr_warn("qcom vendor scmi sub_vendor_id missing: defaulting to CPUCP\n");
		qcom_scmi_devices[QCOM_PLAT_CPUCP].scmi_dev = sdev;
		qcom_scmi_devices[QCOM_PLAT_CPUCP].inited = 1;
	}

	return 0;
}

static const struct scmi_device_id scmi_id_table[] = {
	{ .protocol_id = QCOM_SCMI_VENDOR_PROTOCOL, .name = "qcom_scmi_vendor_protocol" },
	{ },
};
MODULE_DEVICE_TABLE(scmi, scmi_id_table);

static struct scmi_driver qcom_scmi_client_drv = {
	.name		= "qcom-scmi-driver",
	.probe		= scmi_client_probe,
	.id_table	= scmi_id_table,
};
module_scmi_driver(qcom_scmi_client_drv);

MODULE_SOFTDEP("pre: qcom_scmi_vendor");
MODULE_DESCRIPTION("QCOM SCMI client driver");
MODULE_LICENSE("GPL");
