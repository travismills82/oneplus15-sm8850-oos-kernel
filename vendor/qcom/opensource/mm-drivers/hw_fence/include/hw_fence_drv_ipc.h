/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HW_FENCE_DRV_IPC_H
#define __HW_FENCE_DRV_IPC_H

/* ipc clients virtual client-id */
#define HW_FENCE_IPC_CLIENT_ID_APPS_VID 8
#define HW_FENCE_IPC_CLIENT_ID_GPU_VID 9
#define HW_FENCE_IPC_CLIENT_ID_IPE_VID 11
#define HW_FENCE_IPC_CLIENT_ID_VPU_VID 12
#define HW_FENCE_IPC_CLIENT_ID_DPU_VID 25
#define HW_FENCE_IPC_CLIENT_ID_IPA_VID 26
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS1_VID 33
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS2_VID 34
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS3_VID 35
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS4_VID 36
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_VID 46
#define HW_FENCE_IPC_CLIENT_ID_GPU1_VID 54
#define HW_FENCE_IPC_CLIENT_ID_DCP_VID 57
#define HW_FENCE_IPC_CLIENT_ID_LSR_VID 63
#define HW_FENCE_IPC_CLIENT_ID_IFE0_VID 128
#define HW_FENCE_IPC_CLIENT_ID_IFE1_VID 129
#define HW_FENCE_IPC_CLIENT_ID_IFE2_VID 130
#define HW_FENCE_IPC_CLIENT_ID_IFE3_VID 131
#define HW_FENCE_IPC_CLIENT_ID_IFE4_VID 132
#define HW_FENCE_IPC_CLIENT_ID_IFE5_VID 133
#define HW_FENCE_IPC_CLIENT_ID_IFE6_VID 134
#define HW_FENCE_IPC_CLIENT_ID_IFE7_VID 135
#define HW_FENCE_IPC_CLIENT_ID_IFE8_VID 136
#define HW_FENCE_IPC_CLIENT_ID_IFE9_VID 137
#define HW_FENCE_IPC_CLIENT_ID_IFE10_VID 138
#define HW_FENCE_IPC_CLIENT_ID_IFE11_VID 139

/* ipc clients physical client-id */
#define HW_FENCE_IPC_CLIENT_ID_APPS_PID 3
#define HW_FENCE_IPC_CLIENT_ID_GPU_PID 4
#define HW_FENCE_IPC_CLIENT_ID_IPE_PID 5
#define HW_FENCE_IPC_CLIENT_ID_VPU_PID 8
#define HW_FENCE_IPC_CLIENT_ID_DPU_PID 9
#define HW_FENCE_IPC_CLIENT_ID_IFE0_PID 11
#define HW_FENCE_IPC_CLIENT_ID_IFE1_PID 12
#define HW_FENCE_IPC_CLIENT_ID_IFE2_PID 13
#define HW_FENCE_IPC_CLIENT_ID_IFE3_PID 14
#define HW_FENCE_IPC_CLIENT_ID_IFE4_PID 15
#define HW_FENCE_IPC_CLIENT_ID_IFE5_PID 16
#define HW_FENCE_IPC_CLIENT_ID_IFE6_PID 17
#define HW_FENCE_IPC_CLIENT_ID_IFE7_PID 18
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_PID 22

/* ipc clients physical client-id on other targets */
#define HW_FENCE_IPC_CLIENT_ID_IPE_PID_SUN 9
#define HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN 20
#define HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE 2
#define HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE 3
#define HW_FENCE_IPC_CLIENT_ID_IPA_PID_NIOBE 4
#define HW_FENCE_IPC_CLIENT_ID_GPU_PID_NIOBE 8
#define HW_FENCE_IPC_CLIENT_ID_IPE_PID_NIOBE 10
#define HW_FENCE_IPC_CLIENT_ID_VPU_PID_NIOBE 11
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_PID_NIOBE 13
#define HW_FENCE_IPC_CLIENT_ID_IFE0_PID_NIOBE 15
#define HW_FENCE_IPC_CLIENT_ID_IFE1_PID_NIOBE 16
#define HW_FENCE_IPC_CLIENT_ID_IFE2_PID_NIOBE 17
#define HW_FENCE_IPC_CLIENT_ID_IFE3_PID_NIOBE 18
#define HW_FENCE_IPC_CLIENT_ID_IFE4_PID_NIOBE 19
#define HW_FENCE_IPC_CLIENT_ID_IFE5_PID_NIOBE 20
#define HW_FENCE_IPC_CLIENT_ID_IFE6_PID_NIOBE 21
#define HW_FENCE_IPC_CLIENT_ID_IFE7_PID_NIOBE 22
#define HW_FENCE_IPC_CLIENT_ID_IFE8_PID_NIOBE 23
#define HW_FENCE_IPC_CLIENT_ID_IFE9_PID_NIOBE 24
#define HW_FENCE_IPC_CLIENT_ID_IFE10_PID_NIOBE 25
#define HW_FENCE_IPC_CLIENT_ID_IFE11_PID_NIOBE 26

#define HW_FENCE_IPC_CLIENT_ID_APPS_PID_SA8797 5
#define HW_FENCE_IPC_CLIENT_ID_GPU0_PID_SA8797 6
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS1_PID_SA8797 7
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS2_PID_SA8797 8
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS3_PID_SA8797 9
#define HW_FENCE_IPC_CLIENT_ID_VPU_PID_SA8797 26
#define HW_FENCE_IPC_CLIENT_ID_DPU0_PID_SA8797 27
#define HW_FENCE_IPC_CLIENT_ID_DPU1_PID_SA8797 28
#define HW_FENCE_IPC_CLIENT_ID_APPS_NS4_PID_SA8797 31
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_PID_SA8797 33
#define HW_FENCE_IPC_CLIENT_ID_IPA_PID_SA8797 34
#define HW_FENCE_IPC_CLIENT_ID_GPU1_PID_SA8797 35

#define HW_FENCE_IPC_CLIENT_ID_IPE_PID_CANOE 9
#define HW_FENCE_IPC_CLIENT_ID_IFE0_PID_CANOE 12
#define HW_FENCE_IPC_CLIENT_ID_IFE1_PID_CANOE 13
#define HW_FENCE_IPC_CLIENT_ID_IFE2_PID_CANOE 14
#define HW_FENCE_IPC_CLIENT_ID_IFE3_PID_CANOE 15
#define HW_FENCE_IPC_CLIENT_ID_IFE4_PID_CANOE 16
#define HW_FENCE_IPC_CLIENT_ID_IFE5_PID_CANOE 17
#define HW_FENCE_IPC_CLIENT_ID_IFE6_PID_CANOE 18
#define HW_FENCE_IPC_CLIENT_ID_IFE7_PID_CANOE 19
#define HW_FENCE_IPC_CLIENT_ID_VPU_PID_CANOE 20
#define HW_FENCE_IPC_CLIENT_ID_DPU_PID_CANOE 21
#define HW_FENCE_IPC_CLIENT_ID_IPA_PID_CANOE 22
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_PID_CANOE 24

#define HW_FENCE_IPC_CLIENT_ID_APPS_PID_SERAPH 2
#define HW_FENCE_IPC_CLIENT_ID_DPU_PID_SERAPH 3
#define HW_FENCE_IPC_CLIENT_ID_IPA_PID_SERAPH 4
#define HW_FENCE_IPC_CLIENT_ID_GPU0_PID_SERAPH 8
#define HW_FENCE_IPC_CLIENT_ID_IPE_PID_SERAPH 10
#define HW_FENCE_IPC_CLIENT_ID_VPU_PID_SERAPH 11
#define HW_FENCE_IPC_CLIENT_ID_SOCCP_PID_SERAPH 12
#define HW_FENCE_IPC_CLIENT_ID_DCP_PID_SERAPH 14
#define HW_FENCE_IPC_CLIENT_ID_LSR_PID_SERAPH 15
#define HW_FENCE_IPC_CLIENT_ID_IFE0_PID_SERAPH 17
#define HW_FENCE_IPC_CLIENT_ID_IFE1_PID_SERAPH 18
#define HW_FENCE_IPC_CLIENT_ID_IFE2_PID_SERAPH 19
#define HW_FENCE_IPC_CLIENT_ID_IFE3_PID_SERAPH 20
#define HW_FENCE_IPC_CLIENT_ID_IFE4_PID_SERAPH 21
#define HW_FENCE_IPC_CLIENT_ID_IFE5_PID_SERAPH 22
#define HW_FENCE_IPC_CLIENT_ID_IFE6_PID_SERAPH 23
#define HW_FENCE_IPC_CLIENT_ID_IFE7_PID_SERAPH 24
#define HW_FENCE_IPC_CLIENT_ID_IFE8_PID_SERAPH 25
#define HW_FENCE_IPC_CLIENT_ID_IFE9_PID_SERAPH 26
#define HW_FENCE_IPC_CLIENT_ID_IFE10_PID_SERAPH 27
#define HW_FENCE_IPC_CLIENT_ID_IFE11_PID_SERAPH 28

#define HW_FENCE_IPC_COMPUTE_L1_PROTOCOL_ID_KALAMA 2
#define HW_FENCE_IPC_COMPUTE_L1_PROTOCOL_ID_PINEAPPLE 2
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_PINEAPPLE 4
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_SUN 4
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_NIOBE 4
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_SA8797 4
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_CANOE 4
#define HW_FENCE_IPC_FENCE_PROTOCOL_ID_SERAPH 4

#define HW_FENCE_IPCC_HW_REV_170 0x00010700  /* Kalama */
#define HW_FENCE_IPCC_HW_REV_203 0x00020003  /* Pineapple */
#define HW_FENCE_IPCC_HW_REV_2A2 0x00020A02  /* Sun */
#define HW_FENCE_IPCC_HW_REV_2B4 0x00020B04  /* Niobe */
#define HW_FENCE_IPCC_HW_REV_301 0x00030001  /* SA8797 */
#define HW_FENCE_IPCC_HW_REV_312 0x00030102  /* Canoe */
#define HW_FENCE_IPCC_HW_REV_2101 0x00021001  /* Saraph */

#define HW_FENCE_IPCC_PROTOCOL_OFFSET_DEFAULT 0x40000
#define HW_FENCE_IPCC_PROTOCOL_OFFSET_CANOE 0xC0000

#define IPC_PROTOCOLp_CLIENTc_VERSION(base, off, p, c) (base + ((off)*(p)) + (0x1000*c))
#define IPC_PROTOCOLp_CLIENTc_CONFIG(base, off, p, c) (base + 0x8 + ((off)*(p)) + (0x1000*c))
#define IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_ENABLE(base, off, p, c) \
	(base + 0x14 + ((off)*(p)) + (0x1000*c))
#define IPC_PROTOCOLp_CLIENTc_SEND(base, off, p, c) ((base + 0xc) + ((off)*(p)) + (0x1000*c))
#define IPC_PROTOCOLp_CLIENTc_RECV_ID(base, off, p, c) (base + 0x10 + ((off)*(p)) + (0x1000*c))
#define IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_CLEAR(base, off, p, c) \
	(base + 0x1C + ((off)*(p)) + (0x1000*c))
#define HW_FENCE_IPC_RECV_ID_NONE 0xFFFFFFFF

/**
 * HW_FENCE_IPCC_SIGNAL_ID_MAX:
 * Max number of signals supported per-client for IPCC HW
 */
#define HW_FENCE_IPCC_SIGNAL_ID_MAX 32

/**
 * HW_FENCE_IPCC_MIN_VAL_SIGNAL:
 * Minimum signal id used for validation clients
 */
#define HW_FENCE_IPCC_MIN_VAL_SIGNAL 21

/**
 * hw_fence_ipcc_trigger_signal() - Trigger ipc signal for the requested client/signal pair.
 * @drv_data: driver data.
 * @tx_client_id: ipc client id that sends the ipc signal.
 * @rx_client_id: ipc client id that receives the ipc signal.
 * @signal_id: signal id to send.
 *
 * This API triggers the ipc 'signal_id' from the 'tx_client_id' to the 'rx_client_id'
 */
void hw_fence_ipcc_trigger_signal(struct hw_fence_driver_data *drv_data,
	u32 tx_client_id, u32 rx_client_id, u32 signal_id);

/**
 * hw_fence_ipcc_enable_signaling() - Enable ipcc signaling for hw-fence driver.
 * @drv_data: driver data.
 *
 * Return: 0 on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_enable_signaling(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_ipcc_enable_protocol() - Enable ipcc protocol used for hw-fencing
 * (either compute l1 or fence depending on target) for given client.
 * @drv_data: driver data
 * @client_id: hw fence driver client id
 *
 * This should only be called once for each IPCC client, e.g. if protocol is enabled
 * for one dpu client, it should not be called again for another dpu client.
 *
 * Return: 0 on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_enable_protocol(struct hw_fence_driver_data *drv_data, u32 client_id);

/**
 * hw_fence_ipcc_enable_client_signal_pairs() - Enable ipcc signaling for all client-signal
 * pairs required for hw-fencing for given client.
 * @drv_data: driver data.
 * @start_client: first hw fence driver client id for given ipcc client
 *
 * This API enables input signal from driver and fctl (if fctl is separate from driver) for
 * given client. IPCC protocol must be enabled via hw_fence_ipcc_enable_protocol() prior
 * to this call. This API iterates through driver's ipc client table to ensure all client-
 * signal pairs for given client are enabled.
 *
 * Return: 0 on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_enable_client_signal_pairs(struct hw_fence_driver_data *drv_data,
	u32 start_client);

/**
 * hw_fence_ipcc_get_client_virt_id() - Returns the ipc client virtual id that corresponds to the
 *		hw fence driver client.
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * The ipc client id returned by this API is used by the hw fence driver when signaling the fence.
 *
 * Return: client_id on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_get_client_virt_id(struct hw_fence_driver_data *drv_data, u32 client_id);

/**
 * hw_fence_ipcc_get_client_phys_id() - Returns the ipc client physical id that corresponds to the
 *		hw fence driver client.
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * The ipc client id returned by this API is used by the hw fence driver when signaling the fence.
 *
 * Return: client_id on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_get_client_phys_id(struct hw_fence_driver_data *drv_data, u32 client_id);

/**
 * hw_fence_ipcc_get_signal_id() - Returns the ipc signal id that corresponds to the hw fence
 *		driver client.
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * The ipc signal id returned by this API is used by the hw fence driver when signaling the fence.
 *
 * Return: client_id on success or negative errno (-EINVAL)
 */
int hw_fence_ipcc_get_signal_id(struct hw_fence_driver_data *drv_data, u32 client_id);

/**
 * hw_fence_ipcc_needs_rxq_update() - Returns bool to indicate if client uses rx-queue.
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * Return: true if client needs to update rxq, false otherwise
 */
bool hw_fence_ipcc_needs_rxq_update(struct hw_fence_driver_data *drv_data, int client_id);

/**
 * hw_fence_ipcc_signaled_needs_rxq_update() - Returns bool to indicate if client requires
 *		rx-queue update when registering to wait on an already signaled fence.
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * Return: true if client needs to update rxq when dma-fence is signaled, false otherwise
 */
bool hw_fence_ipcc_signaled_needs_rxq_update(struct hw_fence_driver_data *drv_data,
	int client_id);

/**
 * hw_fence_ipcc_signaled_needs_ipc_irq() - Returns bool to indicate if client needs ipc interrupt
 *                                          for already signaled fences
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * Return: true if client needs ipc interrupt for signaled fences, false otherwise
 */
bool hw_fence_ipcc_signaled_needs_ipc_irq(struct hw_fence_driver_data *drv_data, int client_id);

/**
 * hw_fence_ipcc_txq_update_needs_ipc_irq() - Returns bool to indicate if client needs ipc interrupt
 *                                            when updating client tx queue in hlos
 * @drv_data: driver data.
 * @client_id: hw fence driver client id.
 *
 * Return: true if client needs ipc interrupt when updating client tx queue, false otherwise
 */
bool hw_fence_ipcc_txq_update_needs_ipc_irq(struct hw_fence_driver_data *drv_data, int client_id);

/**
 * hw_fence_ipcc_get_signaled_clients_mask() - Returns mask to indicate signals for which clients
 *		were received by HW Fence Driver
 * @drv_data: driver_data
 *
 * Return: mask on success or zero upon error
 */
u64 hw_fence_ipcc_get_signaled_clients_mask(struct hw_fence_driver_data *drv_data);

#endif /* __HW_FENCE_DRV_IPC_H */
