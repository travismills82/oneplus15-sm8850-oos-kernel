// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Oplus. All rights reserved.
 */

#ifndef __HYBRID_FRAME_SYNC_H__
#define __HYBRID_FRAME_SYNC_H__

enum hybrid_frame_sync_id
{
	FRAME_HYBRID_FIRST_ID,
	FRAME_HYBRID_WR,
	FRAME_HYBRID_RD,
	FRAME_HYBRID_WAKE,
	FRAME_HYBRID_MAX_ID,
};

enum hybrid_frame_mode
{
	HYBRID_FRAME_NONE,
	HYBRID_FRAME_PRODUCE,
	HYBRID_FRAME_CONSUME,
	HYBRID_FRAME_SCHED_FRAME,
	HYBRID_FRAME_VSYNC_APP,
	HYBRID_FRAME_RENDER_PASS,

	HYBRID_FRAME_END,
};

struct hybrid_frame_data
{
	int mode;
	int end;
	union {
		int reserved[62];

		struct {
			int buffer_num;
			long long produce_time;
		} produce_data;

		struct {
			long long consume_time;
		} consume_data;

		struct {
			int buffer_num;
			long long produce_time;
			long long consume_time;
			int ds_status;
		} sched_frame_data;

		struct {
			long long render_time;
			long long composite_time;
		} vsync_data;

		struct {
			int total_rp;
			int current_rp;
			long long time;
		} rp_data;
	};
};

#define HYBRID_FRAME_SYNC_MAGIC 0xE1
#define CMD_ID_FRAME_HYBRID_WR \
	_IOWR(HYBRID_FRAME_SYNC_MAGIC, FRAME_HYBRID_WR, struct hybrid_frame_data)
#define CMD_ID_FRAME_HYBRID_RD \
	_IOWR(HYBRID_FRAME_SYNC_MAGIC, FRAME_HYBRID_RD, struct hybrid_frame_data)
#define CMD_ID_FRAME_HYBRID_WAKE \
	_IOWR(HYBRID_FRAME_SYNC_MAGIC, FRAME_HYBRID_WAKE, struct hybrid_frame_data)

int hybrid_frame_sync_init(void);
void hybrid_frame_sync_exit(void);

#endif /* __HYBRID_FRAME_SYNC_H__ */
