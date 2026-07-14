/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_COMMANDS_DEBUG_H__
#define __H_HFI_COMMANDS_DEBUG_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All Debug level commands begin here.
 * "1st MSB byte = 0xFF"
 */
#define HFI_COMMAND_DEBUG_BEGIN                                      0xFF000000

/*
 * hfi_header sample for any debug command:
 * hfi_header.cmd_buff_info.size          : (sizeof(hfi_header) +
 *                                            sizeof(hfi_packet)
 *           .cmd_buff_info.type          : HFI_DEBUG
 *           .device_id                   : 0
 *           .object_id                   : n/a
 *           .timestamp_hi                : ts_hi
 *           .timestamp_lo                : ts_lo
 *           .header_id                   : unique id
 *           .num_packets                 : 'n' packets depending upon the
 *                                          packet cmd
 *
 * hfi_packet sample for any debug packet:
 * hfi_packet.payload_info.size           : sizeof(hfi_packet)
 *                                          (including payload size)
 *           .payload_info.type           : one of enum
 *                                          'hfi_packet_payload_type'
 *           .cmd                         : HFI_COMMAND_DEBUG_XXX
 *           .flags (Host to DCP)         : HFI_TX_FLAGS_XXX
 *                                          (enum hfi_packet_host_flags)
 *                  (DCP to Host)         : HFI_RX_FLAGS_XXX
 *                                          (enum hfi_fw_host_flags)
 *           .id                          : 0
 *           .packet_id                   : unique id
 */

/*
 * HFI_COMMAND_DEBUG_LOOPBACK_NONE - This is the LOOPBACK command
 * This command does not hold any payload.
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_NONE
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_NONE
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_NONE                              0xFF000001
/*
 * HFI_COMMAND_DEBUG_LOOPBACK_U32 is the U32 LOOPBACK command,
 * DCP Receives LOOPBACK command with the u32 test pattern and
 * upon receiving this command, the same data is returned
 * to HLOS.
 *
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_U32
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : u32 test pattern
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_U32                               0xFF000002
/*
 * HFI_COMMAND_DEBUG_LOOPBACK_U64 is the U64 LOOPBACK command,
 * DCP Receives LOOPBACK command with the u64 test pattern and
 * upon receiving this command, the same data is returned
 * to HLOS.
 *
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U64
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_U64
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : u64 test pattern
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_U64                               0xFF000003
/*
 * HFI_COMMAND_DEBUG_LOOPBACK_BLOB is the BLOB LOOPBACK command,
 * DCP Receives LOOPBACK command with the blob of data and
 * upon receiving this command, the same data is returned
 * to HLOS.
 *
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_BLOB
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_BLOB
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : blob of data
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_BLOB                              0xFF000004
/*
 * HFI_COMMAND_DEBUG_LOOPBACK_U32_ARRAY is the U32_ARRAY LOOPBACK command,
 * DCP Receives LOOPBACK command with the u32 array test pattern and
 * upon receiving this command, the same data is returned
 * to HLOS.
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_U32_ARRAY
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : u32 test_value[32]
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_U32_ARRAY                         0xFF000005
/*
 * HFI_COMMAND_DEBUG_LOOPBACK_U64_ARRAY is the U64_ARRAY LOOPBACK command,
 * DCP Receives LOOPBACK command with the u64 array test pattern and
 * upon receiving this command, the same data is returned
 * to HLOS.
 *
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U64_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_LOOPBACK_U64_ARRAY
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : u64 test_value[64]
 */
#define HFI_COMMAND_DEBUG_LOOPBACK_U64_ARRAY                         0xFF000006

/*
 * HFI_COMMAND_DEBUG_MISR_SETUP is the MISR SETUP command,
 * DCP Receives MISR command with setup instructions. Respond with
 * ack for success.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_MISR_SETUP
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : struct hfi_debug_misr_setup_data
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_NONE
 *           .cmd                      : HFI_COMMAND_DEBUG_MISR_SETUP
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 */
#define HFI_COMMAND_DEBUG_MISR_SETUP                                 0xFF000007
/*
 * HFI_COMMAND_DEBUG_MISR_READ is the MISR READ command,
 * DCP Receives MISR command with read instructions. Respond with
 * MISR value for modules in the specified layer.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_MISR_READ
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload                  : struct hfi_debug_misr_read_data
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_MISR_READ
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct misr_read_data_ret
 *
 * struct misr_read_data_ret - MISR read return data
 * @module_type     : module type
 * @num_misr        : number of modules
 * @misr_value      : misr values obtained
 *
 * struct misr_read_data_ret {
 *   enum hfi_debug_misr_module_type module_type;
 *   u32 num_misr;
 *   u32 misr_value[num_modules];
 * };
 */
#define HFI_COMMAND_DEBUG_MISR_READ                                  0xFF000008
/*
 * HFI_COMMAND_DEBUG_INIT is sent from host to DCP to obtain buffer allocation
 * size for panic events.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_INIT
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload (u32 values)     : u32 device_id
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 1
 *
 * Data Contents:
 *  DCP replies back with <key,value> pairs indicating the sizes required to dump panic logs.
 *  The below valid 'keys' can be passed as part of the struct property_data.
 *  - MDSS registers:   key = HFI_PROPERTY_DEBUG_REG_ALLOC
 *  - Event Log:        key = HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC
 *  - Debug Bus:        key = HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC
 *  - State variable:   key = HFI_PROPERTY_DEBUG_STATE_ALLOC
 *
 * Data Layout:
 *  struct debug_device_alloc - panic log dump sizes required for allocation
 *  @buffer_alloc_info: Array of <key,value> pairs indicating the hfi debug alloc property and its
 *                      size.
 *  struct debug_device_alloc {
 *      struct property_data buffer_alloc_info;
 *  }
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_INIT
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload (u32 values)     : struct debug_device_alloc
 */
#define HFI_COMMAND_DEBUG_INIT                                       0xFF000009

/*
 * HFI_COMMAND_DEBUG_SETUP is sent from host to DCP to indicate buffer addresses allocated
 * to the device for dumping panic logs.
 *
 * Data Contents:
 *  Host can use the below valid 'keys' as part of struct property_data to indicate
 *  the allocated address for a given item:
 *  - MDSS registers:   key = HFI_PROPERTY_DEBUG_REG_ADDR
 *  - Event Log:        key = HFI_PROPERTY_DEBUG_EVT_LOG_ADDR
 *  - Debug Bus:        key = HFI_PROPERTY_DEBUG_DBG_BUS_ADDR
 *  - State variable:   key = HFI_PROPERTY_DEBUG_STATE_ADDR
 *
 * Data Layout:
 *  struct debug_device_setup - debug setup info for device_id
 *  @buffer_addr_info: Array of <key,value> pairs indicating the hfi debug alloc property and its
 *                     designated address.
 *  struct debug_device_setup {
 *      struct property_data buffer_addr_info;
 *  }
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_SETUP
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload (u32 values)     : struct debug_device_setup
 */
#define HFI_COMMAND_DEBUG_SETUP                                       0xFF00000A

/*
 * HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE is sent from Host to DCP to subscribe/unsubscribe to
 * display-fatal failure events. Events are set on a per display basis.
 *
 * Data Contents:
 *  The below valid bitmasks can be set as part of the events_mask indicating the panic event.
 *  - HFI_DEBUG_EVENT_UNDERRUN
 *  - HFI_DEBUG_EVENT_HW_RESET
 *  - HFI_DEBUG_EVENT_PP_TIMEOUT
 *
 * Data Layout:
 *  struct debug_display_evt_info - display panic event info
 *  @display_id: Display ID
 *  @events_mask: Bitwise OR of HFI_DEBUG_EVENT_XX
 *  @enable: HFI_TRUE / HFI_FALSE
 *  struct debug_display_evt_info {
 *      u32 display_id
 *      u32 events_mask;
 *      u32 enable;
 *  }
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload (u32 values)     : struct debug_display_evt_info
 *
 */
#define HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE                            0xFF00000B

/*
 * HFI_COMMAND_DEBUG_PANIC_EVENT is sent from DCP to host to indicate a panic event for a
 * given display. The payload contains the display id, device id and bitwise or of events set.
 *
 * Data Contents:
 *  The below valid bitmasks can be set as part of the events_mask indicating the panic event.
 *  - HFI_DEBUG_EVENT_UNDERRUN
 *  - HFI_DEBUG_EVENT_HW_RESET
 *  - HFI_DEBUG_EVENT_PP_TIMEOUT
 *
 * Data Layout:
 *  struct panic_info - panic information
 *  @display_id: Display ID
 *  @events_mask: Bitwise OR of HFI_DEBUG_EVENT_XX
 *  struct panic_info {
 *      u32 display_id;
 *      u32 events_mask;
 *  }
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_PANIC_EVENT
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload (u32 values)     : struct panic_info
 */
#define HFI_COMMAND_DEBUG_PANIC_EVENT                                0xFF00000C
/*
 * HFI_COMMAND_DEBUG_DUMP_REGS is sent from DCP to read MDSS registers at a given offset
 * for a given length.
 *
 * Data Layout:
 *  struct regdump_info - register dump information at a requested offset & length
 *  @device_id: Device ID
 *  @reg_offset: Offset from MDSS base
 *  @length: Required length to be dumped
 *  @buffer_info: Allocated memory for dumping
 *  struct regdump_info {
 *      u32 device_id;
 *      u32 reg_offset;
 *      u32 length;
 *      struct hfi_buff buffer_info;
 *  }
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEBUG_DUMP_REGS
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED
 *           .payload (u32 values)     : struct regdump_info
 */
#define HFI_COMMAND_DEBUG_DUMP_REGS                                  0xFF00000D

#define HFI_COMMAND_DEBUG_END                                        0xFFFFFFFF

#endif // __H_HFI_COMMANDS_DEBUG_H__
