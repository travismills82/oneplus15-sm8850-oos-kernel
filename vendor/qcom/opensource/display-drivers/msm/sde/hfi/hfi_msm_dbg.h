/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _HFI_MSM_DBG_H_
#define _HFI_MSM_DBG_H_

#include "sde_dbg.h"
#include "sde_trace.h"
#include "hfi_kms.h"
#include "hfi_adapter.h"

#if IS_ENABLED(CONFIG_MDSS_HFI)
/**
 * MDSS_DBG_DUMP - trigger dumping of all msm_dbg facilities
 * @dump_blk_mask: mask of all the hw blk-ids that has to be dumped
 * @va_args:	list of named register dump ranges and regions to dump, as
 *		registered previously through msm_dbg_reg_register_base and
 *		msm_dbg_reg_register_dump_range.
 *		Including the special name "panic" will trigger a panic after
 *		the dumping work has completed.
 */
#define MDSS_DBG_DUMP(dump_blk_mask, ...) hfi_dbg_dump(SDE_DBG_DUMP_IN_LOG, __func__, \
		dump_blk_mask, ##__VA_ARGS__, SDE_EVTLOG_DATA_LIMITER)

/**
 * MDSS_DBG_EVT_CTRL - trigger a different driver events
 * event: event that trigger different behavior in the driver
 */
#define MDSS_DBG_CTRL(...) hfi_dbg_ctrl(__func__, ##__VA_ARGS__, \
		SDE_DBG_DUMP_DATA_LIMITER)

/**
 * struct msm_dbg_buf_data -  msm debug buffer data base structure
 *  @reg_addr: pointer to MDSS registers dump address
 *  @evt_log_addr: pointer to event log  dump address
 *  @dbg_bus_addr: pointer to debug bus dump address
 *  @device_state_addr: pointer to device state variable dump address
 */
struct msm_dbg_buf_data {
	struct hfi_shared_addr_map reg_addr;
	struct hfi_shared_addr_map evt_log_addr;
	struct hfi_shared_addr_map dbg_bus_addr;
	struct hfi_shared_addr_map device_state_addr;
};

/**
 * struct msm_dbg_reg_base - register region base.
 * @off: cached offset of region for manual register dumping
 * @cnt: cached range of region for manual register dumping
 * @reg_addr: pointer to MDSS registers dump address
 */
struct msm_dbg_reg_base {
	size_t off;
	size_t cnt;
	struct hfi_shared_addr_map *reg_addr;
};

/**
 * struct hfi_msm_dbg - hfi implementation extension of sde_encoder object
 * @dev: device pointer
 * @msm_dbg_printer: drm printer handle used to print msm_dbg info in devcoredump device
 * @buff_map: stores buffer pointers mapped for reg, event log, debug bus and device
 *            state dumps
 * @base_props: prop helper object for intermediate property collection
 * @mutex: mutex to serialize access to serialze dumps, debugfs access
 * @panic_on_err: whether to kernel panic after triggering dump via debugfs
 * @dump_option: whether to dump registers and dbgbus into memory, kernel log, or both
 * @debugfs_ctrl: enabled/disabled panic or ftrace through debugfs
 * @evtlog_enable: whether eventlogs are enabled
 * @hfi_cb_obj: hfi listener call back object
 * @hfi_cb_obj: register read call back object
 * @reg_base: stores register read user configuration\
 * @dump_len: to track the length of regs in flight of dump
 */
struct hfi_msm_dbg {
	struct device *dev;
	struct dentry *debugfs_root;
	struct sde_dbg_evtlog *evtlog;
	char *read_buf;

	struct drm_printer *msm_dbg_printer;
	struct msm_dbg_buf_data buff_map;
	struct hfi_util_u32_prop_helper *base_props;
	struct mutex mutex;
	u32 panic_on_err;
	u32 dump_option;
	u32 debugfs_ctrl;
	struct hfi_prop_listener hfi_cb_obj;
	struct hfi_prop_listener reg_read_cb_obj;
	struct msm_dbg_reg_base *reg_base;
	u32 dump_len;
	char *dump_buf;
	u32 recovery_off;
	char *dump_recovery;
};

/**
 * hfi_dbg_dump - trigger dumping of all hfi_dbg facilities
 * @name:	string indicating origin of dump
 * @dump_blk_mask: mask of all the hw blk-ids that has to be dumped
 * @va_args:	list of named register dump ranges and regions to dump, as
 *		registered previously through sde_dbg_reg_register_base and
 *		sde_dbg_reg_register_dump_range.
 *		Including the special name "panic" will trigger a panic after
 *		the dumping work has completed.
 * Returns:	none
 */
void hfi_dbg_dump(enum sde_dbg_dump_flag mode, const char *name, u64 dump_blk_mask, ...);

/**
 * hfi_dbg_ctrl - trigger specific actions for the driver with debugging
 *                purposes. Those actions need to be enabled by the debugfs entry
 *                so the driver executes those actions in the corresponding calls.
 * @va_args:	list of actions to trigger
 * Returns:	none
 */
void hfi_dbg_ctrl(const char *name, ...);

/**
 * hfi_msm_dbg_init - initialize hfi debug object
 * @dev:     Pointer to drm device structure
 * Returns:  Pointers to shared memory structure
 */
int hfi_msm_dbg_init(struct device *dev, struct dentry *debugfs_root);

/**
 * hfi_msm_dbg_destroy - destroy hfi debug object
 */
void hfi_msm_dbg_destroy(void);
#else

static inline int hfi_msm_dbg_init(struct device *dev, struct dentry *debugfs_root)
{
	return 0;
}

static inline void hfi_msm_dbg_destroy(void)
{

}

#endif // IS_ENABLED(CONFIG_MDSS_HFI)

#endif  // _HFI_MSM_DBG_H_
