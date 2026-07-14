// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/gunyah/gh_errno.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/gunyah/gh_common.h>
#include <linux/gunyah/gh_rm_drv.h>
#include <soc/qcom/secure_buffer.h>
#include <linux/sizes.h>
#include "gh_rm_drv_private.h"
#include "mem-prot.h"

/* heap mem parcel flags */
#define RM_HEAP_MEM_ADDED	((uint8_t)0x1)
#define RM_HEAP_MEM_ASSIGNED	((uint8_t)0x2)
#define RM_HEAP_MEM_POOL_ALLOC	((uint8_t)0x4)
#define RM_HEAP_MEM_NEEDS_CLEANUP	((uint8_t)0x8)

/* heap labels */
#define HYP_ROOT_HEAP_LABEL	0x0
#define RM_HEAP_LABEL		0x1

bool rm_heap_manager_enabled;
static bool hyp_heap_manage_supported;

static DEFINE_MUTEX(rm_heap_lock);

/*
 * for every 1GB of dmabuf, it would need 5MB of Root Heap memory
 * hence, for every 4KB of dmabuf, it would need 20 Bytes of Root Heap memory
 */
#define ROOT_HEAP_BYTES_PER_4K	20

/*
 * for every 1GB of dmabuf, it would need 3MB of RM Heap memory
 * hence, for every 4KB of dmabuf, it would need 12 Bytes of RM Heap memory
 */
#define RM_HEAP_BYTES_PER_4K	12

/* minimum heap memory that we can add */
#define HEAP_MIN_RESOLUTION	(SZ_4M)

struct rm_heap_mem_parcel {
	gh_memparcel_handle_t mp_handle;
	size_t size;
	uint8_t flags;
	phys_addr_t phys_addr;
	struct list_head list;
};

struct rm_heap_manager {
	int rm_vmid;

	gh_heap_handle_t heap_handle[GH_HYP_HEAP_OBJ_MAX];
	u32 num_parcels[GH_HYP_HEAP_OBJ_MAX];
	unsigned long total_heap_added[GH_HYP_HEAP_OBJ_MAX];
	struct list_head list_mp_handles[GH_HYP_HEAP_OBJ_MAX];
};

static struct rm_heap_manager *rm_heap_manager;

/* get the total hypervisor heap memory added by hlos */
uint64_t gh_rm_heap_get_total_heap_added(void)
{
	if (!rm_heap_manager_enabled)
		return 0;

	return rm_heap_manager->total_heap_added[GH_HYP_HEAP_ROOT] +
			rm_heap_manager->total_heap_added[GH_HYP_HEAP_RM];
}
EXPORT_SYMBOL_GPL(gh_rm_heap_get_total_heap_added);

/* gets the exact heap size required for given dmabuf size. no rounding up! */
static uint64_t __get_dmabuf_heap_size(size_t dmabuf_size, enum gh_hyp_heap_label label)
{
	uint64_t num_4k = dmabuf_size / SZ_4K;

	if (label == GH_HYP_HEAP_ROOT)
		return num_4k * ROOT_HEAP_BYTES_PER_4K;
	else if (label == GH_HYP_HEAP_RM)
		return num_4k * RM_HEAP_BYTES_PER_4K;
	else
		return 0;
}

static inline char *get_heap_object_name(enum gh_hyp_heap_label label)
{
	if (label == GH_HYP_HEAP_ROOT)
		return "Root heap";
	else if (label == GH_HYP_HEAP_RM)
		return "RM heap";
	else
		return "";
}
/* returns size in bytes */
static uint64_t gh_get_rm_heap_free(enum gh_hyp_heap_label label)
{
	struct gh_mem_heap_query_resp_stats_payload *heap_info;
	int ret;
	size_t resp_size;
	uint64_t total_size, allocated_size;
	uint64_t rm_heap_free;

	if (!rm_heap_manager_enabled)
		return 0;

	ret = gh_rm_heap_query(rm_heap_manager->heap_handle[label],
			GH_RM_HEAP_QUERY_TYPE_MEM, (void *)&heap_info, &resp_size);
	if (ret < 0) {
		pr_err("failed to get heap QUERY for %s ret: %d\n",
				get_heap_object_name(label), ret);
		return 0;
	}

	total_size = heap_info->total_size;
	allocated_size = heap_info->allocated_size;

	rm_heap_free = total_size - allocated_size;

	if (resp_size)
		kfree(heap_info);

	return rm_heap_free;
}

/* caller should take the rm_heap_lock lock */
static int __remove_heap_parcel(struct rm_heap_mem_parcel *heap_parcel,
		enum gh_hyp_heap_label label)
{
	int ret = 0;

	if (heap_parcel->flags & RM_HEAP_MEM_ADDED) {
		ret = gh_rm_remove_heap_memory(rm_heap_manager->heap_handle[label],
				heap_parcel->mp_handle);
		if (ret < 0)
			return ret;

		heap_parcel->flags &= ~RM_HEAP_MEM_ADDED;
	}

	if (heap_parcel->flags & RM_HEAP_MEM_ASSIGNED) {
		ret = ghd_rm_mem_reclaim(heap_parcel->mp_handle, 0);
		if (ret) {
			pr_err("ghd_rm_mem_reclaim failed for %s error: %d\n",
					get_heap_object_name(label), ret);
			heap_parcel->flags |= RM_HEAP_MEM_NEEDS_CLEANUP;
			return ret;
		}
		heap_parcel->flags &= ~RM_HEAP_MEM_ASSIGNED;
	}

	if (heap_parcel->flags & RM_HEAP_MEM_POOL_ALLOC) {
		ret = mem_prot_pool_free(heap_parcel->phys_addr, heap_parcel->size,
				CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE);
		if (ret) {
			pr_err("mem_prot_pool_free failed for %s: %d\n",
					get_heap_object_name(label), ret);
			heap_parcel->flags |= RM_HEAP_MEM_NEEDS_CLEANUP;
			return ret;
		}
		heap_parcel->flags &= ~RM_HEAP_MEM_POOL_ALLOC;
	}

	/* all removal done. we can remove from list now */
	list_del(&heap_parcel->list);
	rm_heap_manager->num_parcels[label]--;
	rm_heap_manager->total_heap_added[label] -= heap_parcel->size;
	kfree(heap_parcel);

	return ret;
}

static int __remove_heap_handle(enum gh_hyp_heap_label label, gh_memparcel_handle_t mp_handle)
{
	struct rm_heap_mem_parcel *heap_parcel = NULL;
	int found = 0;
	int ret = -EINVAL;

	mutex_lock(&rm_heap_lock);
	list_for_each_entry(heap_parcel, &rm_heap_manager->list_mp_handles[label], list)
		if (heap_parcel->mp_handle == mp_handle) {
			found = 1;
			break;
		}

	if (!found)
		goto out;

	ret = __remove_heap_parcel(heap_parcel, label);
out:
	mutex_unlock(&rm_heap_lock);

	return ret;
}

static void gh_rm_remove_all_free_heap(enum gh_hyp_heap_label label)
{
	struct gh_mem_heap_query_resp_mem_parcels_payload  *heap_info = NULL;
	int ret, i;
	size_t resp_size;
	u32 n_mp_handles;
	gh_memparcel_handle_t *memparcel_handles;

	ret = gh_rm_heap_query(rm_heap_manager->heap_handle[label],
			GH_RM_HEAP_QUERY_TYPE_MP, (void *)&heap_info, &resp_size);
	if (ret < 0) {
		pr_err("failed to get heap QUERY for %s ret: %d\n",
				get_heap_object_name(label), ret);
		return;
	}

	n_mp_handles = heap_info->n_mp_handles;
	memparcel_handles = heap_info->memparcel_handles;

	for (i = 0; i < n_mp_handles; i++) {
		ret = __remove_heap_handle(rm_heap_manager->heap_handle[label],
				memparcel_handles[i]);
		if (ret)
			pr_err("failed to remove mp handle %d\n", memparcel_handles[i]);
	}

	if (resp_size)
		kfree(heap_info);

	pr_debug("%s total_heap_added: 0x%lx num_parcels: %d\n",
			get_heap_object_name(label),
			rm_heap_manager->total_heap_added[label],
			rm_heap_manager->num_parcels[label]);
}

uint64_t gh_rm_heap_get_hyp_heap_free(void)
{
	if (!rm_heap_manager_enabled)
		return 0;

	return gh_get_rm_heap_free(GH_HYP_HEAP_ROOT) +
			gh_get_rm_heap_free(GH_HYP_HEAP_RM);
}
EXPORT_SYMBOL_GPL(gh_rm_heap_get_hyp_heap_free);

/* size in bytes */
static int gh_rm_heap_add_chunk(size_t size, enum gh_hyp_heap_label label)
{
	int ret, ret2 = 0;
	u8 perm = PERM_READ | PERM_WRITE | PERM_EXEC;
	u16 dst_vmid =  rm_heap_manager->rm_vmid;
	struct rm_heap_mem_parcel *heap_parcel;
	phys_addr_t prot_phys;
	struct gh_acl_desc *acl_desc;
	struct gh_sgl_desc *sgl_desc;
	gh_memparcel_handle_t mp_handle;

	if (!IS_ALIGNED(size, HEAP_MIN_RESOLUTION)) {
		pr_err("size 0x%zx should be aligned to 1MB\n", size);
		return -EINVAL;
	}

	heap_parcel = kzalloc(sizeof(*heap_parcel), GFP_KERNEL);
	if (!heap_parcel)
		return -ENOMEM;

	INIT_LIST_HEAD(&heap_parcel->list);

	ret = mem_prot_pool_alloc(size, &prot_phys,
			CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE);
	if (ret) {
		pr_err("mem_prot_pool_alloc failed for %s: %d\n",
				get_heap_object_name(label), ret);
		goto err_mem_prot;
	}
	heap_parcel->flags |= RM_HEAP_MEM_POOL_ALLOC;

	acl_desc = kzalloc(offsetof(struct gh_acl_desc, acl_entries[1]),
				GFP_KERNEL);
	if (!acl_desc) {
		ret = -ENOMEM;
		goto err_acl_desc;
	}

	acl_desc->n_acl_entries = 1;
	acl_desc->acl_entries[0].vmid = dst_vmid;
	acl_desc->acl_entries[0].perms = perm;

	sgl_desc = kzalloc(offsetof(struct gh_sgl_desc, sgl_entries[1]),
				GFP_KERNEL);
	if (!sgl_desc) {
		ret = -ENOMEM;
		goto err_sgl_desc;
	}

	sgl_desc->n_sgl_entries = 1;
	sgl_desc->sgl_entries[0].ipa_base = prot_phys;
	sgl_desc->sgl_entries[0].size = size;

	ret = ghd_rm_mem_lend(GH_RM_MEM_TYPE_NORMAL, 0, 0,
			acl_desc, sgl_desc, NULL, &mp_handle);
	if (ret) {
		pr_err("ghd_rm_mem_lend failed for %s: %d\n",
				get_heap_object_name(label), ret);
		goto err_assign_mem;
	}
	heap_parcel->flags |= RM_HEAP_MEM_ASSIGNED;

	ret = gh_rm_add_heap_memory(rm_heap_manager->heap_handle[label], mp_handle);
	if (ret < 0) {
		pr_err("failed to add 0x%lx bytes memory to %s\n",
				size, get_heap_object_name(label));
		goto err_add_heap;
	}
	heap_parcel->flags |= RM_HEAP_MEM_ADDED;

	heap_parcel->mp_handle = mp_handle;
	heap_parcel->size = size;
	heap_parcel->phys_addr = prot_phys;

	mutex_lock(&rm_heap_lock);
	list_add(&heap_parcel->list, &rm_heap_manager->list_mp_handles[label]);
	rm_heap_manager->num_parcels[label]++;
	rm_heap_manager->total_heap_added[label] += size;
	mutex_unlock(&rm_heap_lock);

	kfree(acl_desc);
	kfree(sgl_desc);
	pr_debug("%s total_heap_added 0x%lx num_parcels %d\n",
			get_heap_object_name(label),
			rm_heap_manager->total_heap_added[label],
			rm_heap_manager->num_parcels[label]);

	return 0;

err_add_heap:
	ret2 = ghd_rm_mem_reclaim(mp_handle, 0);
	if (ret2)
		pr_err("ghd_rm_mem_reclaim failed in error path\n");
err_assign_mem:
	kfree(sgl_desc);
err_sgl_desc:
	kfree(acl_desc);
err_acl_desc:
	/* free mem_prot_pool only after mp_handle is unassigned */
	if (!ret2 && mem_prot_pool_free(prot_phys, size,
			CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE))
		pr_err("mem_prot_pool_free failed in error path\n");

err_mem_prot:
	kfree(heap_parcel);

	return ret;
}

/*
 * Prior unsuccessful calls to gh_rm_remove_all_free_heap may have left
 * parcels in an in-between state. Try again.
 */
static void remove_chunk_retry(void)
{
	struct rm_heap_mem_parcel *heap_parcel, *temp;

	mutex_lock(&rm_heap_lock);
	list_for_each_entry_safe(heap_parcel, temp,
			&rm_heap_manager->list_mp_handles[GH_HYP_HEAP_ROOT], list) {
		if (heap_parcel->flags & RM_HEAP_MEM_NEEDS_CLEANUP)
			__remove_heap_parcel(heap_parcel, GH_HYP_HEAP_ROOT);
	}

	list_for_each_entry_safe(heap_parcel, temp,
			&rm_heap_manager->list_mp_handles[GH_HYP_HEAP_RM], list) {
		if (heap_parcel->flags & RM_HEAP_MEM_NEEDS_CLEANUP)
			__remove_heap_parcel(heap_parcel, GH_HYP_HEAP_ROOT);
	}
	mutex_unlock(&rm_heap_lock);
}

/*
 * Query and reclaim unused chunks from the gunyah heap(s) back to hlos.
 * Caller responsible for handling races with operations which
 * consume heap memory.
 */
void gh_rm_heap_shrink(void)
{
	if (!rm_heap_manager_enabled)
		return;

	gh_rm_remove_all_free_heap(GH_HYP_HEAP_ROOT);
	gh_rm_remove_all_free_heap(GH_HYP_HEAP_RM);

	pr_debug("root heap free: 0x%llx rm heap free: 0x%llx\n",
			gh_get_rm_heap_free(GH_HYP_HEAP_ROOT),
			gh_get_rm_heap_free(GH_HYP_HEAP_RM));

	remove_chunk_retry();
}
EXPORT_SYMBOL_GPL(gh_rm_heap_shrink);

/*
 * Gunyah operations such as ghd_rm_mem_lend(@size) consume memory from the
 * gunyah heap. Estimate the heap size required and preallocate the
 * missing portion.
 * Caller should ensure rm heap size is not decreased until lend operation is complete.
 */
void gh_rm_heap_memlend_prealloc(size_t dmabuf_size)
{
	size_t heap_increase, heap_free;
	u16 i, heap, fail = 0, num_chunks;

	if (!rm_heap_manager_enabled)
		return;

	for (heap = 0; heap < GH_HYP_HEAP_OBJ_MAX; heap++, fail = 0) {

		heap_increase = __get_dmabuf_heap_size(dmabuf_size, heap);
		heap_free = gh_get_rm_heap_free(heap);

		pr_debug("%s: dmabuf_size 0x%zx heap_increase 0x%zx heap_free 0x%zx\n",
				get_heap_object_name(heap), dmabuf_size, heap_increase, heap_free);

		if (heap_increase > heap_free) {
			heap_increase -= heap_free;
			heap_increase = ALIGN(heap_increase, HEAP_MIN_RESOLUTION);
			num_chunks = heap_increase / HEAP_MIN_RESOLUTION;

			for (i = 0; i < num_chunks; i++)
				if (gh_rm_heap_add_chunk(HEAP_MIN_RESOLUTION, GH_HYP_HEAP_ROOT))
					fail++;

			if (fail)
				pr_err("failed to add %s memory. req size: 0x%zx fail count: %d\n",
				get_heap_object_name(heap), heap_increase, fail);
		}
	}
}
EXPORT_SYMBOL_GPL(gh_rm_heap_memlend_prealloc);

static int gh_rm_get_heap_resource_info(void)
{
	struct gh_vm_get_hyp_res_resp_entry *res_entries = NULL;
	u32 n_res, i;

	res_entries = gh_rm_vm_get_hyp_res(rm_heap_manager->rm_vmid, &n_res);
	if (IS_ERR_OR_NULL(res_entries)) {
		pr_err("get hyp resources failed.\n");
		return PTR_ERR(res_entries);
	}

	/* get the heap object resource info */
	for (i = 0; i < n_res; i++)
		if (res_entries[i].res_type == GH_RM_RES_TYPE_RM_HEAP_OBJECT) {
			switch (res_entries[i].resource_label) {
			case HYP_ROOT_HEAP_LABEL:
				rm_heap_manager->heap_handle[GH_HYP_HEAP_ROOT] =
						res_entries[i].resource_handle;
			break;
			case RM_HEAP_LABEL:
				rm_heap_manager->heap_handle[GH_HYP_HEAP_RM] =
						res_entries[i].resource_handle;
			}

			/*
			 * with resource type GH_RM_RES_TYPE_RM_HEAP_OBJECT present,
			 * we have the hypervisor support to manage its heap.
			 */
			if (!hyp_heap_manage_supported)
				hyp_heap_manage_supported = true;
		}

	kfree(res_entries);
	return 0;
}

static bool check_mem_prot_enabled(void)
{
	phys_addr_t prot_phys;

	if (mem_prot_pool_alloc(HEAP_MIN_RESOLUTION, &prot_phys,
			CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE))
		return false;

	/* mem_prot is enabled */
	if (mem_prot_pool_free(prot_phys, HEAP_MIN_RESOLUTION,
			CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE))
		pr_err("mem_prot_pool_free failed\n");

	return true;
}

static int __init gh_rm_heap_manager_init(void)
{
	int ret, i;

	rm_heap_manager = kzalloc(sizeof(*rm_heap_manager), GFP_KERNEL);
	if (!rm_heap_manager)
		return -ENOMEM;

	rm_heap_manager->rm_vmid = QCOM_SCM_VMID_GH_RM;

	for (i = 0; i < GH_HYP_HEAP_OBJ_MAX; i++)
		INIT_LIST_HEAD(&rm_heap_manager->list_mp_handles[i]);

	ret = gh_rm_get_heap_resource_info();
	if (ret < 0) {
		pr_err("failed to get hypervisor heap resources %d\n", ret);
		goto out_err;
	}

	if (!hyp_heap_manage_supported) {
		pr_err("hyp heap management not supported by Hypervisor\n");
		goto out_err;
	}

	if (!check_mem_prot_enabled()) {
		pr_err("memory protection feature is not supported\n");
		goto out_err;
	}

	rm_heap_manager_enabled = true;

	pr_info("free heap memory %s: 0x%llx %s: 0x%llx\n",
			get_heap_object_name(GH_HYP_HEAP_ROOT),
			gh_get_rm_heap_free(GH_HYP_HEAP_ROOT),
			get_heap_object_name(GH_HYP_HEAP_RM),
			gh_get_rm_heap_free(GH_HYP_HEAP_RM));

	return 0;

out_err:
	kfree(rm_heap_manager);
	return ret;
}
module_init(gh_rm_heap_manager_init);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. Gunyah RM Heap Manager Driver");
MODULE_LICENSE("GPL");
