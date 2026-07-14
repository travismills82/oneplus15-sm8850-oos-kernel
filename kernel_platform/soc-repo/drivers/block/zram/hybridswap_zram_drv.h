// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2030 Oplus. All rights reserved.
 */

#ifndef _HYBRIDSWAP_ZRAM_DRV_H_
#define _HYBRIDSWAP_ZRAM_DRV_H_

#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include "zram_drv.h"

static inline void zram_slot_lock(struct zram *zram, u32 index)
{
	spin_lock(&zram->table[index].lock);
}

static inline void zram_slot_unlock(struct zram *zram, u32 index)
{
	spin_unlock(&zram->table[index].lock);
}

static inline struct zram *dev_to_zram(struct device *dev)
{
	return (struct zram *)dev_to_disk(dev)->private_data;
}

static inline unsigned long zram_get_handle(struct zram *zram, u32 index)
{
	return zram->table[index].handle;
}

static inline void zram_set_handle(struct zram *zram, u32 index, unsigned long handle)
{
	zram->table[index].handle = handle;
}

/* flag operations require table entry lock being held */
static inline bool zram_test_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	return zram->table[index].flags & BIT(flag);
}

static inline void zram_set_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	zram->table[index].flags |= BIT(flag);
}

static inline void zram_clear_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	zram->table[index].flags &= ~BIT(flag);
}

static inline size_t zram_get_obj_size(struct zram *zram, u32 index)
{
	return zram->table[index].flags & (BIT(ZRAM_FLAG_SHIFT) - 1);
}

static inline void zram_set_obj_size(struct zram *zram,
					u32 index, size_t size)
{
	unsigned long flags = zram->table[index].flags >> ZRAM_FLAG_SHIFT;

	zram->table[index].flags = (flags << ZRAM_FLAG_SHIFT) | size;
}
#endif /* _HYBRIDSWAP_ZRAM_DRV_H_ */

