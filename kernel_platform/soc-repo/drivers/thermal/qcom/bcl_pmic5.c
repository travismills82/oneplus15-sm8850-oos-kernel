// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "%s:%s " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spmi.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/thermal.h>
#include <linux/slab.h>
#include <linux/nvmem-consumer.h>
#include <linux/ipc_logging.h>
#include "thermal_zone_internal.h"
#ifndef OPLUS_FEATURE_CHG_BASIC
#define OPLUS_FEATURE_CHG_BASIC
#endif
#ifdef OPLUS_FEATURE_CHG_BASIC
#include <linux/power_supply.h>
#include <linux/proc_fs.h>
#define CREATE_TRACE_POINTS
#include "trace.h"
#include <linux/rtc.h>
#include <linux/time.h>
#endif

#define BCL_DRIVER_NAME       "bcl_pmic5"
#define BCL_MONITOR_EN        0x46
#define BCL_IRQ_STATUS        0x08
#define BCL_REVISION1         0x0
#define BCL_REVISION2         0x01
#define BCL_PARAM_1           0x0e
#define BCL_PARAM_2           0x0f

#define BCL_IBAT_HIGH         0x4B
#define BCL_IBAT_TOO_HIGH     0x4C
#define BCL_IBAT_TOO_HIGH_REV4 0x4D
#define BCL_IBAT_READ         0x86
#define BCL_IBAT_SCALING_UA   78127
#define BCL_IBAT_CCM_SCALING_UA   15625
#define BCL_IBAT_CCM_LANDO_SCALING_UA   152
#define BCL_ADC_IBAT_CCM_LANDO_SCALING_UA   39062
#define BCL_IBAT_SCALING_REV4_UA  93753

#define BCL_VBAT_READ         0x76
#define BCL_VBAT_CONV_REQ     0x72

#define BCL_GEN3_MAJOR_REV    4
#define BCL_PARAM_HAS_ADC      BIT(0)
#define BCL_PARAM_HAS_IBAT_ADC BIT(2)
#define ADC_CMN_BG_ADC_DVDD_SPARE1 0x3050

#define BCL_IRQ_L0       0x1
#define BCL_IRQ_L1       0x2
#define BCL_IRQ_L2       0x4

/*
 * 49827 = 64.879uV (one bit value) * 3 (voltage divider)
 *		* 256 (8 bit shift for MSB)
 */
#define BCL_VBAT_SCALING_UV   49827
#define BCL_VBAT_NO_READING   127
#define BCL_VBAT_BASE_MV      2000
#define BCL_VBAT_INC_MV       25
#define BCL_VBAT_MAX_MV       3600
#define BCL_VBAT_THRESH_BASE  0x8CA
#define BCL_VBAT_THRESH_BASE_NUKU  0x5DC
#define SUBTYPE_ADDR 0x105
#define SUBTYPE_NUKU 0x5D    /* pmh0101 */
#ifdef OPLUS_FEATURE_CHG_BASIC
#define SUBTYPE_PMIH 0x56    /* pmih010x */
#endif

#define BCL_IBAT_CCM_OFFSET   800
#define BCL_IBAT_CCM_LSB      100
#define BCL_IBAT_CCM_MAX_VAL  14

#define BCL_IBAT_CCM_LANDO_OFFSET   900
#define BCL_IBAT_CCM_LANDO_LSB      150
#define BCL_IBAT_CCM_LANDO_MAX_VAL  16

#define BCL_VBAT_4G_NO_READING 0x7fff
#define BCL_GEN4_MAJOR_REV    5
#define BCL_VBAT_SCALING_REV5_NV   194637  /* 64.879uV (one bit) * 3 VD */
#define BCL_IBAT_SCALING_REV5_NA   61037
#define BCL_IBAT_THRESH_SCALING_REV5_UA   156255 /* 610.37uA * 256 */
#define BCL_VBAT_TRIP_CNT     3

#define MAX_PERPH_COUNT       3
#define IPC_LOGPAGES          2

#define BCL_IPC(dev, msg, args...)      do { \
			if ((dev) && (dev)->ipc_log) { \
				ipc_log_string((dev)->ipc_log, \
					"[%s]: %s: " msg, \
					current->comm, __func__, args); \
			} \
		} while (0)

enum bcl_dev_type {
	BCL_IBAT_LVL0,
	BCL_IBAT_LVL1,
	BCL_VBAT_LVL0,
	BCL_VBAT_LVL1,
	BCL_VBAT_LVL2,
	BCL_LVL0,
	BCL_LVL1,
	BCL_LVL2,
	BCL_2S_IBAT_LVL0,
	BCL_2S_IBAT_LVL1,
	BCL_TYPE_MAX,
};

enum bcl_monitor_type {
	BCL_MON_DEFAULT,
	BCL_MON_VBAT_ONLY,
	BCL_MON_IBAT_ONLY,
	BCL_MON_MAX,
};

enum {
	BCLBIG_COMP_VCMP_L0_THR,
	BCLBIG_COMP_VCMP_L1_THR,
	BCLBIG_COMP_VCMP_L2_THR,
	REG_MAX,
};

#ifdef OPLUS_FEATURE_CHG_BASIC
enum {
	PMIC_SUBTYPE_PMH0101,
	PMIC_SUBTYPE_PMIH010X,
	PMIC_SUBTYPE_MAX,
};
#endif

struct bcl_desc {
	bool vadc_type;
	u32 vbat_regs[REG_MAX];
	bool vbat_zone_enabled;
	u32 vcmp_thresh_base;
	u32 vcmp_thresh_max;
};

static char bcl_int_names[BCL_TYPE_MAX][25] = {
	"bcl-ibat-lvl0",
	"bcl-ibat-lvl1",
	"bcl-vbat-lvl0",
	"bcl-vbat-lvl1",
	"bcl-vbat-lvl2",
	"bcl-lvl0",
	"bcl-lvl1",
	"bcl-lvl2",
	"bcl-2s-ibat-lvl0",
	"bcl-2s-ibat-lvl1",
};

enum bcl_ibat_ext_range_type {
	BCL_IBAT_RANGE_LVL0,
	BCL_IBAT_RANGE_LVL1,
	BCL_IBAT_RANGE_LVL2,
	BCL_IBAT_RANGE_MAX,
};

static uint32_t bcl_ibat_ext_ranges[BCL_IBAT_RANGE_MAX] = {
	10,		/* default range factor */
	20,
	25
};

struct bcl_device;

struct bcl_peripheral_data {
	int                     irq_num;
	int                     status_bit_idx;
	long			trip_thresh;
	int                     last_val;
	int                     def_vbat_min_thresh;
	struct mutex            state_trans_lock;
	bool			irq_enabled;
	enum bcl_dev_type	type;
	struct thermal_zone_device_ops ops;
	struct thermal_zone_device *tz_dev;
	struct bcl_device	*dev;
};

#ifdef OPLUS_FEATURE_CHG_BASIC
struct dynamic_vbat_data {
	int temp;
	int vbat_mv_lv0;
	int vbat_mv_lv1;
	int vbat_mv_lv2;
};
#endif

struct bcl_device {
	struct device			*dev;
	struct regmap			*regmap;
	uint16_t			fg_bcl_addr;
	uint8_t				dig_major;
	uint8_t				dig_minor;
	uint8_t				bcl_param_1;
	uint8_t				bcl_type;
	void				*ipc_log;
	int				bcl_monitor_type;
	bool				ibat_ccm_enabled;
	bool				ibat_ccm_lando_enabled;
	bool				ibat_use_qg_adc;
	bool				no_bit_shift;
	uint32_t			ibat_ext_range_factor;
	struct bcl_peripheral_data	param[BCL_TYPE_MAX];
	const struct bcl_desc		*desc;
	struct notifier_block		nb;
#ifdef OPLUS_FEATURE_CHG_BASIC
	bool				support_track;
	int				id;
	bool				support_dynamic_vbat;
	struct dynamic_vbat_data	*dynamic_vbat_config;
	int				dynamic_vbat_config_count;
	struct notifier_block		psy_nb;
	struct work_struct		vbat_check_work;
	int				pmic_type;
#endif
};

static struct bcl_device *bcl_devices[MAX_PERPH_COUNT];
static int bcl_device_ct;
#ifdef OPLUS_FEATURE_CHG_BASIC
static int BCL_LEVEL0_COUNT;
static int BCL_LEVEL1_COUNT;
static int BCL_LEVEL2_COUNT;
struct proc_dir_entry *oplus_bcl_stat;

struct timeval {
	long tv_sec;
	long tv_usec;
};

static int get_pmic_subtype(struct bcl_device *bcl_perph)
{
	unsigned int data = 0;
	int ret = 0;
	ret = regmap_read(bcl_perph->regmap, SUBTYPE_ADDR, &data);
	if (ret < 0) {
		pr_err("Error reading PMIC SUBTYPE, err:%d\n",ret);
		return PMIC_SUBTYPE_MAX;
	}

	if (data == SUBTYPE_NUKU) { /* pmh0101 */
		return PMIC_SUBTYPE_PMH0101;
	} else if (data == SUBTYPE_PMIH ) { /* pmih010x */
		return PMIC_SUBTYPE_PMIH010X;
	} else {
		pr_debug("invalid subtype = 0x%x\n", data);
		return PMIC_SUBTYPE_MAX;
	}
}

static ssize_t bcl_count_show(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[256];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "%d  %d  %d\n", BCL_LEVEL0_COUNT, BCL_LEVEL1_COUNT, BCL_LEVEL2_COUNT);
	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static void do_gettimeofday(struct timeval *tv)
{
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec/1000;
}
#endif
static BLOCKING_NOTIFIER_HEAD(bcl_pmic5_notifier);

void bcl_pmic5_notifier_register(struct notifier_block *n)
{
	blocking_notifier_chain_register(&bcl_pmic5_notifier, n);
}

void bcl_pmic5_notifier_unregister(struct notifier_block *n)
{
	blocking_notifier_chain_unregister(&bcl_pmic5_notifier, n);
}

static int bcl_read_multi_register(struct bcl_device *bcl_perph, int16_t reg_offset,
				unsigned int *data, size_t len)
{
	int ret = 0;

	if (!bcl_perph) {
		pr_err("BCL device not initialized\n");
		return -EINVAL;
	}
	ret = regmap_bulk_read(bcl_perph->regmap,
			       (bcl_perph->fg_bcl_addr + reg_offset),
			       data, len);
	if (ret < 0)
		pr_err("Error reading reg base:0x%04x len:%ld err:%d\n",
				bcl_perph->fg_bcl_addr + reg_offset, len, ret);
	else
		pr_debug("Read register:0x%04x value:0x%02x len:%ld\n",
				bcl_perph->fg_bcl_addr + reg_offset,
				*data, len);

	return ret;
}

static int bcl_read_register(struct bcl_device *bcl_perph, int16_t reg_offset,
				unsigned int *data)
{
	int ret = 0;

	if (!bcl_perph) {
		pr_err("BCL device not initialized\n");
		return -EINVAL;
	}
	ret = regmap_read(bcl_perph->regmap,
			       (bcl_perph->fg_bcl_addr + reg_offset),
			       data);
	if (ret < 0)
		pr_err("Error reading register 0x%04x err:%d\n",
				bcl_perph->fg_bcl_addr + reg_offset, ret);
	else
		pr_debug("Read register:0x%04x value:0x%02x\n",
				bcl_perph->fg_bcl_addr + reg_offset,
				*data);

	return ret;
}

static int bcl_write_register(struct bcl_device *bcl_perph,
				int16_t reg_offset, uint8_t data)
{
	int  ret = 0;
	uint8_t *write_buf = &data;
	uint16_t base;

	if (!bcl_perph) {
		pr_err("BCL device not initialized\n");
		return -EINVAL;
	}
	base = bcl_perph->fg_bcl_addr;
	ret = regmap_write(bcl_perph->regmap, (base + reg_offset), *write_buf);
	if (ret < 0) {
		pr_err("Error reading register:0x%04x val:0x%02x err:%d\n",
				base + reg_offset, data, ret);
		return ret;
	}
	pr_debug("wrote 0x%02x to 0x%04x\n", data, base + reg_offset);

	return ret;
}

static void convert_vbat_thresh_val_to_adc(struct bcl_device *bcl_perph, int *val)
{
	/*
	 * Threshold register can be bit shifted from ADC MSB.
	 * So the scaling factor is half in those cases.
	 */
	if (bcl_perph->no_bit_shift)
		*val = (*val * 1000) / BCL_VBAT_SCALING_UV;
	else
		*val = (*val * 2000) / BCL_VBAT_SCALING_UV;
}

/* Common helper to convert nano unit to milli unit */
static void convert_adc_nu_to_mu_val(unsigned int *val, unsigned int scaling_factor)
{
	*val = div_s64(*val * scaling_factor, 1000000);
}

static void convert_adc_to_vbat_val(int *val)
{
	*val = (*val * BCL_VBAT_SCALING_UV) / 1000;
}

static void convert_vbat_to_vcmp_val(const struct bcl_desc *desc, int vbat, int *val)
{
	if (vbat > desc->vcmp_thresh_max)
		vbat = desc->vcmp_thresh_max;
	else if (vbat < desc->vcmp_thresh_base)
		vbat = desc->vcmp_thresh_base;

	*val = (vbat - desc->vcmp_thresh_base) / BCL_VBAT_INC_MV;
}

static void convert_vcmp_idx_to_vbat_val(const struct bcl_desc *desc, int vcmp_idx, int *val)
{
	*val = desc->vcmp_thresh_base + (BCL_VBAT_INC_MV * vcmp_idx);
}

static void convert_ibat_to_adc_val(struct bcl_device *bcl_perph, int *val, int scaling_factor)
{
	/*
	 * Threshold register can be bit shifted from ADC MSB.
	 * So the scaling factor is half in those cases.
	 */
	if (bcl_perph->ibat_use_qg_adc)
		*val = (int)div_s64(*val * 2000 * 2, scaling_factor);
	else if (bcl_perph->no_bit_shift)
		*val = (int)div_s64(*val * 1000 * bcl_ibat_ext_ranges[BCL_IBAT_RANGE_LVL0],
				scaling_factor);
	else
		*val = (int)div_s64(*val * 2000 * bcl_ibat_ext_ranges[BCL_IBAT_RANGE_LVL0],
				scaling_factor);

}

static void convert_adc_to_ibat_val(struct bcl_device *bcl_perph, int *val, int scaling_factor)
{
	/* Scaling factor will be half if ibat_use_qg_adc is true */
	if (bcl_perph->ibat_use_qg_adc)
		*val = (int)div_s64(*val * scaling_factor, 2 * 1000);
	else
		*val = (int)div_s64(*val * scaling_factor,
				1000 * bcl_ibat_ext_ranges[BCL_IBAT_RANGE_LVL0]);
}

static int8_t convert_ibat_to_ccm_val(int ibat,
	int ibat_ccm_max_val, int ibat_ccm_offset, int ibat_ccm_lsb)
{
	int8_t val = ibat_ccm_max_val;

	val = (int8_t)((ibat - ibat_ccm_offset) / ibat_ccm_lsb);

	if (val > ibat_ccm_max_val) {
		pr_err(
		"CCM thresh:%d is invalid, use MAX supported threshold\n",
			ibat);
		val = ibat_ccm_max_val;
	}

	return val;
}

static int bcl_set_ibat(struct thermal_zone_device *tz, int low, int high)
{
	int ret = 0, ibat_ua, thresh_value;
	int8_t val = 0;
	int16_t addr;
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tz->devdata;

	mutex_lock(&bat_data->state_trans_lock);
	thresh_value = high;
	if (bat_data->trip_thresh == thresh_value)
		goto set_trip_exit;

	if (bat_data->irq_num && bat_data->irq_enabled) {
		disable_irq_nosync(bat_data->irq_num);
		bat_data->irq_enabled = false;
	}
	if (thresh_value == INT_MAX) {
		bat_data->trip_thresh = thresh_value;
		goto set_trip_exit;
	}

	ibat_ua = thresh_value;
	if (bat_data->dev->ibat_ccm_enabled)
		convert_ibat_to_adc_val(bat_data->dev, &thresh_value,
				BCL_IBAT_CCM_SCALING_UA *
				bat_data->dev->ibat_ext_range_factor);
	else if (bat_data->dev->ibat_ccm_lando_enabled)
		convert_ibat_to_adc_val(bat_data->dev, &thresh_value,
				BCL_ADC_IBAT_CCM_LANDO_SCALING_UA *
				bat_data->dev->ibat_ext_range_factor);
	else if (bat_data->dev->dig_major >= BCL_GEN4_MAJOR_REV)
		convert_ibat_to_adc_val(bat_data->dev, &thresh_value,
				BCL_IBAT_THRESH_SCALING_REV5_UA *
				bat_data->dev->ibat_ext_range_factor);
	else if (bat_data->dev->dig_major >= BCL_GEN3_MAJOR_REV)
		convert_ibat_to_adc_val(bat_data->dev, &thresh_value,
				BCL_IBAT_SCALING_REV4_UA *
				bat_data->dev->ibat_ext_range_factor);
	else
		convert_ibat_to_adc_val(bat_data->dev, &thresh_value,
				BCL_IBAT_SCALING_UA *
				bat_data->dev->ibat_ext_range_factor);
	val = (int8_t)thresh_value;
	switch (bat_data->type) {
	case BCL_IBAT_LVL0:
	case BCL_2S_IBAT_LVL0:
		addr = BCL_IBAT_HIGH;
		pr_debug("ibat high threshold:%d mA ADC:0x%02x\n",
				ibat_ua, val);
		break;
	case BCL_IBAT_LVL1:
	case BCL_2S_IBAT_LVL1:
		addr = BCL_IBAT_TOO_HIGH;
		if (bat_data->dev->dig_major >= BCL_GEN3_MAJOR_REV &&
			bat_data->dev->bcl_param_1 & BCL_PARAM_HAS_IBAT_ADC)
			addr = BCL_IBAT_TOO_HIGH_REV4;
		if (bat_data->dev->ibat_ccm_enabled)
			val = convert_ibat_to_ccm_val(ibat_ua,
				BCL_IBAT_CCM_MAX_VAL, BCL_IBAT_CCM_OFFSET, BCL_IBAT_CCM_LSB);
		if (bat_data->dev->ibat_ccm_lando_enabled)
			val = convert_ibat_to_ccm_val(ibat_ua,
				BCL_IBAT_CCM_LANDO_MAX_VAL, BCL_IBAT_CCM_LANDO_OFFSET,
				BCL_IBAT_CCM_LANDO_LSB);
		pr_debug("ibat too high threshold:%d mA ADC:0x%02x\n",
				ibat_ua, val);
		break;
	default:
		goto set_trip_exit;
	}
	ret = bcl_write_register(bat_data->dev, addr, val);
	if (ret)
		goto set_trip_exit;
	bat_data->trip_thresh = ibat_ua;

	if (bat_data->irq_num && !bat_data->irq_enabled) {
		enable_irq(bat_data->irq_num);
		bat_data->irq_enabled = true;
	}

set_trip_exit:
	mutex_unlock(&bat_data->state_trans_lock);

	return ret;
}

static int bcl_read_ibat(struct thermal_zone_device *tz, int *adc_value)
{
	int ret = 0;
	unsigned int val = 0;
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tz->devdata;

	*adc_value = val;
	if (bat_data->dev->dig_major < BCL_GEN4_MAJOR_REV)
		ret = bcl_read_register(bat_data->dev, BCL_IBAT_READ, &val);
	else
		ret = bcl_read_multi_register(bat_data->dev, BCL_IBAT_READ, &val, 2);

	if (ret)
		return ret;

	/* IBat ADC reading is in 2's compliment form */
	if (bat_data->dev->dig_major < BCL_GEN4_MAJOR_REV)
		*adc_value = sign_extend32(val, 7);
	else
		*adc_value = sign_extend32(val, 15);
	if (val == 0) {
		/*
		 * The sensor sometime can read a value 0 if there is
		 * consequtive reads
		 */
		*adc_value = bat_data->last_val;
	} else {
		if (bat_data->dev->ibat_ccm_enabled)
			convert_adc_to_ibat_val(bat_data->dev, adc_value,
				BCL_IBAT_CCM_SCALING_UA *
				bat_data->dev->ibat_ext_range_factor);
		else if (bat_data->dev->ibat_ccm_lando_enabled)
			convert_adc_to_ibat_val(bat_data->dev, adc_value,
				BCL_IBAT_CCM_LANDO_SCALING_UA *
				bat_data->dev->ibat_ext_range_factor);
		else if (bat_data->dev->dig_major >= BCL_GEN4_MAJOR_REV)
			convert_adc_nu_to_mu_val(adc_value,
				BCL_IBAT_SCALING_REV5_NA);
		else if (bat_data->dev->dig_major >= BCL_GEN3_MAJOR_REV)
			convert_adc_to_ibat_val(bat_data->dev, adc_value,
				BCL_IBAT_SCALING_REV4_UA *
					bat_data->dev->ibat_ext_range_factor);
		else
			convert_adc_to_ibat_val(bat_data->dev, adc_value,
				BCL_IBAT_SCALING_UA *
					bat_data->dev->ibat_ext_range_factor);
		bat_data->last_val = *adc_value;
	}
	pr_debug("ibat:%d mA ADC:0x%02x\n", bat_data->last_val, val);
	BCL_IPC(bat_data->dev, "ibat:%d mA ADC:0x%02x\n",
		 bat_data->last_val, val);

	return ret;
}

static int bcl_read_vbat_tz(struct thermal_zone_device *tzd, int *adc_value)
{
	int ret = 0;
	unsigned int val = 0;
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tzd->devdata;

	*adc_value = val;
	if (bat_data->dev->dig_major < BCL_GEN4_MAJOR_REV)
		ret = bcl_read_register(bat_data->dev, BCL_VBAT_READ, &val);
	else
		ret = bcl_read_multi_register(bat_data->dev, BCL_VBAT_READ,
				&val, 2);

	if (ret)
		return ret;

	*adc_value = val;
	if ((bat_data->dev->dig_major < BCL_GEN4_MAJOR_REV &&
			*adc_value == BCL_VBAT_NO_READING) ||
		(bat_data->dev->dig_major >= BCL_GEN4_MAJOR_REV &&
			*adc_value == BCL_VBAT_4G_NO_READING)) {
		*adc_value = bat_data->last_val;
	} else {
		if (bat_data->dev->dig_major < BCL_GEN4_MAJOR_REV)
			convert_adc_to_vbat_val(adc_value);
		else
			convert_adc_nu_to_mu_val(adc_value,
				BCL_VBAT_SCALING_REV5_NV);
		bat_data->last_val = *adc_value;
	}
	pr_debug("vbat:%d mv\n", bat_data->last_val);
	BCL_IPC(bat_data->dev, "vbat:%d mv ADC:0x%02x\n",
			bat_data->last_val, val);

	return ret;
}

static int bcl_set_adc_value(struct bcl_device *bcl_perph,
				int addr_idx, int temp, int *val)
{
	int ret = 0;
	int16_t addr;
	int thresh = temp;

	if (temp <= 0) {
		pr_debug("Invalid input temp\n");
		return -EINVAL;
	} else if (temp < bcl_perph->desc->vcmp_thresh_base) {
		pr_debug("input temp is %d, lower than MIN\n", temp);
		return -EINVAL;
	} else if (temp > bcl_perph->desc->vcmp_thresh_max) {
		pr_debug("input temp is %d, higher than MAX\n", temp);
		return -EINVAL;
	}

	addr = bcl_perph->desc->vbat_regs[addr_idx];
	if ((addr_idx == BCLBIG_COMP_VCMP_L0_THR) &&
				bcl_perph->desc->vadc_type) {
		convert_vbat_thresh_val_to_adc(bcl_perph, &thresh);
		*val = thresh;
	} else {
		convert_vbat_to_vcmp_val(bcl_perph->desc, temp, val);
	}

	ret = bcl_write_register(bcl_perph, addr, *val);
	BCL_IPC(bcl_perph, "threshold:%d mV ADC:0x%x\n", temp, *val);
	return ret;
}

static int bcl_config_vph_cb(struct notifier_block *nb,
				unsigned long val, void *data)
{
	int ret = NOTIFY_OK;
	int *temp = (int *)data;
	int reg_data = 0;
	struct bcl_device *bcl_priv =
			container_of(nb, struct bcl_device, nb);

	ret = bcl_set_adc_value(bcl_priv, val, *temp, &reg_data);
	if (ret < 0)
		pr_err("Fail to set vph regs, err: %d\n", ret);

	pr_debug("trip_id: %lu, vph:%d mV, ADC: 0x%x\n", val, *temp, reg_data);

	return ret;
}

#ifdef OPLUS_FEATURE_CHG_BASIC
static int oplus_bcl_write_vbat_tz(struct bcl_device *bcl_perph, int trip_id, int temp)
{
	int ret = 0;
	int val = 0;
	int16_t addr;
	if (temp <= 0) {
		pr_err("Invalid input temp\n");
		return -EINVAL;
	} else if (temp < BCL_VBAT_THRESH_BASE) {
		pr_err("input temp is %d, lower than MIN\n", temp);
		return -EINVAL;
	} else if (temp > BCL_VBAT_MAX_MV) {
		pr_err("input temp is %d, higher than MAX\n", temp);
		return -EINVAL;
	}
	addr = bcl_perph->desc->vbat_regs[trip_id];
	convert_vbat_to_vcmp_val(bcl_perph->desc, temp, &val);
	ret = bcl_write_register(bcl_perph, addr, val);
	if (ret < 0) {
		pr_err("oplus_bcl_write_vbat_tz fail to set vbat regs, err: %d\n", ret);
		goto exit;
	}
	if (bcl_perph->desc->vbat_zone_enabled)
		blocking_notifier_call_chain(&bcl_pmic5_notifier, trip_id, (void *)&temp);

	pr_debug("oplus_bcl_write_vbat_tz trip_id: %d, vbat:%d mV\n", trip_id, temp);

exit:
	return ret;
}
#endif

static int bcl_write_vbat_tz(struct thermal_zone_device *tzd,
					const struct thermal_trip *trip, int temp)
{
	int ret = 0, val = 0, trip_id = 0;
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tzd->devdata;

	if (bat_data->def_vbat_min_thresh &&
		temp < bat_data->def_vbat_min_thresh) {
		pr_err("User vbat thresh is %d,lower than target MIN:%d\n",
				temp, bat_data->def_vbat_min_thresh);
		return -EINVAL;
	}

	trip_id = trip_to_trip_desc(trip) - tzd->trips;

	mutex_lock(&bat_data->state_trans_lock);
	ret = bcl_set_adc_value(bat_data->dev, trip_id, temp, &val);
	if (ret < 0) {
		pr_err("Fail to set vbat regs, err: %d\n", ret);
		goto exit;
	}

	if (bat_data->dev->desc->vbat_zone_enabled)
		blocking_notifier_call_chain(&bcl_pmic5_notifier, trip_id, (void *)&temp);

	pr_debug("trip_id: %d, vbat:%d mV, ADC: 0x%x\n", trip_id, temp, val);

exit:
	mutex_unlock(&bat_data->state_trans_lock);
	return ret;
}

static struct thermal_zone_device_ops vbat_tzd_ops = {
	.get_temp = bcl_read_vbat_tz,
	.set_trip_temp = bcl_write_vbat_tz,
};

static int bcl_get_trend(struct thermal_zone_device *tz, const struct thermal_trip *trips,
			enum thermal_trend *trend)
{
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tz->devdata;

	mutex_lock(&bat_data->state_trans_lock);
	if (!bat_data->last_val)
		*trend = THERMAL_TREND_DROPPING;
	else
		*trend = THERMAL_TREND_RAISING;
	mutex_unlock(&bat_data->state_trans_lock);

	return 0;
}

static int bcl_set_lbat(struct thermal_zone_device *tz, int low, int high)
{
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tz->devdata;

	mutex_lock(&bat_data->state_trans_lock);

	if (high == INT_MAX &&
		bat_data->irq_num && bat_data->irq_enabled) {
		disable_irq_nosync(bat_data->irq_num);
		disable_irq_wake(bat_data->irq_num);
		bat_data->irq_enabled = false;
		pr_debug("lbat[%d]: disable irq:%d\n",
				bat_data->type,
				bat_data->irq_num);
	} else if (high != INT_MAX &&
		bat_data->irq_num && !bat_data->irq_enabled) {
		enable_irq(bat_data->irq_num);
		enable_irq_wake(bat_data->irq_num);
		bat_data->irq_enabled = true;
		pr_debug("lbat[%d]: enable irq:%d\n",
				bat_data->type,
				bat_data->irq_num);
	}

	mutex_unlock(&bat_data->state_trans_lock);

	return 0;
}

static int bcl_read_lbat(struct thermal_zone_device *tz, int *adc_value)
{
	int ret = 0;
	int ibat = 0, vbat = 0;
	unsigned int val = 0;
	struct bcl_peripheral_data *bat_data =
		(struct bcl_peripheral_data *)tz->devdata;
	struct bcl_device *bcl_perph = bat_data->dev;

	*adc_value = val;
	ret = bcl_read_register(bcl_perph, BCL_IRQ_STATUS, &val);
	if (ret)
		goto bcl_read_exit;
	switch (bat_data->type) {
	case BCL_LVL0:
		*adc_value = val & BCL_IRQ_L0;
		break;
	case BCL_LVL1:
		*adc_value = val & BCL_IRQ_L1;
		break;
	case BCL_LVL2:
		*adc_value = val & BCL_IRQ_L2;
		break;
	default:
		pr_err("Invalid sensor type:%d\n", bat_data->type);
		ret = -ENODEV;
		goto bcl_read_exit;
	}
	bat_data->last_val = *adc_value;
	pr_debug("lbat:%d val:%d\n", bat_data->type,
			bat_data->last_val);
	if (bcl_perph->param[BCL_IBAT_LVL0].tz_dev)
		bcl_read_ibat(bcl_perph->param[BCL_IBAT_LVL0].tz_dev, &ibat);
	else if (bcl_perph->param[BCL_2S_IBAT_LVL0].tz_dev)
		bcl_read_ibat(bcl_perph->param[BCL_2S_IBAT_LVL0].tz_dev, &ibat);
	if (bcl_perph->param[BCL_VBAT_LVL0].tz_dev)
		bcl_read_vbat_tz(bcl_perph->param[BCL_VBAT_LVL0].tz_dev, &vbat);
	BCL_IPC(bcl_perph, "LVLbat:%d val:%d\n", bat_data->type,
			bat_data->last_val);

bcl_read_exit:
	return ret;
}

static irqreturn_t bcl_handle_irq(int irq, void *data)
{
	struct bcl_peripheral_data *perph_data =
		(struct bcl_peripheral_data *)data;
	unsigned int irq_status = 0;
	int ibat = 0, vbat = 0;
	struct bcl_device *bcl_perph;

#ifdef OPLUS_FEATURE_CHG_BASIC
	struct timeval tv;
	static struct power_supply *batt_psy;
	union power_supply_propval psy = {0,};
	int err = 0;
	int vol = 0,curr = 0;
	int level = -1,id = -1;
	long time_s = 0;
#endif

	if (!perph_data->tz_dev)
		return IRQ_HANDLED;
	bcl_perph = perph_data->dev;
	bcl_read_register(bcl_perph, BCL_IRQ_STATUS, &irq_status);

#ifdef OPLUS_FEATURE_CHG_BASIC
	if (bcl_perph->support_track) {
		if (BCL_LEVEL0_COUNT == INT_MAX)
			BCL_LEVEL0_COUNT = 0;
		if (BCL_LEVEL1_COUNT == INT_MAX)
			BCL_LEVEL1_COUNT = 0;
		if (BCL_LEVEL2_COUNT == INT_MAX)
			BCL_LEVEL2_COUNT = 0;
		if (irq_status & 0x04) {
			level = 2;
			BCL_LEVEL2_COUNT++;
		} else if (irq_status & 0x02) {
			level = 1;
			BCL_LEVEL1_COUNT++;
		} else if (irq_status & 0x01) {
			level = 0;
			BCL_LEVEL0_COUNT++;
		}
		do_gettimeofday(&tv);
		time_s = tv.tv_sec;
		id = bcl_perph->id;
		if (!batt_psy)
			batt_psy = power_supply_get_by_name("battery");
		if (batt_psy) {
			err = power_supply_get_property(batt_psy,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, &psy);
			if (err) {
				pr_err("can't get battery voltage:%d\n",err);
			} else {
				vol = psy.intval / 1000;
			}

			err = power_supply_get_property(batt_psy,
				POWER_SUPPLY_PROP_CURRENT_NOW, &psy);
			if (err) {
				pr_err("can't get battery current:%d\n",err);
			} else {
				curr = psy.intval;
			}
		}
		pr_err("%s:time_s = %ld, level =%d, id = %d, vol = %d, curr =%d, irq_status = %x\n",
			 __func__, time_s, level, id, vol, curr, irq_status);
		trace_bcl_stat(time_s, id, level, vol, curr);

	}
#endif

	if (bcl_perph->param[BCL_IBAT_LVL0].tz_dev)
		bcl_read_ibat(bcl_perph->param[BCL_IBAT_LVL0].tz_dev, &ibat);
	else if (bcl_perph->param[BCL_2S_IBAT_LVL0].tz_dev)
		bcl_read_ibat(bcl_perph->param[BCL_2S_IBAT_LVL0].tz_dev, &ibat);
	if (bcl_perph->param[BCL_VBAT_LVL0].tz_dev)
		bcl_read_vbat_tz(bcl_perph->param[BCL_VBAT_LVL0].tz_dev, &vbat);

	if (irq_status & perph_data->status_bit_idx) {
		pr_debug(
		"Irq:%d triggered for bcl type:%s. status:%u ibat=%d vbat=%d\n",
			irq, bcl_int_names[perph_data->type],
			irq_status, ibat, vbat);
		BCL_IPC(bcl_perph,
		"Irq:%d triggered for bcl type:%s. status:%u ibat=%d vbat=%d\n",
			irq, bcl_int_names[perph_data->type],
			irq_status, ibat, vbat);
		thermal_zone_device_update(perph_data->tz_dev,
				THERMAL_TRIP_VIOLATED);
	}

	return IRQ_HANDLED;
}

#ifdef OPLUS_FEATURE_CHG_BASIC
static const struct proc_ops proc_bcl_count = {
	.proc_read		= bcl_count_show,
};
#endif

static int bcl_get_ibat_ext_range_factor(struct platform_device *pdev,
		uint32_t *ibat_range_factor)
{
	int ret = 0;
	const char *name;
	struct nvmem_cell *cell;
	size_t len;
	char *buf;
	uint32_t ext_range_index = 0;

	ret = of_property_read_string(pdev->dev.of_node, "nvmem-cell-names", &name);
	if (ret) {
		*ibat_range_factor = bcl_ibat_ext_ranges[BCL_IBAT_RANGE_LVL0];
		pr_debug("Default ibat range factor enabled %u\n", *ibat_range_factor);
		return 0;
	}

	cell = nvmem_cell_get(&pdev->dev, name);
	if (IS_ERR(cell)) {
		dev_err(&pdev->dev, "failed to get nvmem cell %s\n", name);
		return PTR_ERR(cell);
	}

	buf = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR_OR_NULL(buf)) {
		dev_err(&pdev->dev, "failed to read nvmem cell %s\n", name);
		return PTR_ERR(buf);
	}

	if (len <= 0 || len > sizeof(uint32_t)) {
		dev_err(&pdev->dev, "nvmem cell length out of range %zu\n", len);
		kfree(buf);
		return -EINVAL;
	}
	memcpy(&ext_range_index, buf, min(len, sizeof(ext_range_index)));
	kfree(buf);

	if (ext_range_index >= BCL_IBAT_RANGE_MAX) {
		dev_err(&pdev->dev, "invalid BCL ibat scaling factor %d\n", ext_range_index);
		return -EINVAL;
	}

	*ibat_range_factor = bcl_ibat_ext_ranges[ext_range_index];
	pr_debug("ext_range_index %u, ibat range factor %u\n",
			ext_range_index, *ibat_range_factor);

	return 0;
}

#ifdef OPLUS_FEATURE_CHG_BASIC
static void bcl_get_dynamic_vbat_data(struct platform_device *pdev,
					struct bcl_device *bcl_perph)
{
	int ret;
	struct device_node *dev_node = pdev->dev.of_node;
	int num_elem;
	int buf[64] = {0};
	int i;

	bcl_perph->support_dynamic_vbat = of_property_read_bool(dev_node,"bcl,support_dynamic_vbat");
	pr_err("bcl support_dynamic_vbat:%d, pmic_type:%d\n", bcl_perph->support_dynamic_vbat, bcl_perph->pmic_type);
	if (bcl_perph->support_dynamic_vbat) {
		num_elem = of_property_count_elems_of_size(dev_node, "bcl,dynamic_vbat_data", sizeof(int));
		if (num_elem > 0) {
			if (num_elem % 4) {
				dev_err(&pdev->dev, "invalid len for dynamic_vbat_data\n");
				goto err_exit;
			}
			ret = of_property_read_u32_array(dev_node, "bcl,dynamic_vbat_data", (u32 *)buf, num_elem);
			if (ret) {
				dev_err(&pdev->dev, "dynamic_vbat_data read failed %d\n", ret);
				goto err_exit;
			}

			bcl_perph->dynamic_vbat_config_count = num_elem / 4;
			bcl_perph->dynamic_vbat_config = devm_kcalloc(&pdev->dev,
					bcl_perph->dynamic_vbat_config_count, sizeof(struct dynamic_vbat_data), GFP_KERNEL);
			if (!bcl_perph->dynamic_vbat_config) {
				dev_err(&pdev->dev, "fail to alloc dynamic_vbat_config memory\n");
				goto err_exit;
			}

			for (i = 0; i < bcl_perph->dynamic_vbat_config_count; i++) {
				bcl_perph->dynamic_vbat_config[i].temp = buf[i * 4 + 0];
				bcl_perph->dynamic_vbat_config[i].vbat_mv_lv0 = buf[i * 4 + 1];
				bcl_perph->dynamic_vbat_config[i].vbat_mv_lv1 = buf[i * 4 + 2];
				bcl_perph->dynamic_vbat_config[i].vbat_mv_lv2 = buf[i * 4 + 3];
				dev_err(&pdev->dev, "dynamic_vbat_config[%d]:temp=%d, lv0=%d, lv1=%d, lv2=%d\n", i,
					bcl_perph->dynamic_vbat_config[i].temp,
					bcl_perph->dynamic_vbat_config[i].vbat_mv_lv0,
					bcl_perph->dynamic_vbat_config[i].vbat_mv_lv1,
					bcl_perph->dynamic_vbat_config[i].vbat_mv_lv2);
			}
			return;
		}
	}

err_exit:
	bcl_perph->support_dynamic_vbat = false;
	return;
}
#endif

static int bcl_get_devicetree_data(struct platform_device *pdev,
					struct bcl_device *bcl_perph)
{
	int ret = 0;
	const __be32 *prop = NULL;
	struct device_node *dev_node = pdev->dev.of_node;

	prop = of_get_address(dev_node, 0, NULL, NULL);
	if (prop) {
		bcl_perph->fg_bcl_addr = be32_to_cpu(*prop);
		pr_debug("fg_bcl@%04x\n", bcl_perph->fg_bcl_addr);
	} else {
		dev_err(&pdev->dev, "No fg_bcl registers found\n");
		return -ENODEV;
	}

	bcl_perph->ibat_use_qg_adc =  of_property_read_bool(dev_node,
				"qcom,ibat-use-qg-adc-5a");
	bcl_perph->no_bit_shift =  of_property_read_bool(dev_node,
				"qcom,pmic7-threshold");
	bcl_perph->ibat_ccm_enabled =  of_property_read_bool(dev_node,
						"qcom,ibat-ccm-hw-support");
	bcl_perph->ibat_ccm_lando_enabled = of_property_read_bool(dev_node,
						"qcom,ibat-ccm-lando-hw-support");
	ret = bcl_get_ibat_ext_range_factor(pdev,
					&bcl_perph->ibat_ext_range_factor);

	if (of_property_read_bool(dev_node, "qcom,bcl-mon-vbat-only"))
		bcl_perph->bcl_monitor_type = BCL_MON_VBAT_ONLY;
	else if (of_property_read_bool(dev_node, "qcom,bcl-mon-ibat-only"))
		bcl_perph->bcl_monitor_type = BCL_MON_IBAT_ONLY;
	else
		bcl_perph->bcl_monitor_type = BCL_MON_DEFAULT;

#ifdef OPLUS_FEATURE_CHG_BASIC
	bcl_perph->support_track =  of_property_read_bool(dev_node,"bcl,support_track");
	pr_err("bcl support_track:%d, id:%d\n", bcl_perph->support_track, bcl_perph->id);
	bcl_get_dynamic_vbat_data(pdev, bcl_perph);
#endif
	return ret;
}

static void bcl_fetch_trip(struct platform_device *pdev, enum bcl_dev_type type,
		struct bcl_peripheral_data *data,
		irqreturn_t (*handle)(int, void *))
{
	int ret = 0, irq_num = 0;
	char *int_name = bcl_int_names[type];

	mutex_lock(&data->state_trans_lock);
	data->irq_num = 0;
	data->irq_enabled = false;
	irq_num = platform_get_irq_byname(pdev, int_name);
	if (irq_num > 0 && handle) {
		ret = devm_request_threaded_irq(&pdev->dev,
				irq_num, NULL, handle,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				int_name, data);
		if (ret) {
			dev_err(&pdev->dev,
				"Error requesting trip irq. err:%d\n",
				ret);
			mutex_unlock(&data->state_trans_lock);
			return;
		}
		disable_irq_nosync(irq_num);
		data->irq_num = irq_num;
	} else if (irq_num > 0 && !handle) {
		disable_irq_nosync(irq_num);
		data->irq_num = irq_num;
	}
	mutex_unlock(&data->state_trans_lock);
}

static int vbat_get_default_min_threshold_limit(struct bcl_device *bcl_perph,
				struct bcl_peripheral_data *vbat)
{
	int ret = 0;
	unsigned int val = 0;
	const struct bcl_desc *desc = bcl_perph->desc;

	ret = bcl_read_register(bcl_perph,
			desc->vbat_regs[BCLBIG_COMP_VCMP_L2_THR], &val);
	if (ret < 0 || !val)
		return ret;

	convert_vcmp_idx_to_vbat_val(desc, val, &vbat->def_vbat_min_thresh);

	return 0;
}

static void bcl_vbat_init(struct platform_device *pdev,
		enum bcl_dev_type type, struct bcl_device *bcl_perph)
{
	struct bcl_peripheral_data *vbat = &bcl_perph->param[type];
	unsigned int val = 0;
	int ret;

	mutex_init(&vbat->state_trans_lock);
	vbat->dev = bcl_perph;
	vbat->irq_num = 0;
	vbat->irq_enabled = false;
	vbat->tz_dev = NULL;

	/* If revision 4 or above && bcl support adc, then only enable vbat */
	if (bcl_perph->dig_major >= BCL_GEN3_MAJOR_REV) {
		if (!(bcl_perph->bcl_param_1 & BCL_PARAM_HAS_ADC))
			return;
	} else {
		ret = bcl_read_register(bcl_perph, BCL_VBAT_CONV_REQ, &val);
		if (ret || !val)
			return;
	}
	vbat->ops = vbat_tzd_ops;
	vbat_get_default_min_threshold_limit(bcl_perph, vbat);

	vbat->tz_dev = devm_thermal_of_zone_register(&pdev->dev,
				type, vbat, &vbat->ops);
	if (IS_ERR(vbat->tz_dev)) {
		pr_debug("vbat[%s] register failed. err:%ld\n",
				bcl_int_names[type],
				PTR_ERR(vbat->tz_dev));
		vbat->tz_dev = NULL;
		return;
	}

	ret = thermal_zone_device_enable(vbat->tz_dev);
	if (ret) {
#ifdef OPLUS_FEATURE_CHG_BASIC
		pr_debug("enable thermal_zone_device failed\n");
#endif
		thermal_zone_device_unregister(vbat->tz_dev);
		vbat->tz_dev = NULL;
	}
}

static void bcl_probe_vbat(struct platform_device *pdev,
					struct bcl_device *bcl_perph)
{
	bcl_vbat_init(pdev, BCL_VBAT_LVL0, bcl_perph);
}

static void bcl_ibat_init(struct platform_device *pdev,
			enum bcl_dev_type type, struct bcl_device *bcl_perph)
{
	struct bcl_peripheral_data *ibat = &bcl_perph->param[type];

	mutex_init(&ibat->state_trans_lock);
	ibat->type = type;
	ibat->dev = bcl_perph;
	ibat->irq_num = 0;
	ibat->irq_enabled = false;
	ibat->ops.get_temp = bcl_read_ibat;
	ibat->ops.set_trips = bcl_set_ibat;
	ibat->tz_dev = devm_thermal_of_zone_register(&pdev->dev,
				type, ibat, &ibat->ops);
	if (IS_ERR(ibat->tz_dev)) {
		pr_debug("ibat:[%s] register failed. err:%ld\n",
				bcl_int_names[type],
				PTR_ERR(ibat->tz_dev));
		ibat->tz_dev = NULL;
		return;
	}
	thermal_zone_device_update(ibat->tz_dev, THERMAL_DEVICE_UP);
}

static int bcl_get_ibat_config(struct platform_device *pdev,
		uint32_t *ibat_config)
{
	int ret = 0;
	const char *name;
	struct nvmem_cell *cell;
	size_t len;
	char *buf;

	ret = of_property_read_string(pdev->dev.of_node, "nvmem-cell-names", &name);
	if (ret) {
		*ibat_config = 0;
		pr_debug("Default ibat config enabled %u\n", *ibat_config);
		return 0;
	}

	cell = nvmem_cell_get(&pdev->dev, name);
	if (IS_ERR(cell)) {
		dev_err(&pdev->dev, "failed to get nvmem cell %s\n", name);
		return PTR_ERR(cell);
	}

	buf = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR_OR_NULL(buf)) {
		dev_err(&pdev->dev, "failed to read nvmem cell %s\n", name);
		return PTR_ERR(buf);
	}

	if (len <= 0 || len > sizeof(uint32_t)) {
		dev_err(&pdev->dev, "nvmem cell length out of range %zu\n", len);
		kfree(buf);
		return -EINVAL;
	}
	memcpy(ibat_config, buf, min(len, sizeof(*ibat_config)));
	kfree(buf);

	return 0;
}
static void bcl_probe_ibat(struct platform_device *pdev,
					struct bcl_device *bcl_perph)
{
	uint32_t bcl_config = 0;

	bcl_get_ibat_config(pdev, &bcl_config);

	if (bcl_config == 1) {
		bcl_ibat_init(pdev, BCL_2S_IBAT_LVL0, bcl_perph);
		bcl_ibat_init(pdev, BCL_2S_IBAT_LVL1, bcl_perph);
	} else {
		bcl_ibat_init(pdev, BCL_IBAT_LVL0, bcl_perph);
		bcl_ibat_init(pdev, BCL_IBAT_LVL1, bcl_perph);
	}
}

static void bcl_lvl_init(struct platform_device *pdev,
	enum bcl_dev_type type, int sts_bit_idx, struct bcl_device *bcl_perph)
{
	struct bcl_peripheral_data *lbat = &bcl_perph->param[type];

	mutex_init(&lbat->state_trans_lock);
	lbat->type = type;
	lbat->dev = bcl_perph;
	lbat->status_bit_idx = sts_bit_idx;
	bcl_fetch_trip(pdev, type, lbat, bcl_handle_irq);
	if (lbat->irq_num <= 0)
		return;

	lbat->ops.get_temp = bcl_read_lbat;
	lbat->ops.set_trips = bcl_set_lbat;
	lbat->ops.get_trend = bcl_get_trend;
	lbat->ops.change_mode = qti_tz_change_mode;

	lbat->tz_dev = devm_thermal_of_zone_register(&pdev->dev,
				type, lbat, &lbat->ops);
	if (IS_ERR(lbat->tz_dev)) {
		pr_debug("lbat:[%s] register failed. err:%ld\n",
				bcl_int_names[type],
				PTR_ERR(lbat->tz_dev));
		lbat->tz_dev = NULL;
		return;
	}
	thermal_zone_device_update(lbat->tz_dev, THERMAL_DEVICE_UP);
}

static void bcl_probe_lvls(struct platform_device *pdev,
					struct bcl_device *bcl_perph)
{
	bcl_lvl_init(pdev, BCL_LVL0, BCL_IRQ_L0, bcl_perph);
	bcl_lvl_init(pdev, BCL_LVL1, BCL_IRQ_L1, bcl_perph);
	bcl_lvl_init(pdev, BCL_LVL2, BCL_IRQ_L2, bcl_perph);
}

static int bcl_version_init_and_check(struct bcl_device *bcl_perph)
{
	int ret = 0;
	unsigned int val = 0;

	ret = bcl_read_register(bcl_perph, BCL_REVISION2, &val);
	if (ret < 0)
		return ret;

	bcl_perph->dig_major = val;
	ret = bcl_read_register(bcl_perph, BCL_REVISION1, &val);
	if (ret >= 0)
		bcl_perph->dig_minor = val;

	if (bcl_perph->dig_major >= BCL_GEN3_MAJOR_REV) {
		ret = bcl_read_register(bcl_perph, BCL_PARAM_1, &val);
		if (ret < 0)
			return ret;
		bcl_perph->bcl_param_1 = val;

		val = 0;
		bcl_read_register(bcl_perph, BCL_PARAM_2, &val);
		bcl_perph->bcl_type = val;
	} else {
		bcl_perph->bcl_param_1 = 0;
		bcl_perph->bcl_type = 0;
	}

	if ((bcl_perph->bcl_monitor_type == BCL_MON_VBAT_ONLY) ||
			(bcl_perph->bcl_monitor_type == BCL_MON_IBAT_ONLY)) {
		ret = regmap_read(bcl_perph->regmap, ADC_CMN_BG_ADC_DVDD_SPARE1, &val);
		if (ret < 0) {
			pr_err("Error read reg ADC_CMN_BG_ADC_DVDD_SPARE1, err:%d\n", ret);
			return ret;
		}

		if ((val != 0x71) && (val != 0xF1)) {
			if (bcl_perph->bcl_monitor_type == BCL_MON_VBAT_ONLY) {
				bcl_perph->bcl_monitor_type = BCL_MON_DEFAULT;
				return 0;
			} else if (bcl_perph->bcl_monitor_type == BCL_MON_IBAT_ONLY) {
				pr_debug("BCL monitor Ibat only is not required, value:%d\n",
					val);
				return -ENODEV;
			}
		}
	}

	return 0;
}

static void bcl_configure_bcl_peripheral(struct bcl_device *bcl_perph)
{
	bcl_write_register(bcl_perph, BCL_MONITOR_EN, BIT(7));
}

static void bcl_remove(struct platform_device *pdev)
{
	int i = 0;
	struct bcl_device *bcl_perph =
		(struct bcl_device *)dev_get_drvdata(&pdev->dev);

	for (; i < BCL_TYPE_MAX; i++) {
		if (!bcl_perph->param[i].tz_dev)
			continue;
	}

	if (!bcl_perph->desc->vbat_zone_enabled)
		bcl_pmic5_notifier_unregister(&bcl_perph->nb);
}

#ifdef OPLUS_FEATURE_CHG_BASIC
#define DEFAULT_BATT_TEMP 250
static int bcl_read_battery_temp(struct bcl_device *bcl_perph, int *val)
{
	static struct power_supply *batt_psy;
	union power_supply_propval ret = {0,};
	int rc = 0;

	*val = DEFAULT_BATT_TEMP;
	if (!batt_psy)
		batt_psy = power_supply_get_by_name("battery");
	if (batt_psy) {
		rc = power_supply_get_property(batt_psy,
				POWER_SUPPLY_PROP_TEMP, &ret);
		if (rc) {
			dev_err(bcl_perph->dev, "battery temp read error:%d\n", rc);
			return rc;
		}
		*val = ret.intval;
	} else {
		dev_err(bcl_perph->dev, "get battery psy failed\n");
	}

	return rc;
}

static int battery_supply_callback(struct notifier_block *nb,
			unsigned long event, void *data)
{
	struct power_supply *psy = data;
	struct bcl_device *bcl_perph =
			container_of(nb, struct bcl_device, psy_nb);

	if (strncmp(psy->desc->name, "battery", strlen("battery")) != 0)
		return NOTIFY_OK;
	if (bcl_perph->support_dynamic_vbat)
		schedule_work(&bcl_perph->vbat_check_work);

	return NOTIFY_OK;
}

static void bcl_vbat_check(struct work_struct *work)
{
	struct bcl_device *bcl_perph = container_of(work,
			struct bcl_device, vbat_check_work);
	int batt_temp;
	int i;
	int current_range;
	static int pre_range = -1;
	static int pre_temp = 0;
	struct thermal_zone_device *tz;
	tz = bcl_perph->param[BCL_VBAT_LVL0].tz_dev;
	struct thermal_trip *trip;

	if (!tz) {
		dev_err(bcl_perph->dev, "tz_dev is NULL\n");
		return;
	}

	trip = &tz->trips[BCLBIG_COMP_VCMP_L0_THR].trip;
	if (!trip) {
		dev_err(bcl_perph->dev, "trip is NULL\n");
		return;
	}

	bcl_read_battery_temp(bcl_perph, &batt_temp);

	for (i = 0; i < bcl_perph->dynamic_vbat_config_count; i++) {
		if (batt_temp <= bcl_perph->dynamic_vbat_config[i].temp) {
			current_range = i;
			break;
		}
	}

	if (i == bcl_perph->dynamic_vbat_config_count)
		current_range = bcl_perph->dynamic_vbat_config_count - 1;

	if (abs(pre_temp - batt_temp) > 10 || pre_range != current_range) {
		pre_temp = batt_temp;
		dev_err(bcl_perph->dev, "bcl_vbat_check batt_temp=%d,current_range=%d,pre_range=%d\n",
				batt_temp, current_range, pre_range);
	}

	if (pre_range != current_range && (tz->num_trips == 3)) {
		bcl_write_vbat_tz(tz, &tz->trips[BCLBIG_COMP_VCMP_L0_THR].trip,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv0);
		bcl_write_vbat_tz(tz, &tz->trips[BCLBIG_COMP_VCMP_L1_THR].trip,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv1);
		bcl_write_vbat_tz(tz, &tz->trips[BCLBIG_COMP_VCMP_L2_THR].trip,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv2);
		pre_range = current_range;
		dev_err(bcl_perph->dev, "update bcl vbat batt_temp=%d, lv0=%d, lv1=%d, lv2=%d\n",
				batt_temp,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv0,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv1,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv2);
	}
}

static void pmh0101_bcl_vbat_check(struct work_struct *work)
{
	struct bcl_device *bcl_perph = container_of(work,
			struct bcl_device, vbat_check_work);
	int batt_temp;
	int i;
	int current_range;
	/* pre_range indicates previous volt change range, compare with current range to detect change */
	static int pre_range = -1;
	 /* pre_temp indicates previous temp value, compare with current temp to detect change */
	static int pre_temp = 0;

	bcl_read_battery_temp(bcl_perph, &batt_temp);

	for (i = 0; i < bcl_perph->dynamic_vbat_config_count; i++) {
		if (batt_temp <= bcl_perph->dynamic_vbat_config[i].temp) {
			current_range = i;
			break;
		}
	}
	if (i == bcl_perph->dynamic_vbat_config_count)
		current_range = bcl_perph->dynamic_vbat_config_count - 1;

	if (abs(pre_temp - batt_temp) > 10 || pre_range != current_range) {
		pre_temp = batt_temp;
		dev_err(bcl_perph->dev, "bcl_vbat_check batt_temp=%d,current_range=%d,pre_range=%d\n",
				batt_temp, current_range, pre_range);
	}

	if (pre_range != current_range) {
		oplus_bcl_write_vbat_tz(bcl_perph,BCLBIG_COMP_VCMP_L0_THR,
			bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv0);
		oplus_bcl_write_vbat_tz(bcl_perph,BCLBIG_COMP_VCMP_L1_THR,
			bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv1);
		oplus_bcl_write_vbat_tz(bcl_perph,BCLBIG_COMP_VCMP_L2_THR,
			bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv2);
		pre_range = current_range;
		dev_err(bcl_perph->dev, "update bcl vbat batt_temp=%d, lv0=%d, lv1=%d, lv2=%d\n",
				batt_temp,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv0,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv1,
				bcl_perph->dynamic_vbat_config[current_range].vbat_mv_lv2);
	}
}
#endif

static int bcl_probe(struct platform_device *pdev)
{
	struct bcl_device *bcl_perph = NULL;
	char bcl_name[40];
	int err = 0;
#ifdef OPLUS_FEATURE_CHG_BASIC
	struct proc_dir_entry *proc_node;
#endif

	if (bcl_device_ct >= MAX_PERPH_COUNT) {
		dev_err(&pdev->dev, "Max bcl peripheral supported already.\n");
		return -EINVAL;
	}
	bcl_devices[bcl_device_ct] = devm_kzalloc(&pdev->dev,
					sizeof(*bcl_devices[0]), GFP_KERNEL);
	if (!bcl_devices[bcl_device_ct])
		return -ENOMEM;
	bcl_perph = bcl_devices[bcl_device_ct];
	bcl_perph->dev = &pdev->dev;

#ifdef OPLUS_FEATURE_CHG_BASIC
	bcl_perph->id = bcl_device_ct;
#endif

	bcl_perph->desc = of_device_get_match_data(&pdev->dev);
	if (!bcl_perph->desc)
		return -EINVAL;

	bcl_perph->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!bcl_perph->regmap) {
		dev_err(&pdev->dev, "Couldn't get parent's regmap\n");
		return -EINVAL;
	}

#ifdef OPLUS_FEATURE_CHG_BASIC
	bcl_perph->pmic_type = get_pmic_subtype(bcl_perph); /* get pmic type depends on regmap */
#endif

	bcl_device_ct++;
	err = bcl_get_devicetree_data(pdev, bcl_perph);
	if (err) {
		bcl_device_ct--;
		return err;
	}
	err = bcl_version_init_and_check(bcl_perph);
	if (err) {
		bcl_device_ct--;
		return err;
	}

	switch (bcl_perph->bcl_monitor_type) {
	case BCL_MON_DEFAULT:
		bcl_probe_vbat(pdev, bcl_perph);
		bcl_probe_ibat(pdev, bcl_perph);
		break;
	case BCL_MON_VBAT_ONLY:
		bcl_probe_vbat(pdev, bcl_perph);
		break;
	case BCL_MON_IBAT_ONLY:
		bcl_probe_ibat(pdev, bcl_perph);
		break;
	default:
		bcl_device_ct--;
		return -EINVAL;
	}

	bcl_probe_lvls(pdev, bcl_perph);
	bcl_configure_bcl_peripheral(bcl_perph);

	if (!bcl_perph->desc->vbat_zone_enabled) {
		bcl_perph->nb.notifier_call = bcl_config_vph_cb;
		bcl_pmic5_notifier_register(&bcl_perph->nb);
	}

	dev_set_drvdata(&pdev->dev, bcl_perph);

	snprintf(bcl_name, sizeof(bcl_name), "bcl_0x%04x_%d",
					bcl_perph->fg_bcl_addr,
					bcl_device_ct - 1);

	bcl_perph->ipc_log = ipc_log_context_create(IPC_LOGPAGES,
							bcl_name, 0);
	if (!bcl_perph->ipc_log)
		pr_err("%s: unable to create IPC Logging for %s\n",
					__func__, bcl_name);

#ifdef OPLUS_FEATURE_CHG_BASIC
	oplus_bcl_stat = proc_mkdir("bcl_stat", NULL);
	if (oplus_bcl_stat)
		proc_node = proc_create("bcl_count", 0664, oplus_bcl_stat, &proc_bcl_count);
	else
		dev_err(&pdev->dev, "Couldn't creat oplus bcl_stat\n");

	if (bcl_perph->support_dynamic_vbat) {
		if (bcl_perph->pmic_type == PMIC_SUBTYPE_PMH0101) {
			dev_err(&pdev->dev, "bcl_probe bcl_perph->pmic_type %d init work !\n", bcl_perph->pmic_type);
			INIT_WORK(&bcl_perph->vbat_check_work, pmh0101_bcl_vbat_check);
		} else { /* qcom default support pmih010x, so judge not pmh0101, default init work bac_vbat_check */
			dev_err(&pdev->dev, "bcl_probe bcl_perph->pmic_type %d init work !\n", bcl_perph->pmic_type);
			INIT_WORK(&bcl_perph->vbat_check_work, bcl_vbat_check);
		}

		bcl_perph->psy_nb.notifier_call = battery_supply_callback;
		err = power_supply_reg_notifier(&bcl_perph->psy_nb);
		if (err < 0)
			dev_err(&pdev->dev, "psy notifier register error ret:%d\n", err);
	}
#endif

	return 0;
}

static const struct bcl_desc pmih010x_data = {
	.vadc_type = true,
	.vbat_regs = {
		[BCLBIG_COMP_VCMP_L0_THR]		= 0x48,
		[BCLBIG_COMP_VCMP_L1_THR]		= 0x49,
		[BCLBIG_COMP_VCMP_L2_THR]		= 0x4A,
	},
	.vbat_zone_enabled = true,
	.vcmp_thresh_base = 2250,
	.vcmp_thresh_max = 3600,
};

static const struct bcl_desc pm8550_data = {
	.vadc_type = false,
	.vbat_regs = {
		[BCLBIG_COMP_VCMP_L0_THR]		= 0x47,
		[BCLBIG_COMP_VCMP_L1_THR]		= 0x49,
		[BCLBIG_COMP_VCMP_L2_THR]		= 0x4A,
	},
	.vbat_zone_enabled = false,
	.vcmp_thresh_base = 2250,
	.vcmp_thresh_max = 3600,
};

static const struct bcl_desc pmh0101_data = {
	.vadc_type = false,
	.vbat_regs = {
		[BCLBIG_COMP_VCMP_L0_THR]		= 0x47,
		[BCLBIG_COMP_VCMP_L1_THR]		= 0x49,
		[BCLBIG_COMP_VCMP_L2_THR]		= 0x4A,
	},
#ifdef OPLUS_FEATURE_CHG_BASIC
	.vbat_zone_enabled = true,
#else
	.vbat_zone_enabled = false,
#endif
	.vcmp_thresh_base = 1500,
	.vcmp_thresh_max = 4000,
};

static const struct of_device_id bcl_match[] = {
	{ .compatible = "qcom,bcl-v5", .data = &pmih010x_data},
	{ .compatible = "qcom,pmh0101-bcl-v5", .data = &pmh0101_data},
	{ }
};
MODULE_DEVICE_TABLE(of, bcl_match);

static struct platform_driver bcl_driver = {
	.probe  = bcl_probe,
	.remove = bcl_remove,
	.driver = {
		.name           = BCL_DRIVER_NAME,
		.of_match_table = bcl_match,
	},
};

module_platform_driver(bcl_driver);
MODULE_LICENSE("GPL");
