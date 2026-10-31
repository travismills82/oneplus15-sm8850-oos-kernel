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


if __name__ == '__main__':
    unittest.main()
