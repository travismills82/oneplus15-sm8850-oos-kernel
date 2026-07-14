/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __LINUX_SPI_CNSS_H
#define __LINUX_SPI_CNSS_H

#include <linux/types.h>
#include "asm-generic/errno-base.h"


#define SPI_SOFT_RESET_BIT  BIT(0)
#define SPI_SLEEP_CMD_BIT  BIT(1)
/*
* Protocol Indicator
*/
/*
enum proto_ind {
	UCI_CMD    = 0x21,
	UCI_DATA   = 0x22,
	PERI_CMD   = 0x31,
	PERI_DATA  = 0x32,
	PERI_EVENT = 0x34,
};
*/
/*
* command type: enum aligned with q2spi
*/
enum cmd_type {
	USER_READ = 2,
	USER_DATA_WRITE = 3,
	USER_WRITE = 5,
	SRESET = 6,
};

/*
* priority type
*/
enum priority_type {
	NORMAL = 0,
	HIGH,
};

/*
* Xfer status
*/
enum xfer_status {
	SUCCESS = 0,
	FAILURE = 1,
	TIMEOUT = 6,
	OTHER,
	INVALID_STATUS = -EINVAL,
};

/*
* spi user request
*/
struct spi_usr_request {
	void *data_buf;
	enum cmd_type cmd;
	u32 addr;
	u8 end_point;
	u8 proto_ind;
	u32 data_len;
	enum priority_type priority;
	u8 flow_id;
	_Bool sync;
	u32 reserved[20];
};

/*
* spi client request
*/
struct spi_client_request {
	void *data_buf;
	u32 data_len;
	u8 end_point;
	u8 proto_ind;
	enum cmd_type cmd;
	enum xfer_status status;
	u8 flow_id;
	u32 reserved[20];
};

#ifdef CONFIG_SPI_LOOPBACK_ENABLED
void notification_to_schedule_wq(void);
#endif

#endif //__LINUX_SPI_CNSS_H
