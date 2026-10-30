/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NF_CONNTRACK_EXPECT_PRIV_H
#define _NF_CONNTRACK_EXPECT_PRIV_H

#include <net/netfilter/nf_conntrack_expect.h>

/*
 * Keep the Android/OEM expectation object intact. Every expectation comes from
 * nf_ct_expect_alloc(); only its private slab allocation grows. In particular,
 * the legacy timer_list is never armed or used as a second lifetime owner.
 * RCU readers must not follow exp->master after hash removal: retain the
 * immutable namespace, zone, creator and master identity beside the object.
 */
struct nf_conntrack_expect_priv {
	struct nf_conntrack_expect exp;
	possible_net_t net;
#ifdef CONFIG_NF_CONNTRACK_ZONES
	struct nf_conntrack_zone zone;
#endif
	struct nf_conntrack_helper __rcu *creator;
	struct nf_conntrack_helper *assign_ref;
	struct nf_conntrack_tuple master_tuple;
	u32 master_id;
	u32 expires;
};

static_assert(offsetof(struct nf_conntrack_expect_priv, exp) == 0);

static inline struct nf_conntrack_expect_priv *
nf_ct_expect_priv(const struct nf_conntrack_expect *exp)
{
	return container_of(exp, struct nf_conntrack_expect_priv, exp);
}

static inline bool nf_ct_exp_zone_equal_any(const struct nf_conntrack_expect *exp,
					 const struct nf_conntrack_zone *zone)
{
#ifdef CONFIG_NF_CONNTRACK_ZONES
	return nf_ct_expect_priv(exp)->zone.id == zone->id;
#else
	return true;
#endif
}

static inline const struct nf_conntrack_zone *
nf_ct_exp_zone(const struct nf_conntrack_expect *exp)
{
#ifdef CONFIG_NF_CONNTRACK_ZONES
	return &nf_ct_expect_priv(exp)->zone;
#else
	return &nf_ct_zone_dflt;
#endif
}

static inline bool nf_ct_exp_is_expired(const struct nf_conntrack_expect *exp)
{
	return (s32)(READ_ONCE(nf_ct_expect_priv(exp)->expires) - nfct_time_stamp) <= 0 ||
	       (READ_ONCE(exp->flags) & NF_CT_EXPECT_DEAD);
}

#endif /* _NF_CONNTRACK_EXPECT_PRIV_H */
