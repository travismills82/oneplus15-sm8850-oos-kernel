/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef __LCP_PPDDR_INTERNAL_H
#define __LCP_PPDDR_INTERNAL_H

#include <linux/qti-lcp-ppddr.h>

/**
 * @ingroup tz_os_cfg_phys_ddr_protections_for_regions_id
 *
 * Configure protections for a data region.
 *
 * @smc_id
 *	0x02002702
 *
 * @param_id
 *	0x00000886
 *
 * @return
 *   Zero on success, negative value on failure or if feature is not supported.
 */
#define TZ_CFG_PHYS_DDR_PROTECTIONS_FOR_REGIONS_ID \
	TZ_SYSCALL_CREATE_SMC_ID(TZ_OWNER_SIP, TZ_SVC_DDR, 0x02U)

#define TZ_CFG_PHYS_DDR_PROTECTIONS_FOR_REGIONS_PARAM_ID \
	TZ_SYSCALL_CREATE_PARAM_ID_6( \
			TZ_SYSCALL_PARAM_TYPE_VAL,\
			TZ_SYSCALL_PARAM_TYPE_BUF_RW,\
			TZ_SYSCALL_PARAM_TYPE_VAL,\
			TZ_SYSCALL_PARAM_TYPE_BUF_RW,\
			TZ_SYSCALL_PARAM_TYPE_VAL, \
			TZ_SYSCALL_PARAM_TYPE_VAL)

/*
 * Argument type to configure/query one Physical DDR protected region.
 * Each data region may have multiple tag regions associated with it.
 */
struct phys_protected_ddr_region {
	/* [in] Data region to configure protections for  */
	struct pddr_region ppddr_data_region;

	/*
	 * Subrange of the DDR data region to initialize. If caller wants to
	 * initialize entire region, this should be equal to ppddr_data_region
	 */
	struct pddr_region ppddr_init_data_region;

	/*
	 * [in] (Optional) Pointer to an array of tag regions to be configured
	 * for the given data region. If NULL, TZ will decide where tag regions
	 * are configured.
	 * NOTE: A single region can, in the future have multiple tag types
	 *	 eg part of region can be DAE an another can be MTE.
	 *
	 *	 HLOS can ignore this field for now and let TZ choose.
	 */
	struct pddr_tag_region *ppddr_tag_regions_ptr;

	/*
	 * [in] (Optional) Size of tags_region array in bytes.
	 * If ppddr_tag_regions_ptr is NULL, this field is ignored
	 */
	uint32_t ppddr_tag_regions_size;

	/*
	 * [out, Optional]. Pointer to an array of tag regions that have been
	 * configured for the given data region. If NULL, TZ will not return
	 * the tag regions configured for the region.
	 *
	 * NOTE: The SMC call will fail if any of the tag regions failed to
	 *	 be configured.
	 *
	 *	 This field and next are useful if HLOS wants to know where
	 *	 the tag region is. Ignore this field for now.
	 *
	 */
	uint32_t *ppddr_ret_tag_regions_ptr;

	/*
	 * [in, Optional]. Size of ppddr_ret_tag_regions_ptr array in bytes.
	 * If that field is NULL, this is ignored.
	 */
	uint32_t ppddr_ret_tag_regions_size;

	/*
	 * [out, Optional]. Pointer to the number of tag regions that have
	 * been configured for the given data region. If NULL, it is ignored.
	 *
	 * NOTE: Ignore for now.
	 */
	uint32_t  *ppddr_ret_tag_regions_len_ptr;

	/*
	 * [in, Optional]. Size of ppddr_ret_tag_regions_len in bytes.
	 * If ppddr_ret_tag_regions_len_ptr is NULL, this field is ignored.
	 *
	 * NOTE: This is forfuture-proofing the API. Ignore the field for now.
	 */
	uint32_t ppddr_ret_tag_regions_len_ptr_size;
};

enum cfg_phys_ddr_protection_rsp {
	CFG_PHYS_DDR_PROTECTION_RSP_CMD_COMPLETE = 0,

	/* Command failed due to an unknown error */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_CMD_FAILED = 1,

	/* HW is processing the command. Caller should poll for results */
	CFG_PHYS_DDR_PROTECTION_RSP_CMD_PROCESSING = 2,

	/* Caller should try again */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_HW_IS_BUSY = 3,

	/* The underlying HW does not cover this region */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_REGION_NOT_PROTECTABLE = 4,

	/* Another VM already owns this region, so reject this command */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_REGION_ALREADY_IN_USE = 5,

	/* The command is not supported for this region */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_CMD_NOT_SUPPORTED = 6,

	/* This region is not supposed to be reconfigured */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_NO_CFG_ALLOWED = 7,

	/* HW does not support partial enable for this type of region. */
	CFG_PHYS_DDR_PROTECTION_RSP_ERR_PARTIAL_ENABLE_NOT_SUPPORTED = 8,

	CFG_PHYS_DDR_PROTECTION_RSP_ERR_MAX
};
#endif /* __LCP_PPDDR_INTERNAL_H */
