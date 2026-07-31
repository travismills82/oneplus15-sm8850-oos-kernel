/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2021 Oplus. All rights reserved.
 */
#ifndef _OSVELTE_COMMON_H
#define _OSVELTE_COMMON_H

#define KMODULE_NAME "oplus_bsp_mm_osvelte"

#define DEV_NAME "osvelte"

#define DEV_PATH "/dev/" DEV_NAME

#define OSVELTE_LOG_TAG DEV_NAME

/* declare page-flags here */
#define PG_erm (PG_oem_reserved_1)

/* enable using erm freelist to boost allocation*/
#define PF_MEMALLOC_BOOST	PF__HOLE__00800000
/* put reclaimed folio into ezr freelist */
#define PF_ERM_HOOK_RECLAIMED	PF__HOLE__01000000
#define PF_SHRINK_ANON		PF__HOLE__02000000

static inline void memalloc_hook_reclaimed_save(void)
{
	current->flags |= PF_ERM_HOOK_RECLAIMED;
}

static inline void memalloc_hook_reclaimed_restore(void)
{
	current->flags = (current->flags & ~PF_ERM_HOOK_RECLAIMED);
}

static inline bool memalloc_hook_reclaimed_test(void)
{
	return current->flags & PF_ERM_HOOK_RECLAIMED;
}

static inline void memalloc_boost_save(void)
{
	current->flags |= PF_MEMALLOC_BOOST;
}

static inline void memalloc_boost_restore(void)
{
	current->flags = (current->flags & ~PF_MEMALLOC_BOOST);
}

static inline bool memalloc_boost_test(void)
{
	return current->flags & PF_MEMALLOC_BOOST;
}

enum oplus_mm_scene_bit {
	MM_SCENE_CAMERA = 0,
	/* touchdown on camera app icon. */
	MM_SCENE_CAMERA_PREOPEN,
	MM_SCENE_DISPLAY_OFF,
	NR_MM_SCENE_BIT,
};

enum oplus_mm_symbol {
	OPLUS_MM_KOBJ,
	OPLUS_MM_TASK_ERM_RECLAIMD,
	OPLUS_MM_TASK_CACHED_BOOSTPOOL_PREFILL,
	OMS_END,
};

#define OMTE_COMMON_STRING "8940000:"
enum oplus_mm_trace_event {
	OMTE_COMMON = 8940000,
	OMTE_KWAPD_WAKEUP_HIGH_ORDER,
	OMTE_KWAPD_RECLAIM,
	OMTE_DMA_BUF_ALLOCATION,
	OMTE_DMA_BUF_ALLOCATION_ORDERS,
	OMTE_ERM_RECLAIM,
	OMTE_ERM_RELEASE,
	OMTE_ERM_RELEASE_DONE,
};

/* common ioctl for userspace */
#define __COMMONIO 0xFA
#define CMD_OSVELTE_GET_VERSION		_IO(__COMMONIO, 1)
#define CMD_OSVELTE_SET_SCENE		_IO(__COMMONIO, 2)
#define CMD_OSVELTE_CLEAR_SCENE		_IO(__COMMONIO, 3)

struct osvelte_common_header {
	u32 api_version;
	u64 private_data;
	u32 buffer_len;
	/* payload */
	char data[];
};

/* kgsl.c use osvelte_info */
#define osvelte_info(fmt, ...)      \
	pr_info(OSVELTE_LOG_TAG ": " fmt, ##__VA_ARGS__)

#define osvelte_err(fmt, ...)      \
	pr_err(OSVELTE_LOG_TAG ": " fmt, ##__VA_ARGS__)

long osvelte_common_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int osvelte_common_init(struct kobject *root);
int osvelte_common_exit(void);

extern struct kobject *oplus_mm_kobj;
extern void osvelte_register_symbol(enum oplus_mm_symbol sym, void *data);
extern void *osvelte_read_symbol(enum oplus_mm_symbol sym, bool atomic);
extern int osvelte_set_scene(enum oplus_mm_scene_bit nr, bool set);
extern bool osvelte_test_scene(unsigned long nr);
extern void *osvelte_kallsyms_lookup_name(const char *name);
#endif /* _OSVELTE_COMMON_H */
