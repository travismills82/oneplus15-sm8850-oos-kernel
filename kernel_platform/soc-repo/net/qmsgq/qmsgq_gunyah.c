// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/gunyah/gh_rm_drv.h>
#include <linux/gunyah/gh_msgq.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/sizes.h>
#include <linux/types.h>
#include <linux/of.h>

#include "af_qmsgq.h"
#include "qmsgq_gunyah.h"
#include "qmsgq_transport.h"

#define QMSGQ_SKB_WAKEUP_MS 500

static int qmsgq_gh_send(struct qmsgq_gh_device *qdev, void *buf, size_t len)
{
	size_t left, chunk, offset;
	int rc = 0;

	left = len;
	chunk = 0;
	offset = 0;

	mutex_lock(&qdev->tx_lock);
	while (left > 0) {
		chunk = (left > GH_MSGQ_MAX_MSG_SIZE_BYTES) ? GH_MSGQ_MAX_MSG_SIZE_BYTES : left;
		rc = gh_msgq_send(qdev->msgq_hdl, buf + offset, chunk, GH_MSGQ_TX_PUSH);
		if (rc) {
			if (rc == -ERESTARTSYS) {
				continue;
			} else {
				pr_err("%s: gh_msgq_send failed: %d\n", __func__, rc);
				mutex_unlock(&qdev->tx_lock);
				goto err;
			}
		}
		left -= chunk;
		offset += chunk;
	}
	mutex_unlock(&qdev->tx_lock);
	return 0;

err:
	return rc;
}

static int qmsgq_gh_msgq_recv(void *data)
{
	struct qmsgq_gh_device *qdev = data;
	size_t size;
	void *buf;
	int rc;

	buf = kzalloc(GH_MSGQ_MAX_MSG_SIZE_BYTES, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	while (!kthread_should_stop()) {
		rc = gh_msgq_recv(qdev->msgq_hdl, buf, GH_MSGQ_MAX_MSG_SIZE_BYTES, &size,
				  GH_MSGQ_TX_PUSH);
		if (rc)
			continue;

		if (size <= 0)
			continue;

		qmsgq_transport_process_recv(qdev, buf, size);
		pm_wakeup_ws_event(qdev->sock_ws, QMSGQ_SKB_WAKEUP_MS, true);
	}
	kfree(buf);

	return 0;
}

static int qmsgq_gh_msgq_start(struct qmsgq_gh_device *qdev)
{
	struct device *dev = qdev->dev;
	int rc;

	if (qdev->msgq_hdl) {
		dev_err(qdev->dev, "Already have msgq handle!\n");
		return NOTIFY_DONE;
	}

	qdev->msgq_hdl = gh_msgq_register(qdev->msgq_label);
	if (IS_ERR_OR_NULL(qdev->msgq_hdl)) {
		rc = PTR_ERR(qdev->msgq_hdl);
		dev_err(dev, "msgq register failed rc:%d\n", rc);
		return rc;
	}

	qdev->rx_thread = kthread_run(qmsgq_gh_msgq_recv, qdev, "qmsgq_gh_rx");
	if (IS_ERR_OR_NULL(qdev->rx_thread)) {
		rc = PTR_ERR(qdev->rx_thread);
		dev_err(dev, "Failed to create rx thread rc:%d\n", rc);
		return rc;
	}

	return 0;
}

static int qmsgq_gh_rm_cb(struct notifier_block *nb, unsigned long cmd, void *data)
{
	struct qmsgq_gh_device *qdev = container_of(nb, struct qmsgq_gh_device, rm_nb);
	struct gh_rm_notif_vm_status_payload *vm_status_payload = data;
	u8 vm_status = vm_status_payload->vm_status;
	int rc;

	if (cmd != GH_RM_NOTIF_VM_STATUS)
		return NOTIFY_DONE;

	/* TODO - check for peer */
	switch (vm_status) {
	case GH_RM_VM_STATUS_READY:
		qdev->peer_cid = (unsigned int)vm_status_payload->vmid;
		rc = qmsgq_gh_msgq_start(qdev);
		if (rc)
			dev_err(qdev->dev, "msgq start failed rc[%d]\n", rc);
		break;
	default:
		pr_debug("Unknown notification for vmid = %d vm_status = %d\n",
			 vm_status_payload->vmid, vm_status);
	}

	return NOTIFY_DONE;
}

static int qmsgq_gh_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct qmsgq_gh_device *qdev;
	enum gh_vm_names peer_vmname;
	gh_vmid_t peer_vmid = 0;
	int rc;

	qdev = devm_kzalloc(dev, sizeof(*qdev), GFP_KERNEL);
	if (!qdev)
		return -ENOMEM;
	qdev->dev = dev;
	dev_set_drvdata(&pdev->dev, qdev);

	mutex_init(&qdev->tx_lock);
	mutex_init(&qdev->rx_buf.lock);
	qdev->rx_buf.len = 0;
	qdev->rx_buf.copied = 0;
	qdev->rx_buf.hdr_received = false;

	qdev->send_pkt = qmsgq_gh_send;

	qdev->sock_ws = wakeup_source_register(NULL, "qmsgq_sock_ws");

	rc = of_property_read_u32(np, "msgq-label", &qdev->msgq_label);
	if (rc) {
		dev_err(dev, "failed to read msgq-label info %d\n", rc);
		return rc;
	}

	qdev->rm_nb.notifier_call = qmsgq_gh_rm_cb;
	gh_rm_register_notifier(&qdev->rm_nb);

	rc = of_property_read_u32(np, "peer", &peer_vmname);
	if (rc) {
		dev_err(dev, "failed to read peer vm info %d\n", rc);
		goto out;
	}

	rc = ghd_rm_get_vmid(peer_vmname, &peer_vmid);
	if (rc) {
		dev_err(dev, "failed to get peer vmid %d\n", rc);
		goto out;
	}

	if (peer_vmid != GH_VMID_INVAL) {
		rc = qmsgq_gh_msgq_start(qdev);
		if (rc) {
			dev_err(qdev->dev, "msgq start failed rc[%d]\n", rc);
			goto out;
		}
	}

	rc = qmsgq_transport_init(qdev);
	if (rc) {
		dev_err(qdev->dev, "qmsgq transport init failed rc[%d]\n", rc);
		goto out;
	}

	qmsgq_endpoint_register(&qdev->ep);
	return 0;
out:
	gh_rm_unregister_notifier(&qdev->rm_nb);
	return rc;
}

static void qmsgq_gh_remove(struct platform_device *pdev)
{
	struct qmsgq_gh_device *qdev = dev_get_drvdata(&pdev->dev);

	gh_rm_unregister_notifier(&qdev->rm_nb);

	if (qdev->rx_thread)
		kthread_stop(qdev->rx_thread);

	qmsgq_endpoint_unregister(&qdev->ep);
}

static const struct of_device_id qmsgq_gh_of_match[] = {
	{ .compatible = "qcom,qmsgq-gh" },
	{}
};
MODULE_DEVICE_TABLE(of, qmsgq_gh_of_match);

static struct platform_driver qmsgq_gh_driver = {
	.probe	= qmsgq_gh_probe,
	.remove	= qmsgq_gh_remove,
	.driver = {
		.name	= "qmsgq-gh",
		.of_match_table = qmsgq_gh_of_match,
	}
};
module_platform_driver(qmsgq_gh_driver);

MODULE_ALIAS("gunyah:QMSGQ");
MODULE_DESCRIPTION("Gunyah QMSGQ Transport driver");
MODULE_LICENSE("GPL");
