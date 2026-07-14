#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/ioprio.h>
#include <linux/blk-mq.h>
#include <linux/sa_common.h>

#include <trace/hooks/wqlockup.h>
#include <trace/hooks/blk.h>
#include "oplus_wq_dynamic_priority.h"

#define VIRTUAL_KWORKER_NORMAL_NICE (-1000)
#define VIRTUAL_KWORKER_KBLOCKD_NICE (-1001)
#define WQ_CMP(str)  (strncmp(wq->name, str, sizeof(str) - 1) == 0)

static struct workqueue_attrs *ux_wq_attrs;
static struct workqueue_attrs *ux_wq_attrs_kblockd;
static struct workqueue_struct *oplus_kblockd_workqueue;

struct config_wq_flags {
	char *target_str;
	unsigned int new_flags;
};

static void android_rvh_alloc_and_link_pwqs_handler(void *unused,
	struct workqueue_struct *wq, int *ret, bool *skip)
{
	if (WQ_CMP("loop") || WQ_CMP("kverityd")) {
		*ret = apply_workqueue_attrs_locked(wq, ux_wq_attrs);
		*skip = true;
	} else if (WQ_CMP("opluskblockd")) {
		*ret = apply_workqueue_attrs_locked(wq, ux_wq_attrs_kblockd);
		*skip = true;
	}
}

static struct config_wq_flags oplus_wq_config[] = {
    { "loop", WQ_UNBOUND | WQ_FREEZABLE | WQ_HIGHPRI },
    { "kverityd", WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND },
    { "opluskblockd", WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND },
    // Add more strings and flags as needed.
    { NULL, 0 } // Terminate array with NULL
};

static int handler_alloc_workqueue_pre(struct kprobe *p, struct pt_regs *regs)
{
    const char *fmt = (const char *)regs->regs[0];
    unsigned int flags = (unsigned int)regs->regs[1];

    struct config_wq_flags *item = oplus_wq_config;
    if(fmt) {
        while (item->target_str) {
            if ((strlen(fmt) >= strlen(item->target_str)) && !strncmp(fmt, item->target_str, strlen(item->target_str)) && (item->new_flags != flags)) {
                printk(KERN_INFO "alloc_workqueue: matching fmt '%s', modifying flags from 0x%x to 0x%x\n", fmt, flags, item->new_flags);
                regs->regs[1] = item->new_flags;
                break;
            }
            item++;
        }
    }
    return 0;
}

static struct kprobe oplus_alloc_workqueue_kp = {
    .symbol_name = "alloc_workqueue",
    .pre_handler = handler_alloc_workqueue_pre,
};

static void android_rvh_create_worker_handler(void *unused,
	struct task_struct *task, struct workqueue_attrs *attrs)
{
	if (attrs->nice == VIRTUAL_KWORKER_NORMAL_NICE ||
		attrs->nice == VIRTUAL_KWORKER_KBLOCKD_NICE) {
		oplus_set_ux_state_lock(task, SA_TYPE_LIGHT, -1, true);
		set_user_nice(task, MIN_NICE);
		if (task->comm[8] == 'u')
			task->comm[8] = 'X';
	}
}

static void android_vh_blk_mq_kick_requeue_list_handler(void *unused,
	struct request_queue *q, unsigned long delay, bool *skip)
{
	mod_delayed_work_on(WORK_CPU_UNBOUND, oplus_kblockd_workqueue,
		&q->requeue_work, 0);
	*skip = 1;
	return;
}

static void android_vh_blk_mq_delay_run_hw_queue_handler(void *unused,
	int cpu, struct blk_mq_hw_ctx *hctx, unsigned long delay, bool *skip)
{
	mod_delayed_work_on(cpu, oplus_kblockd_workqueue, &hctx->run_work, delay);
	*skip = 1;
	return;
}

struct tracepoints_table {
	const char *name;
	void *func;
	struct tracepoint *tp;
	bool init;
};

static struct tracepoints_table interests[] = {
	{
		.name = "android_rvh_alloc_and_link_pwqs",
		.func = android_rvh_alloc_and_link_pwqs_handler
	},
	{
		.name = "android_rvh_create_worker",
		.func = android_rvh_create_worker_handler
	},
	{
		.name = "android_vh_blk_mq_delay_run_hw_queue",
		.func = android_vh_blk_mq_delay_run_hw_queue_handler
	},
	{
		.name = "android_vh_blk_mq_kick_requeue_list",
		.func = android_vh_blk_mq_kick_requeue_list_handler
	},
};

#define FOR_EACH_INTEREST(i) \
	for (i = 0; i < sizeof(interests) / sizeof(struct tracepoints_table); \
	i++)

static void lookup_tracepoints(struct tracepoint *tp,
				       void *ignore)
{
	int i;

	FOR_EACH_INTEREST(i) {
		if (strcmp(interests[i].name, tp->name) == 0)
			interests[i].tp = tp;
	}
}

static int wq_install_tracepoints(int start, int end)
{
	int i;
	int cnt = sizeof(interests) / sizeof(struct tracepoints_table);

	if (end > cnt) {
		pr_warn("%s: err: tracepoint end > tp cnt\n",
				THIS_MODULE->name);
		end = cnt;
	}

	for (i = start; i <= end; i++) {
		if (interests[i].tp == NULL) {
			pr_err("%s: tracepoint %s not found\n",
				THIS_MODULE->name, interests[i].name);
			return -1;
		}

		if (!interests[i].init) {
			tracepoint_probe_register(interests[i].tp,
						interests[i].func,
						NULL);
			interests[i].init = true;
		}
	}

	return 0;
}

static void wq_uninstall_tracepoints(void)
{
	int i;

	FOR_EACH_INTEREST(i) {
		if (interests[i].init) {
			tracepoint_probe_unregister(interests[i].tp,
						    interests[i].func,
						    NULL);
		}
	}
}

static int __init oplus_wq_hook_init(void)
{
	int err = 0;

	err = register_kprobe(&oplus_alloc_workqueue_kp);
	if (err < 0) {
		pr_err("%s register_kprobe alloc_workqueue failed, returned %d\n", __func__, err);
		return err;
	}

	/* Install the tracepoints */
	for_each_kernel_tracepoint(lookup_tracepoints, NULL);

	ux_wq_attrs = alloc_workqueue_attrs();
	if (!ux_wq_attrs) {
		pr_err("%s alloc ux_wq_attrs fail!",__func__);
		err = -ENOMEM;
		goto out;
	} else
		ux_wq_attrs->nice = VIRTUAL_KWORKER_NORMAL_NICE;

	ux_wq_attrs_kblockd = alloc_workqueue_attrs();
	if (!ux_wq_attrs_kblockd) {
		pr_err("%s alloc ux_wq_attrs_kblockd fail!",__func__);
		err = -ENOMEM;
		goto err_free_attrs;
	} else
		ux_wq_attrs_kblockd->nice = VIRTUAL_KWORKER_KBLOCKD_NICE;

	err = wq_install_tracepoints(0, 1);
	if (err)
		goto err_free_kblockd_attrs;

	oplus_kblockd_workqueue =  alloc_workqueue("opluskblockd",
  					    WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 0);

	if (!oplus_kblockd_workqueue) {
		err = -ENOMEM;
		pr_err("%s alloc opluskblockd fail!",__func__);
		goto err_free_kblockd_attrs;
	}

	err = wq_install_tracepoints(2, 3);
	if (err)
		goto err_free_wq;

	return err;

err_free_wq:
	destroy_workqueue(oplus_kblockd_workqueue);
err_free_kblockd_attrs:
	free_workqueue_attrs(ux_wq_attrs_kblockd);
err_free_attrs:
	free_workqueue_attrs(ux_wq_attrs);
out:
    return err;
}

static void __exit oplus_wq_hook_exit(void)
{
	destroy_workqueue(oplus_kblockd_workqueue);
	wq_uninstall_tracepoints();
	free_workqueue_attrs(ux_wq_attrs);
	free_workqueue_attrs(ux_wq_attrs_kblockd);
	unregister_kprobe(&oplus_alloc_workqueue_kp);
}

module_init(oplus_wq_hook_init);
module_exit(oplus_wq_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lijiang");
MODULE_AUTHOR("Gray Jia");
MODULE_DESCRIPTION("A kernel module using vendorhook to improve IO performance");
