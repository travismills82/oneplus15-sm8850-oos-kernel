/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_UNPACKER_H__
#define __H_HFI_UNPACKER_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include "hfi_pack_unpack_common.h"

/**
 * HFI_PACK_KEY - Unpack u32 key to Property ID, version and Property value size.
 * @key: Key to unpack.
 * @property_id: Unpacked property ID
 * @version : Unpacked property version
 * @dsize: Unpacked property value size in dwords
 */
#define HFI_UNPACK_KEY(key, property_id, version, size) \
	do {    \
		property_id = key & 0x000FFFFF;                 \
		version     = key & 0x00F00000 >> 20;           \
		size        = key & 0xFF000000 >> 24;           \
	} while (0)

/**
 * hfi_unpacker_get_header_info() - Provides header level info for the command
 *                                  buffer.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @header_info: Pointer to the header info structure. The data in this
 *               structure is populated by the hfi_unpacker_get_header_info()
 *               function.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_unpacker_get_header_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info);

/**
 * hfi_unpacker_get_packet_info() - Provides packet level info for the command
 *                                  buffer.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @packet_num: Packet number in the commands buffer for which the packet
 *              level information is requested. (For eg. If the commands buffer
 *              holds 3 packets, and commands buffer wants packet data for
 *              2nd packet, then packet_num = 2)
 * @packet_info: Pointer to the packet info structure. The data in this
 *               structure is populated by the hfi_unpacker_get_packet_info()
 *               function.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_unpacker_get_packet_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 packet_num, struct hfi_packet_info *packet_info);

/**
 * hfi_unpacker_get_kv_pairs_count() - Provides the number of properties
 *                                     available in the requested
 *                                     packet payload of comamnds buffer.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @packet_info: Packet level information
 * @kv_payload_offset: This field specifies the offset in bytes for <key, value>
 *                   pairs start byte in the hfi packet payload.
 *                   For hfi packets that purely contain just <key, value>
 *                   pairs, this value is 0.
 *                   For some hfi_packets, where <key, value> pairs are added
 *                   following a blob of data, in such cases, this value has to be
 *                   provided which serves as the starting point from the
 *                   hfi packet payload address to indicate the start byte of
 *                   <key, value> pairs.
 *                   For eg: if a blob of data (any struct) size is 2 u32's, and
 *                   <key, value> pairs need to be appended after this blob,
 *                   then this value will be 8 (bytes).
 * @num_props: Pointer to fill the requested <kair, value> pairs count info.
 *             The data in this pointer is populated by the
 *             hfi_unpacker_get_kv_pairs_count() function.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_unpacker_get_kv_pairs_count(
	struct hfi_cmd_buff_hdl *cmd_buf_hdl, struct hfi_packet_info *packet_info,
	u32 kv_payload_offset, u32 *num_props);

/**
 * hfi_unpacker_get_kv_pairs() - Provides array of <keys, value_ptr> for the
 *                               specified number of properties available in the
 *                               requested packet of command buffer.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @packet_info: Packet level information.
 * @kv_pairs_offset: This field specifies the offset in bytes for <key, value>
 *                   pairs start byte in the hfi packet payload.
 *                   For hfi packets that purely contain just <key, value>
 *                   pairs, this value is 0.
 *                   For some hfi_packets, where <key, value> pairs are added
 *                   following a blob of data, in such cases, this value has to be
 *                   provided which serves as the starting point from the
 *                   hfi packet payload address to indicate the start byte of
 *                   <key, value> pairs.
 *                   For eg: if a blob of data (any struct) size is 2 u32's, and
 *                   <key, value> pairs need to be appended after this blob,
 *                   then this value will be 8 (bytes).
 * @num_props: Number of <kv, pairs> info requested for.
 * @kv_pairs: Pointer to the hfi_kv_info structure array. The data in this
 *             structure is populated by the
 *             hfi_unpacker_get_kv_pairs() function.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_unpacker_get_kv_pairs(
	struct hfi_cmd_buff_hdl *cmd_buf_hdl, struct hfi_packet_info *packet_info,
	u32 kv_payload_offset, u32 num_props, struct hfi_kv_info *kv_pairs);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // __H_HFI_UNPACKER_H__
