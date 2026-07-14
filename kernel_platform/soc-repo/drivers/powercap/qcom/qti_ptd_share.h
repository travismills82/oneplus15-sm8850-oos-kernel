/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QCOM_PDT_DS_H__
#define __QCOM_PDT_DS_H__

#include <linux/ipc_logging.h>
#include <linux/soc/qcom/qptf.h>

#define IPC_LOGPAGES 10
#define PDT_DBG_DATA(pdt, msg, ...) do {				\
		dev_dbg(pdt->dev, "%s:" msg, __func__, ##__VA_ARGS__);	\
		if ((pdt) && (pdt)->ipc_log_data) {			\
			ipc_log_string((pdt)->ipc_log_data,		\
			"[%s] "msg"\n",					\
			 current->comm, ##__VA_ARGS__);			\
		}							\
	} while (0)

#define PDT_DBG_EVENT(pdt, msg, ...) do {				\
		dev_dbg(pdt->dev, "%s:" msg, __func__, ##__VA_ARGS__);	\
		if ((pdt) && (pdt)->ipc_log_event) {			\
			ipc_log_string((pdt)->ipc_log_event,		\
			"[%s] "msg"\n",					\
			 current->comm, ##__VA_ARGS__);			\
		}							\
	} while (0)

#define PDT_DS_SMEM_PROC_ID	QCOM_SMEM_HOST_ANY
#define PDT_DS_SMEM_ITEM_HANDLE	624
#define PDT_DS_SMEM_BLOCK_SIZE	(1024*4)
#define PDT_DS_SMEM_DATA_OFFSET	(1024*2)

#define PDT_DS_VERSION	1

/**
 * struct pdt_priv - Structure for pdt private data
 * @dev:		Pointer for PDT device
 * @ipc_log_data:	Handle to ipc_logging for data update buffer
 * @ipc_log_event:	Handle to ipc_logging for qpt events
 * @pdt_nb:		Notifier for periodic data updates
 * @hw_read_lock:	lock to protect avg data update and client request
 */
struct pdt_priv {
	struct device		*dev;
	void			*ipc_log_data;
	void			*ipc_log_event;
	struct notifier_block	pdt_nb;
	struct mutex		hw_read_lock;
};

#endif /* __QCOM_PDT_DS_H__ */
