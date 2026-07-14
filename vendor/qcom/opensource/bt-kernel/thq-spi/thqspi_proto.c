/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
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
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/device.h>
#include "thqspi_reg.h"
#include "thqspi_mbox_reg.h"
#include "thqspi_proto.h"
#define CREATE_TRACE_POINTS
#include "thqspi_trace.h"

struct thqspi_priv *thqspi_drv = NULL;
static int32_t thqspi_transfer(struct thqspi_priv *spi_drv, struct spi_transfer *xfers, int num_xfers);
static int32_t thqspi_send_msg(struct thqspi_priv *spi_drv, struct thqspi_user_request *user_req);
static int32_t thqspi_read_reg(struct thqspi_priv *spi_drv, uint16_t address, uint16_t *value);
static int32_t thqspi_read_reg(struct thqspi_priv *spi_drv, uint16_t address, uint16_t *value);
static bool thqspi_wait_cmd_cmplt(struct thqspi_priv *spi_drv);
static int32_t thqspi_write_reg(struct thqspi_priv *spi_drv, uint16_t address, uint16_t value);
static int32_t thqspi_read_hcr(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *out_buff, uint32_t len, bool nonIncAddr);
static int32_t thqspi_write_hcr(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *in_buff, uint32_t len);
static int32_t thqspi_spi_configure(struct thqspi_priv *spi_drv);
static int32_t thqspi_write_fifomailbox(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *in_buff, uint32_t length);
static int32_t thqspi_read_fifomailbox(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *out_buff, uint32_t length);
static int32_t thqspi_host_ready_for_rx(struct thqspi_priv *spi_drv);
static void thqspi_print_host_status_registers(struct thqspi_priv *spi_drv, HostStatusRegisters_t *status);
static void thqspi_do_work(struct thqspi_priv *spi_drv);

/**
 * FTrace logging
 */
void thqspi_trace_log(struct device *dev, const char *fmt, ...)
{
	struct va_format vaf = {
				.fmt = fmt,
	};
	va_list args;
	va_start(args, fmt);
	vaf.va = &args;
	trace_thqspi_log_info(dev_name(dev), &vaf);
	va_end(args);
}

static void thqspi_print_host_status_registers(struct thqspi_priv *spi_drv, HostStatusRegisters_t *status)
{
	THQSPI_DBG(spi_drv, "Host Interrupt Status Registers:");
	THQSPI_DBG(spi_drv, "HostIntStatus          : 0x%02X", status->IntStatusRegisters.HostIntStatus);
	THQSPI_DBG(spi_drv, "CPUIntStatus           : 0x%02X", status->IntStatusRegisters.CPUIntStatus);
	THQSPI_DBG(spi_drv, "ErrorIntStatus         : 0x%02X", status->IntStatusRegisters.ErrorIntStatus);
	THQSPI_DBG(spi_drv, "CounterIntStatus       : 0x%02X", status->IntStatusRegisters.CounterIntStatus);
	THQSPI_DBG(spi_drv, "Mailbox Frame Register : 0x%02X", status->MBOXFrameRegister);
	THQSPI_DBG(spi_drv, "RX LookAhead Valid     : 0x%02X", status->RXLookAheadValid);
	THQSPI_DBG(spi_drv, "Empty Bytes            : 0x%02X 0x%02X", status->Empty[0], status->Empty[1]);
	THQSPI_DBG(spi_drv, " RX LookAhead Values:");
	for (int i = 0; i < NUMBER_MAILBOXES; ++i) {
		THQSPI_DBG(spi_drv, "  RXLookAhead[%d]            : 0x%08X", i, status->RXLookAhead[i]);
	}
}


/**
 * thqspi_prepare_data_log: Log data dump
 * @thqspi_drv: pointer to main thqspi struct
 * @prefix: prefix to use in log
 * @str: string to dump in log
 * @total: payload size
 * @offset: start of str position
 * @size: total size str
 * return: void
 */
void thqspi_prepare_data_log(struct thqspi_priv *thqspi_drv, char *prefix,
	char *str, int total, int offset, int size)
{
	char buf[DATA_BYTES_PER_LINE * 5];
	char data[DATA_BYTES_PER_LINE * 5];
	int len = min(size, DATA_BYTES_PER_LINE);

	hex_dump_to_buffer(str, len, DATA_BYTES_PER_LINE, 1, buf, sizeof(buf), false);
	scnprintf(data, sizeof(data), "%s[%d-%d of %d]: %s", prefix, offset + 1,
				offset + len, total, buf);
	THQSPI_DBG(thqspi_drv, "%s: %s", __func__, data);
}

/**
 * thqspi_kzalloc: alloc kernel memory
 * @thqspi_drv: pointer to main thqspi struct
 * @size: size of memory to allocate
 * return: pointer to allocated memory if success, null otherwise
 */
void* thqspi_kzalloc(struct thqspi_priv *thqspi_drv, int size)
{
	void *ptr = NULL;
	mutex_lock(&thqspi_drv->mem_lock);
	THQSPI_DBG(thqspi_drv,"%s: requesting size = %d", __func__, size);
	ptr = kzalloc(size, GFP_ATOMIC);
	if (ptr) {
		atomic_inc(&thqspi_drv->thqspi_alloc_cnt);
		THQSPI_DBG(thqspi_drv,"%s: Allocated Total buffers now : %d Current pointer:%p allocated size:%d", __func__, atomic_read(&thqspi_drv->thqspi_alloc_cnt), ptr,size);
	}
	else {
		THQSPI_ERR(thqspi_drv,"%s: Mem alloc failed", __func__);
	}
	mutex_unlock(&thqspi_drv->mem_lock);
	return ptr;
}

/**
 * thqspi_kfree: free allocated memory
 * @thqspi_drv: pointer to main thqspi struct
 * @ptr: pointer to allocated memory
 * return: void
 */
void thqspi_kfree(struct thqspi_priv *thqspi_drv, void **ptr)
{
	mutex_lock(&thqspi_drv->mem_lock);
	if (*ptr) {
		atomic_dec(&thqspi_drv->thqspi_alloc_cnt);
		THQSPI_DBG(thqspi_drv, "%s: Freeing mem %p Remained allocated buffers:%d", __func__, *ptr, atomic_read(&thqspi_drv->thqspi_alloc_cnt));
		kfree(*ptr);
		*ptr = NULL;
	} else {
		THQSPI_ERR(thqspi_drv, "%s: Ptr is null. Nothing to free", __func__);
	}
	mutex_unlock(&thqspi_drv->mem_lock);
}


static void thqspi_free_allocated_memory(struct thqspi_priv *thqspi_drv)
{
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.single_byte);
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.reg_read_write);
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.rx_payload);
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.tx_payload);
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.status_reg);
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->mem_mngr.host_reg);
}

static int thqspi_allocate_memory(struct thqspi_priv *thqspi_drv)
{
	int ret = 0;
	THQSPI_DBG(thqspi_drv, "%s", __func__);

	thqspi_drv->mem_mngr.single_byte = thqspi_kzalloc(thqspi_drv, sizeof(uint8_t));
	if (!thqspi_drv->mem_mngr.single_byte) {
		THQSPI_ERR(thqspi_drv, "%s:single_byte failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

	thqspi_drv->mem_mngr.reg_read_write = thqspi_kzalloc(thqspi_drv, sizeof(uint32_t));
	if (!thqspi_drv->mem_mngr.reg_read_write) {
		THQSPI_ERR(thqspi_drv, "%s:reg_read_write failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

	thqspi_drv->mem_mngr.tx_payload = thqspi_kzalloc(thqspi_drv, SPI_MAILBOX_BUFFER_SIZE);
	if (!thqspi_drv->mem_mngr.tx_payload) {
		THQSPI_ERR(thqspi_drv, "%s:tx_payload failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

	thqspi_drv->mem_mngr.rx_payload = thqspi_kzalloc(thqspi_drv, SPI_MAILBOX_BUFFER_SIZE);
	if (!thqspi_drv->mem_mngr.rx_payload) {
		THQSPI_ERR(thqspi_drv, "%s:rx_payload failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

	thqspi_drv->mem_mngr.status_reg = thqspi_kzalloc(thqspi_drv, sizeof(HostStatusRegisters_t));
	if (!thqspi_drv->mem_mngr.status_reg) {
		THQSPI_ERR(thqspi_drv, "%s:status_reg failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

	thqspi_drv->mem_mngr.host_reg = thqspi_kzalloc(thqspi_drv, sizeof(HostIntEnableRegisters_t));
	if (!thqspi_drv->mem_mngr.host_reg) {
		THQSPI_ERR(thqspi_drv, "%s:host_reg failed", __func__);
		ret = -ENOMEM;
		goto err;
	}

err:
	if (ret < 0) {
		thqspi_free_allocated_memory(thqspi_drv);
	}
	return ret;
}

/**
 * thqspi_transfer: Submit spi_transfers SPI core
 * @spi_drv: pointer to main thqspi struct
 * return: zero if thqspi_transfer is success
 */
static int32_t thqspi_transfer(struct thqspi_priv *spi_drv, struct spi_transfer *xfers, int num_xfers)
{
	int ret = 0;
	mutex_lock(&spi_drv->xfer_lock);
	ret = spi_sync_transfer(spi_drv->spi, xfers,num_xfers);
	mutex_unlock(&spi_drv->xfer_lock);
	return ret;
}

/**
 * thqspi_read_register: read a value from the SPI slave register space
 */
static int32_t thqspi_read_reg(struct thqspi_priv *spi_drv, uint16_t address, uint16_t *value)
{
	int32_t              ret = 0;
	uint16_t             spicmd = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL || value == NULL) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		ret = -EINVAL;
		goto end;
	}

	/* We are actually sending a 16 bit transaction (big endian),     */
	/* however we will map to 8 bit mode (and flip the bits) so that  */
	/* we do not have to change the SPI configuration from 8 to 16    */
	/* bits (or vice versa).                                          */

	/* Control Phase (write only).                                    */
	spicmd = (SPI_READ_16BIT_MODE | SPI_INTERNAL_16BIT_MODE | (address & SPI_REG_ADDRESS_MASK));
	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed in control phase", __func__);
		goto end;
	}

	/* data phase (read only) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = NULL;
	xfer.rx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed in data phase", __func__);
		goto end;
	}
	((uint8_t *)value)[0] = ((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1];
	((uint8_t *)value)[1] = ((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0];
	THQSPI_DBG(spi_drv,"%s: Cmd(0x%x), Value(0x%x)", __func__, spicmd, *value);

end:
	return ret;
}

/**
 * thqspi_write_register: write a value to the SPI slave host configuration register space
 */
static int32_t thqspi_write_reg(struct thqspi_priv *spi_drv, uint16_t address, uint16_t value)
{
	int32_t              ret = 0;
	uint16_t             spicmd = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	/* We are actually sending a 16 bit transaction (big endian),     */
	/* however we will map to 8 bit mode (and flip the bits) so that  */
	/* we do not have to change the SPI configuration from 8 to 16    */
	/* bits (or vice versa).                                          */

	/* Control Phase (write only).                                    */
	spicmd = (SPI_WRITE_16BIT_MODE | SPI_INTERNAL_16BIT_MODE | (address & SPI_REG_ADDRESS_MASK));
	THQSPI_DBG(spi_drv,"%s: Cmd(0x%x), Value(0x%x)", __func__, spicmd, value);

	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];

	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed in control phase", __func__);
		goto end;
	}

	/* data phase (write only) */
	/* convert the value to big endian (in a 4 byte aligned temporary buffer) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&value)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&value)[0];
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;

	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed in data phase", __func__);
		goto end;
	}

end:
	return ret;
}

/**
 * thqspi_wait_cmd_cmplt: poll the slave device until the prior command is complete
 */
static bool thqspi_wait_cmd_cmplt(struct thqspi_priv *spi_drv)
{
	bool                 ret_state = false;
	uint16_t             status = 0;
	uint32_t             index = 0;          // Used as a byte buffer aligned to 4 bytes

	/* Check to see if SPI master has been opened.                       */
	if (spi_drv == NULL) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		goto end;
	}

	for(index = 0;index < SPI_SLV_TRANS_RETRY_COUNT; index++) {
		if (thqspi_read_reg(spi_drv, SPI_SLV_REG_SPI_STATUS, &status) < 0) {
			THQSPI_ERR(spi_drv, "%s read reg failed for SPI_STATUS", __func__);
			goto end;
		}

		if (status & SPI_SLV_REG_SPI_STATUS_BIT_HOST_ACCESS_DONE_MSK) {
			THQSPI_DBG(spi_drv, "%s Command Completion Success", __func__);
			ret_state = true;
			break;
		}
		usleep_range(SPI_SLV_IO_DELAY, SPI_SLV_IO_DELAY + 100);
	}

end:
	return ret_state;
}

/**
 * thqspi_read_hcr: read sequential bytes from SPI slave Host Control Register space
 */
static int32_t thqspi_read_hcr(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *out_buff, uint32_t len, bool non_inc_addr)
{
	int32_t              ret = 0;
	uint8_t              *la_valid = NULL;        // look ahead
	uint8_t              *lookahead0 = NULL;
	uint16_t             spicmd = 0;
	uint16_t             value = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL || out_buff == NULL || len == 0) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	mutex_lock(&thqspi_drv->hcr_lock);
	/* Write the size to HOST_CTRL_BYTE_SIZE */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_HOST_CTRL_BYTE_SIZE, (len | (non_inc_addr ? SPI_SLV_REG_HOST_CTRL_NOADDRINC:0)));
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for CTRL_BYTE_SIZE", __func__);
		goto end;
	}

	/* Write command to 'read from register address' to  HOST_CTRL_CONFIG */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_HOST_CTRL_CONFIG, (HOST_CTRL_REG_DIR_RD | HOST_CTRL_REG_ENABLE | (address & HOST_CTRL_REG_ADDRESS_MSK)));
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for HOST_CTRL_CONFIG", __func__);
		goto end;
	}

	/* Wait for command completion */
	if (thqspi_wait_cmd_cmplt(spi_drv) == false) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed", __func__);
		ret = -EINVAL;
		goto end;
	}

	/* Control Phase (write only).                                    */
	spicmd = (SPI_READ_16BIT_MODE | SPI_INTERNAL_16BIT_MODE | SPI_SLV_REG_HOST_CTRL_RD_PORT);
	THQSPI_DBG(spi_drv,"%s: Spi-Cmd(0x%x)", __func__, spicmd);

	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd:0x%x", __func__, spicmd);
		goto end;
	}


	/* data phase (read only) */
	xfer.len = len;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = NULL;
	xfer.rx_buf = out_buff;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd-data:0x%x", __func__, spicmd);
		goto end;
	}

	/*
		WORKAROUND specifically for LOOKAHEAD registers within an HTC register refresh over SPI. This issue is not yet root caused.
		Symptoms: The RX_LOOKAHEAD_VALID register sometimes reads as 0 even when SPI_SLV_REG_RDBUF_BYTE_AVA >=4.
		Also, the RX_LOOKAHEADn registers always contain garbage. In particular, when using only endpoint0, RX_LOOKAHEAD0 does
		NOT match SPI_SLV_REG_RDBUF_LOOKAHEAD1/2. The workaround here is activated by an HTC Register Refresh -- that is, a
		particular attempt to read status registers that include the LOOKAHEAD registers. Such a read is issued in the normal way,
		but selected values in the buffer to be returned are synthesized, based on values of SPI_SLV registers, and used to
		override the values read. NB: Bytes of SPI_SLV_REG_RDBUF_LOOKAHEAD values 16-bit values with the lowered number byte in the
		HIGH 8 bits; we swap.
	*/
	if ((address == HOST_INT_STATUS_ADDRESS) && (address + len >= INT_STATUS_ENABLE_ADDRESS)) {
		la_valid = out_buff + (RX_LOOKAHEAD_VALID_ADDRESS - address);
		lookahead0 = out_buff + (RX_LOOKAHEAD0_ADDRESS - address);
		THQSPI_DBG(spi_drv,"%s: WAR original RxLookAheadValid:0x%x", __func__, *la_valid);

		ret = thqspi_read_reg(spi_drv, SPI_SLV_REG_RDBUF_BYTE_AVA, &value);
		if (ret < 0) {
			THQSPI_ERR(spi_drv,"%s: read reg failed for RDBUF_BYTE_AVA. Value(%d)", __func__, value);
			goto end;
		}
		if (value <  4) {
			THQSPI_DBG(spi_drv,"%s: LookAhead is not valid", __func__);
			*la_valid = 0;
			goto end;
		}
		*la_valid = 1;
		THQSPI_DBG(spi_drv,"%s: LookAhead is valid", __func__);
		ret = thqspi_read_reg(spi_drv, SPI_SLV_REG_RDBUF_LOOKAHEAD1, &value);
		if (ret < 0) {
			THQSPI_ERR(spi_drv,"%s: read reg failed for RDBUF_LOOKAHEAD. Value(%d)", __func__, value);
			goto end;
		}
		lookahead0[0] = (value >> 8) & 0xFF;
		lookahead0[1] = (value >> 0) & 0xFF;
		ret = thqspi_read_reg(spi_drv, SPI_SLV_REG_RDBUF_LOOKAHEAD2, &value);
		if (ret < 0) {
			THQSPI_ERR(spi_drv,"%s: read reg failed for RDBUF_LOOKAHEAD2. Value(%d)", __func__, value);
			goto end;
		}
		lookahead0[2] = (value >> 8) & 0xFF;
		lookahead0[3] = (value >> 0) & 0xFF;
	}
end:
	mutex_unlock(&thqspi_drv->hcr_lock);
	return ret;
}

/**
 * thqspi_write_hcr: write sequential bytes from SPI slave Host Control Register space
 */
static int32_t thqspi_write_hcr(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *in_buff, uint32_t len)
{
	int32_t              ret = 0;
	uint16_t             spicmd = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL || in_buff == NULL || len == 0) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	mutex_lock(&thqspi_drv->hcr_lock);
	/* Write the size to HOST_CTRL_BYTE_SIZE */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_HOST_CTRL_BYTE_SIZE, len);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for CTRL_BYTE_SIZE", __func__);
		goto end;
	}

	/* Control Phase (write to HOST_CTRL_WR_PORT) */
	spicmd = (SPI_WRITE_16BIT_MODE | SPI_INTERNAL_16BIT_MODE | SPI_SLV_REG_HOST_CTRL_WR_PORT);
	THQSPI_DBG(spi_drv,"%s: Spi-Cmd(0x%x)", __func__, spicmd);

	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	*spi_drv->mem_mngr.reg_read_write = 0;
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];
	THQSPI_DBG(spi_drv,"%s: Register Address(0x%x)", __func__, *spi_drv->mem_mngr.reg_read_write);
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd:0x%x", __func__, spicmd);
		goto  end;
	}

	/* data phase (write only) */
	xfer.len = len;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = in_buff;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd-data:0x%x", __func__, spicmd);
		goto  end;
	}

	/* Write command to 'write to register address' to HOST_CTRL_CONFIG */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_HOST_CTRL_CONFIG, (HOST_CTRL_REG_DIR_WR | HOST_CTRL_REG_ENABLE | (address & HOST_CTRL_REG_ADDRESS_MSK)));
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for HOST_CTRL_CONFIG", __func__);
		goto end;
	}

	/* Wait for command completion */
	if (thqspi_wait_cmd_cmplt(spi_drv) == false) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed", __func__);
		ret = -EINVAL;
		goto end;
	}

end:
	mutex_unlock(&thqspi_drv->hcr_lock);
	return ret;
}

/**
 * thqspi_write_fifomailbox: write data to SPI slave data space
 */
static int32_t thqspi_write_fifomailbox(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *in_buff, uint32_t len)
{
	int32_t              ret = 0;
	uint16_t             spicmd = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL || in_buff == NULL || len == 0) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	/* Write the size to HOST_CTRL_BYTE_SIZE */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_DMA_SIZE, len);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for REG_DMA_SIZE", __func__);
		goto end;
	}

	/* We are actually sending a 16 bit transaction (big endian),     */
	/* however we will map to 8 bit mode (and flip the bits) so that  */
	/* we do not have to change the SPI configuration from 8 to 16    */
	/* bits (or vice versa).                                          */

	/* Control Phase (write only).                                    */
	spicmd = (SPI_WRITE_16BIT_MODE | SPI_EXTERNAL_16BIT_MODE | (address & SPI_REG_ADDRESS_MASK));
	THQSPI_DBG(spi_drv,"%s: Spi-Cmd(0x%x)", __func__, spicmd);

	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];
	THQSPI_DBG(spi_drv,"%s: Register Address(0x%x)", __func__, *spi_drv->mem_mngr.reg_read_write);
	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd:0x%x", __func__, spicmd);
		return ret;
	}

	/* data phase (write only) */
	xfer.len = len;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = in_buff;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd-data:0x%x", __func__, spicmd);
		return ret;
	}

end:
	return ret;
}

/**
 * thqspi_read_fifomailbox: read data from SPI slave data space
 */
static int32_t thqspi_read_fifomailbox(struct thqspi_priv *spi_drv, uint16_t address, uint8_t *out_buff, uint32_t len)
{
	int32_t              ret = 0;
	uint16_t             spicmd = 0;
	struct spi_transfer  xfer = {};

	if (spi_drv == NULL || out_buff == NULL || len == 0) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	/* Write the size to HOST_CTRL_BYTE_SIZE */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_DMA_SIZE, len);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for REG_DMA_SIZE", __func__);
		goto end;
	}

	/* We are actually sending a 16 bit transaction (big endian),     */
	/* however we will map to 8 bit mode (and flip the bits) so that  */
	/* we do not have to change the SPI configuration from 8 to 16    */
	/* bits (or vice versa).                                          */

	/* Control Phase (write only).                                    */
	spicmd = (SPI_READ_16BIT_MODE | SPI_EXTERNAL_16BIT_MODE | (address & SPI_REG_ADDRESS_MASK));
	THQSPI_DBG(spi_drv,"%s: Spi-Cmd(0x%x)", __func__, spicmd);

	/* convert the SPI Command to big endian (in a 4 byte aligned temporary buffer) */
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[0]     = ((uint8_t *)&spicmd)[1];
	((uint8_t *)spi_drv->mem_mngr.reg_read_write)[1]     = ((uint8_t *)&spicmd)[0];
	THQSPI_DBG(spi_drv,"%s: Register Address(0x%x)", __func__, *spi_drv->mem_mngr.reg_read_write);

	xfer.len = 2;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = (uint8_t *)spi_drv->mem_mngr.reg_read_write;
	xfer.rx_buf = NULL;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd:0x%x", __func__, spicmd);
		return ret;
	}

	/* data phase (write only) */
	xfer.len = len;
	xfer.speed_hz = spi_drv->spi_max_freq;
	xfer.tx_buf = NULL;
	xfer.rx_buf = out_buff;
	ret = thqspi_transfer(spi_drv, &xfer, 1);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: spi transaction failed for cmd-data:0x%x", __func__, spicmd);
		return ret;
	}

end:
	return ret;
}
/**
 * thqspi_spi_configure: configure the SPI slave over the SPI bus
 */
static int32_t thqspi_spi_configure(struct thqspi_priv *spi_drv)
{
	int32_t                  ret = 0;
	uint16_t                 value = 0;
	HostIntEnableRegisters_t *host_reg = NULL; // Used as a byte buffer aligned to 4 bytes

	if (spi_drv == NULL) {
		THQSPI_DBG(spi_drv, "%s PID =%d", __func__, current->pid);
		return -EINVAL;
	}

	/* RESET the SPI slave */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_SPI_CONFIG, SPI_SLV_REG_SPI_CONFIG_VAL_RESET);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for SPI_CONFIG RESET", __func__);
		goto end;
	}

	usleep_range(SPI_SLV_RESET_DELAY, SPI_SLV_RESET_DELAY + 100);

	/* Check to make sure slave is responding */
	ret = thqspi_read_reg(spi_drv, SPI_SLV_REG_SPI_STATUS, &value);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: read reg failed for SPI_STATUS. Value(%d)", __func__, value);
		goto end;
	}

	/* Enable SPI slave */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_SPI_CONFIG, SPI_CONFIG_DEFAULT);
	if (ret < 0) {
		THQSPI_ERR(spi_drv,"%s: write reg failed for SPI_CONFIG DEFAULT", __func__);
		goto end;
	}

	/* Enable SPI slave interrupts */
	ret = thqspi_write_reg(spi_drv, SPI_SLV_REG_INTR_ENABLE, SPI_INT_ENABLE_DEFAULT);
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: write Reg failed for INTR_ENABLE", __func__);
		goto end;
	}

	/* configure SPI slave interrupts we are interested in */
	host_reg = spi_drv->mem_mngr.host_reg;
	host_reg->IntStatusEnable        = INT_STATUS_ENABLE_ERROR_MASK | INT_STATUS_ENABLE_CPU_MASK | INT_STATUS_ENABLE_MBOX_DATA_MASK;
	host_reg->CPUIntStatusEnable     = 0xFF;
	host_reg->ErrorIntStatusEnable   = ERROR_STATUS_ENABLE_RX_UNDERFLOW_MASK | ERROR_STATUS_ENABLE_TX_OVERFLOW_MASK;
	host_reg->CounterIntStatusEnable = 0;
	ret = thqspi_write_hcr(spi_drv, INT_STATUS_ENABLE_ADDRESS, (uint8_t *)host_reg, sizeof(HostIntEnableRegisters_t));
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: Write HCR failed for host interrupts", __func__);
		goto end;
	}

	/* enable OOB interrupt */
	if (spi_drv->is_cold_reset_done == false)
		enable_irq(thqspi_drv->irq);
	spi_drv->is_cold_reset_req = false;
	spi_drv->is_cold_reset_done = false;

	/* signal host is ready to receive data */
	ret = thqspi_host_ready_for_rx(spi_drv); // host ready for next message
	if(ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: set host interrupt failed", __func__);
	} else {
		thqspi_drv->mem_mngr.status_reg->RXLookAheadValid &=  ~RX_LOOKAHEAD_VALID_ENDPOINT_0;
	}

	THQSPI_DBG(thqspi_drv,"%s: spi_configure successful", __func__);
end:
	return ret;
}


/**
 * is_rx_data_valid: check if rx data is valid
 * @rx_buf: rx buf read from controller
 * return: true if data is valid, false otherwise
 */
static bool is_rx_data_valid(uint8_t *rx_buf) {
	u8 *p_buf = rx_buf;
	int proto_ind = *p_buf;
	switch(proto_ind) {
		case PROTOID_THREAD:
			return true;
		default:
			return false;
	}
	return false;
}

/**
 * is_cold_reset_req: check if tx data is cold reset request
 * @tx_buf: tx buf read from controller
 * return: true if data is cold reset request, false otherwise
 */
static bool is_cold_reset_req(uint8_t *tx_buf) {
	uint8_t *p_buf = tx_buf;
	uint8_t offset = 0;
	if (p_buf[offset] == PROTOID_THREAD &&
		p_buf[++offset] == THREAD_CORE_COMMAND_PCKT &&
		p_buf[++offset] == THREAD_CORE_PCKT_TYPE_RESET &&
		((p_buf[offset + 1] | p_buf[offset + 2] << 8) == THREAD_COLD_RESET_FLAG_LEN) &&  // payload length
		((p_buf[offset + 3] | p_buf[offset + 4] << 8) == THREAD_COLD_RESET_FLAG))
			return true;
	else
		return false;
	return false;
}

/**
 * is_cold_reset_rsp: check if rx data is cold reset response
 * @rx_buf: rx buf read from controller
 * return: true if data is cold reset response, false otherwise
 */
static bool is_cold_reset_rsp(uint8_t *tx_buf) {
	uint8_t *p_buf = tx_buf;
	uint8_t offset = 0;
	if (p_buf[offset] == PROTOID_THREAD &&
		p_buf[++offset] == THREAD_CORE_RESPONSE_PCKT &&
		p_buf[++offset] == THREAD_CORE_PCKT_TYPE_RESET &&
		((p_buf[offset + 1] | p_buf[offset + 2] << 8) == 1) &&  // payload length
		p_buf[offset + 3] == THREAD_STATUS_SUCCESS)
			return true;
	else
		return false;
	return false;
}

/**
 * __thqspi_read_msg: read client data from controller
 * @thqspi_drv: pointer to main thqspi struct
 * return: zero on success, non-zero otherwise
 */
static int __thqspi_read_msg(struct thqspi_priv *thqspi_drv)
{
	int32_t ret = 0;
	struct thqspi_client_request cp;

	if (thqspi_drv == NULL) {
		THQSPI_ERR(thqspi_drv,"%s: thqspi_drv is null or released", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv->user.is_active) {
		THQSPI_ERR(thqspi_drv,"%s: no user active", __func__);
		return -EINVAL;
	}

	thqspi_drv->user.fifo_full =
			(kfifo_len(&thqspi_drv->user.user_fifo) == MAX_CLIENT_PKTS);

	if (thqspi_drv->user.fifo_full > 0) {
		THQSPI_ERR(thqspi_drv, "%s: user fifo full, bailing", __func__);
		thqspi_drv->read_pending = false;
		return ret;
	}

	/*
	 * fifo available to read
	 * Read the first 4 byte length (stored little endian)
	 */
	mutex_lock(&thqspi_drv->read_lock);
	cp.length = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(&(thqspi_drv->mem_mngr.status_reg->RXLookAhead[0]));
	cp.length += SPI_MAILBOX_LENGTH_PREPEND_SIZE;
	if (cp.length > SPI_MAILBOX_BUFFER_SIZE) {
		THQSPI_ERR(thqspi_drv,"%s: Insufficient Rx Buff: len(%d), max_len(%d)", __func__, cp.length, SPI_MAILBOX_BUFFER_SIZE);
		ret = 0; // skip to next read, instead of erroring out
		goto end;
	} else {
		THQSPI_INFO(thqspi_drv,"%s: Rx buff len(%d)", __func__, cp.length);
	}
	ret = thqspi_read_fifomailbox(thqspi_drv, MBOX_START_ADDR(0), thqspi_drv->mem_mngr.rx_payload, cp.length);
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: read fifo mbox failed", __func__);
		goto end;
	}
	// skip initial 4 bytes which is the prepended length
	cp.length -= SPI_MAILBOX_LENGTH_PREPEND_SIZE;
	cp.data_buf = thqspi_kzalloc(thqspi_drv, cp.length);
	if (cp.data_buf) {
		memcpy((u8*)cp.data_buf,
			thqspi_drv->mem_mngr.rx_payload + SPI_MAILBOX_LENGTH_PREPEND_SIZE,
			cp.length);
	} else {
		THQSPI_ERR(thqspi_drv, "%s: Failed to alloc memory", __func__);
		ret = -ENOMEM;
		goto end;
	}
	if (is_rx_data_valid(cp.data_buf)) {
		thqspi_drv->invalid_data_count = 0;
		if (thqspi_drv->is_cold_reset_req && is_cold_reset_rsp(cp.data_buf)) {
			THQSPI_ERR(thqspi_drv, "%s: Cold-Reset Done", __func__);
			thqspi_drv->is_cold_reset_done = true;
		}
		THQSPI_INFO(thqspi_drv,"%s: data valid", __func__);
		if (thqspi_drv->user.fifo_full) {
			THQSPI_INFO(thqspi_drv,"%s: host buffer full, dont clear host interrupt", __func__);
			thqspi_drv->user.read_pending = true;
		} else {
			ret = kfifo_put(&thqspi_drv->user.user_fifo, cp);
			if (ret == 0) {
				THQSPI_ERR(thqspi_drv,"%s: fifo is full", __func__);
				thqspi_drv->user.fifo_full = true;
			} else {
				THQSPI_DBG(thqspi_drv, "%s: notify data available", __func__);
				atomic_inc(&thqspi_drv->user.rx_avail);
				wake_up_interruptible(&thqspi_drv->user.readwq);
				wake_up(&thqspi_drv->user.readq);
				THQSPI_DBG(thqspi_drv,"%s: rx_avail incremented", __func__);
			}
			ret = thqspi_host_ready_for_rx(thqspi_drv); // host ready for next message
			if(ret < 0) {
				THQSPI_ERR(thqspi_drv,"%s: set host interrupt failed", __func__);
			}
			thqspi_drv->mem_mngr.status_reg->RXLookAheadValid &=  ~RX_LOOKAHEAD_VALID_ENDPOINT_0;
		}
	} else {
		thqspi_drv->invalid_data_count++;
		ret = thqspi_host_ready_for_rx(thqspi_drv); // host ready for next message
		if (ret < 0) {
			THQSPI_ERR(thqspi_drv,"%s: invalid data, set host interrupt failed", __func__);
		}
		thqspi_drv->mem_mngr.status_reg->RXLookAheadValid &=  ~RX_LOOKAHEAD_VALID_ENDPOINT_0;
		THQSPI_ERR(thqspi_drv, "%s: invalid data. Count:%d", __func__, thqspi_drv->invalid_data_count);
	}
end:
	thqspi_drv->read_pending =  false;
	mutex_unlock(&thqspi_drv->read_lock);
	THQSPI_DBG(thqspi_drv,"%s: return)", __func__);
	return ret;
}

/**
 * thqspi_read_msg: kthread work function reader thread
 * @work: pointer to kthread work struct contained in thqspi driver struct
 */
static void thqspi_read_msg(struct kthread_work *work)
{
	struct thqspi_priv *thqspi_drv = container_of(work, struct thqspi_priv, read_msg);
	int ret = 0;
	THQSPI_DBG(thqspi_drv, "%s: Enter", __func__);
	mutex_lock(&thqspi_drv->state_lock);
	if (!thqspi_drv->user.is_active) {
		THQSPI_ERR(thqspi_drv,"%s: no user", __func__);
		if (thqspi_drv->mem_mngr.status_reg->RXLookAheadValid & RX_LOOKAHEAD_VALID_ENDPOINT_0) {
			thqspi_drv->mem_mngr.status_reg->RXLookAheadValid &=  ~RX_LOOKAHEAD_VALID_ENDPOINT_0;
			thqspi_host_ready_for_rx(thqspi_drv);
		}
		return;
	}
	if (thqspi_drv->mem_mngr.status_reg->RXLookAheadValid & RX_LOOKAHEAD_VALID_ENDPOINT_0) {
		thqspi_drv->read_pending = true;
		ret = __thqspi_read_msg(thqspi_drv);
		THQSPI_INFO(thqspi_drv, "%s:read complete", __func__);
		thqspi_drv->mem_mngr.status_reg->RXLookAheadValid &=  ~RX_LOOKAHEAD_VALID_ENDPOINT_0;
		if (ret < 0) {
			THQSPI_ERR(thqspi_drv, "%s: failed", __func__);
			goto end;
		}
	} else {
		THQSPI_ERR(thqspi_drv, "%s: RX_LOOKAHEAD is not set, ignore read request", __func__);
	}
end:
	mutex_unlock(&thqspi_drv->state_lock);
	THQSPI_DBG(thqspi_drv, "%s: Exit", __func__);
	return;
}

/**
 * thqspi_send_msg: Send message to client
 * @user_req: pointer to user request
 */
static int32_t thqspi_send_msg(struct thqspi_priv *spi_drv, struct thqspi_user_request *user_req)
{
	uint32_t ret = 0;
	THQSPI_DBG(thqspi_drv, "%s: Enter ", __func__);

	unsigned long timeout = 0, xfer_timeout = 0;
	xfer_timeout = msecs_to_jiffies(IRQ_TIMEOUT_MS);
	THQSPI_DBG(thqspi_drv,"%s: waiting for oob", __func__);
	timeout = wait_for_completion_interruptible_timeout(&thqspi_drv->buff_wait, xfer_timeout);
	thqspi_drv->wait_to_notify = false;
	if (timeout <= 0) {
		THQSPI_ERR(thqspi_drv,"%s: couldnt get buffer free in time", __func__);
		return -1;
	}
	if (!thqspi_drv->write_pending) {
		// Check if it is cold reset command
		if (user_req->header.length == (SPI_MAILBOX_LENGTH_PREPEND_SIZE +COLD_RESET_CMD_SIZE) &&
			is_cold_reset_req(&user_req->data_buf[SPI_MAILBOX_LENGTH_PREPEND_SIZE])) {
			THQSPI_ERR(thqspi_drv,"%s: Sending Cold Reset Request", __func__);
			thqspi_drv->is_cold_reset_req = true;
		}
		THQSPI_DBG(thqspi_drv,"%s before FIFO write", __func__);
		thqspi_drv->write_pending = true;
		ret = thqspi_write_fifomailbox(thqspi_drv,
						MBOX_END_ADDR(DEFAULT_MAILBOX) - user_req->header.length,
						user_req->data_buf,
						user_req->header.length);
			thqspi_drv->write_pending = false;
	} else {
		THQSPI_ERR(thqspi_drv,"%s: write pending, wait for oob", __func__);
		ret = -1;
	}

	if (ret == 0)
		return sizeof(struct thqspi_user_request);
	else
		return ret;
}

/**
 * thqspi_host_read_for_rx: inform Quartz that host is ready to receive next message
 * @work: pointer bottom half work struct
 * return: void
 */
static int32_t thqspi_host_ready_for_rx(struct thqspi_priv *spi_drv) {
	int32_t ret = 0;
	*spi_drv->mem_mngr.single_byte = INT_WLAN_INTERRUPT_NO0;
	ret = thqspi_write_hcr(thqspi_drv,
		INT_TARGET_ADDRESS,
		spi_drv->mem_mngr.single_byte,
		sizeof(uint8_t));
	return ret;
}

/**
 * thqspi_handle_work: BH processing of IRQ
 * @work: pointer bottom half work struct
 * return: void
 */
static void thqspi_handle_work(struct work_struct *work)
{
	struct thqspi_priv *thqspi_drv = container_of(work, struct thqspi_priv, bh_work);
	THQSPI_DBG(thqspi_drv,"%s: Enter",__func__);


	if (thqspi_drv->invalid_data_count <= MAX_INVALID_DATA_COUNT) {
		while(!gpio_get_value(thqspi_drv->gpio) &&
			thqspi_drv->user.is_active &&
			thqspi_drv->invalid_data_count <= MAX_INVALID_DATA_COUNT) {
			thqspi_do_work(thqspi_drv);
		}
	} else {
		// If invalid data encountered many times, just read one data at a time
		// this is to avoid a scenario where SoC keeps sending invalid data,
		// interrupt line remains active and driver continuously keeps sending
		// request for status of host interrupts
		THQSPI_ERR(thqspi_drv, "%s: invalid data. Count:%d", __func__, thqspi_drv->invalid_data_count);
		if (thqspi_drv->user.is_active)
			thqspi_do_work(thqspi_drv);
	}

	THQSPI_DBG(thqspi_drv,"%s: All work done",__func__);
	return;
}

static void thqspi_do_work(struct thqspi_priv *thqspi_drv)
{
	int ret;
	uint8_t *spi_status = NULL;
	bool none_scheduled = true;
	HostStatusRegisters_t *status_reg = NULL;

	THQSPI_DBG(thqspi_drv,"%s: Enter", __func__);
#if 0 // TBD
	if (atomic_cmpxchg(&thqspi_drv->check_resume_wait, TRUE, FALSE) == TRUE) {
		THQSPI_ERR(thqspi_drv, "%s: In suspend state. Wait for resume", __func__);
		ret = wait_for_completion_timeout(&thqspi_drv->resume_wait, msecs_to_jiffies(CLIENT_WAKE_TOUT_MS));
		if (ret < 0) {
			THQSPI_ERR(thqspi_drv, "%s: System resume did not happen:", __func__);
			return;
		} else if (ret == 0) {
			THQSPI_ERR(thqspi_drv, "%s: System resume didn't happen - timeout:", __func__);
		} else {
			THQSPI_ERR(thqspi_drv, "%s: System resumed", __func__);
		}
	}
#endif
	mutex_lock(&thqspi_drv->state_lock);
	status_reg = thqspi_drv->mem_mngr.status_reg;
	memset(status_reg, 0, sizeof(HostStatusRegisters_t));
	thqspi_drv->hostintstatus_read_pending = true;
	ret = thqspi_read_hcr(thqspi_drv,
	                      HOST_INT_STATUS_ADDRESS,
	                      (uint8_t *)status_reg,
	                      sizeof(HostStatusRegisters_t),
	                      false);
	thqspi_drv->hostintstatus_read_pending = false;
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: failed to read host_int_status", __func__);
		goto end;
	}
	thqspi_print_host_status_registers(thqspi_drv, status_reg);
	if (!thqspi_drv->client_init) {
		THQSPI_ERR(thqspi_drv,"%s: client is turned off", __func__);
		goto end;
	}
	if (status_reg->RXLookAheadValid & RX_LOOKAHEAD_VALID_ENDPOINT_0 &&
		!thqspi_drv->read_pending) {
		THQSPI_INFO(thqspi_drv, "%s: data available to read", __func__);
		none_scheduled = false;
		kthread_queue_work(thqspi_drv->kreader, &thqspi_drv->read_msg);
	}
	else if (thqspi_drv->read_pending) {
		THQSPI_INFO(thqspi_drv, "%s: read pending, read not scheduled", __func__);
	}
	if (status_reg->IntStatusRegisters.CPUIntStatus & CPU_INT_STATUS_INTERRUPT_N0) {
		// clear the CPU Interrupts
		ret = thqspi_write_hcr(thqspi_drv,
		                       CPU_INT_STATUS_ADDRESS,
		                       (uint8_t*)&status_reg->IntStatusRegisters.CPUIntStatus,
		                       sizeof(status_reg->IntStatusRegisters.CPUIntStatus));
		if (!thqspi_drv->write_pending) {
			THQSPI_INFO(thqspi_drv, "%s: schedule write", __func__);
			none_scheduled = false;
			complete(&thqspi_drv->buff_wait);
		}
	}

	spi_status = thqspi_drv->mem_mngr.single_byte;
	/* Check to see if error occurred */
	if (status_reg->IntStatusRegisters.ErrorIntStatus) {
		THQSPI_INFO(thqspi_drv, "%s: SPI Error interrupts: 0x%02x.", __func__, status_reg->IntStatusRegisters.ErrorIntStatus);
		THQSPI_INFO(thqspi_drv, "%s: SPI                 : %s.",    __func__,
			(status_reg->IntStatusRegisters.ErrorIntStatus & ERROR_INT_STATUS_SPI_MASK) ? "set" : "not set");
		THQSPI_INFO(thqspi_drv, "%s: Wakeup              : %s.",    __func__,
			(status_reg->IntStatusRegisters.ErrorIntStatus & ERROR_INT_STATUS_WAKEUP_MASK) ? "set" : "not set");
		THQSPI_INFO(thqspi_drv, "%s: Rx Underflow        : %s.",    __func__,
			(status_reg->IntStatusRegisters.ErrorIntStatus & ERROR_INT_STATUS_RX_UNDERFLOW_MASK) ? "set" : "not set");

		/* Check for the SPI error status, if set we need to    */
		/* determine the error from the SPI status register.    */
		if (status_reg->IntStatusRegisters.ErrorIntStatus & ERROR_INT_STATUS_SPI_MASK) {
			/* Read register to determine the status of the operation */
			if (thqspi_read_hcr(thqspi_drv, SPI_STATUS_ADDRESS, spi_status, sizeof(uint8_t), false)) {
				THQSPI_INFO(thqspi_drv, "%s: SPI Status     : 0x%02x.", __func__, *spi_status);
				THQSPI_INFO(thqspi_drv, "%s:   Address Error: %s.", __func__, (*spi_status & SPI_STATUS_ADDR_ERR_MASK) ? "set" : "not_set");
				THQSPI_INFO(thqspi_drv, "%s:   Address Error: %s.", __func__, (*spi_status & SPI_STATUS_RD_ERR_MASK) ? "set" : "not_set");
				THQSPI_INFO(thqspi_drv, "%s:   Address Error: %s.", __func__, (*spi_status & SPI_STATUS_WR_ERR_MASK) ? "set" : "not_set");

				/* Only write 1 to the error bits to clear them.*/
				*spi_status = 0;
				*spi_status |= (SPI_STATUS_ADDR_ERR_MASK | SPI_STATUS_RD_ERR_MASK | SPI_STATUS_WR_ERR_MASK);
				/* Clear the raised SPI Interrupts */
				if (thqspi_write_hcr(thqspi_drv, SPI_STATUS_ADDRESS, spi_status, sizeof(uint8_t))) {
					THQSPI_INFO(thqspi_drv, "%s : SPI clear SPI error interrupts: SUCCESS.", __func__);
				} else {
					THQSPI_INFO(thqspi_drv, "%s : SPI clear SPI error interrupts: FAILURE.", __func__);
				}

				/* SPI Status cannot be cleared by writing ERR Int Status, so mask this bit */
				status_reg->IntStatusRegisters.ErrorIntStatus &= ~ERROR_INT_STATUS_SPI_MASK;
				/* Check if non SPI errors are set */
				if (status_reg->IntStatusRegisters.ErrorIntStatus != 0) {
					if (thqspi_write_hcr(thqspi_drv,
						ERROR_INT_STATUS_ADDRESS,
						&status_reg->IntStatusRegisters.ErrorIntStatus,
						sizeof(status_reg->IntStatusRegisters.ErrorIntStatus))) {
						THQSPI_INFO(thqspi_drv, "%s : SPI clear Error interrupts: SUCCESS.", __func__);
					} else {
						THQSPI_INFO(thqspi_drv, "%s : SPI clear Error interrupts: FAILURE.", __func__);
					}
				}
			}
		}
	}
	complete(&thqspi_drv->init_irq_wait);

end:
	mutex_unlock(&thqspi_drv->state_lock);
	return;
}

/* thqspi_irq: ISR to handle OOB interrupt */
static irqreturn_t thqspi_irq(int irq, void *data)
{
	struct thqspi_priv *thqspi_drv = data;
	THQSPI_INFO(thqspi_drv, "%s", __func__);

	if (thqspi_drv->user.is_active == false) {
		THQSPI_INFO(thqspi_drv, "%s: Discarding interrupt as client not initialized", __func__);
		return IRQ_HANDLED;
	}

	queue_work(thqspi_drv->bh_work_wq, &thqspi_drv->bh_work);
	THQSPI_INFO(thqspi_drv, "%s: IRQ Handled", __func__);
	return IRQ_HANDLED;
}

static int thqspi_open(struct inode *inode, struct file *filp)
{
	struct cdev *cdev = NULL;
	struct thqspi_chrdev *thqspi_cdev = NULL;
	int32_t ret = 0;

	if (iminor(inode) != THREAD_MN_DEV_NUM) {
		pr_err("%s Err spi dev minor:%d", __func__, iminor(inode));
		return -ENODEV;
	}

	cdev = inode->i_cdev;
	thqspi_cdev = container_of(cdev, struct thqspi_chrdev, c_dev);
	if (!thqspi_cdev) {
		pr_err("%s: chrdev is null", __func__);
		return -EINVAL;
	}

	thqspi_drv = container_of(thqspi_cdev, struct thqspi_priv, chrdev);
	if (!thqspi_drv) {
		pr_err("%s: thqspi is null", __func__);
		return -ENODEV;
	}
	THQSPI_ERR(thqspi_drv, "%s PID =%d", __func__, current->pid);

	mutex_lock(&thqspi_drv->state_lock);
	if (thqspi_drv->user.is_active) {
		THQSPI_ERR(thqspi_drv, "%s: open without release", __func__);
		ret = -EBUSY;
		mutex_unlock(&thqspi_drv->state_lock);
		goto end;
	}

	ret = thqspi_allocate_memory(thqspi_drv);
	if (ret < 0) {
		mutex_unlock(&thqspi_drv->state_lock);
		goto end;
	}

	if (!thqspi_drv->client_irq_buf) {
		thqspi_drv->client_irq_buf = thqspi_kzalloc(thqspi_drv, IRQ_WRITE_SIZE);
		if (!thqspi_drv->client_irq_buf) {
			THQSPI_ERR(thqspi_drv, "%s: client_irq_buf mem alloc failed", __func__);
			ret = -ENOMEM;
			mutex_unlock(&thqspi_drv->state_lock);
			goto end;
		}
	}

	thqspi_drv->user.is_active = true;
	thqspi_drv->invalid_data_count = 0;
	init_waitqueue_head(&thqspi_drv->user.readq);
	init_waitqueue_head(&thqspi_drv->user.readwq);
	init_completion(&thqspi_drv->user.sync_wait);
	INIT_KFIFO(thqspi_drv->user.user_fifo);

	if (!thqspi_drv->client_init) {
		ret = thqspi_spi_configure(thqspi_drv);
		if (ret < 0) {
			THQSPI_ERR(thqspi_drv, "%s: SPI conguration failed", __func__);
			mutex_unlock(&thqspi_drv->state_lock);
			goto end;
		}
		thqspi_drv->client_init = true;
	}
	mutex_unlock(&thqspi_drv->state_lock);
	// Wait for initial handshake interrupt
	ret = wait_for_completion_timeout(&thqspi_drv->init_irq_wait, msecs_to_jiffies(CLIENT_INIT_IRQ_TIMEOUT_MS));
	if (ret <= 0) {
		THQSPI_ERR(thqspi_drv,"%s: Initial handshake interrupt not received", __func__);
		goto end;
	} else {
		THQSPI_ERR(thqspi_drv,"%s: Initial handshake interrupt received", __func__);
		ret = 0;
	}

	THQSPI_DBG(thqspi_drv, "%s: SPI Open Success", __func__);
end:
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv, "%s: SPI Open Failed", __func__);
		thqspi_drv->user.is_active = true;
		thqspi_free_allocated_memory(thqspi_drv);
		if (thqspi_drv->client_irq_buf) {
			thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->client_irq_buf);
			thqspi_drv->client_irq_buf = NULL;
		}
		disable_irq(thqspi_drv->irq);
		thqspi_drv->client_init = false;
		kfifo_free(&thqspi_drv->user.user_fifo);
		thqspi_drv = NULL;
	}
	return ret;
}

static ssize_t thqspi_write(struct file *filp, const char __user *buf, size_t len, loff_t *f_pos)
{
	struct thqspi_user_request user_req;
	uint16_t data_to_send = 0;
	int ret = 0;

	if (!thqspi_drv || !filp || !buf || !len) {
		pr_err("%s: Null pointer", __func__);
		return -EINVAL;
	}
	if (!thqspi_drv->user.is_active) {
	  THQSPI_ERR(thqspi_drv, "%s: open without release", __func__);
		return -EINVAL;
	}
	if (!thqspi_drv->client_init) {
	  THQSPI_ERR(thqspi_drv, "%s: controller is not initialized", __func__);
		return -EINVAL;
	}
	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);

	if (len != sizeof(struct thqspi_user_request)) {
		THQSPI_ERR(thqspi_drv, "%s: Invalid write request length: %lu, expected: %lu",
								__func__, len, sizeof(struct thqspi_user_request));
		return -EINVAL;
	}

	if (copy_from_user(&user_req, buf, len)) {
		THQSPI_ERR(thqspi_drv, "%s: copy_from_user err", __func__);
		return -EFAULT;
	}

  if (user_req.add_header) {
		THQSPI_DBG(thqspi_drv,"%s command:%d, protId:%d  flagGrp:%d pktType:%d, pktLen:%d",
				__func__, user_req.cmd_type, user_req.header.protId, user_req.header.flag_group,
				user_req.header.packet_type, user_req.header.length);
	} else {
		THQSPI_DBG(thqspi_drv,"%s command:%d,  pktLen:%d", __func__, user_req.cmd_type, user_req.header.length);
	}

	if (user_req.cmd_type == THQSPI_WRITE) {
		if (user_req.header.length > (SPI_MAILBOX_BUFFER_SIZE - sizeof(user_req.header))) {
			THQSPI_ERR(thqspi_drv, "%s: UserBuff too big. len(%d), max_len(%lu)",
				__func__,
				user_req.header.length,
				SPI_MAILBOX_BUFFER_SIZE - sizeof(user_req.header));
			return -EFAULT;
		}
		if (user_req.add_header) {
			if (copy_from_user(thqspi_drv->mem_mngr.tx_payload + SPI_MAILBOX_LENGTH_PREPEND_SIZE, (char *)&user_req.header, sizeof(user_req.header))) {
				THQSPI_ERR(thqspi_drv, "%s: data buffer header copy failed", __func__);
				return -EFAULT;
			}
			if (copy_from_user(thqspi_drv->mem_mngr.tx_payload + SPI_MAILBOX_LENGTH_PREPEND_SIZE + sizeof(user_req.header),
					(char *)user_req.data_buf, user_req.header.length)) {
				THQSPI_ERR(thqspi_drv, "%s: data buffer copy failed", __func__);
				return -EFAULT;
			}
			data_to_send = user_req.header.length = user_req.header.length + sizeof(user_req.header);
			user_req.header.length += data_to_send + SPI_MAILBOX_LENGTH_PREPEND_SIZE;
		} else {
			if (copy_from_user(thqspi_drv->mem_mngr.tx_payload + SPI_MAILBOX_LENGTH_PREPEND_SIZE, (char *)user_req.data_buf, user_req.header.length)) {
				THQSPI_ERR(thqspi_drv, "%s: data buffer copy failed", __func__);
				return -EFAULT;
			}
			data_to_send = user_req.header.length;
			user_req.header.length += SPI_MAILBOX_LENGTH_PREPEND_SIZE;
		}
		/* Assign the length of the data in little endian format */
		ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(thqspi_drv->mem_mngr.tx_payload, data_to_send);
		user_req.data_buf = thqspi_drv->mem_mngr.tx_payload;
		thqspi_prepare_data_log(thqspi_drv, "spi write", (char *)user_req.data_buf,
								user_req.header.length, 0, user_req.header.length);
	} else if (user_req.cmd_type != THQSPI_CONFIGURE) {
		THQSPI_ERR(thqspi_drv,"%s: command not handled. cmd_type:%d", __func__, user_req.cmd_type);
		return -1;
	}
#ifdef CONFIG_SLEEP
	ret = pm_runtime_get_sync(thqspi_drv->dev);
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv, "%s: Err pm get sync, with err = %d", __func__,ret);
		pm_runtime_put_noidle(thqspi_drv->dev);
		pm_runtime_set_suspended(thqspi_drv->dev);
		return ret;
	}
#endif

	mutex_lock(&thqspi_drv->state_lock);
	if (user_req.cmd_type == THQSPI_WRITE) {
		user_req.data_buf = thqspi_drv->mem_mngr.tx_payload;
		ret = thqspi_send_msg(thqspi_drv, &user_req);
		THQSPI_DBG(thqspi_drv,"%s: thqspi_send_msg returned", __func__);
	} else {
		THQSPI_DBG(thqspi_drv,"%s: SPI Reconfig received. Add settlement delay", __func__);
		msleep(1000); // give time to settle SPI interface
		ret = thqspi_spi_configure(thqspi_drv);
		if (ret < 0) {
			THQSPI_ERR(thqspi_drv, "%s: SPI conguration failed", __func__);
		} else
			ret = sizeof(struct thqspi_user_request);
	}
	mutex_unlock(&thqspi_drv->state_lock);

#ifdef CONFIG_SLEEP
	pm_runtime_mark_last_busy(thqspi_drv->dev);
	pm_runtime_put_autosuspend(thqspi_drv->dev);
#endif

	THQSPI_DBG(thqspi_drv, "%s PID =%d write complete", __func__, current->pid);
	return ret;
}

static ssize_t thqspi_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct thqspi_client_request cp;
	int ret = 0;

	if (!filp) {
		pr_err("%s Err Null pointer", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv) {
		pr_err("%s: thqspi_drv is null", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv->client_init) {
		pr_err("%s: controller is not initialized", __func__);
		return -EINVAL;
	}

	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);
	if (copy_from_user(&cp, buf, sizeof(struct thqspi_client_request)) != 0) {
		THQSPI_ERR(thqspi_drv,"%s: copy from user failed", __func__);
		return -EFAULT;
	}

	ret = wait_event_interruptible(thqspi_drv->user.readq, atomic_read(&thqspi_drv->user.rx_avail));
	if (ret < 0) {
		THQSPI_ERR(thqspi_drv,"%s: Err wait interrupt ret=%d", __func__, ret);
		return -EFAULT;
	}

	if (atomic_read(&thqspi_drv->user.rx_avail)) {
		struct thqspi_client_request tmp_pkt;
		atomic_dec(&thqspi_drv->user.rx_avail);
		THQSPI_DBG(thqspi_drv,"%s: data available to read", __func__);
		ret = kfifo_get(&thqspi_drv->user.user_fifo, &tmp_pkt);
		if (!ret) {
			THQSPI_ERR(thqspi_drv, "%s: fifo is empty (?)", __func__);
			return -EAGAIN;
		}

		cp.length = tmp_pkt.length;
		ret = copy_to_user(buf, &cp, sizeof(struct thqspi_client_request));
		if (ret) {
			THQSPI_ERR(thqspi_drv,"%s: copy to user cp failed", __func__);
			return -EAGAIN;
		}
		ret = copy_to_user(cp.data_buf, (void *)tmp_pkt.data_buf, cp.length);
		if (ret) {
			THQSPI_ERR(thqspi_drv,"%s: copy to user databuf failed, user_ptr:%p length:%d, ret_val:%d", __func__, cp.data_buf, cp.length, ret);
			return -EAGAIN;
		}
		thqspi_prepare_data_log(thqspi_drv, "spi read", (char *)tmp_pkt.data_buf, tmp_pkt.length, 0, tmp_pkt.length);
		thqspi_kfree(thqspi_drv, (void **)&tmp_pkt.data_buf);
		THQSPI_DBG(thqspi_drv, "%s: fifo size = %d", __func__, kfifo_len(&thqspi_drv->user.user_fifo));
		mutex_lock(&thqspi_drv->state_lock);
		if (thqspi_drv->user.fifo_full == true) {
#ifdef CONFIG_SLEEP
			status = pm_runtime_get_sync(thqspi_drv->dev);
			if (status < 0) {
				THQSPI_ERR(thqspi_drv, "%s: Err pm get sync, with err = %d", __func__,status);
				pm_runtime_put_noidle(thqspi_drv->dev);
				pm_runtime_set_suspended(thqspi_drv->dev);
				return status;
			}
#endif
			THQSPI_DBG(thqspi_drv, "%s: read pending", __func__);
			thqspi_drv->user.read_pending = false;
			kthread_queue_work(thqspi_drv->kreader, &thqspi_drv->read_msg);
#ifdef CONFIG_SLEEP
			pm_runtime_mark_last_busy(thqspi_drv->dev);
			pm_runtime_put_autosuspend(thqspi_drv->dev);
#endif
		}
		mutex_unlock(&thqspi_drv->state_lock);
		ret = (sizeof(struct thqspi_client_request) - ret);
	} else {
		THQSPI_ERR(thqspi_drv, "%s: No client packet avaiable, spurious read request", __func__);
		return -EINVAL;
	}

	THQSPI_DBG(thqspi_drv, "%s PID =%d read complete", __func__, current->pid);
	return ret;
}

static __poll_t thqspi_poll(struct file *filp, poll_table *wait)
{
	__poll_t mask = 0;

	if (!filp) {
		pr_err("%s Err Null pointer", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv) {
		pr_err("%s: spi drv is null", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv->client_init) {
		pr_err("%s: controller is not initialized", __func__);
		return -EINVAL;
	}

	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);
	poll_wait(filp, &thqspi_drv->user.readwq, wait);
	THQSPI_DBG(thqspi_drv, "%s: after poll wait", __func__);
	if (atomic_read(&thqspi_drv->user.rx_avail)) {
		mask = (POLLIN|POLLRDNORM);
		THQSPI_DBG(thqspi_drv,"%s: rx data available", __func__);
	}

	return mask;
}

static int thqspi_release(struct inode *inode, struct file *filp)
{
	int32_t ret = 0;

	if (!filp) {
		pr_err("%s Err Null pointer", __func__);
		return -EINVAL;
	}

	if (!thqspi_drv) {
		pr_err("%s: driver data is null", __func__);
		return -EINVAL;
	}

	THQSPI_DBG(thqspi_drv, "%s:Enter", __func__);

	mutex_lock(&thqspi_drv->state_lock);
	if (thqspi_drv->write_pending || thqspi_drv->read_pending || thqspi_drv->hostintstatus_read_pending) {
		THQSPI_ERR(thqspi_drv,"%s: spi transfer in progress", __func__);
		usleep_range(500000, 1000000);
	}
	/* disable SPI config */
	thqspi_write_reg(thqspi_drv, SPI_SLV_REG_SPI_CONFIG, 0);

	disable_irq(thqspi_drv->irq);
	kfifo_free(&thqspi_drv->user.user_fifo);
	thqspi_drv->user.is_active = false;

#ifdef CONFIG_SLEEP
	ret = pm_runtime_suspend(thqspi_drv->dev);
	THQSPI_DBG(thqspi_drv, "%s: pm_runtime_suspend status = %d", __func__,ret);
#endif
	thqspi_kfree(thqspi_drv, (void **)&thqspi_drv->client_irq_buf);
	thqspi_free_allocated_memory(thqspi_drv);
	thqspi_drv->client_irq_buf = NULL;
	thqspi_drv->client_init = false;
	thqspi_drv->sleep_enabled = false;
	thqspi_drv->is_cold_reset_done = false;
	thqspi_drv->is_cold_reset_req = false;
	atomic_set(&thqspi_drv->user.rx_avail, 0);
	thqspi_drv->invalid_data_count = 0;

	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);
	THQSPI_DBG(thqspi_drv, "%s: memory alloc = %d", __func__, atomic_read(&thqspi_drv->thqspi_alloc_cnt));
	mutex_unlock(&thqspi_drv->state_lock);
	thqspi_drv = NULL;

	return ret;
}

static struct file_operations thqspi_fops = {
	.owner   = THIS_MODULE,
	.open    = thqspi_open,
	.read    = thqspi_read,
	.write   = thqspi_write,
	.poll    = thqspi_poll,
	.release = thqspi_release,
};

/**
 * thqspi_create_chrdev: allocate char device
 * @thqspi_drv: pointer to thqspi main struct
 * return: zero if success, non-zeor otherwise
 */
static int thqspi_create_chrdev(struct thqspi_priv *thqspi_drv)
{
	int ret = 0;

	pr_info("%s PID =%d", __func__, current->pid);

	ret = alloc_chrdev_region(&thqspi_drv->chrdev.spidev, 0, MAX_DEV, "spicnssthrq");
	if (ret < 0) {
		pr_err("%s ret: %d", __func__, ret);
		return ret;
	}
	thqspi_cdev_major = MAJOR(thqspi_drv->chrdev.spidev);
	thqspi_drv->chrdev.thqspi_class = class_create("spithrq");
	if (IS_ERR(thqspi_drv->chrdev.thqspi_class)) {
		THQSPI_ERR(thqspi_drv, "%s err: %ld", __func__, PTR_ERR(thqspi_drv->chrdev.thqspi_class));
		ret = PTR_ERR(thqspi_drv->chrdev.thqspi_class);
		goto error_class_create;
	}

	cdev_init(&thqspi_drv->chrdev.c_dev, &thqspi_fops);
	thqspi_drv->chrdev.c_dev.owner = THIS_MODULE;
	thqspi_drv->chrdev.major = thqspi_cdev_major;
	thqspi_drv->chrdev.minor = 0;
	ret = cdev_add(&thqspi_drv->chrdev.c_dev, MKDEV(thqspi_cdev_major, 0), 1);
	if (ret < 0) {
		printk("%d",ret);
		THQSPI_ERR(thqspi_drv, "%s ret: %d", __func__, ret);
		goto error_device_add;
	}
	thqspi_drv->chrdev.class_dev = device_create(thqspi_drv->chrdev.thqspi_class, NULL,
						MKDEV(thqspi_cdev_major, 0),
						NULL, "spithrq");
	if (IS_ERR(thqspi_drv->chrdev.class_dev)) {
		ret = PTR_ERR(thqspi_drv->chrdev.class_dev);
		THQSPI_ERR(thqspi_drv, "%s ret: %d", __func__, ret);
		goto error_device_create;
	}

	return 0;
error_device_create:
	class_destroy(thqspi_drv->chrdev.thqspi_class);
error_device_add:
	cdev_del(&thqspi_drv->chrdev.c_dev);
error_class_create:
	unregister_chrdev_region(MKDEV(thqspi_cdev_major, 0), MINORMASK);
	return ret;
}

static int thqspi_probe(struct spi_device *spi)
{
	struct thqspi_priv *thqspi_drv;
	int ret = 0;
	struct device_node *node;
	struct device *dev = &spi->dev;

	thqspi_drv = devm_kzalloc(&spi->dev, sizeof(*thqspi_drv), GFP_KERNEL);
	if (!thqspi_drv) {
		pr_err("%s No Memory", __func__);
		return -ENOMEM;
	}

	thqspi_drv->spi = spi;
	thqspi_drv->dev = &spi->dev;
	thqspi_drv->spi->mode = SPI_MODE_3;
	node = thqspi_drv->spi->dev.of_node;
	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);
	thqspi_drv->gpio = of_get_named_gpio(node, "qcom,irq-gpio", 0);
	THQSPI_DBG(thqspi_drv, "%s: gpio = %d", __func__, thqspi_drv->gpio);
	gpio_direction_input(thqspi_drv->gpio);
	thqspi_drv->irq = gpio_to_irq(thqspi_drv->gpio);
	THQSPI_DBG(thqspi_drv, "%s: irq = %d", __func__, thqspi_drv->irq);
	irq_set_status_flags(thqspi_drv->irq, IRQ_NOAUTOEN);
	if (dev != NULL) {
		THQSPI_DBG(thqspi_drv, "%s: device name = %s", __func__,dev_name(dev));
	}
	ret = devm_request_irq(dev, thqspi_drv->irq, thqspi_irq,
				/*IRQF_TRIGGER_HIGH*/IRQF_TRIGGER_FALLING, dev_name(dev), thqspi_drv);
	if (ret) {
		dev_err(dev, "IRQ request failed %d", ret);
		return ret;
	}

	if (of_property_read_u32(node , "spi-max-freq",
					&thqspi_drv->spi_max_freq)) {
		THQSPI_ERR(thqspi_drv, "SPI max freq not specified");
		thqspi_drv->spi_max_freq = THQSPI_CLOCK_FREQUENCY_DEFAULT;
		thqspi_drv->spi->max_speed_hz = THQSPI_CLOCK_FREQUENCY_DEFAULT;
	}
	THQSPI_DBG(thqspi_drv, "%s: spi-max-freq = %d", __func__, thqspi_drv->spi_max_freq);
	ret = thqspi_create_chrdev(thqspi_drv);
	if (ret) {
		goto probe_err;
	}

	thqspi_drv->client_init = false;
	init_waitqueue_head(&thqspi_drv->readq);
	INIT_LIST_HEAD(&thqspi_drv->tx_list);
	INIT_LIST_HEAD(&thqspi_drv->rx_list);
	mutex_init(&thqspi_drv->xfer_lock);
	mutex_init(&thqspi_drv->state_lock);
	mutex_init(&thqspi_drv->read_lock);
	mutex_init(&thqspi_drv->hcr_lock);
	mutex_init(&thqspi_drv->mem_lock);
	mutex_init(&thqspi_drv->sleep_lock);

	thqspi_drv->kreader = kthread_create_worker(0, "kthread_thqspi_reader");
	if (IS_ERR(thqspi_drv->kreader)) {
		THQSPI_ERR(thqspi_drv, "%s failed to create reader worker thread", __func__);
		return PTR_ERR(thqspi_drv->kreader);
	}
	kthread_init_work(&thqspi_drv->read_msg, thqspi_read_msg);
	thqspi_drv->is_cold_reset_req = false;
	thqspi_drv->is_cold_reset_done = false;

	init_completion(&thqspi_drv->sync_wait);
	init_completion(&thqspi_drv->init_irq_wait);
	init_completion(&thqspi_drv->buff_wait);
	init_completion(&thqspi_drv->resume_wait);
	atomic_set(&thqspi_drv->check_resume_wait, FALSE);

	thqspi_drv->bh_work_wq = alloc_workqueue("%s", WQ_UNBOUND|WQ_HIGHPRI, 1, dev_name(dev));
	if (!thqspi_drv->bh_work_wq) {
			THQSPI_ERR(thqspi_drv, "%s: falied to alloc workqueue", __func__);
			destroy_workqueue(thqspi_drv->bh_work_wq);
			return -ENOMEM;
	}
	INIT_WORK(&thqspi_drv->bh_work, thqspi_handle_work);
	thqspi_drv->ipc = ipc_log_context_create(15, dev_name(dev), 0);
	if (!thqspi_drv->ipc && IS_ENABLED(CONFIG_IPC_LOGGING)) {
		dev_err(dev, "Error creating IPC logs");
	}

	thqspi_drv->user.is_active = false;
	dev_dbg(dev, "THQSPI driver probe successful");
	spi_set_drvdata(spi, thqspi_drv);
	THQSPI_INFO(thqspi_drv, "%s: probe successful", __func__);
#ifdef CONFIG_SLEEP
	pm_runtime_enable(thqspi_drv->dev);
	pm_runtime_set_autosuspend_delay(thqspi_drv->dev, THQSPI_AUTOSUSPEND_DELAY);
	pm_runtime_use_autosuspend(thqspi_drv->dev);
#endif
	return 0;

probe_err:
	THQSPI_ERR(thqspi_drv, "%s: probe failed with err = %d", __func__, ret);
	return ret;
}

static void thqspi_remove(struct spi_device *spi)
{
	if (!thqspi_drv) {
		pr_err("%s Nothing to remove", __func__);
		return;
	}

	THQSPI_DBG(thqspi_drv, "%s PID =%d", __func__, current->pid);
#ifdef CONFIG_SLEEP
	pm_runtime_disable(thqspi_drv->dev);
#endif
	destroy_workqueue(thqspi_drv->bh_work_wq);
	kthread_destroy_worker(thqspi_drv->kreader);
	device_destroy(thqspi_drv->chrdev.thqspi_class, MKDEV(thqspi_cdev_major, 0));
	cdev_del(&thqspi_drv->chrdev.c_dev);
	class_unregister(thqspi_drv->chrdev.thqspi_class);
	unregister_chrdev_region(MKDEV(thqspi_cdev_major, THREAD_MN_DEV_NUM), MINORMASK);
	class_destroy(thqspi_drv->chrdev.thqspi_class);
	devm_kfree(&spi->dev, thqspi_drv);
	spi_set_drvdata(spi, NULL);
	thqspi_drv = NULL;
	return;
}

static void thqspi_shutdown(struct spi_device *spi)
{
	thqspi_remove(spi);
	pr_err("%s PID =%d", __func__, current->pid);
}

static int thqspi_runtime_suspend(struct device *dev)
{
#ifdef CONFIG_SLEEP
	int ret = 0;

	THQSPI_DBG(thqspi_drv,"%s", __func__);
	if (thqspi_drv == NULL) {
		THQSPI_ERR(thqspi_drv, "%s: thqspi_drv is null", __func__);
		return 0;
	}
#endif
	THQSPI_ERR(thqspi_drv, "%s PID=%d", __func__, current->pid);
	return 0;
}

static int thqspi_runtime_resume(struct device *dev)
{
#ifdef CONFIG_SLEEP
	pr_err("%s", __func__);
	if (thqspi_drv == NULL) {
		pr_err("%s: thqspi_drv is null", __func__);
		return 0;
	}
#endif
	THQSPI_ERR(thqspi_drv, "%s PID=%d", __func__, current->pid);
	return 0;
}

static int thqspi_suspend(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_SLEEP
	if (thqspi_drv == NULL) {
		pr_err("%s: thqspi_drv is null", __func__);
		return 0;
	}

	THQSPI_ERR(thqspi_drv, "%s", __func__);
	if (!pm_runtime_status_suspended(thqspi_drv->dev)) {
		ret = thqspi_runtime_suspend(dev);
		if (ret != 0)
			THQSPI_ERR(thqspi_drv, "%s runtime suspend failed", __func__);
	}
	if (ret == 0) {
		atomic_set(&thqspi_drv->check_resume_wait, TRUE);
		reinit_completion(&thqspi_drv->resume_wait);
	}
#endif
	return ret;
}

static int thqspi_resume(struct device *dev)
{
#ifdef CONFIG_SLEEP
	pr_err("%s", __func__);

	if (thqspi_drv == NULL) {
		pr_err("%s: thqspi_drv is null", __func__);
		return 0;
	}

	if (!pm_runtime_status_suspended(thqspi_drv->dev)) {
		THQSPI_ERR(thqspi_drv, "%s client not suspended", __func__);
		return 0;
	}
	THQSPI_ERR(thqspi_drv, "%s: calling runtime resume", __func__);
	thqspi_runtime_resume(dev);
	THQSPI_ERR(thqspi_drv, "%s: Setting complete of resume", __func__);
	complete(&thqspi_drv->resume_wait);
	return 0;
#endif
	THQSPI_ERR(thqspi_drv, "%s PID=%d", __func__, current->pid);
	return 0;
}

static const struct dev_pm_ops thqspi_pm = {
	SET_RUNTIME_PM_OPS(thqspi_runtime_suspend,
			   thqspi_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(thqspi_suspend,
				thqspi_resume)
};

static const struct of_device_id thqspi_of_match[] = {
	{.compatible = "qcom,thqspi", },
	{}
};

static struct spi_driver thqspi_driver = {
	.driver = {
			.name = "thqspi",
			.of_match_table = thqspi_of_match,
			.pm = &thqspi_pm,
	},
	.probe = thqspi_probe,
	.remove = thqspi_remove,
	.shutdown =  thqspi_shutdown,
};

module_spi_driver(thqspi_driver);
MODULE_DESCRIPTION("thqspi driver");
MODULE_LICENSE("GPL v2");

