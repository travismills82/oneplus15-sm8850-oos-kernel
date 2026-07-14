// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Oplus. All rights reserved.
 */

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/spinlock.h>

#include "game_ctrl.h"
#include "hybrid_frame_sync.h"
#include "task_boost/heavy_task_boost.h"
#include "critical_task_boost.h"

#define DECLARE_DEBUG_TRACE(name, proto, data)			\
	static void __maybe_unused debug_##name(proto) {	\
		if (unlikely(g_debug_enable)) {			\
			name(data);				\
		}						\
	}
#include "debug_common.h"
#undef DECLARE_DEBUG_TRACE

#define BUFFER_CAPACITY (1 << 4)
#define MAX_BUFFER_SIZE (BUFFER_CAPACITY - 1)

DEFINE_CTL_TABLE_POLL(hybrid_frame_poll);

/************************** symbols ************************/

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_FDRIVE)
extern void (*game_bwmon_on_frame_event)(int cpu, int event);
#endif /* CONFIG_OPLUS_FEATURE_GEAS_FDRIVE */

/************************** data ops ************************/

static DEFINE_RWLOCK(hfs_rwlock);

static int buf_begin = 0;
static int buf_end = 0;
static struct hybrid_frame_data event_buffer[BUFFER_CAPACITY];

static void init_hfs_data(void)
{
	buf_begin = 0;
	buf_end = 0;
	memset(event_buffer, 0, sizeof(event_buffer));
}

static int buffer_size(void)
{
	return (buf_end + BUFFER_CAPACITY - buf_begin) & MAX_BUFFER_SIZE;
}

static struct hybrid_frame_data __maybe_unused *buffer_at(int index)
{
	if (index >= buffer_size()) {
		return NULL;
	}
	return &event_buffer[(buf_begin + index) & MAX_BUFFER_SIZE];
}

static struct hybrid_frame_data *buffer_front(void)
{
	if (buf_begin == buf_end) {
		return NULL;
	}
	return &event_buffer[buf_begin];
}

static struct hybrid_frame_data __maybe_unused *buffer_back(void)
{
	if (buf_begin == buf_end) {
		return NULL;
	}
	return &event_buffer[(buf_end - 1) & MAX_BUFFER_SIZE];
}

static void buffer_pop(void)
{
	if (buf_begin == buf_end) {
		return;
	}
	buf_begin = (buf_begin + 1) & MAX_BUFFER_SIZE;
}

static void buffer_pop_back(void)
{
	if (buf_begin == buf_end) {
		return;
	}
	buf_end = (buf_end - 1) & MAX_BUFFER_SIZE;
}

static bool __maybe_unused buffer_push(struct hybrid_frame_data *data)
{
	if (!data || buffer_size() >= MAX_BUFFER_SIZE) {
		return false;
	}
	event_buffer[buf_end] = *data;
	return true;
}

static struct hybrid_frame_data *buffer_push_alloc(void)
{
	struct hybrid_frame_data *ret = NULL;

	if (buffer_size() >= MAX_BUFFER_SIZE) {
		buffer_pop();
	}
	ret = &event_buffer[buf_end];
	buf_end = (buf_end + 1) & MAX_BUFFER_SIZE;
	return ret;
}

static void frame_sync_handler(struct hybrid_frame_data *data)
{
	switch (data->mode) {
	case HYBRID_FRAME_PRODUCE:
	case HYBRID_FRAME_SCHED_FRAME: {
		cl_notify_frame_produce();
		fl_notify_frame_produce();
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_FDRIVE)
		if (game_bwmon_on_frame_event != NULL) {
			game_bwmon_on_frame_event(255, 1);
		}
#endif /* CONFIG_OPLUS_FEATURE_GEAS_FDRIVE */
		htb_notify_frame_produce();
		ctb_notify_frame_produce(0);
	} break;

	default:
		break;
	}
}

static void log_format_hfd(struct hybrid_frame_data *data)
{
	if (likely(!g_debug_enable)) {
		return;
	}
	trace_pr_val_str("mode", data->mode);
	trace_pr_val_str("end", data->end);
	switch (data->mode) {
	case HYBRID_FRAME_PRODUCE: {
		trace_pr_val_str("buffer_num", data->produce_data.buffer_num);
		trace_pr_val_str("produce_time", data->produce_data.produce_time);
	} break;

	case HYBRID_FRAME_CONSUME: {
		trace_pr_val_str("consume_time", data->consume_data.consume_time);
	} break;

	case HYBRID_FRAME_SCHED_FRAME: {
		trace_pr_val_str("buffer_num", data->sched_frame_data.buffer_num);
		trace_pr_val_str("produce_time", data->sched_frame_data.produce_time);
		trace_pr_val_str("consume_time", data->sched_frame_data.consume_time);
		trace_pr_val_str("ds_status", data->sched_frame_data.ds_status);
	} break;

	case HYBRID_FRAME_VSYNC_APP: {
		trace_pr_val_str("render_time", data->vsync_data.render_time);
		trace_pr_val_str("composite_time", data->vsync_data.composite_time);
	} break;

	case HYBRID_FRAME_RENDER_PASS: {
		trace_pr_val_str("total_rp", data->rp_data.total_rp);
		trace_pr_val_str("current_rp", data->rp_data.current_rp);
		trace_pr_val_str("time", data->rp_data.time);
	} break;

	default:
		break;
	}
}

/************************** proc ops ************************/

static int wake_up_hybrid_frame_poll(void)
{
	atomic_inc(&hybrid_frame_poll.event);
	wake_up_interruptible(&hybrid_frame_poll.wait);
	return 0;
}

static int reset_hybrid_frame_poll(void)
{
	atomic_set(&hybrid_frame_poll.event, 0);
	wake_up_interruptible(&hybrid_frame_poll.wait);
	return 0;
}

static long hfs_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	struct hybrid_frame_data *data;
	void __user *uarg = (void __user *)arg;
	long ret = 0;

	if ((_IOC_TYPE(cmd) != HYBRID_FRAME_SYNC_MAGIC) || (_IOC_NR(cmd) >= FRAME_HYBRID_MAX_ID)) {
		return -EINVAL;
	}

	switch (cmd) {
	case CMD_ID_FRAME_HYBRID_WR:
		write_lock(&hfs_rwlock);
		data = buffer_push_alloc();
		if (data && !copy_from_user(data, uarg, sizeof(struct hybrid_frame_data))) {
			frame_sync_handler(data);
			log_format_hfd(data);
			ret = wake_up_hybrid_frame_poll();
		} else {
			buffer_pop_back();
			ret = -EFAULT;
		}
		write_unlock(&hfs_rwlock);
		break;

	case CMD_ID_FRAME_HYBRID_RD:
		read_lock(&hfs_rwlock);
		data = buffer_front();
		if (data) {
			data->end = buffer_size() <= 1; /* the last buffer */
			log_format_hfd(data);
			ret = copy_to_user(uarg, data, sizeof(struct hybrid_frame_data));
			buffer_pop();
		} else {
			ret = -EFAULT;
		}
		read_unlock(&hfs_rwlock);
		break;

	case CMD_ID_FRAME_HYBRID_WAKE:
		ret = reset_hybrid_frame_poll();
		break;

	default:
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_hfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return hfs_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif /* CONFIG_COMPAT */

static int hfs_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int hfs_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct proc_ops hfs_proc_ops = {
	.proc_ioctl		=	hfs_ioctl,
	.proc_open		=	hfs_open,
	.proc_release		=	hfs_release,
#if IS_ENABLED(CONFIG_COMPAT)
	.proc_compat_ioctl	=	compat_hfs_ioctl,
#endif /* CONFIG_COMPAT */
	.proc_lseek		=	default_llseek,
};

int hybrid_frame_sync_init(void)
{
	if (unlikely(!game_opt_dir)) {
		return -ENOTDIR;
	}

	proc_create_data("hfs_ctrl", 0666, game_opt_dir, &hfs_proc_ops, NULL);

	init_hfs_data();

	return 0;
}

void hybrid_frame_sync_exit(void)
{
	if (!game_opt_dir) {
		return;
	}

	remove_proc_entry("hfs_ctrl", game_opt_dir);
}
