/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _LEDS_QCOM_FLASH_H
#define _LEDS_QCOM_FLASH_H

#if IS_ENABLED(CONFIG_LEDS_QCOM_FLASH)
int qcom_flash_led_get_max_avail_current(
		struct led_classdev *led_cdev, int *max_current_ma);
#else
static inline int qcom_flash_led_get_max_avail_current(
		struct led_classdev *led_cdev, int *max_current_ma)
{
	return -EOPNOTSUPP;
}
#endif

#endif
