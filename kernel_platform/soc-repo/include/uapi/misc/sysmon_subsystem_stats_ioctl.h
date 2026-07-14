/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __QCOM_SYSMON_SUBSYSTEM_STATS_IOCTL_H__
#define __QCOM_SYSMON_SUBSYSTEM_STATS_IOCTL_H__

#include <linux/types.h>

/** Device name for SUBSYSTEM STATS */
#define SUBSYSTEMSTATS_DEVICE_NAME             "/dev/sysmon_subsystem_stats"

/** IOCTL to get HMX utilization */
#define HVX_UTILIZATION_QUERY _IOR('q', 1, u32)
/** IOCTL to get HVX utilization */
#define HMX_UTILIZATION_QUERY _IOR('q', 2, u32)
/** IOCTL to get q6 utilization */
#define Q6_CDSP_UTILIZATION _IOR('q', 3, u32)

#endif /* __QCOM_SUBSYSTEMSTATS_H__ */
