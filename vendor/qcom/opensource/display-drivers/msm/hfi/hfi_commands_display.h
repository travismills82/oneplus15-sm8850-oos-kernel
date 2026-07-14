/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_COMMANDS_DISPLAY_H__
#define __H_HFI_COMMANDS_DISPLAY_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All Display level commands begin here.
 * "1st MSB byte = 0x02"
 */
#define HFI_COMMAND_DISPLAY_BEGIN                                     0x02000000

/*
 * hfi_header sample for any display command:
 * hfi_header.cmd_buff_info.size          : (sizeof(hfi_header) +
 *                                            sizeof(hfi_packet))
 *           .cmd_buff_info.type          : HFI_DISPLAY
 *           .device_id                   : 0
 *           .object_id                   : n/a
 *           .timestamp_hi                : ts_hi
 *           .timestamp_lo                : ts_lo
 *           .header_id                   : unique id
 *           .num_packets                 : 'n' packets depending upon the
 *                                          packet cmd
 *
 * hfi_packet sample for any display packet:
 * hfi_packet.payload_info.size           : sizeof(hfi_packet)
 *                                          (including payload size)
 *           .payload_info.type           : one of enum
 *                                          'hfi_packet_payload_type'
 *           .cmd                         : HFI_COMMAND_DISPLAY_XXX
 *           .flags (Host to DCP)         : HFI_TX_FLAGS_XXX
 *                                          (enum hfi_packet_host_flags)
 *                  (DCP to Host)         : HFI_RX_FLAGS_XXX
 *                                          (enum hfi_fw_host_flags)
 *           .id                          : display id
 *           .packet_id                   : unique id
 */

/*
 * HFI_COMMAND_DISPLAY_SET_PROPERTY  - This command sets display property.
 *                                     From host to DCP, this command sets different Properties
 *                                     for configuring/re-configuring a Display at run-time.
 *                                     This command holds properties IDs to set in the payload.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * Data Contents:
 *  - HFI_DISPLAY_PROPERTY_XXX
 * Data layout:
 *  struct display_data {
 *      struct property_data display_props;
 *  }
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                         : HFI_COMMAND_DISPLAY_SET_PROPERTY
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id for which the
 *                                          properties are applicable
 *           .payload                     : struct display_data
 */
#define HFI_COMMAND_DISPLAY_SET_PROPERTY                              0x02000001

/*
 * HFI_COMMAND_DISPLAY_GET_PROPERTY - This is a host command sent to DCP to get display/layer
 *                                     properties. This command holds the property ID in the
 *                                     payload.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_GET_PROPERTY
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id for which the properties
 *                                          are applicable
 *           .payload                     : property_id
 */
#define HFI_COMMAND_DISPLAY_GET_PROPERTY                              0x02000002

/*
 * HFI_COMMAND_DISPLAY_FRAME_TRIGGER - This is a host command sent to DCP to commit the info from
 *                                     previous packets. This command holds commit flags.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_FRAME_TRIGGER
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id for which the properties
 *                                          are applicable
 *(u32_value).payload [0]                 : one of the commit flags in enum hfi_display_commit_flag
 */
#define HFI_COMMAND_DISPLAY_FRAME_TRIGGER                             0x02000003

/*
 * HFI_COMMAND_DISPLAY_EVENT_REGISTER - This a host command sent to DCP to register for an
 *                                      event notification.The event ID is given in the payload
 *                                      of this command.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_EVENT_REGISTER
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id for which the properties
 *                                          are applicable
 *           .payload                     : one of the event id's in enum hfi_display_event_id
 */
#define HFI_COMMAND_DISPLAY_EVENT_REGISTER                            0x02000004

/*
 * HFI_COMMAND_DISPLAY_EVENT_DEREGISTER - This is a host command sent to DCP to unregister the event
 *                                        in the payload. The event ID is given in the payload of
 *                                        this command.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_EVENT_DEREGISTER
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id for which the properties
 *                                          are applicable
 *           .payload                     : one of the event id's in enum hfi_display_event_id
 */
#define HFI_COMMAND_DISPLAY_EVENT_DEREGISTER                          0x02000005

/*
 * HFI_COMMAND_DISPLAY_ENABLE  -       From host to DCP, this command is used to enable the display
 *                                     and send panel ON commands.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                         : HFI_COMMAND_DISPLAY_ENABLE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : None
 */
#define HFI_COMMAND_DISPLAY_ENABLE                                    0x02000006

/*
 * HFI_COMMAND_DISPLAY_POST_ENABLE  -  From host to DCP, this command is used to send display
 *                                     post ON commands.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_NONE
 *           .cmd                         : HFI_COMMAND_DISPLAY_POST_ENABLE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : None
 */
#define HFI_COMMAND_DISPLAY_POST_ENABLE                               0x02000007

/*
 * HFI_COMMAND_DISPLAY_DISABLE  -  From host to DCP, this command is used to send
 *                                 display pre-off commands.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_NONE
 *           .cmd                         : HFI_COMMAND_DISPLAY_DISABLE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : None
 */
#define HFI_COMMAND_DISPLAY_DISABLE                                   0x02000008

/*
 * HFI_COMMAND_DISPLAY_POST_DISABLE  -  From host to DCP, this command is used to send
 *                                      display off commands.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_NONE
 *           .cmd                         : HFI_COMMAND_DISPLAY_POST_DISABLE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : None
 */
#define HFI_COMMAND_DISPLAY_POST_DISABLE                              0x02000009

/*
 * HFI_COMMAND_DISPLAY_SET_MODE  - From host to DCP, this command gives final mode to
 *                                 DCP which is already validated by mode_validate.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                         : HFI_COMMAND_DISPLAY_SET_MODE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : struct hfi_buff for struct hfi_display_mode_info
 */
#define HFI_COMMAND_DISPLAY_SET_MODE                                  0x0200000A

/*
 * HFI_COMMAND_DISPLAY_MODE_VALIDATE  - From Host to DCP, this command provides mode to
 *                                      DCP for validation.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                         : HFI_COMMAND_DISPLAY_MODE_VALIDATE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : struct hfi_buff for struct hfi_display_mode_info
 */
#define HFI_COMMAND_DISPLAY_MODE_VALIDATE                             0x0200000B

/*
 * HFI_COMMAND_DISPLAY_POWER_CONTROL  -    From DCP to Host, this command
 *                                         tells Host to power ON/OFF Panel/Controller/Phy
 *                                         power supplies. It will also handle Panel Reset
 *                                         with HFI_PANEL_POWER.
 *                                         HFI_TRUE(Enable)/HFI_FALSE(Disable).
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_POWER_CONTROL
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload[0]                  : Bitmask of enum hfi_display_power_control
 *           .payload[1]                  : HFI_TRUE / HFI_FALSE
 */
#define HFI_COMMAND_DISPLAY_POWER_CONTROL                             0x0200000C

/*
 * HFI_COMMAND_DISPLAY_POWER_REGISTER  -    From Host to DCP, this command
 *                                          tells Host to register power with DCP. DCP
 *                                          response is HFI_COMMAND_DISPLAY_POWER_CONTROL.
 *
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_NONE
 *           .cmd                         : HFI_COMMAND_DISPLAY_POST_DISABLE
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED(optional) |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : None
 */
#define HFI_COMMAND_DISPLAY_POWER_REGISTER                            0x0200000D

/*
 * HFI_COMMAND_DISPLAY_LP_STATE_REQ   -     From Host to DCP, this command
 *                                          tells DCP what power state to transition to
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32
 *           .cmd                         : HFI_COMMAND_DISPLAY_LP_STATE_REQ
 *           .flags                       : HFI_TX_FLAGS_NON_DISCARDABLE
 *           .id                          : Bits 0:15 carry the display id
 *           .packet_id                   : unique id
 *           .payload                     : enum hfi_display_power_mode;

 */
#define HFI_COMMAND_DISPLAY_LP_STATE_REQ                              0x0200000E

#define HFI_COMMAND_DISPLAY_END                                       0x02FFFFFF

#endif // __H_HFI_COMMANDS_DISPLAY_H__
