#!/usr/bin/env python3
"""Focused source/control-flow checks; not ARM64 ABI or device qualification."""
import re
import subprocess
import unittest

from test_coloros17_unified_kernel_backports import ROOT, function, host_c

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
enum { REGCACHE_NONE, REGCACHE_RBTREE, REGCACHE_MAPLE, REGCACHE_FLAT };
struct regmap;
struct regcache_ops {
 const char *name;
 int type;
 int (*init)(struct regmap *);
 int (*exit)(struct regmap *);
 int (*read)(struct regmap *, unsigned, unsigned *);
 int (*write)(struct regmap *, unsigned, unsigned);
};
struct reg_default { unsigned reg, def; };
struct regmap_config {
 unsigned num_reg_defaults, num_reg_defaults_raw, val_bits;
 const struct reg_default *reg_defaults;
 const void *reg_defaults_raw;
};
struct regmap {
 int cache_type;
 bool cache_bypass, cache_free, max_register_is_set;
 unsigned num_reg_defaults, num_reg_defaults_raw, cache_word_size;
 unsigned cache_size_raw, max_register, reg_stride;
 void *cache;
 const void *reg_defaults_raw;
 struct reg_default *reg_defaults;
 const struct regcache_ops *cache_ops;
 void (*lock)(void *), (*unlock)(void *);
 void *lock_arg, *dev;
};
static int depth, inits, exits, populates, populate_result, init_result;
static void lock_map(void *p) { assert(!depth); ++depth; }
static void unlock_map(void *p) { assert(depth == 1); --depth; }
static int cache_init(struct regmap *p) { assert(!depth); ++inits; return init_result; }
static int cache_exit(struct regmap *p) { assert(!depth); ++exits; return 0; }
static int test_populate(struct regmap *p) { assert(depth == 1); ++populates; return populate_result; }
static int cache_read(struct regmap *p, unsigned r, unsigned *v) { return 0; }
static int cache_write(struct regmap *p, unsigned r, unsigned v) { return 0; }
#define OPS(n, t) struct regcache_ops n = {#n, t, cache_init, cache_exit, cache_read, cache_write}
OPS(regcache_rbtree_ops, REGCACHE_RBTREE);
OPS(regcache_maple_ops, REGCACHE_MAPLE);
OPS(regcache_flat_ops, REGCACHE_FLAT);
#define dev_warn(...) ((void)0)
#define dev_err(...) ((void)0)
#define dev_dbg(...) ((void)0)
#define GFP_KERNEL 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DIV_ROUND_UP(x, d) (((x) + (d) - 1) / (d))
#define kfree(p) free((void *)(p))
static void *kmemdup_array(const void *p, size_t n, size_t s, int flags) {
 void *copy = malloc(n * s);
 if (copy) memcpy(copy, p, n * s);
 return copy;
}
static int regcache_hw_init(struct regmap *p) { return -EINVAL; }
'''

MAIN = r'''
int main(void) {
 struct reg_default def = {0, 1};
 struct regmap_config config = {.num_reg_defaults = 1, .reg_defaults = &def, .val_bits = 8};
 struct regmap map = {.cache_type = REGCACHE_FLAT, .reg_stride = 1,
                      .lock = lock_map, .unlock = unlock_map};
 populate_result = POPULATE_RESULT;
 init_result = INIT_RESULT;
 int ret = regcache_init(&map, &config);
 assert(inits == 1 && !depth);
 assert(ret == (INIT_RESULT ? INIT_RESULT : POPULATE_RESULT));
 assert(populates == (INIT_RESULT ? 0 : 1));
 assert(exits == (!INIT_RESULT && POPULATE_RESULT ? 1 : 0));
 if (!ret) free(map.reg_defaults);
 return 0;
}
'''

BSG_PREFIX = r'''
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
struct device { int value; };
struct queue_limits { int value; };
struct bsg_job { void *reply; };
struct request { struct bsg_job job; };
struct request_queue { int value; };
struct blk_mq_tag_set { int value; };
struct bsg_set { struct blk_mq_tag_set tag_set; unsigned reply_len; };
typedef int (bsg_job_fn)(struct bsg_job *);
typedef int (bsg_timeout_fn)(struct request *);
#define container_of(p, t, m) ((t *)((char *)(p) - offsetof(t, m)))
#define blk_mq_rq_to_pdu(rq) (&(rq)->job)
#define GFP_KERNEL 0
#define SCSI_SENSE_BUFFERSIZE 96
#define kzalloc(n, flags) calloc(1, n)
static struct bsg_set set;
static struct request_queue queue;
static struct device device;
static struct queue_limits limits;
static int request_fn(struct bsg_job *j) { return 0; }
static int timeout_fn(struct request *r) { return 0; }
static struct request_queue *bsg_setup_queue_with_reply_len(struct device *dev,
 const char *name, struct queue_limits *lim, bsg_job_fn *job_fn,
 bsg_timeout_fn *timeout, int dd_job_size, unsigned reply_len) {
 assert(dev == &device && name[0] == 'b' && lim == &limits);
 assert(job_fn == request_fn && timeout == timeout_fn && dd_job_size == 17);
 set.reply_len = reply_len;
 return &queue;
}
'''

BSG_MAIN = r'''
int main(void) {
 struct request rq = {0};
 assert(bsg_setup_queue(&device, "bsg", &limits, request_fn, timeout_fn, 17) == &queue);
 assert(set.reply_len == 96);
 assert(!bsg_init_rq(&set.tag_set, &rq, 0, 0));
 assert(rq.job.reply);
 ((volatile char *)rq.job.reply)[95] = 1;
 free(rq.job.reply);
 assert(bsg_setup_queue_with_reply_len(&device, "bsg", &limits,
         request_fn, timeout_fn, 17, 104) == &queue);
 assert(!bsg_init_rq(&set.tag_set, &rq, 0, 0));
 assert(rq.job.reply);
 ((volatile char *)rq.job.reply)[103] = 1;
 free(rq.job.reply);
 return 0;
}
'''

UNICODE_PREFIX = r'''
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
enum { UTF8_NFDI, UTF8_NFDICF, UTF8_NMAX };
struct rcu_head { void *next; void (*func)(struct rcu_head *); };
struct utf8data { int value; };
struct utf8data_table {
 const struct utf8data *utf8nfdidata, *utf8nfdicfdata;
 unsigned utf8nfdidata_size, utf8nfdicfdata_size;
};
static struct utf8data entry;
static struct utf8data_table table = {&entry, &entry, 1, 1};
static void *allocated;
static int puts, refs, frees, fail_alloc, unsupported;
static struct rcu_head *queued;
static void (*queued_fn)(struct rcu_head *);
static void *test_zalloc(size_t n) {
 if (fail_alloc) return NULL;
 assert(!allocated);
 return allocated = calloc(1, n);
}
static void test_free(void *p) {
 assert(p == allocated);
 free(p); allocated = NULL; ++frees;
}
static const struct utf8data_table *test_get(void) { ++refs; return &table; }
static void test_put(void) { assert(refs == 1); --refs; ++puts; }
static void call_rcu(struct rcu_head *head, void (*fn)(struct rcu_head *)) {
 assert(!queued && refs == 1 && !frees); queued = head; queued_fn = fn;
}
#define GFP_KERNEL 0
#define kzalloc(n, flags) test_zalloc(n)
#define kfree(p) test_free(p)
#define symbol_request(x) test_get()
#define symbol_put(x) test_put()
#define ERR_PTR(e) ((void *)(long)(e))
#define container_of(p, t, m) ((t *)((char *)(p) - offsetof(t, m)))
'''

UNICODE_HELPERS = r'''
static bool utf8version_is_supported(const struct unicode_map *um, unsigned version) {
 return !unsupported;
}
static const struct utf8data *find_table_version(const struct utf8data *data,
 unsigned size, unsigned version) { return data; }
'''

IO_POLL_PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>
typedef struct { _Atomic int value; } atomic_t;
struct rcu_head { void *next; void (*func)(struct rcu_head *); };
struct percpu_ref { _Atomic int count; };
struct io_ring_ctx {
 bool poll_wq;
 struct percpu_ref refs;
 struct rcu_head poll_wq_rcu;
 atomic_t poll_wq_rcu_pending;
};
static struct io_ring_ctx ctx;
static struct rcu_head *queued;
static void (*queued_fn)(struct rcu_head *);
static int wakes;
#define container_of(p, t, m) ((t *)((char *)(p) - offsetof(t, m)))
static void atomic_set(atomic_t *a, int n) { atomic_store(&a->value, n); }
static int atomic_xchg(atomic_t *a, int n) { return atomic_exchange(&a->value, n); }
static bool wq_has_sleeper(bool *p) { return *p; }
static void percpu_ref_get(struct percpu_ref *p) { atomic_fetch_add(&p->count, 1); }
static void percpu_ref_put(struct percpu_ref *p) { assert(atomic_fetch_sub(&p->count, 1) == 1); }
static void io_poll_wq_wake(struct io_ring_ctx *p) {
 assert(!atomic_load(&p->poll_wq_rcu_pending.value));
 assert(atomic_load(&p->refs.count) == 1); ++wakes;
}
static void call_rcu_hurry(struct rcu_head *head, void (*fn)(struct rcu_head *)) {
 assert(!queued && !queued_fn); queued = head; queued_fn = fn;
}
'''

IO_POLL_MAIN = r'''
static void *wake_thread(void *arg) { io_local_work_wake_poll(&ctx); return NULL; }
static void flush(void) {
 assert(queued == &ctx.poll_wq_rcu && queued_fn);
 struct rcu_head *head = queued;
 void (*fn)(struct rcu_head *) = queued_fn;
 queued = NULL; queued_fn = NULL;
 fn(head);
}
int main(void) {
 io_local_work_wake_poll(&ctx);
 assert(!queued && !atomic_load(&ctx.refs.count));
 ctx.poll_wq = true;
 pthread_t threads[16];
 for (int i = 0; i < 16; ++i) assert(!pthread_create(&threads[i], NULL, wake_thread, NULL));
 for (int i = 0; i < 16; ++i) assert(!pthread_join(threads[i], NULL));
 assert(atomic_load(&ctx.refs.count) == 1 && !wakes);
 assert(atomic_load(&ctx.poll_wq_rcu_pending.value) == 1);
 flush();
 assert(!atomic_load(&ctx.refs.count) && wakes == 1);
 io_local_work_wake_poll(&ctx);
 assert(atomic_load(&ctx.refs.count) == 1);
 flush();
 assert(!atomic_load(&ctx.refs.count) && wakes == 2);
 return 0;
}
'''


class Linux612115Kabi(unittest.TestCase):
    def regcache_case(self, init=0, populate=0):
        text = (ROOT / 'kernel_platform/common/drivers/base/regmap/regcache.c').read_text()
        backend = re.search(r'struct regcache_backend \{.*?\n\};', text, re.S)
        registry = re.search(r'static const struct regcache_backend cache_types\[\] = \{.*?\n\};', text, re.S)
        self.assertIsNotNone(backend)
        self.assertIsNotNone(registry)
        # Exercise the optional callback in a host-only fixture. The production
        # three backends currently leave it NULL; no kernel source is modified.
        table = registry[0].replace('.ops = &regcache_flat_ops',
                                    '.ops = &regcache_flat_ops, .populate = test_populate')
        main = MAIN.replace('INIT_RESULT', str(init)).replace('POPULATE_RESULT', str(populate))
        result = host_c(PREFIX + backend[0] + table +
                        function('drivers/base/regmap/regcache.c', 'int regcache_init(') + main)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_regcache_init_unlocked_populate_locked(self):
        self.regcache_case()

    def test_regcache_populate_failure_unlocked_cleanup(self):
        self.regcache_case(populate=-5)

    def test_regcache_init_failure_frees_defaults(self):
        self.regcache_case(init=-12)

    def test_regcache_external_operations_declaration_preserved(self):
        path = 'kernel_platform/common/drivers/base/regmap/internal.h'
        old = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                       '79c418a771a8717edb4a3ed2b814fb9e89ec4a7c:' + path], text=True)
        new = (ROOT / path).read_text()
        pattern = r'struct regcache_ops \{.*?\n\};'
        self.assertEqual(re.search(pattern, old, re.S)[0], re.search(pattern, new, re.S)[0])

    def test_regcache_backends_remain_core_owned(self):
        base = ROOT / 'kernel_platform/common/drivers/base/regmap'
        for name in ['regcache-flat.c', 'regcache-rbtree.c', 'regcache-maple.c']:
            self.assertNotIn('.populate', (base / name).read_text())
        # Retain the stable IRQ-safe maple teardown, independently of the
        # operations-table representation repair.
        maple = (base / 'regcache-maple.c').read_text()
        self.assertIn('mas_lock_irqsave(&mas, flags);', maple)
        self.assertIn('mas_unlock_irqrestore(&mas, flags);', maple)

    def test_bsg_legacy_signature_preserved(self):
        path = 'kernel_platform/common/include/linux/bsg-lib.h'
        old = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                       '79c418a771a8717edb4a3ed2b814fb9e89ec4a7c:' + path], text=True)
        new = (ROOT / path).read_text()
        pattern = r'struct request_queue \*bsg_setup_queue\(.*?\);'
        self.assertEqual(re.search(pattern, old, re.S)[0], re.search(pattern, new, re.S)[0])

    def test_bsg_actual_legacy_forwarding_and_sized_allocation(self):
        wrapper = function('block/bsg-lib.c', 'struct request_queue *bsg_setup_queue(')
        alloc = function('block/bsg-lib.c', 'static int bsg_init_rq(')
        result = host_c(BSG_PREFIX + wrapper + alloc + BSG_MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_bsg_undersized_reply_negative_control(self):
        wrapper = function('block/bsg-lib.c', 'struct request_queue *bsg_setup_queue(')
        alloc = function('block/bsg-lib.c', 'static int bsg_init_rq(')
        self.assertEqual(alloc.count('kzalloc(bset->reply_len, GFP_KERNEL)'), 1)
        alloc = alloc.replace('kzalloc(bset->reply_len, GFP_KERNEL)',
                              'kzalloc(SCSI_SENSE_BUFFERSIZE + 0 * bset->reply_len, GFP_KERNEL)')
        result = host_c(BSG_PREFIX + wrapper + alloc + BSG_MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_ufs_uses_sized_reply_not_legacy_wrapper(self):
        probe = function('drivers/ufs/core/ufs_bsg.c', 'int ufs_bsg_probe(')
        self.assertIn('bsg_setup_queue_with_reply_len(', probe)
        self.assertIn('sizeof(struct ufs_rpmb_reply)', probe)
        self.assertNotIn('bsg_setup_queue(', probe)
        transport = function('block/bsg-lib.c', 'static int bsg_transport_sg_io_fn(')
        self.assertIn('job->reply_len = bset->reply_len;', transport)

    def unicode_case(self, main):
        source = (ROOT / 'kernel_platform/common/fs/unicode/utf8-core.c').read_text()
        header = (ROOT / 'kernel_platform/common/include/linux/unicode.h').read_text()
        public = re.search(r'struct unicode_map \{.*?\n\};', header, re.S)[0]
        private = re.search(r'struct unicode_map_rcu \{.*?\n\};', source, re.S)[0]
        code = UNICODE_PREFIX + public + private + UNICODE_HELPERS
        for signature in ['struct unicode_map *utf8_load(',
                          'static void utf8_unload_rcu(', 'void utf8_unload(']:
            code += function('fs/unicode/utf8-core.c', signature)
        result = host_c(code + main)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_unicode_deferred_table_and_allocation_lifetime(self):
        self.unicode_case(r'''
int main(void) {
 _Static_assert(sizeof(struct unicode_map) == 32, "public prefix");
 _Static_assert(offsetof(struct unicode_map_rcu, map) == 0, "allocation base");
 struct unicode_map *um = utf8_load(1);
 assert(um == allocated && refs == 1 && !puts && !frees);
 utf8_unload(um);
 assert(queued && queued_fn && refs == 1 && !puts && !frees);
 queued_fn(queued);
 assert(!refs && puts == 1 && frees == 1 && !allocated);
 return 0;
}
''')

    def test_unicode_load_failure_releases_private_allocation(self):
        self.unicode_case(r'''
int main(void) {
 unsupported = 1;
 assert(utf8_load(1) == ERR_PTR(-EINVAL));
 assert(!allocated && !refs && puts == 1 && frees == 1 && !queued);
 return 0;
}
''')

    def test_unicode_allocation_failure_and_null_unload(self):
        self.unicode_case(r'''
int main(void) {
 fail_alloc = 1;
 assert(utf8_load(1) == ERR_PTR(-ENOMEM));
 utf8_unload(NULL);
 assert(!allocated && !refs && !puts && !frees && !queued);
 return 0;
}
''')

    def test_unicode_public_declaration_restored(self):
        path = 'kernel_platform/common/include/linux/unicode.h'
        old = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                       '79c418a771a8717edb4a3ed2b814fb9e89ec4a7c:' + path], text=True)
        new = (ROOT / path).read_text()
        pattern = r'struct unicode_map \{.*?\n\};'
        self.assertEqual(re.search(pattern, old, re.S)[0], re.search(pattern, new, re.S)[0])

    def test_io_poll_reference_and_coalescing(self):
        code = function('io_uring/io_uring.c', 'static void io_poll_wq_wake_rcu(')
        code += function('io_uring/io_uring.c', 'static void io_local_work_wake_poll(')
        result = host_c(IO_POLL_PREFIX + code + IO_POLL_MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_io_poll_missing_reference_negative_control(self):
        code = function('io_uring/io_uring.c', 'static void io_poll_wq_wake_rcu(')
        code += function('io_uring/io_uring.c', 'static void io_local_work_wake_poll(')
        self.assertEqual(code.count('percpu_ref_get(&ctx->refs);'), 1)
        code = code.replace('percpu_ref_get(&ctx->refs);', '(void)percpu_ref_get;')
        result = host_c(IO_POLL_PREFIX + code + IO_POLL_MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_io_poll_fix_body_unchanged_and_state_at_tail(self):
        path = 'kernel_platform/common/io_uring/io_uring.c'
        raw = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                       '46fb47434d1067af6076f6491fc046ba6bf912ba:' + path], text=True)
        for signature in ['static void io_poll_wq_wake_rcu(', 'static void io_local_work_wake_poll(']:
            self.assertIn(function('io_uring/io_uring.c', signature), raw)
        header = (ROOT / 'kernel_platform/common/include/linux/io_uring_types.h').read_text()
        self.assertGreater(header.index('ANDROID_KABI_IGNORE(9, struct rcu_head poll_wq_rcu)'),
                           header.index('**sqe_pages;'))
        self.assertIn('ANDROID_KABI_IGNORE(10, atomic_t poll_wq_rcu_pending)', header)

    def test_perf_budget_consumes_reserve_without_counter_alias(self):
        header = (ROOT / 'kernel_platform/common/include/linux/sched/user.h').read_text()
        self.assertIn('ANDROID_KABI_USE(1, atomic_long_t perf_mlock);', header)
        self.assertGreater(header.index('ANDROID_KABI_USE(1, atomic_long_t perf_mlock)'),
                           header.index('struct ratelimit_state ratelimit;'))
        self.assertIn('atomic_long_t locked_vm;', header)

    def test_perf_accounting_and_user_allocation_implementation_unchanged(self):
        for path in ['kernel/events/core.c', 'kernel/user.c']:
            full = 'kernel_platform/common/' + path
            raw = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                           '46fb47434d1067af6076f6491fc046ba6bf912ba:' + full])
            self.assertEqual((ROOT / full).read_bytes(), raw)
        user = (ROOT / 'kernel_platform/common/kernel/user.c').read_text()
        self.assertIn('kmem_cache_zalloc(uid_cachep, GFP_KERNEL)', user)


if __name__ == '__main__':
    unittest.main()
