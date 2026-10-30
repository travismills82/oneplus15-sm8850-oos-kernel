// SPDX-License-Identifier: GPL-2.0-only
/* Disposable VM test only; never built into the production Canoe image.
 * Uses real expectation APIs to verify pair rollback/retry and DEAD rejection.
 * Synthetic unconfirmed masters are allocated and freed by this test; they do
 * not pretend to be a real packet-path H.323 session. FTP stress is separate.
 */
#include <linux/module.h>
#include <linux/inet.h>
#include <linux/rcupdate.h>
#include <net/net_namespace.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include "nf_conntrack_expect_priv.h"

static const struct nf_conntrack_expect_policy policy = {
	.max_expected = 8,
	.timeout = 30,
	.name = "pair-test",
};

static int dummy_help(struct sk_buff *skb, unsigned int protoff,
		      struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	return NF_ACCEPT;
}

static struct nf_conntrack_helper helper = {
	.name = "cve80668-test",
	.me = THIS_MODULE,
	.expect_policy = &policy,
	.expect_class_max = 0,
	.help = dummy_help,
	.refcnt = REFCOUNT_INIT(1),
};

static struct nf_conntrack_helper retired_helper = {
	.name = "cve80668-gone",
	.me = THIS_MODULE,
	.expect_policy = &policy,
	.help = dummy_help,
	.refcnt = REFCOUNT_INIT(0),
};

static struct nf_conn *make_master(u16 port)
{
	struct nf_conntrack_tuple original = {}, reply = {};
	struct nf_conn_help *help;
	struct nf_conn *ct;

	original.src.l3num = AF_INET;
	original.src.u3.ip = in_aton("10.77.0.1");
	original.dst.u3.ip = in_aton("10.77.0.2");
	original.src.u.tcp.port = htons(port);
	original.dst.u.tcp.port = htons(1720);
	original.dst.protonum = IPPROTO_TCP;
	if (!nf_ct_invert_tuple(&reply, &original))
		return ERR_PTR(-EINVAL);
	ct = nf_conntrack_alloc(&init_net, &nf_ct_zone_dflt, &original, &reply,
				GFP_KERNEL);
	if (IS_ERR(ct))
		return ct;
	/* Alloc returns an unconfirmed, unreferenced object owned only here.
	 * It is never inserted into conntrack's hash or exposed to packet path.
	 */
	help = nf_ct_helper_ext_add(ct, GFP_KERNEL);
	if (!help) {
		nf_conntrack_free(ct);
		return ERR_PTR(-ENOMEM);
	}
	rcu_assign_pointer(help->helper, &helper);
	return ct;
}

static struct nf_conntrack_expect *make_expect(struct nf_conn *ct, u16 port)
{
	union nf_inet_addr source = { .ip = in_aton("10.77.0.2") };
	union nf_inet_addr destination = { .ip = in_aton("10.77.0.1") };
	struct nf_conntrack_expect *exp;
	__be16 dport = htons(port);

	exp = nf_ct_expect_alloc(ct);
	if (!exp)
		return NULL;
	nf_ct_expect_init(exp, NF_CT_EXPECT_CLASS_DEFAULT, AF_INET, &source,
			  &destination, IPPROTO_UDP, NULL, &dport);
	return exp;
}

static int __init pair_test_init(void)
{
	struct nf_conntrack_expect *pair[2] = {}, *blocker = NULL;
	struct nf_conn *master = NULL, *other = NULL;
	struct nf_conn_help *help;
	u32 master_id = 0;
	int result = -EINVAL;

	master = make_master(42000);
	if (IS_ERR(master))
		return PTR_ERR(master);
	other = make_master(42001);
	if (IS_ERR(other)) {
		result = PTR_ERR(other);
		other = NULL;
		goto out;
	}
	rcu_read_lock();
	pair[0] = make_expect(master, 37000);
	pair[1] = make_expect(master, 37001);
	blocker = make_expect(other, 37001);
	if (!pair[0] || !pair[1] || !blocker) {
		result = -ENOMEM;
		goto out_rcu;
	}
	help = nfct_help(master);
	master_id = nf_ct_get_id(master);
	/* A dynamic helper retires refcnt from one to zero before deletion.
	 * Failed creator/assignment pins must not leak or publish an object.
	 */
	rcu_assign_pointer(help->helper, &retired_helper);
	{
		struct nf_conntrack_expect *late = nf_ct_expect_alloc(master);

		if (late) {
			nf_ct_expect_put(late);
			result = -EINVAL;
			goto out_rcu;
		}
	}
	rcu_assign_pointer(help->helper, &helper);
	pair[0]->helper = &retired_helper;
	result = nf_ct_expect_related_pair(pair, 0);
	if (result != -ESHUTDOWN || help->expecting[0]) {
		result = -EINVAL;
		goto out_rcu;
	}
	pr_info("CVE80668_PAIR: PASS retired creator and assignment pins fail closed\n");
	pair[0]->helper = &helper;
	pair[1]->helper = &helper;
	result = nf_ct_expect_related(blocker, 0);
	if (result)
		goto out_rcu;
	result = nf_ct_expect_related_pair(pair, 0);
	if (result != -EBUSY || help->expecting[0] ||
	    !hlist_empty(&help->expectations) ||
	    !hlist_unhashed(&pair[0]->hnode) ||
	    !hlist_unhashed(&pair[1]->hnode) ||
	    refcount_read(&pair[0]->use) != 1 ||
	    refcount_read(&pair[1]->use) != 1) {
		pr_err("CVE80668_PAIR: FAIL second-tuple rollback result=%d count=%u\n",
		       result, help->expecting[0]);
		result = -EINVAL;
		goto out_rcu;
	}
	pr_info("CVE80668_PAIR: PASS second-tuple conflict rolls back first count and publishes neither\n");
	nf_ct_remove_expectations(other);
	result = nf_ct_expect_related_pair(pair, 0);
	if (result || help->expecting[0] != 2 ||
	    refcount_read(&pair[0]->use) != 2 ||
	    refcount_read(&pair[1]->use) != 2 ||
	    timer_pending(&pair[0]->timeout) || pair[0]->timeout.function) {
		pr_err("CVE80668_PAIR: FAIL retry result=%d count=%u\n", result,
		       help->expecting[0]);
		result = -EINVAL;
		goto out_rcu;
	}
	pr_info("CVE80668_PAIR: PASS same pair safely reused after failed uncommitted insertion\n");
	if (refcount_read(&helper.refcnt) != 6) {
		pr_err("CVE80668_PAIR: FAIL creator/assignment refs after retry=%u\n",
		       refcount_read(&helper.refcnt));
		result = -EINVAL;
		goto out_rcu;
	}
	pr_info("CVE80668_PAIR: PASS retries retain exactly three creator and two assignment pins\n");
	nf_ct_unexpect_related(pair[0]);
	if (!(READ_ONCE(pair[0]->flags) & NF_CT_EXPECT_DEAD)) {
		result = -EINVAL;
		goto out_rcu;
	}
	if (!IS_ENABLED(CONFIG_DEBUG_NET)) {
		/* Production-like DEBUG_NET=n: this invalid API call must return
		 * -EINVAL; DEBUG_NET=y intentionally additionally warns. Do not
		 * hide that warning or disable panic-on-warn in the strict VM.
		 */
		result = nf_ct_expect_related_pair(pair, 0);
		if (result != -EINVAL || help->expecting[0] != 2 ||
		    refcount_read(&pair[0]->use) != 2 ||
		    refcount_read(&pair[1]->use) != 2) {
			result = -EINVAL;
			goto out_rcu;
		}
		pr_info("CVE80668_PAIR: PASS DEAD reinsert rejected, counts unchanged (DEBUG_NET=n)\n");
	} else {
		pr_info("CVE80668_PAIR: SKIP deliberate DEAD reinsert WARN with DEBUG_NET=y; strict panic-on-warn retained\n");
	}
	nf_ct_remove_expectations(master);
	if (help->expecting[0] || !hlist_empty(&help->expectations) ||
	    refcount_read(&pair[0]->use) != 1 ||
	    refcount_read(&pair[1]->use) != 1) {
		result = -EINVAL;
		goto out_rcu;
	}
	result = 0;
out_rcu:
	rcu_read_unlock();
out:
	if (master)
		nf_ct_remove_expectations(master);
	if (other)
		nf_ct_remove_expectations(other);
	/* Test holds only caller expectation refs after list removal. Release
	 * masters first, then verify namespace/tuple snapshots without touching
	 * an already-freed SLAB_TYPESAFE_BY_RCU master object.
	 */
	if (other)
		nf_conntrack_free(other);
	if (master)
		nf_conntrack_free(master);
	if (!result && (nf_ct_exp_net(pair[0]) != &init_net ||
	    nf_ct_expect_priv(pair[0])->master_tuple.src.u3.ip != in_aton("10.77.0.1") ||
	    nf_ct_expect_priv(pair[0])->master_id != master_id ||
	    rcu_access_pointer(nf_ct_expect_priv(pair[0])->creator) != &helper)) {
		pr_err("CVE80668_PAIR: FAIL immutable metadata after master free\n");
		result = -EINVAL;
	}
	if (blocker)
		nf_ct_expect_put(blocker);
	if (pair[1])
		nf_ct_expect_put(pair[1]);
	if (pair[0])
		nf_ct_expect_put(pair[0]);
	rcu_barrier();
	if (refcount_read(&helper.refcnt) != 1 ||
	    refcount_read(&retired_helper.refcnt) != 0) {
		pr_err("CVE80668_PAIR: FAIL helper pin leak after RCU drain\n");
		result = -EINVAL;
	}
	if (!result)
		pr_info("CVE80668_PAIR: PASS cleanup, snapshot lifetime and RCU drain\n");
	else
		pr_err("CVE80668_PAIR: FAIL result=%d\n", result);
	return result;
}

static void __exit pair_test_exit(void) { }
module_init(pair_test_init);
module_exit(pair_test_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Disposable VM conntrack pair rollback regression test");
