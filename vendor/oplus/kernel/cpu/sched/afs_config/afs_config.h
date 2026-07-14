/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */
#ifndef _AFS_CONFIG_H_
#define _AFS_CONFIG_H_

#include <linux/sched.h>
#include <linux/types.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "afs_config: " fmt
#define AFS_CONFIG_MAX    2

#define AFS_CONFIG_MAGIC 'q'


enum {
	AFS_CONFIG_UPDATE = 1,
	AFS_CONFIG_GET_SCENE_CONFIG = 2,
};

struct scene_config {
	int scene_type;
	char animation_type;
	char enhance_level;
	char brk_type;
	char enable;
};

struct afs_config {
	int version;
	int scene_type_max;
	struct scene_config *scenes;
};

struct afs_config_info {
	int cur;
	struct afs_config* configs[AFS_CONFIG_MAX];
};

struct scene_config_request {
	int scene_type;
	struct scene_config scene_config;
};

#define IOCTL_AFS_CONFIG_UPDATE           \
	_IOW(AFS_CONFIG_MAGIC, AFS_CONFIG_UPDATE, struct afs_config)
#define IOCTL_AFS_CONFIG_GET_SCENE_CONFIG \
	_IOR(AFS_CONFIG_MAGIC, AFS_CONFIG_GET_SCENE_CONFIG, struct scene_config_request)


#endif /* _AFS_CONFIG_H_ */
