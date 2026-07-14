/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PROPERTIES_DEVICE_H__
#define __H_HFI_PROPERTIES_DEVICE_H__

/*
 * This is documentation file. Not used for header inclusion.
 */

/*
 * All device property IDs begin here.
 * For device properties: 16 - 19 bits of the property id = 0x1
 */
#define HFI_PROPERTY_DEVICE_BEGIN                                    0x00010000

/*
 * HFI_PROPERTY_DEVICE_INIT_DCP_HW_VERSION - Gets DCP hardware version. Firmware
 *                                       is expected to send this property as
 *                                       part of
 *                                       HFI_CMD_DEVICE_INIT command packet
 *                                       payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DCP_HW_VERSION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_DCP_HW_VERSION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : dcp hardware version
 */
#define HFI_PROPERTY_DEVICE_INIT_DCP_HW_VERSION                      0x00010001

/*
 * HFI_PROPERTY_DEVICE_INIT_MDSS_HW_VERSION - Gets MDSS hardware version.
 *                                       Firmware is expected to send this
 *                                       property as part of
 *                                       HFI_CMD_DEVICE_INIT command packet
 *                                       payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_MDSS_HW_VERSION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_MDSS_HW_VERSION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : mdss hardware version
 */
#define HFI_PROPERTY_DEVICE_INIT_MDSS_HW_VERSION                      0x00010002

/*
 * HFI_PROPERTY_DEVICE_INIT_VIG_INDICES - Gets VIG layer count and indices, where
 *                                        For each layer index, which is 4 Bytes total size:
 *                                        The upper 1 MSB Byte is Rect-Id
 *                                        The lower 3 LSB Bytes are Layer-Id.
 *                                        For the Most Significant 1 Byte (MSB), the Rect-Id,
 *                                        if index:
 *                                        MSB is 0 : The index is for Rect 0 Layer
 *                                        MSB is 1 : The index is for Rect 1 Layer
 *                                        For the Least Significant 3 Bytes (LSB), the Layer-Id
 *                                        matches the original Rect (Rect 0 or Rect 1) ID.
 *                                        E.g. if vig index is 0x01000000(i.e., VIG_0_REC1),
 *                                        The upper 1 MSB Byte is 01 for Rect 1,
 *                                        The lower 3 LSB Bytes is 000000 for layer id 0
 *                                        Firmware is expected to send this property
 *                                        as part of HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                        command packet payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_VIG_INDICES
 *     (u32_key) payload [0]       : HFI_PROPERTY_DEVICE_INIT_VIG_INDICES |
 *                                   (version=0 << 20) | (dsize=n+1 << 24)
 *     (u32_key) payload [1]       : count of vig (n)
 *   (u32_value) payload [2..n]    : array of 'n' vig_indices
 */
#define HFI_PROPERTY_DEVICE_INIT_VIG_INDICES                         0x00010004

/*
 * HFI_PROPERTY_DEVICE_INIT_DMA_INDICES - Gets DMA layer count and indices. where,
 *                                      For each layer index, which is 4 Bytes total size:
 *                                      The upper 1 MSB Byte is Rect-Id
 *                                      The lower 3 LSB Bytes are Layer-Id.
 *                                      For the Most Significant 1 Byte (MSB), the Rect-Id,
 *                                      if index:
 *                                      MSB is 0 : The index is for Rect 0 Layer
 *                                      MSB is 1 : The index is for Rect 1 Layer
 *                                      For the Least Significant 3 Bytes (LSB), the Layer-Id
 *                                      matches the original Rect (Rect 0 or Rect 1) ID.
 *                                      E.g. if dma index is 0x01000000(i.e., DMA_0_REC1),
 *                                      The upper 1 MSB Byte is 01 for Rect 1,
 *                                      The lower 3 LSB Bytes is 000000 for layer id 0
 *                                      Firmware is expected to send this property
 *                                      as part of HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                      command packet payload.
 * Data layout:
 *  struct property_array dma_indices;
 *  dma_indices.values = {x, x+1, ..} - starts from the next integer
 *                                            value of vig_indices
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DMA_INDICES
 *     (u32_key) payload [0]       : HFI_PROPERTY_DEVICE_INIT_DMA_INDICES |
 *                                   (version=0 << 20) | (dsize=7 << 24)
 *     (u32_key) payload [1]       : dma_indices.count
 *   (u32_value) payload [2]       : dma_indices.values[]
 */
#define HFI_PROPERTY_DEVICE_INIT_DMA_INDICES                         0x00010005

/*
 * HFI_PROPERTY_DEVICE_INIT_DCP_FW_VERSION - Gets DCP firmware version.
 *                                       Firmware is expected to send this
 *                                       property as part of
 *                                       HFI_CMD_DEVICE_INIT command packet
 *                                       payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DCP_FW_VERSION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_DCP_FW_VERSION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : dcp firmware version
 */
#define HFI_PROPERTY_DEVICE_INIT_DCP_FW_VERSION                      0x00010003


/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT - Gets max display count.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : max_display_count
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_DISPLAY_COUNT                   0x00010006

/*
 * HFI_PROPERTY_DEVICE_INIT_WB_INDICES - Gets writeback block count and indices.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 * Data layout:
 *  struct property_array wb_indices;
 *  wb_indices.values = {0, ..}
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_WB_INDICES
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_WB_INDICES |
 *                               (version=0 << 20) | (dsize=2 << 24 )
 *   (u32_value) payload [1]   : wb_indices.count
 *   (u32_value) payload [2]   : wb_indices.values[]
 */
#define HFI_PROPERTY_DEVICE_INIT_WB_INDICES                          0x00010007

/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION - Gets maximum writeback
 *                                  linear resolution. Firmware is expected
 *                                  send this property as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit -
 *              HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION
 *     (u32_key) payload [0]   :
 *                          HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : writeback_max_linear_width << 16 |
 *                               writeback_max_linear_height
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_WB_LINEAR_RESOLUTION            0x00010008

/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION - Gets maximum writeback
 *                                  ubwc resolution. Firmware is expected
 *                                  send this property as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit -
 *              HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION
 *     (u32_key) payload [0]   :
 *                          HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : writeback_max_ubwc_width << 16 |
 *                               writeback_max_ubwc_height
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_WB_UBWC_RESOLUTION              0x00010009

/*
 * HFI_PROPERTY_DEVICE_INIT_DSI_INDICES - Gets DSI block count and indices.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 * Data layout:
 *  struct property_array dsi_indices;
 *  dsi_indices.values = {HFI_DISPLAY_INDEX_X, ..}
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DSI_INDICES
 *     (u32_key) payload [0]       : HFI_PROPERTY_DEVICE_INIT_DSI_INDICES |
 *                                   (version=0 << 20) | (dsize=6 << 24 )
 *   (u32_value) payload [1]       : dsi_indices.count
 *   (u32_value) payload [2]       : dsi_indices.values[]
 */
#define HFI_PROPERTY_DEVICE_INIT_DSI_INDICES                         0x0001000A

/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION - Gets maximum DSI resolution.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : dsi_max_width << 16 | dsi_max_height
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_DSI_RESOLUTION                  0x0001000B

/*
 * HFI_PROPERTY_DEVICE_INIT_DP_INDICES - Gets DP block count and indices.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 * Data layout:
 *  struct property_array dp_indices;
 *  dp_indices.values = {HFI_DISPLAY_INDEX_X, ..}
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DP_INDICES
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_DP_INDICES |
 *                                (version=0 << 20) | (dsize=2 << 24 )
 *   (u32_value) payload [1]   : dp_indices.count
 *   (u32_value) payload [2]   : dp_indices.values[]
 */
#define HFI_PROPERTY_DEVICE_INIT_DP_INDICES                          0x0001000C

/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION - Gets maximum dp block
 *                                  resolution. Firmware is expected send this
 *                                  property as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : dp_max_width << 16 | dp_max_height
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_DP_RESOLUTION                   0x0001000D

/*
 * HFI_PROPERTY_DEVICE_INIT_DS_INDICES - Gets DS block count and indices.
 *                                  Firmware is expected to send this property
 *                                  as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 * Data layout:
 *  struct property_array ds_indices;
 *  ds_indices.values = {0, ..}
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_DS_INDICES
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_DS_INDICES |
 *                                (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : ds_indices.count
 *   (u32_value) payload [2]   : ds_indices.values[]
 */
#define HFI_PROPERTY_DEVICE_INIT_DS_INDICES                          0x0001000E

/*
 * HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION - Gets maximum DS block
 *                                  resolution. Firmware is expected to send
 *                                  this property as part of
 *                                  HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS
 *                                  command packet payload.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION |
 *                               (version=0 << 20) | (dsize=1 << 24)
 *   (u32_value) payload [1]   : ds_max_width << 16 | ds_max_height
 */
#define HFI_PROPERTY_DEVICE_INIT_MAX_DS_RESOLUTION                   0x0001000F

/* Device Resource Property IDs start here */

/*
 * HFI_PROPERTY_DEVICE_CORE_AB - This property defines DPU AB vote for
 *                              MNOC(SDE_virt_master-> SDE_virt_slave(SDE_NOC))
 *                              in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_CORE_AB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_CORE_AB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_CORE_AB                                  0x00010010

/*
 * HFI_PROPERTY_DEVICE_CORE_IB - This property defines DPU IB vote for
 *                              MNOC(SDE_virt_master-> SDE_virt_slave(SDE_NOC))
 *                              in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_CORE_IB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_CORE_IB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_CORE_IB                                  0x00010011

/*
 * HFI_PROPERTY_DEVICE_CORE_POWER_RAIL - This property defines DPU power rail
 *                                       vote.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP.
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_CORE_POWER_RAIL
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_CORE_POWER_RAIL |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_CORE_POWER_RAIL                          0x00010012

/*
 * HFI_PROPERTY_DEVICE_LLCC_AB - This property defines DPU LLCC AB vote
 *                               for LLCC(SDE_master_on_MNOC->LLCC) in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_LLCC_AB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_LLCC_AB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_LLCC_AB                                  0x00010013

/*
 * HFI_PROPERTY_DEVICE_LLCC_IB - This property defines DPU LLCC IB vote
 *                               for LLCC(SDE_master_on_MNOC->LLCC) in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_LLCC_IB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_LLCC_IB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_LLCC_IB                                  0x00010014

/*
 * HFI_PROPERTY_DEVICE_DRAM_AB - This property defines DPU DRAM AB vote
 *                               for DRAM(LLCC(SDE)->DDR) in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_DRAM_AB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_DRAM_AB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_DRAM_AB                                  0x00010015

/*
 * HFI_PROPERTY_DEVICE_DRAM_IB - This property defines DPU DRAM IB vote
 *                               for DRAM(LLCC(SDE)->DDR) in Kbps.
 * Host to DCP:
 * - This resource property is added to HFI_COMMAND_DEVICE_RESOURCE_REGISTER
 *   device command packet payload to register this resource to DCP.
 * DCP to Host Callback:
 * - Since this resource is registered with DCP, DCP can callback to HLOS
 *   with this property via HFI_COMMAND_DEVICE_CALLBACK_RESOURCE_VOTE device
 *   command to vote a specific value it derived for this resource.
 *
 * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEVICE_DRAM_IB
 *     (u32_key) payload [0]   : HFI_PROPERTY_DEVICE_DRAM_IB |
 *                               (version=0 << 20) | (dsize=2 << 24)
 *   (u32_value) payload [1]   : vote_data_lsb
 *   (u32_value) payload [2]   : vote_data_msb
 */
#define HFI_PROPERTY_DEVICE_DRAM_IB                                  0x00010016

/* Device Resource Property IDs end here */

/*
 * All device property IDs end here
 */
#define HFI_PROPERTY_DEVICE_END                                      0x0001FFFF

#endif // __H_HFI_PROPERTIES_DEVICE_H__

