/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_CORE_DEBUG_PACKET_H_
#define _HFI_CORE_DEBUG_PACKET_H_

#if IS_ENABLED(CONFIG_DEBUG_FS)

#define HFI_ERROR                                                      1
#define HFI_HEADER_CMD_BUFF_TYPE_START_BIT                            24
#define HFI_PACKET_PAYLOAD_TYPE_START_BIT                             21
#define HFI_COMMAND_DEBUG_LOOPBACK_U32                        0xFF000002
#define HFI_COMMAND_DEVICE_INIT                               0x01000001
#define HFI_COMMAND_PANEL_INIT_PANEL_CAPS                     0x03000001
#define HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS               0x03000002
#define HFI_COMMAND_PANEL_INIT_GENERIC_CAPS                   0x03000003
#define HFI_COMMAND_DISPLAY_SET_MODE                          0x0200000A
#define HFI_COMMAND_DISPLAY_SET_PROPERTY		      0x02000001
#define HFI_COMMAND_DISPLAY_FRAME_TRIGGER		      0x02000003

// panel properties
#define HFI_PROPERTY_PANEL_TIMING_MODE_COUNT                         0x00040001
#define HFI_PROPERTY_PANEL_INDEX                                     0x00040002
#define HFI_PROPERTY_PANEL_CLOCKRATE                                 0x00040003
#define HFI_PROPERTY_PANEL_FRAMERATE                                 0x00040004
#define HFI_PROPERTY_PANEL_RESOLUTION_DATA                           0x00040005
#define HFI_PROPERTY_PANEL_JITTER                                    0x00040006
#define HFI_PROPERTY_PANEL_COMPRESSION_DATA                          0x0004007
#define HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY                          0x00040008
#define HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX                    0x00040009
#define HFI_PROPERTY_PANEL_NAME                                      0x0004000A
#define HFI_PROPERTY_PANEL_PHYSICAL_TYPE                             0x0004000B
#define HFI_PROPERTY_PANEL_BPP                                       0x0004000C
#define HFI_PROPERTY_PANEL_LANES_STATE                               0x0004000D
#define HFI_PROPERTY_PANEL_LANE_MAP                                  0x0004000E
#define HFI_PROPERTY_PANEL_COLOR_ORDER                               0x0004000F
#define HFI_PROPERTY_PANEL_DMA_TRIGGER                               0x00040010
#define HFI_PROPERTY_PANEL_TX_EOT_APPEND                             0x00040011
#define HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE                       0x00040012
#define HFI_PROPERTY_PANEL_BLLP_POWER_MODE                           0x00040013
#define HFI_PROPERTY_PANEL_TRAFFIC_MODE                              0x00040014
#define HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID                        0x00040015
#define HFI_PROPERTY_PANEL_WR_MEM_START                              0x00040016
#define HFI_PROPERTY_PANEL_WR_MEM_CONTINUE                           0x00040017
#define HFI_PROPERTY_PANEL_TE_DCS_COMMAND                            0x00040018
#define HFI_PROPERTY_PANEL_OPERATING_MODE                            0x00040019
#define HFI_PROPERTY_PANEL_BL_MIN_LEVEL                              0x0004001A
#define HFI_PROPERTY_PANEL_BL_MAX_LEVEL                              0x0004001B
#define HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL                      0x0004001C
#define HFI_PROPERTY_PANEL_CTRL_NUM                                  0x0004001D
#define HFI_PROPERTY_PANEL_PHY_NUM                                   0x0004001E
#define HFI_PROPERTY_PANEL_RESET_SEQUENCE                            0x0004001F
#define HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE                      0x00040020
#define HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE                  0x00040021

// display properties
#define HFI_PROPERTY_DISPLAY_ATTACH_LAYER	0x00020018
#define HFI_PROPERTY_LAYER_BLEND_TYPE		0x00030002
#define HFI_PROPERTY_LAYER_ALPHA		0x00030003
#define HFI_PROPERTY_LAYER_ZPOS			0x00030004
#define HFI_PROPERTY_LAYER_SRC_ROI		0x00030005
#define HFI_PROPERTY_LAYER_DEST_ROI		0x00030006
#define HFI_PROPERTY_LAYER_SRC_IMG_SIZE_W	0x00030009
#define HFI_PROPERTY_LAYER_SRC_IMG_SIZE_H	0x00030008
#define HFI_PROPERTY_LAYER_SRC_ADDR		0x0003000A
#define HFI_PROPERTY_LAYER_SRC_FORMAT		0x0003000B

/* 24 bits long */
#define HFI_HEADER_SIZE_MAX     \
	((1 << HFI_HEADER_CMD_BUFF_TYPE_START_BIT) - 1)
/* 20 bits long */
#define HFI_PACKET_SIZE_MAX     \
	((1 << HFI_PACKET_PAYLOAD_TYPE_START_BIT) - 1)

#define GET_HEADER_SIZE(data)           \
	(data & ((1 << HFI_HEADER_CMD_BUFF_TYPE_START_BIT) - 1))
#define GET_HEADER_CMD_BUFF_TYPE(data)         \
	(data >> HFI_HEADER_CMD_BUFF_TYPE_START_BIT)
#define GET_PACKET_SIZE(data)           \
	(data & ((1 << HFI_PACKET_PAYLOAD_TYPE_START_BIT) - 1))
#define GET_PACKET_PAYLOAD_TYPE(data)   \
	(data >> HFI_PACKET_PAYLOAD_TYPE_START_BIT)

#define HFI_PACK_KEY(property_id, version, dsize)               \
	(property_id | (version << 20) | (dsize << 24))

#define HFI_UNPACK_KEY(key, property_id, version, size)         \
	do {                                                    \
		property_id = (key & 0x000FFFFF);               \
		version     = ((key & 0x00F00000) >> 20);       \
		size        = ((key & 0xFF000000) >> 24);       \
	} while (0)

enum hfi_packet_payload_type {
	HFI_PAYLOAD_NONE                = 0x0,
	HFI_PAYLOAD_U32                 = 0x1,
	HFI_PAYLOAD_U32_ARRAY           = 0x2,
	HFI_PAYLOAD_U64                 = 0x3,
	HFI_PAYLOAD_U64_ARRAY           = 0x4,
	HFI_PAYLOAD_BLOB                = 0x5,
};

enum hfi_cmd_buff_type {
	HFI_CMD_BUFF_SYSTEM          = 0x1,
	HFI_CMD_BUFF_DEVICE          = 0x2,
	HFI_CMD_BUFF_DISPLAY         = 0x3,
	HFI_CMD_BUFF_DEBUG           = 0x4,
	HFI_CMD_BUFF_VIRTUALIZATION  = 0x5,
};

enum hfi_packet_tx_flags {
	HFI_TX_FLAGS_NONE              = 0x0,
	HFI_TX_FLAGS_INTR_REQUIRED     = 0x1,
	HFI_TX_FLAGS_RESPONSE_REQUIRED = 0x2,
	HFI_TX_FLAGS_NON_DISCARDABLE   = 0x4,
};

enum hfi_packet_rx_flags {
	HFI_RX_FLAGS_NONE               = 0x0,
	HFI_RX_FLAGS_SUCCESS            = 0x1,
	HFI_RX_FLAGS_INFORMATION        = 0x2,
	HFI_RX_FLAGS_DEVICE_ERROR       = 0x4,
	HFI_RX_FLAGS_SYSTEM_ERROR       = 0x8,
};

struct hfi_cmd_buff_hdl {
	void *cmd_buffer;
	u32 size;
};

struct hfi_header_info {
	u32 num_packets;
	enum hfi_cmd_buff_type cmd_buff_type;
	u32 object_id;
	u32 header_id;
};

struct hfi_packet_info {
	u32 cmd;
	u32 id;
	u32 flags;
	u32 packet_id;
	enum hfi_packet_payload_type payload_type;
	u32 payload_size;
	void *payload_ptr;
};

struct hfi_header {
	u32 cmd_buff_info;
	u16 device_id;
	u16 object_id;
	u32 timestamp_hi;
	u32 timestamp_lo;
	u32 header_id;
	u32 reserved[2];
	u32 num_packets;
};

struct hfi_packet {
	u32 payload_info;
	u32 cmd;
	u32 flags;
	u32 id;
	u32 packet_id;
	u32 reserved[3];
};

struct hfi_kv_info {
	u32 key;
	void *value_ptr;
};

int hfi_create_header(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info);

int hfi_create_full_packet(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info);

int hfi_append_packet_with_kv_pairs(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 cmd, enum hfi_packet_payload_type payload_type,
	u32 kv_pairs_offset, struct hfi_kv_info *kv_pairs,
	u32 num_props, u32 append_size);

int hfi_unpacker_get_header_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_header_info *header_info);

int hfi_unpacker_get_packet_info(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	u32 packet_num, struct hfi_packet_info *packet_info);

#endif // CONFIG_DEBUG_FS

#endif // _HFI_CORE_DEBUG_PACKET_H_
