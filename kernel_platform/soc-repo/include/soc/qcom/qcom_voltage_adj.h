/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QCOM_VADJ_H__
#define __QCOM_VADJ_H__

#define GLOBAL_VALID_MASK 0x8
#define CPU_VALID_MASK 0x80
#define GLOBAL_MASK 0x7
#define CPU_MASK 0x70
#define CPU_SHFT 0x4
#define VALID_MASK 0x88

#define SDAM2_MEM17 0x7151

#define MAX_IDX 7

struct qvadj_platform_data {
	struct mutex lock;
	struct regmap *regmap;
	struct dentry *root;
};

static struct qvadj_platform_data *pd;

#if IS_ENABLED(CONFIG_QCOM_VADJ)

int qvadj_get_cur_voltage_overrides(u8 *global_rails_index, u8 *cpu_rails_index);
int qvadj_set_next_voltage_overrides(u8 global_rails_index, u8 cpu_rails_index);

#else

int qvadj_get_cur_voltage_overrides(u8 *global_rails_index, u8 *cpu_rails_index)
{ return -ENODEV; }
int qvadj_set_next_voltage_overrides(u8 *global_rails_index, u8 *cpu_rails_index)
{ return -ENODEV; }

#endif
#endif /*__QCOM_VADJ_H__ */
