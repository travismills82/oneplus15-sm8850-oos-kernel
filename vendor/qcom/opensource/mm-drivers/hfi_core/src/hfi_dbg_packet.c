// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#if IS_ENABLED(CONFIG_DEBUG_FS)

#include <linux/string.h>
#include <linux/ktime.h>
#include "hfi_core_debug.h"
#include "hfi_dbg_packet.h"

static int hfi_validate_cmd_buff_size(struct hfi_header *hdr,
	u32 allocated_size, u32 bytes_to_add)
{
	u32 bytes_used;

	/*
	 * check if alloacted command buffer size is atleast
	 * long enough for hfi_header
	 */
	if (allocated_size < sizeof(struct hfi_header)) {
		HFI_CORE_ERR(
			"command buff size %u is not atleast header size\n",
			allocated_size);
		return -HFI_ERROR;
	}

	// check if allocated size has enough space for incoming bytes
	bytes_used = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (allocated_size < bytes_used + bytes_to_add) {
		HFI_CORE_ERR(
			"invalid cmd buff size, bytes allocated %u required: %u\n",
			allocated_size, bytes_used + bytes_to_add);
		return -HFI_ERROR;
	}

	return 0;
}

int hfi_create_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info)
{
	ktime_t t;
	u32 device_id = 0;

	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !header_info) {
		HFI_CORE_ERR("invalid params\n");
		return -HFI_ERROR;
	}

	struct hfi_header *hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;

	if (cmd_buf_hdl->size < sizeof(struct hfi_header)) {
		HFI_CORE_ERR("invalid cmd buf size: %u\n", cmd_buf_hdl->size);
		return -HFI_ERROR;
	}

	memset(hdr, 0, sizeof(struct hfi_header));
	hdr->cmd_buff_info = ((header_info->cmd_buff_type) <<
		HFI_HEADER_CMD_BUFF_TYPE_START_BIT) |
		sizeof(struct hfi_header);
	hdr->device_id = device_id;
	hdr->object_id = header_info->object_id;
	t = ktime_get();
	hdr->timestamp_hi = 0;
	hdr->timestamp_lo = (t & 0xFFFFFFFF);
	hdr->header_id = header_info->header_id;
	hdr->num_packets = 0;

	HFI_CORE_DBG_H("-\n");
	return 0;
}

int hfi_create_full_packet(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info)
{
	struct hfi_header *hdr;
	struct hfi_packet *pkt_hdr;
	int ret = 0;
	u32 pkt_size;

	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !packet_info) {
		HFI_CORE_ERR("invalid params\n");
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	pkt_size = sizeof(struct hfi_packet) + packet_info->payload_size;
	ret = hfi_validate_cmd_buff_size(hdr, cmd_buf_hdl->size, pkt_size);
	if (ret)
		return ret;

	pkt_hdr = (struct hfi_packet *)
		((u8 *)hdr + GET_HEADER_SIZE(hdr->cmd_buff_info));
	memset(pkt_hdr, 0, sizeof(struct hfi_packet));
	pkt_hdr->payload_info = ((packet_info->payload_type) <<
		HFI_PACKET_PAYLOAD_TYPE_START_BIT) | pkt_size;
	pkt_hdr->cmd = packet_info->cmd;
	pkt_hdr->flags = packet_info->flags;
	pkt_hdr->id = packet_info->id;
	pkt_hdr->packet_id = packet_info->packet_id;
	if (packet_info->payload_size) {
		if (!packet_info->payload_ptr) {
			HFI_CORE_ERR(
				"payload ptr is null but payload size: %u\n",
				packet_info->payload_size);
			return -HFI_ERROR;
		}
		memcpy((u8 *)pkt_hdr + sizeof(struct hfi_packet),
			packet_info->payload_ptr, packet_info->payload_size);
	}

	// update header
	hdr->cmd_buff_info += pkt_size;
	hdr->num_packets++;

	HFI_CORE_DBG_H("-\n");
	return 0;
}

static int hfi_sanitize_cmd_buff(struct hfi_header *hdr)
{
	struct hfi_packet *packet;
	u8 *pkt;
	u32 packet_size, total_pkt_size = 0;
	u32 hdr_size = 0;

	HFI_CORE_DBG_H("+\n");

	hdr_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (hdr_size < sizeof(struct hfi_header) + sizeof(struct hfi_packet)) {
		HFI_CORE_ERR("invalid header size %d\n", hdr_size);
		return -HFI_ERROR;
	}

	pkt = (u8 *)((u8 *)hdr + sizeof(struct hfi_header));
	total_pkt_size = sizeof(struct hfi_header);

	/* validate all packet sizes*/
	for (int i = 0; i < hdr->num_packets; i++) {
		/* check if hdr size is long enough for this
		 * packet header.
		 */
		if (hdr_size < (total_pkt_size + sizeof(struct hfi_packet))) {
			HFI_CORE_ERR(
				"invalid hdr size %d, req atleast: %lu for pkt %d\n",
				hdr_size,
				total_pkt_size + sizeof(struct hfi_packet), i);
			return -HFI_ERROR;
		}
		packet = (struct hfi_packet *)pkt;
		packet_size = GET_PACKET_SIZE(packet->payload_info);
		if (packet_size < sizeof(struct hfi_packet)) {
			HFI_CORE_ERR(
				"invalid packet size %d for pkt id: %u\n",
				packet_size, packet->packet_id);
			return -HFI_ERROR;
		}
		total_pkt_size += packet_size;
		pkt += packet_size;
	}

	/* validate total size of all packets */
	if (hdr_size != total_pkt_size) {
		HFI_CORE_ERR(
			"mismatch in pkt header size %u and calculated pkt size: %u\n",
			hdr_size, total_pkt_size);
		return -HFI_ERROR;
	}

	HFI_CORE_DBG_H("-\n");
	return 0;
}

int hfi_unpacker_get_header_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info)
{
	struct hfi_header *hdr;
	u32 hdr_size = 0;

	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer) {
		HFI_CORE_ERR("invalid params\n");
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	hdr_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (hdr_size < sizeof(struct hfi_header)) {
		HFI_CORE_ERR("invalid header size %u size\n", hdr_size);
		return -HFI_ERROR;
	}

	header_info->num_packets = hdr->num_packets;
	header_info->cmd_buff_type =
		GET_HEADER_CMD_BUFF_TYPE(hdr->cmd_buff_info);
	header_info->object_id = hdr->object_id;
	header_info->header_id = hdr->header_id;

	HFI_CORE_DBG_H("-\n");
	return 0;
}

int hfi_unpacker_get_packet_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 packet_num, struct hfi_packet_info *packet_info)
{
	struct hfi_header *hdr;
	struct hfi_packet *curr_pkt_hdr, *next_pkt_hdr;

	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !packet_info) {
		HFI_CORE_ERR("invalid params\n");
		return -HFI_ERROR;
	}

	hdr = (struct hfi_header *)cmd_buf_hdl->cmd_buffer;
	// sanitize full cmd buffer only once (during extracting 1st packet info)
	if (packet_num == 1) {
		if (hfi_sanitize_cmd_buff(hdr)) {
			HFI_CORE_ERR("packet sanity failed\n");
			return -HFI_ERROR;
		}
	}

	// check if requested packet exists
	if (packet_num > hdr->num_packets) {
		HFI_CORE_ERR("packet number %u does not exist\n", packet_num);
		return -HFI_ERROR;
	}

	// traverse to requested packet
	curr_pkt_hdr = (struct hfi_packet *)
		((u8 *)hdr + sizeof(struct hfi_header));
	if (packet_num > 1) {
		for (int i = 0; i < packet_num - 1 ; i++) {
			next_pkt_hdr = (struct hfi_packet *)
				((u8 *)curr_pkt_hdr +
					GET_PACKET_SIZE(
					curr_pkt_hdr->payload_info));
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
		GET_PACKET_SIZE(curr_pkt_hdr->payload_info) -
		sizeof(struct hfi_packet);
	if (packet_info->payload_size) {
		packet_info->payload_ptr = (void *)
			((u8 *)curr_pkt_hdr + sizeof(struct hfi_packet));
	}

	HFI_CORE_DBG_H("-\n");
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
		HFI_CORE_ERR(
			"invalid payload size:%u or no packet to append: %u\n",
			payload_size, hdr->num_packets);
		return NULL;
	}

	// traverse to last packet
	curr_pkt_hdr = (struct hfi_packet *)
		((u8 *)hdr + sizeof(struct hfi_header));
	packet_current_size = GET_PACKET_SIZE(curr_pkt_hdr->payload_info);
	if (num_packets > 1) {
		for (int i = 0; i < num_packets - 1 ; i++) {
			next_pkt_hdr = (struct hfi_packet *)
				((u8 *)curr_pkt_hdr + packet_current_size);
			curr_pkt_hdr = next_pkt_hdr;
			packet_current_size =
				GET_PACKET_SIZE(curr_pkt_hdr->payload_info);
		}
	}

	/*
	 * check if packet payload type and cmd matches given payload type
	 * and cmd
	 */
	if (GET_PACKET_PAYLOAD_TYPE(curr_pkt_hdr->payload_info) !=
		payload_type || curr_pkt_hdr->cmd != cmd) {
		HFI_CORE_ERR(
			"mismatch: pkt payload type[%u] req[%u] or pkt hdr cmd[%u] req cmd[%u]\n",
			GET_PACKET_PAYLOAD_TYPE(curr_pkt_hdr->payload_info),
			payload_type, curr_pkt_hdr->cmd, cmd);
		return NULL;
	}

	// check if packet or header size has reached its limit
	header_current_size = GET_HEADER_SIZE(hdr->cmd_buff_info);
	if (packet_current_size + payload_size > HFI_PACKET_SIZE_MAX ||
		header_current_size + payload_size > HFI_HEADER_SIZE_MAX) {
		HFI_CORE_ERR(
			"packet %u or header %u is full, cannot append payload\n",
			curr_pkt_hdr->packet_id, hdr->header_id);
			return NULL;
	}

	return curr_pkt_hdr;
}

int hfi_append_packet_with_kv_pairs(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 cmd, enum hfi_packet_payload_type payload_type,
	u32 kv_pairs_offset, struct hfi_kv_info *kv_pairs, u32 num_props,
	u32 append_size)
{
	struct hfi_header *hdr;
	struct hfi_packet *packet_to_append_hdr;
	u32 packet_current_size, value_size;
	u32 *pkt_kv_pairs_counter_idx;
	u32 *key;
	void *value_ptr;

	HFI_CORE_DBG_H("+\n");
	if (!cmd_buf_hdl || !cmd_buf_hdl->cmd_buffer || !kv_pairs) {
		HFI_CORE_ERR("invalid params\n");
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
	packet_current_size =
		GET_PACKET_SIZE(packet_to_append_hdr->payload_info);

	for (int i = 0; i < num_props; i++) {
		key = (u32 *)
			((u8 *)packet_to_append_hdr + packet_current_size);
		*key = kv_pairs[i].key;
		value_ptr = (void *)(key + 1);
		value_size = ((*key & 0xFF000000) >> 24) * 4; // convert dwords to bytes
		if (!kv_pairs[i].value_ptr) {
			HFI_CORE_ERR("value ptr is null for key: %u\n",
				kv_pairs->key);
			return -HFI_ERROR;
		}
		memcpy(value_ptr, kv_pairs[i].value_ptr, value_size);
		packet_current_size += (4 + value_size); // key size + value size
	}
	*pkt_kv_pairs_counter_idx += num_props;

	// update packet header size and header size in bytes
	packet_to_append_hdr->payload_info += append_size;
	hdr->cmd_buff_info += append_size;

	HFI_CORE_DBG_H("-\n");
	return 0;
}

#endif // CONFIG_DEBUG_FS
