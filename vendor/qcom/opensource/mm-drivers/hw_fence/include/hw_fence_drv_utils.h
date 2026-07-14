/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HW_FENCE_DRV_UTILS_H
#define __HW_FENCE_DRV_UTILS_H

/**
 * HW_FENCE_CLIENT_ID_CTRL_QUEUE:
 * Client ID representing ctrl queue. This bit is set in signaled clients mask if hw fence driver
 * should read ctrl rx queue
 */
#define HW_FENCE_CLIENT_ID_CTRL_QUEUE 0

/**
 * HW_FENCE_MAX_CLIENT_TYPE_STATIC:
 * Total number of client types without configurable number of sub-clients (none)
 */
#define HW_FENCE_MAX_CLIENT_TYPE_STATIC 0

/**
 * HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE:
 * Maximum number of client types with configurable number of sub-clients (e.g. IPE, VPU, IFE, IPA)
 */
#define HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE \
	((HW_FENCE_CLIENT_MAX - HW_FENCE_CLIENT_ID_CTX0) / MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT)

/**
 * HW_FENCE_MAX_STATIC_CLIENTS_INDEX:
 * Maximum number of static clients, i.e. clients without configurable numbers of sub-clients.
 * This is only the client_id:0 reserved for ctrl queue.
 */
#define HW_FENCE_MAX_STATIC_CLIENTS_INDEX HW_FENCE_CLIENT_ID_CTX0

/**
 * enum hw_fence_mem_reserve - Types of reservations for the carved-out memory.
 * HW_FENCE_MEM_RESERVE_CTRL_QUEUE: Reserve memory for the ctrl rx/tx queues.
 * HW_FENCE_MEM_RESERVE_LOCKS_REGION: Reserve memory for the per-client locks memory region.
 * HW_FENCE_MEM_RESERVE_TABLE: Reserve memory for the hw-fences global table.
 * HW_FENCE_MEM_RESERVE_CLIENT_QUEUE: Reserve memory per-client for the rx/tx queues.
 * HW_FENCE_MEM_RESERVE_EVENTS_BUFF: Reserve memory for the debug events
 */
enum hw_fence_mem_reserve {
	HW_FENCE_MEM_RESERVE_CTRL_QUEUE,
	HW_FENCE_MEM_RESERVE_LOCKS_REGION,
	HW_FENCE_MEM_RESERVE_TABLE,
	HW_FENCE_MEM_RESERVE_CLIENT_QUEUE,
	HW_FENCE_MEM_RESERVE_EVENTS_BUFF
};

#define hw_fence_wait_event_timeout(waitq, cond, timeout_ms, ret)	\
	do {								\
		ktime_t cur_ktime;					\
		ktime_t exp_ktime;					\
		s64 wait_time_jiffies = msecs_to_jiffies(timeout_ms);	\
\
		exp_ktime = ktime_add_ms(ktime_get(), timeout_ms);	\
		do {							\
			ret = wait_event_timeout(waitq, cond,		\
					wait_time_jiffies);		\
			cur_ktime = ktime_get();			\
		} while ((!cond) && (ret == 0) &&			\
			(ktime_compare(ktime_sub(exp_ktime, cur_ktime), ktime_set(0, 0)) > 0));\
	} while (0)

/**
 * global_atomic_store() - Inter-processor lock
 * @drv_data: hw fence driver data
 * @lock: memory to lock
 * @val: if true, api locks the memory, if false it unlocks the memory
 */
void global_atomic_store(struct hw_fence_driver_data *drv_data, uint64_t *lock, bool val);

/**
 * hw_fence_utils_init_virq() - Initialize doorbell (i.e. vIRQ) for SVM to HLOS signaling
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_init_virq(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_init_soccp_irq() - Initialize interrupt handler for SOCCP to HLOS signaling
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_init_soccp_irq(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_register_soccp_ssr_notifier() - registers rproc ssr notifier for soccp
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_register_soccp_ssr_notifier(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_process_signaled_clients_mask() - Process the mask containing HW Fence client IDs
 *                                                  that HW Fence Driver is responsible for, i.e.
 *                                                  ctrl queue and validation clients.
 * @drv_data: hw fence driver data.
 * @mask: mask with signaled clients
 */
void hw_fence_utils_process_signaled_clients_mask(struct hw_fence_driver_data *drv_data,
	u64 mask);

/**
 * hw_fence_utils_alloc_mem() - Allocates the carved-out memory pool that will be used for the HW
 *                              Fence global table, locks and queues.
 * @hw_fence_drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_alloc_mem(struct hw_fence_driver_data *hw_fence_drv_data);

/**
 * hw_fence_utils_reserve_mem() - Reserves memory from the carved-out memory pool.
 * @drv_data: hw fence driver data.
 * @type: memory reservation type.
 * @phys: physical address of the carved-out memory pool
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_reserve_mem(struct hw_fence_driver_data *drv_data,
	enum hw_fence_mem_reserve type, phys_addr_t *phys, void **pa, u32 *size, int client_id);

/**
 * hw_fence_utils_parse_dt_props() -  Init dt properties
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_parse_dt_props(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_map_ipcc() -  Maps IPCC registers and enable signaling
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_map_ipcc(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_map_qtime() -  Maps qtime register
 * @drv_data: hw fence driver data
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_map_qtime(struct hw_fence_driver_data *drv_data);

/**
 * hw_fence_utils_cleanup_fence() -  Cleanup the hw-fence from a specified client
 * @drv_data: hw fence driver data
 * @hw_fence_client: client, for which the fence must be cleared
 * @hw_fence: hw-fence to cleanup
 * @hash: hash of the hw-fence to cleanup
 * @reset_flags: flags to determine how to handle the reset
 *
 * Returns zero if success, otherwise returns negative error code.
 */
int hw_fence_utils_cleanup_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct msm_hw_fence *hw_fence, u64 hash,
	u32 reset_flags);

/**
 * hw_fence_utils_fence_error_cb() - Invokes fence error callback registered by specified client
 *
 * @hw_fence_client: client, for which fence error callback must be invoked
 * @ctxt_id: context id of the hw-fence
 * @seqno: sequence number of the hw-fence
 * @hash: hash of the hw-fence
 * @flags: flags of the hw-fence
 * @error: error of the hw-fence
 *
 * Returns zero if success, otherwise returns negative error code
 */
int hw_fence_utils_fence_error_cb(struct msm_hw_fence_client *hw_fence_client, u64 ctxt_id,
	u64 seqno, u64 hash, u64 flags, u32 error);

/**
 * hw_fence_utils_get_client_id_priv() - Gets the index into clients struct within hw fence driver
 *                                       from the client_id used externally
 *
 * Performs a 1-to-1 mapping for all client IDs less than HW_FENCE_MAX_STATIC_CLIENTS_INDEX,
 * otherwise consolidates client IDs of clients with configurable number of sub-clients. Fails if
 * provided with client IDs for such clients when support for those clients is not configured in
 * device-tree.
 *
 * @drv_data: hw fence driver data
 * @client_id: external client_id to get internal client_id for
 *
 * Returns client_id < drv_data->clients_num if success, otherwise returns HW_FENCE_CLIENT_MAX
 */
enum hw_fence_client_id hw_fence_utils_get_client_id_priv(struct hw_fence_driver_data *drv_data,
	enum hw_fence_client_id client_id);

/**
 * hw_fence_utils_get_queues_num() - Returns number of client queues for the client_id.
 *
 * @drv_data: driver data
 * @client_id: hw fence driver client id
 *
 * Returns: number of client queues
 */
int hw_fence_utils_get_queues_num(struct hw_fence_driver_data *drv_data, int client_id);

/**
 * hw_fence_utils_get_skip_fctl_ref() - Returns if client avoids creating fences with fctl
 *                                      refcount initialized.
 *
 * @drv_data: driver data
 * @client_id: hw fence driver client id
 *
 * Returns: number of client queues
 */
int hw_fence_utils_get_skip_fctl_ref(struct hw_fence_driver_data *drv_data, int client_id);

/**
 * hw_fence_utils_update_power_payload() - Initialize a power payload for given client and
 * requested power state.
 *
 * @drv_data: driver data
 * @payload: payload to be initialized
 * @client_id: hw fence driver client id
 * @state: true if enabling power, false otherwise
 *
 * Returns: number of client queues
 */
void hw_fence_utils_update_power_payload(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_queue_payload_enable_power *payload, enum hw_fence_client_id client_id,
	bool state);

/**
 * hw_fence_utils_set_power_vote() - Sets the power vote for soccp.
 *
 * @drv_data: driver data
 * @client_id: client id that is requesting power
 * @state: power state to set
 *
 * Returns: 0 if successful, error if not
 */
int hw_fence_utils_set_power_vote(struct hw_fence_driver_data *drv_data,
	enum hw_fence_client_id client_id, bool state);
#endif /* __HW_FENCE_DRV_UTILS_H */
