// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt)	"pdt_ds: %s: " fmt, __func__

#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/qcom/smem.h>

#include "qti_ptd_share.h"

#define PDT_HW "pdt-hw"

static uint8_t num_nodes;
static char *smem_buffer;

int qptm_share_node_info_cb(struct qptm *qptm, void *data)
{
	int ret;
	struct pdt_priv *pdt = (struct pdt_priv *)data;
	uint16_t node_id;
	const char *node_name;
	int i;

	if (qptm == NULL || data == NULL)
		return -EINVAL;

	ret = qptm_get_node_id(qptm);
	if (ret < 0) {
		PDT_DBG_EVENT(pdt,
			"unable to read node_id: %d", ret);
		return ret;
	}
	node_id = (uint16_t)ret;

	node_name = qptm_get_node_name(qptm);

	*(uint16_t *)smem_buffer = node_id;
	PDT_DBG_EVENT(pdt, "node_id=0x%x", node_id);
	smem_buffer += sizeof(uint16_t);
	if (node_name != NULL) {
		PDT_DBG_EVENT(pdt, "node_name=%s", node_name);
		for (i = 0; i < strlen(node_name); ++i) {
			*(uint8_t *)smem_buffer = (uint8_t)node_name[i];
			smem_buffer += sizeof(uint8_t);
		}
	}
	*(uint8_t *)smem_buffer = (uint8_t)0;
	smem_buffer += sizeof(uint8_t);

	return 0;
}

int qti_pdt_init_data_sharing(struct pdt_priv *pdt)
{
	int ret;
	uint64_t xo_tick;

	if (pdt == NULL)
		return -EINVAL;

	if (!qcom_smem_is_available())
		return -ENODEV;

	ret = qcom_smem_alloc(PDT_DS_SMEM_PROC_ID, PDT_DS_SMEM_ITEM_HANDLE, PDT_DS_SMEM_BLOCK_SIZE);
	if (ret != 0)
		return ret;

	smem_buffer = qcom_smem_get(PDT_DS_SMEM_PROC_ID, PDT_DS_SMEM_ITEM_HANDLE, NULL);

	if (IS_ERR(smem_buffer))
		return (int)PTR_ERR(smem_buffer);

	PDT_DBG_EVENT(pdt, "smem_buffer legend address=0x%lx", (long)smem_buffer);
	PDT_DBG_EVENT(pdt, "smem_buffer data   address=0x%lx",
			(long)(smem_buffer + PDT_DS_SMEM_DATA_OFFSET));

	ret = qptm_available_node_count();
	if (ret < 0) {
		PDT_DBG_EVENT(pdt, "unable to read number of voltage nodes: %d",
			ret);
		return ret;
	}
	num_nodes = (uint8_t)ret;

	PDT_DBG_EVENT(pdt, "number of enabled voltage nodes=%d", num_nodes);

	/*
	 * Write out the legend to the shared memory
	 *
	 * Legend format:
	 *
	 * <version><xo_tick><#nodes><node_id><node_name><0>...<xo_tick>
	 * <uint8>  <uint64> <uint8> <uint16> <string>   <0>...<uint64>
	 **/
	mutex_lock(&pdt->hw_read_lock);
	xo_tick = __arch_counter_get_cntvct();
	*(uint8_t *)smem_buffer = (uint8_t)PDT_DS_VERSION;
	smem_buffer += sizeof(uint8_t);
	*(uint64_t *)smem_buffer = (uint64_t)xo_tick;
	smem_buffer += sizeof(uint64_t);
	*(uint8_t *)smem_buffer = (uint8_t)num_nodes;
	smem_buffer += sizeof(uint8_t);
	qptm_for_each_node(qptm_share_node_info_cb, pdt);
	*(uint64_t *)smem_buffer = (uint64_t)xo_tick;
	mutex_unlock(&pdt->hw_read_lock);

	return 0;
}

int qptm_share_node_data_cb(struct qptm *qptm, void *data)
{
	int ret;
	struct pdt_priv *pdt = (struct pdt_priv *)data;
	uint16_t node_id;
	uint64_t total_energy_uj;
	uint64_t power_avg_uw;

	if (qptm == NULL || data == NULL)
		return -EINVAL;

	ret = qptm_get_node_id(qptm);
	if (ret < 0) {
		PDT_DBG_EVENT(pdt,
			"unable to read node_id: %d", ret);
		return ret;
	}
	node_id = (uint16_t)ret;

	ret = qptm_get_energy_in_uj(qptm, &total_energy_uj);
	if (ret < 0) {
		PDT_DBG_EVENT(pdt,
			"unable to read total_energy_uj for node_id %d: %d",
			node_id, ret);
		return ret;
	}

	ret = qptm_get_power_in_uw(qptm, &power_avg_uw);
	if (ret < 0) {
		PDT_DBG_EVENT(pdt,
			"unable to read power_avg_uw for node_id %d: %d",
			node_id, ret);
		return ret;
	}

	*(uint16_t *)smem_buffer = node_id;
	PDT_DBG_EVENT(pdt, "node_id=0x%x", node_id);
	smem_buffer += sizeof(uint16_t);
	*(uint64_t *)smem_buffer = total_energy_uj;
	smem_buffer += sizeof(uint64_t);
	*(uint64_t *)smem_buffer = power_avg_uw;
	smem_buffer += sizeof(uint64_t);
	PDT_DBG_DATA(pdt,
		"0x%04x, 0x%016llx, 0x%016llx",
		node_id,
		total_energy_uj,
		power_avg_uw);

	return 0;
}

int qti_pdt_share_updated_data_cb(struct notifier_block *nb, unsigned long action, void *data)
{
	struct pdt_priv *pdt = container_of(nb, struct pdt_priv, pdt_nb);
	uint64_t xo_tick;

	if (pdt == NULL)
		return -EINVAL;

	if (!qcom_smem_is_available())
		return -ENODEV;

	smem_buffer = qcom_smem_get(PDT_DS_SMEM_PROC_ID, PDT_DS_SMEM_ITEM_HANDLE, NULL);

	if (IS_ERR(smem_buffer))
		return (int)PTR_ERR(smem_buffer);

	/* Point to the data section of the shared memory buffer */
	smem_buffer += PDT_DS_SMEM_DATA_OFFSET;

	/*
	 * Write out the data to the shared memory
	 *
	 * Data format:
	 *
	 * <version><xo_tick><#nodes><node_id><power_value_0><power_value_1><xo_tick>
	 * <uint8>  <uint64> <uint8> <uint16> <uint64>       <uint64>       <uint64>
	 **/
	mutex_lock(&pdt->hw_read_lock);
	xo_tick = __arch_counter_get_cntvct();
	*(uint8_t *)smem_buffer = (uint8_t)PDT_DS_VERSION;
	smem_buffer += sizeof(uint8_t);
	*(uint64_t *)smem_buffer = (uint64_t)xo_tick;
	smem_buffer += sizeof(uint64_t);
	*(uint8_t *)smem_buffer = (uint8_t)num_nodes;
	smem_buffer += sizeof(uint8_t);
	PDT_DBG_DATA(pdt, "node_id total_energy_uj     power_avg_uw");
	qptm_for_each_node(qptm_share_node_data_cb, pdt);
	*(uint64_t *)smem_buffer = (uint64_t)xo_tick;
	PDT_DBG_EVENT(pdt, "xo_tick=%lld (0x%llx)", xo_tick, xo_tick);
	mutex_unlock(&pdt->hw_read_lock);

	return 0;
}

static int qti_pdt_hw_init(struct pdt_priv *pdt)
{
	int rc;

	mutex_init(&pdt->hw_read_lock);

	/* Initialize pdt data sharing */
	rc = qti_pdt_init_data_sharing(pdt);
	if (rc < 0) {
		dev_err(pdt->dev,
			"Failed to initialize data sharing for pdt, rc=%d\n", rc);
		return rc;
	}

	/* Register for pdt data updates */
	pdt->pdt_nb.notifier_call = qti_pdt_share_updated_data_cb;
	qptm_data_update_notifier_register(&pdt->pdt_nb);

	return 0;
}

static void qti_ptd_hw_release(struct pdt_priv *pdt)
{
	qptm_data_update_notifier_unregister(&pdt->pdt_nb);
}

static int qti_ptd_device_probe(struct platform_device *pdev)
{
	int ret;
	struct pdt_priv *pdt;

	pdt = devm_kzalloc(&pdev->dev, sizeof(*pdt), GFP_KERNEL);
	if (!pdt)
		return -ENOMEM;

	pdt->dev = &pdev->dev;
	pdt->ipc_log_data = ipc_log_context_create(IPC_LOGPAGES, "pdt_data", 0);
	if (!pdt->ipc_log_data)
		dev_err(pdt->dev, "%s: unable to create IPC Logging for %s\n",
					__func__, "pdt_data");

	pdt->ipc_log_event = ipc_log_context_create(IPC_LOGPAGES, "pdt_event", 0);
	if (!pdt->ipc_log_event)
		dev_err(pdt->dev, "%s: unable to create IPC Logging for %s\n",
					__func__, "pdt_event");

	ret = qti_pdt_hw_init(pdt);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: init failed\n", __func__);
		goto clean_up;
	}
	platform_set_drvdata(pdev, pdt);
	dev_set_drvdata(pdt->dev, pdt);

	return 0;

clean_up:
	ipc_log_context_destroy(pdt->ipc_log_data);
	ipc_log_context_destroy(pdt->ipc_log_event);
	return ret;
}

static void qti_ptd_device_remove(struct platform_device *pdev)
{
	struct pdt_priv *pdt = platform_get_drvdata(pdev);

	ipc_log_context_destroy(pdt->ipc_log_data);
	ipc_log_context_destroy(pdt->ipc_log_event);
	qti_ptd_hw_release(pdt);
}

static const struct of_device_id qti_ptd_device_match[] = {
	{.compatible = "qcom,power-telemetry-data-share"},
	{}
};

static struct platform_driver qti_ptd_share_module = {
	.probe          = qti_ptd_device_probe,
	.remove         = qti_ptd_device_remove,
	.driver         = {
		.name   = PDT_HW,
		.of_match_table = qti_ptd_device_match,
	},
};

module_platform_driver(qti_ptd_share_module);

MODULE_SOFTDEP("pre: qti_power_telemetry");
MODULE_DESCRIPTION("Qualcomm Power Telemetry data share module");
MODULE_LICENSE("GPL");
