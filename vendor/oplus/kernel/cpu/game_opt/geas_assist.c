// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#include "game_ctrl.h"
#include "../geas/cpu/geas_cpu_external.h"


/* for geas*/
enum gameopt_geas_cmd {
	NOTIFY_GEAS_GAME_ON,
	NOTIFY_GEAS_UPDATE,
};

struct gameopt_geas_info {
	struct multitask_ofb_base_info b_info;
	struct multitask_load_info l_info;
	int type;
};

int (*geas_update_ofb_base_info_ptr)(struct multitask_ofb_base_info *base_info) = NULL;
EXPORT_SYMBOL(geas_update_ofb_base_info_ptr);
int (*geas_update_load_info_ptr)(struct multitask_load_info *load_info) = NULL;
EXPORT_SYMBOL(geas_update_load_info_ptr);

#define GAMEOPT_GEAS_MAGIC 0xDD
#define CMD_ID_GAMEOPT_GEAS_BASE_INFO \
	_IOWR(GAMEOPT_GEAS_MAGIC, NOTIFY_GEAS_GAME_ON, struct multitask_load_info)
#define CMD_ID_GAMEOPT_GEAS_UPDATE_LOAD \
	_IOWR(GAMEOPT_GEAS_MAGIC, NOTIFY_GEAS_UPDATE, struct multitask_load_info)

static int geas_ctrl_open(struct inode *inode, struct file *file) {
	return 0;
}

static int geas_ctrl_release(struct inode *inode, struct file *file) {
	return 0;
}

static long geas_update_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct gameopt_geas_info* info;
	void __user *uarg = (void __user *)arg;
	long ret = 0;

	info = kzalloc(sizeof(struct gameopt_geas_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s failed malloc cmd %x _IOC_TYPE(cmd) %x _IOC_NR(cmd) %x\n", __func__, cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		return -ENOMEM;
	}
	if ((_IOC_TYPE(cmd) != GAMEOPT_GEAS_MAGIC) || (_IOC_NR(cmd) > NOTIFY_GEAS_UPDATE)) {
		kfree(info);
		pr_err("%s err cmd %x _IOC_TYPE(cmd) %x _IOC_NR(cmd) %x\n", __func__, cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));

		return -EINVAL;
	}
	if (copy_from_user(info, uarg, sizeof(struct gameopt_geas_info))) {
		kfree(info);
		return -EFAULT;
	}
	switch (cmd) {
	case CMD_ID_GAMEOPT_GEAS_BASE_INFO:
		if (geas_update_ofb_base_info_ptr)
			ret = geas_update_ofb_base_info_ptr(&info->b_info);
		break;

	case CMD_ID_GAMEOPT_GEAS_UPDATE_LOAD:
		if (geas_update_load_info_ptr)
			/* ret: <0 : sys err 0: geas err 1: success and update */
			ret = geas_update_load_info_ptr(&info->l_info);
		if (ret >= 0) {
			int ret_1;
			struct gameopt_geas_info *info_user = (struct gameopt_geas_info *)arg;
			ret_1 = copy_to_user(&info_user->l_info.out, &info->l_info.out, sizeof(struct multitask_load_out));
			if (ret_1) {
				pr_err("%s copy_to_user err ret %d cmd %x _IOC_TYPE(cmd) %x _IOC_NR(cmd) %x\n",
					__func__, ret_1, cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
				kfree(info);
				return ret_1;
			}
			ret = 1;
		} else
			ret = 0;
		break;

	default:
		pr_err("%s failed not found cmd %x %lx %lx\n",
			__func__, cmd, CMD_ID_GAMEOPT_GEAS_BASE_INFO, CMD_ID_GAMEOPT_GEAS_UPDATE_LOAD);
	break;
	}
	kfree(info);
	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_geas_update_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return geas_update_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif /* CONFIG_COMPAT */

static const struct proc_ops geas_update_proc_ops = {
	.proc_open  = geas_ctrl_open,
	.proc_ioctl = geas_update_ioctl,
	.proc_release = geas_ctrl_release,
#if IS_ENABLED(CONFIG_COMPAT)
	.proc_compat_ioctl	= compat_geas_update_ioctl,
#endif /* CONFIG_COMPAT */
	.proc_lseek = default_llseek,
};

int geas_assist_init(void)
{
	if (unlikely(!game_opt_dir)) {
		return -ENOTDIR;
	}
	proc_create_data("geas_assist", 0664, game_opt_dir, &geas_update_proc_ops, NULL);

	return 0;
}

