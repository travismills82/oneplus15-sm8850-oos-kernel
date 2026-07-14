// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include "hfi_core_probe.h"
#include "hfi_core.h"
#include "hfi_core_debug.h"

static int msm_hfi_core_probe_init(struct platform_device *pdev)
{
	int rc;
	struct hfi_core_drv_data *drv_data;
	struct client_data *client = NULL;

	HFI_CORE_DBG_H("+\n");

	drv_data = kzalloc(sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data) {
		HFI_CORE_ERR("%s: drv data allocation failed\n", __func__);
		return -ENOMEM;
	}

	client = &drv_data->client_data[HFI_CORE_CLIENT_ID_0];
	client->ipc_info.type = HFI_IPC_TYPE_MBOX;
	client->resource_info.internal_data = (struct hfi_core_internal_data *)
		(of_device_get_match_data(&pdev->dev));
	dev_set_drvdata(&pdev->dev, drv_data);
	drv_data->dev = (void *)(&pdev->dev);

	/* initialize hfi core driver resources */
	rc = hfi_core_init(drv_data);
	if (rc)
		goto error;

	HFI_CORE_DBG_H("%s: probe success\n", __func__);
	HFI_CORE_DBG_H("-\n");
	return rc;

error:
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(drv_data);
	drv_data = (void *) -EPROBE_DEFER;

	HFI_CORE_DBG_INFO("error %d\n", rc);

	return rc;
}

static int msm_hfi_core_probe(struct platform_device *pdev)
{
	int rc = 0;

	HFI_CORE_DBG_H("+\n");

	if (!pdev) {
		HFI_CORE_ERR("%s: null platform dev\n", __func__);
		return -EINVAL;
	}

	if (of_device_is_compatible(pdev->dev.of_node, "qcom,msm-hfi-core"))
		rc = msm_hfi_core_probe_init(pdev);
	if (rc)
		goto err_exit;

	HFI_CORE_DBG_H("-\n");
	return 0;

err_exit:
	if (rc == -EPROBE_DEFER)
		HFI_CORE_DBG_INFO("probe defer for hfi core\n");
	else
		HFI_CORE_ERR_ONCE("error %d\n", rc);

	return rc;
}

#if (KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE)
static int msm_hfi_core_remove(struct platform_device *pdev)
#else
static void msm_hfi_core_remove(struct platform_device *pdev)
#endif
{
	int rc = 0;
	struct hfi_core_drv_data *drv_data;

	HFI_CORE_DBG_H("+\n");

	if (!pdev) {
		HFI_CORE_ERR("null platform dev\n");
		rc = -EINVAL;
		goto exit;
	}

	drv_data = dev_get_drvdata(&pdev->dev);
	if (!drv_data) {
		HFI_CORE_ERR("null driver data\n");
		rc = -EINVAL;
		goto exit;
	}

	rc = hfi_core_deinit(drv_data);
	if (rc) {
		HFI_CORE_ERR("failed to deinit hfi core driver data\n");
		rc = -EINVAL;
		goto exit;
	}

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(drv_data);
	drv_data = (void *) -EPROBE_DEFER;

exit:
	HFI_CORE_DBG_H("-\n");
#if (KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE)
	return rc;
#endif
}

static const struct hfi_core_internal_data msmxxxx_data = {
	.host_id = HFI_HOST_PRIMARY_VM,
	.hfi_table_version = 0x00000001,
	.hfi_header_version = 0x00000001,
	.hfi_res_table = {
		/* Assigned DCP-Device ID*/
		.device_id = 0,
		.num_res = 1,
		.res_types = {
			HFI_QUEUE_VIRTIO_VIRTQ,
		},
		.virtqueues = {
			.num_queues = 4,
			/* High Priority Rx & Tx Queues */
			.queue[0] = {
				.type = HFI_VIRT_QUEUE_FULL_DUP,
				.priority = 1,
				.tx_elements = 32,
				.rx_elements = 32,
				.tx_buff_size_bytes = 4096,
				.rx_buff_size_bytes = 4096,
			},
			/* Medium Prio Rx & Tx Queues */
			.queue[1] = {
				.type = HFI_VIRT_QUEUE_FULL_DUP,
				.priority = 2,
				.tx_elements = 32,
				.rx_elements = 32,
				.tx_buff_size_bytes = 4096,
				.rx_buff_size_bytes = 4096,
			},
			/* Highest Priority Async Rx & Tx Queues */
			.queue[2] = {
				.type = HFI_VIRT_QUEUE_FULL_DUP,
				.priority = 0,
				.tx_elements = 32,
				.rx_elements = 32,
				.tx_buff_size_bytes = 4096,
				.rx_buff_size_bytes = 4096,
			},
			/* Events Rx & Tx Queues */
			.queue[3] = {
				.type = HFI_VIRT_QUEUE_FULL_DUP,
				.priority = 3,
				.tx_elements = 16,
				.rx_elements = 16,
				.tx_buff_size_bytes = 4096,
				.rx_buff_size_bytes = 4096,
			},
		},
	},
};

static const struct of_device_id msm_hfi_core_dt_match[] = {
	/* TODO:
	 * "qcom,msm-hfi-core" -> attached to msmxxxx_data is only
	 * debug change since device tree is using "qcom,msm-hfi-core"
	 * compatible
	 */
	{.compatible = "qcom,msm-hfi-core", .data = &msmxxxx_data },
	{.compatible = "qcom,msmxxxx-hfi-core", .data = &msmxxxx_data },
	{}
};

static struct platform_driver msm_hfi_core_driver = {
	.probe = msm_hfi_core_probe,
	.remove = msm_hfi_core_remove,
	.driver = {
		.name = "msm-hfi-core",
		.of_match_table = of_match_ptr(msm_hfi_core_dt_match),
	},
};

static int __init msm_hfi_core_init(void)
{
	int rc = 0;

	HFI_CORE_DBG_H("+\n");

	rc = platform_driver_register(&msm_hfi_core_driver);
	if (rc) {
		HFI_CORE_ERR("%s: failed to register platform driver\n",
			__func__);
		return rc;
	}

	HFI_CORE_DBG_H("-\n");

	return 0;
}

static void __exit msm_hfi_core_exit(void)
{
	HFI_CORE_DBG_H("+\n");

	platform_driver_unregister(&msm_hfi_core_driver);

	HFI_CORE_DBG_H("-\n");
}

module_init(msm_hfi_core_init);
module_exit(msm_hfi_core_exit);

MODULE_DESCRIPTION("QTI HFI Core Driver");
MODULE_LICENSE("GPL");
