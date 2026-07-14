/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef MEM_BUF_PRIVATE_H
#define MEM_BUF_PRIVATE_H

#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/mem-buf.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define MEM_BUF_MEMPARCEL_INVALID (U32_MAX)

#define MEM_BUF_CAP_SUPPLIER	BIT(0)
#define MEM_BUF_CAP_CONSUMER	BIT(1)
#define MEM_BUF_CAP_DUAL (MEM_BUF_CAP_SUPPLIER | MEM_BUF_CAP_CONSUMER)
extern unsigned char mem_buf_capability;
extern struct device *mem_buf_dev;
extern struct dentry *mem_buf_debugfs_root;

/*
 * @kref - A refcount for @sgt and @sgt's private data, which
 *	is expected to contain a 'struct mem_buf_vmperm'
 * @rcu - Usage based off of mm/shrinker.c
 */
struct mem_buf_vmperm {
	struct list_head list;
	u32 flags;
	int current_vm_perms;
	u32 mapcount;
	int *vmids;
	int *perms;
	unsigned int nr_acl_entries;
	unsigned int max_acl_entries;
	struct dma_buf *dmabuf;
	struct sg_table *sgt;
	size_t size;
	gh_memparcel_handle_t memparcel_hdl;
	struct mutex lock;
	mem_buf_dma_buf_destructor dtor;
	void *dtor_data;
	void (*kref_release)(struct kref *kref);
	struct kref *kref;
	struct rcu_head rcu;
};


int mem_buf_dma_buf_init(void);

/* Hypervisor Interface */
int mem_buf_assign_mem(u32 op, struct sg_table *sgt,
		       struct mem_buf_lend_kernel_arg *arg);
int mem_buf_unassign_mem(struct sg_table *sgt, int *src_vmids,
			 unsigned int nr_acl_entries,
			 gh_memparcel_handle_t hdl);
int mem_buf_hyp_assign_table(struct sg_table *sgt, u32 *src_vmid, int source_nelems,
			     int *dest_vmids, int *dest_perms, int dest_nelems);

#ifdef CONFIG_QCOM_MEM_BUF_DEV_GH
struct gh_acl_desc *mem_buf_vmid_perm_list_to_gh_acl(int *vmids, int *perms,
						     unsigned int nr_acl_entries);
struct gh_sgl_desc *mem_buf_sgt_to_gh_sgl_desc(struct sg_table *sgt);
int mem_buf_map_mem_s2(u32 op, gh_memparcel_handle_t *__memparcel_hdl,
			struct gh_acl_desc *acl_desc, struct gh_sgl_desc **sgl_desc,
			int src_vmid);
int mem_buf_unmap_mem_s2(gh_memparcel_handle_t memparcel_hdl);
int mem_buf_gh_acl_desc_to_vmid_perm_list(struct gh_acl_desc *acl_desc,
					  int **vmids, int **perms);
size_t mem_buf_get_sgl_buf_size(struct gh_sgl_desc *sgl_desc);
struct sg_table *dup_gh_sgl_desc_to_sgt(struct gh_sgl_desc *sgl_desc);
struct gh_sgl_desc *dup_gh_sgl_desc(struct gh_sgl_desc *sgl_desc);
int mem_buf_assign_mem_gunyah(u32 op, struct sg_table *sgt,
			      struct mem_buf_lend_kernel_arg *arg);
int mem_buf_unassign_mem_gunyah(gh_memparcel_handle_t memparcel_hdl);
#else
static inline struct gh_acl_desc *mem_buf_vmid_perm_list_to_gh_acl(int *vmids, int *perms,
								   unsigned int nr_acl_entries)
{
	return ERR_PTR(-EINVAL);
}

static inline struct gh_sgl_desc *mem_buf_sgt_to_gh_sgl_desc(struct sg_table *sgt)
{
	return ERR_PTR(-EINVAL);
}

static inline int mem_buf_map_mem_s2(u32 op, gh_memparcel_handle_t *__memparcel_hdl,
				     struct gh_acl_desc *acl_desc, struct gh_sgl_desc **__sgl_desc,
				     int src_vmid)
{
	return -EINVAL;
}

static inline int mem_buf_unmap_mem_s2(gh_memparcel_handle_t memparcel_hdl)
{
	return -EINVAL;
}

static inline int mem_buf_gh_acl_desc_to_vmid_perm_list(struct gh_acl_desc *acl_desc,
							int **vmids, int **perms)
{
	return -EINVAL;
}

static inline size_t mem_buf_get_sgl_buf_size(struct gh_sgl_desc *sgl_desc)
{
	return 0;
}

static inline struct sg_table *dup_gh_sgl_desc_to_sgt(struct gh_sgl_desc *sgl_desc)
{
	return ERR_PTR(-EINVAL);
}

static inline struct gh_sgl_desc *dup_gh_sgl_desc(struct gh_sgl_desc *sgl_desc)
{
	return ERR_PTR(-EINVAL);
}

static inline int mem_buf_assign_mem_gunyah(u32 op, struct sg_table *sgt,
					    struct mem_buf_lend_kernel_arg *arg)
{
	return 0;
}

static inline int mem_buf_unassign_mem_gunyah(gh_memparcel_handle_t memparcel_hdl)
{
	return 0;
}
#endif
#endif
