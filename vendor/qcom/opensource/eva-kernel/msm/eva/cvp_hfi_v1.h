/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.​
 */

#ifndef __H_CVP_HFI_V1_H__
#define __H_CVP_HFI_V1_H__

#include "cvp_comm_def.h"

#define HFI_CMD_CLIENT_DATA_RESERVE_2_OFFSET_IN_UWORD32       (0xC)

#define HFI_COMMON_BASE				(0)
#define HFI_DOMAIN_BASE_COMMON		(HFI_COMMON_BASE + 0)
#define HFI_DOMAIN_BASE_CVP			(HFI_COMMON_BASE + 0x04000000)

#define HFI_ARCH_COMMON_OFFSET		(0)

#define  HFI_CMD_START_OFFSET		(0x00010000)
#define  HFI_MSG_START_OFFSET		(0x00020000)

#define  HFI_ERR_NONE                                   (HFI_COMMON_BASE)        /**< Status: No error */
#define  HFI_ERR_SYS_FATAL                              (HFI_COMMON_BASE + 0x1)  /**< Fatal system error */
#define  HFI_ERR_SYS_INVALID_PARAMETER                  (HFI_COMMON_BASE + 0x2)  /**< Invalid system parameter encountered */
#define  HFI_ERR_SYS_VERSION_MISMATCH                   (HFI_COMMON_BASE + 0x3)  /**< Interface version mismatch */
#define  HFI_ERR_SYS_INSUFFICIENT_RESOURCES             (HFI_COMMON_BASE + 0x4)  /**< Insufficient system resources */
#define  HFI_ERR_SYS_MAX_SESSIONS_REACHED               (HFI_COMMON_BASE + 0x5)  /**< Maximum number of sessions reached */
#define  HFI_ERR_SYS_SESSION_IN_USE                     (HFI_COMMON_BASE + 0x7)  /**< Session ID specified is in use */
#define  HFI_ERR_SYS_SESSION_ID_OUT_OF_RANGE            (HFI_COMMON_BASE + 0x8)  /**< ID is out of range */
#define  HFI_ERR_SYS_UNSUPPORTED_TRIGCMD                (HFI_COMMON_BASE + 0xA)  /**< Unsupported TRIGCMD command*/
#define  HFI_ERR_SYS_UNSUPPORTED_RESOURCES              (HFI_COMMON_BASE + 0xB)  /**< Unsupported resource*/
#define  HFI_ERR_SYS_UNSUPPORT_CMD                      (HFI_COMMON_BASE + 0xC)  /**< Command is not supported*/
#define  HFI_ERR_SYS_CMDSIZE                            (HFI_COMMON_BASE + 0xD)  /**< command size err*/
#define  HFI_ERR_SYS_UNSUPPORT_PROPERTY                 (HFI_COMMON_BASE + 0xE)  /**< Unsupported property*/
#define  HFI_ERR_SYS_INIT_EXPECTED                      (HFI_COMMON_BASE + 0xF)  /**< Upon FW start, first command must be SYS_INIT*/
#define  HFI_ERR_SYS_INIT_IGNORED                       (HFI_COMMON_BASE + 0x10) /**< After FW started, SYS_INIT will be ignored*/
#define  HFI_ERR_SYS_MAX_DME_SESSIONS_REACHED           (HFI_COMMON_BASE + 0x11) /**< Maximum DME sessions Reached */
#define  HFI_ERR_SYS_MAX_FD_SESSIONS_REACHED            (HFI_COMMON_BASE + 0x12) /**< Maximum FD sessions Reached */
#define  HFI_ERR_SYS_MAX_ODT_SESSIONS_REACHED           (HFI_COMMON_BASE + 0x13) /**< Maximum ODT sessions Reached*/
#define  HFI_ERR_SYS_MAX_CV_SESSIONS_REACHED            (HFI_COMMON_BASE + 0x14) /**< Maximum CV sessions Reached*/
#define  HFI_ERR_SYS_INVALID_SESSION_TYPE               (HFI_COMMON_BASE + 0x15) /**< Invalid session TYPE. */
#define  HFI_ERR_SYS_NOC_ERROR							(HFI_COMMON_BASE + 0x16) /**< NOC Error encountered */

									/**
									Level 2 Comment: "Session Level Error types"
									Common HFI_ERROR_SESSION_X values to be used as session level error/warning
									for event and messages
									*/
#define  HFI_ERR_SESSION_FATAL                          (HFI_COMMON_BASE + 0x1001)  /**< Fatal session error */
#define  HFI_ERR_SESSION_INVALID_PARAMETER              (HFI_COMMON_BASE + 0x1002)  /**< Invalid session parameter */
#define  HFI_ERR_SESSION_BAD_POINTER                    (HFI_COMMON_BASE + 0x1003)  /**< Bad pointer encountered */
#define  HFI_ERR_SESSION_INVALID_SESSION_ID             (HFI_COMMON_BASE + 0x1004)  /**< Invalid session ID. eventData2 specifies the session ID. */
#define  HFI_ERR_SESSION_INVALID_STREAM_ID              (HFI_COMMON_BASE + 0x1005)  /**< Invalid stream ID. eventData2 specifies the stream ID. */
#define  HFI_ERR_SESSION_INCORRECT_STATE_OPERATION      (HFI_COMMON_BASE + 0x1006)  /**< Incorrect state for specified operation */
#define  HFI_ERR_SESSION_UNSUPPORTED_PROPERTY           (HFI_COMMON_BASE + 0x1007)  /**< Unsupported property. eventData2 specifies the property index. */
#define  HFI_ERR_SESSION_UNSUPPORTED_SETTING            (HFI_COMMON_BASE + 0x1008)  /**< Unsupported property setting. eventData2 specifies the property index. */
#define  HFI_ERR_SESSION_INSUFFICIENT_RESOURCES         (HFI_COMMON_BASE + 0x1009)  /**< Insufficient resources for session */
#define  HFI_ERR_SESSION_STREAM_CORRUPT_OUTPUT_STALLED  (HFI_COMMON_BASE + 0x100A)  /**< Stream is found to be corrupt; processing is stalled */
#define  HFI_ERR_SESSION_STREAM_CORRUPT                 (HFI_COMMON_BASE + 0x100B)  /**< Stream is found to be corrupt; processing is recoverable */
#define  HFI_ERR_SESSION_RESERVED                       (HFI_COMMON_BASE + 0x100C)  /**< Reserved  */
#define  HFI_ERR_SESSION_UNSUPPORTED_STREAM             (HFI_COMMON_BASE + 0x100D)  /**< Unsupported stream */
#define  HFI_ERR_SESSION_CMDSIZE                        (HFI_COMMON_BASE + 0x100E)  /**< Command packet size err*/
#define  HFI_ERR_SESSION_UNSUPPORT_CMD                  (HFI_COMMON_BASE + 0x100F)  /**< Command is not supported*/
#define  HFI_ERR_SESSION_UNSUPPORT_BUFFERTYPE           (HFI_COMMON_BASE + 0x1010)  /**< BufferType is not supported*/
#define  HFI_ERR_SESSION_BUFFERCOUNT_TOOSMALL           (HFI_COMMON_BASE + 0x1011)  /**< Buffer Count is less than default*/
#define  HFI_ERR_SESSION_INVALID_SCALE_FACTOR           (HFI_COMMON_BASE + 0x1012)  /**< Downscaling not possible */
#define  HFI_ERR_SESSION_UPSCALE_NOT_SUPPORTED          (HFI_COMMON_BASE + 0x1013)  /**< Upscaling not possible */
#define  HFI_ERR_SESSION_CANNOT_KEEP_ASPECT_RATIO       (HFI_COMMON_BASE + 0x1014)  /**< Cannot maintain aspect ratio */
#define  HFI_ERR_SESSION_ADDRESS_NOT_ALIGNED            (HFI_COMMON_BASE + 0x1016)   /**Address is not aligned */
#define  HFI_ERR_SESSION_BUFFERSIZE_TOOSMALL            (HFI_COMMON_BASE + 0x1017)  /**< Buffer Count is less than default*/
#define  HFI_ERR_SESSION_ABORTED                        (HFI_COMMON_BASE + 0x1018)  /**< error caused by session abort*/
#define  HFI_ERR_SESSION_BUFFER_ALREADY_SET             (HFI_COMMON_BASE + 0x1019)  /**< Cannot set buffer multiple times without releasing in between. */
#define  HFI_ERR_SESSION_BUFFER_ALREADY_RELEASED        (HFI_COMMON_BASE + 0x101A)  /**< Cannot release buffer multiple times without setting in between. */
#define  HFI_ERR_SESSION_END_BUFFER_NOT_RELEASED        (HFI_COMMON_BASE + 0x101B)  /**< Session was ended without properly releasing all buffers */
#define  HFI_ERR_SESSION_FLUSHED                        (HFI_COMMON_BASE + 0x101C)  /**< Cannot set buffer multiple times without releasing in between. */
#define  HFI_ERR_SESSION_KERNEL_MAX_STREAMS_REACHED     (HFI_COMMON_BASE + 0x101D) /*Maximum Streams per Kernel reached in a session*/
#define  HFI_ERR_SESSION_MAX_STREAMS_REACHED            (HFI_COMMON_BASE + 0x101E) /*Maximum Streams Reached in a session*/
#define  HFI_ERR_SESSION_HW_HANG_DETECTED               (HFI_COMMON_BASE + 0x101F) /*HW hang was detected in one of the HW blocks for a frame*/

#define HFI_CV_KERNEL_FPX         0x00000001 /**< Harris Corner Detector            */
#define HFI_CV_KERNEL_SCALER      0x00000002 /**< DownScale                         */
#define HFI_CV_KERNEL_NCC         0x00000004 /**< NCC                               */
#define HFI_CV_KERNEL_DFS         0x00000008 /**< Depth From Stereo                 */
#define HFI_CV_KERNEL_WARP_NCC    0x00000010 /**< Warp NCC                          */
#define HFI_CV_KERNEL_OF          0x00000020 /**< Optical Flow/TME                  */
#define HFI_CV_KERNEL_ORB         0x00000040 /**< ORB compute & Matching            */
#define HFI_CV_KERNEL_PYS_HCD     0x00000080 /**< Pyramid DS & HCD                  */
#define HFI_CV_KERNEL_BLOB        0x00000100 /**< Blob detector                     */
#define HFI_CV_KERNEL_DESCRIPTOR  0x00000200 /**< Patch Descriptor                  */
#define HFI_CV_KERNEL_MATCH       0x00000400 /**< Matcher                           */
#define HFI_CV_KERNEL_PPU         0x00000800 /**< PPU                               */
#define HFI_CV_KERNEL_LME         0x00001000 /**< Local motion Estimation ppu/mpu   */
#define HFI_CV_KERNEL_ICA         0x00002000 /**< Image Correction & Adjustment     */
#define HFI_CV_KERNEL_GCX         0x00004000 /**< Geometric Correction for XR    */
#define HFI_CV_KERNEL_XRA         0x00008000 /**< XRA                               */
#define HFI_CV_KERNEL_CSC         0x00010000 /**< CSC for XR    */
#define HFI_CV_KERNEL_LSR         0x00020000 /**< LSR for XR    */
#define HFI_CV_KERNEL_ITOF        0x00040000 /**< Time of Flight Sensor    */
#define HFI_CV_KERNEL_RGE         0x00080000 /**< RGE, Reprojection and Grid-inversion Engine */
#define HFI_CV_KERNEL_SPSTAT      0x00100000 /**< FTexture (Spatial Statistics)     */
#define HFI_CV_KERNEL_GME         0x00200000 /**< Global motion Estimation fdu   */
#define HFI_CV_KERNEL_STL         0x00400000 /**< stl   */
#define HFI_CV_KERNEL_WARP        0x00800000 /**< warp   */
#define HFI_CV_KERNEL_DCM_NONPOR  0x01000000
#define HFI_CV_KERNEL_TME_NONPOR  0x02000000
#define HFI_CV_KERNEL_PRE_NONPOR  0x04000000
#define HFI_CV_KERNEL_POST_NONPOR 0x08000000

#define HFI_EVENT_SYS_ERROR				(HFI_COMMON_BASE + 0x1)
#define HFI_EVENT_SESSION_ERROR			(HFI_COMMON_BASE + 0x2)

#define  HFI_TME_PROFILE_DEFAULT	0x00000001
#define  HFI_TME_PROFILE_FRC		0x00000002
#define  HFI_TME_PROFILE_ASW		0x00000004
#define  HFI_TME_PROFILE_DFS_BOKEH	0x00000008

#define HFI_TME_LEVEL_INTEGER		0x00000001

#define HFI_BUFFER_INPUT				(HFI_COMMON_BASE + 0x1)
#define HFI_BUFFER_OUTPUT				(HFI_COMMON_BASE + 0x2)
#define HFI_BUFFER_OUTPUT2				(HFI_COMMON_BASE + 0x3)
#define HFI_BUFFER_INTERNAL_PERSIST		(HFI_COMMON_BASE + 0x4)
#define HFI_BUFFER_INTERNAL_PERSIST_1		(HFI_COMMON_BASE + 0x5)
#define HFI_BUFFER_COMMON_INTERNAL_SCRATCH	(HFI_COMMON_BASE + 0x6)
#define HFI_BUFFER_COMMON_INTERNAL_SCRATCH_1	(HFI_COMMON_BASE + 0x7)
#define HFI_BUFFER_COMMON_INTERNAL_SCRATCH_2	(HFI_COMMON_BASE + 0x8)
#define HFI_BUFFER_COMMON_INTERNAL_RECON	(HFI_COMMON_BASE + 0x9)
#define HFI_BUFFER_EXTRADATA_OUTPUT		(HFI_COMMON_BASE + 0xA)
#define HFI_BUFFER_EXTRADATA_OUTPUT2		(HFI_COMMON_BASE + 0xB)
#define HFI_BUFFER_EXTRADATA_INPUT		(HFI_COMMON_BASE + 0xC)


#define HFI_PROPERTY_SYS_COMMON_START		\
	(HFI_DOMAIN_BASE_COMMON + HFI_ARCH_COMMON_OFFSET + 0x0000)
#define HFI_PROPERTY_SYS_DEBUG_CONFIG		\
	(HFI_PROPERTY_SYS_COMMON_START + 0x001)
#define HFI_PROPERTY_SYS_RESOURCE_OCMEM_REQUIREMENT_INFO	\
	(HFI_PROPERTY_SYS_COMMON_START + 0x002)
#define HFI_PROPERTY_SYS_CONFIG_VCODEC_CLKFREQ				\
	(HFI_PROPERTY_SYS_COMMON_START + 0x003)
#define HFI_PROPERTY_SYS_IDLE_INDICATOR         \
	(HFI_PROPERTY_SYS_COMMON_START + 0x004)
#define  HFI_PROPERTY_SYS_CODEC_POWER_PLANE_CTRL     \
	(HFI_PROPERTY_SYS_COMMON_START + 0x005)
#define  HFI_PROPERTY_SYS_IMAGE_VERSION    \
	(HFI_PROPERTY_SYS_COMMON_START + 0x006)
#define  HFI_PROPERTY_SYS_CONFIG_COVERAGE    \
	(HFI_PROPERTY_SYS_COMMON_START + 0x007)
#define  HFI_PROPERTY_SYS_UBWC_CONFIG    \
	(HFI_PROPERTY_SYS_COMMON_START + 0x008)

#define HFI_DEBUG_MSG_LOW					0x00000001
#define HFI_DEBUG_MSG_MEDIUM					0x00000002
#define HFI_DEBUG_MSG_HIGH					0x00000004
#define HFI_DEBUG_MSG_ERROR					0x00000008
#define HFI_DEBUG_MSG_FATAL					0x00000010
#define HFI_DEBUG_MSG_PERF					0x00000020
#define HFI_DEBUG_MSG_TIME					0x00000080
#define HFI_DEBUG_MSG_CRC_EN					0x00000100

#define HFI_DEBUG_MODE_QUEUE					0x00000001
#define HFI_DEBUG_MODE_QDSS					0x00000002

#define HFI_CMD_SYS_COMMON_START			\
(HFI_DOMAIN_BASE_COMMON + HFI_ARCH_COMMON_OFFSET + HFI_CMD_START_OFFSET \
	+ 0x0000)
#define HFI_CMD_SYS_INIT		(HFI_CMD_SYS_COMMON_START + 0x001)
#define HFI_CMD_SYS_PC_PREP		(HFI_CMD_SYS_COMMON_START + 0x002)
#define HFI_CMD_SYS_SET_RESOURCE	(HFI_CMD_SYS_COMMON_START + 0x003)
#define HFI_CMD_SYS_RELEASE_RESOURCE (HFI_CMD_SYS_COMMON_START + 0x004)
#define HFI_CMD_SYS_SET_PROPERTY	(HFI_CMD_SYS_COMMON_START + 0x005)
#define HFI_CMD_SYS_GET_PROPERTY	(HFI_CMD_SYS_COMMON_START + 0x006)
#define HFI_CMD_SYS_SESSION_INIT	(HFI_CMD_SYS_COMMON_START + 0x007)
#define HFI_CMD_SYS_SESSION_END		(HFI_CMD_SYS_COMMON_START + 0x008)
#define HFI_CMD_SYS_SET_BUFFERS		(HFI_CMD_SYS_COMMON_START + 0x009)
#define HFI_CMD_SYS_SESSION_ABORT	(HFI_CMD_SYS_COMMON_START + 0x00A)
#define HFI_CMD_SYS_TEST_START		(HFI_CMD_SYS_COMMON_START + 0x100)

#define HFI_MSG_SYS_COMMON_START			\
	(HFI_DOMAIN_BASE_COMMON + HFI_ARCH_COMMON_OFFSET +	\
	HFI_MSG_START_OFFSET + 0x0000)
#define HFI_MSG_SYS_INIT_DONE			(HFI_MSG_SYS_COMMON_START + 0x1)
#define HFI_MSG_SYS_PC_PREP_DONE		(HFI_MSG_SYS_COMMON_START + 0x2)
#define HFI_MSG_SYS_RELEASE_RESOURCE	(HFI_MSG_SYS_COMMON_START + 0x3)
#define HFI_MSG_SYS_DEBUG			(HFI_MSG_SYS_COMMON_START + 0x4)
#define HFI_MSG_SYS_SESSION_INIT_DONE	(HFI_MSG_SYS_COMMON_START + 0x6)
#define HFI_MSG_SYS_SESSION_END_DONE	(HFI_MSG_SYS_COMMON_START + 0x7)
#define HFI_MSG_SYS_IDLE		(HFI_MSG_SYS_COMMON_START + 0x8)
#define HFI_MSG_SYS_COV                 (HFI_MSG_SYS_COMMON_START + 0x9)
#define HFI_MSG_SYS_PROPERTY_INFO	(HFI_MSG_SYS_COMMON_START + 0xA)
#define HFI_MSG_SYS_SESSION_ABORT_DONE	(HFI_MSG_SYS_COMMON_START + 0xC)
#define HFI_MSG_SESSION_SYNC_DONE      (HFI_MSG_SESSION_OX_START + 0xD)

#define HFI_MSG_SESSION_COMMON_START		\
	(HFI_DOMAIN_BASE_COMMON + HFI_ARCH_COMMON_OFFSET +	\
	HFI_MSG_START_OFFSET + 0x1000)
#define HFI_MSG_EVENT_NOTIFY	(HFI_MSG_SESSION_COMMON_START + 0x1)
#define HFI_MSG_SESSION_GET_SEQUENCE_HEADER_DONE	\
	(HFI_MSG_SESSION_COMMON_START + 0x2)

#define HFI_CMD_SYS_TEST_SSR	(HFI_CMD_SYS_TEST_START + 0x1)
#define HFI_TEST_SSR_SW_ERR_FATAL	0x1
#define HFI_TEST_SSR_SW_DIV_BY_ZERO	0x2
#define HFI_TEST_SSR_HW_WDOG_IRQ	0x3
#define HFI_TEST_SSR_XTENSA_NOC	0x6

#define HFI_CMD_SESSION_CVP_START	\
	(HFI_DOMAIN_BASE_CVP + HFI_ARCH_COMMON_OFFSET +	\
	HFI_CMD_START_OFFSET + 0x1000)

#define  HFI_CMD_SESSION_CVP_SET_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x001)
#define  HFI_CMD_SESSION_CVP_RELEASE_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x002)

#define  HFI_CMD_SESSION_CVP_DS\
	(HFI_CMD_SESSION_CVP_START + 0x003)
#define  HFI_CMD_SESSION_CVP_HCD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x004)
#define  HFI_CMD_SESSION_CVP_HCD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x005)
#define  HFI_CMD_SESSION_CVP_CV_HOG_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x006)
#define  HFI_CMD_SESSION_CVP_CV_HOG_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x007)
#define  HFI_CMD_SESSION_CVP_SVM\
	(HFI_CMD_SESSION_CVP_START + 0x008)
#define  HFI_CMD_SESSION_CVP_NCC_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x009)
#define  HFI_CMD_SESSION_CVP_NCC_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x00A)
#define  HFI_CMD_SESSION_CVP_DFS_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x00B)
#define  HFI_CMD_SESSION_CVP_DFS_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x00C)
#define  HFI_CMD_SESSION_CVP_FTEXT\
	(HFI_CMD_SESSION_CVP_START + 0x00F)

/* ==========CHAINED OPERATIONS===================*/
#define  HFI_CMD_SESSION_CVP_CV_HOG_SVM_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x010)
#define  HFI_CMD_SESSION_CVP_CV_HOG_SVM_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x011)
#define  HFI_CMD_SESSION_CVP_CV_HOG_SVM_HCD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x012)
#define  HFI_CMD_SESSION_CVP_CV_HOG_SVM_HCD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x013)
#define  HFI_CMD_SESSION_CVP_OPTICAL_FLOW\
	(HFI_CMD_SESSION_CVP_START + 0x014)

/* ===========USECASE OPERATIONS===============*/
#define  HFI_CMD_SESSION_CVP_DC_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x030)
#define  HFI_CMD_SESSION_CVP_DC_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x031)
#define  HFI_CMD_SESSION_CVP_DCM_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x034)
#define  HFI_CMD_SESSION_CVP_DCM_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x035)

#define  HFI_CMD_SESSION_CVP_DME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x039)
#define  HFI_CMD_SESSION_CVP_GME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x03B)
#define  HFI_CMD_SESSION_CVP_GME_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x03A)

#define  HFI_CMD_SESSION_CVP_LME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x159)
#define  HFI_CMD_SESSION_CVP_LME_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x15A)

#define  HFI_CMD_SESSION_EVA_SPSTAT_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x00F)
#define  HFI_CMD_SESSION_EVA_SPSTAT_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x00E)

#define  HFI_CMD_SESSION_EVA_DME_ONLY_CONFIG\
    (HFI_CMD_SESSION_CVP_START + 0x040)
#define  HFI_CMD_SESSION_EVA_DME_ONLY_FRAME\
    (HFI_CMD_SESSION_CVP_START + 0x041)

#define  HFI_CMD_SESSION_CVP_CV_TME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x047)
#define  HFI_CMD_SESSION_CVP_CV_TME_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x048)
#define  HFI_CMD_SESSION_CVP_CV_OD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x049)
#define  HFI_CMD_SESSION_CVP_CV_OD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x04A)
#define  HFI_CMD_SESSION_CVP_CV_ODT_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x04B)
#define  HFI_CMD_SESSION_CVP_CV_ODT_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x04C)

#define  HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x04D)
#define HFI_CMD_SESSION_CVP_PYS_HCD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x050)
#define HFI_CMD_SESSION_CVP_PYS_HCD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x051)
#define HFI_CMD_SESSION_CVP_SET_MODEL_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x052)
#define HFI_CMD_SESSION_CVP_FD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x053)
#define HFI_CMD_SESSION_CVP_FD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x054)
#define HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x055)
#define  HFI_CMD_SESSION_CVP_RELEASE_MODEL_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x056)
#define  HFI_CMD_SESSION_CVP_SGM_DFS_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x057)
#define  HFI_CMD_SESSION_CVP_SGM_DFS_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x058)
#define  HFI_CMD_SESSION_CVP_SGM_OF_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x059)
#define  HFI_CMD_SESSION_CVP_SGM_OF_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x05A)
#define  HFI_CMD_SESSION_CVP_GCE_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x05B)
#define  HFI_CMD_SESSION_CVP_GCE_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x05C)
#define  HFI_CMD_SESSION_CVP_WARP_NCC_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x05D)
#define  HFI_CMD_SESSION_CVP_WARP_NCC_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x05E)
#define  HFI_CMD_SESSION_CVP_DMM_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x05F)
#define  HFI_CMD_SESSION_CVP_DMM_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x060)
#define HFI_CMD_SESSION_CVP_FLUSH\
	(HFI_CMD_SESSION_CVP_START + 0x061)
#define  HFI_CMD_SESSION_CVP_WARP_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x062)
#define  HFI_CMD_SESSION_CVP_WARP_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x063)
#define  HFI_CMD_SESSION_CVP_DMM_PARAMS\
	(HFI_CMD_SESSION_CVP_START + 0x064)
#define  HFI_CMD_SESSION_CVP_WARP_DS_PARAMS\
	(HFI_CMD_SESSION_CVP_START + 0x065)
#define  HFI_CMD_SESSION_EVA_WARP_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x066)
#define  HFI_CMD_SESSION_EVA_WARP_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x067)
#define  HFI_CMD_SESSION_CVP_XRA_BLOB_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x069)
#define  HFI_CMD_SESSION_CVP_XRA_BLOB_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x06A)
#define  HFI_CMD_SESSION_EVA_DESCRIPTOR_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x06B)
#define  HFI_CMD_SESSION_EVA_DESCRIPTOR_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x06C)
#define  HFI_CMD_SESSION_CVP_XRA_MATCH_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x06D)
#define  HFI_CMD_SESSION_CVP_XRA_MATCH_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x06E)


#define HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x070)
#define HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS\
	(HFI_CMD_SESSION_CVP_START + 0x071)
#define HFI_CMD_SESSION_CVP_SNAPSHOT_WRITE_DONE\
	(HFI_CMD_SESSION_CVP_START + 0x072)
#define HFI_CMD_SESSION_CVP_SET_SNAPSHOT_MODE\
	(HFI_CMD_SESSION_CVP_START + 0x073)
#define  HFI_CMD_SESSION_EVA_ITOF_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x078)
#define  HFI_CMD_SESSION_EVA_ITOF_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x079)
#define  HFI_CMD_SESSION_EVA_SCALER_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x080)
#define  HFI_CMD_SESSION_EVA_SCALER_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x081)
#define  HFI_CMD_SESSION_EVA_DLFD_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x07C)
#define  HFI_CMD_SESSION_EVA_DLFD_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x07D)
#define  HFI_CMD_SESSION_CVP_RGE_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x07E)
#define  HFI_CMD_SESSION_CVP_RGE_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x07F)
#define  HFI_CMD_SESSION_EVA_DLFL_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x080)
#define  HFI_CMD_SESSION_EVA_DLFL_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x081)
#define  HFI_CMD_SESSION_CVP_SYNX\
	(HFI_CMD_SESSION_CVP_START + 0x086)
#define  HFI_CMD_SESSION_EVA_START\
	(HFI_CMD_SESSION_CVP_START + 0x088)
#define  HFI_CMD_SESSION_EVA_STOP\
	(HFI_CMD_SESSION_CVP_START + 0x089)
#define  HFI_CMD_SESSION_CVP_ICA_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x100)
#define  HFI_CMD_SESSION_CVP_ICA_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x101)
#define  HFI_CMD_SESSION_CVP_DS_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x02F)

#define  HFI_CMD_SESSION_EVA_DFS_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x00B)
#define  HFI_CMD_SESSION_EVA_DFS_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x00C)
#define  HFI_CMD_SESSION_EVA_LME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x059)
#define  HFI_CMD_SESSION_EVA_LME_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x05A)
#define  HFI_CMD_SESSION_EVA_MATCH_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x06D)
#define  HFI_CMD_SESSION_EVA_MATCH_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x06E)
#define  HFI_CMD_SESSION_CVP_FPX_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x004)
#define  HFI_CMD_SESSION_CVP_FPX_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x005)
#define  HFI_CMD_SESSION_EVA_GME_CONFIG\
	(HFI_CMD_SESSION_CVP_START + 0x03B)
#define  HFI_CMD_SESSION_EVA_GME_FRAME\
	(HFI_CMD_SESSION_CVP_START + 0x03A)


#define HFI_MSG_SESSION_CVP_START	\
	(HFI_DOMAIN_BASE_CVP + HFI_ARCH_COMMON_OFFSET +	\
	HFI_MSG_START_OFFSET + 0x1000)

#define HFI_MSG_SESSION_CVP_SET_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x001)
#define HFI_MSG_SESSION_CVP_RELEASE_BUFFERS \
	(HFI_MSG_SESSION_CVP_START + 0x002)
#define HFI_MSG_SESSION_CVP_DS\
	(HFI_MSG_SESSION_CVP_START + 0x003)
#define HFI_MSG_SESSION_CVP_FPX\
	(HFI_MSG_SESSION_CVP_START + 0x004)
#define HFI_MSG_SESSION_CVP_CV_HOG\
	(HFI_MSG_SESSION_CVP_START + 0x005)
#define HFI_MSG_SESSION_CVP_SVM\
	(HFI_MSG_SESSION_CVP_START + 0x006)
#define HFI_MSG_SESSION_CVP_NCC\
	(HFI_MSG_SESSION_CVP_START + 0x007)
#define HFI_MSG_SESSION_EVA_DFS\
	(HFI_MSG_SESSION_CVP_START + 0x008)
#define HFI_MSG_SESSION_CVP_TME\
	(HFI_MSG_SESSION_CVP_START + 0x009)
#define HFI_MSG_SESSION_EVA_SPSTAT\
	(HFI_MSG_SESSION_CVP_START + 0x00A)

#define HFI_MSG_SESSION_CVP_ICA\
	(HFI_MSG_SESSION_CVP_START + 0x014)

#define HFI_MSG_SESSION_CVP_DME\
	(HFI_MSG_SESSION_CVP_START + 0x023)
#define  HFI_MSG_SESSION_EVA_DME_ONLY\
    (HFI_MSG_SESSION_CVP_START + 0x050)
#define HFI_MSG_SESSION_CVP_OPERATION_CONFIG (HFI_MSG_SESSION_CVP_START + 0x030)

#define HFI_MSG_SESSION_CVP_SET_PERSIST_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x034)
#define HFI_MSG_SESSION_CVP_SET_MODEL_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x036)
#define HFI_MSG_SESSION_CVP_FD\
	(HFI_MSG_SESSION_CVP_START + 0x037)
#define HFI_MSG_SESSION_CVP_RELEASE_PERSIST_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x038)
#define  HFI_MSG_SESSION_CVP_RELEASE_MODEL_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x039)
#define  HFI_MSG_SESSION_CVP_SGM_OF\
	(HFI_MSG_SESSION_CVP_START + 0x03A)
#define  HFI_MSG_SESSION_CVP_GCE\
	(HFI_MSG_SESSION_CVP_START + 0x03B)
#define  HFI_MSG_SESSION_CVP_WARP_NCC\
	(HFI_MSG_SESSION_CVP_START + 0x03C)
#define  HFI_MSG_SESSION_CVP_DMM\
	(HFI_MSG_SESSION_CVP_START + 0x03D)
#define  HFI_MSG_SESSION_CVP_SGM_DFS\
	(HFI_MSG_SESSION_CVP_START + 0x03E)
#define  HFI_MSG_SESSION_CVP_WARP\
	(HFI_MSG_SESSION_CVP_START + 0x03F)
#define  HFI_MSG_SESSION_CVP_DMM_PARAMS\
	(HFI_MSG_SESSION_CVP_START + 0x040)
#define  HFI_MSG_SESSION_CVP_WARP_DS_PARAMS\
	(HFI_MSG_SESSION_CVP_START + 0x041)
#define  HFI_MSG_SESSION_CVP_SET_SNAPSHOT_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x045)
#define  HFI_MSG_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS\
	(HFI_MSG_SESSION_CVP_START + 0x046)
#define  HFI_MSG_EVENT_NOTIFY_SNAPSHOT_READY\
	(HFI_MSG_SESSION_CVP_START + 0x047)

#define  HFI_MSG_SESSION_EVA_WARP\
	(HFI_MSG_SESSION_CVP_START + 0x042)
#define  HFI_MSG_SESSION_EVA_LME \
	(HFI_MSG_SESSION_CVP_START + 0x03A)

#define HFI_MSG_SESSION_CVP_FLUSH\
	(HFI_MSG_SESSION_CVP_START + 0x004A)
#define HFI_MSG_SESSION_EVA_START\
	(HFI_MSG_SESSION_CVP_START + 0x0058)
#define HFI_MSG_SESSION_EVA_STOP\
	(HFI_MSG_SESSION_CVP_START + 0x0059)

#define  HFI_MSG_SESSION_CVP_PYS_HCD\
	(HFI_MSG_SESSION_CVP_START + 0x022)
#define  HFI_MSG_SESSION_EVA_DESCRIPTOR\
	(HFI_MSG_SESSION_CVP_START + 0x053)
#define  HFI_MSG_SESSION_EVA_MATCH\
	(HFI_MSG_SESSION_CVP_START + 0x054)
#define  HFI_MSG_SESSION_EVA_GME\
	(HFI_MSG_SESSION_CVP_START + 0x060)
#define  HFI_MSG_SESSION_EVA_SCALER\
	(HFI_MSG_SESSION_CVP_START + 0x062)
#define  HFI_MSG_SESSION_CVP_FPX\
	(HFI_MSG_SESSION_CVP_START + 0x004)

#define CVP_IFACEQ_MAX_PKT_SIZE       1024
#define CVP_IFACEQ_MED_PKT_SIZE       768
#define CVP_IFACEQ_MIN_PKT_SIZE       8
#define CVP_IFACEQ_VAR_SMALL_PKT_SIZE 100
#define CVP_IFACEQ_VAR_LARGE_PKT_SIZE 512
#define CVP_IFACEQ_VAR_HUGE_PKT_SIZE  (1024*12)

/* HFI packet info needed for sanity check */
#define HFI_DFS_CONFIG_CMD_SIZE	38
#define HFI_DFS_FRAME_CMD_SIZE	16

#define HFI_DMM_CONFIG_CMD_SIZE	194
#define HFI_DMM_BASIC_CONFIG_CMD_SIZE	51
#define HFI_DMM_FRAME_CMD_SIZE	28

#define HFI_PERSIST_CMD_SIZE	11

#define HFI_DS_CONFIG_CMD_SIZE 11
#define HFI_DS_CMD_SIZE	50

#define HFI_OF_CONFIG_CMD_SIZE 34
#define HFI_OF_FRAME_CMD_SIZE 24

#define HFI_ODT_CONFIG_CMD_SIZE 23
#define HFI_ODT_FRAME_CMD_SIZE 33

#define HFI_OD_CONFIG_CMD_SIZE 24
#define HFI_OD_FRAME_CMD_SIZE 12

#define HFI_NCC_CONFIG_CMD_SIZE 47
#define HFI_NCC_FRAME_CMD_SIZE 22

#define HFI_ICA_CONFIG_CMD_SIZE 127
#define HFI_ICA_FRAME_CMD_SIZE 14

#define HFI_HCD_CONFIG_CMD_SIZE 46
#define HFI_HCD_FRAME_CMD_SIZE 18

#define HFI_DCM_CONFIG_CMD_SIZE 20
#define HFI_DCM_FRAME_CMD_SIZE 19

#define HFI_PYS_HCD_CONFIG_CMD_SIZE 461
#define HFI_PYS_HCD_FRAME_CMD_SIZE 66

#define HFI_FD_CONFIG_CMD_SIZE 28
#define HFI_FD_FRAME_CMD_SIZE  10

struct cvp_hfi_cmd_session_flush_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 flush_type;
};

struct cvp_hfi_cmd_session_get_property_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 num_properties;
	u32 rg_property_data[1];
};

struct cvp_hfi_msg_sys_session_abort_done_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
};

struct cvp_hfi_msg_sys_property_info_packet {
	u32 size;
	u32 packet_type;
	u32 num_properties;
	u32 rg_property_data[] __counted_by(size);
};

enum session_flags {
	SESSION_PAUSE = BIT(1),
};

struct cvp_hal_session {
	struct list_head list;
	void *session_id;
	u32 flags;
	void *device;
};

struct cvp_hfi_debug_config {
	u32 debug_config;
	u32 debug_mode;
};

struct cvp_hfi_enable {
	u32 enable;
};

#define HFI_RESOURCE_SYSCACHE 0x00000002

struct cvp_hfi_resource_subcache_type {
	u32 target_hw;
	u32 sc_id;
};

struct cvp_hfi_resource_syscache_info_type {
	u32 num_entries;
	struct cvp_hfi_resource_subcache_type rg_subcache_entries[];
};

enum HFI_SYSCACHE_TARGET_TYPE {
	HFI_SYSCACHE_TARGET_EVA_CPU,
	HFI_SYSCACHE_TARGET_FDU,
	HFI_SYSCACHE_TARGET_MPU,
	HFI_SYSCACHE_TARGET_GCE,
	HFI_SYSCACHE_TARGET_FDX,
};

struct cvp_hal_cmd_pkt_hdr {
	u32 size;
	u32 packet_type;
};

struct cvp_hal_msg_pkt_hdr {
	u32 size;
	u32 packet;
};

struct cvp_hal_session_cmd_pkt {
	u32 size;
	u32 packet_type;
	u32 session_id;
};

struct cvp_hfi_cmd_sys_init_packet {
	u32 size;
	u32 packet_type;
	u32 arch_type;
};

struct cvp_hfi_cmd_sys_pc_prep_packet {
	u32 size;
	u32 packet_type;
};

struct cvp_hfi_cmd_sys_set_resource_packet {
	u32 size;
	u32 packet_type;
	u32 resource_handle;
	u32 resource_type;
	u32 rg_resource_data[];
};

struct cvp_hfi_cmd_sys_release_resource_packet {
	u32 size;
	u32 packet_type;
	u32 resource_type;
	u32 resource_handle;
};

struct cvp_hfi_cmd_sys_set_property_packet {
	u32 size;
	u32 packet_type;
	u32 num_properties;
	u32 rg_property_data[];
};

struct cvp_hfi_cmd_sys_get_property_packet {
	u32 size;
	u32 packet_type;
	u32 num_properties;
	u32 rg_property_data[];
};

struct cvp_hfi_cmd_sys_session_init_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 session_type;
	u32 session_kmask;
	u32 session_prio;
	u32 is_secure;
	u32 dsp_ac_mask;
};

struct cvp_hfi_cmd_sys_session_end_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
};

struct cvp_hfi_cmd_sys_set_buffers_packet {
	u32 size;
	u32 packet_type;
	u32 buffer_type;
	u32 buffer_size;
	u32 num_buffers;
	u32 rg_buffer_addr[];
};

struct cvp_hfi_cmd_sys_set_ubwc_config_packet_type {
	u32 size;
	u32 packet_type;
	struct {
		u32 max_channel_override : 1;
		u32 mal_length_override : 1;
		u32 hb_override : 1;
		u32 bank_swzl_level_override : 1;
		u32 bank_spreading_override : 1;
		u32 reserved : 27;
	} override_bit_info;
	u32 max_channels;
	u32 mal_length;
	u32 highest_bank_bit;
	u32 bank_swzl_level;
	u32 bank_spreading;
	u32 reserved[2];
};

struct cvp_hfi_cmd_session_set_property_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 num_properties;
	u32 rg_property_data[];
};

struct cvp_hfi_client {
	u32 data1;
	u32 data2;
	u32 data3;
	u32 data4;
	u64 kdata;
	u64 transaction_id;
	u32 reserved1;
	u32 reserved2;
} __packed;

struct cvp_hfi_buf_type {
	u32 iova;
	u32 size;
	u32 offset;
	u32 flags;
	u32 reserved1;
	u32 reserved2;
	u32 fence_type;
	u32 input_handle;
	u32 output_handle;
	u32 ndebug_flags;
	u32 ncrc;
};

struct cvp_hfi_cmd_session_set_buffers_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	struct cvp_hfi_client client_data;
	struct cvp_hfi_buf_type buf_type;
} __packed;

struct cvp_session_release_buffers_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	struct cvp_hfi_client client_data;
	u32 kernel_type;
	u32 buffer_type;
	u32 num_buffers;
	u32 buffer_idx;
} __packed;

struct cvp_hfi_header_type {
	u32 size;
	u32 packet_type;
	u32 session_id;
	struct cvp_hfi_client client_data;
	u32 stream_idx;
	u32 packet_crc;
} __packed;

struct cvp_hfi_cmd_session_hdr {
	struct cvp_hfi_header_type header;
} __packed;

struct cvp_hfi_msg_session_hdr {
	struct cvp_hfi_header_type header;
	u32 error_type;
} __packed;

#ifdef TEMP_WORKAROUND
struct cvp_hfi_msg_session_hdr_old_format {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
	struct cvp_hfi_client client_data;
	u32 stream_idx;
} __packed;
#endif

struct cvp_hfi_dumpmsg_session_hdr {
	struct cvp_hfi_header_type header;
	u32 error_type;
	u32 dump_offset;
	u32 dump_size;
} __packed;

#define HFI_MAX_HW_ACTIVATIONS_PER_FRAME (6)

enum hfi_hw_thread {
	HFI_HW_FDU,
	HFI_HW_MPU,
	HFI_HW_OD,
	HFI_HW_ICA,
	HFI_HW_VADL,
	HFI_HW_TOF,
	HFI_HW_RGE,
	HFI_HW_XRA,
	HFI_HW_LSR,
	HFI_MAX_HW_THREADS
};

struct cvp_hfi_msg_session_hdr_ext {
	struct cvp_hfi_header_type header;
	u32 error_type;
	u32 busy_cycles;
	u32 total_cycles;
	u32 hw_cycles[HFI_MAX_HW_THREADS][HFI_MAX_HW_ACTIVATIONS_PER_FRAME];
	u32 fw_cycles[HFI_MAX_HW_ACTIVATIONS_PER_FRAME];
} __packed;

struct cvp_hfi_buffer_mapping_type {
	u32 index;
	u32 device_addr;
	u32 size;
};

struct cvp_hfi_cmd_session_sync_process_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 sync_id;
	u32 rg_data[];
};

struct cvp_hfi_msg_event_notify_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 event_id;
	u32 event_data1;
	u32 event_data2;
	u32 rg_ext_event_data[];
};

struct cvp_hfi_msg_session_op_cfg_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
	struct cvp_hfi_client client_data;
	u32 stream_idx;
	u32 op_conf_id;
} __packed;

struct cvp_hfi_msg_sys_init_done_packet {
	u32 size;
	u32 packet_type;
	u32 error_type;
	u32 num_properties;
	u32 rg_property_data[];
};

struct cvp_hfi_msg_sys_pc_prep_done_packet {
	u32 size;
	u32 packet_type;
	u32 error_type;
};

struct cvp_hfi_msg_sys_release_resource_done_packet {
	u32 size;
	u32 packet_type;
	u32 resource_handle;
	u32 error_type;
};

struct cvp_hfi_msg_sys_session_init_done_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
	u32 num_properties;
	u32 rg_property_data[];
};

struct cvp_hfi_msg_sys_session_end_done_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
};

struct cvp_hfi_msg_session_get_sequence_header_done_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
	u32 header_len;
	u32 sequence_header;
};

struct cvp_hfi_msg_sys_debug_packet {
	u32 size;
	u32 packet_type;
	u32 msg_type;
	u32 msg_size;
	u32 time_stamp_hi;
	u32 time_stamp_lo;
	u8 rg_msg_data[];
};

struct cvp_hfi_packet_header {
	u32 size;
	u32 packet_type;
};

/**
 * Structure corresponding to HFI_CVP_BUFFER_TYPE
 */

struct cvp_buf_type {
	__s32 fd;
	__u32 size;
	__u32 offset;
	__u32 flags;
	__u32 reserved1;
	__u32 reserved2;
	__u32 fence_type;
	__u32 input_handle;
	__u32 output_handle;
	__u32 debug_flags;
	__u32 crc;
	__u32 context_bank_id;
};

struct cvp_hfi_persist_buffer_packet {
	struct cvp_hfi_header_type sHeader;
	u32 nCVKernelType;
	struct cvp_buf_type nPersist1Buffer;
	struct cvp_buf_type nPersist2Buffer;
	struct cvp_buf_type nPersist3Buffer;
};

struct cvp_hfi_sfr_struct {
	u32 bufSize;
	u8 rg_data[];
};

struct cvp_hfi_cmd_sys_test_ssr_packet {
	u32 size;
	u32 packet_type;
	u32 trigger_type;
};

struct cvp_hfi_msg_sys_session_ctrl_done_packet {
	u32 size;
	u32 packet_type;
	u32 session_id;
	u32 error_type;
	struct cvp_hfi_client client_data;
};

enum buf_map_type {
	MAP_PERSIST = 1,
	UNMAP_PERSIST = 2,
	MAP_FRAME = 3,
	MAP_INVALID,
};

static inline enum buf_map_type cvp_find_map_type(int pkt_type)
{
	if (pkt_type == HFI_CMD_SESSION_CVP_SET_PERSIST_BUFFERS ||
			pkt_type == HFI_CMD_SESSION_CVP_SET_MODEL_BUFFERS ||
			pkt_type == HFI_CMD_SESSION_CVP_DMM_PARAMS ||
			pkt_type == HFI_CMD_SESSION_CVP_SET_SNAPSHOT_BUFFERS ||
			pkt_type == HFI_CMD_SESSION_CVP_WARP_DS_PARAMS ||
			pkt_type == HFI_CMD_SESSION_EVA_DLFL_CONFIG)
		return MAP_PERSIST;
	else if (pkt_type == HFI_CMD_SESSION_CVP_RELEASE_PERSIST_BUFFERS ||
			pkt_type ==
				HFI_CMD_SESSION_CVP_RELEASE_SNAPSHOT_BUFFERS)
		return UNMAP_PERSIST;
	else
		return MAP_FRAME;
}

static inline bool is_params_pkt(int pkt_type)
{
	if (pkt_type == HFI_CMD_SESSION_CVP_DMM_PARAMS ||
		pkt_type == HFI_CMD_SESSION_CVP_WARP_DS_PARAMS)
		return true;

	return false;
}

#endif
