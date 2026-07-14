#ifndef __TASK_LOAD_TRACK_H__
#define __TASK_LOAD_TRACK_H__

#include <linux/types.h>
#include <linux/time64.h>
#include <linux/sched/clock.h>

#define TLT_INFO_PAGE_SIZE (1 << 5)
#define TT_USER_SIG (45)
#define TT_USER_TIMER_DURATION (5 * NSEC_PER_MSEC)
#define TT_USER_MAX_LOOP 30
#define TT_USER_SIG_CANCEL (1 << 7)
#define TT_USER_SIG_MASK (TT_USER_SIG_CANCEL - 1)

enum tlt_flag
{
	TASK_LOAD_TRACK_ENABLE,
};

enum tt_user_flag
{
	TASK_TRACK_USER_ENABLE,
	TASK_TRACK_USER_GC,
};

enum tlt_cmd_id
{
	TLT_FIRST_ID, /* reserved word */
	TLT_STATE_CHANGE,
	TLT_ADD_TASK,
	TLT_REMOVE_TASK,
	TLT_READ_TASK_LOAD,
	TLT_MAX_ID,
};

struct tlt_info
{
	int size;
	uint64_t data[TLT_INFO_PAGE_SIZE * 3];
};

#define TLT_MAGIC 0xE1
#define CMD_ID_TLT_STATE_CHANGE \
	_IOWR(TLT_MAGIC, TLT_STATE_CHANGE, struct tlt_info)
#define CMD_ID_TLT_ADD_TASK \
	_IOWR(TLT_MAGIC, TLT_ADD_TASK, struct tlt_info)
#define CMD_ID_TLT_REMOVE_TASK \
	_IOWR(TLT_MAGIC, TLT_REMOVE_TASK, struct tlt_info)
#define CMD_ID_TLT_READ_TASK_LOAD \
	_IOWR(TLT_MAGIC, TLT_READ_TASK_LOAD, struct tlt_info)

int task_load_track_init(void);
void task_load_track_exit(void);

#endif // __TASK_LOAD_TRACK_H__
