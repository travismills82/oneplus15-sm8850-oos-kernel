/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_PROPERTIES_DEBUG_H__
#define __H_HFI_PROPERTIES_DEBUG_H__

/*
 * All debug property IDs begin here.
 */

 #define HFI_PROPERTY_DEBUG_BEGIN                                    0x00050000

 /*
  * HFI_PROPERTY_DEBUG_REG_ALLOC - DCP informs HLOS the size required for dumping registers.
  * Firmware is expected to send this property as part of HFI_COMMAND_DEBUG_INIT
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_REG_ALLOC
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_REG_ALLOC |
  *                               (version=0 << 20) | (dsize=1 << 24)
  *   (u32_value) payload [1]   : u32 size
  */
#define HFI_PROPERTY_DEBUG_REG_ALLOC                                0x00050001

 /*
  * HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC - DCP informs HLOS the size required for dumping debug bus
  * Firmware is expected to send this property as part of HFI_COMMAND_DEBUG_INIT
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC |
  *                               (version=0 << 20) | (dsize=1 << 24)
  *   (u32_value) payload [1]   : u32 size
  */
#define HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC                            0x00050002

 /*
  * HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC - DCP informs HLOS the size required for dumping event log
  * Firmware is expected to send this property as part of HFI_COMMAND_DEBUG_INIT
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC |
  *                               (version=0 << 20) | (dsize=1 << 24)
  *   (u32_value) payload [1]   : u32 size
  */
#define HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC                           0x00050003

 /*
  * HFI_PROPERTY_DEBUG_STATE_ALLOC - DCP informs HLOS the size required for dumping state variable
  * Firmware is expected to send this property as part of HFI_COMMAND_DEBUG_INIT
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_STATE_ALLOC
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_STATE_ALLOC |
  *                               (version=0 << 20) | (dsize=1 << 24)
  *   (u32_value) payload [1]   : u32 size
  */
#define HFI_PROPERTY_DEBUG_STATE_ALLOC                             0x00050004

 /*
  * HFI_PROPERTY_DEBUG_REG_ADDR - HLOS informs DCP the buffer address for dumping registers
  * HLOS is expected to send this property as part of HFI_COMMAND_DEBUG_SETUP
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_REG_ADDR
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_REG_ADDR |
  *                               (version=0 << 20) | (dsize=5 << 24)
  *   (u32_value) payload [1-5]   : struct hfi_buff
  */
#define HFI_PROPERTY_DEBUG_REG_ADDR                                0x00050005

 /*
  * HFI_PROPERTY_DEBUG_DBG_BUS_ADDR - HLOS informs DCP the buffer address for dumping debug bus
  * HLOS is expected to send this property as part of HFI_COMMAND_DEBUG_SETUP
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_DBG_BUS_ADDR
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_DBG_BUS_ADDR |
  *                               (version=0 << 20) | (dsize=5 << 24)
  *   (u32_value) payload [1-5]   : struct hfi_buff
  */
#define HFI_PROPERTY_DEBUG_DBG_BUS_ADDR                            0x00050006

 /*
  * HFI_PROPERTY_DEBUG_EVT_LOG_ADDR - HLOS informs DCP the buffer address for dumping event log
  * HLOS is expected to send this property as part of HFI_COMMAND_DEBUG_SETUP
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_EVT_LOG_ADDR
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_EVT_LOG_ADDR |
  *                               (version=0 << 20) | (dsize=5 << 24)
  *   (u32_value) payload [1-5]   : struct hfi_buff
  */
#define HFI_PROPERTY_DEBUG_EVT_LOG_ADDR                            0x00050007

 /*
  * HFI_PROPERTY_DEBUG_STATE_ADDR - HLOS informs DCP the buffer address for dumping state variable
  * HLOS is expected to send this property as part of HFI_COMMAND_DEBUG_SETUP
  * command packet payload.
  *
  * @BasicFuntionality @DeviceInit - HFI_PROPERTY_DEBUG_STATE_ADDR
  *     (u32_key) payload [0]   : HFI_PROPERTY_DEBUG_STATE_ADDR |
  *                               (version=0 << 20) | (dsize=5 << 24)
  *   (u32_value) payload [1-5]   : struct hfi_buff
  */
#define HFI_PROPERTY_DEBUG_STATE_ADDR                              0x00050008

#define HFI_PROPERTY_DEBUG_END                                     0x0005FFFF

#endif // __H_HFI_PROPERTIES_DEBUG_H__
