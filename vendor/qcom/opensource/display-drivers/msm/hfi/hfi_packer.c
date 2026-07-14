// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "hfi_packer.h"

#define HFI_PACKER "packer"

static int hfi_validate_cmd_buff_size(struct hfi_header *hdr,
	u32 allocated_size, u32 bytes_to_add)
{
	u32 bytes_used;

	/*
	 * check if alloacted command buffer size is at least
	 * long enough for hfi_header
	 */
	if (allocated_size < sizeof(struct hfi_header)) {
		hfi_pk_printf(HFI_PACKER,
			"%s: command buff size %u is not at least header size\n", __func__,
			allocated_size);
		return -HFI_ERROR;
	}

	// check if allocated size has enough space for incoming bytes
	bytes_used = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (allocated_size < bytes_used + bytes_to_add) {
		hfi_pk_printf(HFI_PACKER,
			"%s: invalid cmd buff size, bytes allocated %u required: %u\n",
			__func__, allocated_size, bytes_used + bytes_to_add);
		return -HFI_ERROR;
	}

	return 0;
}

int hfi_create_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info)
{
	//time_t t;
	u32 device_id = 0;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !header_info) {
		hfi_pk_printf(HFI_PACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	struct hfi_header *hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;

	if (cmd_buf_hdl->size < sizeof(struct hfi_header)) {
		hfi_pk_printf(HFI_PACKER, "%s: invalid cmd buf size: %u\n",
			__func__, cmd_buf_hdl->size);
		return -HFI_ERROR;
	}

	memset(hdr, 0, sizeof(struct hfi_header));
	hdr->cmd_buff_info = ((header_info->cmd_buff_type) <<
		HFI_HEADER_CMD_BUFF_TYPE_START_BIT) | sizeof(struct hfi_header);
	hdr->device_id = device_id;
	hdr->object_id = header_info->object_id;
	hdr->timestamp_hi = 0;
	//hdr->timestamp_lo = (u32)(time(&t));
	hdr->header_id = header_info->header_id;
	hdr->num_packets = 0;
	return 0;
}

int hfi_create_full_packet(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info)
{
	struct hfi_header *hdr;
	struct hfi_packet *pkt_hdr;
	int rc = 0;
	u32 pkt_size;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !packet_info ||
		!packet_info->payload_ptr) {
		hfi_pk_printf(HFI_PACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	pkt_size = sizeof(struct hfi_packet) + packet_info->payload_size;
	rc = hfi_validate_cmd_buff_size(hdr, cmd_buf_hdl->size, pkt_size);
	if (rc)
		return rc;

	pkt_hdr = (struct hfi_packet *)
		((u8 *)hdr + GET_HEADER_SIZE(hdr->cmd_buff_info));
	memset(pkt_hdr, 0, sizeof(struct hfi_packet));
	pkt_hdr->payload_info = ((packet_info->payload_type) <<
		HFI_PACKET_PAYLOAD_TYPE_START_BIT) | pkt_size;
	pkt_hdr->cmd = packet_info->cmd;
	pkt_hdr->flags = packet_info->flags;
	pkt_hdr->id = packet_info->id;
	pkt_hdr->packet_id = packet_info->packet_id;
	if (packet_info->payload_size)
		memcpy((u8 *)pkt_hdr + sizeof(struct hfi_packet),
			packet_info->payload_ptr, packet_info->payload_size);

	// update header
	hdr->cmd_buff_info += pkt_size;
	hdr->num_packets++;

	return 0;
}

int hfi_create_packet_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info)
{
	struct hfi_header *hdr;
	struct hfi_packet *pkt_hdr;
	int rc = 0;
	u32 pkt_size;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !packet_info) {
		hfi_pk_printf(HFI_PACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	pkt_size = sizeof(struct hfi_packet);
	rc = hfi_validate_cmd_buff_size(hdr, cmd_buf_hdl->size, pkt_size);
	if (rc)
		return rc;

	pkt_hdr = (struct hfi_packet *)
		((u8 *)hdr + GET_HEADER_SIZE(hdr->cmd_buff_info));
	memset(pkt_hdr, 0, sizeof(struct hfi_packet));
	pkt_hdr->payload_info = ((packet_info->payload_type) <<
		HFI_PACKET_PAYLOAD_TYPE_START_BIT) | pkt_size;
	pkt_hdr->cmd = packet_info->cmd;
	pkt_hdr->flags = packet_info->flags;
	pkt_hdr->id = packet_info->id;
	pkt_hdr->packet_id = packet_info->packet_id;

	// update header
	hdr->cmd_buff_info += pkt_size;
	hdr->num_packets++;

	return 0;
}

static struct hfi_packet *hfi_get_sanitized_append_packet(
	struct hfi_cmd_buff_hdl *cmd_buf_hdl, u32 cmd,
	enum hfi_packet_payload_type payload_type, u32 payload_size)
{
	struct hfi_header *hdr;
	struct hfi_packet *next_pkt_hdr, *curr_pkt_hdr;
	u32 num_packets = 0, packet_current_size = 0, header_current_size = 0;
	int rc = 0;

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	rc = hfi_validate_cmd_buff_size(hdr, cmd_buf_hdl->size, payload_size);
	if (rc)
		return NULL;

	num_packets = hdr->num_packets;
	if (!payload_size || !num_packets) {
		hfi_pk_printf(HFI_PACKER,
			"%s: invalid payload size:%u or no packet to append: %u\n",
			__func__, payload_size, hdr->num_packets);
		return NULL;
	}

	// traverse to last packet
	curr_pkt_hdr = (struct hfi_packet *)((u8 *)hdr + sizeof(struct hfi_header));
	packet_current_size = GET_PACKET_SIZE(curr_pkt_hdr->payload_info);
	if (num_packets > 1) {
		for (int i = 0; i < num_packets - 1 ; i++) {
			next_pkt_hdr = (struct hfi_packet *)((u8 *)curr_pkt_hdr +
				packet_current_size);
			curr_pkt_hdr = next_pkt_hdr;
			packet_current_size = GET_PACKET_SIZE(curr_pkt_hdr->payload_info);
		}
	}

	// check if packet payload type and cmd matches given payload type and cmd
	if (GET_PACKET_PAYLOAD_TYPE(curr_pkt_hdr->payload_info) != payload_type ||
		curr_pkt_hdr->cmd != cmd) {
		hfi_pk_printf(HFI_PACKER,
			"%s: mismatch in pkt payload type %u and given payload: %u",
			__func__, GET_PACKET_PAYLOAD_TYPE(curr_pkt_hdr->payload_info),
			payload_type);
		hfi_pk_printf(HFI_PACKER,
			"%s: or mismatch in pkt hdr cmd: %u and given cmd: %u\n",
			__func__, curr_pkt_hdr->cmd, cmd);
		return NULL;
	}

	// check if packet or header size has reached its limit
	header_current_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (packet_current_size + payload_size > HFI_PACKET_SIZE_MAX ||
		header_current_size + payload_size > HFI_HEADER_SIZE_MAX) {
		hfi_pk_printf(HFI_PACKER,
			"%s: packet %u or header %u is full, cannot append payload\n",
			__func__, curr_pkt_hdr->packet_id, hdr->header_id);
		return NULL;
	}

	return curr_pkt_hdr;
}

int hfi_append_packet_with_kv_pairs(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 cmd, enum hfi_packet_payload_type payload_type, u32 kv_pairs_offset,
	struct hfi_kv_info *kv_pairs, u32 num_props, u32 append_size)
{
	struct hfi_header *hdr;
	struct hfi_packet *packet_to_append_hdr;
	u32 packet_current_size, value_size;
	u32 *pkt_kv_pairs_counter_idx;
	u32 *key;
	void *value_ptr;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !kv_pairs) {
		hfi_pk_printf(HFI_PACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;

	packet_to_append_hdr = hfi_get_sanitized_append_packet(
			cmd_buf_hdl, cmd, payload_type, append_size);
	if (!packet_to_append_hdr)
		return -HFI_ERROR;

	// apend kv pairs at the end of last packet
	pkt_kv_pairs_counter_idx = (u32 *)((u8 *)packet_to_append_hdr +
		(sizeof(struct hfi_packet) + kv_pairs_offset));
	if (*pkt_kv_pairs_counter_idx == 0) {
		/*
		 * packet size should be incremented by 1 dword to accommodate
		 * <key, value> pairs counter index.
		 */
		packet_to_append_hdr->payload_info += 4;
		hdr->cmd_buff_info += 4;
	}
	packet_current_size = GET_PACKET_SIZE(packet_to_append_hdr->payload_info);

	for (int i = 0; i < num_props; i++) {
		key = (u32 *)((u8 *)packet_to_append_hdr + packet_current_size);
		*key = kv_pairs[i].key;
		value_ptr = (void *)(key + 1);
		value_size = ((*key & 0xFF000000) >> 24) * 4; // convert dwords to bytes
		if (!kv_pairs[i].value_ptr) {
			hfi_pk_printf(HFI_PACKER,
				"%s: value ptr is null for key: %u\n", __func__, kv_pairs->key);
			return -HFI_ERROR;
		}
		memcpy(value_ptr, kv_pairs[i].value_ptr, value_size);
		packet_current_size += (4 + value_size); // key size + value size
	}
	*pkt_kv_pairs_counter_idx += num_props;

	// update packet header size and header size in bytes
	packet_to_append_hdr->payload_info += append_size;
	hdr->cmd_buff_info += append_size;

	return 0;
}
