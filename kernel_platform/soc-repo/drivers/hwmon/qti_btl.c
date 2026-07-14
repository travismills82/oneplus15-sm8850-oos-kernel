// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/device.h>
#include <linux/fixp-arith.h>
#include <linux/hwmon.h>
#include <linux/kernel.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define BATT_TEMP_UPDATED_TH_TIME		0x48

#define BATT_TEMP_CONFIG_REG			0x49
  #define BATT_TEMP_LOG_MODE_MASK		GENMASK(3, 0)
  #define BATT_TEMP_LOG_MODE_OFF_MASK		GENMASK(0, 0)
  #define BATT_TEMP_LOG_MODE_ON_MASK		GENMASK(1, 1)
  #define BATT_THERM_PULL_UP_MASK		GENMASK(7, 4)

#define BATT_TEMP_LOGGING_ENABLE_REG		0x4A

#define BATT_TEMP_SAMPLE_INTERVAL_REG		0x4B
  #define BTL_SAMPLE_INTERVAL_STEP_MS		100
  #define BTL_SAMPLE_INTERVAL_MIN_MS		100
  #define BTL_SAMPLE_INTERVAL_MAX_MS		25500

#define BATT_TEMP_THRESHOLD_LT_REG		0x58
  #define BATT_TEMP_THRESHOLD_REG_LEN		2

#define BATT_TEMP_MIN_TEMP_REG			0x5C
#define BATT_TEMP_MAX_MIN_REG_LEN		2
  #define BATT_TEMP_MIN_TEMP_INIT_VAL		0
  #define BATT_TEMP_MIN_TEMP			-40000

#define BATT_TEMP_MAX_TEMP_REG			0x5E
  #define BATT_TEMP_MAX_TEMP_INIT_VAL		0xFFFF
  #define BATT_TEMP_MAX_TEMP			98000

#define BATT_TEMP_DATA_LOGGING_UT_REG		0x7C
  #define BATT_TEMP_DATA_LOGGING_REG_LEN	4
  #define BATT_TEMP_DATA_LOGGING_INIT_VAL	0

#define BATT_TEMP_PBS_TRIG_SET			0xE5

#define BTL_THRESHOLD_LIST_SIZE			7
#define BTL_TEMP_RANGE_COUNT			8

enum btl_batt_therm_adc_pull_up {
	RESISTANCE_PU_100K = 0,
	RESISTANCE_PU_10K,
};

struct batt_temp_log {
	struct device *dev;
	struct nvmem_device *nvmem;
	u8 sample_interval;
	int temp_threshold[BTL_THRESHOLD_LIST_SIZE];
	u32 temp_count[BTL_TEMP_RANGE_COUNT];
	u8 mode;
	const struct vadc_map_pt *map;
	int map_size;
	bool updated_th_time;
};

struct vadc_map_pt {
	s32 x;
	s32 y;
};

/* ADC to temperature table for 100k batt_therm. */
static const struct vadc_map_pt adc_gen4_batt_therm_100k_map[] = {
	{ 0xfb46,	-40000 },
	{ 0xfa80,	-38000 },
	{ 0xf99e,	-36000 },
	{ 0xf89d,	-34000 },
	{ 0xf77b,	-32000 },
	{ 0xf634,	-30000 },
	{ 0xf4c5,	-28000 },
	{ 0xf329,	-26000 },
	{ 0xf15f,	-24000 },
	{ 0xef61,	-22000 },
	{ 0xed2e,	-20000 },
	{ 0xeac0,	-18000 },
	{ 0xe816,	-16000 },
	{ 0xe52d,	-14000 },
	{ 0xe201,	-12000 },
	{ 0xde92,	-10000 },
	{ 0xdadf,	-8000 },
	{ 0xd6e5,	-6000 },
	{ 0xd2a7,	-4000 },
	{ 0xce24,	-2000 },
	{ 0xc95f,	0 },
	{ 0xc45b,	2000 },
	{ 0xbf1b,	4000 },
	{ 0xb9a4,	6000 },
	{ 0xb3fb,	8000 },
	{ 0xae28,	10000 },
	{ 0xa82f,	12000 },
	{ 0xa21a,	14000 },
	{ 0x9bf0,	16000 },
	{ 0x95b8,	18000 },
	{ 0x8f7b,	20000 },
	{ 0x8941,	22000 },
	{ 0x8311,	24000 },
	{ 0x7cf3,	26000 },
	{ 0x76ed,	28000 },
	{ 0x7105,	30000 },
	{ 0x6b41,	32000 },
	{ 0x65a6,	34000 },
	{ 0x6037,	36000 },
	{ 0x5af8,	38000 },
	{ 0x55ec,	40000 },
	{ 0x5113,	42000 },
	{ 0x4c71,	44000 },
	{ 0x4804,	46000 },
	{ 0x43cd,	48000 },
	{ 0x3fcc,	50000 },
	{ 0x3c01,	52000 },
	{ 0x3869,	54000 },
	{ 0x3504,	56000 },
	{ 0x31d0,	58000 },
	{ 0x2ecb,	60000 },
	{ 0x2bf4,	62000 },
	{ 0x2948,	64000 },
	{ 0x26c5,	66000 },
	{ 0x246a,	68000 },
	{ 0x2234,	70000 },
	{ 0x2022,	72000 },
	{ 0x1e31,	74000 },
	{ 0x1c60,	76000 },
	{ 0x1aac,	78000 },
	{ 0x1914,	80000 },
	{ 0x1796,	82000 },
	{ 0x1630,	84000 },
	{ 0x14e2,	86000 },
	{ 0x13a9,	88000 },
	{ 0x1284,	90000 },
	{ 0x1172,	92000 },
	{ 0x1071,	94000 },
	{ 0xf80,	96000 },
	{ 0xe9f,	98000 }
};

/* ADC to temperature table for 10k batt_therm */
static const struct vadc_map_pt adc_gen4_batt_therm_10k_map[] = {
	{ 0xf617,	-40000 },
	{ 0xf4d3,	-38000 },
	{ 0xf36e,	-36000 },
	{ 0xf1e6,	-34000 },
	{ 0xf03a,	-32000 },
	{ 0xee66,	-30000 },
	{ 0xec6a,	-28000 },
	{ 0xea43,	-26000 },
	{ 0xe7f0,	-24000 },
	{ 0xe56f,	-22000 },
	{ 0xe2c0,	-20000 },
	{ 0xdfe1,	-18000 },
	{ 0xdcd2,	-16000 },
	{ 0xd993,	-14000 },
	{ 0xd624,	-12000 },
	{ 0xd285,	-10000 },
	{ 0xceb9,	-8000 },
	{ 0xcabf,	-6000 },
	{ 0xc69b,	-4000 },
	{ 0xc24e,	-2000 },
	{ 0xbddb,	0 },
	{ 0xb947,	2000 },
	{ 0xb493,	4000 },
	{ 0xafc4,	6000 },
	{ 0xaadd,	8000 },
	{ 0xa5e4,	10000 },
	{ 0xa0dd,	12000 },
	{ 0x9bcc,	14000 },
	{ 0x96b5,	16000 },
	{ 0x919e,	18000 },
	{ 0x8c89,	20000 },
	{ 0x877c,	22000 },
	{ 0x827b,	24000 },
	{ 0x7d89,	26000 },
	{ 0x78aa,	28000 },
	{ 0x73e0,	30000 },
	{ 0x6f2f,	32000 },
	{ 0x6a99,	34000 },
	{ 0x6620,	36000 },
	{ 0x61c6,	38000 },
	{ 0x5d8d,	40000 },
	{ 0x5975,	42000 },
	{ 0x5580,	44000 },
	{ 0x51ae,	46000 },
	{ 0x4e00,	48000 },
	{ 0x4a74,	50000 },
	{ 0x470d,	52000 },
	{ 0x43c9,	54000 },
	{ 0x40a7,	56000 },
	{ 0x3da7,	58000 },
	{ 0x3ac9,	60000 },
	{ 0x380c,	62000 },
	{ 0x356e,	64000 },
	{ 0x32ef,	66000 },
	{ 0x308e,	68000 },
	{ 0x2e4a,	70000 },
	{ 0x2c21,	72000 },
	{ 0x2a13,	74000 },
	{ 0x281f,	76000 },
	{ 0x2643,	78000 },
	{ 0x247e,	80000 },
	{ 0x22d0,	82000 },
	{ 0x2137,	84000 },
	{ 0x1fb2,	86000 },
	{ 0x1e41,	88000 },
	{ 0x1ce2,	90000 },
	{ 0x1b95,	92000 },
	{ 0x1a59,	94000 },
	{ 0x192c,	96000 },
	{ 0x180f,	98000 }
};

static int adc_to_temp_map(struct batt_temp_log *btl, int adc_val)
{
	int i = 0;
	int temp;

	while (i < btl->map_size && btl->map[i].x > adc_val)
		i++;

	if (i == 0)
		temp = btl->map[0].y;
	else if (i == btl->map_size)
		temp = btl->map[btl->map_size - 1].y;
	else
		temp = fixp_linear_interpolate(btl->map[i - 1].x, btl->map[i - 1].y,
					       btl->map[i].x, btl->map[i].y, adc_val);

	return temp;
}

static u64 temp_to_adc_map(struct batt_temp_log *btl, int temp)
{
	int adc_val;
	int i = 0;

	while (i < btl->map_size && btl->map[i].y < temp)
		i++;

	if (i == 0)
		adc_val = btl->map[0].x;
	else if (i == btl->map_size)
		adc_val = btl->map[btl->map_size - 1].x;
	else
		adc_val = fixp_linear_interpolate(btl->map[i - 1].y, btl->map[i - 1].x,
						     btl->map[i].y, btl->map[i].x, temp);

	return adc_val;
}

static int btl_read_temp_counts(struct batt_temp_log *btl)
{
	int i, ret;
	u32 data;
	u8 reg;

	for (i = 0; i < BTL_TEMP_RANGE_COUNT; i++) {
		reg = BATT_TEMP_DATA_LOGGING_UT_REG - (BATT_TEMP_DATA_LOGGING_REG_LEN * i);
		ret = nvmem_device_read(btl->nvmem, reg, BATT_TEMP_DATA_LOGGING_REG_LEN, &data);
		if (ret < 0)
			return ret;

		btl->temp_count[i] = data;
	}

	return ret;
}

static int btl_reset_temp_counts(struct batt_temp_log *btl)
{
	int i, ret, val;
	u8 reg;

	val = BATT_TEMP_MIN_TEMP_INIT_VAL;
	reg = BATT_TEMP_MIN_TEMP_REG;
	ret = nvmem_device_write(btl->nvmem, reg, BATT_TEMP_MAX_MIN_REG_LEN, &val);
	if (ret < 0)
		return ret;

	val = BATT_TEMP_MAX_TEMP_INIT_VAL;
	reg = BATT_TEMP_MAX_TEMP_REG;
	ret = nvmem_device_write(btl->nvmem, reg, BATT_TEMP_MAX_MIN_REG_LEN, &val);
	if (ret < 0)
		return ret;

	val = BATT_TEMP_DATA_LOGGING_INIT_VAL;
	for (i = 0; i < BTL_TEMP_RANGE_COUNT; i++) {
		reg = BATT_TEMP_DATA_LOGGING_UT_REG - (BATT_TEMP_DATA_LOGGING_REG_LEN * i);
		ret = nvmem_device_write(btl->nvmem, reg, BATT_TEMP_DATA_LOGGING_REG_LEN, &val);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int btl_read_temp_record(struct batt_temp_log *btl, unsigned int reg, int *temp)
{
	u16 adc_val;
	int ret;

	ret = nvmem_device_read(btl->nvmem, reg, 2, &adc_val);
	if (ret < 0)
		return ret;

	*temp = adc_to_temp_map(btl, adc_val);

	return 0;
}

static int btl_read_temp_thresholds(struct batt_temp_log *btl)
{
	int i, ret;
	u8 reg;

	for (i = 0; i < BTL_THRESHOLD_LIST_SIZE; i++) {
		reg = BATT_TEMP_THRESHOLD_LT_REG - (BATT_TEMP_THRESHOLD_REG_LEN * i);
		ret = btl_read_temp_record(btl, reg, &btl->temp_threshold[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int btl_write_temp_thresholds(struct batt_temp_log *btl, int *temp_threshold)
{
	u16 adc_val;
	int i, ret;
	u8 reg;

	for (i = 0; i < BTL_THRESHOLD_LIST_SIZE; i++) {
		adc_val = temp_to_adc_map(btl, temp_threshold[i + 1]);
		reg = BATT_TEMP_THRESHOLD_LT_REG - (BATT_TEMP_THRESHOLD_REG_LEN * i);
		ret = nvmem_device_write(btl->nvmem, reg, 2, &adc_val);
		if (ret < 0)
			return ret;

		btl->temp_threshold[i] = temp_threshold[i + 1];
	}

	return ret;
}

static int validate_thresholds(struct batt_temp_log *btl, int *threshold)
{
	int i;

	if (threshold[0] != BATT_TEMP_MIN_TEMP) {
		dev_err(btl->dev, "Error, Temperature threshold should start with %d\n",
			BATT_TEMP_MIN_TEMP);
		return -EINVAL;
	}

	for (i = 1; i < BTL_TEMP_RANGE_COUNT; i++) {
		if (threshold[i] > BATT_TEMP_MAX_TEMP || threshold[i] < BATT_TEMP_MIN_TEMP) {
			dev_err(btl->dev, "Error, Temperature threshold should be within [%d %d]\n",
				BATT_TEMP_MIN_TEMP, BATT_TEMP_MAX_TEMP);
			return -EINVAL;
		}

		if (threshold[i] <= threshold[i - 1]) {
			dev_err(btl->dev, "Error, Temperature thresholds should be in increasing order\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int btl_enable_logging(struct batt_temp_log *btl)
{
	u8 val = 1;
	int ret;

	if (!(btl->mode & BATT_TEMP_LOG_MODE_ON_MASK))
		return 0;

	ret = nvmem_device_write(btl->nvmem, BATT_TEMP_LOGGING_ENABLE_REG, 1, &val);
	if (ret < 0)
		return ret;

	return nvmem_device_write(btl->nvmem, BATT_TEMP_PBS_TRIG_SET, 1, &val);
}

static int btl_disable_logging(struct batt_temp_log *btl)
{
	u8 val = 0;
	int ret;

	ret = nvmem_device_write(btl->nvmem, BATT_TEMP_LOGGING_ENABLE_REG, 1, &val);
	if (ret < 0)
		return ret;

	val = 1;
	return nvmem_device_write(btl->nvmem, BATT_TEMP_PBS_TRIG_SET, 1, &val);
}

static int btl_set_temp_thresholds(struct batt_temp_log *btl, int *threshold)
{
	u8 val = 1;
	int ret;

	ret = validate_thresholds(btl, threshold);
	if (ret < 0)
		return ret;

	ret = btl_disable_logging(btl);
	if (ret < 0)
		return ret;

	ret = btl_reset_temp_counts(btl);
	if (ret < 0)
		return ret;

	ret = btl_write_temp_thresholds(btl, threshold);
	if (ret < 0)
		return ret;

	if (!btl->updated_th_time) {
		ret = nvmem_device_write(btl->nvmem, BATT_TEMP_UPDATED_TH_TIME, 1, &val);
		if (ret < 0)
			return ret;
		btl->updated_th_time = true;
	}

	return btl_enable_logging(btl);
}

static int btl_set_sample_interval_ms(struct batt_temp_log *btl, int interval_ms)
{
	u8 sample_interval;
	u8 val = 1;
	int ret;

	if (interval_ms > BTL_SAMPLE_INTERVAL_MAX_MS ||
	    interval_ms < BTL_SAMPLE_INTERVAL_MIN_MS) {
		dev_err(btl->dev, "Error, sample interval should be within [%u %u]\n",
			BTL_SAMPLE_INTERVAL_MIN_MS, BTL_SAMPLE_INTERVAL_MAX_MS);
		return -EINVAL;
	}

	ret = btl_disable_logging(btl);
	if (ret < 0)
		return ret;

	ret = btl_reset_temp_counts(btl);
	if (ret < 0)
		return ret;

	sample_interval = interval_ms / BTL_SAMPLE_INTERVAL_STEP_MS;
	ret = nvmem_device_write(btl->nvmem, BATT_TEMP_SAMPLE_INTERVAL_REG, 1, &sample_interval);
	if (ret < 0)
		return ret;

	btl->sample_interval = sample_interval;

	if (!btl->updated_th_time) {
		ret = nvmem_device_write(btl->nvmem, BATT_TEMP_UPDATED_TH_TIME, 1, &val);
		if (ret < 0)
			return ret;
		btl->updated_th_time = true;
	}

	return btl_enable_logging(btl);
}

static ssize_t sample_interval_ms_show(struct device *dev, struct device_attribute *attr,
				       char *buf)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 btl->sample_interval * BTL_SAMPLE_INTERVAL_STEP_MS);
}

static ssize_t sample_interval_ms_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	u16 val;
	int ret;

	if (kstrtou16(buf, 0, &val))
		return -EINVAL;

	ret = btl_set_sample_interval_ms(btl, val);
	if (ret < 0)
		return ret;

	return count;
}
DEVICE_ATTR_RW(sample_interval_ms);

static ssize_t max_temperature_show(struct device *dev, struct device_attribute *attr,
				    char *buf)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	int temp, ret;

	ret = btl_read_temp_record(btl, BATT_TEMP_MAX_TEMP_REG, &temp);
	if (ret < 0)
		return ret;

	return scnprintf(buf, PAGE_SIZE, "%d\n", temp);
}
DEVICE_ATTR_RO(max_temperature);

static ssize_t min_temperature_show(struct device *dev, struct device_attribute *attr,
				    char *buf)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	int temp, ret;

	ret = btl_read_temp_record(btl, BATT_TEMP_MIN_TEMP_REG, &temp);
	if (ret < 0)
		return ret;

	return scnprintf(buf, PAGE_SIZE, "%d\n", temp);
}
DEVICE_ATTR_RO(min_temperature);

static ssize_t temperature_counter_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	int count = 0;
	int i, ret;

	ret = btl_read_temp_counts(btl);
	if (ret < 0)
		return ret;

	for (i = 0; i < BTL_TEMP_RANGE_COUNT; i++)
		count += scnprintf(buf + count, PAGE_SIZE - count, "%u ", btl->temp_count[i]);

	buf[count - 1] = '\n';

	return count;
}
DEVICE_ATTR_RO(temperature_counter);

static ssize_t temperature_threshold_show(struct device *dev, struct device_attribute *attr,
					  char *buf)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	int count = 0;
	int i;

	count += scnprintf(buf + count, PAGE_SIZE - count, "%i ", BATT_TEMP_MIN_TEMP);
	for (i = 0; i < BTL_THRESHOLD_LIST_SIZE; i++)
		count += scnprintf(buf + count, PAGE_SIZE - count, "%d ", btl->temp_threshold[i]);

	buf[count - 1] = '\n';

	return count;
}

static ssize_t temperature_threshold_store(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct batt_temp_log *btl = dev_get_drvdata(dev);
	int threshold[BTL_TEMP_RANGE_COUNT];
	int i, pos, val, ret;
	int offset = 0;

	for (i = 0; i < BTL_TEMP_RANGE_COUNT; i++) {
		ret = sscanf(buf + offset, "%d %n", &val, &pos);
		if (ret != 1) {
			dev_err(btl->dev, "Failed to read temperature threshold list\n");
			return -EINVAL;
		}

		offset += pos;
		threshold[i] = val;
	}

	ret = btl_set_temp_thresholds(btl, threshold);
	if (ret < 0)
		return ret;

	return count;
}
DEVICE_ATTR_RW(temperature_threshold);

static struct attribute *qti_btl_attrs[] = {
	&dev_attr_max_temperature.attr,
	&dev_attr_min_temperature.attr,
	&dev_attr_sample_interval_ms.attr,
	&dev_attr_temperature_threshold.attr,
	&dev_attr_temperature_counter.attr,
	NULL,
};

static const struct attribute_group qti_btl_group = {
	.name = "qti_btl",
	.attrs = qti_btl_attrs,
};
__ATTRIBUTE_GROUPS(qti_btl);

static int btl_config(struct batt_temp_log *btl)
{
	int threshold_list[BTL_TEMP_RANGE_COUNT];
	int ret, sample_ms, size;
	u8 reg;

	ret = nvmem_device_read(btl->nvmem, BATT_TEMP_CONFIG_REG, 1, &reg);
	if (ret < 0)
		return ret;
	btl->mode = FIELD_GET(BATT_TEMP_LOG_MODE_MASK, reg);

	if (FIELD_GET(BATT_THERM_PULL_UP_MASK, reg) == RESISTANCE_PU_10K) {
		btl->map = adc_gen4_batt_therm_10k_map;
		btl->map_size = ARRAY_SIZE(adc_gen4_batt_therm_10k_map);
	} else {
		btl->map = adc_gen4_batt_therm_100k_map;
		btl->map_size = ARRAY_SIZE(adc_gen4_batt_therm_100k_map);
	}

	ret = btl_read_temp_thresholds(btl);
	if (ret < 0)
		return ret;

	ret = nvmem_device_read(btl->nvmem, BATT_TEMP_SAMPLE_INTERVAL_REG,
				1, &btl->sample_interval);
	if (ret < 0)
		return ret;

	ret = nvmem_device_read(btl->nvmem, BATT_TEMP_UPDATED_TH_TIME, 1, &reg);
	if (ret < 0)
		return ret;
	btl->updated_th_time = reg;

	if (btl->updated_th_time)
		return 0;

	if (of_property_present(btl->dev->of_node, "qcom,temperature-thresholds")) {
		size = of_property_count_u32_elems(btl->dev->of_node,
						   "qcom,temperature-thresholds");
		if (size != BTL_TEMP_RANGE_COUNT) {
			dev_err(btl->dev, "Error, Incorrect temperature thresholds list size\n");
			return -EINVAL;
		}

		ret = of_property_read_u32_array(btl->dev->of_node, "qcom,temperature-thresholds",
						 threshold_list, BTL_TEMP_RANGE_COUNT);
		if (ret < 0)
			return ret;

		ret = btl_set_temp_thresholds(btl, threshold_list);
		if (ret < 0)
			return ret;
	}

	if (of_property_present(btl->dev->of_node, "qcom,sample-interval-ms")) {
		ret = of_property_read_u32(btl->dev->of_node, "qcom,sample-interval-ms",
					   &sample_ms);
		if (ret < 0)
			return ret;

		ret = btl_set_sample_interval_ms(btl, sample_ms);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int qti_btl_probe(struct platform_device *pdev)
{
	struct batt_temp_log *btl;
	struct device *hwmon_dev;
	int ret;

	btl = devm_kzalloc(&pdev->dev, sizeof(*btl), GFP_KERNEL);
	if (!btl)
		return -ENOMEM;

	btl->dev = &pdev->dev;
	dev_set_drvdata(btl->dev, btl);

	btl->nvmem = devm_nvmem_device_get(btl->dev, "btl-sdam");
	if (IS_ERR(btl->nvmem))
		return dev_err_probe(btl->dev, PTR_ERR(btl->nvmem),
				     "Failed to get btl SDAM device\n");

	ret = btl_config(btl);
	if (ret < 0)
		return ret;

	hwmon_dev = devm_hwmon_device_register_with_groups(&pdev->dev, "qti_btl",
							   btl, qti_btl_groups);
	if (IS_ERR(hwmon_dev))
		return dev_err_probe(btl->dev, PTR_ERR(hwmon_dev),
				     "Failed to register hwmon device for qti_btl\n");

	return 0;
}

static const struct of_device_id qti_btl_match_table[] = {
	{ .compatible = "qcom,btl" },
	{},
};

static struct platform_driver qti_btl_driver = {
	.driver = {
		.name = "qti_btl",
		.of_match_table = qti_btl_match_table,
	},
	.probe = qti_btl_probe,
};
module_platform_driver(qti_btl_driver);

MODULE_DESCRIPTION("QTI Battery Temperature Logging driver");
MODULE_LICENSE("GPL");
