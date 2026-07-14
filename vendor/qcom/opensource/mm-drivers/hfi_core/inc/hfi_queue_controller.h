/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_QUEUE_CONTROLLER_H__
#define __HFI_QUEUE_CONTROLLER_H__

#include "hfi_core.h"

/**
 * set_tx_buffer() - Set TX buffer descriptor.
 *
 * This calls sets the buffer within the transport layer, so the buffer
 * is received by the other end, receiving the buffers transferred by this
 * client. Once this API is called, buffer must not be dereferenced anymore
 * by this client.
 *
 * Return: 0 on success or negative errno
 */
int set_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * get_rx_buffer() - Get rx buffer descriptor .
 *
 * This calls get the buffer descriptor from the clients Rx queues.
 *
 * Return: 0 on success or negative errno
 */
int get_rx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc);

/**
 * get_tx_buffer() - Get tx buffer descriptor .
 *
 * This call gets a Tx buffer that must be filled by the client
 * with the data to send to the other end.
 *
 * Return: 0 on success or negative errno.
 */
int get_tx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buf_desc);

/**
 * put_rx_buffer() - Releases Rx buffer.
 *
 * This call releases the rx buffer based on the buffer descriptor provided.
 *
 * Return: 0 on success or negative errno
 */
int put_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * put_tx_buffer() - Releases Tx buffer.
 *
 * This call releases the tx buffer based on the buffer descriptor provided.
 *
 * Return: 0 on success or negative errno
 */
int put_rx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * get_queue_size_req() - Gets the minimum size required to create a queue.
 *
 * This call gets the minimum size required to for the creation of
 * the queue.
 *
 * Return: size
 */
int get_queue_size_req(u32 qdepth);

/**
 * init_queues() - Setup queues for firmware communication
 *
 * This call creates all queues required for the client aligning to
 * the resources requirements specified in clients compat table
 *
 * Return: 0 on success or negative errno
 */
int init_queues(enum hfi_core_client_id, struct hfi_core_drv_data *drv_data);

/**
 * deinit_queues() - Destroy all queues to stop firmware communication.
 *
 * This call destroys all queues created by init_queues() API.
 *
 * Return: 0 on success or negative errno
 */
int deinit_queues(enum hfi_core_client_id, struct hfi_core_drv_data *drv_data);

#if IS_ENABLED(CONFIG_DEBUG_FS)
/**
 * set_device_tx_buffer() - Set TX buffer descriptor for device mode
 *
 * This calls sets the buffer within the transport layer, so the buffer
 * is received by the other end, receiving the buffers transferred by this
 * client. Once this API is called, buffer must not be dereferenced anymore by
 * this client.
 *
 * Return: 0 on success or negative errno
 */
int set_device_tx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

/**
 * get_device_rx_buffer() - Get rx buffer descriptor for device mode.
 *
 * This calls get the buffer descriptor from the clients Rx queues.
 *
 * Return: 0 on success or negative errno
 */
int get_device_rx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buff_desc);

/**
 * get_device_tx_buffer() - Get tx buffer descriptor for device mode.
 *
 * This call gets a Tx buffer that must be filled by the client
 * with the data to send to the other end.
 *
 * Return: 0 on success or negative errno.
 */
int get_device_tx_buffer(struct hfi_core_drv_data *drv_data,
	u32 client_id, struct hfi_core_cmds_buf_desc *buf_desc);

/**
 * put_device_rx_buffer() - Releases Rx buffer for device mode.
 *
 * This call releases the tx buffer based on the buffer descriptor provided.
 *
 * Return: 0 on success or negative errno
 */
int put_device_rx_buffer(struct hfi_core_drv_data *drv_data, u32 client_id,
	struct hfi_core_cmds_buf_desc **buff_desc, u32 num_buff_desc);

#endif // CONFIG_DEBUG_FS

#endif // __HFI_QUEUE_CONTROLLER_H__
