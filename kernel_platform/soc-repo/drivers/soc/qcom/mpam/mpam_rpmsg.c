// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "mpam_rpmsg: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/rpmsg.h>
#include <soc/qcom/mpam.h>

#include "mpam_dsp.h"

#define MPAM_RPMSG_TIMEOUT		msecs_to_jiffies(100)

static struct rpmsg_device *rpmsgdev;
static struct completion rpmsg_complete;
static struct platform_mpam_bw_limit_cfg cdsp_limit_cfg;
static struct dsp_status_t cdsp_status;
static int cdsp_version;

struct platform_mpam_bw_limit_cfg *qcom_mpam_get_bw_limit(void)
{
	return &cdsp_limit_cfg;
}
EXPORT_SYMBOL_GPL(qcom_mpam_get_bw_limit);

static inline void set_cdsp_limit_cfg(void)
{
	cdsp_limit_cfg.limit_ratio = cdsp_status.ratio;
	cdsp_limit_cfg.limit_mbps = cdsp_status.bwlimit;
	cdsp_limit_cfg.max_bw = cdsp_status.bwmax;
}

int qcom_mpam_get_bw_limit_rpmsg(void)
{
	int result = -ENODEV;
	struct mpam2dsp_msg_t rpmsg;

	if (!rpmsgdev || !cdsp_version)
		return result;

	rpmsg.feature_id = MPAM_DSP_FEATURE_GET_BW_LIMIT;
	rpmsg.version = MPAM_DSP_GLINK_VERSION;
	rpmsg.size = sizeof(rpmsg);

	result = rpmsg_send(rpmsgdev->ept, &rpmsg, sizeof(rpmsg));

	if (result) {
		pr_err("Get cdsp bw limit failed %d\n", result);
		return result;
	}

	if (!wait_for_completion_timeout(&rpmsg_complete, MPAM_RPMSG_TIMEOUT)) {
		pr_err("Get cdsp bw limit timeout\n");
		cdsp_limit_cfg.limit_ratio = -1;
		cdsp_limit_cfg.limit_mbps = -1;
		return -EIO;
	}

	if (cdsp_status.status) {
		pr_err("Get cdsp bw limit failed ret %d\n", cdsp_status.status);
		return -EIO;
	}

	set_cdsp_limit_cfg();

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_mpam_get_bw_limit_rpmsg);


int qcom_mpam_set_bw_limit_rpmsg(int ratio)
{
	int result = -ENODEV;
	struct mpam2dsp_msg_t rpmsg;

	if (!rpmsgdev || !cdsp_version)
		return result;

	rpmsg.feature_id = MPAM_DSP_FEATURE_SET_BW_LIMIT;
	rpmsg.version = MPAM_DSP_GLINK_VERSION;
	rpmsg.size = sizeof(rpmsg);
	rpmsg.fs.limit_params.set_ratio = 1;
	rpmsg.fs.limit_params.ratio = ratio;

	result = rpmsg_send(rpmsgdev->ept, &rpmsg, sizeof(rpmsg));

	if (result) {
		pr_err("Set cdsp bw limit failed %d\n", result);
		return result;
	}

	if (!wait_for_completion_timeout(&rpmsg_complete, MPAM_RPMSG_TIMEOUT)) {
		pr_err("Set cdsp bw limit timeout\n");
		cdsp_limit_cfg.limit_ratio = -1;
		cdsp_limit_cfg.limit_mbps = -1;
		return -EIO;
	}

	if (cdsp_status.status) {
		pr_err("Set cdsp bw limit failed ret %d\n", cdsp_status.status);
		return -EIO;
	}

	set_cdsp_limit_cfg();

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_mpam_set_bw_limit_rpmsg);

static int qcom_mpam_rpmsg_callback(struct rpmsg_device *dev, void *data,
		int len, void *priv, u32 addr)
{
	struct dsp2mpam_msg_t *msg = (struct dsp2mpam_msg_t *)data;

	if (!data || (len < sizeof(*msg))) {
		dev_err(&dev->dev,
			"Invalid message in rpmsg callback, length: %d, expected: %lu\n",
			len, sizeof(*msg));
		return -EINVAL;
	}

	if (msg->feature_id == MPAM_DSP_FEATURE_STATUS) {
		memcpy(&cdsp_status, &msg->fs.dsp_state, sizeof(struct dsp_status_t));

		if (cdsp_version)
			complete(&rpmsg_complete);
		else {
			cdsp_version = msg->version;
			set_cdsp_limit_cfg();
		}
	}

	return 0;
}

static int qcom_mpam_rpmsg_probe(struct rpmsg_device *dev)
{
	rpmsgdev = dev;

	return 0;
}

static void qcom_mpam_rpmsg_remove(struct rpmsg_device *dev)
{
	rpmsgdev = NULL;
	cdsp_version = 0;
	cdsp_limit_cfg.limit_ratio = -1;
	cdsp_limit_cfg.limit_mbps = -1;
}

static const struct rpmsg_device_id qcom_mpam_rpmsg_match[] = {
	{ MPAM_DSP_GLINK_GUID },
	{ },
};

static struct rpmsg_driver qcom_mpam_rpmsg_driver = {
	.id_table = qcom_mpam_rpmsg_match,
	.probe = qcom_mpam_rpmsg_probe,
	.remove = qcom_mpam_rpmsg_remove,
	.callback = qcom_mpam_rpmsg_callback,
	.drv = {
		.name = "qcom_mpam_rpmsg",
	},
};

int __init qcom_mpam_rpmsg_init(void)
{
	init_completion(&rpmsg_complete);

	return register_rpmsg_driver(&qcom_mpam_rpmsg_driver);
}

void __exit qcom_mpam_rpmsg_exit(void)
{
	complete_all(&rpmsg_complete);

	unregister_rpmsg_driver(&qcom_mpam_rpmsg_driver);
}
