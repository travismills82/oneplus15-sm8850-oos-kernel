#include <linux/types.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/signal.h>
#include <linux/cleanup.h>
#include <trace/hooks/sched.h>
#include <trace/hooks/cpufreq.h>

#include "game_ctrl.h"
#include "task_load_track.h"
#include "oem_data/gts_common.h"

#define DECLARE_DEBUG_TRACE(name, proto, data)			\
	static void __maybe_unused debug_##name(proto) {	\
		if (unlikely(g_debug_enable)) {			\
			name(data);				\
		}						\
	}
#include "debug_common.h"
#undef DECLARE_DEBUG_TRACE

/************************** record info ************************/

spinlock_t g_lock;

struct freq_stat
{
	spinlock_t lock;
	struct task_struct *cur_tsk;
};

DEFINE_PER_CPU(struct freq_stat, g_rq_stats);

struct tracking_list_node
{
	struct list_head node;
	struct task_struct *p;
};

static struct list_head tracking_list = LIST_HEAD_INIT(tracking_list);

static bool tt_user_enable;
static int gc_running_cnt;
static int loop_cnt;
static struct task_struct *sig_task;

static struct hrtimer gc_user_timer;
static struct irq_work start_gc_action_irq_work;
static struct irq_work cancel_gc_action_irq_work;

static enum hrtimer_restart gc_user_timer_callback(struct hrtimer *timer);
static void start_gc_action(struct irq_work *irq_work);
static void cancel_gc_action(struct irq_work *irq_work);

struct file_ops_struct
{
	int file_data;
	int *data_ptr;
} g_file_ops;

void init_tlt_stats(void)
{
	int cpu;
	struct freq_stat *stat;

	spin_lock_init(&g_lock);

	for_each_possible_cpu(cpu) {
		stat = per_cpu_ptr(&g_rq_stats, cpu);
		spin_lock_init(&stat->lock);
		stat->cur_tsk = NULL;
	}

	g_file_ops.file_data = 0;
	g_file_ops.data_ptr = &g_file_ops.file_data;

	gc_running_cnt = 0;
	loop_cnt = 0;
	sig_task = NULL;

	hrtimer_init(&gc_user_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gc_user_timer.function = gc_user_timer_callback;

	init_irq_work(&start_gc_action_irq_work, start_gc_action);
	init_irq_work(&cancel_gc_action_irq_work, cancel_gc_action);
}

/************************** struct ops ************************/

#ifndef div64_u64_roundup
u64 div64_u64_roundup(u64 dividend, u64 divisor)
{
	return div64_u64((dividend) + (divisor - 1), divisor);
}
#endif

u64 scale_exec_time(u64 delta, struct cpufreq_policy *policy)
{
	int cpu = cpumask_first(policy->cpus);
	u64 task_exec_scale;
	unsigned int cur_freq, max_freq;

	cur_freq = policy->cur;
	max_freq = policy->cpuinfo.max_freq;

	if (unlikely(max_freq <= 0) || unlikely(cur_freq > max_freq)) {
		return delta;
	}

	task_exec_scale = div64_u64_roundup(
		cur_freq * arch_scale_cpu_capacity(cpu), max_freq);

	return delta * task_exec_scale;
}

void update_task_load(
	struct task_struct *p,
	struct game_task_struct *gts,
	struct cpufreq_policy *policy,
	u64 now)
{
	u64 delta;

	if (unlikely(p == NULL || gts == NULL)) {
		return;
	}

	delta = now - gts->demand.last_update_time;
	gts->demand.curr_runtime_sum += delta;
	gts->demand.curr_demand += scale_exec_time(delta, policy);
	gts->demand.last_update_time = now;

	debug_trace_pr_val_com(
		"runtime_", p->pid, gts->demand.curr_runtime_sum);
	debug_trace_pr_val_com(
		"demand_", p->pid, gts->demand.curr_demand);
}

bool tlt_enable(void)
{
	return BIT(TASK_LOAD_TRACK_ENABLE) & READ_ONCE(g_file_ops.file_data);
}

void tlt_add_task(struct tlt_info *info)
{
	struct task_struct *ts = NULL;
	struct game_task_struct *gts = NULL;

	for (int i = 0; i < info->size; i++) {
		debug_trace_pr_val_str("add", info->data[i]);
		rcu_read_lock();
		ts = find_task_by_vpid(info->data[i]);
		if (ts_to_gts(ts, &gts)) {
			gts->demand.tracking = 1;
		}
		rcu_read_unlock();
	}
}

void tlt_remove_task(struct tlt_info *info)
{
	struct task_struct *ts = NULL;
	struct game_task_struct *gts = NULL;

	for (int i = 0; i < info->size; i++) {
		debug_trace_pr_val_str("remove", info->data[i]);
		rcu_read_lock();
		ts = find_task_by_vpid(info->data[i]);
		if (ts_to_gts(ts, &gts) && gts->demand.tracking) {
			gts->demand.tracking = 0;
			gts->demand.handoff = 1;
		}
		rcu_read_unlock();
	}
}

void tlt_read_task_load(struct tlt_info *info)
{
	unsigned long flags;
	int cpu;
	u64 now = sched_clock();
	u64 curr_runtime_sum;
	u64 curr_demand;
	struct task_struct *ts = NULL;
	struct game_task_struct *gts = NULL;
	struct freq_stat *stat;

	for (int i = info->size - 1; i >= 0; i--) {
		debug_trace_pr_val_str("read", info->data[i]);
		rcu_read_lock();
		ts = find_task_by_vpid(info->data[i]);
		if (ts_to_gts(ts, &gts) && gts->demand.tracking) {
			if (task_is_running(ts)) {
				cpu = task_cpu(ts);
				if (cpu < 0 || cpu >= nr_cpu_ids) {
					goto out;
				}
				stat = per_cpu_ptr(&g_rq_stats, cpu);
				spin_lock_irqsave(&stat->lock, flags);
				if (gts->demand.tracking && ts == stat->cur_tsk) {
					update_task_load(ts, gts, cpufreq_cpu_get_raw(cpu), now);
				}
				spin_unlock_irqrestore(&stat->lock, flags);
			}
out:
			curr_runtime_sum = gts->demand.curr_runtime_sum;
			curr_demand = gts->demand.curr_demand;
			info->data[i * 3] = curr_runtime_sum - gts->demand.prev_runtime_sum;
			info->data[i * 3 + 1] = curr_demand - gts->demand.prev_demand;
			info->data[i * 3 + 2] = now - gts->demand.last_rollover_time;
			gts->demand.prev_runtime_sum = curr_runtime_sum;
			gts->demand.prev_demand = curr_demand;
			gts->demand.last_rollover_time = now;
		} else {
			info->data[i * 3] = 0;
			info->data[i * 3 + 1] = 0;
			info->data[i * 3 + 2] = 0;
		}
		rcu_read_unlock();
	}
	info->size = info->size * 3;
}

void tt_user_set(int pid, int value, bool set)
{
	struct task_struct *task = NULL;
	struct game_task_struct *gts = NULL;

	switch (value) {
	case TASK_TRACK_USER_ENABLE: {
		scoped_guard(spinlock_irqsave, &g_lock) {
			scoped_guard (rcu) {
				if (pid > 0) {
					task = find_task_by_vpid(pid);
				} else {
					task = current;
				}
				if (task) {
					get_task_struct(task);
				}
			}
			if (task != sig_task) {
				if (sig_task) {
					put_task_struct(sig_task);
				}
				sig_task = task;
			} else {
				if (task) {
					put_task_struct(task);
				}
			}
			WRITE_ONCE(tt_user_enable, set);
		}
		debug_trace_pr_val_str("sigpid", current->pid);
	} break;

	case TASK_TRACK_USER_GC: {
		rcu_read_lock();
		task = find_task_by_vpid(pid);
		if (task && ts_to_gts(task, &gts)) {
			WRITE_ONCE(gts->tt_user.gc_tracking, set);
		}
		rcu_read_unlock();
		debug_trace_pr_val_str("gcpid", pid);
	} break;

	default:
		break;
	}
}

/************************** sig action ************************/

static void send_gc_action_sig(bool cancel)
{
	kernel_siginfo_t siginfo;
	struct task_struct *task = NULL;

	siginfo.si_signo = TT_USER_SIG;
	siginfo.si_errno = 0;
	siginfo.si_code = SI_KERNEL;
	siginfo.si_int = (cancel ? TT_USER_SIG_CANCEL : 0) | TASK_TRACK_USER_GC;

	scoped_guard(spinlock_irqsave, &g_lock) {
		if (sig_task && sig_task->exit_state) {
			put_task_struct(sig_task);
			sig_task = NULL;
		}
		if (sig_task) {
			scoped_guard (rcu) {
				get_task_struct(sig_task);
				task = sig_task;
			}
		}
	}

	if (task) {
		send_sig_info(TT_USER_SIG, &siginfo, task);
		put_task_struct(task);
	}

	debug_trace_pr_val_str("gc", !cancel);
}

static void start_gc_action(struct irq_work *irq_work)
{
	send_gc_action_sig(false);
}

static void cancel_gc_action(struct irq_work *irq_work)
{
	send_gc_action_sig(true);
}

static void send_gc_action_sig_irq_work(bool cancel)
{
	struct irq_work *work;

	if (cancel) {
		work = &cancel_gc_action_irq_work;
	} else {
		work = &start_gc_action_irq_work;
	}

	if (work) {
		if (likely(cpu_online(raw_smp_processor_id()))) {
			irq_work_queue(work);
		} else {
			irq_work_queue_on(work, cpumask_any(cpu_online_mask));
		}
	}
}

/************************** timer func ************************/

static enum hrtimer_restart gc_user_timer_callback(struct hrtimer *timer)
{
	if (gc_running_cnt <= 0 || loop_cnt > TT_USER_MAX_LOOP) {
		loop_cnt = 0;
		send_gc_action_sig_irq_work(true);
		return HRTIMER_NORESTART;
	}
	loop_cnt++;
	hrtimer_forward_now(timer, ktime_set(0, TT_USER_TIMER_DURATION));
	return HRTIMER_RESTART;
}

/************************** vendor hooks ************************/

static void sched_switch_handler(void *unused, bool preempt,
		struct task_struct *prev, struct task_struct *next, unsigned int prev_state)
{
	unsigned long flags;
	int cpu;
	u64 now;
	struct game_task_struct *pgts = NULL;
	struct game_task_struct *ngts = NULL;
	struct freq_stat *stat = NULL;

	if (!tlt_enable()) {
		return;
	}

	now = sched_clock();

	if (ts_to_gts(prev, &pgts) && (pgts->demand.tracking || pgts->demand.handoff)) {
		cpu = task_cpu(prev);
		stat = per_cpu_ptr(&g_rq_stats, cpu);
		spin_lock_irqsave(&stat->lock, flags);

		if (unlikely(!pgts->demand.tracking || prev != stat->cur_tsk)) {
			debug_trace_pr_val_com("miss_p_", cpu, prev->pid);
			goto pts_unlock;
		}

		update_task_load(prev, pgts, cpufreq_cpu_get_raw(cpu), now);

		if (likely(stat->cur_tsk != NULL)) {
			put_task_struct(stat->cur_tsk);
		}
		stat->cur_tsk = NULL;

pts_unlock:
		if (unlikely(pgts->demand.handoff)) {
			debug_trace_pr_val_com("handoff_", cpu, prev->pid);
			pgts->demand.handoff = 0;
		}

		spin_unlock_irqrestore(&stat->lock, flags);
	}

	if (ts_to_gts(next, &ngts) && ngts->demand.tracking) {
		cpu = task_cpu(next);
		stat = per_cpu_ptr(&g_rq_stats, cpu);
		spin_lock_irqsave(&stat->lock, flags);

		if (unlikely(!ngts->demand.tracking)) {
			debug_trace_pr_val_com("miss_n_", cpu, next->pid);
			goto nts_unlock;
		}

		get_task_struct(next);
		stat->cur_tsk = next;

nts_unlock:
		ngts->demand.last_update_time = now;
		spin_unlock_irqrestore(&stat->lock, flags);
	}
}

static void sched_switch_tt_user_handler(void *unused, bool preempt,
		struct task_struct *prev, struct task_struct *next, unsigned int prev_state)
{
	unsigned long flags;
	struct game_task_struct *pgts = NULL;
	struct game_task_struct *ngts = NULL;

	if (!tt_user_enable) {
		return;
	}

	if (ts_to_gts(prev, &pgts)) {
		if (game_pid == prev->tgid && pgts->tt_user.gc_tracking) {
			spin_lock_irqsave(&g_lock, flags);
			if (likely(gc_running_cnt > 0)) {
				gc_running_cnt--;
			}
			spin_unlock_irqrestore(&g_lock, flags);
		}
	}

	if (ts_to_gts(next, &ngts)) {
		if (game_pid == next->tgid && ngts->tt_user.gc_tracking) {
			spin_lock_irqsave(&g_lock, flags);
			gc_running_cnt++;
			if (!hrtimer_active(&gc_user_timer)) {
				hrtimer_start(&gc_user_timer, ktime_set(0, TT_USER_TIMER_DURATION), HRTIMER_MODE_REL);
				send_gc_action_sig_irq_work(false);
			}
			spin_unlock_irqrestore(&g_lock, flags);
		}
	}
}

static void cpufreq_fast_switch_handler(void *unused, struct cpufreq_policy *policy,
		unsigned int *target_freq, unsigned int old_target_freq)
{
	unsigned long flags;
	int cpu;
	u64 now;
	struct cpumask *cpus;
	struct game_task_struct *gts = NULL;
	struct freq_stat *stat = NULL;

	if (!tlt_enable() || unlikely(policy == NULL)) {
		return;
	}

	now = sched_clock();
	cpus = policy->cpus;

	for_each_cpu(cpu, cpus) {
		stat = per_cpu_ptr(&g_rq_stats, cpu);
		spin_lock_irqsave(&stat->lock, flags);

		if (stat->cur_tsk == NULL || !ts_to_gts(stat->cur_tsk, &gts)) {
			goto unlock;
		}

		update_task_load(stat->cur_tsk, gts, policy, now);

unlock:
		spin_unlock_irqrestore(&stat->lock, flags);
	}
}

void register_tlt_hooks(void)
{
	register_trace_sched_switch(sched_switch_handler, NULL);
	register_trace_sched_switch(sched_switch_tt_user_handler, NULL);
	register_trace_android_vh_cpufreq_fast_switch(cpufreq_fast_switch_handler, NULL);
}

void unregister_tlt_hooks(void)
{
	unregister_trace_sched_switch(sched_switch_handler, NULL);
	unregister_trace_sched_switch(sched_switch_tt_user_handler, NULL);
	unregister_trace_android_vh_cpufreq_fast_switch(cpufreq_fast_switch_handler, NULL);
}

/************************** proc ops ************************/

static long tlt_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tlt_info data;
	void __user *uarg = (void __user *)arg;
	long ret = 0;

	if ((_IOC_TYPE(cmd) != TLT_MAGIC) ||
		(_IOC_NR(cmd) >= TLT_MAX_ID)) {
		return -EINVAL;
	}

	if (copy_from_user(&data, uarg, sizeof(data))) {
		return -EFAULT;
	}

	data.size = min(TLT_INFO_PAGE_SIZE, data.size);

	switch (cmd) {
	case CMD_ID_TLT_STATE_CHANGE:
		WRITE_ONCE(g_file_ops.file_data, data.data[0]);
		break;

	case CMD_ID_TLT_ADD_TASK:
		tlt_add_task(&data);
		break;

	case CMD_ID_TLT_REMOVE_TASK:
		tlt_remove_task(&data);
		break;

	case CMD_ID_TLT_READ_TASK_LOAD:
		tlt_read_task_load(&data);
		if (copy_to_user(uarg, &data, sizeof(data))) {
			return -EFAULT;
		}
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef GAME_OPT_PROC_READ_DEBUG
static int tlt_info_show(struct seq_file *m, void *v)
{
	char page[1024] = {0};
	ssize_t len = 0;

	len += snprintf(page + len, sizeof(page) - len, "file_data=%d\n",
						g_file_ops.file_data);

	seq_puts(m, page);

	return 0;
}
#endif /* GAME_OPT_PROC_READ_DEBUG */

static int tlt_proc_open(struct inode *inode, struct file *file)
{
#ifdef GAME_OPT_PROC_READ_DEBUG
	return single_open(file, tlt_info_show, inode);
#else
	return 0;
#endif /* GAME_OPT_PROC_READ_DEBUG */
}

static ssize_t tlt_write(
	struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
#ifdef GAME_OPT_PROC_READ_DEBUG
	int *pval = (int *)pde_data(file_inode(file));
	int val = 0;
	char kbuf[32] = {0};
	int err;

	if (count > 32) {
		return -EINVAL;
	}

	if (copy_from_user(kbuf, buf, count)) {
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EINVAL;
	}

	err = kstrtoint(strstrip(kbuf), 10, &val);

	if (err) {
		return -EINVAL;
	}

	WRITE_ONCE(*pval, val);
#endif /* GAME_OPT_PROC_READ_DEBUG */

	return count;
}

static int tlt_proc_release(struct inode *inode, struct file *file)
{
	return 0;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_tlt_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	return tlt_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif /* CONFIG_COMPAT */

static int tt_user_show(struct seq_file *m, void *v)
{
	return 0;
}

static int tt_user_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, tt_user_show, inode);
}

static ssize_t tt_user_proc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	char page[32] = {0};
	int ret, pid, value, set;

	ret = simple_write_to_buffer(page, sizeof(page) - 1, ppos, buf, count);
	if (ret <= 0)
		return ret;

	ret = sscanf(page, "%d %d %d", &pid, &value, &set);
	if (ret != 3) {
		return -EINVAL;
	}

	tt_user_set(pid, value, !!set);

	return count;
}

static const struct proc_ops tlt_sys_ctrl_proc_ops = {
	.proc_ioctl		=	tlt_ioctl,
	.proc_open		=	tlt_proc_open,
	.proc_write		=	tlt_write,
#ifdef GAME_OPT_PROC_READ_DEBUG
	.proc_read		=	seq_read,
#endif /* GAME_OPT_PROC_READ_DEBUG */
	.proc_release		=	tlt_proc_release,
#if IS_ENABLED(CONFIG_COMPAT)
	.proc_compat_ioctl	=	compat_tlt_ioctl,
#endif /* CONFIG_COMPAT */
	.proc_lseek		=	default_llseek,
};

static const struct proc_ops tt_user_proc_ops = {
	.proc_open	=	tt_user_proc_open,
	.proc_write	=	tt_user_proc_write,
	.proc_read	=	seq_read,
	.proc_lseek	=	seq_lseek,
	.proc_release	=	single_release,
};

bool tlt_create_proc_entry(void)
{
	if (!game_opt_dir)
		return false;

	proc_create_data("tlt_ctrl", 0664, game_opt_dir,
		&tlt_sys_ctrl_proc_ops, g_file_ops.data_ptr);
	proc_create_data("tt_user", 0664, game_opt_dir,
		&tt_user_proc_ops, NULL);
	return true;
}

bool tlt_remove_proc_entry(void)
{
	if (!game_opt_dir)
		return false;

	remove_proc_entry("tlt_ctrl", game_opt_dir);
	remove_proc_entry("tt_user", game_opt_dir);
	return true;
}

/************************** public function ************************/

int task_load_track_init(void)
{
	init_tlt_stats();
	register_tlt_hooks();
	tlt_create_proc_entry();
	return 0;
}

void task_load_track_exit(void)
{
	hrtimer_cancel(&gc_user_timer);

	unregister_tlt_hooks();
	tlt_remove_proc_entry();
}
