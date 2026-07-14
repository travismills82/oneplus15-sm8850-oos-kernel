/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_IPC_H__
#define __HFI_IPC_H__

#include "hfi_core.h"

enum ipc_notification_type {
	/* Rx Queue of the client is updated */
	HFI_IPC_EVENT_QUEUE_NOTIFY = 0x1,
	/* Power notification for client */
	HFI_IPC_EVENT_POWER_NOTIFY,
	HFI_IPC_EVENT_MAX
};

typedef int (*hfi_ipc_cb)(void *data, enum hfi_core_client_id client_id,
	enum ipc_notification_type ipc_notify);

/**
 * init_ipc() - IPC initialization.
 *
 * This call initializes the communication channel like mbox etc.
 * required for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int init_ipc(struct hfi_core_drv_data *drv_data, hfi_ipc_cb hfi_core_cb);

/**
 * deinit_ipc() - IPC deinitialization.
 *
 * This call deinitializes the communication channel like mbox etc.
 * required for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int deinit_ipc(struct hfi_core_drv_data *drv_data);

/**
 * trigger_ipc() - trigger ipc signal
 *
 * This call trigger ipc signal to remote processor
 *
 * Return: 0 on success or negative errno
 */
int trigger_ipc(u32 client_id, struct hfi_core_drv_data *drv_data,
	enum ipc_notification_type ipc_notify);

#endif // __HFI_IPC_H__
