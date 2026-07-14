/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define NUM_TRS_ERS_URG_CMD_REGS 8

/* Context values used to select SMMU CB */
#define DEFAULT_SMMU_CONTEXT 0

/* Transfer request opcodes */
#define COPY		0
#define COMP		1
#define DECOMP		2
#define COMP_DRY_RUN	3
#define COMP_AND_COPY	4
#define COMP_OVERFLOW_DRY_RUN	5

/* Event status codes */
#define OP_OK		0x0
#define OP_BUS_ERROR	0x1
#define COMP_TOO_BIG	0x2
#define DECOMP_CRC_ERR	0x3
#define OP_NOOP		0x4
#define OP_TIMED_OUT	0x5
#define OP_URG_ONGOING	0xf

/* LLC cache allocation hints */
#define LLC_NO_CACHE_OP 0x0
#define LLC_ALLOC	0x1

#define URG_CMD_CONTEXT_SPACING 0x10

/* Core-enablement values */
#define QPACE_RUN 0x1
#define QPACE_STOP 0x0
#define QPACE_STATE_CHANGE_TIMEOUT_US 10000

/* Register physical offsets */
#define QPACE_GEN_REGS_BASE_ADDR		0x31400000

#define QPACE_GEN_CORE_REGS_BASE_ADDR		0x31401000
#define QPACE_COMP_CORE_REGS_BASE_ADDR		0x31402000
#define QPACE_DECOMP_CORE_REGS_BASE_ADDR	0x31406000
#define QPACE_COPY_CORE_REGS_BASE_ADDR		0x3140A000

#define QPACE_URG_REGS_BASE_ADDR		0x31410000
#define QPACE_TR_REGS_BASE_ADDR			0x31430000
#define QPACE_ER_REGS_BASE_ADDR			0x31450000
#define QPACE_EE_REGS_BASE_ADDR			0x31470000

