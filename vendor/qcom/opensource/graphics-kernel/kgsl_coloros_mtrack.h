/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ColorOS 17.0.0.102 OSVELTE client contract. The older shared source header
 * assigns the GPU to the wrong runtime registry slot for the retained module.
 * Keep this firmware-bound declaration local to the updated GPU client.
 */
#ifndef _KGSL_COLOROS_MTRACK_H
#define _KGSL_COLOROS_MTRACK_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>

enum mtrack_type {
	MTRACK_DMABUF,
	MTRACK_GPU,
	MTRACK_ZRAM,
	MTRACK_ERM,
	MTRACK_UXMEM_POOL,
	MTRACK_MAX
};

enum mtrack_subtype {
	MTRACK_DMABUF_SYSTEM_HEAP,
	MTRACK_DMABUF_POOL,
	MTRACK_DMABUF_BOOST_POOL,
	MTRACK_GPU_TOTAL,
	MTRACK_GPU_PROC_KERNEL,
	MTRACK_ZRAM_ORIG,
	MTRACK_ZRAM_COMPR_BYTES,
	MTRACK_ZRAM_MEMUSED,
	MTRACK_ZRAM_SAMEPAGES,
	MTRACK_ERM_LRU,
	MTRACK_ERM_FREE,
	MTRACK_UXMEM_POOL_TOTAL,
	MTRACK_SUBTYPE_MAX
};

struct mtrack_debugger {
	long (*mem_usage)(enum mtrack_subtype type);
	long (*pid_mem_usage)(enum mtrack_subtype type, pid_t pid);
	void (*dump_usage_stat)(bool verbose);
};

int register_mtrack_debugger(enum mtrack_type type,
			    struct mtrack_debugger *debugger);
void unregister_mtrack_debugger(enum mtrack_type type,
			       struct mtrack_debugger *debugger);
int register_mtrack_procfs(enum mtrack_type t, const char *name, umode_t mode,
			 const struct proc_ops *proc_ops, void *data);
void unregister_mtrack_procfs(enum mtrack_type t, const char *name);

#endif
