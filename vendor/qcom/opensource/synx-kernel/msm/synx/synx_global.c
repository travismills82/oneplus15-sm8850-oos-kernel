// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/hwspinlock.h>
#include <linux/string.h>

#include "synx_debugfs.h"
#include "synx_interop.h"
#include "synx_global.h"

static struct synx_shared_mem synx_gmem;
static struct hwspinlock *synx_hwlock;
static uint32_t glcoredata_size;

static inline struct synx_global_coredata *synx_fetch_global_coredata_object(u32 idx)
{
	return (struct synx_global_coredata *)((uint8_t *)synx_gmem.table + idx * glcoredata_size);
}

static u32 synx_gmem_lock_owner(u32 idx)
{
	/*
	 * subscribers field of global table index 0 is used to
	 * maintain synx gmem lock owner data.
	 * core updates the field after acquiring the lock and
	 * before releasing the lock appropriately.
	 */
	return synx_gmem.table[0].subscribers;
}

static void synx_gmem_lock_owner_set(u32 idx)
{
	synx_gmem.table[0].subscribers = SYNX_CORE_APSS;
}

static void synx_gmem_lock_owner_clear(u32 idx)
{
	if (synx_gmem.table[0].subscribers != SYNX_CORE_APSS)
		dprintk(SYNX_WARN, "reset lock owned by core %u\n",
			synx_gmem.table[0].subscribers);

	synx_gmem.table[0].subscribers = SYNX_CORE_MAX;
}

static int synx_gmem_lock(u32 idx, unsigned long *flags)
{
	int rc;
	if (get_ipclite_feature(IPCLITE_GLOBAL_LOCK))
		rc = ipclite_global_spin_lock_timeout((idx & 0xFF), SYNX_HWSPIN_TIMEOUT, flags);
	else {
		if (!synx_hwlock) {
			dprintk(SYNX_ERR, "hwspinlock request failed\n");
			return -SYNX_INVALID;
		}

		rc = hwspin_lock_timeout_irqsave(synx_hwlock, SYNX_HWSPIN_TIMEOUT, flags);
		if (!rc)
			synx_gmem_lock_owner_set(idx);
	}
	return rc;
}

static void synx_gmem_unlock(u32 idx, unsigned long *flags)
{
	if (get_ipclite_feature(IPCLITE_GLOBAL_LOCK))
		ipclite_global_spin_unlock(idx & 0xFF, flags);
	else {
		synx_gmem_lock_owner_clear(idx);
		hwspin_unlock_irqrestore(synx_hwlock, flags);
	}
}

static void synx_global_print_data(
	struct synx_global_coredata *synx_g_obj,
	const char *func)
{
	int i = 0;

	dprintk(SYNX_VERB, "%s: status %u, handle %u, refcount %u",
		func, synx_g_obj->status,
		synx_g_obj->handle, synx_g_obj->refcount);

	dprintk(SYNX_VERB, "%s: subscribers %u, waiters %u, pending %u",
		func, synx_g_obj->subscribers, synx_g_obj->waiters,
		synx_g_obj->num_child);

	for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++)
		if (synx_g_obj->parents[i])
			dprintk(SYNX_VERB, "%s: parents %u:%u",
				func, i, synx_g_obj->parents[i]);
}

bool synx_fetch_global_shared_memory_handle_details(u32 synx_handle,
		struct synx_global_coredata *synx_global_entry)
{
	int rc = SYNX_SUCCESS;
	u32 idx;
	unsigned long flags;
	struct synx_global_coredata *entry;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return false;
	}
	idx = synx_handle & SYNX_HANDLE_INDEX_MASK;
	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return false;
	}
	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return false;
	}
	entry = synx_fetch_global_coredata_object(idx);
	memcpy(synx_global_entry, entry, glcoredata_size);
	synx_gmem_unlock(idx, &flags);

	return true;
}

int synx_global_dump_shared_memory(void)
{
	int rc = SYNX_SUCCESS, idx;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_INVALID;
	}

	/* Print bitmap memory*/
	for (idx = 0; idx < SHRD_MEM_DUMP_NUM_BMAP_WORDS; idx++) {
		rc = synx_gmem_lock(idx, &flags);

		if (rc) {
			dprintk(SYNX_ERR, "Failed to lock entry %d\n", idx);
			return rc;
		}

		dprintk(SYNX_VERB, "%s: idx %d, bitmap value %d",
		__func__, idx, synx_gmem.bitmap[idx]);

		synx_gmem_unlock(idx, &flags);
	}

	/* Print table memory*/
	for (idx = 0;
		idx < SHRD_MEM_DUMP_NUM_BMAP_WORDS * sizeof(u32) * NUM_CHAR_BIT;
		idx++) {
		rc = synx_gmem_lock(idx, &flags);

		if (rc) {
			dprintk(SYNX_ERR, "Failed to lock entry %d\n", idx);
			return rc;
		}

		dprintk(SYNX_VERB, "%s: idx %d\n", __func__, idx);

		synx_g_obj = synx_fetch_global_coredata_object(idx);
		synx_global_print_data(synx_g_obj, __func__);

		synx_gmem_unlock(idx, &flags);
	}
	return rc;
}

int synx_gmem_init(void)
{
	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (!get_ipclite_feature(IPCLITE_GLOBAL_LOCK)) {
		synx_hwlock = hwspin_lock_request_specific(SYNX_HWSPIN_ID);
		if (!synx_hwlock) {
			dprintk(SYNX_ERR, "hwspinlock request failed\n");
			return -SYNX_NOMEM;
		}
	}

	/* zero idx not allocated for clients */
	ipclite_global_test_and_set_bit(0,
		(ipclite_atomic_uint32_t *)synx_gmem.bitmap);
	memset(&synx_gmem.table[0], 0, glcoredata_size);

	return SYNX_SUCCESS;
}

int synx_global_free_synx_hwlock(void)
{

	if (!get_ipclite_feature(IPCLITE_GLOBAL_LOCK)) {
		if (!synx_hwlock) {
			dprintk(SYNX_ERR, "hwspinlock is NULL\n");
			return -SYNX_INVALID;
		}
		hwspin_lock_free(synx_hwlock);
	}

	return SYNX_SUCCESS;
}

u32 synx_global_map_core_id(enum synx_core_id id)
{
	u32 host_id;

	switch (id) {
	case SYNX_CORE_APSS:
		host_id = IPCMEM_APPS; break;
	case SYNX_CORE_NSP:
		host_id = IPCMEM_CDSP; break;
	case SYNX_CORE_IRIS:
		host_id = IPCMEM_VPU; break;
	case SYNX_CORE_EVA:
		host_id = IPCMEM_CVP; break;
	case SYNX_CORE_ICP:
		host_id = IPCMEM_CAM; break;
	case SYNX_CORE_SOCCP:
		host_id = IPCMEM_SOCCP; break;
	case SYNX_CORE_ICP1:
		host_id = IPCMEM_CAM1; break;
	case SYNX_CORE_GMU:
		host_id = IPCMEM_GPU; break;
	default:
		host_id = IPCMEM_NUM_HOSTS;
		dprintk(SYNX_ERR, "invalid core id\n");
	}

	return host_id;
}

int synx_global_memory_is_empty(void)
{
	u32 index = 0;
	const u32 size = SYNX_GLOBAL_MAX_OBJS;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table)
		return -SYNX_NOMEM;

	index = find_next_bit((unsigned long *)synx_gmem.bitmap,
			size, index + 1);

	if (index >= size)
		return SYNX_SUCCESS;

	while (index < size) {
		dprintk(SYNX_MEM, "global index being used %d\n", index);
		synx_g_obj = synx_fetch_global_coredata_object(index);
		synx_global_print_data(synx_g_obj, __func__);
		index = find_next_bit((unsigned long *)(synx_gmem.bitmap),
				size, index + 1);
	}

	return -SYNX_INVALID;
}

int synx_global_alloc_index(u32 *idx)
{
	int rc = SYNX_SUCCESS;
	u32 prev, index;
	const u32 size = SYNX_GLOBAL_MAX_OBJS;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (IS_ERR_OR_NULL(idx)) {
		dprintk(SYNX_ERR, "invalid idx\n");
		return -SYNX_INVALID;
	}

	do {
		index = find_first_zero_bit((unsigned long *)synx_gmem.bitmap, size);
		if (index >= size) {
			rc = -SYNX_NOMEM;
			break;
		}
		prev = ipclite_global_test_and_set_bit(index % 32,
				(ipclite_atomic_uint32_t *)(synx_gmem.bitmap + index/32));
		if ((prev & (1UL << (index % 32))) == 0) {
			*idx = index;
			dprintk(SYNX_MEM, "allocated global idx %u\n", *idx);
			break;
		}
	} while (true);

	return rc;
}

int synx_global_init_coredata(u32 h_synx, u64 security_key)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;
	u32 idx = h_synx & SYNX_HANDLE_INDEX_MASK;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	if (synx_g_obj->status != 0 || synx_g_obj->refcount != 0 ||
		synx_g_obj->subscribers != 0 || synx_g_obj->handle != 0 ||
		synx_g_obj->parents[0] != 0) {
		dprintk(SYNX_ERR,
				"entry not cleared for idx %u,\n"
				"synx_g_obj->status %d,\n"
				"synx_g_obj->refcount %d,\n"
				"synx_g_obj->subscribers %d,\n"
				"synx_g_obj->handle %u,\n"
				"synx_g_obj->parents[0] %d\n",
				idx, synx_g_obj->status,
				synx_g_obj->refcount,
				synx_g_obj->subscribers,
				synx_g_obj->handle,
				synx_g_obj->parents[0]);
		synx_gmem_unlock(idx, &flags);
		return -SYNX_INVALID;
	}
#if defined(CONFIG_EXTENSIBLE_GLCOREDATA)
	if (synx_g_obj->security_key != 0) {
		dprintk(SYNX_ERR, "Security key not cleared for idx %u\n", idx);
		synx_gmem_unlock(idx, &flags);
		return -SYNX_INVALID;
	}
#endif
	memset(synx_g_obj, 0, glcoredata_size);
	/* set status to active */
	synx_g_obj->status = SYNX_STATE_ACTIVE;
	synx_g_obj->refcount = 1;
	synx_g_obj->subscribers = (1UL << SYNX_CORE_APSS);
	synx_g_obj->handle = h_synx;
#if defined(CONFIG_EXTENSIBLE_GLCOREDATA)
	if (security_key)
		synx_g_obj->security_key = security_key;
#endif
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

static int synx_global_get_waiting_cores_locked(
	struct synx_global_coredata *synx_g_obj,
	bool *cores)
{
	int i;

	synx_global_print_data(synx_g_obj, __func__);
	for (i = 0; i < SYNX_CORE_MAX; i++) {
		if (synx_g_obj->waiters & (1UL << i)) {
			cores[i] = true;
			dprintk(SYNX_VERB,
				"waiting for handle %u/n",
				synx_g_obj->handle);
		}
	}

	/* clear waiter list so signals are not repeated */
	synx_g_obj->waiters = 0;

	return SYNX_SUCCESS;
}

int synx_global_get_waiting_cores(u32 idx, bool *cores)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (IS_ERR_OR_NULL(cores) || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid, cores:%pK, idx:%u\n", cores, idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_get_waiting_cores_locked(synx_g_obj, cores);
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

int synx_global_set_waiting_core(u32 idx, enum synx_core_id id)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_g_obj->waiters |= (1UL << id);
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

int synx_global_get_subscribed_cores(u32 idx, bool *cores)
{
	int i;
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (IS_ERR_OR_NULL(cores) || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid, cores:%pK, idx:%u\n", cores, idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	for (i = 0; i < SYNX_CORE_MAX; i++)
		if (synx_g_obj->subscribers & (1UL << i))
			cores[i] = true;
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

int synx_global_fetch_handle_details(u32 idx, u32 *h_synx)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (IS_ERR_OR_NULL(h_synx) || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid, h_synx:%pK, idx:%u\n", h_synx, idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	*h_synx = synx_g_obj->handle;
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

int synx_global_set_subscribed_core(u32 idx, enum synx_core_id id)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_g_obj->subscribers |= (1UL << id);
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

int synx_global_clear_subscribed_core(u32 idx, enum synx_core_id id)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_g_obj->subscribers &= ~(1UL << id);
	synx_gmem_unlock(idx, &flags);

	return SYNX_SUCCESS;
}

u32 synx_global_get_parents_num(u32 idx)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;
	u32 i, count = 0;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return 0;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return 0;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++) {
		if (synx_g_obj->parents[i] != 0)
			count++;
	}
	synx_gmem_unlock(idx, &flags);

	return count;
}

static int synx_global_get_parents_locked(
	struct synx_global_coredata *synx_g_obj, u32 *parents)
{
	u32 i;

	if (!synx_g_obj || !parents) {
		dprintk(SYNX_ERR, "invalid synx g_obj or invalid parents\n");
		return -SYNX_NOMEM;
	}

	for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++)
		parents[i] = synx_g_obj->parents[i];

	return SYNX_SUCCESS;
}

int synx_global_get_parents(u32 idx, u32 *parents)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table || !parents) {
		dprintk(SYNX_ERR, "synx_gmem is NULL or invalid parents\n");
		return -SYNX_NOMEM;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	rc = synx_global_get_parents_locked(synx_g_obj, parents);
	synx_gmem_unlock(idx, &flags);

	return rc;
}

u32 synx_global_get_status(u32 idx)
{
	int rc;
	unsigned long flags;
	u32 status = SYNX_STATE_ACTIVE;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return 0;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return 0;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	if (synx_g_obj->status != SYNX_STATE_ACTIVE && synx_g_obj->num_child == 0)
		status = synx_g_obj->status;
	synx_gmem_unlock(idx, &flags);

	return status;
}

u32 synx_global_test_status_set_wait(u32 idx,
	enum synx_core_id id)
{
	int rc;
	unsigned long flags;
	u32 status;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return 0;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return 0;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return 0;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_print_data(synx_g_obj, __func__);
	status = synx_g_obj->status;
	/* if handle is still ACTIVE */
	if (status == SYNX_STATE_ACTIVE || synx_g_obj->num_child != 0) {
		synx_g_obj->waiters |= (1UL << id);
		status = SYNX_STATE_ACTIVE;
	}
	else
		dprintk(SYNX_DBG, "handle %u already signaled %u",
			synx_g_obj->handle, synx_g_obj->status);
	synx_gmem_unlock(idx, &flags);

	return status;
}

int synx_global_test_status_set_parent_child_wait(u32 idx,
	enum synx_core_id id)
{
	int rc;
	unsigned long flags;
	u32 status;
	struct synx_global_coredata *synx_g_obj;
	u32 h_parents[SYNX_GLOBAL_MAX_PARENTS] = {0};
	u32 i;
	bool no_parent = true;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return 0;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return 0;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return 0;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_print_data(synx_g_obj, __func__);
	status = synx_g_obj->status;
	if (synx_g_obj->num_child != 0) {
		dprintk(SYNX_ERR,
			"composite handle cannot be directly marked as waiting client.");
		synx_gmem_unlock(idx, &flags);
		return -SYNX_INVALID;
	}

	if (status == SYNX_STATE_ACTIVE) {
		/*
		 * Currently if a handle has parent, then only on the parent is marked as waiter
		 * to avoid multiple interrupts for each children. A client waiting on child will
		 * get the signal only when parent gets signaled.
		 */
		synx_global_get_parents_locked(synx_g_obj, h_parents);

		// check if there is any non-zero value in h_parents
		for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++) {
			if (h_parents[i] != 0) {
				no_parent = false;
				break;
			}
		}

		if (no_parent) {
			synx_g_obj->waiters |= (1UL << id);
		} else {
			synx_gmem_unlock(idx, &flags);
			for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++) {
				if (h_parents[i] != 0) {
					dprintk(SYNX_DBG, "Setting waiter for parent idx %d\n",
						h_parents[i]);
					synx_global_set_waiting_core(h_parents[i], id);
				}
			}
			return status;
		}
	} else
		dprintk(SYNX_DBG, "handle %u already signaled %u\n",
			synx_g_obj->handle, synx_g_obj->status);
	synx_gmem_unlock(idx, &flags);

	return status;
}

int synx_global_update_status_core(u32 idx,
	u32 status, bool is_recursion)
{
	u32 i, p_idx;
	int rc;
	bool clear = false;
	unsigned long flags;
	uint64_t data;
	struct synx_global_coredata *synx_g_obj;
	u32 h_parents[SYNX_GLOBAL_MAX_PARENTS] = {0};
	bool wait_cores[SYNX_CORE_MAX] = {false};

	if (!synx_gmem.table)
		return -SYNX_NOMEM;

	if (!synx_is_valid_idx(idx) || status <= SYNX_STATE_ACTIVE)
		return -SYNX_INVALID;

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_print_data(synx_g_obj, __func__);
	/* prepare for cross core signaling */
	data = synx_g_obj->handle;
	data <<= 32;
	if (synx_g_obj->num_child != 0) {
		/* composite handle cannot be signaled directly*/
		if (!is_recursion) {
			synx_gmem_unlock(idx, &flags);
			return -SYNX_INVALID;
		}
		/* composite handle */
		synx_g_obj->num_child--;
		if (synx_g_obj->status == SYNX_STATE_ACTIVE ||
			(status > SYNX_STATE_SIGNALED_SUCCESS &&
			status <= SYNX_STATE_SIGNALED_MAX))
			synx_g_obj->status = status;

		if (synx_g_obj->num_child == 0) {
			data |= synx_g_obj->status;
			synx_global_get_waiting_cores_locked(synx_g_obj,
				wait_cores);
			synx_global_get_parents_locked(synx_g_obj, h_parents);

			/* release ref held by constituting handles */
			synx_g_obj->refcount--;
			if (synx_g_obj->refcount == 0) {
				memset(synx_g_obj, 0,
					glcoredata_size);
				clear = true;
			}
		} else {
			/* pending notification from  handles */
			data = 0;
			dprintk(SYNX_DBG,
				"Child notified parent handle %u, pending %u\n",
				synx_g_obj->handle, synx_g_obj->num_child);
		}
	} else {
		if (synx_g_obj->status != SYNX_STATE_ACTIVE) {
			synx_gmem_unlock(idx, &flags);
			return -SYNX_ALREADY;
		}

		synx_g_obj->status = status;
		data |= synx_g_obj->status;
		synx_global_get_waiting_cores_locked(synx_g_obj,
			wait_cores);
		synx_global_get_parents_locked(synx_g_obj, h_parents);
	}
	synx_gmem_unlock(idx, &flags);

	if (clear) {
		ipclite_global_test_and_clear_bit(idx%32,
			(ipclite_atomic_uint32_t *)(synx_gmem.bitmap + idx/32));
		dprintk(SYNX_MEM,
			"cleared global idx %u\n", idx);
	}

	/* notify waiting clients on signal */
	if (data) {
		/* notify wait client */

	/* In case of SSR, someone might be waiting on same core
	 * However, in other cases, synx_signal API will take care
	 * of signaling handles on same core and thus we don't need
	 * to send interrupt
	 */
		if (status == SYNX_STATE_SIGNALED_SSR)
			i = 0;
		else
			i = 1;

		for (; i < SYNX_CORE_MAX ; i++) {
			if (!wait_cores[i])
				continue;
			dprintk(SYNX_DBG,
				"invoking ipc signal handle %u, status %u\n",
				synx_g_obj->handle, synx_g_obj->status);
			if (ipclite_msg_send(
				synx_global_map_core_id(i),
				data))
				dprintk(SYNX_ERR,
					"ipc signaling %llu to core %u failed\n",
					data, i);
		}
	}

	/* handle parent notifications */
	for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++) {
		p_idx = h_parents[i];
		if (p_idx == 0)
			continue;
		synx_global_update_status_core(p_idx, status, true);
	}

	return SYNX_SUCCESS;
}

int synx_global_update_status(u32 idx, u32 status)
{
	int rc = -SYNX_INVALID;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (!synx_is_valid_idx(idx) || status <= SYNX_STATE_ACTIVE) {
		dprintk(SYNX_ERR, "signaling with wrong status:%u or invalid idx:%u\n",
			status, idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	if (synx_g_obj->num_child != 0) {
		/* composite handle cannot be signaled */
		goto fail;
	} else if (synx_g_obj->status != SYNX_STATE_ACTIVE) {
		rc = -SYNX_ALREADY;
		goto fail;
	}
	synx_gmem_unlock(idx, &flags);

	return synx_global_update_status_core(idx, status, false);

fail:
	synx_gmem_unlock(idx, &flags);
	return rc;
}

int synx_global_get_ref(u32 idx)
{
	int rc;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_print_data(synx_g_obj, __func__);
	if (synx_g_obj->handle && synx_g_obj->refcount)
		synx_g_obj->refcount++;
	else
		rc = -SYNX_NOENT;
	synx_gmem_unlock(idx, &flags);

	return rc;
}

void synx_global_put_ref(u32 idx)
{
	int rc;
	bool clear = false;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return;
	}

	if (!synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_g_obj->refcount--;
	if (synx_g_obj->refcount == 0) {
		memset(synx_g_obj, 0, glcoredata_size);
		clear = true;
	}
	synx_gmem_unlock(idx, &flags);

	if (clear) {
		ipclite_global_test_and_clear_bit(idx%32,
			(ipclite_atomic_uint32_t *)(synx_gmem.bitmap + idx/32));
		dprintk(SYNX_MEM, "cleared global idx %u\n", idx);
	}
}

int synx_global_merge(u32 *idx_list, u32 num_list, u32 p_idx)
{
	int rc = -SYNX_INVALID;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;
	u32 i, j = 0;
	u32 idx;
	u32 num_child_signaled = 0;
	u32 parent_status = SYNX_STATE_ACTIVE;
	int err = SYNX_SUCCESS;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (!synx_is_valid_idx(p_idx)) {
		dprintk(SYNX_ERR, "invalid p_idx:%u\n", p_idx);
		return -SYNX_INVALID;
	}

	if (num_list == 0)
		return SYNX_SUCCESS;

	rc = synx_gmem_lock(p_idx, &flags);
	if (rc)
		return rc;
	synx_g_obj = synx_fetch_global_coredata_object(p_idx);
	if (synx_g_obj->handle && synx_g_obj->refcount) {
		synx_g_obj->num_child += num_list;
		synx_g_obj->refcount++;
	}
	synx_gmem_unlock(p_idx, &flags);

	while (j < num_list) {
		idx = idx_list[j];

		if (!synx_is_valid_idx(idx)) {
			dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
			goto fail;
		}

		rc = synx_gmem_lock(idx, &flags);
		if (rc) {
			dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
			goto fail;
		}

		synx_g_obj = synx_fetch_global_coredata_object(idx);
		for (i = 0; i < SYNX_GLOBAL_MAX_PARENTS; i++) {
			if (synx_g_obj->parents[i] == 0) {
				synx_g_obj->parents[i] = p_idx;
				break;
			}
		}

		if (synx_g_obj->status != SYNX_STATE_ACTIVE) {
			if (synx_g_obj->num_child == 0)
				num_child_signaled += 1;
			if (synx_g_obj->status >
				SYNX_STATE_SIGNALED_SUCCESS &&
				synx_g_obj->status <= SYNX_STATE_SIGNALED_MAX)
				parent_status = synx_g_obj->status;
			else if (parent_status == SYNX_STATE_ACTIVE)
				parent_status = synx_g_obj->status;
		}

		dprintk(SYNX_MEM, "synx_obj->status %d parent status %d\n",
			synx_g_obj->status, parent_status);
		synx_gmem_unlock(idx, &flags);

		if (i >= SYNX_GLOBAL_MAX_PARENTS) {
			rc = -SYNX_NOMEM;
			dprintk(SYNX_ERR, "Number of parents exceeded the limit for handle %u\n",
				synx_g_obj->handle);
			goto fail;
		}

		j++;
	}

	rc = synx_gmem_lock(p_idx, &flags);
	if (rc)
		goto fail;
	synx_g_obj = synx_fetch_global_coredata_object(p_idx);
	synx_g_obj->num_child -= num_child_signaled;
	if (synx_g_obj->num_child == 0 && num_child_signaled)
		synx_g_obj->refcount -= 1;
	if (synx_g_obj->status == SYNX_STATE_ACTIVE ||
		((parent_status > SYNX_STATE_SIGNALED_SUCCESS &&
		parent_status <= SYNX_STATE_SIGNALED_MAX) &&
		!(synx_g_obj->status > SYNX_STATE_SIGNALED_SUCCESS &&
		synx_g_obj->status <= SYNX_STATE_SIGNALED_MAX)))
		synx_g_obj->status = parent_status;
	synx_global_print_data(synx_g_obj, __func__);
	synx_gmem_unlock(p_idx, &flags);

	return SYNX_SUCCESS;

fail:
	err = synx_gmem_lock(p_idx, &flags);
	if (err)
		return err;

	synx_g_obj = synx_fetch_global_coredata_object(p_idx);
	synx_g_obj->num_child -= (num_child_signaled + (num_list - j));
	synx_g_obj->status = SYNX_STATE_SIGNALED_ERROR;
	if (synx_g_obj->num_child == 0)
		synx_g_obj->refcount -= 1;

	synx_gmem_unlock(p_idx, &flags);

	return rc;
}

int synx_global_recover_interop(enum synx_core_id core_id,
	struct synx_hwfence_interops *hwfence_shared_ops)
{
	int rc = SYNX_SUCCESS;
	u32 idx = 0;
	const u32 size = SYNX_GLOBAL_MAX_OBJS;
	unsigned long flags;
	struct synx_global_coredata *synx_g_obj;
	uint32_t h_hwfence = 0;
	uint16_t waiting_cores = 0;
	bool update;
	int *clear_idx = NULL;
	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	clear_idx = kzalloc(sizeof(int)*SYNX_GLOBAL_MAX_OBJS, GFP_KERNEL);

	if (!clear_idx) {
		dprintk(SYNX_ERR, "clear_idx allocation failed\n");
		return -SYNX_NOMEM;
	}

	ipclite_recover(synx_global_map_core_id(core_id));

	/* recover synx gmem lock if it was owned by core in ssr */
	if (get_ipclite_feature(IPCLITE_GLOBAL_LOCK)) {
		for (int idx = 0; idx < IPCLITE_MAX_GLOBAL_LOCK; idx++)
			ipclite_global_spin_bust(idx, synx_global_map_core_id(core_id));
	} else if (synx_gmem_lock_owner(0) == core_id) {
		synx_gmem_lock_owner_clear(0);
		hwspin_unlock_raw(synx_hwlock);
	}

	idx = find_next_bit((unsigned long *)synx_gmem.bitmap,
			size, idx + 1);
	while (idx < size) {
		update = false;
		rc = synx_gmem_lock(idx, &flags);
		if (rc)
			goto free;
		synx_g_obj = synx_fetch_global_coredata_object(idx);
		if (synx_g_obj->refcount &&
			 synx_g_obj->subscribers & (1UL << core_id)) {
			synx_g_obj->subscribers &= ~(1UL << core_id);
			synx_g_obj->refcount--;
			h_hwfence = synx_g_obj->h_hwfence;
			waiting_cores = synx_g_obj->waiters;

			if (synx_g_obj->refcount == 0) {
				memset(synx_g_obj, 0, glcoredata_size);
				clear_idx[idx] = 1;
			} else if (synx_g_obj->status == SYNX_STATE_ACTIVE) {
				update = true;
			}
		}
		synx_gmem_unlock(idx, &flags);
		if (update) {
			synx_global_update_status_core(idx,
				SYNX_STATE_SIGNALED_SSR, false);

			if (core_id == SYNX_CORE_SOCCP &&
				(waiting_cores & (1UL << core_id)) &&
				!IS_ERR_OR_NULL(hwfence_shared_ops) &&
				!IS_ERR_OR_NULL(hwfence_shared_ops->signal_fence)) {
				hwfence_shared_ops->signal_fence(core_id, true, h_hwfence,
					SYNX_STATE_SIGNALED_SSR);
			}
		}
		idx = find_next_bit((unsigned long *)synx_gmem.bitmap,
				size, idx + 1);
	}

	for (idx = 1; idx < size; idx++) {
		if (clear_idx[idx]) {
			ipclite_global_test_and_clear_bit(idx % 32,
				(ipclite_atomic_uint32_t *)(synx_gmem.bitmap + idx/32));
			dprintk(SYNX_MEM, "released global idx %u\n", idx);
		}
	}
free:
	kfree(clear_idx);

	return rc;

}

int synx_global_recover(enum synx_core_id core_id)
{
	return synx_global_recover_interop(core_id, NULL);
}

int synx_global_test_status_update_coredata(u32 idx,
	enum synx_core_id id, u32 h_hwfence,
	bool is_waiter)
{
	int rc;
	unsigned long flags;
	u32 status;
	struct synx_global_coredata *synx_g_obj;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc) {
		dprintk(SYNX_ERR, "Failed to lock entry %u\n", idx);
		return rc;
	}
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	synx_global_print_data(synx_g_obj, __func__);
	status = synx_g_obj->status;
	/* if handle is still ACTIVE */
	if (status == SYNX_STATE_ACTIVE || synx_g_obj->num_child != 0) {
		synx_g_obj->refcount++;
		synx_g_obj->subscribers |= (1UL << id);
		if (is_waiter)
			synx_g_obj->waiters |= (1UL << id);
		synx_g_obj->h_hwfence = h_hwfence;
		status = SYNX_STATE_ACTIVE;
	}
	else
		dprintk(SYNX_DBG, "handle %u already signaled %u",
			synx_g_obj->handle, synx_g_obj->status);
	synx_gmem_unlock(idx, &flags);

	return status;
}

int synx_global_recover_index(enum synx_core_id core_id, bool global_unlock,
	u32 idx, u32 status)
{
	int rc = SYNX_SUCCESS;
	unsigned long flags;
	u32 state = SYNX_STATE_INVALID;
	struct synx_global_coredata *synx_g_obj;
	bool clear = false;
	bool update = false;
	uint32_t h_synx, h_hwfence;

	if (!synx_gmem.table) {
		dprintk(SYNX_ERR, "synx_gmem is NULL\n");
		return -SYNX_NOMEM;
	}

	if (core_id >= SYNX_CORE_MAX || !synx_is_valid_idx(idx)) {
		dprintk(SYNX_ERR, "invalid idx:%u\n", idx);
		return -SYNX_INVALID;
	}

	// Incase if SOCCP might have taken hw_mutex before crash
	/* recover synx gmem lock if it was owned by core in ssr */
	if (get_ipclite_feature(IPCLITE_GLOBAL_LOCK) && global_unlock)
		ipclite_global_spin_bust(idx, synx_global_map_core_id(core_id));
	else if (global_unlock && synx_gmem_lock_owner(0) == core_id) {
		synx_gmem_lock_owner_clear(0);
		hwspin_unlock_raw(synx_hwlock);
	}

	rc = synx_gmem_lock(idx, &flags);
	if (rc)
		return rc;
	synx_g_obj = synx_fetch_global_coredata_object(idx);
	if (synx_g_obj->refcount &&
			synx_g_obj->subscribers & (1UL << core_id)) {
		synx_g_obj->subscribers &= ~(1UL << core_id);
		synx_g_obj->refcount--;
		state = synx_g_obj->status;
		h_synx = synx_g_obj->handle;
		h_hwfence = synx_g_obj->h_hwfence;

		if (synx_g_obj->refcount == 0) {
			memset(synx_g_obj, 0, glcoredata_size);
			clear = true;
		} else if (synx_g_obj->status == SYNX_STATE_ACTIVE)
			update = true;
	}
	synx_gmem_unlock(idx, &flags);

	if (update)
		rc = synx_global_update_status_core(idx, status, false);
	else if (clear)	{
		ipclite_global_test_and_clear_bit(idx % 32,
			(ipclite_atomic_uint32_t *)(synx_gmem.bitmap + idx/32));
		dprintk(SYNX_MEM, "released global idx %u\n", idx);
	}

	dprintk(SYNX_DBG, "h_synx %u, h_hwfence %u, curr state %u, status %u",
		h_synx, h_hwfence, state, status);

	return rc;
}

int synx_global_mem_init(void)
{
	int rc;
	int bitmap_size = SYNX_GLOBAL_MAX_OBJS/32;
	struct global_region_info mem_info;
	void *gl_coredata = NULL;

	rc = get_global_partition_info(&mem_info);
	if (rc) {
		dprintk(SYNX_ERR, "error setting up global shared memory\n");
		return rc;
	}

	memset(mem_info.virt_base, 0, mem_info.size);
	dprintk(SYNX_DBG, "global shared memory %pK size %u\n",
		mem_info.virt_base, mem_info.size);

	synx_gmem.bitmap = (u32 *)mem_info.virt_base;
	synx_gmem.locks = synx_gmem.bitmap + bitmap_size;
	synx_gmem.table =
		(struct synx_global_coredata *)(synx_gmem.locks + 2);
	dprintk(SYNX_DBG, "global memory bitmap %pK, table %pK\n",
		synx_gmem.bitmap, synx_gmem.table);
	glcoredata_size = (mem_info.custom_value == 0) ?
		sizeof(struct synx_global_coredata) : mem_info.custom_value;
	dprintk(SYNX_DBG, "Global coredata size is %u\n", glcoredata_size);

	gl_coredata = synx_fetch_global_coredata_object(SYNX_GLOBAL_MAX_OBJS);
	if ((uintptr_t)gl_coredata > (uintptr_t)((uint8_t *)mem_info.virt_base + mem_info.size)) {
		dprintk(SYNX_ERR, "Global coredata outside of shared mem limits");
		return -SYNX_INVALID;
	}

	return synx_gmem_init();
}
