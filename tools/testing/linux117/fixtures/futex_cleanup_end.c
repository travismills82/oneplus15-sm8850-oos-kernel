/* SPDX-License-Identifier: GPL-2.0-only */
/* Exact function from Linux stable 2b7d0e7dfbeabfb2eb4bbdeb08e9d5a0d954034a,
 * kernel/futex/core.c. Keep the test independent of unpushed upstream refs.
 */
static void futex_cleanup_end(struct task_struct *tsk, int state)
{
	scoped_guard(raw_spinlock_irq, &tsk->pi_lock)
		tsk->futex_state = state;

	/*
	 * Drop the exit protection. This unblocks waiters which observed
	 * FUTEX_STATE_EXITING to reevaluate the state.
	 */
	mutex_unlock(&tsk->futex_exit_mutex);
}
