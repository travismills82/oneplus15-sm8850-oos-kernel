/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef __LINUX_THQSPI_H
#define __LINUX_THQSPI_H

#include <linux/types.h>
#include "asm-generic/errno-base.h"

#define NUM_BYTES_VS_SPINEL_HEADER  (3) 
/*
 * command type: enum aligned with q2spi
 */
enum thqspi_cmdtype {
	THQSPI_WRITE      = 1,
	THQSPI_CONFIGURE  = 2,
};

/*
 * thqspi_header
 */
struct thqspi_header {
	uint8_t  protId;        // Thread protocol identifier
	uint8_t  flag_group;
	uint8_t  packet_type;
	uint16_t length;
} __attribute__((packed));

/*
 * thqspi user request
 */
struct thqspi_user_request {
	enum thqspi_cmdtype cmd_type;
	bool add_header;    // true, if driver needs to pre-pend header
	struct thqspi_header header;
	void *data_buf;    // byte array of size packetLen
	uint32_t reserved[4];
} __attribute__((packed));

/*
 * thqspi client request: Passed by user space driver when it calls read
 */
struct thqspi_client_request {
	uint16_t length; // Length of packet. It is 3 more than the payload from controller.
	                 // In mission mode, transport driver will ignore initial 3 bytes in data_buf.
	                 // In FTM mode, transport driver will use initial 3 bytes for setting the
	                 // vendor specific SPINEL header, before pushing the msg to FTM app.
	uint8_t *data_buf;
	uint32_t reserved[4];
} __attribute__((packed));

#endif //__LINUX_THQSPI_H
