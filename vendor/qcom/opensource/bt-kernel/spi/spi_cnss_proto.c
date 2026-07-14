/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

/*
* SPI cnss protocol driver for UWB
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/device.h>
#include "spi_cnss_proto.h"
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
#include "spi_stub_driver.h"
#endif
#define CREATE_TRACE_POINTS
#include "spi-cnss-trace.h"

#define WRITE_RETRY 2
#define DATA_BYTES_PER_LINE 64
#define SANITY_CHECK_ITERATION 5
#define BT_MINOR_DEV_NUM 0
#define UWB_MINOR_DEV_NUM 1
u32 slave_config = 0x05000000;
int user_id = 0xFFFF;


#define CONFIG_SLEEP
#define CONFIG_AGGRESSIVE_SLEEP

#define NUM_OF_TRIALS_DURING_OPEN 10
#define NUM_OF_TRIALS_DURING_TRANS 5
static void spi_cnss_notify_data_avail(struct spi_cnss_user *user);
static void spi_cnss_reinit_xfer(struct spi_transfer* xfer, int size);
static int spi_cnss_register_xfer(struct spi_cnss_priv *spi_drv, u8 reg, u8 opcode);
void* spi_cnss_kzalloc(struct spi_cnss_priv *spi_drv, int size);
void spi_cnss_kfree(struct spi_cnss_priv *spi_drv, void **ptr);

static void spi_cnss_wakeup_sequence(struct spi_cnss_priv *spi_drv);
static int spi_cnss_send_byte_cmd(struct spi_cnss_priv *spi_drv, int cmd);
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
extern int spi_stub_driver_read(u8 *tx_buf, u8 *rx_buf);
extern bool spi_stub_driver_process_incoming_cmd(u8* tx_buf, u8* rx_buf, u8 cmd_len);
struct spi_cnss_priv *test_drv;

void notification_to_schedule_wq(void)
{
	pr_info("%s\n",__func__);
	queue_work(test_drv->bh_work_wq, &test_drv->bh_work);
}
EXPORT_SYMBOL(notification_to_schedule_wq);
#endif

/**
 * FTrace logging
 */
void spi_cnss_trace_log(struct device *dev, const char *fmt, ...)
{
	struct va_format vaf = {
				.fmt = fmt,
	};
	va_list args;
	va_start(args, fmt);
	vaf.va = &args;
	trace_spi_cnss_log_info(dev_name(dev), &vaf);
	va_end(args);
}

/**
 * is_peri_cmd: is cmd is peri cmd
 * @id: proto ind
 * return: true if peri cmd, false otherwise
 */
static bool is_peri_cmd(u8 id)
{
	return (id == PERI_CMD ||
			id == PERI_DATA ||
			id == PERI_EVT);
}

/**
 * get_usr: get user client
 * @proto_ind: protocol indicator
 * return: uwb if UCI req/data
 */
static int get_usr(int proto_ind)
{
	int ret = UWB;
	switch (proto_ind) {
		case UCI_REQ:
		case UCI_DATA:
			ret = UWB;
			break;
		default:
			ret = BT;
	}
	return ret;
}

/**
 * is_rx_data_valid: check if rx data is valid
 * @rx_buf: rx buf read from controller
 * return: true if data is valid, false otherwise
 */
static bool is_rx_data_valid(u8 *rx_buf)
{
	u8 *p_buf = rx_buf;
	int proto_ind = *p_buf;
	switch(proto_ind) {
		case UCI_REQ:
		case UCI_DATA:
		case PERI_CMD:
		case PERI_DATA:
		case PERI_EVT:
			return true;
		default:
			return false;
	}
	return false;
}

/**
 * spi_cnss_notify_data_avail: notify rx available to user space
 * @usr: user-space client specific struct
 * return: void
 */
void spi_cnss_notify_data_avail(struct spi_cnss_user *usr)
{
	struct spi_cnss_priv *spi_drv = NULL;
	if (!usr) {
		pr_err("%s: usr is null\n",__func__);
		return;
	}
	spi_drv = container_of(usr, struct spi_cnss_priv, user[usr->id]);
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
	if (!spi_drv) {
		pr_err("%s: spi drv is null\n",__func__);
		return;
	}
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	atomic_inc(&usr->rx_avail);
	SPI_CNSS_DBG(spi_drv,"%s: rx_avail incremented\n",__func__);
	wake_up_interruptible(&usr->readwq);
	wake_up(&usr->readq);
}

/**
 * spi_cnss_parse_and_enqueue: parse and queue data to FIFO
 * @spi_drv: pointer to main spi_cnss struct
 * @rx_buf: pointer to rx buffer
 * @data_len: rx data length
 * return: void
 */

static void spi_cnss_parse_and_enqueue(struct spi_cnss_priv *spi_drv, u8 *rx_buf, int data_len)
{
	struct spi_client_request cp;
	struct spi_cnss_user *usr;
	int ret = 0;
	u8 index = FREAD_TX_SIZE;
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	index = 0;
#endif
	cp.proto_ind = rx_buf[index];
	ret++;
	if (is_peri_cmd(cp.proto_ind)) {
		cp.end_point = rx_buf[index+1];
		ret++;
	} else {
		cp.end_point = get_usr(cp.proto_ind);
	}

	cp.flow_id = (index+ret);//repurposed for offset.
	cp.data_len = (data_len - ret);
	cp.data_buf = spi_cnss_kzalloc(spi_drv, data_len+index);
	if (cp.data_buf) {
		memcpy((u8*)cp.data_buf, rx_buf, data_len+index);
	} else {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError failed to alloc memory\n",__func__);
		return;
	}
	usr = &spi_drv->user[cp.end_point];
	ret = kfifo_put(&usr->user_fifo, cp);
	if (ret == 0/*kfifo_len(&usr.user_fifo) == MAX_CLIENT_PKTS*/) {
		SPI_CNSS_ERR(spi_drv,"%s: fifo is full\n",__func__);
		usr->fifo_full = true;
		return;
	}
	SPI_CNSS_DBG(spi_drv, "%s: notify data available\n",__func__);
	spi_cnss_notify_data_avail(usr);
}

/**
 * spi_cnss_multi_transfer: submit multi xfers to SPI core
 * @spi_drv: pointer to main spi_cnss struct
 * @xfers: pointer to spi transfer
 * @num_xfers: number of transfers to be queued
 * return: zero if spi_sync_transfer is success
 */
#ifndef CONFIG_SPI_LOOPBACK_ENABLED
static int spi_cnss_multi_transfer(struct spi_cnss_priv *spi_drv, struct spi_transfer *xfers, int num_xfers)
{
	int ret = 0;
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
	mutex_lock(&spi_drv->xfer_lock);
	SPI_CNSS_DBG(spi_drv, "%s acquired lock \n",__func__);
	ret = spi_sync_transfer(spi_drv->spi, xfers,num_xfers);
	mutex_unlock(&spi_drv->xfer_lock);
	SPI_CNSS_DBG(spi_drv, "%s exiting \n",__func__);
	return ret;
}
#endif

/**
 * spi_cnss_single_transfer: Submit single spi msg to SPI core
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero if spi_sync_transfer is success
 */
static int spi_cnss_single_transfer(struct spi_cnss_priv *spi_drv)
{
	int ret = 0;
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
	mutex_lock(&spi_drv->xfer_lock);
	SPI_CNSS_DBG(spi_drv, "%s acquire lock\n",__func__);
	ret = spi_sync(spi_drv->spi, &spi_drv->spi_msg1);
	mutex_unlock(&spi_drv->xfer_lock);
	SPI_CNSS_DBG(spi_drv, "%s: exit\n",__func__);
	return ret;
}

/**
 * spi_cnss_prepare_data_log: Log data dump
 * @spi_drv: pointer to main spi_cnss struct
 * @prefix: prefix to use in log
 * @str: string to dump in log
 * @total: payload size
 * @offset: start of str position
 * @size: total size str
 * return: void
 */
void spi_cnss_prepare_data_log(struct spi_cnss_priv *spi_drv, char *prefix,
										char *str, int total, int offset, int size)
{
	char buf[DATA_BYTES_PER_LINE * 5];
	char data[DATA_BYTES_PER_LINE * 5];
	int len = min(size, DATA_BYTES_PER_LINE);

	hex_dump_to_buffer(str, len, DATA_BYTES_PER_LINE, 1, buf, sizeof(buf), false);
	scnprintf(data, sizeof(data), "%s[%d-%d of %d]: %s", prefix, offset + 1,
				offset + len, total, buf);
	SPI_CNSS_DBG(spi_drv, "%s: %s\n", __func__, data);
}

/**
 * spi_cnss_kzalloc: alloc kernel memory
 * @spi_drv: pointer to main spi_cnss struct
 * @size: size of memory to allocate
 * return: pointer to allocated memory if success, null otherwise
 */
void* spi_cnss_kzalloc(struct spi_cnss_priv *spi_drv, int size)
{
	void *ptr = NULL;
	mutex_lock(&spi_drv->mem_lock);
	SPI_CNSS_DBG(spi_drv,"%s: requesting size = %d\n",__func__, size);
	ptr = kzalloc(size, GFP_ATOMIC);
	if (ptr) {
		atomic_inc(&spi_drv->spi_alloc_cnt);
		SPI_CNSS_DBG(spi_drv,"%s: Allocated Total buffers now : %d Current pointer:%p allocated size:%d\n", __func__, atomic_read(&spi_drv->spi_alloc_cnt), ptr,size);
	}
	else {
		SPI_CNSS_ERR(spi_drv,"%s: Mem alloc failed \n", __func__);
	}
	mutex_unlock(&spi_drv->mem_lock);
	return ptr;
}

/**
 * spi_cnss_kfree: free allocated memory
 * @spi_drv: pointer to main spi_cnss struct
 * @ptr: pointer to allocated memory
 * return: void
 */
void spi_cnss_kfree(struct spi_cnss_priv *spi_drv, void **ptr)
{
    mutex_lock(&spi_drv->mem_lock);
    if (*ptr) {
        atomic_dec(&spi_drv->spi_alloc_cnt);
        SPI_CNSS_DBG(spi_drv, "%s: Freeing mem %p Remained allocated buffers:%d\n", __func__, *ptr, atomic_read(&spi_drv->spi_alloc_cnt));
        kfree(*ptr);
        *ptr = NULL;
    } else {
        SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Ptr is null. Nothing to free \n", __func__);
    }
    mutex_unlock(&spi_drv->mem_lock);
}

/**
 * spi_cnss_nop_cmd: write NOP to controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: spi write status
 */
static int spi_cnss_nop_cmd(struct spi_cnss_priv *spi_drv)
{
	int ret = -1;
	SPI_CNSS_DBG(spi_drv, "%s: writing NOP cmd\n",__func__);
	if (spi_drv->mem_mngr.nop_cmd_buf == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
		return -ENOMEM;
	}
	ret = spi_write(spi_drv->spi, spi_drv->mem_mngr.nop_cmd_buf, NOP_CMD_LEN);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi write failed = %d\n",__func__, ret);
	}
	return ret;
}

/**
 * spi_cnss_soft_reset: write NOP to controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: spi write status
 */
static int spi_cnss_soft_reset(struct spi_cnss_priv *spi_drv)
{
	struct spi_transfer *xfer;
	int ret;
	u32 addr,val = 0;
	SPI_CNSS_DBG(spi_drv,"%s",__func__);

	u8* soft_reset_buf = spi_cnss_kzalloc(spi_drv, IRQ_WRITE_SIZE);

	if (!soft_reset_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError mem alloc failed\n",__func__);
		return -ENOMEM;
	}
	soft_reset_buf[0] = SPI_WRITE_OPCODE;
	addr = cpu_to_be32(spi_drv->client.HCINT_BASE_ADDR);
	memcpy(&soft_reset_buf[1], &addr, ADDR_BYTES);
	val |= SOFT_RESET_IRQ;
	memcpy(&soft_reset_buf[5], &val, sizeof(u32));

	xfer = &spi_drv->spi_xfer1;
	spi_cnss_reinit_xfer(xfer, 1);

	xfer->tx_buf = &soft_reset_buf;
	xfer->len = IRQ_WRITE_SIZE;
	xfer->speed_hz = spi_drv->spi_max_freq;
	ret = spi_cnss_single_transfer(spi_drv);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv,"%s: write failed =  %d\n",__func__, ret);
		goto end;
	}
	ret = spi_cnss_nop_cmd(spi_drv);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv,"%s: NOP failed = %d\n",__func__, ret);
	}
end:
	spi_cnss_kfree(spi_drv, (void **)&soft_reset_buf);
	return ret;
}

static void spi_cnss_reinit_xfer(struct spi_transfer* xfer, int size)
{
	int i;
	for(i = 0; i < size; i++) {
		xfer[i].tx_buf = NULL;
		xfer[i].rx_buf = NULL;
		xfer[i].len = 0;
	}
}

/**
 * prepare_notifiers: prepare HLEN and HCIRQ spi msg
 * @spi_drv: pointer to main spi_cnss struct
 * @xfer: pointer to spi transfer
 * @index: index to spi transfer array to be populated
 * @data_len: payload length
 * return: zero on success, non-zero otherwise
 */
#ifndef CONFIG_SPI_LOOPBACK_ENABLED
static int prepare_notifiers(struct spi_cnss_priv *spi_drv,
						struct spi_transfer *xfer, int index, int data_len)
{
	//prepare HLEN
	u8 cmnd = SPI_WRITE_OPCODE;
	u8 *txbuf, *txbuf1;
	u32 val = HOST_IRQ;
	int offset;
	u32 addr;//, len;
	if (spi_drv->mem_mngr.notifier_one == NULL ||
		spi_drv->mem_mngr.notifier_two == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null!",__func__);
		return -ENOMEM;
	}
	txbuf = spi_drv->mem_mngr.notifier_one;
	txbuf1 = spi_drv->mem_mngr.notifier_two;
	memcpy(txbuf, &cmnd, sizeof(cmnd));
	offset = sizeof(cmnd);
	addr = spi_drv->client.HBUF_LEN_ADDR;
	addr = cpu_to_be32(addr);
	memcpy(txbuf + offset, &addr, ADDR_BYTES);
	offset += ADDR_BYTES;
	SPI_CNSS_DBG(spi_drv, "%s: hlen = %d\n",__func__, data_len);
	memcpy(txbuf + offset, &data_len, sizeof(u32));
	xfer[index].tx_buf = txbuf;
	xfer[index].speed_hz = spi_drv->spi_max_freq;
	xfer[index].cs_change = 1;
	xfer[index].len = NOTIFIER_WRITE_SIZE;
	index++;
	offset = 0;

	xfer[index].len = IRQ_WRITE_SIZE;

	xfer[index].tx_buf = txbuf1;
	xfer[index].speed_hz = spi_drv->spi_max_freq;
	memcpy(txbuf1, &cmnd, sizeof(cmnd));
	offset = sizeof(cmnd);
	addr = spi_drv->client.HCINT_BASE_ADDR;
	addr = cpu_to_be32(addr);
	memcpy(txbuf1 + offset, &addr, ADDR_BYTES);
	offset += ADDR_BYTES;
	memcpy(txbuf1 + offset,&val, 4);
	return 0;
}

/**
 * spi_cnss_build_transfer: build spi tx msg
 * @spi_drv: pointer to main spi_cnss struct
 * @tx_buf: pointer to tx buffer
 * @rx_buf: pointer to rx buffer
 * @payload_len: payload length
 * @len: total length of tx buffer
 * return: zero on success, non-zero otherwise
 */
static int spi_cnss_build_transfer(struct spi_cnss_priv *spi_drv,
			u8* tx_buf, u8* rx_buf, int payload_len, int len, bool update_hlen)
{
	struct spi_transfer xfer[3] = {};
	//struct spi_device *spi;
	struct spi_cnss_user *user = NULL;
	int ret = 0, offset = 0, num_xfer = 0;
	mutex_lock(&spi_drv->sleep_lock);
	SPI_CNSS_DBG(spi_drv,"%s: len = %d, payload = %d\n",__func__, len, payload_len);
	xfer[offset].tx_buf = tx_buf;
	xfer[offset].speed_hz = spi_drv->spi_max_freq;
	xfer[offset].cs_change = 1;
	if (rx_buf) {
		xfer[offset].rx_buf = rx_buf;
	}
	xfer[offset].len = len;
	offset++;
	if (spi_drv->write_pending == true) {
		user = &spi_drv->user[user_id];
		SPI_CNSS_DBG(spi_drv,"%s: check sync_wait completion user_id = %d\n",__func__, user_id);
		if (completion_done(&user->sync_wait)) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError sync_wait completed abrubtly!",__func__);
			BUG();
		}
	}
	ret = prepare_notifiers(spi_drv, xfer, offset, payload_len);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError failed prepare write notifier",__func__);
		mutex_unlock(&spi_drv->sleep_lock);
		return ret;
	}
	num_xfer = 3;

	spi_drv->client.HBUF_LEN = update_hlen?payload_len:0;
	SPI_CNSS_DBG(spi_drv, "%s: HBUF_LEN = %d, client_state = %d\n",__func__, spi_drv->client.HBUF_LEN, spi_drv->client_state);
#ifdef CONFIG_AGGRESSIVE_SLEEP
	if (spi_drv->client_state == ASLEEP && update_hlen && (!gpio_get_value(spi_drv->gpio))) {
		mod_timer(&spi_drv->client_sleep_timer, jiffies + msecs_to_jiffies(CLIENT_WAKE_TIME_OUT));
		spi_cnss_wakeup_sequence(spi_drv);
		if (spi_drv->client_state == AWAKE) {
			SPI_CNSS_DBG(spi_drv, "%s:reset mod timer to 100msec\n",__func__);
			mod_timer(&spi_drv->client_sleep_timer, jiffies + msecs_to_jiffies(SPI_CLIENT_SLEEP_TIME_MS));
		}
		SPI_CNSS_DBG(spi_drv, "%s: After wakeup sequence client stat = %d\n",__func__, spi_drv->client_state);
		if (spi_drv->client_state == CLIENT_WAKEUP)
			spi_cnss_nop_cmd(spi_drv);
	} else if (update_hlen &&
		(spi_drv->client_state != AWAKE || gpio_get_value(spi_drv->gpio) || spi_drv->context_read_pending)) {
		spi_cnss_nop_cmd(spi_drv);
		SPI_CNSS_DBG(spi_drv, "%s: After nop client stat = %d\n",__func__, spi_drv->client_state);
	}
#endif
	ret = spi_cnss_multi_transfer(spi_drv, xfer, num_xfer);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI transaction failed\n",__func__);
	}
	mutex_unlock(&spi_drv->sleep_lock);
	return ret;
}
#endif

/**
 * spi_cnss_prepare_xfer: prepare multi transfer from host to controller
 * @spi_drv: pointer to main spi_cnss struct
 * @usr_pkt: pointer to spi_cnss_packet
 * @cmd: command type
 * return: zero on success, non-zero otherwise
 */
static int spi_cnss_prepare_xfer(struct spi_cnss_priv *spi_drv,
				struct spi_cnss_packet *usr_pkt, enum cmd_type cmd)
{
	u8 *tx_buf = NULL;
	u8 *rx_buf = NULL;
	u8 cmnd;
	int alloc_size = 0;
	int offset = 0, ret, payload_len;//, len = 0;
	u32 addr;
	if (spi_drv->mem_mngr.tx_payload == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
		return -ENOMEM;
	}
	if (cmd == USER_WRITE) {
		bool peri = false;
		u8 proto_ind = usr_pkt->user_req.proto_ind;
		u8 end_point = usr_pkt->user_req.end_point;
		SPI_CNSS_DBG(spi_drv,"%s: proto_ind = %d; end_point = %d\n",__func__, proto_ind, end_point);
		alloc_size = usr_pkt->user_req.data_len + PROTO_BYTE;
		if (is_peri_cmd(proto_ind)) {
			alloc_size++;
			peri = true;
		}
		payload_len = alloc_size;
		alloc_size += ((alloc_size % DATA_WORD_LEN)?
					DATA_WORD_LEN - (alloc_size % DATA_WORD_LEN) : 0);
		alloc_size += ADDR_BYTES + CMD_SIZE;
		tx_buf = spi_drv->mem_mngr.tx_payload;
		memset(tx_buf, 0, spi_drv->client.HBUF_SIZE);
		cmnd = SPI_WRITE_OPCODE;

		memcpy(tx_buf + offset, &cmnd, sizeof(cmnd));
		offset += sizeof(cmnd);
		addr = cpu_to_be32(spi_drv->client.HBUF_BASE_ADDR);
		memcpy(tx_buf+sizeof(cmnd), &addr, ADDR_BYTES);
		offset += ADDR_BYTES;
		memcpy(tx_buf+offset, &proto_ind, PROTO_BYTE);
		offset += PROTO_BYTE;
		if (peri) {
			memcpy(tx_buf+offset, &usr_pkt->user_req.end_point, PROTO_BYTE);
			offset += PROTO_BYTE;
		}

		memcpy(tx_buf+offset, usr_pkt->user_req.data_buf, usr_pkt->user_req.data_len);
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
		spi_stub_driver_process_incoming_cmd(&tx_buf[5], rx_buf, payload_len);
		ret = 0;
#else
		ret = spi_cnss_build_transfer(spi_drv, tx_buf, rx_buf, payload_len, alloc_size, true);
#endif
		return ret;
	}
	return 0;
}

/**
 * spi_cnss_clear_clen: clear clen after successful read
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
#ifndef CONFIG_SPI_LOOPBACK_ENABLED
static int spi_cnss_clear_clen(struct spi_cnss_priv *spi_drv)
{
	struct spi_transfer xfer[2] = {};
	u8 *txbuf,*txbuf1, ret,offset = 0;
	u32 addr, val = HOST_IRQ;
	u8 cmd = SPI_WRITE_OPCODE;
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	if (spi_drv->mem_mngr.clen_notifier_one == NULL ||
		spi_drv->mem_mngr.clen_notifier_two == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
		return -ENOMEM;
	}
	txbuf = spi_drv->mem_mngr.clen_notifier_one;
	txbuf1 = spi_drv->mem_mngr.clen_notifier_two;
	addr = cpu_to_be32(spi_drv->client.CBUF_LEN_ADDR);
	txbuf[0] = cmd;
	memcpy(txbuf+sizeof(cmd), &addr, ADDR_BYTES);
	txbuf1[0] = cmd;
	offset += sizeof(cmd);
	addr = cpu_to_be32(spi_drv->client.HCINT_BASE_ADDR);
	memcpy(txbuf1+offset, &addr, ADDR_BYTES);
	offset += ADDR_BYTES;
	memcpy(txbuf1+offset, &val, sizeof(u32));
	spi_cnss_reinit_xfer(xfer, 2);
	xfer[0].tx_buf = txbuf;
	xfer[0].len = NOTIFIER_WRITE_SIZE;
	xfer[0].speed_hz = spi_drv->spi_max_freq;
	xfer[0].cs_change = 1;
	xfer[1].tx_buf = txbuf1;
	xfer[1].len = IRQ_WRITE_SIZE;
	xfer[1].speed_hz = spi_drv->spi_max_freq;

	ret = spi_cnss_multi_transfer(spi_drv,xfer, 2);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi xfer failed to clear clen\n",__func__);
	}
	return ret;
}
#endif

#ifdef CONFIG_AGGRESSIVE_SLEEP
static void spi_cnss_process_aggressive_sleep(struct spi_cnss_priv *spi_drv)
{
	//SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	if (!timer_pending(&spi_drv->client_sleep_timer)) {
		if (!spi_drv->context_read_pending && spi_drv->client.CBUF_LEN == 0 &&
			!gpio_get_value(spi_drv->gpio) && spi_drv->client_state == AWAKE && !spi_drv->write_pending) {
			SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
			spi_cnss_send_byte_cmd(spi_drv, SLEEP_CMD_BYTE);
		}
	}

}
#endif
/**
 * spi_cnss_send_byte_cmd: send single byte cmd e.g
 * sleep proto byte to controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
#ifdef CONFIG_SLEEP
static int spi_cnss_send_byte_cmd(struct spi_cnss_priv *spi_drv, int cmd)
{
	u32 addr;
	u8 *txbuf;
	int ret;
	SPI_CNSS_DBG(spi_drv,"%s",__func__);
	if (spi_drv->client_state == ASLEEP) {
		return 1;
	}
#ifdef CONFIG_AGGRESSIVE_SLEEP
	if (!spi_drv->sleep_enabled && (cmd == SLEEP_CMD_BYTE)) {
		SPI_CNSS_DBG(spi_drv,"%s: sleep disabled by app",__func__);
		return -1;
	}
#endif
	if (spi_drv->mem_mngr.single_byte_cmd_buf == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
		return -ENOMEM;
	}
	txbuf = spi_drv->mem_mngr.single_byte_cmd_buf;
	txbuf[0] = SPI_WRITE_OPCODE;
	addr = cpu_to_be32(spi_drv->client.HBUF_BASE_ADDR);
	memcpy(&txbuf[1], &addr, ADDR_BYTES);
	txbuf[CMD_BYTE_OFFSET] = cmd;
	if (cmd == SLEEP_CMD_BYTE) {
		spi_drv->client_state = ASLEEP;
	}
	ret = spi_cnss_build_transfer(spi_drv, txbuf, NULL, 1, IRQ_WRITE_SIZE, false);
	return ret;
}
#endif

/**
 * spi_cnss_read_len:read len from controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
static int spi_cnss_read_len(struct spi_cnss_priv *spi_drv)
{
	struct spi_transfer *xfer;
	u8 ret, index;
	u32 addr;
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	u8 *clen_rx_buf, *clen_tx_buf;

	if (spi_drv->mem_mngr.len_tx_buf == NULL ||
		spi_drv->mem_mngr.len_rx_buf == NULL) {
		ret = -ENOMEM;
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
		goto err;
	}
	memset(spi_drv->mem_mngr.len_tx_buf, 0, FREAD_TX_SIZE + (2* FREAD_RX_SIZE));
	memset(spi_drv->mem_mngr.len_rx_buf, 0, FREAD_TX_SIZE + (2* FREAD_RX_SIZE));
	clen_rx_buf = spi_drv->mem_mngr.len_rx_buf;
	clen_tx_buf =  spi_drv->mem_mngr.len_tx_buf;
	xfer = &spi_drv->spi_xfer1;
	spi_cnss_reinit_xfer(xfer, 1);
	xfer->tx_buf = clen_tx_buf;
	xfer->rx_buf = clen_rx_buf;
	clen_tx_buf[0] = SPI_FREAD_OPCODE;
	addr = spi_drv->client.CBUF_LEN_ADDR;
	addr = cpu_to_be32(addr);
	memcpy(&clen_tx_buf[1], &addr, ADDR_BYTES);
	xfer->len = FREAD_TX_SIZE + (2* FREAD_RX_SIZE);
	xfer->speed_hz = spi_drv->spi_max_freq;
	ret = spi_cnss_single_transfer(spi_drv);
	SPI_CNSS_DBG(spi_drv,"%s: spi xfer returned\n",__func__);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi xfer failed to read hlen\n",__func__);
		goto err;
	}
	index = FREAD_TX_SIZE;
	spi_drv->client.CBUF_LEN = ((clen_rx_buf[index + 3]) << 24 |
								(clen_rx_buf[index + 2]) << 16 |
								(clen_rx_buf[index + 1]) << 8 |
								clen_rx_buf[index]);
	spi_drv->client.HBUF_LEN = ((clen_rx_buf[index + 7]) << 24 |
								(clen_rx_buf[index + 6]) << 16 |
								(clen_rx_buf[index + 5]) << 8 |
								clen_rx_buf[index+4]);
	SPI_CNSS_DBG(spi_drv,"%s: CLEN = %d, HLEN = %d\n",__func__, spi_drv->client.CBUF_LEN, spi_drv->client.HBUF_LEN);
	if ((spi_drv->client.CBUF_LEN >= CONTEXT_BUF_SIZE) ||
		(spi_drv->client.HBUF_LEN >= CONTEXT_BUF_SIZE)) {
		SPI_CNSS_ERR(spi_drv,"%s: Incorrect clen or hlen from controller \n",__func__);
		ret = -EINVAL;
	}
err:
	return ret;
}

/**
 * spi_cnss_clear_irq: clear client to host irq
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
static int spi_cnss_clear_irq(struct spi_cnss_priv *spi_drv)
{
	struct spi_transfer *xfer;
	u8 ret;
	u32 addr;
	bool local_alloc = false;
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);

	if (!spi_drv->client_irq_buf) {
		spi_drv->client_irq_buf = spi_cnss_kzalloc(spi_drv, IRQ_WRITE_SIZE);
		if (!spi_drv->client_irq_buf) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError mem alloc failed\n",__func__);
			return -ENOMEM;
		}
		local_alloc = true;
	}
	memset(spi_drv->client_irq_buf, 0, IRQ_WRITE_SIZE);
	spi_drv->client_irq_buf[0] = SPI_WRITE_OPCODE;
	addr = spi_drv->client.CHINT_BASE_ADDR;
	addr = cpu_to_be32(addr);
	memcpy(&spi_drv->client_irq_buf[1], &addr, ADDR_BYTES);

	xfer = &spi_drv->spi_xfer1;
	spi_cnss_reinit_xfer(xfer, 1);
	xfer->tx_buf = spi_drv->client_irq_buf;
	xfer->len = IRQ_WRITE_SIZE;
	xfer->speed_hz = spi_drv->spi_max_freq;

	ret = spi_cnss_single_transfer(spi_drv);
	SPI_CNSS_DBG(spi_drv,"%s spi xfer returned\n",__func__);
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi xfer failed to clear clen\n",__func__);
	}
	if (local_alloc)
		spi_cnss_kfree(spi_drv, (void **)&spi_drv->client_irq_buf);
	return ret;

}

/**
 * spi_cnss_wakeup_client: wake up controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
int spi_cnss_wakeup_client(struct spi_cnss_priv *spi_drv, int retry)
{
	int i,ret = 0;
	SPI_CNSS_INFO(spi_drv, "%s\n",__func__);
	if (spi_drv->client_init && spi_drv->client_state != ASLEEP) {
		SPI_CNSS_ERR(spi_drv,"%s: client is not asleep, bailing\n",__func__);
		return -EIO;
	}
	spi_drv->client_state = AWAKE_PENDING;
	for (i = 0; i < retry; i++) {
	//Write NOP and wait for interrupt
		SPI_CNSS_DBG(spi_drv, "%s: writing NOP cmd: try = %d",__func__, i);
		reinit_completion(&spi_drv->wake_wait);
		ret = spi_cnss_nop_cmd(spi_drv);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi write failed = %d\n",__func__, ret);
			return ret;
		}
		if (spi_drv->client_state == AWAKE_PENDING) {
			ret = wait_for_completion_timeout(&spi_drv->wake_wait, msecs_to_jiffies(NOP_XFER_TIMEOUT));
		}
		if (ret >= 0 && spi_drv->client_state == AWAKE) {
			SPI_CNSS_DBG(spi_drv, "%s: client awake\n",__func__);
			ret = spi_cnss_nop_cmd(spi_drv);
			break;
		}
		if (!spi_drv->client_init && ((i + 1) % SANITY_CHECK_ITERATION == 0) &&
			spi_drv->client_state != AWAKE) {
			ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_SANITY_REG, SPI_REGISTER_READ);
			if (ret == 0) {
				spi_drv->client_state = AWAKE;
				break;
			}
		}
	}
	if (spi_drv->client_state != AWAKE) {
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_SANITY_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Controller is in bad state or not powered on: %d\n",__func__, ret);
			spi_drv->client_state = ASLEEP;
			ret = -ENODEV;
		}
	}
	return ret;
}

/**
 * __spi_cnss_read_msg: read client data from controller
 * @spi_drv: pointer to main spi_cnss struct
 * return: zero on success, non-zero otherwise
 */
static int __spi_cnss_read_msg(struct spi_cnss_priv *spi_drv)
{
	int i, read, ret = 0;
	u8 cmd = SPI_FREAD_OPCODE;
	u8 index = FREAD_TX_SIZE;
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	u8 length = 0;
#endif
	if (spi_drv == NULL) {
		SPI_CNSS_ERR(spi_drv,"%s: spi_drv is null or released\n",__func__);
		return -EINVAL;
	}
	read = spi_drv->usr_cnt;
	SPI_CNSS_DBG(spi_drv, "%s: active clients = %d\n",__func__, read);
	for (i = 0; i < MAX_DEV; i++) {
		if (spi_drv->user[i].is_active) { /*spi_drv->user[i].fifo_full*/
			spi_drv->user[i].fifo_full =
				(kfifo_len(&spi_drv->user[i].user_fifo) == MAX_CLIENT_PKTS);
			if (spi_drv->user[i].fifo_full) read--;
		}
	}
	SPI_CNSS_DBG(spi_drv, "%s: active clients = %d\n",__func__, read);
	if (read > 0) {
		//fifo available to read
		u8 *tx_buf, *rx_buf;
		struct spi_transfer *xfer;
		u32 addr;
		if (spi_drv->mem_mngr.rx_cmd_buf == NULL ||
			spi_drv->mem_mngr.rx_payload == NULL) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
			return -ENOMEM;
		}
		rx_buf = spi_drv->mem_mngr.rx_payload;
		memset(rx_buf, 0, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
		tx_buf = spi_drv->mem_mngr.rx_cmd_buf;
		memset(tx_buf, 0, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
		memcpy(tx_buf, &cmd, sizeof(cmd));
		addr = cpu_to_be32(spi_drv->client.CBUF_BASE_ADDR);
		memcpy(tx_buf+sizeof(cmd), &addr, ADDR_BYTES);
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
		length = spi_stub_driver_read(tx_buf, rx_buf);
		SPI_CNSS_INFO(spi_drv,"%s: read complete; length = %d\n", __func__, length);
		goto loop_back;
#endif

		xfer = &spi_drv->spi_xfer1;
		spi_cnss_reinit_xfer(xfer, 1);
		xfer->tx_buf = tx_buf;
		xfer->rx_buf = rx_buf;
		xfer->speed_hz = spi_drv->spi_max_freq;
		xfer->len = FREAD_TX_SIZE + spi_drv->client.CBUF_LEN;
		//ret = spi_sync(spi_drv->spi, &spi_drv->spi_msg1);
		ret = spi_cnss_single_transfer(spi_drv);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv,"%s: spi transaction failed\n",__func__);
			return ret;
		}
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
loop_back:
		index = 0;
#endif
		SPI_CNSS_INFO(spi_drv,"%s: data valid, 1st = %x, 2nd = %x\n", __func__, rx_buf[index], rx_buf[index+1]);
		if(is_rx_data_valid(&rx_buf[index])) {
			int usr;
			if (is_peri_cmd(rx_buf[index])) {
				usr = rx_buf[index + 1];
			} else {
				usr = get_usr(rx_buf[index]);
			}
			SPI_CNSS_INFO(spi_drv,"%s: data valid, usr = %d\n", __func__, usr);
			if (spi_drv->user[usr].fifo_full) {
				SPI_CNSS_INFO(spi_drv,"%s: host buffer full, dont clear CBUF LEN\n", __func__);
				spi_drv->user[usr].read_pending = true;
			} else {
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
				spi_cnss_parse_and_enqueue(spi_drv, rx_buf, length);
				spi_drv->client.CBUF_LEN = 0;
#else
				spi_cnss_parse_and_enqueue(spi_drv, rx_buf, spi_drv->client.CBUF_LEN);
				ret = spi_cnss_clear_clen(spi_drv);
				if(ret < 0) {
					SPI_CNSS_ERR(spi_drv,"%s: spi xfer to clear clen failed\n",__func__);
				} else {
					spi_drv->client.CBUF_LEN = 0;
				}
#endif
				spi_drv->read_pending =  false;
			}
		} else {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError invalid data\n",__func__);
			spi_drv->read_pending = false;
		}
	}else {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError No active client, bailing\n",__func__);
		spi_drv->read_pending = false;
	}
	SPI_CNSS_DBG(spi_drv,"%s: return\n",__func__);
	return ret;
}

/**
 * spi_cnss_read_msg: kthread work function reader thread
 * @work: pointer to kthread work struct contained in spi_cnss driver struct
 */
static void spi_cnss_read_msg(struct kthread_work *work)
{
	struct spi_cnss_priv *spi_drv = container_of(work, struct spi_cnss_priv, read_msg);
	int ret = 0;
	SPI_CNSS_DBG(spi_drv, "%s: Enter\n",__func__);
	if (!spi_drv->usr_cnt) {
		SPI_CNSS_ERR(spi_drv,"%s: no user, clear CLEN\n",__func__);
		if (spi_drv->client.CBUF_LEN > 0) {
			spi_cnss_clear_clen(spi_drv);
			return;
		}
	}
	mutex_lock(&spi_drv->read_lock);
	if (spi_drv->client.CBUF_LEN > 0) {
		spi_drv->read_pending = true;
		ret = __spi_cnss_read_msg(spi_drv);
		SPI_CNSS_INFO(spi_drv, "%s:read complete\n",__func__);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError failed\n",__func__);
		}
	} else {
		SPI_CNSS_DBG(spi_drv, "%s: CBUF LEN is cleared, ignore read request\n",__func__);
	}
	mutex_unlock(&spi_drv->read_lock);
#ifdef CONFIG_AGGRESSIVE_SLEEP
	if (spi_drv->sleep_enabled) {
		usleep_range(2000, 2100);
		spi_cnss_process_aggressive_sleep(spi_drv);

	}
#endif
	SPI_CNSS_DBG(spi_drv, "%s: Exit\n",__func__);
}

/**
 * __spi_cnss_send_msg: process tx queue and send to controller
 * @spi_drv: pointer to main spi_cnss struct
 * @usr: pointer to user request
 * return: zero if success, non-zer otherwise
 */
static int __spi_cnss_send_msg(struct spi_cnss_priv *spi_drv, struct spi_cnss_user **usr)
{
	int ret = 0;
	int len = 0;
	struct spi_cnss_packet *user_pkt,*user_pkt_temp;
	//struct client_info client = spi_drv->client;

	if (list_empty(&spi_drv->tx_list)) {
		SPI_CNSS_ERR(spi_drv,"%s: no tx pending\n",__func__);
		return -ENODATA;

	}
	mutex_lock(&spi_drv->read_lock);
	len = spi_drv->client.HBUF_LEN;
	mutex_unlock(&spi_drv->read_lock);
#ifdef CONFIG_AGGRESSIVE_SLEEP
	mod_timer(&spi_drv->client_sleep_timer, jiffies + msecs_to_jiffies(SPI_CLIENT_SLEEP_TIME_MS));
#endif
	if (len != 0) {
		unsigned long timeout = 0, xfer_timeout = 0;
		reinit_completion(&spi_drv->buff_wait);
		xfer_timeout = msecs_to_jiffies(SPI_IRQ_TIMEOUT);
		SPI_CNSS_DBG(spi_drv,"%s: waiting for oob\n",__func__);
		spi_drv->wait_to_notify = true;
		timeout = wait_for_completion_interruptible_timeout(&spi_drv->buff_wait, xfer_timeout);
		spi_drv->wait_to_notify = false;
		if (timeout <= 0 && spi_drv->client.HBUF_LEN != 0) {
			SPI_CNSS_ERR(spi_drv,"%s: couldnt get buffer free in time\n",__func__);
			list_for_each_entry_safe(user_pkt, user_pkt_temp, &spi_drv->tx_list, list) {
				*usr = &spi_drv->user[user_pkt->id];
			}
			return -EBUSY;
		}
	}
	mutex_lock(&spi_drv->irq_lock);
	len = spi_drv->client.HBUF_LEN;
	SPI_CNSS_DBG(spi_drv,"%s: Read len again %d\n",__func__, len);
	if (len == 0 && !spi_drv->write_pending) {
		list_for_each_entry_safe(user_pkt, user_pkt_temp, &spi_drv->tx_list, list) {
			SPI_CNSS_DBG(spi_drv,"%s user id = %d\n",__func__, user_pkt->id);
			spi_drv->write_pending = true;
			user_id = user_pkt->id;
			ret = spi_cnss_prepare_xfer(spi_drv, user_pkt, USER_WRITE);
			spi_drv->write_pending = false;
			*usr = &spi_drv->user[user_id];
		}
	} else {
		SPI_CNSS_ERR(spi_drv,"%s: host buffer not available or write pending, wait for oob\n",__func__);
		ret = -EBUSY;
	}
	mutex_unlock(&spi_drv->irq_lock);
	return ret;
}

/**
 * spi_cnss_send_msg: kthread work function write thread
 * @work: pointer to kthread work struct
 */
static void spi_cnss_send_msg(struct kthread_work *work)
{
	struct spi_cnss_priv *spi_drv = container_of(work, struct spi_cnss_priv, send_msg);
	int ret = 0;
	struct spi_cnss_user *usr = NULL;
	SPI_CNSS_DBG(spi_drv, "%s: Enter \n",__func__);
	if (spi_drv == NULL || !spi_drv->client_init) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi_drv is null or released\n",__func__);
		return;
	}
	ret = __spi_cnss_send_msg(spi_drv, &usr);
	if (ret == 0) {
		SPI_CNSS_DBG(spi_drv, "%s: Sync wait completed\n",__func__);
	} else if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError failed \n",__func__);
		atomic_set(&spi_drv->write_err_code, ret);
	}
	complete(&usr->sync_wait);
	SPI_CNSS_DBG(spi_drv, "%s: Exit\n",__func__);
}

static void spi_cnss_free_allocated_memory(struct spi_cnss_priv *spi_drv)
{
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.len_rx_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.len_tx_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.nop_cmd_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.notifier_one);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.notifier_two);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.register_rx_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.register_tx_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.rx_cmd_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.rx_payload);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.single_byte_cmd_buf);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.tx_payload);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.clen_notifier_one);
	spi_cnss_kfree(spi_drv, (void **)&spi_drv->mem_mngr.clen_notifier_two);
}

static int spi_cnss_allocate_memory(struct spi_cnss_priv *spi_drv)
{
	int ret = 0;
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
	spi_drv->mem_mngr.tx_payload = spi_cnss_kzalloc(spi_drv, CONTEXT_BUF_SIZE);
	if (!spi_drv->mem_mngr.tx_payload) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrortx_payload failed\n",__func__);
		ret = -ENOMEM;
		goto err;
	}
	spi_drv->mem_mngr.rx_payload = spi_cnss_kzalloc(spi_drv, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
	if (!spi_drv->mem_mngr.rx_payload) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorrx_payload failed\n",__func__);
		ret = -ENOMEM;
		goto err;
	}

	spi_drv->mem_mngr.notifier_one = spi_cnss_kzalloc(spi_drv, NOTIFIER_WRITE_SIZE);
	if (!spi_drv->mem_mngr.notifier_one) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrornotifier_one failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.notifier_two = spi_cnss_kzalloc(spi_drv, NOTIFIER_WRITE_SIZE);
	if (!spi_drv->mem_mngr.notifier_two) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrornotifier_two failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.nop_cmd_buf = spi_cnss_kzalloc(spi_drv, NOP_CMD_LEN);
	if (!spi_drv->mem_mngr.nop_cmd_buf) {;
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrornop_cmd_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.single_byte_cmd_buf = spi_cnss_kzalloc(spi_drv, NOTIFIER_WRITE_SIZE);
	if (!spi_drv->mem_mngr.single_byte_cmd_buf) {
		SPI_CNSS_DBG(spi_drv, "%s:single_byte_cmd_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.rx_cmd_buf = spi_cnss_kzalloc(spi_drv, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
	if (!spi_drv->mem_mngr.rx_cmd_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorrx_cmd_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.len_tx_buf = spi_cnss_kzalloc(spi_drv,(FREAD_TX_SIZE + (2* FREAD_RX_SIZE)));
	if (!spi_drv->mem_mngr.len_tx_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorlen_tx_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.len_rx_buf = spi_cnss_kzalloc(spi_drv, (FREAD_TX_SIZE + (2* FREAD_RX_SIZE)));
	if (!spi_drv->mem_mngr.len_rx_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorlen_rx_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}

	spi_drv->mem_mngr.register_tx_buf = spi_cnss_kzalloc(spi_drv, REG_TX_SIZE);
	if (!spi_drv->mem_mngr.register_tx_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorregister_tx_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;
	}
	spi_drv->mem_mngr.register_rx_buf = spi_cnss_kzalloc(spi_drv, REG_RX_SIZE);
	if (!spi_drv->mem_mngr.register_rx_buf) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorregister_rx_buf failed\n",__func__);
		ret = -ENOMEM;
		goto err;
	}
	spi_drv->mem_mngr.clen_notifier_one = spi_cnss_kzalloc(spi_drv, NOTIFIER_WRITE_SIZE);
	if (!spi_drv->mem_mngr.clen_notifier_one) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorclen_notifier_one failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}
	spi_drv->mem_mngr.clen_notifier_two = spi_cnss_kzalloc(spi_drv, NOTIFIER_WRITE_SIZE);
	if (!spi_drv->mem_mngr.clen_notifier_two) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssErrorclen_notifer_two failed\n",__func__);
		ret = -ENOMEM;
		goto err;

	}
err:
	if (ret < 0) {
		spi_cnss_free_allocated_memory(spi_drv);
	}
	return ret;
}

/**
 * spi_cnss_read_context_info: read context info
 * @spi_drv: pointer to main spi_cnss struct
 * @is_irq: false when this function is invoked during controller init
 *          true when IRQ is triggered
 * return: zero if success, non-zer otherwise
 */
static int spi_cnss_read_context_info(struct spi_cnss_priv *spi_drv, bool is_irq)
{
	int ret = 0;
#ifndef CONFIG_SPI_LOOPBACK_ENABLED
	struct spi_transfer *xfer;
#endif
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	if (is_irq) {
		mutex_lock(&spi_drv->read_lock);
		if (spi_drv->client_state == CLIENT_WAKEUP ||
			spi_drv->client_state == ASLEEP) {
			spi_drv->client_state = AWAKE;
			spi_cnss_nop_cmd(spi_drv);
		}
		spi_cnss_clear_irq(spi_drv);
		ret = spi_cnss_read_len(spi_drv);
		if ((ret >= 0) && spi_drv->state_transition &&
			gpio_get_value(spi_drv->gpio)) {
			SPI_CNSS_ERR(spi_drv,"%s: IRQ failed to clear after wakeup, retrying",__func__);
			spi_cnss_clear_irq(spi_drv);//if Peri waking up from sleep misses first clear irq
			ret = spi_cnss_read_len(spi_drv);
			if (ret < 0) {
				SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError read len failed\n",__func__);
				return ret;
			}
			spi_drv->state_transition = false;
		}
		//spi_cnss_read_clen(spi_drv);
		//spi_cnss_read_hlen(spi_drv);
		mutex_unlock(&spi_drv->read_lock);
	} else {
		u8 *txbuf,*rxbuf, cmd = 0;
		u32 addr = 0;
		u8 index =0;
		if (spi_drv->mem_mngr.rx_cmd_buf == NULL ||
			spi_drv->mem_mngr.rx_payload == NULL) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null\n",__func__);
			return -ENOMEM;
		}
		txbuf = spi_drv->mem_mngr.rx_cmd_buf;
		rxbuf = spi_drv->mem_mngr.rx_payload;
		memset(txbuf, 0, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
		memset(rxbuf, 0, CONTEXT_BUF_SIZE + FREAD_TX_SIZE);
		cmd |= SPI_FREAD_OPCODE;
		memcpy(txbuf, &cmd, sizeof(u8));
		addr |= SPI_CONTEXT_INFO_BASE;
		memcpy(txbuf+sizeof(cmd), &addr, ADDR_BYTES);
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
		spi_stub_driver_process_incoming_cmd(txbuf, rxbuf, 9);
#else
		xfer = &spi_drv->spi_xfer1;
		spi_cnss_reinit_xfer(xfer, 1);
		xfer->tx_buf = txbuf;
		xfer->rx_buf = rxbuf;
		xfer->len = FREAD_TX_SIZE + CONTEXT_INFO_READ_SIZE;
		xfer->speed_hz = spi_drv->spi_max_freq;
		SPI_CNSS_DBG(spi_drv, "%s: calling single transfer\n",__func__);
		ret = spi_cnss_single_transfer(spi_drv);
#endif
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError context info read failed\n",__func__);
			spi_cnss_kfree(spi_drv, (void **)&txbuf);
			spi_cnss_kfree(spi_drv, (void **)&rxbuf);
			return ret;
		}
		SPI_CNSS_DBG(spi_drv, "%s: calling single transfer\n",__func__);
		if (rxbuf != NULL) {
			index = FREAD_TX_SIZE;
			index++;//skip protocol version
			spi_drv->client.buffer_pointer = 0;
			spi_drv->client.buffer_pointer = rxbuf[index+2] << 16 | rxbuf[index+1] << 8 | rxbuf[index];
			index += 3;
			spi_drv->client.HBUF_SIZE = (rxbuf[index+3] << 24 | rxbuf[index+2] << 16 | rxbuf[index+1] << 8 | rxbuf[index]);
			index +=4;
			spi_drv->client.CBUF_SIZE = (rxbuf[index+3] << 24 | rxbuf[index+2] << 16 | rxbuf[index+1] << 8 | rxbuf[index]);
			index += 4;
			spi_drv->client.HCINT_BASE_ADDR = (rxbuf[index+3] << 24 | rxbuf[index+2] << 16 | rxbuf[index+1] << 8 | rxbuf[index]);
			index += 4;
			spi_drv->client.CHINT_BASE_ADDR = (rxbuf[index+3] << 24 | rxbuf[index+2] << 16 | rxbuf[index+1] << 8 | rxbuf[index]);

			spi_drv->client.CBUF_LEN_ADDR = spi_drv->client.buffer_pointer;
			spi_drv->client.HBUF_LEN_ADDR = spi_drv->client.buffer_pointer + sizeof(u32);
			spi_drv->client.HBUF_BASE_ADDR = spi_drv->client.HBUF_LEN_ADDR + sizeof(u32);
			spi_drv->client.CBUF_BASE_ADDR = spi_drv->client.HBUF_BASE_ADDR + spi_drv->client.HBUF_SIZE;

			SPI_CNSS_DBG(spi_drv,"%s: HBUF_SIZE = %x, CBUF_SIZE = %x\n", __func__, spi_drv->client.HBUF_SIZE, spi_drv->client.CBUF_SIZE);
			SPI_CNSS_DBG(spi_drv,"%s: buffer_pointer = %x, CBUF_LEN_ADDR = %x, HBUF_LEN_ADDR = %x\n",
				__func__, spi_drv->client.buffer_pointer, spi_drv->client.CBUF_LEN_ADDR,spi_drv->client.HBUF_LEN_ADDR);
			SPI_CNSS_DBG(spi_drv,"%s: HCINT_BASE_ADDR = %x, CHINT_BASE_ADDR = %x, HBUF_BASE_ADDR = %x, CBUF_BASE_ADDR = %x\n",
				__func__, spi_drv->client.HCINT_BASE_ADDR, spi_drv->client.CHINT_BASE_ADDR,spi_drv->client.HBUF_BASE_ADDR, spi_drv->client.CBUF_BASE_ADDR);

#ifndef CONFIG_SPI_LOOPBACK_ENABLED
			ret = spi_cnss_read_context_info(spi_drv, true);
#endif
			spi_drv->client_init = true;
			ret = 0;
		}
	}
	return ret;
}
#ifdef CONFIG_AGGRESSIVE_SLEEP
void spi_cnss_sleep_timeout_handler(struct timer_list *t)
{
	struct spi_cnss_priv *spi_drv = from_timer(spi_drv, t, client_sleep_timer);
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);
	if (spi_drv->context_read_pending || spi_drv->read_pending || gpio_get_value(spi_drv->gpio) ||
		spi_drv->client_state == ASLEEP || spi_drv->write_pending ||
		timer_pending(&spi_drv->client_sleep_timer) || spi_drv->client_state == RESET) {
		SPI_CNSS_DBG(spi_drv, "%s: read pending or client asleep or resetting.Ignore sleep cmd\n",__func__);
		return;
	}


	queue_work(spi_drv->sleep_wq, &spi_drv->sleep_work);
}

static void spi_cnss_handle_sleep(struct work_struct *work)
{
	struct spi_cnss_priv *spi_drv = container_of(work, struct spi_cnss_priv, sleep_work);

	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	mutex_lock(&spi_drv->state_lock);
	if (spi_drv->context_read_pending || spi_drv->read_pending || gpio_get_value(spi_drv->gpio) ||
		spi_drv->client_state == ASLEEP || spi_drv->write_pending ||
		timer_pending(&spi_drv->client_sleep_timer) || spi_drv->client_state == RESET) {
		SPI_CNSS_DBG(spi_drv, "%s: read pending or client asleep or resetting.Ignore sleep cmd\n",__func__);
		goto unlock;
	}
	spi_cnss_send_byte_cmd(spi_drv, SLEEP_CMD_BYTE);
unlock:
	mutex_unlock(&spi_drv->state_lock);
}
#endif
void spi_cnss_wakeup_sequence(struct spi_cnss_priv *spi_drv)
{
	int ret = 0;
	ret = spi_cnss_wakeup_client(spi_drv, NUM_OF_TRIALS_DURING_TRANS);
	if (ret < 0 && (spi_drv->client_state == ASLEEP)) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Failed to wakeup client\n",__func__);
		return;
	}
	if (ret != -1) {
		spi_cnss_read_context_info(spi_drv, true);
	}
}

/**
 * spi_cnss_handle_work: BH processing of IRQ
 * @work: pointer bottom half work struct
 * return: void
 */
static void spi_cnss_handle_work(struct work_struct *work)
{
	struct spi_cnss_priv *spi_drv = container_of(work, struct spi_cnss_priv, bh_work);
	int ret;
	bool none_scheduled = true;

	if (atomic_cmpxchg(&spi_drv->check_resume_wait, TRUE, FALSE) == TRUE) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError In suspend state. Wait for resume\n", __func__);
		ret = wait_for_completion_timeout(&spi_drv->resume_wait, msecs_to_jiffies(CLIENT_WAKE_TIME_OUT));
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError System resume did not happen:\n", __func__);
			return;
		} else if (ret == 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError System resume didn't happen - timeout:\n", __func__);
		} else {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError System resumed\n", __func__);
		}
	}
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	spi_drv->client.CBUF_LEN = 275;
	kthread_queue_work(spi_drv->kreader, &spi_drv->read_msg);
	return;
#endif
	mutex_lock(&spi_drv->irq_lock);
	spi_drv->context_read_pending = true;
	ret = spi_cnss_read_context_info(spi_drv, true);
	spi_drv->context_read_pending = false;
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv,"%s: failed to read context info\n",__func__);
		mutex_unlock(&spi_drv->irq_lock);
		return;
	}
	if (!spi_drv->client_init) {
		SPI_CNSS_ERR(spi_drv,"%s: client is turned off\n",__func__);
		mutex_unlock(&spi_drv->irq_lock);
		return;
	}
	//check HLEN/CLEN
	if (spi_drv->client.CBUF_LEN > 0 && !spi_drv->read_pending) {
		SPI_CNSS_INFO(spi_drv, "%s: data available to read\n",__func__);
		none_scheduled = false;
		kthread_queue_work(spi_drv->kreader, &spi_drv->read_msg);
	}
	else if (spi_drv->read_pending) {
		SPI_CNSS_INFO(spi_drv,"%s: read pending, read not scheduled\n",__func__);
	}
	if (spi_drv->client.HBUF_LEN == 0 && !spi_drv->write_pending &&
		spi_drv->wait_to_notify) {
		SPI_CNSS_INFO(spi_drv, "%s: schedule write\n",__func__);
		none_scheduled = false;
		//kthread_queue_work(spi_drv->kwriter, &spi_drv->send_msg);
		complete(&spi_drv->buff_wait);
	}
	mutex_unlock(&spi_drv->irq_lock);
#ifdef CONFIG_AGGRESSIVE_SLEEP
	if (none_scheduled && spi_drv->sleep_enabled) {
		usleep_range(2000, 2200);
		SPI_CNSS_DBG(spi_drv, "%s:None scheduled, send sleep\n",__func__);
		spi_cnss_process_aggressive_sleep(spi_drv);
	}
#endif
}

/* spi_cnss_irq: ISR to handle OOB interrupt */
static irqreturn_t spi_cnss_irq(int irq, void *data)
{
	struct spi_cnss_priv *spi_drv = data;
	//Read HLEN/CLEN and then schedule work
	SPI_CNSS_INFO(spi_drv, "%s\n",__func__);
	if (!gpio_get_value(spi_drv->gpio)) {
		SPI_CNSS_INFO(spi_drv, "%s: stale irq.Igonore it \n",__func__);
		return IRQ_HANDLED;
	}
	if (!spi_drv->client_init ||
		spi_drv->client_state == AWAKE_PENDING) {
		SPI_CNSS_INFO(spi_drv, "%s: power on ack\n",__func__);
		spi_drv->client_state = AWAKE;
		spi_drv->state_transition = true;
		complete(&spi_drv->wake_wait);
		return IRQ_HANDLED;
	} else if (spi_drv->client_state != AWAKE) {
		spi_drv->client_state = CLIENT_WAKEUP;
		spi_drv->state_transition = true;
	} else {
		spi_drv->state_transition = false;
	}
	queue_work(spi_drv->bh_work_wq, &spi_drv->bh_work);
	return IRQ_HANDLED;
}
/**
 * spi_cnss_transfer: will schedule write thread
 * @spi_drv: pointer to spi_cnss main struct
 * @usr: pointer to user request struct
 * @len: payload length
 * return: len if success, ETIMEDOUT if tx fails
 */
static int spi_cnss_transfer(struct spi_cnss_priv *spi_drv,
										struct spi_cnss_user *usr, size_t len)
{
	unsigned long timeout = 0, xfer_timeout = 0;

	reinit_completion(&usr->sync_wait);
	kthread_queue_work(spi_drv->kwriter, &spi_drv->send_msg);

	xfer_timeout = msecs_to_jiffies(XFER_TIMEOUT);
	SPI_CNSS_DBG(spi_drv, "%s: waiting on sync_wait\n",__func__);
	timeout = wait_for_completion_timeout(
				&usr->sync_wait, xfer_timeout);
	if (timeout <= 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError err timeout for sync_wait\n",__func__);
		return -ETIMEDOUT;
	}
	SPI_CNSS_DBG(spi_drv, "%s: sync_wait complete with len = %lu\n",__func__, len);
	return len;
}

/**
 * spi_cnss_register_xfer: register read/write
 * @spi_drv: pointer to spi_cnss main struct
 * @reg: register address
 * @opcode: read/write
 * return: zero if success, non-zeor otherwise
 */

static int spi_cnss_register_xfer(struct spi_cnss_priv *spi_drv, u8 reg, u8 opcode)
{
	int ret = 0;
	u8 *tx_buf, *rx_buf;
#ifndef CONFIG_SPI_LOOPBACK_ENABLED
	struct spi_transfer *xfer = NULL;
#endif
	if (spi_drv->mem_mngr.register_tx_buf == NULL ||
		spi_drv->mem_mngr.register_rx_buf == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError buffer is null",__func__);
		return -ENOMEM;
	}
	tx_buf = spi_drv->mem_mngr.register_tx_buf;
	rx_buf = spi_drv->mem_mngr.register_rx_buf;
	memset(tx_buf, 0, REG_TX_SIZE);
	memset(rx_buf, 0, REG_RX_SIZE);
	tx_buf[0] = opcode;
	tx_buf[1] = reg;

	if (reg == SPI_SLAVE_CONFIG_REG) {
		if (opcode == SPI_REGISTER_WRITE) {
			u32 payload = slave_config;
			payload = cpu_to_be32(payload);
			memcpy(&tx_buf[2], &payload, sizeof(payload));
			//xfer->rx_buf = NULL;
		}
	}

#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	spi_stub_driver_process_incoming_cmd(tx_buf,rx_buf, 8);
#else
	xfer = &spi_drv->spi_xfer1;
	spi_cnss_reinit_xfer(xfer, 1);
	xfer->tx_buf = tx_buf;
	if (reg == SPI_SLAVE_CONFIG_REG &&
		opcode == SPI_REGISTER_WRITE) {
		xfer->rx_buf = NULL;
	} else {
		xfer->rx_buf = rx_buf;
	}
	xfer->len = REG_TX_SIZE;
	xfer->speed_hz = spi_drv->spi_max_freq;
	ret = spi_cnss_single_transfer(spi_drv);
#endif
	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError reg rd/wr failed = %d\n",__func__, ret);
	} else {
		u8 index = REGISTER_READ_SIZE;
		int val = (rx_buf[index] << 24 | rx_buf[index+1] << 16 | rx_buf[index+2] << 8 | rx_buf[index+3]);
		SPI_CNSS_DBG(spi_drv, "%s: reg = %x", __func__, reg);
		SPI_CNSS_DBG(spi_drv, "%s: value = %x\n",__func__, val);
		if (reg == SPI_SLAVE_CONFIG_REG && opcode == SPI_REGISTER_READ) {
			val = val & 0xF0FFFFFF;
			slave_config |= val;
		}
		else if (reg == SPI_SLAVE_SANITY_REG ) {
			if (val == 0xdeadbeef) {
				spi_drv->client_state = AWAKE;
				ret = 0;
			} else {
				SPI_CNSS_ERR(spi_drv,"%s: sanity reg val incorrect.Either controller in bad state or not powered on\n",__func__);
				ret = -ENODEV;
			}
		}
	}
	return ret;
}

/**
 * spi_cnss_controller_init: controller initialization during uwb on
 * @spi_drv: pointer to spi_cnss main struct
 * return: zero if success, non-zeor otherwise
 */
static int spi_cnss_controller_init(struct spi_cnss_priv *spi_drv)
{

	int ret = 0;
	SPI_CNSS_DBG(spi_drv, "%s\n",__func__);

#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	spi_drv->client_state = AWAKE;
	test_drv = spi_drv;
	spi_stub_driver_reg_cb(notification_to_schedule_wq);
#else
	enable_irq(spi_drv->irq);
	ret = spi_cnss_wakeup_client(spi_drv, NUM_OF_TRIALS_DURING_OPEN);

	if (ret < 0 && gpio_get_value(spi_drv->gpio)) {
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_SANITY_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Controller is in bad state or not powered on: %d\n",__func__, ret);
			spi_drv->client_state = ASLEEP;
			return -ENODEV;
		} else {
			spi_drv->client_state = AWAKE;
		}
	}
#endif
	if (spi_drv->client_state == AWAKE) {
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_DEVICE_ID_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI_SLAVE_DEVICE_ID_REG read failed: %d\n",__func__, ret);
			return ret;
		}
		SPI_CNSS_DBG(spi_drv,"%s: slave dev id read\n",__func__);
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_STATUS_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI_SLAVE_STATUS_REG read failed: %d\n",__func__, ret);
			return ret;
		}
		SPI_CNSS_DBG(spi_drv,"%s:status reg read\n",__func__);
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_CONFIG_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI_SLAVE_CONFIG_REG read failed: %d\n",__func__, ret);
			return ret;
		}

		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_CONFIG_REG, SPI_REGISTER_WRITE);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI_SLAVE_CONFIG_REG write failed: %d\n",__func__, ret);
			return ret;
		}

		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_CONFIG_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError SPI_SLAVE_CONFIG_REG read failed: %d\n",__func__, ret);
			return ret;
		}

		SPI_CNSS_DBG(spi_drv,"%s:read slave sanity reg\n",__func__);
		ret = spi_cnss_register_xfer(spi_drv, SPI_SLAVE_SANITY_REG, SPI_REGISTER_READ);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError init failed, bailing out\n",__func__);
			return ret;
		}
		SPI_CNSS_DBG(spi_drv,"%s:slave config written\n",__func__);
		ret = spi_cnss_read_context_info(spi_drv, false);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi_cnss_read_context_info, init failed: %d\n",__func__, ret);
			return ret;
		}
	}
	return ret;
}
static int spi_cnss_open(struct inode *inode, struct file *filp)
{
	struct spi_cnss_priv *spi_drv;
	struct spi_cnss_user *usr;
	struct cdev *cdev;
	struct spi_cnss_chrdev *spi_cnss_cdev;
	int rc = 0, ret=0;

	rc = iminor(inode);
	if (rc >= MAX_DEV) {
		pr_err("%s Err spi dev minor:%d\n", __func__, rc);
		return -ENODEV;
	}

	cdev = inode->i_cdev;
	spi_cnss_cdev = container_of(cdev, struct spi_cnss_chrdev, c_dev[rc]);
	if (!spi_cnss_cdev) {
		pr_err("%s: chrdev is null\n",__func__);
		return -EINVAL;
	}
	spi_drv = container_of(spi_cnss_cdev, struct spi_cnss_priv, chrdev);
	if (!spi_drv) {
		pr_err("%s: spi_cnss is null\n",__func__);
		return -ENODEV;
	}
	SPI_CNSS_ERR(spi_drv, "%s rc =%d PID =%d\n", __func__, rc, current->pid);
	mutex_lock(&spi_drv->state_lock);
	usr = &spi_drv->user[rc];
	if (usr->is_active) {
		SPI_CNSS_ERR(spi_drv, "%s SpiCnssError spi open without release\n", __func__);
		ret = -EBUSY;
		goto end;
	}
	ret = spi_cnss_allocate_memory(spi_drv);
	if (ret < 0) {
		goto end;
	}
	if (!spi_drv->client_irq_buf) {
		spi_drv->client_irq_buf = spi_cnss_kzalloc(spi_drv, IRQ_WRITE_SIZE);
		if (!spi_drv->client_irq_buf) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError client_irq_buf mem alloc failed\n", __func__);
			ret = -ENOMEM;
			goto end;
		}
	}
	if (!spi_drv->client_init) {
		spi_drv->client_state = ASLEEP;
		ret = spi_cnss_controller_init(spi_drv);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi_cnss_controller_init failed\n", __func__);
			spi_cnss_kfree(spi_drv, (void **)&spi_drv->client_irq_buf);
			disable_irq(spi_drv->irq);
			spi_drv->client_init = false;
			spi_cnss_free_allocated_memory(spi_drv);
			goto end;
		}
	}
	usr->id = rc;
	usr->is_active = true;
	init_waitqueue_head(&usr->readq);
	init_waitqueue_head(&usr->readwq);
	init_completion(&usr->sync_wait);
	INIT_KFIFO(usr->user_fifo);
	filp->private_data = usr;
	spi_drv->usr_cnt++;
end:
	mutex_unlock(&spi_drv->state_lock);
	return ret;
}

static ssize_t spi_cnss_write(struct file *filp, const char __user *buf, size_t len, loff_t *f_pos)
{
	struct spi_cnss_priv *spi_drv;
	struct spi_cnss_user *usr;
	struct spi_usr_request *user_req;
	struct spi_cnss_packet user_pkt;
	void *data_buf = NULL;
	int ret,rc;
	int err_code = 0;

	if (!filp || !buf || !len | !filp->private_data) {
		pr_err("%s: Null pointer\n", __func__);
		return -EINVAL;
	}
	usr = filp->private_data;
	spi_drv = container_of(usr, struct spi_cnss_priv, user[usr->id]);
	if (!spi_drv) {
		pr_err("%s: spi_drv is null\n",__func__);
		ret = -ENODEV;
		goto end;
	}
	if (!spi_drv->client_init) {
		pr_err("%s: controller is not initialized\n",__func__);
		return -EINVAL;
	}
	user_req = &user_pkt.user_req;
	atomic_set(&spi_drv->write_err_code, 0);
	rc = usr->id;
	user_pkt.id = rc;
	SPI_CNSS_DBG(spi_drv, "%s PID =%d\n", __func__, current->pid);
	if (len != sizeof(struct spi_usr_request)) {
		SPI_CNSS_ERR(spi_drv, "%sInvalid write request length: %lu, expected: %lu\n",
								__func__, len, sizeof(struct spi_usr_request));
		return -EINVAL;
	}
	if (copy_from_user(user_req, buf, len)) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError copy_from_user err\n",__func__);
		return -EFAULT;
	}

	SPI_CNSS_DBG(spi_drv,"%s command = %d, len = %d, sleep bit = %d\n",
				__func__, user_req->cmd, user_req->data_len, user_req->reserved[0]);
	spi_drv->sleep_enabled = (user_req->reserved[0] & SPI_SLEEP_CMD_BIT);
	if (user_req->cmd == USER_WRITE) {
		data_buf = spi_cnss_kzalloc(spi_drv, user_req->data_len);
		if (!data_buf) {
			SPI_CNSS_ERR(spi_drv,"%s: buffer alloc failed\n",__func__);
			return -ENOMEM;
		}
		if (copy_from_user(data_buf, user_req->data_buf, user_req->data_len)) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError data buffer copy failed\n",__func__);
			return -EFAULT;
		}
		user_req->data_buf = data_buf;
		spi_cnss_prepare_data_log(spi_drv, "spi write", (char *)data_buf,
								user_req->data_len, 0, user_req->data_len);
	} else if (user_req->cmd == SRESET) {
		ret = spi_cnss_soft_reset(spi_drv);
		goto end;
	} else {
		SPI_CNSS_ERR(spi_drv,"%s: command not handled\n",__func__);
		return -EINVAL;

	}
#ifdef CONFIG_SLEEP
		ret = pm_runtime_get_sync(spi_drv->dev);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Err pm get sync, with err = %d\n",__func__,ret);
			pm_runtime_put_noidle(spi_drv->dev);
			pm_runtime_set_suspended(spi_drv->dev);
			spi_cnss_kfree(spi_drv, (void **)&data_buf);
			return ret;
		}
#endif

	mutex_lock(&spi_drv->queue_lock);
	list_add_tail(&user_pkt.list, &spi_drv->tx_list);
	ret = spi_cnss_transfer(spi_drv, &spi_drv->user[rc], len);
	err_code = atomic_read(&spi_drv->write_err_code);
	if ((ret != -ETIMEDOUT) && (err_code < 0)) {
		ret = err_code;
		SPI_CNSS_ERR(spi_drv,"%s: Write Failure=%d\n",__func__, ret);
	}
	if (data_buf) {
		spi_cnss_kfree(spi_drv, (void **)&data_buf);
	}
	list_del(&user_pkt.list);
	mutex_unlock(&spi_drv->queue_lock);
end:
#ifdef CONFIG_SLEEP
	if (spi_drv) {
		pm_runtime_mark_last_busy(spi_drv->dev);
		pm_runtime_put_autosuspend(spi_drv->dev);
	}
#endif
	return ret;
}

static ssize_t spi_cnss_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct spi_cnss_user *usr;
	struct spi_cnss_priv *spi_drv;
	struct spi_client_request cp;
	int ret,status;
	if (!filp || !filp->private_data) {
		pr_err("%s Err Null pointer\n", __func__);
		return -EINVAL;
	}
	usr = filp->private_data;
	spi_drv = container_of(usr, struct spi_cnss_priv, user[usr->id]);
	if (!spi_drv) {
		pr_err("%s: spi_drv is null\n",__func__);
		return -EINVAL;
	}
	if (!spi_drv->client_init) {
		pr_err("%s: controller is not initialized\n",__func__);
		return -EINVAL;
	}
	if (copy_from_user(&cp, buf, sizeof(struct spi_client_request)) != 0) {
		SPI_CNSS_ERR(spi_drv,"%s: copy from user failed\n",__func__);
		return -EFAULT;
	}
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	if (!atomic_read(&usr->rx_avail))
		queue_work(spi_drv->bh_work_wq, &spi_drv->bh_work);
#endif

	ret = wait_event_interruptible(usr->readq, atomic_read(&usr->rx_avail));

	if (ret < 0) {
		SPI_CNSS_ERR(spi_drv,"%s: Err wait interrupt ret=%d\n",__func__, ret);
	}
	if (atomic_read(&usr->rx_avail)) {
		struct spi_client_request tmp_pkt;
		atomic_dec(&usr->rx_avail);
		SPI_CNSS_DBG(spi_drv,"%s: data available to read\n",__func__);
		ret = kfifo_get(&usr->user_fifo, &tmp_pkt);
		if (!ret) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError fifo is empty (?)\n",__func__);
			return -EAGAIN;
		}
		cp.data_len = tmp_pkt.data_len;
		cp.proto_ind = tmp_pkt.proto_ind;
		cp.end_point = tmp_pkt.end_point;
		cp.flow_id = tmp_pkt.flow_id;
		SPI_CNSS_DBG(spi_drv,"%s: data_len = %d, proto_ind= %x, endpoint= %d, flow_id= %d\n",
					__func__, cp.data_len, cp.proto_ind, cp.end_point, cp.flow_id);
		ret = copy_to_user(buf, &cp, sizeof(struct spi_client_request));
		if (ret) {
			SPI_CNSS_ERR(spi_drv,"%s: copy to user cp failed\n",__func__);
			return -EAGAIN;
		}
		ret = copy_to_user(cp.data_buf, (void *)&tmp_pkt.data_buf[tmp_pkt.flow_id], cp.data_len);
		if (ret) {
			SPI_CNSS_ERR(spi_drv,"%s: copy to user databuf failed\n",__func__);
			return -EAGAIN;
		}
		spi_cnss_prepare_data_log(spi_drv, "spi read", (char *)&tmp_pkt.data_buf[tmp_pkt.flow_id],
					cp.data_len, 0, cp.data_len);
		spi_cnss_kfree(spi_drv, (void **)&tmp_pkt.data_buf);
		SPI_CNSS_DBG(spi_drv, "%s: fifo size = %d\n",__func__, kfifo_len(&usr->user_fifo));
		if (usr->fifo_full == true) {
#ifdef CONFIG_SLEEP
			status = pm_runtime_get_sync(spi_drv->dev);
			if (status < 0) {
				SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Err pm get sync, with err = %d\n",__func__,status);
				pm_runtime_put_noidle(spi_drv->dev);
				pm_runtime_set_suspended(spi_drv->dev);
				return status;
			}
#endif
			SPI_CNSS_DBG(spi_drv, "%s: read pending\n",__func__);
			usr->read_pending = false;
			kthread_queue_work(spi_drv->kreader, &spi_drv->read_msg);
#ifdef CONFIG_SLEEP
			pm_runtime_mark_last_busy(spi_drv->dev);
			pm_runtime_put_autosuspend(spi_drv->dev);
#endif
		}
		ret = (sizeof(struct spi_client_request) - ret);
	} else {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError No client packet avaiable, spurious read request",__func__);
		return -EINVAL;
	}
	return ret;
}

static __poll_t spi_cnss_poll(struct file *filp, poll_table *wait)
{
	struct spi_cnss_priv *spi_drv;
	struct spi_cnss_user *usr;
	__poll_t mask = 0;

	if (!filp || !filp->private_data) {
		pr_err("%s Err Null pointer\n", __func__);
		return -EINVAL;
	}
	usr = filp->private_data;
	if (!usr) {
		pr_err("%s: private data is null\n",__func__);
		return -EINVAL;
	}
	spi_drv = container_of(usr, struct spi_cnss_priv, user[usr->id]);
	if (!spi_drv) {
		pr_err("%s: spi drv is null\n",__func__);
		return -EINVAL;
	}
	if (!spi_drv->client_init) {
		pr_err("%s: controller is not initialized\n",__func__);
		return -ENODEV;
	}
	SPI_CNSS_DBG(spi_drv, "%s PID =%d\n", __func__, current->pid);
	poll_wait(filp, &usr->readwq, wait);
	SPI_CNSS_DBG(spi_drv, "%s: after poll wait", __func__);
	if (atomic_read(&usr->rx_avail)) {
		mask = (POLLIN|POLLRDNORM);
		SPI_CNSS_DBG(spi_drv,"%s: rx data available", __func__);
	}
	return mask;
}

static int spi_cnss_release(struct inode *inode, struct file *filp)
{
	struct spi_cnss_user *usr;
	struct spi_cnss_priv *spi_drv = NULL;
	int ret;

	if (!filp || !filp->private_data) {
		pr_err("%s Err Null pointer\n", __func__);
		return -EINVAL;
	}
	usr = filp->private_data;
	if (!usr) {
		pr_err("%s: private data is null\n",__func__);
		return -EINVAL;
	}
	spi_drv = container_of(usr, struct spi_cnss_priv, user[usr->id]);
	if (!spi_drv) {
		pr_err("%s: driver data is null\n",__func__);
		return -EINVAL;
	}
	SPI_CNSS_ERR(spi_drv, "%s PID =%d\n", __func__, current->pid);
	mutex_lock(&spi_drv->state_lock);
	if (spi_drv->write_pending || spi_drv->read_pending || spi_drv->context_read_pending) {
		SPI_CNSS_ERR(spi_drv,"%s: spi transfer in progress\n",__func__);
		usleep_range(500000, 1000000);
	}
	usr->is_active = false;
	spi_drv->usr_cnt--;
	SPI_CNSS_DBG(spi_drv, "%s usr_cnt = %d\n", __func__, spi_drv->usr_cnt);
	if (spi_drv->usr_cnt == 0) {
#ifdef CONFIG_SLEEP
		if (spi_drv->client_state == ASLEEP) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Client asleep. Waking it up\n",__func__);
			spi_cnss_wakeup_sequence(spi_drv);
			if (spi_drv->client_state != ASLEEP) {
				SPI_CNSS_DBG(spi_drv, "%s: wakeup client success\n",__func__);
			} else {
				SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Failed to wakeup client\n",__func__);
			}
		}
		disable_irq(spi_drv->irq);
		// Wait for 100 msec before sending reset cmd byte.
		msleep(100);
		spi_drv->client_state = RESET;
		SPI_CNSS_DBG(spi_drv, "%s:Sending RESET_CMD_BYTE\n",__func__);
		ret = spi_cnss_send_byte_cmd(spi_drv, RESET_CMD_BYTE);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv,"%s: failed to send reset indication cmd\n",__func__);
			spi_drv->client_state = AWAKE;
		}
		// Wait for 100 msec after sending reset cmd byte.
		msleep(100);
		ret = pm_runtime_suspend(spi_drv->dev);
		SPI_CNSS_DBG(spi_drv, "%s: pm_runtime_suspend status = %d\n",__func__,ret);
#endif
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Before fifo size = %d\n",__func__, kfifo_len(&usr->user_fifo));
		kfifo_free(&usr->user_fifo);
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError After fifo size = %d\n",__func__, kfifo_len(&usr->user_fifo));

		spi_cnss_kfree(spi_drv, (void **)&spi_drv->client_irq_buf);
		spi_cnss_free_allocated_memory(spi_drv);
		spi_drv->client_irq_buf = NULL;
		spi_drv->client_init = false;
		spi_drv->sleep_enabled = false;
		atomic_set(&usr->rx_avail, 0);
#ifdef CONFIG_AGGRESSIVE_SLEEP
#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
		del_timer_sync(&spi_drv->client_sleep_timer);
#else
		timer_delete_sync(&spi_drv->client_sleep_timer);
#endif
#endif
	}
	SPI_CNSS_ERR(spi_drv,"%s: memory alloc = %d\n",__func__, atomic_read(&spi_drv->spi_alloc_cnt));
#ifdef CONFIG_SPI_LOOPBACK_ENABLED
	spi_stub_driver_unreg_cb();
#endif
	mutex_unlock(&spi_drv->state_lock);
	return 0;
}

static struct file_operations spi_cnss_fops = {
	.owner   = THIS_MODULE,
	.open    = spi_cnss_open,
	.read    = spi_cnss_read,
	.write   = spi_cnss_write,
	.poll    = spi_cnss_poll,
	.release = spi_cnss_release,
};

/**
 * spi_cnss_create_chrdev: allocate 2 char devices
 * @spi_drv: pointer to spi_cnss main struct
 * return: zero if success, non-zeor otherwise
 */
static int spi_cnss_create_chrdev(struct spi_cnss_priv *spi_drv)
{
	int ret = 0, i;

	pr_info("%s PID =%d\n", __func__, current->pid);

	ret = alloc_chrdev_region(&spi_drv->chrdev.spidev, 0, MAX_DEV,"spicnssdev");
	if (ret < 0) {
		pr_err("%s ret: %d\n", __func__, ret);
		return ret;
	}
	spi_cnss_cdev_major = MAJOR(spi_drv->chrdev.spidev);
	spi_drv->chrdev.spi_cnss_class = class_create("spicnssdev");
	if (IS_ERR(spi_drv->chrdev.spi_cnss_class)) {
		SPI_CNSS_ERR(spi_drv, "%s err: %ld\n", __func__, PTR_ERR(spi_drv->chrdev.spi_cnss_class));
		ret = PTR_ERR(spi_drv->chrdev.spi_cnss_class);
		goto error_class_create;
	}
	for (i = 0; i < MAX_DEV; i++) {
		cdev_init(&spi_drv->chrdev.c_dev[i], &spi_cnss_fops);
		spi_drv->chrdev.c_dev[i].owner = THIS_MODULE;
		spi_drv->chrdev.major = spi_cnss_cdev_major;
		spi_drv->chrdev.minor = i;
		ret = cdev_add(&spi_drv->chrdev.c_dev[i], MKDEV(spi_cnss_cdev_major,i), 1);
		if (ret < 0) {
			printk("%d",ret);
			SPI_CNSS_ERR(spi_drv, "%s ret: %d\n", __func__, ret);
			goto error_device_add;
		}
		if (i == UWB_MINOR_DEV_NUM) {
			spi_drv->chrdev.class_dev = device_create(spi_drv->chrdev.spi_cnss_class, NULL,
								MKDEV(spi_cnss_cdev_major,i),
								NULL, "spiuwb");
		} else {
			spi_drv->chrdev.class_dev = device_create(spi_drv->chrdev.spi_cnss_class, NULL,
								MKDEV(spi_cnss_cdev_major,i),
								NULL, "spibt");
		}
		if (IS_ERR(spi_drv->chrdev.class_dev)) {
			ret = PTR_ERR(spi_drv->chrdev.class_dev);
			SPI_CNSS_ERR(spi_drv, "%s ret: %d\n", __func__, ret);
			goto error_device_create;
		}
	}
	return 0;
error_device_create:
	class_destroy(spi_drv->chrdev.spi_cnss_class);
error_device_add:
	for (i = 0; i < MAX_DEV; i++) {
		cdev_del(&spi_drv->chrdev.c_dev[i]);
	}
error_class_create:
	unregister_chrdev_region(MKDEV(spi_cnss_cdev_major, 0), MINORMASK);
	return ret;
}
static int spi_cnss_probe(struct spi_device *spi)
{
	struct spi_cnss_priv *spi_drv;
	int ret = 0;
	struct device_node *node;
	struct device *dev = &spi->dev;
	int i;

	if (dev == NULL) {
		pr_err("%s dev is null\n", __func__);
		return -ENODEV;
	}
	spi_drv = devm_kzalloc(&spi->dev, sizeof(*spi_drv), GFP_KERNEL);
	if (!spi_drv) {
		pr_err("%s No Memory\n", __func__);
		return -ENOMEM;
	}

	spi_drv->spi = spi;
	spi_drv->dev = &spi->dev;
	node = spi_drv->spi->dev.of_node;
	SPI_CNSS_DBG(spi_drv, "%s PID =%d\n", __func__, current->pid);
	spi_drv->gpio = of_get_named_gpio(node, "qcom,irq-gpio", 0);
	SPI_CNSS_DBG(spi_drv, "%s: gpio = %d\n",__func__, spi_drv->gpio);
	gpio_direction_input(spi_drv->gpio);
	spi_drv->irq = gpio_to_irq(spi_drv->gpio);
	SPI_CNSS_DBG(spi_drv, "%s: irq = %d\n",__func__, spi_drv->irq);
	irq_set_status_flags(spi_drv->irq, IRQ_NOAUTOEN);
	SPI_CNSS_DBG(spi_drv, "%s: device name = %s\n",__func__,dev_name(dev));
	ret = devm_request_irq(dev, spi_drv->irq, spi_cnss_irq,
				/*IRQF_TRIGGER_HIGH*/IRQF_TRIGGER_RISING, dev_name(dev), spi_drv);
	if (ret) {
		dev_err(dev, "IRQ request failed %d\n", ret);
		return ret;
	}

	if (of_property_read_u32(node , "spi-max-freq",
					&spi_drv->spi_max_freq)) {
		SPI_CNSS_ERR(spi_drv, "SPI max freq not specified\n");
		spi_drv->spi_max_freq = 10000000;
	}
	SPI_CNSS_DBG(spi_drv, "%s: spi-max-freq = %d\n",__func__, spi_drv->spi_max_freq);
	ret = spi_cnss_create_chrdev(spi_drv);
	if (ret) {
		goto probe_err;
	}
	spi_drv->usr_cnt = 0;
	spi_drv->client_init = false;
	init_waitqueue_head(&spi_drv->readq);
	INIT_LIST_HEAD(&spi_drv->tx_list);
	INIT_LIST_HEAD(&spi_drv->rx_list);
	mutex_init(&spi_drv->queue_lock);
	mutex_init(&spi_drv->xfer_lock);
	mutex_init(&spi_drv->state_lock);
	mutex_init(&spi_drv->read_lock);
	mutex_init(&spi_drv->irq_lock);
	mutex_init(&spi_drv->mem_lock);
	mutex_init(&spi_drv->sleep_lock);
	spi_message_init(&spi_drv->spi_msg1);
	spi_message_add_tail(&spi_drv->spi_xfer1, &spi_drv->spi_msg1);

	spi_drv->kwriter = kthread_create_worker(0, "kthread_spi_cnss_writer");
	if (IS_ERR(spi_drv->kwriter)) {
		SPI_CNSS_ERR(spi_drv, "%s failed to create writer worker thread",__func__);
		return PTR_ERR(spi_drv->kwriter);
	}
	kthread_init_work(&spi_drv->send_msg, spi_cnss_send_msg);

	spi_drv->kreader = kthread_create_worker(0, "kthread_spi_cnss_reader");
	if (IS_ERR(spi_drv->kreader)) {
		SPI_CNSS_ERR(spi_drv, "%s failed to create reader worker thread",__func__);
		return PTR_ERR(spi_drv->kreader);
	}
	kthread_init_work(&spi_drv->read_msg, spi_cnss_read_msg);

	init_completion(&spi_drv->sync_wait);
	init_completion(&spi_drv->wake_wait);
	init_completion(&spi_drv->buff_wait);
	init_completion(&spi_drv->resume_wait);
	atomic_set(&spi_drv->check_resume_wait, FALSE);

	spi_drv->client_state = ASLEEP;

	spi_drv->bh_work_wq = alloc_workqueue("%s", WQ_UNBOUND|WQ_HIGHPRI, 1, dev_name(dev));
	if (!spi_drv->bh_work_wq) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError falied to alloc workqueue", __func__);
			return -ENOMEM;
	}
	INIT_WORK(&spi_drv->bh_work,spi_cnss_handle_work);
	spi_drv->ipc = ipc_log_context_create(15, dev_name(dev), 0);
	if (!spi_drv->ipc && IS_ENABLED(CONFIG_IPC_LOGGING)) {
		dev_err(dev, "Error creating IPC logs\n");
	}
#ifdef CONFIG_AGGRESSIVE_SLEEP
	spi_drv->sleep_wq = alloc_workqueue("spi_sleep_wq",WQ_UNBOUND|WQ_HIGHPRI, 0);
	if (!spi_drv->sleep_wq) {
			SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError falied to alloc spi sleep workqueue", __func__);
			destroy_workqueue(spi_drv->bh_work_wq);
			return -ENOMEM;
	}
	INIT_WORK(&spi_drv->sleep_work, spi_cnss_handle_sleep);
	timer_setup(&spi_drv->client_sleep_timer, spi_cnss_sleep_timeout_handler, 0);
#endif
	for (i = 0; i < MAX_DEV; i++) {
		spi_drv->user[i].is_active = false;
	}
	dev_dbg(dev, "SPI CNSS driver probe successful\n");
	spi_set_drvdata(spi, spi_drv);
	SPI_CNSS_INFO(spi_drv, "%s: probe successful\n", __func__);
#ifdef CONFIG_SLEEP
	pm_runtime_enable(spi_drv->dev);
	pm_runtime_set_autosuspend_delay(spi_drv->dev, SPI_AUTOSUSPEND_DELAY);
	pm_runtime_use_autosuspend(spi_drv->dev);
#endif
	return 0;

probe_err:
	SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError probe failed with err = %d\n", __func__, ret);
	return ret;
}

static void spi_cnss_remove(struct spi_device *spi)
{
	struct spi_cnss_priv *spi_drv;
	int i;

	spi_drv = spi_get_drvdata(spi);
	SPI_CNSS_DBG(spi_drv, "%s PID =%d\n", __func__, current->pid);
#ifdef CONFIG_SLEEP
	pm_runtime_disable(spi_drv->dev);
#endif
	destroy_workqueue(spi_drv->bh_work_wq);
#ifdef CONFIG_AGGRESSIVE_SLEEP
	destroy_workqueue(spi_drv->sleep_wq);
#endif
	kthread_destroy_worker(spi_drv->kwriter);
	kthread_destroy_worker(spi_drv->kreader);
	for (i = 0; i < MAX_DEV; i++) {
		device_destroy(spi_drv->chrdev.spi_cnss_class, MKDEV(spi_cnss_cdev_major, i));
		cdev_del(&spi_drv->chrdev.c_dev[i]);
	}
	class_unregister(spi_drv->chrdev.spi_cnss_class);
	unregister_chrdev_region(MKDEV(spi_cnss_cdev_major, 0), MINORMASK);
	class_destroy(spi_drv->chrdev.spi_cnss_class);
	devm_kfree(&spi->dev, spi_drv);
	spi_set_drvdata(spi, NULL);
}

static void spi_cnss_shutdown(struct spi_device *spi)
{
	spi_cnss_remove(spi);
	pr_err("%s PID =%d\n", __func__, current->pid);
}
static bool spi_cnss_client_sleep(struct spi_cnss_priv *spi_drv)
{
	bool status = false;

	if (!spi_drv->read_pending && spi_drv->client.CBUF_LEN == 0 && !spi_drv->write_pending
		&& spi_drv->client_state == AWAKE && !gpio_get_value(spi_drv->gpio)
		&& !spi_drv->context_read_pending && spi_drv->client_state != RESET) {
		status = true;
	}
	SPI_CNSS_DBG(spi_drv,"%s status:%d\n",__func__, status);
	return status;
}
static int spi_cnss_runtime_suspend(struct device *dev)
{
#ifdef CONFIG_SLEEP
	int ret = 0;
	struct spi_device *spi = to_spi_device(dev);
	struct spi_cnss_priv *spi_drv = spi_get_drvdata(spi);
	SPI_CNSS_DBG(spi_drv,"%s\n",__func__);
	if (spi_drv == NULL) {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError spi_drv is null\n",__func__);
		return 0;
	}
	if (spi_cnss_client_sleep(spi_drv)) {
		SPI_CNSS_ERR(spi_drv,"%s: putting client to sleep\n",__func__);
		ret = spi_cnss_send_byte_cmd(spi_drv, SLEEP_CMD_BYTE);
		if (ret < 0) {
			SPI_CNSS_ERR(spi_drv,"%s: failed to send sleep cmd\n",__func__);
			spi_drv->client_state = AWAKE;
		}
	} else if ((spi_drv->client_state == ASLEEP) || (spi_drv->client_state == RESET)) {
		SPI_CNSS_DBG(spi_drv,"%s:client asleep or getting reset\n",__func__);
		return 0;
	} else {
		SPI_CNSS_INFO(spi_drv,"%s: read/write pending or client asleep, returning busy\n",__func__);
		return -EBUSY;
	}
#endif
	SPI_CNSS_ERR(spi_drv, "%s PID=%d\n", __func__, current->pid);
	return 0;
}

static int spi_cnss_runtime_resume(struct device *dev)
{
#ifdef CONFIG_SLEEP
	struct spi_device *spi = to_spi_device(dev);
	struct spi_cnss_priv *spi_drv = spi_get_drvdata(spi);

	pr_err("%s\n", __func__);
	if (spi_drv == NULL) {
		pr_err("%s: spi_drv is null\n",__func__);
		return 0;
	}
	if (spi_drv->usr_cnt == 0) {
		pr_err("%s: no active clients\n",__func__);
		return 0;
	}
	if (spi_drv->client_state == AWAKE) {
		return 0;
	}
	spi_cnss_wakeup_sequence(spi_drv);

	if (spi_drv->client_state != ASLEEP) {
		SPI_CNSS_DBG(spi_drv, "%s: wakeup client success\n",__func__);
		return 0;
	} else {
		SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Failed to wakeup client\n",__func__);
		return -1;
	}
#endif
	SPI_CNSS_ERR(spi_drv, "%s PID=%d\n", __func__, current->pid);
	return 0;
}

static int spi_cnss_suspend(struct device *dev)
{
#ifdef CONFIG_SLEEP
	int ret = 0;

	struct spi_device *spi = to_spi_device(dev);
	struct spi_cnss_priv *spi_drv = spi_get_drvdata(spi);

	if (spi_drv == NULL) {
		pr_err("%s: spi_drv is null\n",__func__);
		return 0;
	}

	SPI_CNSS_ERR(spi_drv, "%s\n", __func__);
	if (!pm_runtime_status_suspended(spi_drv->dev)) {
		ret = spi_cnss_runtime_suspend(dev);
		if (ret != 0)
			SPI_CNSS_ERR(spi_drv, "%s runtime suspend failed\n", __func__);
	}
	if (ret == 0) {
		atomic_set(&spi_drv->check_resume_wait, TRUE);
		reinit_completion(&spi_drv->resume_wait);
	}
#endif
	return ret;
}

static int spi_cnss_resume(struct device *dev)
{
#ifdef CONFIG_SLEEP
	struct spi_device *spi = to_spi_device(dev);
	struct spi_cnss_priv *spi_drv = spi_get_drvdata(spi);

	pr_err("%s\n", __func__);

	if (spi_drv == NULL) {
		pr_err("%s: spi_drv is null\n",__func__);
		return 0;
	}
	if (spi_drv->usr_cnt == 0) {
		pr_err("%s: no active clients\n",__func__);
		return 0;
	}

	if (!pm_runtime_status_suspended(spi_drv->dev)) {
		SPI_CNSS_ERR(spi_drv, "%s client not suspended\n", __func__);
		return 0;
	}
	SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError calling runtime resume\n", __func__);
	spi_cnss_runtime_resume(dev);
	SPI_CNSS_ERR(spi_drv, "%s:SpiCnssError Setting complete of resume\n", __func__);
	complete(&spi_drv->resume_wait);
	return 0;
#endif
	SPI_CNSS_ERR(spi_drv, "%s PID=%d\n", __func__, current->pid);
	return 0;
}

static const struct dev_pm_ops spi_cnss_pm = {
	SET_RUNTIME_PM_OPS(spi_cnss_runtime_suspend,
			   spi_cnss_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(spi_cnss_suspend,
				spi_cnss_resume)
};
static const struct of_device_id spi_cnss_of_match[] = {
	{.compatible = "qcom,spi-cnss", },
	{}
};
static struct spi_driver spi_cnss_driver = {
	.driver = {
			.name = "spi-cnss",
			.of_match_table = spi_cnss_of_match,
			.pm = &spi_cnss_pm,
	},
	.probe = spi_cnss_probe,
	.remove = spi_cnss_remove,
	.shutdown =  spi_cnss_shutdown,
};

module_spi_driver(spi_cnss_driver);
MODULE_DESCRIPTION("spi cnss driver");
MODULE_LICENSE("GPL v2");
