/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __H_HFI_DEFS_DEBUG_H__
#define __H_HFI_DEFS_DEBUG_H__

/*
 * This is documentation file. Not used for header inclusion.
 */
/**
 * @brief Definitions for display panic events.
 */

/**
 * @def HFI_DEBUG_EVENT_UNDERRUN
 * @brief Bitmask for event indicating an underrun.
 */
#define HFI_DEBUG_EVENT_UNDERRUN   (1 << 0)

/**
 * @def HFI_DEBUG_EVENT_HW_RESET
 * @brief Bitmask for event indicating a hardware reset.
 */
#define HFI_DEBUG_EVENT_HW_RESET   (1 << 1)

/**
 * @def HFI_DEBUG_EVENT_PP_TIMEOUT
 * @brief Bitmask for event indicating a ping-pong timeout.
 */
#define HFI_DEBUG_EVENT_PP_TIMEOUT (1 << 2)

/**
 * struct debug_display_evt_info - display panic event info
 * @display_id: Display ID
 * @events_mask: Bitwise 'OR' of panic event flags
 * @enable: Enable/Disable events
 */
struct debug_display_evt_info {
	uint32_t display_id;
	uint32_t events_mask;
	uint32_t enable;
};

/**
 * struct panic_info - display panic information
 * @display_id: Display ID
 * @events_mask: Bitwise 'OR' of panic event flags
 */
struct panic_info {
	uint32_t display_id;
	uint32_t events_mask;
};


/**
 * struct regdump_info - register dump information at a requested offset & length
 * @device_id: Device ID
 * @reg_offset: Offset from MDSS base
 * @length: Required length to be dumped
 * @buffer_info: Allocated memory for dumping
 */
struct regdump_info {
	uint32_t device_id;
	u32 reg_offset;
	u32 length;
	struct hfi_buff buffer_info;
};

/*
 * enum hfi_debug_misr_module - Module to obtain misr
 * @HFI_DEBUG_MISR_DSI      :   Dsi module
 * @HFI_DEBUG_MISR_MIXER    :   Mixer module
 * @HFI_DEBUG_MISR_INTF     :   Interface module
 */
enum hfi_debug_misr_module_type {
	HFI_DEBUG_MISR_DSI                     = 0x0,
	HFI_DEBUG_MISR_MIXER                   = 0x1,
	HFI_DEBUG_MISR_INTF                    = 0x2,
};

/*
 * struct misr_setup_data - MISR setup information
 * @display_id      : display_id of required display
 * @enable          : Enable bit for MISR setup register
 * @frame_count     : Number of frames for MISR setup register
 * @module_type     : module to obtain MISR value from
 */
struct misr_setup_data {
	u32 display_id;
	u32 enable;
	u32 frame_count;
	enum hfi_debug_misr_module_type module_type;
};

/*
 * struct misr_read_data - MISR read information
 * @display_id      : display_id of required display
 * @module_type     : module to obtain MISR value from
 */
struct misr_read_data {
	u32 display_id;
	enum hfi_debug_misr_module_type module_type;
};

#endif // __H_HFI_DEFS_DEBUG_H__
