/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PACKER_H__
#define __H_HFI_PACKER_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include "hfi_pack_unpack_common.h"

/**
 * HFI_PACK_KEY - Pack given property id, version and dsize to u32 key.
 * @key: Packed key with given property id, version and dsize
 * @property_id: Property ID to pack
 * @version : Property version to pack
 * @dsize: Property value size in dwords
 */
#define HFI_PACK_KEY(property_id, version, dsize) \
	(property_id | (version << 20) | (dsize << 24))

/**
 * hfi_create_header() - Creates the header for the command buffer.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @header_info: Pointer to the header info structure. The data in this
 *               structure is populated by the caller of this API.
 *               Since this API creates just the header, and does not
 *               add any hfi packets to it, "num_packets" field should be 0.
 *               Otherwise, this field is ignored by the API.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_create_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info);

/**
 * hfi_create_full_packet() - Creates a hfi packet along with provided
 *                            payload under the hfi header available in
 *                            cmd_buf_desc.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @packet_info: Pointer to the packet info structure. The data in this
 *               structure is populated by the caller of this API.
 *               This API is not meant for <key, value> pair packing.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_create_full_packet(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info);

/**
 * hfi_create_packet_header() - Creates just the hfi packet for the provided
 *                            packet info with no payload. This API ignores
 *                            any payload provided at packet_info.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @packet_info: Pointer to the packet info structure. The data in this
 *               structure is populated by the caller of this API.
 *
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_create_packet_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info);

/**
 * hfi_append_packet_with_kv_pairs(): Appends given <key, value> pairs under
 *                               the last hfi packet available at cmd_buf_desc
 *                               if the last packet's 'cmd' and 'payload type'
 *                               matches with the provided parameters 'cmd' and
 *                               'payload type'.
 *                               If no packet is available, this API
 *                               cannot append the given <key, value> pairs
 *                               and returns error. Hence, for the caller
 *                               to use this API, caller should have created
 *                               corresponding packet header by using
 *                               hfi_create_packet_header() API.
 * @cmd_buf_hdl: Pointer to the commands buffer handle.
 * @cmd: Packet command ID
 * @payload_type: Type of payload of the packet.
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
 * @kv_pairs: Pointer to the array of <key, value> pairs to be appended.
 *            Property ids (keys) are to be passed in packed form
 *            (Packed with dsize and version info)
 * @num_props: Number of properties provided to append to the packet
 *                  payload
 * @append_size: Total payload size to be appended (total size of all
 *               keys and values in bytes)
 * Return Value: 0 upon success, Negative value if failure.
 */
int hfi_append_packet_with_kv_pairs(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 cmd, enum hfi_packet_payload_type payload_type, u32 kv_pairs_offset,
	struct hfi_kv_info *kv_pairs, u32 num_props, u32 append_size);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // __H_HFI_PACKER_H__
