// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */


#include "hfi_unpacker.h"

#define HFI_UNPACKER "unpacker"

static int hfi_sanitize_cmd_buff(struct hfi_header *hdr)
{
	struct hfi_packet *packet;
	u8 *pkt;
	u32 packet_size, total_pkt_size = 0;
	u32 hdr_size = 0;

	hdr_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (hdr_size < sizeof(struct hfi_header) + sizeof(struct hfi_packet)) {
		hfi_pk_printf(HFI_UNPACKER,
			"%s: invalid header size %d\n", __func__, hdr_size);
		return -HFI_ERROR;
	}

	pkt = (u8 *)((u8 *)hdr + sizeof(struct hfi_header));
	total_pkt_size = sizeof(struct hfi_header);

	/* validate all packet sizes*/
	for (int i = 0; i < hdr->num_packets; i++) {
		// check if hdr size is long enough for this packet header.
		if (hdr_size < (total_pkt_size + sizeof(struct hfi_packet))) {
			hfi_pk_printf(HFI_UNPACKER,
				"%s: invalid hdr size %d, req at least: %lu for pkt %u\n",
				__func__, hdr_size, total_pkt_size + sizeof(struct hfi_packet), i);
			return -HFI_ERROR;
		}
		packet = (struct hfi_packet *)pkt;
		packet_size = GET_PACKET_SIZE(packet->payload_info);
		if (packet_size < sizeof(struct hfi_packet)) {
			hfi_pk_printf(HFI_UNPACKER,
				"%s: invalid packet size %d for pkt id: %u\n",
				__func__, packet_size, packet->packet_id);
			return -HFI_ERROR;
		}
		total_pkt_size += packet_size;
		pkt += packet_size;
	}

	/* validate total size of all packets */
	if (hdr_size != total_pkt_size) {
		hfi_pk_printf(HFI_UNPACKER,
			"%s: mismatch in pkt header size %u and calculated pkt size: %u\n",
			__func__, hdr_size, total_pkt_size);
		return -HFI_ERROR;
	}

	return 0;
}

int hfi_unpacker_get_header_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info)
{
	struct hfi_header *hdr;
	u32 hdr_size = 0;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer) {
		hfi_pk_printf(HFI_UNPACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	hdr_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (hdr_size < sizeof(struct hfi_header)) {
		hfi_pk_printf(HFI_UNPACKER, "%s: invalid header size %u size\n",
			__func__, hdr_size);
		return -HFI_ERROR;
	}

	header_info->num_packets = hdr->num_packets;
	header_info->cmd_buff_type = GET_HEADER_CMD_BUFF_TYPE(hdr->cmd_buff_info);
	header_info->object_id = hdr->object_id;
	header_info->header_id = hdr->header_id;

	return 0;
}

int hfi_unpacker_get_packet_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 packet_num, struct hfi_packet_info *packet_info)
{
	struct hfi_header *hdr;
	struct hfi_packet *curr_pkt_hdr, *next_pkt_hdr;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !packet_info) {
		hfi_pk_printf(HFI_UNPACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	// sanitize full cmd buffer only once (during extracting 1st packet info)
	if (packet_num == 1) {
		if (hfi_sanitize_cmd_buff(hdr)) {
			hfi_pk_printf(HFI_UNPACKER, "%s: packet sanity failed\n", __func__);
			return -HFI_ERROR;
		}
	}

	// check if requested packet exists
	if (packet_num > hdr->num_packets) {
		hfi_pk_printf(HFI_UNPACKER,
			"%s: packet number %u does not exist\n", __func__, packet_num);
		return -HFI_ERROR;
	}

	// traverse to requested packet
	curr_pkt_hdr = (struct hfi_packet *)((u8 *)hdr + sizeof(struct hfi_header));
	if (packet_num > 1) {
		for (int i = 0; i < packet_num - 1 ; i++) {
			next_pkt_hdr = (struct hfi_packet *)
				((u8 *)curr_pkt_hdr +
					GET_PACKET_SIZE(curr_pkt_hdr->payload_info));
			curr_pkt_hdr = next_pkt_hdr;
		}
	}

	packet_info->cmd = curr_pkt_hdr->cmd;
	packet_info->flags = curr_pkt_hdr->flags;
	packet_info->id = curr_pkt_hdr->id;
	packet_info->packet_id = curr_pkt_hdr->packet_id;
	packet_info->payload_type =
		GET_PACKET_PAYLOAD_TYPE(curr_pkt_hdr->payload_info);
	packet_info->payload_size =
		GET_PACKET_SIZE(curr_pkt_hdr->payload_info) - sizeof(struct hfi_packet);
	if (packet_info->payload_size) {
		packet_info->payload_ptr =
			(void *)((u8 *)curr_pkt_hdr + sizeof(struct hfi_packet));
	}

	return 0;
}

int hfi_unpacker_get_kv_pairs_count(
	struct hfi_cmd_buff_hdl *cmd_buf_hdl, struct hfi_packet_info *packet_info,
	u32 kv_payload_offset, u32 *num_props)
{
	struct hfi_header *hdr;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer ||
		!packet_info || !packet_info->payload_ptr || !num_props) {
		hfi_pk_printf(HFI_UNPACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	*num_props = *((u32 *)((u8 *)packet_info->payload_ptr + kv_payload_offset));

	return 0;
}

int hfi_unpacker_get_kv_pairs(
	struct hfi_cmd_buff_hdl *cmd_buf_hdl, struct hfi_packet_info *packet_info,
	u32 kv_payload_offset, u32 num_props, struct hfi_kv_info *kv_pairs)
{
	u32 *kv_pairs_counter_idx, *kv_pairs_key;
	u32 value_size;

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer ||
		!packet_info || !packet_info->payload_ptr) {
		hfi_pk_printf(HFI_UNPACKER, "%s: invalid params\n", __func__);
		return -HFI_ERROR;
	}

	kv_pairs_counter_idx = (u32 *)((u8 *)packet_info->payload_ptr + kv_payload_offset);
	if (*kv_pairs_counter_idx != num_props) {
		hfi_pk_printf(HFI_UNPACKER,
			"%s: invalid num of properties requested, have: %u, requested: %u\n",
			__func__, *kv_pairs_counter_idx, num_props);
		return -HFI_ERROR;
	}
	kv_pairs_key = kv_pairs_counter_idx + 1;
	for (int i = 0; i < num_props; i++) {
		kv_pairs[i].key = *kv_pairs_key;
		kv_pairs[i].value_ptr = (void *)(kv_pairs_key + 1);
		value_size = (*kv_pairs_key & 0xFF000000) >> 24;
		kv_pairs_key = kv_pairs_key + 1 + value_size;
	}

	return 0;
}
