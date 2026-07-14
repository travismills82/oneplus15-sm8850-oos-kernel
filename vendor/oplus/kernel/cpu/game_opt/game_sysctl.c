#include <linux/sysctl.h>

#include "game_sysctl.h"

extern struct ctl_table_poll framesync_poll;
extern struct ctl_table_poll hybrid_frame_poll;

static int proc_doepoll(
	const struct ctl_table *table, int write,
	void *buffer, unsigned long *lenp, long long *ppos)
{
	return -ENOSYS;
}

static struct ctl_table hfs_table[] = {
	{
		.procname	=	"epoll_notify",
		.mode		=	0664,
		.proc_handler	=	proc_doepoll,
		.poll		=	&framesync_poll,
	},
	{
		.procname	=	"hybrid_frame",
		.mode		=	0664,
		.proc_handler	=	proc_doepoll,
		.poll		=	&hybrid_frame_poll,
	},
};

struct ctl_table_header *hdr;

int game_sysctl_init(void)
{
	hdr = register_sysctl("game_frame_sync", hfs_table);
	return 0;
}

void game_sysctl_deinit(void)
{
	unregister_sysctl_table(hdr);
}
