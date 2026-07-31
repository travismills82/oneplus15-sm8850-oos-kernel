// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2021 Oplus. All rights reserved.
 */
#include <linux/seq_file.h>
#include <linux/mm.h>

#include <trace/hooks/mm.h>
#include "../mm/internal.h"

#include "common.h"
#include "internal.h"
#include "mm-hooks.h"

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

struct common_data {
	DECLARE_BITMAP(scene, BITS_PER_LONG);
	/* Prevent concurrent execution of device init */
	struct rw_semaphore init_lock;
	/* debug symbols */
	void *symbols[OMS_END];
	/* timer */
	bool timer_init;
	/* kobj */
	struct kobject *common_kobj;
	kallsyms_lookup_name_t kp_kallsyms_lookup_name;
};

static const char * const scene_to_txt[NR_MM_SCENE_BIT] = {
	"camera",
	"camera_preopen",
	"display_off",
};

/* replace this with pointer */
static struct common_data g_common;
static void __set_or_clear_scene(unsigned long nr, bool set)
{
	struct common_data *data = &g_common;

	if (set)
		set_bit(nr, data->scene);
	else
		clear_bit(nr, data->scene);
}

static int set_or_clear_scene(unsigned int cmd, unsigned long arg)
{
	bool set = cmd == CMD_OSVELTE_SET_SCENE;
	void __user *argp = (void __user *) arg;
	struct osvelte_common_header header;
	unsigned long nr;

	if (copy_from_user(&header, argp, sizeof(header)))
		return -EFAULT;

	nr = header.private_data;
	/* santity check */
	if (nr >= NR_MM_SCENE_BIT)
		return -EINVAL;

	osvelte_logi("cmd: %d nr: %lu", cmd, nr);
	trace_oplus_mm_vh_set_or_clear_scene(set, nr);
	/* preopen for camera app. but it means camera would open soon */
	if (nr == MM_SCENE_CAMERA_PREOPEN)
		return 0;

	__set_or_clear_scene(nr, set);
	return 0;
}

int osvelte_set_scene(enum oplus_mm_scene_bit nr, bool set)
{
	trace_oplus_mm_vh_set_or_clear_scene(set, nr);

	if (unlikely(nr >= NR_MM_SCENE_BIT))
		return -EINVAL;

	__set_or_clear_scene(nr, set);
	return 0;
}
EXPORT_SYMBOL_GPL(osvelte_set_scene);

bool osvelte_test_scene(unsigned long nr)
{
	struct common_data *data = &g_common;

	return test_bit(nr, data->scene);
}
EXPORT_SYMBOL_GPL(osvelte_test_scene);

void osvelte_register_symbol(enum oplus_mm_symbol sym, void *addr)
{
	struct common_data *data = &g_common;

	down_write(&data->init_lock);
	data->symbols[sym] = addr;
	up_write(&data->init_lock);
}
EXPORT_SYMBOL_GPL(osvelte_register_symbol);

void *osvelte_read_symbol(enum oplus_mm_symbol sym, bool atomic)
{
	struct common_data *data = &g_common;
	void *addr = NULL;

	if (sym >= OMS_END)
		goto out;

	/* for vendor hook, fast return, this may not safe */
	if (atomic)
		return data->symbols[sym];

	down_read(&data->init_lock);
	addr = data->symbols[sym];
	up_read(&data->init_lock);
out:
	return addr;
}
EXPORT_SYMBOL_GPL(osvelte_read_symbol);

void *osvelte_kallsyms_lookup_name(const char *name)
{
	struct common_data *data = &g_common;

	if (unlikely(!data->kp_kallsyms_lookup_name))
		return NULL;

	return (void *)data->kp_kallsyms_lookup_name(name);
}
EXPORT_SYMBOL_GPL(osvelte_kallsyms_lookup_name);

long osvelte_common_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	if (cmd < CMD_COMMON_MIN || cmd > CMD_COMMON_MAX)
		return CMD_COMMON_INVALID;

	switch (cmd) {
	case CMD_OSVELTE_SET_SCENE:
	case CMD_OSVELTE_CLEAR_SCENE:
		ret = set_or_clear_scene(cmd, arg);
		break;
	default:
		break;
	}
	return ret;
}

static ssize_t stats_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct common_data *data = &g_common;
	int size = 0, i;
	struct task_struct *task;

	/* now only use 64bit */
	size += sysfs_emit_at(buf, size, "[scene %lx]\n", data->scene[0]);
	for (i = MM_SCENE_CAMERA; i < NR_MM_SCENE_BIT; i++)
		size += sysfs_emit_at(buf, size, "%-20s %d\n", scene_to_txt[i],
				      test_bit(i, data->scene));

	down_read(&data->init_lock);
	size += sysfs_emit_at(buf, size, "[symbols]\n");
	size += sysfs_emit_at(buf, size, "%-20s %p\n", "oplus_mm",
			      data->symbols[OPLUS_MM_KOBJ]);

	task = (struct task_struct *)data->symbols[OPLUS_MM_TASK_ERM_RECLAIMD];
	size += sysfs_emit_at(buf, size, "%-20s %d\n", "erm_reclaimd",
			      task == NULL ? -1 : task->tgid);
	task = (struct task_struct *)data->symbols[OPLUS_MM_TASK_CACHED_BOOSTPOOL_PREFILL];
	size += sysfs_emit_at(buf, size, "%-20s %d\n", "bp_refill",
			      task == NULL ? -1 : task->tgid);
	up_read(&data->init_lock);
	return size;
}

static struct kobj_attribute stats_attr = __ATTR_RO(stats);

static struct attribute *attrs[] = {
	&stats_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int init_kallsyms_lookup_name(void)
{
	struct common_data *data = &g_common;
	struct kprobe kp = {
		.symbol_name = "kallsyms_lookup_name",
	};
	int ret;

	ret = register_kprobe(&kp);
	if (ret) {
		osvelte_loge("failed to read kallsyms_lookup_name\n");
		return ret;
	}
	data->kp_kallsyms_lookup_name = (void *)kp.addr;
	unregister_kprobe(&kp);
	osvelte_logi("+\n");
	return 0;
}

int osvelte_common_init(struct kobject *root)
{
	struct common_data *data = &g_common;
	int ret;

	init_rwsem(&data->init_lock);
	data->common_kobj = kobject_create_and_add("common", root);
	if (!data->common_kobj) {
		osvelte_loge("failed to create sysfs common_kobj\n");
		return -ENOMEM;
	}

	ret = sysfs_create_group(data->common_kobj, &attr_group);
	if (ret) {
		osvelte_loge("failed to create sysfs common group\n");
		kobject_put(data->common_kobj);
		return -ENOMEM;
	}
	init_kallsyms_lookup_name();
	osvelte_register_symbol(OPLUS_MM_KOBJ, oplus_mm_kobj);
	return 0;
}

int osvelte_common_exit(void)
{
	/* unsafe, exit do not support for now */
	return 0;
}
