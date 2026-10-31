#!/usr/bin/env python3
"""Host control-flow/provenance tests, not ARM64 or device qualification."""
import csv
from pathlib import Path
import unittest

from test_coloros17_unified_kernel_backports import function, host_c

ROOT = Path(__file__).resolve().parents[2]

IDENTITY_PREFIX = r'''
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
typedef uint64_t u64;
typedef struct { _Atomic uint64_t v; } atomic64_t;
#define likely(x) (x)
#define WARN_ON_ONCE(x) (x)
static uint64_t atomic64_read(atomic64_t *p) { return atomic_load(&p->v); }
static uint64_t atomic64_add_return(int n, atomic64_t *p) {
 return atomic_fetch_add(&p->v, n) + n;
}
static uint64_t atomic64_cmpxchg_relaxed(atomic64_t *p, uint64_t old, uint64_t n) {
 atomic_compare_exchange_strong_explicit(&p->v, &old, n,
   memory_order_relaxed, memory_order_relaxed);
 return old;
}
'''
IDENTITY_MAIN = r'''
static atomic64_t shared;
static uint64_t ids[16];
static void *worker(void *arg) {
 uintptr_t slot = (uintptr_t)arg;
 ids[slot] = get_object_id(&shared);
 return 0;
}
int main(void) {
 pthread_t threads[16];
 for (uintptr_t i = 0; i < 16; i++)
  assert(!pthread_create(&threads[i], 0, worker, (void *)i));
 for (int i = 0; i < 16; i++) assert(!pthread_join(threads[i], 0));
 assert(ids[0]);
 for (int i = 1; i < 16; i++) assert(ids[i] == ids[0]);
 atomic64_t objects[64] = {0};
 uint64_t distinct[64];
 for (int i = 0; i < 64; i++) {
  distinct[i] = get_object_id(&objects[i]);
  assert(distinct[i] && distinct[i] != ids[0]);
  assert(get_object_id(&objects[i]) == distinct[i]);
  for (int j = 0; j < i; j++) assert(distinct[j] != distinct[i]);
 }
 /* A new lifetime at the same address must not reuse its former key ID. */
 atomic_store(&objects[0].v, 0);
 uint64_t reused = get_object_id(&objects[0]);
 assert(reused && reused != distinct[0]);
 return 0;
}
'''

PAGEWALK_PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
enum { ACTION_SUBTREE, ACTION_AGAIN, ACTION_CONTINUE };
typedef struct { bool none; } pmd_t;
typedef struct { pmd_t pmd[1]; } pud_t;
struct mm_walk;
struct mm_walk_ops {
 void (*pte_entry)(void);
 void (*install_pte)(void);
 int (*pte_hole)(unsigned long, unsigned long, int, struct mm_walk *);
 int (*pmd_entry)(pmd_t *, unsigned long, unsigned long, struct mm_walk *);
};
struct mm_walk { const struct mm_walk_ops *ops; int action; void *mm, *vma; };
#define pmd_offset(p, a) (&(p)->pmd[0])
#define pmd_addr_end(a, e) (e)
#define pmd_none(p) ((p).none)
#define pmd_present(p) (!(p).none)
#define pmd_trans_huge(p) false
#define pmd_devmap(p) false
#define pmd_leaf(p) false
#define split_huge_pmd(v, p, a) ((void)0)
#define real_depth(d) (d)
#define __pte_alloc(m, p) 0
static int walk_pte_range(pmd_t *p, unsigned long a, unsigned long e,
                         struct mm_walk *w) { return 0; }
'''
PAGEWALK_MAIN = r'''
static int callbacks;
static int clear_on_retry(pmd_t *p, unsigned long a, unsigned long e,
                          struct mm_walk *w) {
 ++callbacks;
 p->none = true;
 w->action = ACTION_AGAIN;
 return 0;
}
static void lower_entry(void) {}
int main(void) {
 pud_t pud = {0};
 struct mm_walk_ops ops = {.pte_entry = lower_entry, .pmd_entry = clear_on_retry};
 struct mm_walk w = {.ops = &ops, .action = ACTION_SUBTREE};
 assert(walk_pmd_range(&pud, 0, 1, &w) == 0);
 assert(callbacks == 1);
 assert(w.action == ACTION_SUBTREE);
 return 0;
}
'''


class Linux612115(unittest.TestCase):
    def test_complete_interval_accounting(self):
        path = ROOT / 'docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.115-commits.tsv'
        with path.open() as f:
            rows = list(csv.DictReader(f, delimiter='\t'))
        self.assertEqual([int(r['order']) for r in rows], list(range(1, 247)))
        self.assertEqual(len({r['upstream'] for r in rows}), 246)
        self.assertEqual(sum(r['action'] in ('IMPORTED', 'ADAPTED_CONTEXT') for r in rows), 231)
        self.assertEqual(rows[-1]['upstream'], '03bae623ec326c2738198739625832aab0eb120a')
        for row in rows:
            if row['action'] != 'ADAPTED_CONTEXT':
                self.assertEqual(row['upstream_patch_id'], row['local_patch_id'])

    def test_actual_futex_identity_uniqueness_and_atomic_publication(self):
        code = function('kernel/futex/core.c', 'static u64 get_object_id(')
        result = host_c(IDENTITY_PREFIX + code + IDENTITY_MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_constant_identity_negative_control(self):
        # This is a deliberately broken local test model, never kernel source.
        code = function('kernel/futex/core.c', 'static u64 get_object_id(')
        self.assertEqual(code.count('return new;'), 1)
        code = code.replace('return new;', 'return 1;', 1)
        result = host_c(IDENTITY_PREFIX + code + IDENTITY_MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_actual_pagewalk_cleared_pmd_retry(self):
        code = function('mm/pagewalk.c', 'static int walk_pmd_range(')
        result = host_c(PAGEWALK_PREFIX + code + PAGEWALK_MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_pagewalk_missing_reset_negative_control(self):
        code = function('mm/pagewalk.c', 'static int walk_pmd_range(')
        self.assertEqual(code.count('walk->action = ACTION_SUBTREE;'), 1)
        code = code.replace('walk->action = ACTION_SUBTREE;', '', 1)
        result = host_c(PAGEWALK_PREFIX + code + PAGEWALK_MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_actual_restart_error_mapping(self):
        code = function('io_uring/io_uring.h', 'static inline ssize_t io_fixup_restart_res(')
        prefix = r'''
#include <assert.h>
#include <sys/types.h>
#include <errno.h>
#define ERESTARTSYS 512
#define ERESTARTNOINTR 513
#define ERESTARTNOHAND 514
#define ERESTART_RESTARTBLOCK 516
'''
        main = r'''
int main(void) {
 const int restart[] = {ERESTARTSYS, ERESTARTNOINTR, ERESTARTNOHAND,
                        ERESTART_RESTARTBLOCK};
 for (int i = 0; i < 4; i++) assert(io_fixup_restart_res(-restart[i]) == -EINTR);
 assert(io_fixup_restart_res(0) == 0);
 assert(io_fixup_restart_res(42) == 42);
 assert(io_fixup_restart_res(-EBADF) == -EBADF);
 assert(io_fixup_restart_res(-EOPNOTSUPP) == -EOPNOTSUPP);
 assert(io_fixup_restart_res(-529) == -529);
 return 0;
}
'''
        result = host_c(prefix + code + main)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
