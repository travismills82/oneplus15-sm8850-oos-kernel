/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef __LINUX_THQSPI_PROTO_H
#define __LINUX_THQSPI_PROTO_H

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/timer.h>
#include <linux/ipc_logging.h>
#include "thqspi.h"

#define MAX_DEV               (1)
#define MAX_INIT_RETRY        (2)
#define DEVICE_NAME_MAX_LEN  (64)
#define DATA_WORD_LEN         (4)
#define MAX_CLIENT_PKTS      (32)
#define DATA_BYTES_PER_LINE  (64)
#define CLIENT_WAKE_TOUT_MS (300)
#define THREAD_MN_DEV_NUM     (0)
#define IRQ_WRITE_SIZE        (9)  // TBD fix size
#define IRQ_TIMEOUT_MS      (500)
#define CLIENT_INIT_IRQ_TIMEOUT_MS (2000)
#define PROTOID_THREAD     (0x41)
#define COLD_RESET_CMD_SIZE (0x07)
#define THREAD_CORE_COMMAND_PCKT    (0x21)
#define THREAD_CORE_RESPONSE_PCKT   (0x41)
#define THREAD_CORE_PCKT_TYPE_RESET (0x01)
#define THREAD_COLD_RESET_FLAG (0x8000)
#define THREAD_COLD_RESET_FLAG_LEN (0x02)
#define THREAD_STATUS_SUCCESS      (0x00)
#define MAX_INVALID_DATA_COUNT     (0x03)

/*
 * The following is a utility MACRO that exists to Assign a 
 * specified DWord_t to an unaligned Memory Address.  This MACRO
 * accepts as it's first parameter the Memory Address to store the
 * specified Little Endian DWord_t into.  The second parameter is the
 * actual DWord_t value to store into the specified Memory Location.
 * NOTE * The second parameter needs to be stored in the Endian
 *          format of the Native Host's processor. 
 */
#define ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(_x, _y)  \
{                                                                   \
  ((uint8_t  *)(_x))[0] = ((uint8_t )(((uint32_t)(_y)) & 0xFF));         \
  ((uint8_t  *)(_x))[1] = ((uint8_t )((((uint32_t)(_y)) >> 8) & 0xFF));  \
  ((uint8_t  *)(_x))[2] = ((uint8_t )((((uint32_t)(_y)) >> 16) & 0xFF)); \
  ((uint8_t  *)(_x))[3] = ((uint8_t )((((uint32_t)(_y)) >> 24) & 0xFF)); \
}

/*
 * Some valid values:
 * 3000000, 4000000, 6000000, 8000000, 12000000, 24000000, 48000000
 */
#define THQSPI_CLOCK_FREQUENCY_DEFAULT (3000000)


static int thqspi_cdev_major;
static int thqspi_alloc_count;

#define THQSPI_LOG_DBG_MSG(print, dev, x...) do { \
if (print) { \
	if (dev) \
		dev_dbg((dev), x); \
	else \
		pr_debug(x); \
} \
} while (0)

#define THQSPI_LOG_ERR_MSG(print, dev, x...) do { \
if (print) { \
	if (dev) \
		dev_err((dev), x); \
	else \
		pr_err(x); \
} \
} while (0)

#define THQSPI_INFO(spi_ptr, x...) do { \
if (spi_ptr) { \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, x);\
	if (spi_ptr->dev) \
		thqspi_trace_log(spi_ptr->dev, x); \
} \
} while (0)

#define THQSPI_DBG(spi_ptr, x...) do { \
if (spi_ptr) { \
	THQSPI_LOG_DBG_MSG (true, spi_ptr->dev, x); \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, x); \
	if (spi_ptr->dev) \
		thqspi_trace_log(spi_ptr->dev, x); \
} \
} while (0)

#define THQSPI_ERR(spi_ptr, x...) do { \
if (spi_ptr) { \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, "THQSPIERR- " x); \
	THQSPI_LOG_ERR_MSG(true, spi_ptr->dev, "THQSPIERR- " x); \
	if (spi_ptr->dev) \
		thqspi_trace_log(spi_ptr->dev, "THQSPIERR- " x); \
	pr_info("THQSPIERR- " x); \
	pr_info("\n"); \
} \
} while (0)

enum sleep_state {
	ASLEEP,
	AWAKE_PENDING,
	CLIENT_WAKEUP,
	AWAKE,
	RESET,
};

/**
 * Char device struct
 */
struct thqspi_chrdev {
	dev_t spidev;
	struct cdev c_dev;
	int major;
	int minor;
	char dev_name[DEVICE_NAME_MAX_LEN];
	struct device *class_dev;
	struct class *thqspi_class;
};

/**
 * thqspi_packet: stuct to store user request
 */
struct thqspi_packet {
	int status;
	struct thqspi_user_request user_req;
};

/**
 * thqspi_user: user space client structure
 */
struct thqspi_user {
	atomic_t rx_avail;
	bool is_active;
	bool fifo_full;
	bool read_pending;
	wait_queue_head_t readq;
	wait_queue_head_t readwq;
	DECLARE_KFIFO(user_fifo, struct thqspi_client_request, MAX_CLIENT_PKTS);
	struct completion sync_wait;
};

struct memory_manager {
	uint8_t                  *single_byte;
	uint32_t                 *reg_read_write;
	uint8_t                  *tx_payload;
	uint8_t                  *rx_payload;
	HostStatusRegisters_t    *status_reg;
	HostIntEnableRegisters_t *host_reg; // Used as a byte buffer aligned to 4 bytes
};

/**
 * struct thqspi_priv - structure to store spi cnss information
 * @spi: pointer to spi device
 * @spi_msg1: message for single transfer
 * @sp_xfer1: single msg transfer
 * @chrdev: char device structure
 * @user: thqspi_user structure, user-space client associated with char dev
 * @tx_list: struct list head for tx packet
 * @rx_list: struct list head for rx
 * @kreader: kernel read thread to read data from controller
 * @read_msg: work function to process read request
 * @dev: driver model representation of the device
 * @spi_max_freq: spi max freq
 * @irq: interrupt number
 * @gpio: gpio for interrupt
 * @readq: wait queue for rx data
 * @hcr_lock: mutex lock for hcr operations
 * @queue_lock: mutex lock for tx list
 * @xfer_lock: mutex lock for spi transfer
 * @user_req: user request struct
 * @sync_wait: completion for user request
 * @bh_work: work to queue for bh ISR
 * @bh_work_wq: workqueue pointer for bh ISR
 * @write_pending: write process book keeper
 * @read_pending: read process book keeper
 * @client_init: client status
 * @wake_wait: completion for wakeup sequence
 * @buff_wait: completion for CPU Int0 // Client ready to receive
 * @wait_to_notify: bool for buff_wait completion
 * @state_lock: mutex to protect client state update
 * @client_irq_buf: pointer to hold clear irq buf
 * @spi_alloc_cnt: number of memory chunks allocated
 * @is_cold_reset_req: is this cold reset request
 * @is_cold_reset_done: is cold reset complete
 *
 */
struct thqspi_priv {
	struct spi_device *spi;
	struct thqspi_chrdev chrdev;
	struct thqspi_user user;
	struct list_head tx_list;
	struct list_head rx_list;
	struct kthread_worker *kreader;
	struct kthread_work read_msg;
	struct device *dev;
	u32 spi_max_freq;
	int irq;
	int gpio;
	wait_queue_head_t readq;
	struct mutex hcr_lock;
	struct mutex xfer_lock;
	struct mutex read_lock;
	struct mutex sleep_lock;
	struct thqspi_user_request user_req;
	struct completion sync_wait;
	struct work_struct bh_work;
	struct workqueue_struct *bh_work_wq;
	struct work_struct sleep_work;
	struct workqueue_struct *sleep_wq;
	bool write_pending;
	bool read_pending;
	bool hostintstatus_read_pending;
	bool client_init;
	struct completion init_irq_wait;
	struct completion buff_wait;
	struct completion resume_wait;
	atomic_t check_resume_wait;
	bool wait_to_notify;
	struct mutex state_lock;
	struct mutex mem_lock;
	u8 *client_irq_buf;
	atomic_t thqspi_alloc_cnt;
	struct memory_manager mem_mngr;
	void *ipc;
	bool sleep_enabled;
	bool is_cold_reset_req;
	bool is_cold_reset_done;
	uint8_t invalid_data_count;
};
#endif //__LINUX_THQSPI_PROTO_H
