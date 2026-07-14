// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include "hfi_msm_dbg.h"
#include "hfi_props.h"
#include "hfi_utils.h"

#define DBG_INIT_TRY_COUNT 100
#define DBG_INIT_STEP_US 1000
#define HFI_DBG_BASE_PROP_MAX_SIZE 1024

#define DEFAULT_PANIC		1
#define DBG_CTRL_STOP_FTRACE	BIT(0)
#define DBG_CTRL_PANIC_UNDERRUN	BIT(1)

#define MAX_BUFF_SIZE ((3072 - 256) * 1024)
#define ROW_BYTES			16

struct hfi_msm_dbg *hfi_dbg;

static u32 _hfi_dbg_read_prop(u32 hfi_prop,
		u32 *payload, u32 max_words)
{
	u32 read = 0;
	u32 prop_id = HFI_PROP_ID(hfi_prop);

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg\n");
		return read;
	}

	switch (prop_id) {
	case HFI_PROPERTY_DEBUG_REG_ALLOC:
		hfi_dbg->buff_map.reg_addr.size = payload[read++];
		break;
	case HFI_PROPERTY_DEBUG_EVT_LOG_ALLOC:
		hfi_dbg->buff_map.evt_log_addr.size = payload[read++];
		break;
	case HFI_PROPERTY_DEBUG_DBG_BUS_ALLOC:
		hfi_dbg->buff_map.dbg_bus_addr.size = payload[read++];
		break;
	case HFI_PROPERTY_DEBUG_STATE_ALLOC:
		hfi_dbg->buff_map.device_state_addr.size = payload[read++];
		break;
	default:
		SDE_ERROR("Unknown property ID (%u)\n", prop_id);
	}

	return read;
}

static int hfi_dbg_parse_payload(void *payload, u32 size)
{
	struct hfi_util_kv_parser kv_parser;
	u32 prop, max_words, last_read = 0;
	u32 *value_ptr;
	int ret, read_sz;
	u32 read = 0;
	u32 *data = (u32 *)payload;
	u32 prop_count;

	prop_count = *data++;
	read++;

	read_sz = (sizeof(u32) * read);
	ret  = hfi_util_kv_parser_init(&kv_parser, size - read_sz, data);
	if (ret) {
		SDE_ERROR("failed to get int prop parser\n");
		return ret;
	}

	for (int i = 0; i < prop_count; i++) {
		ret = hfi_util_kv_parser_get_next(&kv_parser, last_read, &prop,
				&value_ptr, &max_words);
		if (ret) {
			SDE_ERROR("failed to get next prop %d\n", ret);
			return ret;
		}

		last_read = _hfi_dbg_read_prop(prop, value_ptr, max_words);
		if (!last_read) {
			SDE_ERROR("failed to get next prop\n");
			return ret;
		}
	}

	return ret;
}

static void hfi_dbg_property_handler(u32 display_id, u32 cmd_id,
		void *payload, u32 size, struct hfi_prop_listener *listener)
{
	if (!hfi_dbg) {
		SDE_ERROR("invalid object or listener from FW\n");
		return;
	}
	if (cmd_id == HFI_COMMAND_DEBUG_INIT) {
		if (!payload) {
			SDE_ERROR("No payload specified\n");
			return;
		}

		hfi_dbg_parse_payload(payload, size);

	} else {
		SDE_ERROR("invalid hfi command 0x%x\n", cmd_id);
	}
}

void _hfi_dump_dbg_bus(void)
{
	char buf[SDE_EVTLOG_BUF_MAX];
	bool in_mem, in_dump, in_log;
	char *dump_addr = NULL;
	int i;
	u32 dump_size, sz_read;

	if (!hfi_dbg->buff_map.dbg_bus_addr.local_addr || !hfi_dbg->buff_map.dbg_bus_addr.size)
		return;

	dump_addr = hfi_dbg->buff_map.dbg_bus_addr.local_addr;
	dump_size = hfi_dbg->buff_map.dbg_bus_addr.size;

	if (!dump_addr || !dump_size) {
		SDE_ERROR("unable to find mem_base\n");
		return;
	}

	in_mem = (hfi_dbg->dump_option & SDE_DBG_DUMP_IN_MEM);
	in_dump = (hfi_dbg->dump_option & SDE_DBG_DUMP_IN_COREDUMP);
	in_log = (hfi_dbg->dump_option & (SDE_DBG_DUMP_IN_LOG | SDE_DBG_DUMP_IN_LOG_LIMITED));

	if (!in_log && !in_mem && !in_dump)
		return;

	if (dump_addr && in_dump) {
		drm_printf(hfi_dbg->msm_dbg_printer,
			"===================dgb_bus================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < dump_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (dump_size - i > SDE_EVTLOG_BUF_MAX) ? SDE_EVTLOG_BUF_MAX :
					dump_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			drm_printf(hfi_dbg->msm_dbg_printer, "%s", buf);
		}
		mutex_unlock(&hfi_dbg->mutex);
		drm_printf(hfi_dbg->msm_dbg_printer, "\n");
	}

	if (in_log) {
		pr_info("===================dgb_bus================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < dump_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (dump_size - i > SDE_EVTLOG_BUF_MAX) ? SDE_EVTLOG_BUF_MAX :
					dump_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			pr_info("%s\n", buf);
		}
		mutex_unlock(&hfi_dbg->mutex);
		pr_info("\n");
	}

}

void _hfi_dump_reg_all(void)
{
	u32 in_log, in_mem, in_dump, sz_read;
	u32 *dump_addr = NULL;
	int i, dump_size;
	char buf[SDE_EVTLOG_BUF_MAX];

	if (!hfi_dbg->buff_map.reg_addr.local_addr || !hfi_dbg->buff_map.reg_addr.size)
		return;

	dump_addr = hfi_dbg->buff_map.reg_addr.local_addr;
	dump_size = hfi_dbg->buff_map.reg_addr.size;

	in_log = (hfi_dbg->dump_option & (SDE_DBG_DUMP_IN_LOG | SDE_DBG_DUMP_IN_LOG_LIMITED));
	in_mem = (hfi_dbg->dump_option & SDE_DBG_DUMP_IN_MEM);
	in_dump = (hfi_dbg->dump_option & SDE_DBG_DUMP_IN_COREDUMP);

	pr_debug("reg_dump_flag=%d in_log=%d in_mem=%d\n",
		hfi_dbg->dump_option, in_log, in_mem);

	if (!in_log && !in_mem && !in_dump)
		return;

	if (in_dump && dump_addr) {
		drm_printf(hfi_dbg->msm_dbg_printer,
			"===================reg_dump================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < dump_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (dump_size - i > SDE_EVTLOG_BUF_MAX) ? SDE_EVTLOG_BUF_MAX :
						dump_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			drm_printf(hfi_dbg->msm_dbg_printer, "%s", buf);
		}
		mutex_unlock(&hfi_dbg->mutex);
		drm_printf(hfi_dbg->msm_dbg_printer, "\n");
	}

	if (in_log) {
		pr_info("===================reg_dump================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < dump_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (dump_size - i > SDE_EVTLOG_BUF_MAX) ? SDE_EVTLOG_BUF_MAX :
						dump_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			pr_info("%s\n", buf);
		}
		mutex_unlock(&hfi_dbg->mutex);
		pr_info("\n");
	}

}

void hfi_evtlog_dump_all(struct sde_dbg_evtlog *evtlog)
{
	char buf[SDE_EVTLOG_BUF_MAX];
	bool update_last_entry = true;
	u32 in_log, in_mem, in_dump, sz_read;
	char *dump_addr = NULL;
	int i;

	if (!evtlog->dumped_evtlog || !evtlog->log_size)
		return;

	in_log = evtlog->dump_mode & (SDE_DBG_DUMP_IN_LOG | SDE_DBG_DUMP_IN_LOG_LIMITED);
	in_mem = evtlog->dump_mode & SDE_DBG_DUMP_IN_MEM;
	in_dump = evtlog->dump_mode & SDE_DBG_DUMP_IN_COREDUMP;

	if (!in_log && !in_mem && !in_dump)
		return;

	dump_addr = evtlog->dumped_evtlog;

	if (in_dump && dump_addr) {
		drm_printf(hfi_dbg->msm_dbg_printer, "===================evtlog================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < evtlog->log_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (evtlog->log_size - i > SDE_EVTLOG_BUF_MAX) ?
				SDE_EVTLOG_BUF_MAX : evtlog->log_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			drm_printf(hfi_dbg->msm_dbg_printer, "%s", buf);
		}
		mutex_unlock(&hfi_dbg->mutex);
		drm_printf(hfi_dbg->msm_dbg_printer, "\n");
	}

	if (in_log) {
		pr_info("===================evtlog================\n");
		mutex_lock(&hfi_dbg->mutex);
		for (i = 0; i < evtlog->log_size; i += SDE_EVTLOG_BUF_MAX) {
			sz_read = (evtlog->log_size - i > SDE_EVTLOG_BUF_MAX) ?
				SDE_EVTLOG_BUF_MAX : evtlog->log_size - i;
			memcpy(buf, dump_addr + i, sz_read);
			pr_info("%s\n", buf);
			update_last_entry = false;
		}
		mutex_unlock(&hfi_dbg->mutex);
		pr_info("\n");
	}
}

/**
 * _hfi_dump_all - dumps all registers, eventlogs, debugbus
 * @do_panic: whether to trigger a panic after dumping
 * @name: string indicating origin of dump
 * @dump_secure: flag to indicate dumping in secure-session
 * @dump_blk_mask: mask of all the hw blk-ids that has to be dumped
 */
static void _hfi_dump_all(bool do_panic, const char *name, bool dump_secure,
		u64 dump_blk_mask)
{
	ktime_t start, end;
	u32 in_dump = 0;

	if ((hfi_dbg->evtlog->enable & SDE_EVTLOG_ALWAYS))
		hfi_evtlog_dump_all(hfi_dbg->evtlog);

	in_dump = (hfi_dbg->dump_option);

	start = ktime_get();
	_hfi_dump_reg_all();
	end = ktime_get();

	dev_info(hfi_dbg->dev,
		"reg-dump logging time start_us:%llu, end_us:%llu , duration_us:%llu\n",
		ktime_to_us(start), ktime_to_us(end), ktime_us_delta(end, start));

	start = ktime_get();
	_hfi_dump_dbg_bus();
	end = ktime_get();

	dev_info(hfi_dbg->dev,
			"debug-bus logging time start_us:%llu, end_us:%llu , duration_us:%llu\n",
			ktime_to_us(start), ktime_to_us(end), ktime_us_delta(end, start));

	if (do_panic && hfi_dbg->panic_on_err && (!in_dump))
		panic(name);
}

void hfi_dbg_dump(enum sde_dbg_dump_flag dump_mode, const char *name, u64 dump_blk_mask, ...)
{
	bool do_panic = false;

	bool dump_secure = false;
	struct drm_print_iterator iter;
	struct drm_printer p;

	iter.data = hfi_dbg->read_buf;
	iter.start = 0;
	iter.remain = MAX_BUFF_SIZE;

	p = drm_coredump_printer(&iter);
	drm_printf(&p, "---\n");
	drm_printf(&p, "module: " KBUILD_MODNAME "\n");
	drm_printf(&p, "HFI dump\n");

	hfi_dbg->msm_dbg_printer = &p;

	if (!(hfi_dbg->evtlog->enable & SDE_EVTLOG_ALWAYS))
		return;

	hfi_dbg->evtlog->dump_mode = dump_mode;
	hfi_dbg->dump_option = dump_mode;

	_hfi_dump_all(do_panic, name, dump_secure, dump_blk_mask);

}

int hfi_dbg_device_setup(struct hfi_kms *hfi_kms)
{
	struct hfi_cmdbuf_t *cmd_buf;
	int ret;
	u64 prop_val;
	struct hfi_prop_u64 prop_u64;

	if (!hfi_dbg || !hfi_kms) {
		SDE_ERROR("Invalid params\n");
		return -EINVAL;
	}

	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		SDE_ERROR("failed to get hfi command buffer\n");
		return -EINVAL;
	}

	if (hfi_dbg->buff_map.reg_addr.size && hfi_dbg->buff_map.reg_addr.remote_addr) {
		prop_val = (u64) hfi_dbg->buff_map.reg_addr.remote_addr;
		prop_u64.val_lo = HFI_VAL_L32(prop_val);
		prop_u64.val_hi = HFI_VAL_H32(prop_val);
		hfi_util_u32_prop_helper_add_prop(hfi_dbg->base_props, HFI_PROPERTY_DEBUG_REG_ADDR,
				HFI_VAL_U32_ARRAY, &prop_u64, sizeof(struct hfi_prop_u64));
	}

	if (hfi_dbg->buff_map.evt_log_addr.size && hfi_dbg->buff_map.evt_log_addr.remote_addr) {
		prop_val = (u64) hfi_dbg->buff_map.evt_log_addr.remote_addr;
		prop_u64.val_lo = HFI_VAL_L32(prop_val);
		prop_u64.val_hi = HFI_VAL_H32(prop_val);
		hfi_util_u32_prop_helper_add_prop(hfi_dbg->base_props,
			HFI_PROPERTY_DEBUG_EVT_LOG_ADDR, HFI_VAL_U32_ARRAY,
			&prop_u64, sizeof(struct hfi_prop_u64));
	}

	if (hfi_dbg->buff_map.dbg_bus_addr.size &&
		hfi_dbg->buff_map.dbg_bus_addr.remote_addr) {
		prop_val = (u64) hfi_dbg->buff_map.dbg_bus_addr.remote_addr;
		prop_u64.val_lo = HFI_VAL_L32(prop_val);
		prop_u64.val_hi = HFI_VAL_H32(prop_val);
		hfi_util_u32_prop_helper_add_prop(hfi_dbg->base_props,
			HFI_PROPERTY_DEBUG_DBG_BUS_ADDR, HFI_VAL_U32_ARRAY,
			&prop_u64, sizeof(struct hfi_prop_u64));
	}

	if (hfi_dbg->buff_map.device_state_addr.size &&
		hfi_dbg->buff_map.device_state_addr.remote_addr) {
		prop_val = (u64) hfi_dbg->buff_map.device_state_addr.remote_addr;
		prop_u64.val_lo = HFI_VAL_L32(prop_val);
		prop_u64.val_hi = HFI_VAL_H32(prop_val);
		hfi_util_u32_prop_helper_add_prop(hfi_dbg->base_props,
			HFI_PROPERTY_DEBUG_STATE_ADDR, HFI_VAL_U32_ARRAY,
			&prop_u64, sizeof(struct hfi_prop_u64));
	}

	ret = hfi_adapter_add_set_property(cmd_buf,
			HFI_COMMAND_DEBUG_SETUP,
			MSM_DRV_HFI_ID,
			HFI_PAYLOAD_TYPE_U32_ARRAY,
			hfi_util_u32_prop_helper_get_payload_addr(hfi_dbg->base_props),
			hfi_util_u32_prop_helper_get_size(hfi_dbg->base_props),
			HFI_HOST_FLAGS_NONE);

	ret = hfi_adapter_set_cmd_buf(cmd_buf);
	SDE_EVT32(MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_SETUP, ret, SDE_EVTLOG_FUNC_CASE1);
	if (ret) {
		SDE_ERROR("failed to send debug-init command\n");
		return ret;
	}

	return ret;
}

static int hfi_dbg_recovery_reg_open(struct inode *inode, struct file *file)
{
	if (!inode || !file)
		return -EINVAL;

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg!\n");
		return -EINVAL;
	}

	hfi_dbg->recovery_off = 0;
	if (!hfi_dbg->dump_recovery) {
		hfi_dbg->dump_recovery = kvzalloc(PAGE_SIZE, GFP_KERNEL);
		if (!hfi_dbg->dump_recovery)
			return -ENOMEM;

		SDE_ERROR("allocated dump buf\n");
	}
	memset(hfi_dbg->dump_recovery, 0, PAGE_SIZE);

	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	file->private_data = hfi_dbg;
	return 0;
}

static ssize_t hfi_dbg_dump_reg_read(struct file *file,
			char __user *user_buf, size_t count, loff_t *ppos)
{
	ssize_t len = 0, read_count = 0;
	struct hfi_msm_dbg *dbg;
	u32 *dump_addr;
	ssize_t buf_len = PAGE_SIZE;
	size_t sz_left;
	int i;
	bool is_valid_addr = true;

	dbg = file->private_data;
	if (!dbg) {
		SDE_ERROR("invalid handle\n");
		return -ENODEV;
	}

	if (!user_buf || !ppos || !hfi_dbg->buff_map.reg_addr.local_addr)
		return 0;

	dump_addr = hfi_dbg->buff_map.reg_addr.local_addr + hfi_dbg->recovery_off;
	sz_left = hfi_dbg->buff_map.reg_addr.size - hfi_dbg->recovery_off;
	read_count = min(count, sz_left);

	if (hfi_dbg->recovery_off >= hfi_dbg->buff_map.reg_addr.size)
		return 0;

	if (!dump_addr || sz_left <= 0)
		return 0;

	mutex_lock(&hfi_dbg->mutex);
	for (i = 0; i < read_count; i += sizeof(u32)) {
		if (!dump_addr) {
			is_valid_addr = false;
			break;
		}
		len += scnprintf(hfi_dbg->dump_recovery + len, buf_len - len, " %08x",
					*dump_addr);
		dump_addr++;
	}
	mutex_unlock(&hfi_dbg->mutex);

	if (len > count) {
		SDE_ERROR("exceeding length for debugfs recovery read\n");
		return -EINVAL;
	}

	if (copy_to_user(user_buf, hfi_dbg->dump_recovery, len))
		len = -EFAULT;

	*ppos += len;
	if (is_valid_addr)
		hfi_dbg->recovery_off += read_count;
	else
		hfi_dbg->recovery_off = hfi_dbg->buff_map.reg_addr.size;

	SDE_ERROR("debugfs data for len:%zd bytes count:%zu dbg->off:%d sz_left: %zu\n",
		len, count, hfi_dbg->recovery_off, sz_left);

	return len;
}

static const struct file_operations dump_recovery_reg_fops = {
	.open = hfi_dbg_recovery_reg_open,
	.read = hfi_dbg_dump_reg_read,
};

static ssize_t hfi_dbg_dump_info_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	size_t sz_left;
	void __iomem *dump_addr;

	if (!buff || !ppos || !hfi_dbg->buff_map.evt_log_addr.local_addr)
		return -EINVAL;

	dump_addr = hfi_dbg->buff_map.evt_log_addr.local_addr;
	sz_left = hfi_dbg->buff_map.evt_log_addr.size - *ppos;
	len = min(count, sz_left);

	if (sz_left == 0)
		return 0;

	mutex_lock(&hfi_dbg->mutex);
	if (copy_to_user(buff, dump_addr + *ppos, len))
		len = -EFAULT;

	mutex_unlock(&hfi_dbg->mutex);

	if (len < 0 || len > count) {
		SDE_ERROR("len is more than user buffer size\n");
		return 0;
	}

	*ppos += len;

	return len;
}

static const struct file_operations dump_dbg_info_fops = {
	.open = simple_open,
	.read = hfi_dbg_dump_info_read,
};

static int _hfi_dbg_reg_base_register(struct hfi_msm_dbg *dbg)
{
	struct msm_dbg_reg_base *reg_base = NULL;

	if (!dbg) {
		SDE_ERROR("invalid pointer to hfi_debug\n");
		return -EINVAL;
	}

	reg_base = kvzalloc(sizeof(*reg_base), GFP_KERNEL);
	if (!reg_base)
		return -ENOMEM;

	if (!dbg->buff_map.reg_addr.size) {
		SDE_ERROR("Shared buffer has invalid size\n");
		return -EINVAL;
	}

	reg_base->off = 0;
	reg_base->cnt = 0;
	reg_base->reg_addr = &hfi_dbg->buff_map.reg_addr;

	dbg->reg_base = reg_base;

	return 0;
}

/*
 * hfi_dbg_reg_base_open - debugfs open handler for reg base
 * @inode: debugfs node
 * @file: file handle
 */
static int hfi_dbg_reg_base_open(struct inode *inode, struct file *file)
{
	int rc = 0;

	if (!inode || !file)
		return -EINVAL;

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg!\n");
		return -EINVAL;
	}

	if (!hfi_dbg->reg_base) {
		rc = _hfi_dbg_reg_base_register(hfi_dbg);
		if (rc) {
			SDE_ERROR("Failed to create register base structure, error %d\n", rc);
			return rc;
		}
	}

	hfi_dbg->dump_len = 0;
	if (!hfi_dbg->dump_buf) {
		hfi_dbg->dump_buf = kvzalloc(PAGE_SIZE, GFP_KERNEL);
		if (!hfi_dbg->dump_buf)
			return -ENOMEM;

		SDE_ERROR("allocated dump buf\n");
	}


	memset(hfi_dbg->dump_buf, 0, PAGE_SIZE);

	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	file->private_data = hfi_dbg->reg_base;
	return 0;
}

/**
 * hfi_dbg_reg_base_offset_read - read current offset and len of register base
 * @file: file handler
 * @user_buf: user buffer content from debugfs
 * @count: size of user buffer
 * @ppos: position offset of user buffer
 */
static ssize_t hfi_dbg_reg_base_offset_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	struct msm_dbg_reg_base *dbg;
	int len = 0;
	char buf[24] = {'\0'};

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg\n");
		return -EINVAL;
	}

	if (!file)
		return -EINVAL;

	dbg = file->private_data;
	if (!dbg)
		return -ENODEV;

	if (*ppos)
		return 0; /* the end */

	mutex_lock(&hfi_dbg->mutex);
	if (dbg->off % sizeof(u32)) {
		mutex_unlock(&hfi_dbg->mutex);
		return -EFAULT;
	}

	len = scnprintf(buf, sizeof(buf), "0x%08zx %zx\n", dbg->off, dbg->cnt);
	if (len < 0 || len >= sizeof(buf)) {
		mutex_unlock(&hfi_dbg->mutex);
		return 0;
	}

	if ((count < sizeof(buf)) || copy_to_user(buff, buf, len)) {
		mutex_unlock(&hfi_dbg->mutex);
		return -EFAULT;
	}

	*ppos += len; /* increase offset */
	mutex_unlock(&hfi_dbg->mutex);

	return len;
}

/**
 * hfi_dbg_reg_base_offset_write - set new offset and len to debugfs reg base
 * @file: file handler
 * @user_buf: user buffer content from debugfs
 * @count: size of user buffer
 * @ppos: position offset of user buffer
 */
static ssize_t hfi_dbg_reg_base_offset_write(struct file *file,
			const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct msm_dbg_reg_base *dbg;
	u32 off = 0;
	u32 cnt = 0;
	u32 max_count;
	char buf[24];

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg\n");
		return -EINVAL;
	}

	if (!file)
		return -EINVAL;

	dbg = file->private_data;
	if (!dbg)
		return -ENODEV;

	if (count >= sizeof(buf))
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = 0; /* end of string */

	if (sscanf(buf, "%8x %x", &off, &cnt) != 2)
		return -EFAULT;

	if (off % sizeof(u32))
		return -EINVAL;

	if (cnt == 0)
		return -EINVAL;

	max_count = PAGE_SIZE / 64;
	if (cnt > max_count) {
		SDE_ERROR("exceeding buffer limit:%d requested count:%d\n", max_count, cnt);
		cnt = max_count;
	}

	mutex_lock(&hfi_dbg->mutex);
	dbg->off = off;
	dbg->cnt = cnt;
	mutex_unlock(&hfi_dbg->mutex);

	SDE_DEBUG("offset=%x cnt=%x\n", off, cnt);

	return count;
}

static const struct file_operations dump_dbg_off_fops = {
	.open = hfi_dbg_reg_base_open,
	.read = hfi_dbg_reg_base_offset_read,
	.write = hfi_dbg_reg_base_offset_write,
};

static void hfi_dbg_reg_read_handler(u32 display_id, u32 cmd_id,
		void *payload, u32 size, struct hfi_prop_listener *listener)
{
	/* Listener function required to use buffer blocking */
	if (cmd_id != HFI_COMMAND_DEBUG_DUMP_REGS) {
		SDE_ERROR("Invalid command\n");
		return;
	}
}

/**
 * hfi_dbg_reg_base_reg_read - read len from reg base at current offset
 * @file: file handler
 * @user_buf: user buffer content from debugfs
 * @count: size of user buffer
 * @ppos: position offset of user buffer
 */
static ssize_t hfi_dbg_reg_base_reg_read(struct file *file,
			char __user *user_buf, size_t count, loff_t *ppos)
{
	struct msm_dbg_reg_base *dbg;
	struct hfi_cmdbuf_t *cmd_buf;
	struct platform_device *pdev = NULL;
	struct drm_device *ddev = NULL;
	struct msm_drm_private *priv = NULL;
	struct sde_kms *kms;
	struct hfi_kms *hfi_kms;
	struct regdump_info *payload;
	u64 buff_addr;
	u32 cur_offset = 0;
	u32 *local_regs;
	ssize_t len = 0;
	ssize_t buf_len = PAGE_SIZE;
	int i, max_rows, rc;

	if (!file)
		return -EINVAL;

	if (!hfi_dbg) {
		SDE_ERROR("invalid pointer to hfi_dbg\n");
		return -EINVAL;
	}

	pdev = to_platform_device(hfi_dbg->dev);
	ddev = platform_get_drvdata(pdev);

	if (!ddev || !ddev->dev_private) {
		SDE_ERROR("Invalid drm device node\n");
		return -EINVAL;
	}

	priv = ddev->dev_private;

	kms = to_sde_kms(priv->kms);
	hfi_kms = to_hfi_kms(kms);

	dbg = file->private_data;
	if (!dbg) {
		SDE_ERROR("invalid handle\n");
		return -ENODEV;
	}

	if (!user_buf || !ppos)
		return -EINVAL;

	/* edge condition - dump is done. return 0 */
	if (hfi_dbg->dump_len >= dbg->off)
		return 0;

	/* ask fw for dump only once */
	if (!hfi_dbg->dump_len) {
		cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
				MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
		if (!cmd_buf) {
			SDE_ERROR("failed to get hfi command buffer\n");
			return -EINVAL;
		}

		payload = kmalloc(sizeof(struct regdump_info), GFP_KERNEL);
		// payload->device_id = MSM_DRV_HFI_ID; NOT REQUIRED.
		payload->reg_offset = dbg->off;
		payload->length = dbg->cnt * sizeof(u32);

		buff_addr = (u64) hfi_dbg->buff_map.reg_addr.remote_addr;
		if (!(buff_addr > 0)) {
			SDE_ERROR("Invalid remote buffer address\n");
			return -EINVAL;
		}
		payload->buffer_info.addr_l = HFI_VAL_L32(buff_addr);
		payload->buffer_info.addr_h = HFI_VAL_H32(buff_addr);
		payload->buffer_info.size = hfi_dbg->buff_map.reg_addr.size;

		hfi_dbg->reg_read_cb_obj.hfi_prop_handler = hfi_dbg_reg_read_handler;

		rc = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEBUG_DUMP_REGS,
				MSM_DRV_HFI_ID, HFI_PAYLOAD_TYPE_U32_ARRAY, payload,
				sizeof(*payload), &hfi_dbg->reg_read_cb_obj,
				HFI_HOST_FLAGS_RESPONSE_REQUIRED);
		if (rc) {
			SDE_ERROR("Failed to add property\n");
			return rc;
		}

		SDE_EVT32(MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_DUMP_REGS, SDE_EVTLOG_FUNC_CASE1);
		rc = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
		SDE_EVT32(MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_DUMP_REGS, rc, SDE_EVTLOG_FUNC_CASE2);
		if (rc) {
			SDE_ERROR("failed to send debug-dump-regs command\n");
			return rc;
		}
	}

	if (!hfi_dbg->buff_map.reg_addr.local_addr)
		return 0;

	local_regs = hfi_dbg->buff_map.reg_addr.local_addr;
	max_rows = (dbg->cnt / 4);
	for (i = 0; i <= max_rows; i++) {
		int j = dbg->cnt % 4; // columns in last line

		SDE_ERROR("cur_offset:0x%x dbg->off:0x%zx\n", cur_offset, dbg->off);

		if ((i < max_rows) || j)
			len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len,
					"0x%08zx:", (cur_offset + dbg->off));

		if ((i < max_rows) || j)
			len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len, " %08x",
					*local_regs);
		if ((i  < max_rows) || (j > 1))
			len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len, " %08x",
					*(local_regs + 1));
		if ((i < max_rows) || (j > 2))
			len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len, " %08x",
					*(local_regs + 2));
		if (i < max_rows)
			len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len, " %08x",
					*(local_regs + 3));

		len += scnprintf(hfi_dbg->dump_buf + len, buf_len - len, "\n");
		local_regs += 0x4;
	}

	if (len > count) {
		SDE_ERROR("exceeding length for debugfs reg read\n");
		return -EINVAL;
	}

	SDE_ERROR("debugfs data for len:%zd bytes count:%zu dbg->cnt:%zd\n", len, count, dbg->cnt);
	mutex_lock(&hfi_dbg->mutex);
	if (copy_to_user(user_buf, hfi_dbg->dump_buf, len))
		len = -EFAULT;
	mutex_unlock(&hfi_dbg->mutex);

	*ppos += len;
	hfi_dbg->dump_len = dbg->off;

	return len;
}

static const struct file_operations dump_dbg_reg_fops = {
	.open = hfi_dbg_reg_base_open,
	.read = hfi_dbg_reg_base_reg_read,
};

static ssize_t hfi_dbg_bus_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	size_t sz_left;
	void __iomem *dump_addr;

	if (!buff || !ppos || !hfi_dbg->buff_map.dbg_bus_addr.local_addr)
		return 0;

	dump_addr = hfi_dbg->buff_map.dbg_bus_addr.local_addr;
	sz_left = hfi_dbg->buff_map.dbg_bus_addr.size - *ppos;
	len = min(count, sz_left);

	if (sz_left == 0)
		return 0;

	mutex_lock(&hfi_dbg->mutex);
	if (copy_to_user(buff, dump_addr + *ppos, len))
		len = -EFAULT;
	mutex_unlock(&hfi_dbg->mutex);

	*ppos += len;

	return len;
}

static const struct file_operations dump_dbg_bus_info_fops = {
	.open = simple_open,
	.read = hfi_dbg_bus_read,
};

static ssize_t hfi_dbg_dev_state_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	size_t sz_left;
	void __iomem *dump_addr;

	if (!buff || !ppos || !hfi_dbg->buff_map.device_state_addr.local_addr)
		return 0;

	dump_addr = hfi_dbg->buff_map.device_state_addr.local_addr;
	sz_left = hfi_dbg->buff_map.device_state_addr.size - *ppos;
	len = min(count, sz_left);

	if (sz_left == 0)
		return 0;

	mutex_lock(&hfi_dbg->mutex);
	if (copy_to_user(buff, dump_addr + *ppos, len))
		len = -EFAULT;
	mutex_unlock(&hfi_dbg->mutex);

	*ppos += len;

	return len;
}

static const struct file_operations dump_dev_state_fops = {
	.open = simple_open,
	.read = hfi_dbg_dev_state_read,
};

void hfi_dbg_ctrl(const char *name, ...)
{
	int i = 0;
	va_list args;
	char *blk_name = NULL;

	/* no debugfs controlled events are enabled, just return */
	if (!hfi_dbg->debugfs_ctrl)
		return;

	va_start(args, name);

	while ((blk_name = va_arg(args, char*))) {
		if (i++ >= SDE_EVTLOG_MAX_DATA) {
			SDE_ERROR("could not parse all dbg arguments\n");
			break;
		}

		if (IS_ERR_OR_NULL(blk_name))
			break;

		if (!strcmp(blk_name, "stop_ftrace") &&
				hfi_dbg->debugfs_ctrl & DBG_CTRL_STOP_FTRACE) {
			pr_debug("tracing off\n");
			tracing_off();
		}

		if (!strcmp(blk_name, "panic_underrun") &&
				hfi_dbg->debugfs_ctrl & DBG_CTRL_PANIC_UNDERRUN) {
			SDE_ERROR("panic underrun\n");
			MDSS_DBG_DUMP(SDE_DBG_BUILT_IN_ALL, "panic");
		}

	}

	va_end(args);
}

/**
 * hfi_dbg_ctrl_read - debugfs read handler for debug ctrl read
 * @file: file handler
 * @buff: user buffer content for debugfs
 * @count: size of user buffer
 * @ppos: position offset of user buffer
 */
static ssize_t hfi_dbg_ctrl_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	char buf[24] = {'\0'};

	if (!buff || !ppos)
		return -EINVAL;

	if (*ppos)
		return 0;	/* the end */

	len = scnprintf(buf, sizeof(buf), "0x%x\n", hfi_dbg->debugfs_ctrl);
	pr_debug("%s: ctrl:0x%x len:0x%zx\n",
		__func__, hfi_dbg->debugfs_ctrl, len);

	if (len < 0 || len >= sizeof(buf))
		return 0;

	if ((count < sizeof(buf)) || copy_to_user(buff, buf, len)) {
		SDE_ERROR("error copying the buffer! count:0x%zx\n", count);
		return -EFAULT;
	}

	*ppos += len;	/* increase offset */
	return len;
}

/**
 * hfi_dbg_ctrl_write - debugfs read handler for debug ctrl write
 * @file: file handler
 * @user_buf: user buffer content from debugfs
 * @count: size of user buffer
 * @ppos: position offset of user buffer
 */
static ssize_t hfi_dbg_ctrl_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	u32 dbg_ctrl = 0;
	char buf[24];

	if (!file) {
		SDE_ERROR("DbgDbg: %s: error no file --\n", __func__);
		return -EINVAL;
	}

	if (count >= sizeof(buf))
		return -EFAULT;


	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = 0; /* end of string */

	if (kstrtouint(buf, 0, &dbg_ctrl)) {
		SDE_ERROR("%s: error in the number of bytes\n", __func__);
		return -EFAULT;
	}

	pr_debug("dbg_ctrl_read:0x%x\n", dbg_ctrl);
	hfi_dbg->debugfs_ctrl = dbg_ctrl;

	return count;
}

static const struct file_operations dump_dbg_ctrl_fops = {
	.open = simple_open,
	.read = hfi_dbg_ctrl_read,
	.write = hfi_dbg_ctrl_write,
};

int hfi_msm_dbg_init(struct device *dev, struct dentry *debugfs_root)
{
	int ret;
	struct hfi_cmdbuf_t *cmd_buf;
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *ddev = platform_get_drvdata(pdev);
	struct msm_drm_private *priv = NULL;
	struct sde_kms *kms;
	struct hfi_kms *hfi_kms;

	if (!ddev || !ddev->dev_private) {
		SDE_ERROR("Invalid drm device node\n");
		return -EINVAL;
	}
	priv = ddev->dev_private;

	kms = to_sde_kms(priv->kms);
	hfi_kms = to_hfi_kms(kms);

	priv->debug_root = debugfs_root;

	hfi_dbg = kvzalloc(sizeof(struct hfi_msm_dbg), GFP_KERNEL);
	if (!hfi_dbg) {
		SDE_ERROR("failed to allocate hfi_dbg\n");
		return -EINVAL;
	}

	if (!hfi_dbg->read_buf) {
		hfi_dbg->read_buf = kvzalloc(MAX_BUFF_SIZE, GFP_KERNEL);
		if (!hfi_dbg->read_buf)
			SDE_ERROR("failed to allocated dump buffer\n");
	}

	hfi_dbg->evtlog = sde_evtlog_init();

	hfi_dbg->dev = dev;
	hfi_dbg->debugfs_root = debugfs_root;

	mutex_init(&hfi_dbg->mutex);
	hfi_dbg->buff_map.reg_addr.size = 0;
	hfi_dbg->buff_map.evt_log_addr.size = 0;
	hfi_dbg->buff_map.dbg_bus_addr.size = 0;
	hfi_dbg->buff_map.device_state_addr.size = 0;
	hfi_dbg->panic_on_err = DEFAULT_PANIC;
	hfi_dbg->dump_option = SDE_DBG_DEFAULT_DUMP_MODE;
	hfi_dbg->evtlog->enable = SDE_EVTLOG_DEFAULT_ENABLE;
	hfi_dbg->evtlog->dump_mode = SDE_DBG_DEFAULT_DUMP_MODE;

	hfi_dbg->base_props = hfi_util_u32_prop_helper_alloc(HFI_DBG_BASE_PROP_MAX_SIZE);
	if (IS_ERR(hfi_dbg->base_props)) {
		SDE_ERROR("failed to allocate memory for base prop collector\n");
		ret = PTR_ERR(hfi_dbg->base_props);
		goto free_kv;
	}

	cmd_buf = hfi_adapter_get_cmd_buf(&hfi_kms->hfi_client,
			MSM_DRV_HFI_ID, HFI_CMDBUF_TYPE_GET_DEBUG_DATA);
	if (!cmd_buf) {
		SDE_ERROR("failed to get hfi command buffer\n");
		return -EINVAL;
	}

	hfi_dbg->hfi_cb_obj.hfi_prop_handler = hfi_dbg_property_handler;
	ret = hfi_adapter_add_get_property(cmd_buf, HFI_COMMAND_DEBUG_INIT,
			MSM_DRV_HFI_ID, HFI_PAYLOAD_TYPE_NONE, NULL, 0,
			&hfi_dbg->hfi_cb_obj, HFI_HOST_FLAGS_RESPONSE_REQUIRED);
	if (ret) {
		SDE_ERROR("failed to add debug-init command\n");
		return ret;
	}

	SDE_EVT32(MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_INIT, SDE_EVTLOG_FUNC_CASE1);
	ret = hfi_adapter_set_cmd_buf_blocking(cmd_buf);
	SDE_EVT32(MSM_DRV_HFI_ID, HFI_COMMAND_DEBUG_INIT, ret, SDE_EVTLOG_FUNC_CASE2);
	if (ret) {
		SDE_ERROR("failed to send debug-init command\n");
		return ret;
	}

	if (hfi_dbg->buff_map.reg_addr.size)
		hfi_adapter_buffer_alloc(&hfi_dbg->buff_map.reg_addr);
	if (hfi_dbg->buff_map.evt_log_addr.size)
		hfi_adapter_buffer_alloc(&hfi_dbg->buff_map.evt_log_addr);
	if (hfi_dbg->buff_map.dbg_bus_addr.size)
		hfi_adapter_buffer_alloc(&hfi_dbg->buff_map.dbg_bus_addr);
	if (hfi_dbg->buff_map.device_state_addr.size)
		hfi_adapter_buffer_alloc(&hfi_dbg->buff_map.device_state_addr);

	ret = hfi_dbg_device_setup(hfi_kms);
	if (ret) {
		SDE_ERROR("failed to send debug-setup command\n");
		return ret;
	}

	hfi_dbg->evtlog->log_size = hfi_dbg->buff_map.evt_log_addr.size;
	hfi_dbg->evtlog->dumped_evtlog = hfi_dbg->buff_map.evt_log_addr.local_addr;

	debugfs_create_file("recovery_reg", 0600, debugfs_root, NULL, &dump_recovery_reg_fops);
	debugfs_create_file("dump", 0600, debugfs_root, NULL, &dump_dbg_info_fops);
	debugfs_create_file("recovery_dbgbus", 0600, debugfs_root, NULL, &dump_dbg_bus_info_fops);
	debugfs_create_file("recovery_dev_state", 0600, debugfs_root, NULL, &dump_dev_state_fops);

	debugfs_create_file("dbg_ctrl", 0600, debugfs_root, NULL, &dump_dbg_ctrl_fops);
	debugfs_create_u32("enable", 0600, debugfs_root, &(hfi_dbg->evtlog->enable));
	debugfs_create_u32("panic", 0600, debugfs_root, &hfi_dbg->panic_on_err);
	debugfs_create_u32("dump_mode", 0600, debugfs_root, &hfi_dbg->dump_option);
	debugfs_create_u32("evtlog_dump", 0600, debugfs_root, &(hfi_dbg->evtlog->dump_mode));

	debugfs_create_file("mdss_off", 0600, debugfs_root, NULL, &dump_dbg_off_fops);
	debugfs_create_file("mdss_reg", 0600, debugfs_root, NULL, &dump_dbg_reg_fops);

	return ret;
free_kv:
	kfree(hfi_dbg->base_props);

	return -EINVAL;
}

void hfi_msm_dbg_destroy(void)
{
	if (!hfi_dbg)
		return;

	hfi_adapter_buffer_dealloc(&hfi_dbg->buff_map.reg_addr);
	hfi_adapter_buffer_dealloc(&hfi_dbg->buff_map.evt_log_addr);
	hfi_adapter_buffer_dealloc(&hfi_dbg->buff_map.dbg_bus_addr);
	hfi_adapter_buffer_dealloc(&hfi_dbg->buff_map.device_state_addr);

	mutex_destroy(&hfi_dbg->mutex);
	kfree(hfi_dbg->read_buf);
	kfree(hfi_dbg);
}
