// SPDX-License-Identifier: GPL-2.0-only
/*
 * Disposable VM-only workqueue lifetime stress. Never include in Canoe outputs.
 * Repeated attribute replacement leaves a canceled inactive pwq to retire while
 * an older pool still has active work. This exercises the pending-pwq lifetime
 * path, but finite stress is not proof that every race interleaving occurred.
 */
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#define START_TIMEOUT_MS 2000
#define HOLD_TIMEOUT_MS 5000
#define MAX_ITERATIONS 2048

static bool guest_only;
module_param(guest_only, bool, 0400);
MODULE_PARM_DESC(guest_only, "Required explicit guest opt-in; also requires five_cves_vm=1 on guest command line");

static unsigned int iterations = 256;
module_param(iterations, uint, 0400);
MODULE_PARM_DESC(iterations, "Bounded lifetime-stress iterations (1..2048)");

struct stress_case {
	struct work_struct active;
	struct work_struct canceled;
	struct completion started;
	struct completion release;
	atomic_t active_runs;
	atomic_t canceled_runs;
	atomic_t wait_timeouts;
};

static void active_workfn(struct work_struct *work)
{
	struct stress_case *test = container_of(work, struct stress_case, active);

	atomic_inc(&test->active_runs);
	complete(&test->started);
	if (!wait_for_completion_timeout(&test->release,
				msecs_to_jiffies(HOLD_TIMEOUT_MS)))
		atomic_inc(&test->wait_timeouts);
}

static void canceled_workfn(struct work_struct *work)
{
	struct stress_case *test = container_of(work, struct stress_case,
						      canceled);

	atomic_inc(&test->canceled_runs);
}

static void initialize_case(struct stress_case *test)
{
	INIT_WORK(&test->active, active_workfn);
	INIT_WORK(&test->canceled, canceled_workfn);
	init_completion(&test->started);
	init_completion(&test->release);
	atomic_set(&test->active_runs, 0);
	atomic_set(&test->canceled_runs, 0);
	atomic_set(&test->wait_timeouts, 0);
}

/* Do not rely on a load parameter alone to protect the host or physical phone. */
static int guest_guard(void)
{
	static const char marker[] = "five_cves_vm=1";
	struct file *file;
	char *buf, *match;
	loff_t pos = 0;
	ssize_t len;
	int result = -EPERM;

	if (!IS_ENABLED(CONFIG_X86_64) || !guest_only)
		return -EPERM;
	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	file = filp_open("/proc/cmdline", O_RDONLY, 0);
	if (IS_ERR(file)) {
		result = PTR_ERR(file);
		goto free_buf;
	}
	len = kernel_read(file, buf, PAGE_SIZE - 1, &pos);
	filp_close(file, NULL);
	if (len <= 0 || len >= PAGE_SIZE - 1)
		goto free_buf;
	buf[len] = '\0';
	match = buf;
	while ((match = strstr(match, marker))) {
		char next = match[sizeof(marker) - 1];

		if ((match == buf || isspace(match[-1])) &&
		    (!next || isspace(next))) {
			result = 0;
			break;
		}
		match++;
	}
free_buf:
	kfree(buf);
	return result;
}

static int lifetime_iteration(unsigned int number)
{
	struct workqueue_attrs *attrs;
	struct workqueue_struct *wq;
	struct stress_case test;
	int result;

	initialize_case(&test);
	attrs = alloc_workqueue_attrs();
	if (!attrs)
		return -ENOMEM;
	wq = alloc_workqueue("five-cves-life", WQ_UNBOUND, 1);
	if (!wq) {
		result = -ENOMEM;
		goto free_attrs;
	}
	/* Explicitly start on a pool distinct from both subsequent nice levels. */
	attrs->nice = 0;
	result = apply_workqueue_attrs(wq, attrs);
	if (result)
		goto out;
	if (!queue_work(wq, &test.active)) {
		result = -EINVAL;
		goto out;
	}
	if (!wait_for_completion_timeout(&test.started,
					msecs_to_jiffies(START_TIMEOUT_MS))) {
		result = -ETIMEDOUT;
		goto out;
	}
	attrs->nice = 1;
	result = apply_workqueue_attrs(wq, attrs);
	if (result)
		goto out;
	if (!queue_work(wq, &test.canceled)) {
		result = -EINVAL;
		goto out;
	}
	if (!cancel_work_sync(&test.canceled)) {
		result = -EINVAL;
		goto out;
	}
	/* Release the inactive pwq's base references while A still owns its slot. */
	attrs->nice = 2;
	result = apply_workqueue_attrs(wq, attrs);
	if (result)
		goto out;
	usleep_range(1000, 2000);
out:
	/* Release before cancellation/drain on EVERY path, including start timeout. */
	complete_all(&test.release);
	cancel_work_sync(&test.active);
	cancel_work_sync(&test.canceled);
	flush_workqueue(wq);
	destroy_workqueue(wq);
	if (!result && (atomic_read(&test.active_runs) != 1 ||
			atomic_read(&test.canceled_runs) != 0 ||
			atomic_read(&test.wait_timeouts)))
		result = -EINVAL;
	if (result)
		pr_err("FIVE_CVES_WQ: FAIL iteration=%u error=%d active=%d canceled=%d timeouts=%d\n",
		       number, result, atomic_read(&test.active_runs),
		       atomic_read(&test.canceled_runs),
		       atomic_read(&test.wait_timeouts));
free_attrs:
	free_workqueue_attrs(attrs);
	return result;
}

static int queue_controls(void)
{
	struct workqueue_struct *wq;
	struct stress_case test;
	unsigned int kind;
	int result = 0;

	for (kind = 0; kind < 3; kind++) {
		initialize_case(&test);
		if (kind == 0)
			wq = alloc_workqueue("five-cves-percpu", WQ_PERCPU, 1);
		else if (kind == 1)
			wq = alloc_ordered_workqueue("five-cves-ordered", 0);
		else
			wq = alloc_workqueue("five-cves-empty", WQ_UNBOUND, 1);
		if (!wq)
			return -ENOMEM;
		if (kind != 2 && !queue_work(wq, &test.canceled))
			result = -EINVAL;
		flush_workqueue(wq);
		destroy_workqueue(wq);
		if (atomic_read(&test.canceled_runs) != (kind != 2))
			result = -EINVAL;
		if (result)
			return result;
	}
	pr_info("FIVE_CVES_WQ: PASS controls=3 percpu=1 ordered=1 empty=1\n");
	return 0;
}

static int __init five_cves_workqueue_init(void)
{
	unsigned int i;
	int result = guest_guard();

	if (result) {
		pr_err("FIVE_CVES_WQ: REFUSED requires x86_64 guest_only=1 and guest cmdline five_cves_vm=1\n");
		return result;
	}
	if (!iterations || iterations > MAX_ITERATIONS)
		return -EINVAL;
	pr_info("FIVE_CVES_WQ: START iterations=%u active_timeout_ms=%u\n",
		iterations, HOLD_TIMEOUT_MS);
	result = queue_controls();
	if (result)
		return result;
	for (i = 0; i < iterations; i++) {
		result = lifetime_iteration(i);
		if (result)
			return result;
		if (!((i + 1) % 64))
			pr_info("FIVE_CVES_WQ: PROGRESS completed=%u\n", i + 1);
		cond_resched();
	}
	pr_info("FIVE_CVES_WQ: PASS iterations=%u active=%u canceled_callbacks=0 timeouts=0 controls=3\n",
		iterations, iterations);
	return 0;
}

static void __exit five_cves_workqueue_exit(void)
{
	pr_info("FIVE_CVES_WQ: UNLOAD all test queues already drained\n");
}

module_init(five_cves_workqueue_init);
module_exit(five_cves_workqueue_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Guarded disposable-VM pending workqueue lifetime stress");
