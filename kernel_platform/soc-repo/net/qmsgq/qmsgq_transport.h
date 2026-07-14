/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef __QMSGQ_TRANSPORT_H_
#define __QMSGQ_TRANSPORT_H_

int qmsgq_transport_init(struct qmsgq_gh_device *qdev);
int qmsgq_transport_process_recv(struct qmsgq_gh_device *qdev, char *rx_buf, size_t len);
#endif
