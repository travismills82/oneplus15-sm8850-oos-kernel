/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_COLOR_PROC_H_
#define _HFI_COLOR_PROC_H_

#include "sde_hw_sspp.h"

/**
 * hfi_sspp_setup_csc - setup color space conversion in HFI path
 * @ctx: Pointer to pipe context
 * @data: Pointer to config structure
 */
void hfi_sspp_setup_csc(struct sde_hw_pipe *ctx, struct sde_csc_cfg *data);

/**
 * hfi_setup_ucsc_igcv1 - set UCSC IGC cp block in HFI path
 * @ctx: Pointer to pipe object
 * @index: Pipe rectangle to operate on
 * @mode: Pointer to sde_hw_cp_cfg object containing IGC mode data
 */
void hfi_setup_ucsc_igcv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data);

/**
 * hfi_setup_ucsc_gcv1 - api to set UCSC GC cp block in HFI path
 * @ctx: pointer to pipe object
 * @index: pipe rectangle to operate on
 * @data: pointer to sde_hw_cp_cfg object containing gc mode data
 */
void hfi_setup_ucsc_gcv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data);

/**
 * hfi_setup_ucsc_cscv1 - api to set UCSC CSC cp block in HFI path
 * @ctx: pointer to pipe object
 * @index: pipe rectangle to operate on
 * @data: pointer to sde_hw_cp_cfg object containing drm_msm_ucsc_csc data
 */
void hfi_setup_ucsc_cscv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data);

/**
 * hfi_setup_ucsc_unmultv1 - api to set UCSC UNMULT cp block in HFI path
 * @ctx: pointer to pipe object
 * @index: pipe rectangle to operate on
 * @data: pointer to sde_hw_cp_cfg object containing bool data
 */
void hfi_setup_ucsc_unmultv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data);

/**
 * hfi_setup_ucsc_alpha_ditherv1 - set UCSC ALPHA DITHER cp block in HFI path
 * @ctx: Pointer to pipe object
 * @index: Pipe rectangle to operate on
 * @data: Pointer to sde_hw_cp_cfg object containing bool data
 */
void hfi_setup_ucsc_alpha_ditherv1(struct sde_hw_pipe *ctx,
		enum sde_sspp_multirect_index index, void *data);

/**
 * hfi_setup_dspp_pa_dither_v1_7 - setup DSPP dither feature in HFI path
 * @ctx: Pointer to DSPP context
 * @cfg: Pointer to dither data
 */
void hfi_setup_dspp_pa_dither_v1_7(struct sde_hw_dspp *ctx, void *cfg);

/**
 * hfi_setup_dspp_spr_dither_v2 - setup DSPP SPR dither feature in HFI path
 * @ctx: Pointer to DSPP context
 * @cfg: Pointer to dither data
 */
void hfi_setup_dspp_spr_dither_v2(struct sde_hw_dspp *ctx, void *cfg);
#endif /* _HFI_COLOR_PROC_H_ */
