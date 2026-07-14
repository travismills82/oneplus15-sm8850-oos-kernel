/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _MEM_BUF_ALTMAP_H
#define _MEM_BUF_ALTMAP_H


#include <linux/dma-buf.h>
#include <linux/mem-buf.h>
#include <uapi/linux/mem-buf.h>
#include <linux/gunyah/gh_rm_drv.h>
#include "../../drivers/dma-buf/heaps/qcom_sg_ops.h"

extern struct gen_pool *dmabuf_mem_pool;

/*
 * mem_buf_dmabuf_obj
 * A dmabuf object representing memory shared/lent by another Virtual machine
 * and obtained via mem_buf_retrieve().
 * @buffer: data type expected by qcom_sg_buf_ops
 * @pgmap: Arguments to memremap_pages
 * @memmap: alternate memmap for the dmabuf if present
 * @memmap_base: base ipa address of alternate memmap memory
 * @memmap_size: ipa size of alternate memmap memory
 */
struct mem_buf_dmabuf_obj {
	struct qcom_sg_buffer buffer;
	struct dev_pagemap pgmap;
	void *memmap;
	unsigned long memmap_base;
	size_t memmap_size;
};

struct dmabuf_membuf_helper {
	void *(*dmabuf_membuf_alloc)(struct mem_buf_allocation_data *alloc_data);
	void (*dmabuf_membuf_free)(void *__membuf);
	struct gh_sgl_desc *(*dmabuf_membuf_get_sgl)(void *__membuf);
	int (*dmabuf_membuf_prepare_altmap)(struct mem_buf_dmabuf_obj *obj,
			struct gh_sgl_desc **__sgl_desc, size_t dmabuf_size);
	size_t (*dmabuf_membuf_determine_memmap_size)(size_t dmabuf_size);
};

#if IS_ENABLED(CONFIG_QCOM_DMABUF_HEAPS_TVM_CARVEOUT)
void register_helper(struct dmabuf_membuf_helper *helper_obj);
#else
static inline void register_helper(struct dmabuf_membuf_helper *helper_obj)
{
}
#endif /* CONFIG_QCOM_DMABUF_HEAPS_TVM_CARVEOUT */

int prepare_altmap(struct mem_buf_dmabuf_obj *obj,
		struct gh_sgl_desc **__sgl_desc, size_t dmabuf_size);

size_t determine_memmap_size(size_t dmabuf_size);

#endif /* _MEM_BUF_ALTMAP_H */
