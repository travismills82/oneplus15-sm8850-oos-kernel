/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _GUNYAH_TRACE_H
#define _GUNYAH_TRACE_H

#define GUNYAH_TRACE_BUF_MAGIC 0x46554236
#define ENTRY_SIZE 64
#define MAX_CLASS_SIZE 128

#define ADDRSPACE_INFO_TRACE_INFO_ID 1

/**
 * struct gunyah_trace_irq_info - IRQ information for Gunyah trace
 * @type: Type of the IRQ
 * @irq: IRQ number
 * @flags: Flags associated with the IRQ
 * @res0: Reserved field
 */
struct gunyah_trace_irq_info {
	__le32 type;
	__le32 irq;
	__le32 flags;
	__le32 res0;
};

/**
 * struct gunyah_trace_info - Main information about a Gunyah trace provided by address info
 * @trace_ipa: IPA of the trace buffer
 * @trace_size: Size of the trace buffer
 * @trace_dbl_irq: IRQ information for gunyah trace
 */
struct gunyah_trace_info {
	__le64 trace_ipa;
	__le64 trace_size;
	struct gunyah_trace_irq_info trace_dbl_irq;
};

/**
 * struct gunyah_trace_class_bitmap - A struct representing the relation between bitmap and
 * class name
 * @bit: The bitmap value.
 * @name: The name of the trace class.
 */
struct gunyah_trace_class_bitmap {
	uint64_t bit;
	const char *name;
};

enum gunyah_trace_class_type {
	GUNYAH_TRACE_CLASS_ERROR = 0,
	GUNYAH_TRACE_CLASS_DEBUG,
	GUNYAH_TRACE_CLASS_USER,
	GUNYAH_TRACE_CLASS_TRACE_LOG_BUFFER,
	GUNYAH_TRACE_CLASS_LOG_BUFFER,
	GUNYAH_TRACE_CLASS_INFO,
	GUNYAH_TRACE_CLASS_MEMDB,
	GUNYAH_TRACE_CLASS_PROFILE_LEVEL1,
	GUNYAH_TRACE_CLASS_PROFILE_LEVEL2,
	GUNYAH_TRACE_CLASS_PSCI,
	GUNYAH_TRACE_CLASS_VGIC,
	GUNYAH_TRACE_CLASS_VGIC_DEBUG,
	GUNYAH_TRACE_CLASS_WAIT_QUEUE,
	GUNYAH_TRACE_CLASS_MAX
};

const struct gunyah_trace_class_bitmap gunyah_trace_class_bitmap[] = {
	{ 0,	"error" },
	{ 1,	"debug" },
	{ 2,	"user" },
	{ 4,	"trace_log_buffer" },
	{ 5,	"log_buffer" },
	{ 6,	"info" },
	{ 7,	"memdb" },
	{ 8,	"profile_level1" },
	{ 9,	"profile_level2" },
	{ 16,	"psci" },
	{ 17,	"vgic" },
	{ 18,	"vgic_debug" },
	{ 32,	"wait_queue" },
};

/**
 * struct gunyah_trace_header - Header structure for Gunyah trace buffer
 * @buf_magic: Magic number to identify the trace buffer format
 * @entry_num: Number of entries in the trace buffer
 * @cpu_mask: Bitmap indicating which CPUs are being traced
 * @index: Current entry index in the trace buffer
 * @notify_mask: Mask for notification events
 * @wrap_count: Number of times the trace buffer has wrapped around
 * @res0_2: Reserved bytes for future use
 * @flags: Flags for trace buffer configuration
 * @version: Version of the trace buffer format
 */
struct gunyah_trace_header {
	__le32 buf_magic;
	__le32 entry_num;
	__le64 cpu_mask[4];
	__le32 index;
	__le32 notify_mask;
	__le32 wrap_count;
	uint8_t res0_2[8];
	__le16 flags;
	__le16 version;
};

#endif
