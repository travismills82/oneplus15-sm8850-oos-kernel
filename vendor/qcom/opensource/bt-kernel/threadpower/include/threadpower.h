/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __LINUX_THREAD_POWER_H
#define __LINUX_THREAD_POWER_H

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/skbuff.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/of_device.h>
#include <soc/qcom/cmd-db.h>
#include <linux/kdev_t.h>
#include <linux/refcount.h>
#include <linux/idr.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/soc/qcom/qcom_aoss.h>
#include <linux/pinctrl/qcom-pinctrl.h>
#include <linux/soc/qcom/qcom_aoss.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>

#define THREAD_DEFAULT_LOG_LVL        0x03
#define THREAD_DEBUG_LOG_LVL          0x04
#define THREAD_INFO_LOG_LVL           0x08

static uint8_t log_lvl = THREAD_DEFAULT_LOG_LVL;

#define THREAD_ERR(fmt, arg...) \
	pr_err("THREAD %s: " fmt "\n", __func__, ## arg)

#define THREAD_WARN(fmt, arg...) \
	pr_warn("THREAD %s: " fmt "\n", __func__, ## arg)

#define THREAD_DBG(fmt, arg...) \
	{ if (log_lvl >= THREAD_DEBUG_LOG_LVL) \
		pr_err("THREAD %s: " fmt "\n", __func__, ## arg); \
	else \
		pr_debug("THREAD %s: " fmt "\n", __func__, ## arg); }

#define THREAD_INFO(fmt, arg...) \
	{ if (log_lvl >= THREAD_INFO_LOG_LVL) \
		pr_err("THREAD %s: " fmt "\n", __func__, ## arg);\
	else \
		pr_info("THREAD %s: " fmt "\n", __func__, ## arg); }

#define GPIO_HIGH                       0x00000001
#define GPIO_LOW                        0x00000000

#define THREAD_STATE_SUCCESS            0x00
#define THREAD_STATE_FAILED             0x01

#define THREAD_CMD_PWR_VOTE             0xAAA1
#define THREAD_CMD_GET_RESOURCE_STATE   0xAAA2
#define THREAD_CMD_GET_CHIPSET_ID       0xAAA3
#define THREAD_CMD_UPDATE_CHIPSET_VER   0xAAA4
#define THREAD_CMD_PANIC                0xAAA5

#define MAX_PROP_SIZE                   32

enum power_vote_request {
	THREAD_PWR_CMD_CLIENT_KILLED = -3,
	THREAD_PWR_CMD_WAITING_RSP = -2,
	THREAD_PWR_CMD_FAIL_RSP_RECV = -1,
	THREAD_PWR_CMD_RSP_RECV = 0
};

enum THREAD_PWR_VOTE_OPTION {
	POWER_ON,
	POWER_OFF,
	POWER_RETENION_EN,
	POWER_RETENION_DIS,
	THREAD_PWR_MAX_OPTION
};

struct log_index {
	int init;
	int crash;
};

struct log_gpio_status {
	int during_pwr_on;
	int during_crash;
};

struct vreg_data {
	struct regulator *reg;  /* voltage regulator handle */
	const char *name;       /* regulator name */
	u32 min_vol;            /* min voltage level */
	u32 max_vol;            /* max voltage level */
	u32 load_curr;          /* current */
	bool is_enabled;        /* is this regulator enabled? */
	bool is_retention_supp; /* does this regulator support retention mode */
	struct log_index indx;  /* Index for reg. w.r.t init & crash */
};

struct pwr_data {
	char compatible[32];
	struct vreg_data *reg_resource;
	int num_reg_resource;
};

struct thread_pwr_struct {
	int is_probe_pending;
	int is_reg_resource_avl;
	int is_gpio_resource_avl;
	char chip_set_id[MAX_PROP_SIZE];
	int chipset_version;
	int thread_enable_gpio;
	struct log_gpio_status thread_en_pin_state;
};







#endif /*__LINUX_THREAD_POWER_H */
