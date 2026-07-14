/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __LINUX_SPI_CNSS_PROTO_H
#define __LINUX_SPI_CNSS_PROTO_H

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/timer.h>
#include <linux/ipc_logging.h>
#include "spi_cnss.h"

#define MAX_DEV 2
#define MAX_INIT_RETRY 2
#define DEVICE_NAME_MAX_LEN 64
#define DATA_WORD_LEN 4
#define XFER_TIMEOUT 5000
#define MAX_CLIENT_PKTS 32
#define SPI_IRQ_TIMEOUT 300
#define NOP_XFER_TIMEOUT 150
#define SPI_AUTOSUSPEND_DELAY 300 //XFER_TIMEOUT + 50
#define CLIENT_WAKE_TIME_OUT 350 //NOP_XFER_TIMEOUT*2 + 50
#define SPI_CLIENT_SLEEP_TIME_MS 100

#define SPI_WRITE_OPCODE    (0x02)
#define SPI_FREAD_OPCODE    (0x0B)
#define SPI_REGISTER_READ   (0x81)
#define SPI_REGISTER_WRITE  (0x82)
#define NOP_CMD_LEN 100

//Registers
#define SPI_SLAVE_SANITY_REG    (0x00)
#define SPI_SLAVE_DEVICE_ID_REG (0x04)
#define SPI_SLAVE_STATUS_REG    (0x08)
#define SPI_SLAVE_CONFIG_REG    (0x0C)

#define SPI_CONTEXT_INFO_BASE 0x00000000

#define UCI_REQ   0x21
#define UCI_DATA  0x22
#define PERI_CMD  0x31
#define PERI_DATA 0x32
#define PERI_EVT  0x34


#define CMD_SIZE                  1
#define ADDR_BYTES                4
#define PROTO_BYTE                1
#define DUMMY_CYCLE               5
#define REG_READ_DUMMY_CYCLE      2
#define CONTEXT_BUF_SIZE          512
#define WRITE_SIZE                (CMD_SIZE + ADDR_BYTES + PROTO_BYTE)
#define NOTIFIER_WRITE_SIZE       9
#define FREAD_TX_SIZE             (5 + DUMMY_CYCLE)//1 byte opcode + 4 byte addr + 4 byte dummy cycles
#define FREAD_RX_SIZE             4
#define FWRITE_SIZE               FREAD_RX_SIZE
//#define SLEEP_CMD_SIZE          6
#define IRQ_WRITE_SIZE            9
#define REGISTER_READ_SIZE        (2 + REG_READ_DUMMY_CYCLE)//1 byte opcode + 1 byte addr +  2 byte dummy cycles
#define CONTEXT_INFO_READ_SIZE    (5 + DUMMY_CYCLE + 20)//1 opcode + 4 addr + 5 dummy + 20 bytes context info
#define UWB                       1
#define BT                        0
#define HOST_IRQ                  0x00000001
#define SOFT_RESET_IRQ            0x00008000
#define SLEEP_CMD_BYTE            0xFE
#define RESET_CMD_BYTE            0xF0
#define CMD_BYTE_OFFSET           5
#define REG_TX_SIZE               16
#define REG_RX_SIZE               16
//static u8 *client_irq_buf;

/*static u8 *host_irq_buf;
static u8 *hlen_tx_buf;
static u8 *hlen_rx_buf;
static u8 *clen_tx_buf;
static u8 *clen_rx_buf;
static u8 *soft_reset_buf;
static u8 *nop_cmd_buf;
static u8 *sleep_cmd_buf;*/
//static u8 *client_sleep_buff;
static int spi_cnss_cdev_major;
static int spi_cnss_alloc_count;

#define SPI_LOG_DBG_MSG(print, dev, x...) do { \
if (print) { \
	if (dev) \
		dev_dbg((dev), x); \
	else \
		pr_debug(x); \
} \
} while (0)

#define SPI_LOG_ERR_MSG(print, dev, x...) do { \
if (print) { \
	if (dev) \
		dev_err((dev), x); \
	else \
		pr_err(x); \
} \
} while (0)

#define SPI_CNSS_INFO(spi_ptr, x...) do { \
if (spi_ptr) { \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, x);\
	if (spi_ptr->dev) \
		spi_cnss_trace_log(spi_ptr->dev, x); \
	/*pr_info(x);*/ \
} \
} while (0)

#define SPI_CNSS_DBG(spi_ptr, x...) do { \
if (spi_ptr) { \
	SPI_LOG_DBG_MSG (true, spi_ptr->dev, x); \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, x); \
	if (spi_ptr->dev) \
		spi_cnss_trace_log(spi_ptr->dev, x); \
	/*pr_info(x);*/ \
} \
} while (0)

#define SPI_CNSS_ERR(spi_ptr, x...) do { \
if (spi_ptr) { \
	if (spi_ptr->ipc) \
		ipc_log_string(spi_ptr->ipc, x); \
	SPI_LOG_ERR_MSG(true, spi_ptr->dev, x); \
	if (spi_ptr->dev) \
		spi_cnss_trace_log(spi_ptr->dev, x); \
	pr_info(x); \
} \
} while (0)

enum sleep_state {
	ASLEEP,
	AWAKE_PENDING,
	CLIENT_WAKEUP,
	AWAKE,
	RESET,
};

struct memory_manager {
	u8* tx_payload;
	u8* rx_payload;
	u8* rx_cmd_buf;
	u8* notifier_one;
	u8* notifier_two;
	u8 *host_irq_buf;
	/*u8 *hlen_tx_buf;
	u8 *hlen_rx_buf;*/
	u8 *len_tx_buf;
	u8 *len_rx_buf;
	u8 *soft_reset_buf;
	u8 *nop_cmd_buf;
	u8 *single_byte_cmd_buf;
	u8* register_tx_buf;
	u8* register_rx_buf;
	u8* clen_notifier_one;
	u8* clen_notifier_two;
};

struct client_info {
	u32 buffer_pointer;
	u32 HBUF_LEN;
	u32 CBUF_LEN;
	u32 HBUF_SIZE;
	u32 CBUF_SIZE;
	u32 HBUF_LEN_ADDR;
	u32 CBUF_LEN_ADDR;
	u32 HBUF_BASE_ADDR;
	u32 CBUF_BASE_ADDR;
	u32 HCINT_BASE_ADDR;
	u32 CHINT_BASE_ADDR;
};
/*
* Char device struct
*/
struct spi_cnss_chrdev {
	dev_t spidev;
	struct cdev c_dev[MAX_DEV];
	int major;
	int minor;
	char dev_name[DEVICE_NAME_MAX_LEN];
	struct device *class_dev;
	struct class *spi_cnss_class;
};

/**
 * spi_cnss_packet: stuct to store user request
 */
struct spi_cnss_packet {
	int id;
	int status;
	struct spi_usr_request user_req;
	struct list_head list;
};

/**
 * spi_cnss_user: user space client structure
 */
struct spi_cnss_user {
	int id;
	atomic_t rx_avail;
	bool is_active;
	bool fifo_full;
	bool read_pending;
	wait_queue_head_t readq;
	wait_queue_head_t readwq;
	DECLARE_KFIFO(user_fifo, struct spi_client_request, MAX_CLIENT_PKTS);
	struct completion sync_wait;
};
/**
 * struct spi_cnss_priv - structure to store spi cnss information
 * @spi: pointer to spi device
 * @spi_msg1: message for single transfer
 * @sp_xfer1: single msg transfer
 * @chrdev: char device structure
 * @user: spi_cnss_user structure, user-space client associated with char dev
 * @tx_list: struct list head for tx packet
 * @rx_list: struct list head for rx
 * @kwriter: kernel write thread for user request
 * @send_msg: work function to process user request
 * @kreader: kernel read thread to read data from controller
 * @read_msg: work function to process read request
 * @dev: driver model representation of the device
 * @spi_max_freq: spi max freq
 * @irq: interrupt number
 * @gpio: gpio for interrupt
 * @readq: wait queue for rx data
 * @queue_lock: mutex lock for tx list
 * @xfer_lock: mutex lock for spi transfer
 * @usr_req: user request struct
 * @sync_wait: completion for user request
 * @bh_work: work to queue for bh ISR
 * @bh_work_wq: workqueue pointer for bh ISR
 * @rx_avail: used to notify rx data to client
 * @client: struct to store context info address
 * @write_pending: write process book keeper
 * @read_pending: read process book keeper
 * @client_init: client status
 * @usr_cnt: keeps count of active users
 * @client_state: used to store sleep state
 * @wake_wait: completion for wakeup sequence
 * @buff_wait: completion for kwriter if it is waiting for HBUF
 * @wait_to_notify: bool for buff_wait completion
 * @state_lock: mutex to protect client state update
 * @client_irq_buf: pointer to hold clear irq buf
 * @spi_alloc_cnt: number of memory chunks allocated
 *
 */
struct spi_cnss_priv {
	struct spi_device *spi;
	struct spi_message spi_msg1;
	struct spi_transfer spi_xfer1;
	struct spi_cnss_chrdev chrdev;
	struct spi_cnss_user user[MAX_DEV];
	struct list_head tx_list;
	struct list_head rx_list;
	struct kthread_worker *kwriter;
	struct kthread_work send_msg;
	struct kthread_worker *kreader;
	struct kthread_work read_msg;
	struct device *dev;
	u32 spi_max_freq;
	int irq;
	int gpio;
	wait_queue_head_t readq;
	//wait_queue_head_t readwq;
	struct mutex queue_lock;
	struct mutex xfer_lock;
	struct mutex read_lock;
	struct mutex irq_lock;
	struct mutex sleep_lock;
	struct spi_usr_request usr_req;
	struct completion sync_wait;
	struct work_struct bh_work;
	struct workqueue_struct *bh_work_wq;
	struct work_struct sleep_work;
	struct workqueue_struct *sleep_wq;
	atomic_t rx_avail;
	struct client_info client;
	bool write_pending;
	bool read_pending;
	bool context_read_pending;
	bool client_init;
	u8 usr_cnt;
	enum sleep_state client_state;
	bool state_transition;
	struct completion wake_wait;
	struct completion buff_wait;
	struct completion resume_wait;
	atomic_t check_resume_wait;
	bool wait_to_notify;
	struct mutex state_lock;
	struct mutex mem_lock;
	u8 *client_irq_buf;
	atomic_t spi_alloc_cnt;
	struct timer_list client_sleep_timer;
	struct memory_manager mem_mngr;
	void *ipc;
	bool sleep_enabled;
	atomic_t write_err_code;
};
#endif //__LINUX_SPI_CNSS_PROTO_H
