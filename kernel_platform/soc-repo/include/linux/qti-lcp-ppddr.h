/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QTI_LCP_PPDDR_H_
#define __QTI_LCP_PPDDR_H_
/*
 * Interfaces to configure LCP Protected Physical DDR, aka LCP-DARE, Regions
 */

/* Physical DDR region info */
struct pddr_region {
	uint64_t start_addr;
	uint64_t end_addr;
};

/* Physical DDR region tag info
 *	Type specifies DE, DAE, DARE, etc.
 *	Size of the tag region varies by the type.
 */
struct pddr_tag_region {
	uint64_t start_addr;
	uint64_t end_addr;
	uint8_t  type;
};

enum cfg_phys_ddr_protection_cmd {
	/* Use this to poll for results for each region */
	CFG_PHYS_DDR_PROTECTION_CMD_GET_CMD_RESULT_FOR_REGIONS = 0,
	CFG_PHYS_DDR_PROTECTION_CMD_DISABLE_REGIONS = 1,
	CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_DE = 2,
	CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DAE = 3,
	CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_AND_INIT_DARE = 4,
	CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_MTE = 5,
	CFG_PHYS_DDR_PROTECTION_CMD_ENABLE_DE_AND_MTE = 6,
	CFG_PHYS_DDR_PROTECTION_CMD_MAX
};

/*
 * Describe one Physical Protected DDR Region.
 * Each region may have multiple tag regions associated with it and
 * each tag region maybe of a different type (DARE+MTE).
 *
 * NOTE: Linux ignores tag regions for now and lets TZ manage them.
 */
struct ppddr_region {
	struct pddr_region data_region;

	enum cfg_phys_ddr_protection_cmd lcp_mem_type;

	/*
	 * A region maybe marked as say DAE in the DT but may not be enabled
	 * at a given time or may have been disabled by the hypervisor. Use
	 * ->valid to represent that. When configuring a region, using
	 * cfg_pddr_protected_region(), Linux assumes region is valid and
	 * ignores the ->valid field.
	 */
	uint8_t valid;

	/*
	 * Use tz_os_request_hyp_owned_lcp_regions_info() to determine
	 * number and size of tag_regions
	 */
	struct pddr_tag_region *tag_regions;
	uint32_t num_tag_regions;
};

/*
 * Configure physical protections for the DDR region given by @region.
 *
 * Return 0 on success or an error code on failure.
 *
 * NOTE: Linux ignores tag regions for now and lets TZ manage them.
 */
extern int qcom_scm_cfg_pddr_protected_region(struct ppddr_region *region);

#endif /* __QTI_LCP_PPDDR_H_ */
