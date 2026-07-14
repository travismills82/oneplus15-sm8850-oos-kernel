/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_COMMANDS_PANEL_H__
#define __H_HFI_COMMANDS_PANEL_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All Panel level commands begin here.
 * "1st MSB byte = 0x03"
 */
#define HFI_COMMAND_PANEL_BEGIN                                         0x03000000

/*
 * hfi_header sample for any panel init commands:
 * hfi_header.cmd_buff_info.size          : sizeof(hfi_header) +
 *                                            sizeof(hfi_packet)
 * hfi_header.cmd_buff_info.type          : HFI_DEVICE
 *           .device_id                   : 0
 *           .object_id                   : n/a
 *           .timestamp_hi                : ts_hi
 *           .timestamp_lo                : ts_lo
 *           .header_id                   : unique id
 *           .num_packets                 : 2 + number of timing nodes
 *                                          (HFI_COMMAND_PANEL_INIT_PANEL_CAPS packet,
 *                                           HFI_COMMAND_PANEL_INIT_GENERIC_CAPS packet
 *                                           HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                           packet * no of timing modes of
 *                                           the panel)
 *
 * hfi_packet sample for any panel packet:
 * hfi_packet.payload_info.size           : sizeof(hfi_packet)
 *                                          (including payload size)
 *           .payload_info.type           : one of enum
 *                                          'hfi_packet_payload_type'
 *           .cmd                         : HFI_COMMAND_PANEL_XXX
 *           .flags (Host to DCP)         : HFI_TX_FLAGS_XXX
 *                                          (enum hfi_packet_tx_flags)
 *                  (DCP to Host)         : HFI_RX_FLAGS_XXX
 *                                          (enum hfi_packet_rx_flags)
 *           .id                          : 0
 *           .packet_id                   : unique id
 */

/*
 * HFI_COMMAND_PANEL_INIT_PANEL_CAPS - This is a host command sent to DCP to
 *                            initialize a panel. This command along with other
 *                            packet commands listed below make up to all the
 *                            fundamental information required to
 *                            setup / initialize a panel. Panel initialization
 *                            packet commands include:
 *                              - HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 *                                (Packet type 1, 1st packet)
 *                              - HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *                                (Packet type 2, can be 'n' number of packets. And
 *                                 'n' is number of timing nodes)
 *                              - HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *                                (Packet type 3, the packet follwoing to the
 *                                HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS, or
 *                                nothing but the last packet).
 *                            All these packets are sent together to DCP under
 *                            a single hfi header to initialize a panel.
 *
 * hfi_packet.payload_info.size        : sizeof(hfi_packet)
 *                                       (including payload size)
 *           .payload_info.type        : HFI_PAYLOAD_U32
 *           .cmd                      : HFI_COMMAND_PANEL_INIT_PANEL_CAPS
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .payload                  : <key/value> pairs of all panel
 *                                       setup data properties
 */
#define HFI_COMMAND_PANEL_INIT_PANEL_CAPS                            0x03000001

/*
 * HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS - This command carries the data
 *                                   specific to each timing mode of the panel.
 *
 * hfi_packet.payload_info.size        : sizeof(hfi_packet)
 *                                       (including payload size)
 *           .payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
 *           .flags                    : HFI_TX_FLAGS_NON_DISCARDABLE
 *           .payload                  : <key/value> pairs of all panel
 *                                       timing mode properties
 */
#define HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS                      0x03000002

/*
 * HFI_COMMAND_PANEL_INIT_GENERIC_CAPS - This command carries all the
 *                                       generic / NON-timing mode properties.
 *
 * hfi_packet.payload_info.size        : sizeof(hfi_packet)
 *                                       (including payload size)
 *           .payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
 *           .flags                    : HFI_TX_FLAGS_NON_DISCARDABLE
 *           .payload                  : <key/value> pairs of all panel
 *                                       generic properties
 */
#define HFI_COMMAND_PANEL_INIT_GENERIC_CAPS                          0x03000003

#define HFI_COMMAND_PANEL_END                                        0x03FFFFFF

#endif // __H_HFI_COMMANDS_PANEL_H__
