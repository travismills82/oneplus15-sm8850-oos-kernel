/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_KEY_INTERNAL_H
#define _LINUX_KEY_INTERNAL_H

#include <linux/key.h>

#ifdef CONFIG_KEYS
/* Core-owned allocation.  The public tag remains the allocation base for RCU. */
struct key_tag_alloc {
	struct key_tag tag;
	atomic64_t index_id;
};
#endif

#endif /* _LINUX_KEY_INTERNAL_H */
