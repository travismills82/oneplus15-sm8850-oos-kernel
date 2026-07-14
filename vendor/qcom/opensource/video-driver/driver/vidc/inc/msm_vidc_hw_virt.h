/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _H_MSM_VIDC_HW_VIRT_H_
#define _H_MSM_VIDC_HW_VIRT_H_

#define GVM_SSR_DEVICE_DRIVER       0x80000000
#define GVM_SSR                     0x300

struct msm_vidc_full_virtualization {
	uint32_t vmid;
	uint32_t device_core_mask;
	bool virtualization_en;
	bool is_gvm_open;
	bool gvm_deinit;
	struct task_struct *pvm_event_handler_thread;
};

#ifdef MSM_VIDC_HW_VIRT

#include "vidc_hw_virt.h"

#else

#include <linux/list.h>
#define MAX_HW_VIRT_CMD_PAYLOAD_SIZE (1024)

struct virtio_video_msm_hw_event {
	__le32 event_type; /* One of VIRTIO_VIDEO_EVENT_* types */
	__le32 stream_id;
	__u8 payload[MAX_HW_VIRT_CMD_PAYLOAD_SIZE - 2 * sizeof(__le32)];
};

struct virtio_video_queuing_event {
	struct virtio_video_msm_hw_event evt;
	uint32_t device_id;
	struct list_head list;
};

static inline int virtio_video_queue_event_wait(
	struct virtio_video_msm_hw_event *evt)
{
	return -EINVAL;
}

static inline int32_t virtio_video_msm_cmd_open_gvm(uint32_t vm_id,
	uint32_t device_id_mask,
	uint32_t *device_core_mask)
{
	return -EINVAL;
}
static inline int32_t virtio_video_msm_cmd_close_gvm(void)
{
	return -EINVAL;
}
static inline int32_t virtio_video_msm_cmd_open_gvm_session(uint32_t *device_id,
	uint32_t *session_id)
{
	return -EINVAL;
}
static inline int32_t virtio_video_msm_cmd_pause_gvm_session(uint32_t device_id,
	uint32_t session_id)
{
	return -EINVAL;
}
static inline int32_t virtio_video_msm_cmd_resume_gvm_session(uint32_t device_id,
	uint32_t session_id)
{
	return -EINVAL;
}

#endif

#endif //_H_MSM_VIDC_HW_VIRT_H_
