/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _GH_VM_H
#define _GH_VM_H

#include <linux/gunyah_qtvm.h>

#ifdef CONFIG_QTVM_WITH_AVF
#define GH_VM_BEFORE_POWERUP		GUNYAH_QTVM_BEFORE_POWERUP
#define GH_VM_POWERUP_FAIL		GUNYAH_QTVM_POWERUP_FAIL
#define GH_VM_EARLY_POWEROFF		GUNYAH_QTVM_EARLY_POWEROFF
#define GH_VM_POWEROFF			GUNYAH_QTVM_POWEROFF
#define GH_VM_EXITED			GUNYAH_QTVM_EXITED
#define GH_VM_CRASH			GUNYAH_QTVM_CRASH

#else
#define GH_VM_BEFORE_POWERUP		0x1
#define GH_VM_POWERUP_FAIL		0x2
#define GH_VM_EARLY_POWEROFF		0x3
#define GH_VM_POWEROFF			0x4
#define GH_VM_EXITED			0x5
#define GH_VM_CRASH			0x6

#endif

#ifdef CONFIG_QTVM_WITH_AVF
/* VM power notifications */
static inline int gh_register_vm_notifier(struct notifier_block *nb)
{
	return gunyah_qtvm_register_notifier(nb);
}

static inline int gh_unregister_vm_notifier(struct notifier_block *nb)
{
	return gunyah_qtvm_unregister_notifier(nb);
}

#elif IS_ENABLED(CONFIG_GH_SECURE_VM_LOADER)
int gh_register_vm_notifier(struct notifier_block *nb);
int gh_unregister_vm_notifier(struct notifier_block *nb);

#else
static inline int gh_register_vm_notifier(struct notifier_block *nb)
{
	return 0;
}
static inline int gh_unregister_vm_notifier(struct notifier_block *nb)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_GH_PROXY_SCHED) || IS_ENABLED(CONFIG_GH_VCPU_MGR)
int gh_poll_vcpu_run(gh_vmid_t vmid);
#else
static inline int gh_poll_vcpu_run(gh_vmid_t vmid)
{
	return 0;
}
#endif

#endif /* _GH_VM_H */
