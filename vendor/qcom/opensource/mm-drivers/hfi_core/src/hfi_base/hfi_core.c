// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/module.h>
#include "hfi_interface.h"
#include "hfi_core.h"
#include "hfi_if_abstraction.h"
#include "hfi_swi.h"
#include "hfi_smmu.h"
#include "hfi_ipc.h"
#include "hfi_queue_controller.h"
#include "hfi_core_debug.h"

struct hfi_core_drv_data *drv_data;

static int hfi_ipc_core_cb(void *data, enum hfi_core_client_id client_idx,
	enum ipc_notification_type ipc_notify)
{
	struct client_data *client_data;
	u32 flags = 0;
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || client_idx >= HFI_CORE_CLIENT_ID_MAX) {
		ret = -EINVAL;
		HFI_CORE_ERR("invalid client id provided: client_id : %d\n",
			client_idx);
		goto error;
	}
	client_data = &drv_data->client_data[client_idx];

	switch (ipc_notify) {
	case (HFI_IPC_EVENT_QUEUE_NOTIFY):
		if (client_idx != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
#if IS_ENABLED(CONFIG_DEBUG_FS)
			if (hfi_core_loop_back_mode_enable &&
				client_data->power_event &&
				(!(*(int *)client_data->power_event))) {
				(*(int *)client_data->power_event) = true;
				wake_up_all((wait_queue_head_t *)
					client_data->wait_queue);
				break;
			}
#endif // CONFIG_DEBUG_FS
			if (client_data->xfer_event &&
				(!(*(int *)client_data->xfer_event))) {
				(*(int *)client_data->xfer_event) = true;
				wake_up_all((wait_queue_head_t *)
					client_data->wait_queue);
				break;
			}
		}
		if (client_data && client_data->cb_fn) {
			client_data->cb_fn(client_data->session,
				client_data->cb_data, flags);
		}

		break;
	case (HFI_IPC_EVENT_POWER_NOTIFY):
		/* notify IFAL about the power notification for this client */
		power_notification(client_idx, drv_data);
		break;
	default:
		HFI_CORE_ERR("invalid IPC notification: %d\n", ipc_notify);
		break;
	}

	HFI_CORE_DBG_H("-\n");
error:
	return ret;
}

int hfi_core_init(struct hfi_core_drv_data *init_drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!init_drv_data) {
		HFI_CORE_ERR("invalid params\n");
		ret = -EINVAL;
		goto exit;
	}
	drv_data = init_drv_data;

	ret = init_smmu(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to init smmu ret :%d\n", ret);
		goto exit;
	}

	ret = init_swi(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to init swi ret :%d\n", ret);
		goto exit;
	}

	ret = init_resources(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to init queues ret :%d\n", ret);
		goto exit;
	}

	ret = init_ipc(drv_data, hfi_ipc_core_cb);
	if (ret) {
		HFI_CORE_ERR("failed to init ipc ret :%d\n", ret);
		goto exit;
	}

	ret = hfi_core_dbg_debugfs_register(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to register debugfs ret :%d\n", ret);
		goto exit;
	}

	HFI_CORE_DBG_H("-\n");
exit:
	return ret;
}

int hfi_core_deinit(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		ret = -EINVAL;
		goto exit;
	}

	hfi_core_dbg_debugfs_unregister(drv_data);

	ret = deinit_ipc(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit ipc ret :%d\n", ret);
		goto exit;
	}

	ret = deinit_resources(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit resources ret :%d\n", ret);
		goto exit;
	}

	ret = deinit_swi(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit swi ret :%d\n", ret);
		goto exit;
	}

	ret = deinit_smmu(drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit smmu ret :%d\n", ret);
		goto exit;
	}

	HFI_CORE_DBG_H("-\n");
exit:
	return ret;
}

struct hfi_core_session *hfi_core_open_session(
	struct hfi_core_open_params *params)
{
	struct hfi_core_session *hfi_handle;
	u32 client_id;
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!params || !params->ops ||
		params->client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid hfi open params or client id\n");
		return NULL;
	}
	client_id = params->client_id;

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (client_id == HFI_CORE_CLIENT_ID_0) {
		if (msm_hfi_fail_client_0_reg)
			return NULL;
	}
#endif /* CONFIG_DEBUG_FS */

	/* if same client requested again, return previous handle */
	if (drv_data->client_data[client_id].session) {
		HFI_CORE_ERR("client: %d already present\n", client_id);
		return drv_data->client_data[client_id].session;
	}

	hfi_handle = kzalloc(sizeof(*hfi_handle), GFP_KERNEL);
	if (!hfi_handle) {
		HFI_CORE_ERR("failed to allocate memory for hfi_handle\n");
		return NULL;
	}

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		ret = set_power_vote(drv_data, true);
		if (ret) {
			HFI_CORE_ERR("failed to vote power, ret: %d\n", ret);
			goto error;
		}
	}

	hfi_handle->client_id = client_id;
	drv_data->client_data[client_id].session = hfi_handle;
	drv_data->client_data[client_id].cb_fn = params->ops->hfi_cb_fn;
	drv_data->client_data[client_id].cb_data = params->ops->cb_data;
	if (client_id == HFI_CORE_CLIENT_ID_LOOPBACK_DCP)
		goto exit;

	ret = power_init(client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to power on for client: %d ret: %d\n",
			client_id, ret);
		goto error;
	}

exit:
	HFI_CORE_DBG_H("-\n");
	return hfi_handle;

error:
	kfree(hfi_handle);
	return NULL;
}
EXPORT_SYMBOL_GPL(hfi_core_open_session);

int hfi_core_close_session(struct hfi_core_session *hfi_handle)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!hfi_handle) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (hfi_handle->client_id < HFI_CORE_CLIENT_ID_0 ||
		hfi_handle->client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client: %d\n", hfi_handle->client_id);
		return -EINVAL;
	}

	/* remove client data for drv data */
	drv_data->client_data[hfi_handle->client_id].cb_fn = NULL;
	drv_data->client_data[hfi_handle->client_id].cb_data = NULL;
	drv_data->client_data[hfi_handle->client_id].session = NULL;

	ret = power_deinit(hfi_handle->client_id, drv_data);
	if (ret) {
		HFI_CORE_ERR("failed to deinit power for client: %d ret: %d\n",
			hfi_handle->client_id, ret);
	}

	kfree(hfi_handle);

	HFI_CORE_DBG_H("-\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hfi_core_close_session);

int hfi_core_cmds_tx_buf_get(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("invalid hfi session or buffer desc\n");
		return -EINVAL;
	}

	ret = get_tx_buffer(drv_data, hfi_session->client_id, buff_desc);
	if (ret) {
		HFI_CORE_ERR("invalid hfi buffer descriptor\n");
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hfi_core_cmds_tx_buf_get);

int hfi_core_cmds_rx_buf_get(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("invalid hfi session or buffer desc\n");
		return -EINVAL;
	}

	ret = get_rx_buffer(drv_data, hfi_session->client_id, buff_desc);
	if (ret) {
		HFI_CORE_DBG_INFO("failed to get buffer ret:%d\n", ret);
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hfi_core_cmds_rx_buf_get);

int hfi_core_cmds_tx_buf_send(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc,
	u32 flags)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* update tx-buff signal */
	ret = set_tx_buffer(drv_data, hfi_session->client_id, buff_desc,
		num_buff_desc);
	if (ret) {
		HFI_CORE_ERR("failed to set tx buff for signal\n");
		return ret;
	}
	/*
	 * Memory barrier to make sure buffer is written to queue
	 */
	wmb();

	/* trigger the ipc now after setting the tx-buff */
	if (flags & HFI_CORE_SET_FLAGS_TRIGGER_IPC) {
		ret = trigger_ipc(hfi_session->client_id, drv_data, HFI_IPC_EVENT_QUEUE_NOTIFY);
		if (ret) {
			HFI_CORE_ERR("failed to triger ipc for tx buff\n");
			return ret;
		}
	} else {
		HFI_CORE_DBG_INFO("IPC triggering not requested\n");
	}
	/*
	 * Memory barrier to make sure value is written on to ipcc register
	 */
	wmb();

	HFI_CORE_DBG_H("-\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hfi_core_cmds_tx_buf_send);

int hfi_core_release_rx_buffer(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* release Rx Buff	*/
	return put_rx_buffer(drv_data, hfi_session->client_id, buff_desc,
		num_buff_desc);
}
EXPORT_SYMBOL_GPL(hfi_core_release_rx_buffer);

int hfi_core_release_tx_buffer(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc)
{
	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* release Tx Buff without signal */
	return put_tx_buffer(drv_data, hfi_session->client_id, buff_desc,
		num_buff_desc);
}
EXPORT_SYMBOL_GPL(hfi_core_release_tx_buffer);

int hfi_core_cmds_tx_device_buf_send(struct hfi_core_session *hfi_session,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc, u32 flags)
{
	int rc = 0;

	HFI_CORE_DBG_H("+\n");

	if (!hfi_session || !buff_desc) {
		HFI_CORE_ERR("%s: invalid params\n", __func__);
		return -EINVAL;
	}

	/* update tx-buff signal */
	rc = set_device_tx_buffer(drv_data, hfi_session->client_id, buff_desc,
		num_buff_desc);
	if (rc) {
		HFI_CORE_ERR("%s: failed to set tx buff for signal\n",
			__func__);
		return rc;
	}

	/* trigger the ipc now after setting the tx-buff */
	if (flags & HFI_CORE_SET_FLAGS_TRIGGER_IPC)
		trigger_ipc(hfi_session->client_id, drv_data,
			HFI_IPC_EVENT_QUEUE_NOTIFY);

	HFI_CORE_DBG_H("-\n");
	return rc;
}
EXPORT_SYMBOL_GPL(hfi_core_cmds_tx_device_buf_send);

int hfi_core_allocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info,
	u32 size, enum hfi_core_dma_alloc_type type, u32 flags)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!alloc_info || !size) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (!IS_ALIGNED(size, HFI_CORE_IOMMU_MAP_SIZE_ALIGNMENT)) {
		HFI_CORE_ERR("failed to get aligned size\n");
		return -EINVAL;
	}
	alloc_info->size_allocated = size;

	/* allocate memory */
	ret = smmu_alloc_and_map_for_drv(drv_data, &alloc_info->phy_addr,
		alloc_info->size_allocated, &alloc_info->cpu_va, type);
	if (ret) {
		HFI_CORE_ERR("failed to alloc, ret: %d\n", ret);
		return ret;
	}

	/* map memory */
	ret = smmu_mmap_for_fw(drv_data, alloc_info->phy_addr, &alloc_info->mapped_iova,
		alloc_info->size_allocated, flags);
	if (ret) {
		HFI_CORE_ERR("failed to map to fw, ret: %d\n", ret);
		goto mmap_fail;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;

mmap_fail:
	/* unmap for drv */
	if (alloc_info->cpu_va)
		smmu_unmap_for_drv(alloc_info->cpu_va, alloc_info->size_allocated);
	alloc_info->size_allocated = 0;
	alloc_info->cpu_va = NULL;

	HFI_CORE_DBG_H("-\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hfi_core_allocate_shared_mem);

int hfi_core_deallocate_shared_mem(struct hfi_core_mem_alloc_info *alloc_info)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!alloc_info || !alloc_info->size_allocated) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	/* unmap for fw */
	ret = smmu_unmmap_for_fw(drv_data, alloc_info->mapped_iova,
		alloc_info->size_allocated);
	if (ret) {
		HFI_CORE_ERR("unmap failed\n");
		return -EINVAL;
	}
	/* unmap for drv */
	if (alloc_info->cpu_va)
		smmu_unmap_for_drv(alloc_info->cpu_va, alloc_info->size_allocated);

	HFI_CORE_DBG_H("-\n");
	return ret;

}
EXPORT_SYMBOL_GPL(hfi_core_deallocate_shared_mem);
