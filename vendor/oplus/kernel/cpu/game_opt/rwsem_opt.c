#include <linux/types.h>
#include <linux/errno.h>
#include <trace/hooks/rwsem.h>
#include <linux/time64.h>
#include <linux/list.h>

#include "game_ctrl.h"
#include "oem_data/gts_common.h"

#define DECLARE_DEBUG_TRACE(name, proto, data)			\
	static void __maybe_unused debug_##name(proto) {	\
		if (unlikely(g_debug_enable)) {			\
			name(data);				\
		}						\
	}
#include "debug_common.h"
#undef DECLARE_DEBUG_TRACE

#define RWSEM_WRITER_LOCKED	(1UL << 0)
#define RWSEM_FLAG_WAITERS	(1UL << 1)
#define RWSEM_FLAG_HANDOFF	(1UL << 2)
#define RWSEM_FLAG_READFAIL	(1UL << (BITS_PER_LONG - 1))

#define RWSEM_READER_SHIFT	8
#define RWSEM_READER_BIAS	(1UL << RWSEM_READER_SHIFT)
#define RWSEM_READER_MASK	(~(RWSEM_READER_BIAS - 1))
#define RWSEM_WRITER_MASK	RWSEM_WRITER_LOCKED
#define RWSEM_LOCK_MASK		(RWSEM_WRITER_MASK|RWSEM_READER_MASK)
#define RWSEM_READ_FAILED_MASK	(RWSEM_WRITER_MASK|RWSEM_FLAG_WAITERS|\
				RWSEM_FLAG_HANDOFF|RWSEM_FLAG_READFAIL)

enum rwsem_waiter_type {
	RWSEM_WAITING_FOR_WRITE,
	RWSEM_WAITING_FOR_READ
};

struct rwsem_waiter {
	struct list_head list;
	struct task_struct *task;
	enum rwsem_waiter_type type;
	unsigned long timeout;
	bool handoff_set;
};

#define rwsem_first_waiter(sem) \
	list_first_entry(&sem->wait_list, struct rwsem_waiter, list)

/************************** record info ************************/

struct rwsem_opt_list_node
{
	struct list_head node;
	struct task_struct *p;
};

/* record tracking list */
static struct list_head rwsem_opt_list = LIST_HEAD_INIT(rwsem_opt_list);

spinlock_t lock_stealing_lock;

static struct rwsem_opt_list_node *find_rwsem_opt_list_node(struct list_head *head, struct task_struct *task)
{
	struct rwsem_opt_list_node *iter, *target = NULL;

	list_for_each_entry(iter, head, node) {
		if (iter->p == task) {
			target = iter;
			break;
		}
	}

	return target;
}

static bool add_rwsem_opt_list_node(struct list_head *head, struct task_struct *task)
{
	struct rwsem_opt_list_node *new_node;

	if (!task) {
		return false;
	}
	new_node = kzalloc(sizeof(struct rwsem_opt_list_node), GFP_KERNEL);
	if (!new_node) {
		return false;
	}
	get_task_struct(task);
	new_node->p = task;
	list_add_tail(&new_node->node, head);
	return true;
}

static bool del_rwsem_opt_list_node(struct list_head *head, struct task_struct *task)
{
	struct rwsem_opt_list_node *target = find_rwsem_opt_list_node(head, task);
	if (target) {
		if (target->p) {
			put_task_struct(target->p);
		}
		list_del(&target->node);
		kfree(target);
	}
	return true;
}

static void clear_rwsem_opt_list_node(struct list_head *head)
{
	struct rwsem_opt_list_node *iter, *next;
	struct task_struct *task;
	struct game_task_struct *gts;

	list_for_each_entry_safe(iter, next, head, node) {
		task = iter->p;
		if (task && ts_to_gts(task, &gts)) {
			gts->rwsem_flag &= ~BIT(DIRECT_RSTEAL);
		}
		if (task) {
			put_task_struct(task);
		}
		list_del(&iter->node);
		kfree(iter);
	}
}

/************************** data ops ************************/

void init_rwsem_opt_data(void)
{
	spin_lock_init(&lock_stealing_lock);
}

void clear_rwsem_opt_data(void)
{
	unsigned long flags;

	spin_lock_irqsave(&lock_stealing_lock, flags);
	clear_rwsem_opt_list_node(&rwsem_opt_list);
	spin_unlock_irqrestore(&lock_stealing_lock, flags);
}

/************************** vendor hooks ************************/

static void android_vh_rwsem_read_trylock_failed_handler(
	void *unused, struct rw_semaphore *sem, long *cntp, int *ret)
{
	struct game_task_struct *gts = NULL;

	if (cntp == NULL) {
		return;
	}

	if (likely(current->tgid != game_pid)) {
		return;
	}

	if (ts_to_gts(current, &gts) && (gts->rwsem_flag & BIT(DIRECT_RSTEAL))) {
		*cntp |= RWSEM_FLAG_HANDOFF << RWSEM_READER_SHIFT;
		debug_trace_pr_val_str("cntp", *cntp);
	}
}

void register_rwsem_opt_hooks(void)
{
	register_trace_android_vh_rwsem_read_trylock_failed(android_vh_rwsem_read_trylock_failed_handler, NULL);
}

void unregister_rwsem_opt_hooks(void)
{
	unregister_trace_android_vh_rwsem_read_trylock_failed(android_vh_rwsem_read_trylock_failed_handler, NULL);
}

/************************** proc ops ************************/

static int lock_stealing_show(struct seq_file *m, void *v)
{
	char page[RESULT_PAGE_SIZE] = {0};
	ssize_t len = 0;
	unsigned long flags;
	struct rwsem_opt_list_node *iter;

	spin_lock_irqsave(&lock_stealing_lock, flags);
	list_for_each_entry(iter, &rwsem_opt_list, node) {
		len += snprintf(page + len, RESULT_PAGE_SIZE - 1, "pid=%d, name=%s\n", iter->p->pid, iter->p->comm);
	}
	spin_unlock_irqrestore(&lock_stealing_lock, flags);
	seq_puts(m, page);

	return 0;
}

static int lock_stealing_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, lock_stealing_show, inode);
}

static ssize_t lock_stealing_proc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	char page[32] = {0};
	int ret, pid, value;
	unsigned long flags;
	struct task_struct *task = NULL;
	struct game_task_struct *gts = NULL;

	ret = simple_write_to_buffer(page, sizeof(page) - 1, ppos, buf, count);
	if (ret <= 0)
		return ret;

	ret = sscanf(page, "%d %d", &pid, &value);
	if (ret != 2) {
		return -EINVAL;
	}

	spin_lock_irqsave(&lock_stealing_lock, flags);
	if (pid <= 0) {
		clear_rwsem_opt_list_node(&rwsem_opt_list);
	} else {
		rcu_read_lock();
		task = find_task_by_vpid(pid);
		if (task && ts_to_gts(task, &gts)) {
			if (gts->rwsem_flag & BIT(DIRECT_RSTEAL)) {
				if (value <= 0 && del_rwsem_opt_list_node(&rwsem_opt_list, task)) {
					gts->rwsem_flag &= ~BIT(DIRECT_RSTEAL);
				}
			} else {
				if (value > 0 && add_rwsem_opt_list_node(&rwsem_opt_list, task)) {
					gts->rwsem_flag |= BIT(DIRECT_RSTEAL);
				}
			}
		}
		rcu_read_unlock();
	}
	spin_unlock_irqrestore(&lock_stealing_lock, flags);

	return count;
}

static const struct proc_ops lock_stealing_proc_ops = {
	.proc_open	=	lock_stealing_proc_open,
	.proc_write	=	lock_stealing_proc_write,
	.proc_read	=	seq_read,
	.proc_lseek	=	seq_lseek,
	.proc_release	=	single_release,
};

void rwsem_opt_create_proc_entry(void)
{
	if (!game_opt_dir)
		return;

	proc_create_data("lock_stealing", 0664, game_opt_dir, &lock_stealing_proc_ops, NULL);
}

void rwsem_opt_remove_proc_entry(void)
{
	if (!game_opt_dir)
		return;

	remove_proc_entry("lock_stealing", game_opt_dir);
}

/************************** public function ************************/

int rwsem_opt_init(void)
{
	init_rwsem_opt_data();
	register_rwsem_opt_hooks();
	rwsem_opt_create_proc_entry();
	return 0;
}

void rwsem_opt_exit(void)
{
	clear_rwsem_opt_data();
	unregister_rwsem_opt_hooks();
	rwsem_opt_remove_proc_entry();
}
