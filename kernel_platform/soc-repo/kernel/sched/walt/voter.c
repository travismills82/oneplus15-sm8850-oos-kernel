// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2017, 2019-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "voter.h"
#include "trace.h"

static DEFINE_SPINLOCK(votable_list_slock);
static LIST_HEAD(votable_list);

/**
 * vote_set_any()
 * @votable:	votable object
 * @client_id:	client number of the latest voter
 * @eff_res:	sets 0 or 1 based on the voting
 * @eff_id:	Always returns the client_id argument
 *
 * Note that for SET_ANY voter, the value is always same as enabled. There is
 * no idea of a voter abstaining from the election. Hence there is never a
 * situation when the effective_id will be invalid, during election.
 *
 * Context:
 *	Must be called with the votable->lock held
 */
static void vote_set_any(struct votable *votable, int client_id,
				int *eff_res, int *eff_id)
{
	int i;

	*eff_res = 0;

	for (i = 0; i < votable->num_clients; i++)
		*eff_res |= votable->votes[i].enabled;

	*eff_id = client_id;
}

/**
 * vote_min() -
 * @votable:	votable object
 * @client_id:	client number of the latest voter
 * @eff_res:	sets this to the min. of all the values amongst enabled voters.
 *		If there is no enabled client, this is set to INT_MAX
 * @eff_id:	sets this to the client id that has the min value amongst all
 *		the enabled clients. If there is no enabled client, sets this
 *		to -EINVAL
 *
 * Context:
 *	Must be called with the votable->lock held
 */
static void vote_min(struct votable *votable, int client_id,
				int *eff_res, int *eff_id)
{
	int i;

	*eff_res = INT_MAX;
	*eff_id = -EINVAL;
	for (i = 0; i < votable->num_clients; i++) {
		if (votable->votes[i].enabled
			&& *eff_res > votable->votes[i].value) {
			*eff_res = votable->votes[i].value;
			*eff_id = i;
		}
	}
	if (*eff_id == -EINVAL)
		*eff_res = -EINVAL;
}

/**
 * vote_max() -
 * @votable:	votable object
 * @client_id:	client number of the latest voter
 * @eff_res:	sets this to the max. of all the values amongst enabled voters.
 *		If there is no enabled client, this is set to -EINVAL
 * @eff_id:	sets this to the client id that has the max value amongst all
 *		the enabled clients. If there is no enabled client, sets this to
 *		-EINVAL
 *
 * Context:
 *	Must be called with the votable->lock held
 */
static void vote_max(struct votable *votable, int client_id,
				int *eff_res, int *eff_id)
{
	int i;

	*eff_res = INT_MIN;
	*eff_id = -EINVAL;
	for (i = 0; i < votable->num_clients; i++) {
		if (votable->votes[i].enabled &&
				*eff_res < votable->votes[i].value) {
			*eff_res = votable->votes[i].value;
			*eff_id = i;
		}
	}
	if (*eff_id == -EINVAL)
		*eff_res = -EINVAL;
}

/**
 * get_client_vote() -
 * get_client_vote_locked() -
 *             The unlocked and locked variants of getting a client's voted
 *             value.
 * @votable:   the votable object
 * @client_id: client of interest
 *
 * Returns:
 *     The value the client voted for. -EINVAL is returned if the client
 *     is not enabled or the client is not found.
 */
int get_client_vote_locked(struct votable *votable, int client_id)
{

	if (!votable || (client_id < 0))
		return -EINVAL;

	if ((votable->type != VOTE_SET_ANY)
	    && !votable->votes[client_id].enabled)
		return -EINVAL;

	return votable->votes[client_id].value;
}

int get_client_vote(struct votable *votable, int client_id)
{
	int value;
	unsigned long flags;

	if (!votable || (client_id < 0))
		return -EINVAL;

	raw_spin_lock_irqsave(&votable->vote_lock, flags);
	value = get_client_vote_locked(votable, client_id);
	raw_spin_unlock_irqrestore(&votable->vote_lock, flags);
	return value;
}


/**
 * get_effective_result() -
 * get_effective_result_locked() -
 *		The unlocked and locked variants of getting the effective value
 *		amongst all the enabled voters.
 *
 * @votable:	the votable object
 *
 * Returns:
 *	The effective result.
 *	For MIN and MAX votable, returns -EINVAL when the votable
 *	object has been created but no clients have casted their votes or
 *	the last enabled client disables its vote.
 *	For SET_ANY votable it returns 0 when no clients have casted their votes
 *	because for SET_ANY there is no concept of abstaining from election. The
 *	votes for all the clients of SET_ANY votable is defaulted to false.
 */
int get_effective_result_locked(struct votable *votable)
{
	if (!votable)
		return -EINVAL;

	return votable->effective_result;
}

int get_effective_result(struct votable *votable)
{
	int value;
	unsigned long flags;

	if (!votable)
		return -EINVAL;

	raw_spin_lock_irqsave(&votable->vote_lock, flags);
	value = get_effective_result_locked(votable);
	raw_spin_unlock_irqrestore(&votable->vote_lock, flags);
	return value;
}

/**
 * get_effective_client() -
 * get_effective_client_locked() -
 *		The unlocked and locked variants of getting the effective client
 *		amongst all the enabled voters.
 *
 * @votable:	the votable object
 *
 * Returns:
 *	The effective client.
 *	For MIN and MAX votable, returns NULL when the votable
 *	object has been created but no clients have casted their votes or
 *	the last enabled client disables its vote.
 *	For SET_ANY votable it returns NULL too when no clients have casted
 *	their votes. But for SET_ANY since there is no concept of abstaining
 *	from election, the only client that casts a vote or the client that
 *	caused the result to change is returned.
 */
int get_effective_client_locked(struct votable *votable)
{
	if (!votable)
		return -EINVAL;

	return votable->effective_client_id;
}

int get_effective_client(struct votable *votable)
{
	unsigned long flags;
	int client_id;

	if (!votable)
		return -EINVAL;

	raw_spin_lock_irqsave(&votable->vote_lock, flags);
	client_id = get_effective_client_locked(votable);
	raw_spin_unlock_irqrestore(&votable->vote_lock, flags);

	return client_id;
}

/**
 * vote() -
 *
 * @votable:	the votable object
 * @client_id:  the voting client
 * @enabled:	This provides a means for the client to exclude himself from
 *		election. This clients val (the next argument) will be
 *		considered only when he has enabled his participation.
 *		Note that this takes a differnt meaning for SET_ANY type, as
 *		there is no concept of abstaining from participation.
 *		Enabled is treated as the boolean value the client is voting.
 * @val:	The vote value. This is ignored for SET_ANY votable types.
 *		For MIN, MAX votable types this value is used as the
 *		clients vote value when the enabled is true, this value is
 *		ignored if enabled is false.
 *
 * The callback is called only when there is a change in the election results or
 * if it is the first time someone is voting.
 * Client needs to ensure that votes for client_id are serialized, votable
 * framework does not serialize vote for a client. It however serializes
 * election i.e. finding the effective result and calling the callback.
 *
 * Returns:
 *	The return from the callback when present and needs to be called
 *	or zero.
 *
 */
int vote(struct votable *votable, int client_id, bool enabled, int val)
{
	int effective_id = -EINVAL;
	int effective_result;
	int rc = 0;
	bool similar_vote = false;
	unsigned long flags;

	if (!votable || (client_id < 0))
		return -EINVAL;


	/*
	 * for SET_ANY the val is to be ignored, set it
	 * to enabled so that the election still works based on
	 * value regardless of the type
	 */
	if (votable->type == VOTE_SET_ANY)
		val = enabled;

	if ((votable->votes[client_id].enabled == enabled) &&
		(votable->votes[client_id].value == val)) {
		similar_vote = true;
	} else {
		votable->votes[client_id].enabled = enabled;
		votable->votes[client_id].value = val;
	}

	if (similar_vote && votable->voted_on)
		return 0;

	raw_spin_lock_irqsave(&votable->vote_lock, flags);
	trace_sched_client_vote(votable->name, client_id, enabled, val);
	switch (votable->type) {
	case VOTE_MIN:
		vote_min(votable, client_id, &effective_result, &effective_id);
		break;
	case VOTE_MAX:
		vote_max(votable, client_id, &effective_result, &effective_id);
		break;
	case VOTE_SET_ANY:
		vote_set_any(votable, client_id,
				&effective_result, &effective_id);
		break;
	default:
		rc = -EINVAL;
		goto out;
	}

	/*
	 * Note that the callback is called with a NULL string and -EINVAL
	 * result when there are no enabled votes
	 */
	if (!votable->voted_on
			|| (effective_result != votable->effective_result)) {
		votable->effective_client_id = effective_id;
		votable->effective_result = effective_result;

		trace_sched_votable_result(votable);

		if (votable->callback)
			rc = votable->callback(votable, votable->data,
					effective_result,
					effective_id);
	}

	votable->voted_on = true;
out:
	raw_spin_unlock_irqrestore(&votable->vote_lock, flags);
	return rc;
}

int rerun_election(struct votable *votable)
{
	int rc = 0;
	int effective_result;
	unsigned long flags;

	if (!votable)
		return -EINVAL;

	raw_spin_lock_irqsave(&votable->vote_lock, flags);
	effective_result = get_effective_result_locked(votable);
	if (votable->callback)
		rc = votable->callback(votable,
			votable->data,
			effective_result,
			votable->effective_client_id);
	raw_spin_unlock_irqrestore(&votable->vote_lock, flags);
	return rc;
}

struct votable *create_votable(const char *name,
				int votable_type,
				int (*callback)(struct votable *votable,
					void *data,
					int effective_result,
					int effective_client),
				void *data)
{
	struct votable *votable;
	unsigned long flags;

	if (!name)
		return ERR_PTR(-EINVAL);

	if (votable_type >= NUM_VOTABLE_TYPES) {
		pr_err("Invalid votable_type specified for voter\n");
		return ERR_PTR(-EINVAL);
	}

	votable = kzalloc(sizeof(struct votable), GFP_KERNEL);
	if (!votable)
		return ERR_PTR(-ENOMEM);

	votable->name = kstrdup(name, GFP_KERNEL);
	if (!votable->name) {
		kfree(votable);
		return ERR_PTR(-ENOMEM);
	}

	votable->num_clients = NUM_MAX_CLIENTS;
	votable->callback = callback;
	votable->type = votable_type;
	votable->data = data;
	raw_spin_lock_init(&votable->vote_lock);

	/*
	 * Because effective_result and client states are invalid
	 * before the first vote, initialize them to -EINVAL
	 */
	votable->effective_result = -EINVAL;
	if (votable->type == VOTE_SET_ANY)
		votable->effective_result = 0;
	votable->effective_client_id = -EINVAL;

	spin_lock_irqsave(&votable_list_slock, flags);
	list_add(&votable->list, &votable_list);
	spin_unlock_irqrestore(&votable_list_slock, flags);

	return votable;
}

void destroy_votable(struct votable *votable)
{
	unsigned long flags;

	if (!votable)
		return;

	spin_lock_irqsave(&votable_list_slock, flags);
	list_del(&votable->list);
	spin_unlock_irqrestore(&votable_list_slock, flags);

	kfree(votable->name);
	kfree(votable);
}
