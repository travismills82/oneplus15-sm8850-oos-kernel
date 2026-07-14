/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef __HAB_VIRQ_HGY_H
#define __HAB_VIRQ_HGY_H

int hgy_virq_tx_register(struct hvirq_dbl *dbl, int dbl_label);
int hgy_virq_rx_register(struct hvirq_dbl *dbl, int dbl_label);
int hgy_virq_send(struct hvirq_dbl *dbl);
int hgy_virq_tx_unregister(struct hvirq_dbl *dbl);
int hgy_virq_rx_unregister(struct hvirq_dbl *dbl);
int hgy_get_virq_num_id(void **virqdev, int label);
#endif
