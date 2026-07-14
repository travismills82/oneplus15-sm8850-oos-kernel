#include "cfbt_state.h"
#include <linux/module.h>
#include <linux/kernel.h>

static struct {
	atomic_t state;
} cfbt_work_state;

void cfbt_set_no_work(void)
{
	atomic_set(&cfbt_work_state.state, CFBT_STATE_NO_WORK);
}

void cfbt_set_prepare_work(void)
{
	atomic_set(&cfbt_work_state.state, CFBT_STATE_PREPARE_WORK);
}

void cfbt_set_working(void)
{
	atomic_set(&cfbt_work_state.state, CFBT_STATE_WORKING);
}

void cfbt_set_leave_work(void)
{
	atomic_set(&cfbt_work_state.state, CFBT_STATE_LEAVE_WORK);
}

void cfbt_set_finish_work(void)
{
	atomic_set(&cfbt_work_state.state, CFBT_STATE_FINISH_WORK);
}

bool cfbt_is_state_invalid(void)
{
	int current_state = atomic_read(&cfbt_work_state.state);
	return (current_state == CFBT_STATE_NO_WORK ||
			current_state == CFBT_STATE_LEAVE_WORK ||
			current_state == CFBT_STATE_PREPARE_WORK ||
			current_state == CFBT_STATE_FINISH_WORK);
}
