/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __MPAM_DSP_H__
#define __MPAM_DSP_H__

#define APSS_NAME                             "apss"
#define GLINK_TRANSPORT_NAME                  "SMEM"
/* Glink channel name */
#define MPAM_DSP_GLINK_GUID                   "mpam-glink-apps-dsp"

/* Driver version */
#define MPAM_DSP_GLINK_VERSION                0x1u

/* Feature IDs for mpam2dsp messages */
#define MPAM_DSP_FEATURE_SET_BW_LIMIT         1u
/**< Feature Id to set Bandwidth Limit Params*/
#define MPAM_DSP_FEATURE_GET_BW_LIMIT         2u
/**< Feature Id to get the Bandwidth Limit Params*/

/* Feature IDs for dsp2mpam messages */
#define MPAM_DSP_FEATURE_STATUS               3u
/**< Feature Id for DSP status */

/* Return values in status field */
#define MPAM_DSP_CMD_SUCCESS                    0u
/**< Error code on Success */
#define MPAM_DSP_CMD_FAILURE                    1u
/**< Error code on Failure */
#define MPAM_DSP_CMD_INVALID_PARAMS             2u
/**< Error code on Invalid Parameters/Config */
#define MPAM_DSP_CMD_CONFIG_EXIST               3u
/**< Error code to indicate configuration already applied */
#define MPAM_DSP_CMD_INVALID_FEATURE_ID         4u
/**< Error code to indicate invalid fearure request received t */
#define MPAM_DSP_CMD_INTERNAL_QUEUE_ERROR       5u
/**< Error code to indicate internal queue error */

struct dsp_status_t {

	unsigned int ratio;
	/**< Applied bandwidth limit ratio in percentage */

	unsigned int bwlimit;
	/**< Applied bandwidth limit in MBps */

	unsigned int bwmax;
	/**< Maximum bandwidth supported in MBps */

	unsigned int status;
	/**< Return value for the command */
};

struct dsp2mpam_msg_t {

	unsigned int feature_id;
	/**< feature identifier */

	unsigned int size;
	/**< Size of the message in bytes */

	unsigned int version;
	/**< Version of DSP implementation */

	unsigned int reserved1;
	/**< Reserved parameter for future use */

	unsigned int reserved2;
	/**< Reserved parameter for future use */

	union {
		struct dsp_status_t dsp_state;
		/**< Param structure for MPAM_DSP_FEATURE_STATUS feature Id */
	} fs;

};

struct set_bw_limit_params_t {

	unsigned int set_ratio;
	/**< Master flag to set the ratio, 1 - when bandwidth limit needs to be set */

	unsigned int ratio;
	/**< Bandwidth limit ratio to be set, valid only when set_ratio is set */
};

struct mpam2dsp_msg_t {

	unsigned int feature_id;
	/**< feature identifier */

	unsigned int size;
	/**< Size of the message in bytes */

	unsigned int version;
	/**< Version of APPS side implementation */

	unsigned int reserved1;
	/**< Reserved parameter for future use */

	unsigned int reserved2;
	/**< Reserved parameter for future use */

	union {
		struct set_bw_limit_params_t limit_params;
		/**< Param structure for MPAM_DSP_FEATURE_SET_BW_LIMIT */
	} fs;
};

#endif // __MPAM_DSP_H__
