/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_SWI_H__
#define __HFI_SWI_H__

struct hfi_core_drv_data;

/**
 * init_swi() - Global SWI register initialization.
 *
 * This call programs the initial global swi registers required
 * for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int init_swi(struct hfi_core_drv_data *drv_data);

/**
 * deinit_swi() - Global SWI register deinitialization.
 *
 * This call clears the initial global swi registers programmed
 * for the device to communicate with DCP.
 *
 * Return: 0 on success or negative errno
 */
int deinit_swi(struct hfi_core_drv_data *drv_data);

/**
 * swi_reg_write() - Write to SWI register
 *
 * Write client resource table mmap address to client specific SWI reg.
 * This address is read by firmware to setup client resources at firmware
 * side.
 *
 * Return: 0 on success or negative errno
 */
int swi_setup_resources(u32 client_id, struct hfi_core_drv_data *drv_data);

/**
 * swi_reg_power_off() - Write SWI register power bit to off state
 *
 * Write SWI reg power bit to off state in order to end communication for
 * the specified client.
 *
 * Return: 0 on success or negative errno
 */
int swi_reg_power_off(u32 client_id, struct hfi_core_drv_data *drv_data);

#endif // __HFI_SWI_H__
