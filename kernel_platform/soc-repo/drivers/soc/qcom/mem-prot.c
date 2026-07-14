// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/cma.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/genalloc.h>
#include <linux/dma-direct.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping-fast.h>
#include <linux/mem-prot.h>

struct mem_prot_pool {
	struct list_head list_node;
	struct gen_pool *gen_pool;

	/* start virtual address of the pool region */
	void *vaddr;

	enum cfg_phys_ddr_protection_cmd prot_type;
	size_t size;
	bool enabled;
	struct device *dev;
	const char *name;
	struct mutex lock;
};


/* Only one pool for each DDR protection type is supported. */
struct mem_prot_pool mem_prot_pools[CFG_PHYS_DDR_PROTECTION_CMD_MAX];

static DEFINE_MUTEX(debugfs_pool_mutex);
static LIST_HEAD(debugfs_pool_list);

static void mem_prot_debugfs_list_add(struct mem_prot_pool *pool)
{
	if (!pool)
		return;

	mutex_lock(&debugfs_pool_mutex);
	list_add(&pool->list_node, &debugfs_pool_list);
	mutex_unlock(&debugfs_pool_mutex);
}

static int mem_prot_debuginfo_show(struct seq_file *s, void *ignored)
{
	struct mem_prot_pool *pool;

	list_for_each_entry(pool, &debugfs_pool_list, list_node) {
		struct gen_pool *gen_pool = pool->gen_pool;

		seq_printf(s, "Pool Type: %s\n", pool->name);
		seq_printf(s, "managed (bytes): %zu\n", pool->size);
		seq_printf(s, "available (bytes): %zu\n", gen_pool ? gen_pool_avail(gen_pool) : 0);
		seq_printf(s, "Enabled: %s\n", pool->enabled ? "true" : "false");
		seq_puts(s, "-----------------------------\n");
	}

	return 0;
}

static int mem_prot_debuginfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, mem_prot_debuginfo_show, inode->i_private);
}

static const struct file_operations mem_prot_debugfs_fops = {
	.open = mem_prot_debuginfo_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static enum cfg_phys_ddr_protection_cmd get_mem_prot_type(struct device *dev)
{
	if (of_property_present(dev->of_node, "qcom,lcp-dare"))
		return CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE;
	else if (of_property_present(dev->of_node, "qcom,lcp-dae"))
		return CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DAE;
	else if (of_property_present(dev->of_node, "qcom,lcp-de"))
		return CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_DE;
	else
		return CFG_PHYS_DDR_PROTECTION_CMD_MAX;
}

static int configure_ddr_protection(phys_addr_t start_addr, size_t size,
				enum cfg_phys_ddr_protection_cmd prot_type)
{
	struct ppddr_region ppddr_region;
	struct pddr_region *d_region = &ppddr_region.data_region;
	phys_addr_t end_addr = start_addr + size;

	d_region->start_addr = start_addr;
	d_region->end_addr = end_addr;

	ppddr_region.lcp_mem_type = prot_type;

	return qcom_scm_cfg_pddr_protected_region(&ppddr_region);
}

static void mem_prot_disable_pool(struct mem_prot_pool *pool)
{
	struct gen_pool *gen_pool = pool->gen_pool;
	struct device *dev = pool->dev;
	struct gen_pool_chunk *chunk;
	phys_addr_t phys;
	int ret;

	chunk = list_first_entry(&gen_pool->chunks, struct gen_pool_chunk, next_chunk);
	phys = chunk->start_addr;

	ret = configure_ddr_protection(phys, pool->size,
				CFG_PHYS_DDR_PROTECTION_CMD_DISABLE_REGIONS);
	if (ret) {
		pr_err("Failed to disable DDR protection for memory pool.\n");
		return;
	}

	/* destroy and free gen_pool memory */
	gen_pool_destroy(gen_pool);

	/* Free the cma memory back to buddy */
	dma_free_coherent(dev, pool->size, pool->vaddr, phys_to_dma(dev, phys));

	pool->enabled = false;
	pool->gen_pool = NULL;
}

int mem_prot_pool_free(phys_addr_t phys, size_t size, enum cfg_phys_ddr_protection_cmd prot_type)
{
	struct mem_prot_pool *pool = &mem_prot_pools[prot_type];

	if (!pool->gen_pool)
		return -EINVAL;

	gen_pool_free(pool->gen_pool, (unsigned long)phys, size);

	mutex_lock(&pool->lock);

	/* Check for if gen_pool has already been destroyed by another user. */
	if (!pool->gen_pool) {
		mutex_unlock(&pool->lock);
		return 0;
	}

	if (pool->enabled && gen_pool_avail(pool->gen_pool) == pool->size)
		mem_prot_disable_pool(pool);

	mutex_unlock(&pool->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(mem_prot_pool_free);

static int mem_prot_enable_pool(struct mem_prot_pool *pool)
{
	struct device *dev = pool->dev;
	struct gen_pool *gen_pool;
	dma_addr_t dma_handle;
	phys_addr_t phys_addr;
	int ret;

	pool->vaddr = dma_alloc_coherent(dev, pool->size, &dma_handle, GFP_KERNEL);
	if (!pool->vaddr)
		return -EINVAL;

	phys_addr = dma_to_phys(dev, dma_handle);
	/*
	 * Create gen_pool to manage protected region.
	 * Note: These settings create the pool with a min alloc
	 * order of page size. Hence all allocations will be in the
	 * magnitude of page size.
	 */
	gen_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (IS_ERR(gen_pool)) {
		ret = PTR_ERR(gen_pool);
		goto gen_pool_create_err;
	}

	gen_pool_set_algo(gen_pool, gen_pool_best_fit, NULL);

	/* Add protected region memory into gen_pool */
	ret = gen_pool_add(gen_pool, (unsigned long)phys_addr,
			   pool->size, NUMA_NO_NODE);
	if (ret)
		goto gen_pool_add_err;

	ret = configure_ddr_protection(phys_addr, pool->size, pool->prot_type);
	if (ret)
		goto ddr_config_err;

	pool->gen_pool = gen_pool;
	pool->enabled = true;

	return 0;

ddr_config_err:
gen_pool_add_err:
	gen_pool_destroy(gen_pool);
gen_pool_create_err:
	dma_free_coherent(dev, pool->size, pool->vaddr, dma_handle);
	return ret;
}

int mem_prot_pool_alloc(size_t size, phys_addr_t *phys,
			enum cfg_phys_ddr_protection_cmd prot_type)
{
	struct mem_prot_pool *pool = &mem_prot_pools[prot_type];
	unsigned long addr;
	int ret;

	if (!pool->dev)
		return -EINVAL;

	mutex_lock(&pool->lock);
	if (!pool->enabled) {
		ret = mem_prot_enable_pool(pool);
		if (ret) {
			pr_err("Failed to enable ddr region protection for %s with err: %d\n",
				pool->name, ret);
			goto err;
		}
	}

	addr = gen_pool_alloc(pool->gen_pool, size);
	if (addr == 0) {
		pr_err("Not enough free memory in %s to meet alloc request.\n", pool->name);
		ret = -ENOMEM;
		goto err;
	}

	mutex_unlock(&pool->lock);

	*phys = (phys_addr_t)addr;
	return 0;

err:
	mutex_unlock(&pool->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(mem_prot_pool_alloc);

static int mem_prot_pool_create(struct device *dev, struct device_node *node)
{
	struct reserved_mem *rmem;
	enum cfg_phys_ddr_protection_cmd prot_type;
	struct mem_prot_pool *prot_pool;

	/* Read protection type from the DT */
	prot_type = get_mem_prot_type(dev);
	if (prot_type == CFG_PHYS_DDR_PROTECTION_CMD_MAX) {
		dev_err(dev, "Invalid memory protection type specified in DT.\n");
		return -EINVAL;
	}

	rmem = of_reserved_mem_lookup(node);
	if (!rmem)
		return -EINVAL;

	prot_pool = &mem_prot_pools[prot_type];

	prot_pool->prot_type = prot_type;
	prot_pool->size = rmem->size;
	prot_pool->enabled = false;
	prot_pool->dev = dev;
	prot_pool->name = dev_name(dev);

	mutex_init(&prot_pool->lock);
	mem_prot_debugfs_list_add(prot_pool);

	return 0;
}

static int mem_prot_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *node;
	struct device *dev = &pdev->dev;

	node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!node)
		return dev_err_probe(dev, -EINVAL, "Error reading \"memory-region\" from DT.\n");

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (ret)
		return dev_err_probe(dev, ret, "dma_set_mask_and_coherent failed.");

	ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, 0);
	if (ret) {
		dev_err_probe(dev, ret, "Failed to initialize cma memory.\n");
		goto probe_err;
	}

	ret = mem_prot_pool_create(dev, node);
	if (ret) {
		dev_err_probe(dev, ret, "Failed to create pool for %s.\n", dev_name(dev));
		goto probe_err;
	}

	pr_debug("%s device probe successful!\n", dev_name(dev));
	return 0;

probe_err:
	of_reserved_mem_device_release(dev);
	return ret;
}

static void mem_prot_remove(struct platform_device *pdev)
{
	of_reserved_mem_device_release(&pdev->dev);
}

static const struct of_device_id mem_prot_match_tbl[] = {
	{.compatible = "qcom,mem-prot-pool"},
	{},
};

static struct platform_driver mem_prot_driver = {
	.probe = mem_prot_dev_probe,
	.remove = mem_prot_remove,
	.driver = {
		.name = "mem-prot-driver",
		.of_match_table = of_match_ptr(mem_prot_match_tbl),
	},
};

static int __init mem_prot_init(void)
{
	struct dentry *dir;

	dir = debugfs_create_dir("mem-prot", NULL);
	if (IS_ERR(dir))
		pr_err("Unable to create mem-prot debugfs.\n");
	else
		debugfs_create_file("pools_info", 0600, dir, NULL, &mem_prot_debugfs_fops);

	return platform_driver_register(&mem_prot_driver);
}
module_init(mem_prot_init);

static void __exit mem_prot_exit(void)
{
	debugfs_lookup_and_remove("mem-prot", NULL);
	platform_driver_unregister(&mem_prot_driver);
}
module_exit(mem_prot_exit);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. LCP DARE Dynamic Enablement driver");
MODULE_LICENSE("GPL");
