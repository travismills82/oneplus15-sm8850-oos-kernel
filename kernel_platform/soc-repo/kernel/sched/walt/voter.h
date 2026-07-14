/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2017, 2019-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __PMIC_VOTER_H
#define __PMIC_VOTER_H

#include <linux/mutex.h>

#define NUM_MAX_CLIENTS		8

struct client_vote {
	bool	enabled;
	int	value;
};

struct votable {
	const char		*name;
	struct list_head	list;
	struct client_vote	votes[NUM_MAX_CLIENTS];
	int			num_clients;
	int			type;
	int			effective_client_id;
	int			effective_result;
	raw_spinlock_t		vote_lock;
	void			*data;
	int			(*callback)(struct votable *votable,
						void *data,
						int effective_result,
						int effective_client);
	bool			voted_on;
};

enum votable_type {
	VOTE_MIN,
	VOTE_MAX,
	VOTE_SET_ANY,
	NUM_VOTABLE_TYPES,
};

bool is_client_vote_enabled(struct votable *votable, int client_id);
bool is_client_vote_enabled_locked(struct votable *votable, int client_id);
int get_client_vote(struct votable *votable, int client_id);
int get_client_vote_locked(struct votable *votable, int client_id);
int get_effective_result(struct votable *votable);
int get_effective_result_locked(struct votable *votable);
int get_effective_client(struct votable *votable);
int get_effective_client_locked(struct votable *votable);
int vote(struct votable *votable, int client_id, bool state, int val);
int rerun_election(struct votable *votable);
struct votable *create_votable(const char *name,
				int votable_type,
				int (*callback)(struct votable *votable,
						void *data,
						int effective_result,
						int effective_client),
				void *data);
void destroy_votable(struct votable *votable);

#endif /* __PMIC_VOTER_H */
