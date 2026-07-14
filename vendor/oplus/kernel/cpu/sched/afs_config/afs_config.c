/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>
#include <linux/seq_file.h>

#include "afs_config.h"

#define AFS_CONFIG_DIR       "oplus_afs_config"
#define AFS_CONFIG_FILE_NAME "afs_config"

static struct proc_dir_entry *afs_config_dir;
static struct afs_config_info afs_config_data;
static int afs_enable = 1;

static int afs_config_show(struct seq_file *m, void *v)
{
	int cur;
	struct afs_config *config;
	int i;

	cur = READ_ONCE(afs_config_data.cur);
	if (unlikely(cur < 0 || cur > 1)) {
		pr_err("%s: invalid afs_config.cur=%d\n", __func__, cur);
		return -EINVAL;
	}

	rcu_read_lock();
	config = rcu_dereference(afs_config_data.configs[cur]);
	if (unlikely(!config || !config->scenes)) {
		pr_err("%s: invalid config at index %d\n", __func__, cur);
		rcu_read_unlock();
		return -EINVAL;
	}

	seq_printf(m, "Version: %d\n", config->version);
	seq_printf(m, "%-12s %-16s %-14s %-9s %-7s\n",
		"scene_type", "animation_type",
		"enhance_level", "brk_type", "enable");

	for (i = 0; i <= config->scene_type_max; i++) {
	seq_printf(m, "%-12d %-16d %-14d %-9d %-7d\n",
			config->scenes[i].scene_type,
			config->scenes[i].animation_type,
			config->scenes[i].enhance_level,
			config->scenes[i].brk_type,
			config->scenes[i].enable);
	}
	rcu_read_unlock();
	return 0;
}

static int afs_config_open(struct inode *inode, struct file *file)
{
	return single_open(file, afs_config_show, NULL);
}

static int scene_config_get(struct scene_config_request *scene_config)
{
	int cur;
	struct afs_config *config;
	struct scene_config *scene_data;

	if (unlikely(!afs_enable)) {
		return -1;
	}
	cur = READ_ONCE(afs_config_data.cur);
	if (unlikely(cur < 0 || cur > 1)) {
		pr_err("%s: invalid afs_config.cur\n", __func__);
		return -EINVAL;
	}

	rcu_read_lock();
	config = rcu_dereference(afs_config_data.configs[cur]);
	if (unlikely(!config || !config->scenes)) {
		pr_err("%s: invalid config\n", __func__);
		rcu_read_unlock();
		return -EINVAL;
	}

	if (unlikely(scene_config->scene_type < 0 ||
		scene_config->scene_type > config->scene_type_max)) {
		pr_err("%s: invalid scene_type\n", __func__);
		rcu_read_unlock();
		return -EINVAL;
	}

	scene_data = &config->scenes[scene_config->scene_type];
	memcpy(&scene_config->scene_config, scene_data, sizeof(struct scene_config));
	rcu_read_unlock();
	return 0;
}

static int afs_config_update(struct afs_config *config)
{
	int cur;
	int new;
	struct afs_config *new_config;
	struct afs_config *old_config;

	cur = READ_ONCE(afs_config_data.cur);
	if (cur < 0 || cur > 1) {
		pr_err("%s: invalid cur\n", __func__);
		return -EINVAL;
	}
	new = cur ^ 1;

	new_config = (struct afs_config *)kzalloc(sizeof(struct afs_config), GFP_ATOMIC);
	if (NULL == new_config) {
		pr_err("%s: vmalloc failed\n", __func__);
		return -ENOMEM;
	}
	new_config->version = config->version;
	new_config->scene_type_max = config->scene_type_max;

	if (new_config->version <= 0) {
		pr_err("%s: invalid version\n", __func__);
		kfree(new_config);
		return -EINVAL;
	}

	if (NULL != afs_config_data.configs[cur]) {
		if (new_config->version <= afs_config_data.configs[cur]->version) {
			pr_err("%s: new_config version <= old_config version\n", __func__);
			kfree(new_config);
			return -EINVAL;
		}
	}

	new_config->scenes = (struct scene_config *)kzalloc(sizeof(struct scene_config) *
				(new_config->scene_type_max + 1), GFP_ATOMIC);
	memset(new_config->scenes, 0, sizeof(struct scene_config) * (new_config->scene_type_max + 1));
	if (NULL == new_config->scenes) {
		pr_err("%s: vmalloc failed\n", __func__);
		kfree(new_config->scenes);
		kfree(new_config);
		return -ENOMEM;
	}
	if (copy_from_user(new_config->scenes, config->scenes, sizeof(struct scene_config) *
				(new_config->scene_type_max + 1))) {
		kfree(new_config);
		return -EFAULT;
	}
	if (NULL == afs_config_data.configs[cur]) {
		rcu_assign_pointer(afs_config_data.configs[new], new_config);
		WRITE_ONCE(afs_config_data.cur, new);
	} else {
		old_config = rcu_dereference(afs_config_data.configs[cur]);
		rcu_assign_pointer(afs_config_data.configs[new], new_config);
		WRITE_ONCE(afs_config_data.cur, new);
		synchronize_rcu();
		if (NULL != old_config) {
			if (NULL != old_config->scenes) {
				kfree(old_config->scenes);
			}
			kfree(old_config);
		}
	}

	return 0;
}

static long afs_config_ioctl(struct file *file, unsigned int cmd, unsigned long __arg)
{
	struct afs_config afs_config_data_local;
	struct scene_config_request scene_config_req;
	void __user *arg = (void __user *) __arg;

	if (AFS_CONFIG_MAGIC != _IOC_TYPE(cmd) ||
		_IOC_NR(cmd) > AFS_CONFIG_GET_SCENE_CONFIG) {
		pr_err("%s: invalid ioctl cmd\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case IOCTL_AFS_CONFIG_GET_SCENE_CONFIG:
		if (copy_from_user(&scene_config_req, arg, sizeof(struct scene_config_request))) {
			pr_err("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}

		if (scene_config_get(&scene_config_req)) {
			pr_err("%s: scene_config_get failed\n", __func__);
			return -EINVAL;
		}
		if (copy_to_user(arg, &scene_config_req, sizeof(struct scene_config_request))) {
			pr_err("%s: copy_to_user failed\n", __func__);
			return -EFAULT;
		}
		break;
	case IOCTL_AFS_CONFIG_UPDATE:
		if (copy_from_user(&afs_config_data_local, arg, sizeof(struct afs_config))) {
			pr_err("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}

		if (afs_config_update(&afs_config_data_local)) {
			pr_err("%s: scene_config_update failed\n", __func__);
			return -EINVAL;
		}
		break;
	default:
		break;
	}
	return 0;
}

#ifdef CONFIG_COMPAT
static long afs_config_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return afs_config_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct proc_ops afs_config_fops = {
	.proc_open = afs_config_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
	.proc_ioctl = afs_config_ioctl,
#ifdef CONFIG_COMPAT
	.proc_compat_ioctl = afs_config_compat_ioctl,
#endif
};

static int afs_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", afs_enable);
	return 0;
}

static ssize_t afs_enable_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos)
{
	char buffer[32];
	int val;

	if (count >= sizeof(buffer))
	return -EINVAL;

	if (copy_from_user(buffer, buf, count))
	return -EFAULT;

	buffer[count] = '\0';
	if (kstrtoint(buffer, 0, &val))
	return -EINVAL;

	afs_enable = val ? 1 : 0;
	return count;
}

static int afs_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, afs_enable_show, NULL);
}

static const struct proc_ops afs_enable_fops = {
	.proc_open    = afs_enable_open,
	.proc_read    = seq_read,
	.proc_write   = afs_enable_write,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init afs_config_init(void)
{
	afs_config_dir = proc_mkdir(AFS_CONFIG_DIR, NULL);
	if (NULL == afs_config_dir) {
		pr_err("%s: proc_mkdir failed\n", __func__);
		return -ENOMEM;
	}

	if (NULL == proc_create(AFS_CONFIG_FILE_NAME, 0666, afs_config_dir, &afs_config_fops)) {
		pr_err("%s: proc_create failed\n", __func__);
		remove_proc_subtree(AFS_CONFIG_DIR, NULL);
		return -ENOMEM;
	}

	if (!proc_create("afs_enable", 0666, afs_config_dir, &afs_enable_fops)) {
		pr_err("%s: failed to create afs_enable proc entry\n", __func__);
		remove_proc_subtree(AFS_CONFIG_DIR, NULL);
		return -ENOMEM;
	}
	pr_info("init successfully.");
	return 0;
}

static void __exit afs_config_exit(void)
{
	remove_proc_subtree(AFS_CONFIG_DIR, NULL);
	pr_info("exit successfully.");
}

module_init(afs_config_init);
module_exit(afs_config_exit);
MODULE_DESCRIPTION("Oplus Afs config");
MODULE_LICENSE("GPL v2");
