/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_COMMANDS_DEVICE_H__
#define __H_HFI_COMMANDS_DEVICE_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All Device level commands begin here.
 * "1st MSB byte = 0x01"
 */
#define HFI_COMMAND_DEVICE_BEGIN                                     0x01000000

/*
 * hfi_header sample for any device command:
 * hfi_header.cmd_buff_info.size          : (sizeof(hfi_header) +
 *                                            sizeof(hfi_packet)
 *           .cmd_buff_info.type          : HFI_DEVICE
 *           .device_id                   : 0
 *           .object_id                   : n/a
 *           .timestamp_hi                : ts_hi
 *           .timestamp_lo                : ts_lo
 *           .header_id                   : unique id
 *           .num_packets                 : 'n' packets depending upon the
 *                                          packet cmd
 *
 * hfi_packet sample for any device packet:
 * hfi_packet.payload_info.size           : sizeof(hfi_packet)
 *                                          (including payload size)
 *           .payload_info.type           : one of enum
 *                                          'hfi_packet_payload_type'
 *           .cmd                         : HFI_COMMAND_DEVICE_XXX
 *           .flags (Host to DCP)         : HFI_TX_FLAGS_XXX
 *                                          (enum hfi_packet_tx_flags)
 *                  (DCP to Host)         : HFI_RX_FLAGS_XXX
 *                                          (enum hfi_packet_rx_flags)
 *           .id                          : 0
 *           .packet_id                   : unique id
 */

/*
 * HFI_COMMAND_DEVICE_INIT - This command is used to initialize a device.
 *                       From host to dcp, this command triggers the
 *                       initialization of the device. This does not hold any
 *                       payload during communication.
 *                       As a response from DCP to host, this command packet
 *                       is returned along with firmware version as payload.
 *                       Accompying this packet are all other packets required
 *                       for initialization of a device.
 *
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_NONE
 *           .cmd                         : HFI_COMMAND_DEVICE_INIT
 *           .flags                       : HFI_TX_FLAGS_INTR_REQUIRED |
 *                                          HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *
 * DCP to Host:
 * hfi_header.num_packets                 : 7
 *
 * hfi_packet.payload_info.type           : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                         : HFI_COMMAND_DEVICE_INIT
 *           .flags                       : HFI_RX_FLAGS_SUCCESS
 *           .payload                     : <key, value> pair of
 *                                          dcp / mdss hw versions and dcp fw
 *                                          version.
 */
#define HFI_COMMAND_DEVICE_INIT                                      0x01000001

/*
 * In response to HFI_COMMAND_DEVICE_INIT command sent by host, DCP sends
 * multiple packets required for device initialization along with returning
 * HFI_COMMAND_DEVICE_INIT command packet as the first packet. Following
 * packets are detailed below:
 * HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS - Response packet [2] of
 *                                      HFI_COMMAND_DEVICE_INIT. This command
 *                                      carries the data specific to basic
 *                                      device setup.
 * Data Contents:
 *  - HFI_PROPERTY_DEVICE_INIT_VIG_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_DMA_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT
 *  - HFI_PROPERTY_DEVICE_INIT_WB_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION
 *  - HFI_PROPERTY_DEVICE_INIT_DSI_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION
 *  - HFI_PROPERTY_DEVICE_INIT_DP_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION
 *  - HFI_PROPERTY_DEVICE_INIT_DS_INDICES
 *  - HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : <key/value> pairs of Device properties
 *                                        (refer HFI_PROPERTY_DEVICE_INIT_XXX /
 *                                         Data Contents property definitions
 *                                         for this)
 */
#define HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS                  0x01000002

/*
 * HFI_COMMAND_DEVICE_INIT_VIG_CAPS - Response packet [3] of
 *                                    HFI_COMMAND_DEVICE_INIT. This command
 *                                    carries the data specific to VIG layers.
 * Data layout (Array sizes are variable and determined at runtime):
 *  @num_of_vig_formats: Number of formats supported by VIG
 *  @vig_format: Array of formats supported by VIG
 *  @vig_props: Array of vig properties <key, value> pairs.
 *  struct vig_data {
 *      u32 num_of_vig_formats;
 *      enum hfi_color_formats vig_format[num_of_vig_formats];
 *      struct property_data vig_props;
 *  }
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_VIG_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct vig_data
 */
#define HFI_COMMAND_DEVICE_INIT_VIG_CAPS                             0x01000003

/*
 * HFI_COMMAND_DEVICE_INIT_DMA_CAPS - Response packet [4] of
 *                                    HFI_COMMAND_DEVICE_INIT. This command
 *                                    carries the data specific to DMA layers.
 * Data layout (Array sizes are variable and determined at runtime):
 *  @num_of_dma_formats: Number of formats supported by DMA
 *  @dma_format: Array of formats supported by DMA
 *  @dma_props: Array of dma properties <key, value> pairs.
 *  struct dma_data {
 *      u32 num_of_dma_formats;
 *      enum hfi_color_formats dma_format[num_of_dma_formats];
 *      struct property_data dma_props;
 *  }
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_DMA_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct dma_data
 */
#define HFI_COMMAND_DEVICE_INIT_DMA_CAPS                             0x01000004

/*
 * HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS - Response packet [5] of
 *                                             HFI_COMMAND_DEVICE_INIT. This
 *                                             command carries the data common
 *                                             to both VIG and DMA layers.
 * Data layout:
 *  @common_props: Array of common (vig /dma) properties <key, value> pairs.
 *  struct common_data {
 *      struct property_data common_props;
 *  }
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct common_data
 */
#define HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS                    0x01000005

/*
 * HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS - Response packet [6] of
 *                                        HFI_COMMAND_DEVICE_INIT. This command
 *                                        carries the data specific to display.
 * Data Contents:
 *  - HFI_DISPLAY_PROPERTY_XXX
 * Data layout:
 *  @display_props: Array of display properties <key, value> pairs.
 *  struct display_data {
 *      struct property_data display_props;
 *  }
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct display_data
 */
#define HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS                         0x01000006

/*
 * HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS - Response packet [7] of
 *                                           HFI_COMMAND_DEVICE_INIT. This
 *                                           command carries the data specific
 *                                           to display writeback block.
 * Data Contents:
 *  - HFI_DISPLAY_WB_PROPERTY_XXX
 * Data layout (Array sizes are variable and determined at runtime):
 *  @num_of_wb_formats: Number of formats supported by Writeback block
 *  @wb_format: Array of formats supported by Writeback block
 *  @display_wb_props: Array of writeback block properties <key, value> pairs.
 *  struct display_wb_data {
 *      u32 num_of_wb_formats;
 *      enum hfi_color_formats wb_format[num_of_wb_formats];
 *      struct property_data display_wb_props;
 *  }
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct display_wb_data
 */
#define HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS                      0x01000007

/*
 * HFI_COMMAND_DEVICE_RESOURCE_REGISTER - This is a host command sent to DCP to
 *                                        register all resources like clocks
 *                                        and bandwidths. This command is sent
 *                                        to DCP as part of device
 *                                        initialization process. All resources
 *                                        (BWs properties) are added
 *                                        to the payload on this command packet.
 *                                        DCP is expected to read through this
 *                                        list of property IDs to register
 *                                        itself with all the clocks and BWs.
 * Data layout:
 *  struct resources_register {
 *      u32 num_of_resources;
 *      // property ids array of clks and BWs
 *      u32 resource_property_id[num_of_resources];
 *  }
 *
 * hfi_header.num_packets              : 1
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32
 *           .cmd                      : HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *           .flags                    : HFI_TX_FLAGS_RESPONSE_REQUIRED |
 *                                          HFI_TX_FLAGS_NON_DISCARDABLE
 *           .payload                  : struct resources_register
 */
#define HFI_COMMAND_DEVICE_RESOURCE_REGISTER                         0x01000008

/*
 * HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE - This is the callback command
 *                                             sent from DCP to host to request
 *                                             required resource voting for the
 *                                             value derived by DCP.
 * Data Layout:
 *  struct resource_vote {
 *      // <key/value> pairs of clks and BWs
 *      struct property_data_u64 vote_props;
 *  }
 *
 * hfi_header.num_packets           : 1
 *
 * hfi_packet.payload_info.type     : HFI_PAYLOAD_U64
 *           .cmd                   : HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE
 *           .flags                 : HFI_RX_FLAGS_NONE
 *           .payload               : struct resource_vote
 */
#define HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE                    0x01000009

/*
 * HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS - Response packet [8] of
 *                                    HFI_COMMAND_DEVICE_INIT. This command
 *                                    carries the data specific to VIG layers for rect 1.
 * Data layout (Array sizes are variable and determined at runtime):
 *  @num_of_vig_formats: Number of formats supported by VIG rect 1
 *  @vig_format: Array of formats supported by VIG rect 1
 *  @vig_props: Array of vig rect 1 properties <key, value> pairs.
 *  struct vig_r1_data {
 *      u32 num_of_vig_formats;
 *      enum hfi_color_formats vig_format[num_of_vig_formats];
 *      struct property_data vig_props;
 *  }
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct vig_r1_data
 */
#define HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS                          0x0100000A

/*
 * HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS - Response packet [9] of
 *                                    HFI_COMMAND_DEVICE_INIT. This command
 *                                    carries the data specific to DMA layers for rect 1.
 * Data layout (Array sizes are variable and determined at runtime):
 *  @num_of_dma_formats: Number of formats supported by DMA Rect 1
 *  @dma_format: Array of formats supported by DMA Rect 1
 *  @dma_props: Array of dma rect 1 properties <key, value> pairs.
 *  struct dma_r1_data {
 *      u32 num_of_dma_formats;
 *      enum hfi_color_formats dma_format[num_of_dma_formats];
 *      struct property_data dma_props;
 *  }
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS
 *           .flags                    : HFI_RX_FLAGS_SUCCESS
 *           .payload                  : struct dma_r1_data
 */
#define HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS                          0x0100000B

/**
 * HFI_COMMAND_DEVICE_LUT_DMA_LAST_CMD - This command is used to send the allocated
 *                                    REG-DMA Last Command buffer address and size
 *                                    from Host to DCP.
 * Host to DCP:
 * hfi_header.num_packets                 : 1
 *
 * Data layout:
 * struct hfi_buff_dpu - hfi buffer accessible by dpu
 * @flags    :  flags
 * @iova     :  input and output virtual address
 * @len      :  length of buffer
 * struct hfi_buff_dpu {
 *      u32 flags;
 *      u32 iova;
 *      u32 len;
 * }
 *
 * hfi_packet.payload_info.type        : HFI_PAYLOAD_U32_ARRAY
 *           .cmd                      : HFI_COMMAND_DEVICE_LUT_DMA_LAST_CMD
 *           .flags                    : HFI_TX_FLAGS_NONE
 *           .payload                  : struct hfi_buff_dpu
 */
#define HFI_COMMAND_DEVICE_LUT_DMA_LAST_CMD                          0x0100000C

#define HFI_COMMAND_DEVICE_END                                       0x01FFFFFF

#endif // __H_HFI_COMMANDS_DEVICE_H__

