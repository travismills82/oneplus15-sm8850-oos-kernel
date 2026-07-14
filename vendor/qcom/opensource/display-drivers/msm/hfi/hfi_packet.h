/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PACKET_H__
#define __H_HFI_PACKET_H__

#include <hfi_defs_common.h>

/*
 * This is documentation file. Not used for header inclusion.
 * - HFI_COMMAND_XXX  - These are hfi_packet commands.
 * - HFI_PROPERTY_XXX - These are hfi properties that are transacted through
 *                      payload of hfi packets
 * - All commands are classified into different types based on their level of
 *   operation. For eg: device level commands, display level commands, layer
 *   level commands etc. These commands are grouped as such using the 1st MSB
 *   byte of the 32 bit commands definitions.
 * - Declaration of underlying structures used in description of Data layout
 *   through out any hfi_commands_x.h / hfi_properties_x.h file are detailed
 *   below. These structures, along with any other Data Layout structures
 *   contained in hfi_commands_x.h / hfi_properties_x.h contain variable size
 *   arrays whose size is determined at runtime depending upon the type of
 *   property. Hence these structures are not straight forward to use for
 *   implementation. But rather be customized for array sizes depending upon
 *   the command type during implementation.
 *   struct property_u32 / property_u64
 *   @property_key: This serves as the 'key' for <key, value> pair.
 *                This u32 value is divided as below:
 *                Bits 0-15 : Property ID
 *                Bits 16-19: Property Type
 *                Bits 20-23: Property Version
 *                Bits 24-31: Property value size in Dwords (Beyond 256
 *                            Dwords, properties must use mapped buffers)
 *   @property_value: This serves as the 'value for <key, value> pair.
 *                  'x' being the size of array, is determined by
 *                  bits 24-31 of property_key.
 *                  For u32 'x'= 1, payload info is HFI_PAYLOAD_U32,
 *                  For u32 'x' > 1, payload info is HFI_PAYLOAD_U32_ARRAY
 *   struct property_u32 {
 *      u32 property_key;
 *      u32 property_value[x];
 *   };
 *   struct property_u64 {
 *      u32 property_key;
 *      u64 property_value[x];
 *   };
 *
 *   struct property_data / property_data_u64:
 *   @num_of_properties: Total number of properties or <key, value> pairs
 *   @props:             Array of <key, value> pairs.
 *   struct property_data {
 *      u32 num_of_properties;
 *      struct property_u32 props[num_of_properties];
 *   };
 *   struct property_data_u64 {
 *      u32 num_of_properties;
 *      struct property_u64 props[num_of_properties];
 *   };
 *
 *   struct property_array:
 *   @count   : Total number of values available (array size)
 *   @values : Array of u32 values
 *   struct property_array {
 *      u32 count;
 *      u32 values[count];
 *  };
 *
 * Property Tag Details:
 * @BasicFuntionality => required for basic functionality
 * @VIG => valid for VIG layer only properties
 * @DMA => valid for VIG layer only properties
 * @VIGandDMA => valid for VIG and DMA layer properties
 * @Layer => valid for Layer Only properties
 * @Display => valid for Display Only Properties
 * @DeviceInit => valid for MDSS Initialization
 * @PanelInit => valid for Panel Initialization
 */

/*
 * @cmd_buff_info : Bits 0:23 (size) - Total size of all packets in bytes
 *                              (including headers).
 *                  Bits 24:31 (type) - Commands Buffer type identifier for
 *                            which all the Packets are applicable (System,
 *                            Device, Display, Debug). E.g. For a HFI container
 *                            holding the ‘commit’ data, the ‘Display’ type
 *                            should be specified. If changes are required
 *                            in future to this header, they must be
 *                            reflected in this type, therefore implementers
 *                            must confirm a known type before typecasting
 *                            this header. Value of this is one of enum
 *                            "hfi_cmd_buff_type". This field is denoted
 *                            by ‘cmd_buff_type’.
 * @device_id : Device id for multiple devices in a single channel
 *              (Reserved for future use).
 * @object_id : This object is an identifier to be interpreted depending on the
 *              ‘cmd_buff_type’, its purpose is to allow the decoder of the
 *              commands buffer to take decisions ahead of time about the
 *              processing of the packets that will follow this header.
 *              E.g. for a HFI Commands-Buffer, holding the ‘Display’ type,
 *              this field will carry the Display-ID for which the commands
 *              buffers are applicable, which allows the decoder side of this
 *              commands buffer to schedule the job accordingly.
 * @timestamp_hi : Timestamp high-value.
 * @timestamp_lo : Timestamp low-value.
 * @header_id : Unique identifier for this Header. This is an opaque identifier
 *              for DCP, as this is populated by the Host, and DCP will return
 *              the same identifier as part of the response.
 * @reserved : Reserved for future use.
 * @num_packets : Number of HFI Packets included with this header.
 */
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

/*
 * @payload_info : Bits 0:20 (size) - Size of the Packet in bytes
 *                                   (including Payload)
 *                 Bits 21:31 (type) - Payload info, i.e. type of the payload
 *                                     (e.g. blob, u32, etc). One of enum
 *                                     hfi_packet_payload_type
 * @cmd          : Command to be executed
 *                 (e.g. HFI_COMMAND_DISPLAY_SET_PROPERTY).
 *                 Where, the first MSB byte hold the 'command type'
 *                 (e.g. device-init type), and rest of the bits, would hold
 *                 the specific command
 * @flags        : Packet Flags. Represented as bit-masks. Depend on sender
 *                 of packet:
 *                 Host_packet_flags: Flags set by Host
 *                                    from enum hfi_packet_host_flags
 *                 fw_packet_flags: Flags set by Firmware
 *                                      from enum hfi_packet_fw_flags
 * @id           : Bits 0:15: Carries the per-type id, e.g. for display
 *                            commands it carries the display-id, for
 *                            layer-commands it carries the layer-id,
 *                            depending on the command type.
 *                 Bits 16:31: Reserved for future use
 * @packet_id    : host hfi_packet contains unique packet id.
 *                 Firmware returns host packet id in response packets when
 *                 applicable. If not applicable, Firmware sets it to zero.
 * @reserved[3]  : reserved for future use
 * @payload      : The actual payload carried by this packet starts where the
 *                 'reserved' field ends. The size of this payload would be
 *                 determined by the 'payload_info'.
 */
struct hfi_packet {
	u32 payload_info;
	u32 cmd;
	u32 flags;
	u32 id;
	u32 packet_id;
	u32 reserved[3];
};

/*
 * HFI_PAYLOAD_NONE      : No payload within the packet
 * HFI_PAYLOAD_U32       : Payload contains a single u32 value.
 * HFI_PAYLOAD_U32_ARRAY : Payload contains multiple u32 values.
 * HFI_PAYLOAD_U64       : Payload contains a single u64 value.
 * HFI_PAYLOAD_U64_ARRAY : Payload contains multiple u64 values.
 * HFI_PAYLOAD_BLOB      : Payload contains a variable size binary blob.
 */
enum hfi_packet_payload_type {
	HFI_PAYLOAD_NONE                = 0x0,
	HFI_PAYLOAD_U32                 = 0x1,
	HFI_PAYLOAD_U32_ARRAY           = 0x2,
	HFI_PAYLOAD_U64                 = 0x3,
	HFI_PAYLOAD_U64_ARRAY           = 0x4,
	HFI_PAYLOAD_BLOB                = 0x5,
};

/*
 * HFI_CMD_BUFF_SYSTEM  : Command Buffer to carry system-level commands.
 *                       These perform modifications or query information
 *                       at a System Level.
 * HFI_CMD_BUFF_DEVICE  : Command Buffer to carry Device-level commands.
 *                       These perform modifications or query information
 *                       at a Device Level.
 * HFI_CMD_BUFF_DISPLAY : Command Buffer to carry Display-level commands.
 *                       These perform modifications or query information
 *                       at a Display Level.
 *                       This can include Layer + Events Commands
 * HFI_CMD_BUFF_DEBUG   : Command Buffer to carry Debug commands.
 *                       This type can be included standalone or within
 *                       the display group
 * HFI_CMD_BUFF_VIRTUALIZATION : Command Buffer to carry Virtualization
 *                              commands. These are only allowed to Trusted
 *                              Hosts. Commands carry System Level
 *                              Packet types.
 */
enum hfi_cmd_buff_type {
	HFI_CMD_BUFF_SYSTEM          = 0x1,
	HFI_CMD_BUFF_DEVICE          = 0x2,
	HFI_CMD_BUFF_DISPLAY         = 0x3,
	HFI_CMD_BUFF_DEBUG           = 0x4,
	HFI_CMD_BUFF_VIRTUALIZATION  = 0x5,
};

/*
 * HFI_TX_FLAGS_NONE              : None
 * HFI_TX_FLAGS_INTR_REQUIRED     : Interrupt required for this command
 * HFI_TX_FLAGS_RESPONSE_REQUIRED : Receiver of the message is required to send
 *                                  a response packet
 *                                  (but not necessarily an irq).
 * HFI_TX_FLAGS_NON_DISCARDABLE   : Receiver of the message is not expected to
 *                                  discard the packet (even if something is
 *                                  invalid).
 *                                  This to be used during system, bind/unbind,
 *                                  etc. commands.
 */
enum hfi_packet_tx_flags {
	HFI_TX_FLAGS_NONE              = 0x0,
	HFI_TX_FLAGS_INTR_REQUIRED     = 0x1,
	HFI_TX_FLAGS_RESPONSE_REQUIRED = 0x2,
	HFI_TX_FLAGS_NON_DISCARDABLE   = 0x4,
};

/*
 * HFI_RX_FLAGS_NONE        : None
 * HFI_RX_FLAGS_SUCCESS     : Flag is set when the receiver of the message has
 *                            successfully processed the packet.
 * HFI_RX_FLAGS_INFORMATION : Flag is set by the receiver of the packet
 *                            if the processing of a packet received from the
 *                            sender resulted in additional information
 *                            (like an error) and the receiver can continue,
 *                            but needs to notify the sender of the
 *                            information.
 * HFI_RX_FLAGS_DEVICE_ERROR : Flag is set by the receiver of the packet as
 *                             a response to sender if the received packet
 *                             processing resulted in error.
 *                             Sender of the packet will follow-up and
 *                             handle the error.
 * HFI_RX_FLAGS_SYSTEM_ERROR : Flag is set by the receiver of the packet as a
 *                             response to the packet sender when a
 *                             system-level error has occurred.
 */
enum hfi_packet_rx_flags {
	HFI_RX_FLAGS_NONE               = 0x0,
	HFI_RX_FLAGS_SUCCESS            = 0x1,
	HFI_RX_FLAGS_INFORMATION        = 0x2,
	HFI_RX_FLAGS_DEVICE_ERROR       = 0x4,
	HFI_RX_FLAGS_SYSTEM_ERROR       = 0x8,
};

#endif // __H_HFI_PACKET_H__
