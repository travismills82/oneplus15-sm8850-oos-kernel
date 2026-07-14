/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PACK_UNPACK_COMMON_H__
#define __H_HFI_PACK_UNPACK_COMMON_H__

#include <linux/string.h>
#include <linux/kernel.h>
#include "hfi_packet.h"

#ifndef u8
typedef uint8_t u8;
#endif
#ifndef u16
typedef uint16_t u16;
#endif
#ifndef u32
typedef uint32_t u32;
#endif
#ifndef u64
typedef uint64_t u64;
#endif

#define hfi_pk_printf(_str, __fmt, ...) pr_err(_str ": " __fmt, ##__VA_ARGS__)

/* 24 bits long */
#define HFI_HEADER_SIZE_MAX                                   0x00FFFFFF
/* 20 bits long */
#define HFI_PACKET_SIZE_MAX                                   0x000FFFFF
/* */
#define HFI_HEADER_CMD_BUFF_TYPE_START_BIT                                   24
#define HFI_PACKET_PAYLOAD_TYPE_START_BIT                                21

#define GET_HEADER_SIZE(data)           \
	(data & ((1 << HFI_HEADER_CMD_BUFF_TYPE_START_BIT) - 1))
#define GET_HEADER_CMD_BUFF_TYPE(data)         \
	(data >> HFI_HEADER_CMD_BUFF_TYPE_START_BIT)
#define GET_PACKET_SIZE(data)           \
	(data & ((1 << HFI_PACKET_PAYLOAD_TYPE_START_BIT) - 1))
#define GET_PACKET_PAYLOAD_TYPE(data)   \
	(data >> HFI_PACKET_PAYLOAD_TYPE_START_BIT)

#define HFI_ERROR                                                      1

/**
 * struct hfi_cmd_buff_hdl - Commands buffer handle
 * @cmd_buffer: Pointer to the command buffer memory
 * @size: Size of the command buffer memory in bytes.
 */
struct hfi_cmd_buff_hdl {
	void *cmd_buffer;
	u32 size;
};

/**
 * struct hfi_kv_info - Provides <key, value> pairs info for the commands
 *                      buffer packet payload.
 * @key: Property_id details (packed with version and dsize info).
 * @value_ptr: Pointer to the value corresponding to the property id
 *             available in the 'key' field.
 */
struct hfi_kv_info {
	u32 key;
	void *value_ptr;
};

/**
 * struct hfi_header_info - Provides commands buffer header level information.
 * @num_packets: Total number of packets available in the command
 *               buffer.
 * @cmd_buff_type: Commands Buffer type identifier for which all
 *                 the Packets are applicable.
 *                 Value of this is one of enum "hfi_cmd_buff_type".
 * @object_id: This is an object identifier for the cmd_buff_type.
 * @header_id: Unique identifier for this Header. This is an opaque identifier
 *              for DCP, as this is populated by the Host, and DCP will return
 *              the same identifier as part of the response.
 */
struct hfi_header_info {
	u32 num_packets;
	enum hfi_cmd_buff_type cmd_buff_type;
	u32 object_id;
	u32 header_id;
};

/**
 * struct hfi_packet_info - Provides commands buffer packet level information.
 * @cmd: Type of command (e.g. HFI_CMD_DISPLAY_GET_PROPERTY).
 * @id: Per command type id (e.g. for display commands it carries the
 *      display-id, for layer-commands it carries the layer-id, depending
 *      on the 'cmd' type)
 * @flags: Packet flags.
 * @packet_id: unique id of the packet.
 * @payload_type: Type of the payload of the packet.
 * @payload_size: Size of the payload of the packet in bytes.
 * @payload_ptr: Pointer to the payload of the packet.
 */
struct hfi_packet_info {
	u32 cmd;
	u32 id;
	u32 flags;
	u32 packet_id;
	enum hfi_packet_payload_type payload_type;
	u32 payload_size;
	void *payload_ptr;
};

#endif // __H_HFI_PACK_UNPACK_COMMON_H__
