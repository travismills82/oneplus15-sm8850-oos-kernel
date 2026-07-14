// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of_reserved_mem.h>
#include <soc/qcom/secure_buffer.h>
#include <linux/memory_hotplug.h>
#include <linux/memory.h>
#include <linux/genalloc.h>
#include <linux/mem-buf-altmap.h>
#include <linux/gunyah/gh_rm_heap_manager.h>

#include <linux/mem-buf.h>
#include "mem-buf-dev.h"
#include "mem-buf-ids.h"

struct device *mem_buf_dev;
EXPORT_SYMBOL_GPL(mem_buf_dev);

unsigned char mem_buf_capability;
EXPORT_SYMBOL_GPL(mem_buf_capability);

struct gen_pool *dmabuf_mem_pool;
EXPORT_SYMBOL_GPL(dmabuf_mem_pool);

struct dentry *mem_buf_debugfs_root;
EXPORT_SYMBOL_GPL(mem_buf_debugfs_root);

static DEFINE_MUTEX(mem_buf_heap_lock);

#define POOL_MIN_ALLOC_ORDER SECTION_SIZE_BITS

int mem_buf_hyp_assign_table(struct sg_table *sgt, u32 *src_vmid, int source_nelems,
			     int *dest_vmids, int *dest_perms, int dest_nelems)
{
	char *verb;
	int ret;

	if (!mem_buf_vm_uses_hyp_assign())
		return 0;

	verb = *src_vmid == current_vmid ? "Assign" : "Unassign";

	pr_debug("%s memory to target VMIDs\n", verb);
	ret = hyp_assign_table(sgt, src_vmid, source_nelems, dest_vmids, dest_perms, dest_nelems);
	if (ret < 0)
		pr_err("Failed to %s memory for rmt allocation rc: %d\n", verb, ret);
	else
		pr_debug("Memory %s to target VMIDs\n", verb);

	return ret;
}

int mem_buf_assign_mem(u32 op, struct sg_table *sgt,
		       struct mem_buf_lend_kernel_arg *arg)
{
	int src_vmid[] = {current_vmid};
	int src_perms[] = {PERM_READ | PERM_WRITE | PERM_EXEC};
	int ret, ret2, i;
	struct scatterlist *sg;
	size_t dmabuf_size = 0;

	if (!sgt || !arg->nr_acl_entries || !arg->vmids || !arg->perms)
		return -EINVAL;

	arg->memparcel_hdl = MEM_BUF_MEMPARCEL_INVALID;

	mutex_lock(&mem_buf_heap_lock);

	for_each_sgtable_sg(sgt, sg, i)
		dmabuf_size += sg->length;

	/* check if we need to increase hypervisor heap memory for Gunyah VM usecase */
	if (mem_buf_vm_uses_gunyah(arg->vmids, arg->nr_acl_entries) > 0)
		gh_rm_heap_memlend_prealloc(dmabuf_size);

	ret = mem_buf_hyp_assign_table(sgt, src_vmid, ARRAY_SIZE(src_vmid), arg->vmids, arg->perms,
					arg->nr_acl_entries);
	if (ret)
		goto out_err;

	ret = mem_buf_assign_mem_gunyah(op, sgt, arg);
	if (ret) {
		ret2 = mem_buf_hyp_assign_table(sgt, arg->vmids, arg->nr_acl_entries,
					src_vmid, src_perms, ARRAY_SIZE(src_vmid));
		if (ret2 < 0) {
			pr_err("hyp_assign failed while recovering from another error: %d\n",
			       ret2);
			ret = -EADDRNOTAVAIL;
			goto out_err;
		}
	}

out_err:
	mutex_unlock(&mem_buf_heap_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(mem_buf_assign_mem);

int mem_buf_unassign_mem(struct sg_table *sgt, int *src_vmids,
			 unsigned int nr_acl_entries,
			 gh_memparcel_handle_t memparcel_hdl)
{
	int dst_vmid[] = {current_vmid};
	int dst_perm[] = {PERM_READ | PERM_WRITE | PERM_EXEC};
	int ret;

	if (!sgt || !src_vmids || !nr_acl_entries)
		return -EINVAL;

	mutex_lock(&mem_buf_heap_lock);

	if (memparcel_hdl != MEM_BUF_MEMPARCEL_INVALID) {
		ret = mem_buf_unassign_mem_gunyah(memparcel_hdl);
		if (ret)
			goto out_err;
	}

	/* reclaim any free hyp heap memory */
	gh_rm_heap_shrink();

	ret = mem_buf_hyp_assign_table(sgt, src_vmids, nr_acl_entries,
			       dst_vmid, dst_perm, ARRAY_SIZE(dst_vmid));

out_err:
	mutex_unlock(&mem_buf_heap_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(mem_buf_unassign_mem);

#ifdef CONFIG_QCOM_MEM_BUF_IPA_RESERVE
static int mem_buf_reserve_ipa(struct device *dev)
{
	const struct range pluggable_range = mhp_get_pluggable_range(true);
	struct range range;
	u32 flags;
	u64 size, ipa_base;
	char *propname;
	int ret;

	/* qcom,ipa-range includes range.start & range.end */
	propname = "qcom,ipa-range";
	ret = of_property_read_u64_index(dev->of_node, propname, 0, &range.start);
	ret |= of_property_read_u64_index(dev->of_node, propname, 1, &range.end);
	if (ret) {
		dev_info(dev, "Missing %s. Skipping ipa space reservation\n", propname);
		return 0;
	}

	range.start = max(range.start, pluggable_range.start);
	range.end = min(range.end, pluggable_range.end);

	ret = of_property_read_u64(dev->of_node, "qcom,dmabuf-ipa-size", &size);
	if (ret) {
		dev_err(dev, "Failed to parse qcom,dmabuf-ipa-size property %d start 0x%llx end 0x%llx\n",
				ret, range.start, range.end);
		return -EINVAL;
	}

	flags = GH_RM_IPA_RESERVE_NORMAL;
	ret = gh_rm_ipa_reserve(size, memory_block_size_bytes(), range, flags, 0, &ipa_base);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Hypervisor ipa reserve not supported %d\n", ret);
		return ret;
	}

	dmabuf_mem_pool = gen_pool_create(POOL_MIN_ALLOC_ORDER, -1);
	if (!dmabuf_mem_pool) {
		dev_err(dev, "gen_pool_create create failed %d\n", ret);
		return -ENOMEM;
	}

	ret = gen_pool_add(dmabuf_mem_pool, ipa_base, size, -1);
	if (ret) {
		dev_err(dev, "gen_pool_add create failed %d\n", ret);
		return ret;
	}

	return 0;
}
#else
static inline int mem_buf_reserve_ipa(struct device *dev)
{
	return -EINVAL;
}
#endif /* CONFIG_QCOM_MEM_BUF_IPA_RESERVE */

static int mem_buf_probe(struct platform_device *pdev)
{
	int ret, unused;
	struct device *dev = &pdev->dev;
	u64 dma_mask = IS_ENABLED(CONFIG_ARM64) ? DMA_BIT_MASK(64) :
		DMA_BIT_MASK(32);

#ifdef CONFIG_QCOM_MEM_BUF_IPA_RESERVE
	ret = mem_buf_reserve_ipa(dev);
	if (ret)
		return dev_err_probe(dev, ret, "mem_buf_reserve_ipa failed\n");
#endif

	ret = mem_buf_dma_buf_init();
	if (ret)
		return dev_err_probe(dev, ret, "mem_buf_dma_buf_init failed\n");

	if (of_property_match_string(dev->of_node, "qcom,mem-buf-capabilities",
				     "supplier") >= 0)
		mem_buf_capability = MEM_BUF_CAP_SUPPLIER;
	else if (of_property_match_string(dev->of_node,
					    "qcom,mem-buf-capabilities",
					    "consumer") >= 0)
		mem_buf_capability = MEM_BUF_CAP_CONSUMER;
	else if (of_property_match_string(dev->of_node,
					    "qcom,mem-buf-capabilities",
					    "dual") >= 0)
		mem_buf_capability = MEM_BUF_CAP_DUAL;
	else
		mem_buf_capability = 0;

	ret = dma_set_mask_and_coherent(dev, dma_mask);
	if (ret) {
		dev_err(dev, "Unable to set dma mask: %d\n", ret);
		return ret;
	}

	if (of_find_property(dev->of_node, "memory-region", &unused)) {
		ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, 0);
		if (ret) {
			dev_err(dev, "Failed to get memory-region property %d\n", ret);
			return ret;
		}
	}

	ret = mem_buf_vm_init(dev);
	if (ret) {
		dev_err(dev, "mem_buf_vm_init failed %d\n", ret);
		return ret;
	}

	mem_buf_dev = dev;
	return 0;
}

static void mem_buf_remove(struct platform_device *pdev)
{
	mem_buf_dev = NULL;
}

static const struct of_device_id mem_buf_match_tbl[] = {
	 {.compatible = "qcom,mem-buf"},
	 {},
};

static struct platform_driver mem_buf_driver = {
	.probe = mem_buf_probe,
	.remove = mem_buf_remove,
	.driver = {
		.name = "mem-buf",
		.of_match_table = of_match_ptr(mem_buf_match_tbl),
	},
};

static int __init mem_buf_dev_init(void)
{
	/* This returns an error if CONFIG_DEBUG_FS is disabled. Ignore it. */
	mem_buf_debugfs_root = debugfs_create_dir("mem_buf", NULL);

	return platform_driver_register(&mem_buf_driver);
}
module_init(mem_buf_dev_init);

static void __exit mem_buf_dev_exit(void)
{
	mem_buf_vm_exit();
	platform_driver_unregister(&mem_buf_driver);
	debugfs_remove_recursive(mem_buf_debugfs_root);
}
module_exit(mem_buf_dev_exit);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. Memory Buffer Sharing driver");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(DMA_BUF);
