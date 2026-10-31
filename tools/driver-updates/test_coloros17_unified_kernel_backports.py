#!/usr/bin/env python3
"""Host-only provenance and extracted-C regressions; never access the phone.

RCU/acquire stubs test control flow, not weak-memory ordering or real FBE.
The complete production ARM64 build and physical qualification are separate.
"""
import os
from pathlib import Path
import resource
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = 'ebbbddf1b770557879dca5b847a11d4452ac61b9'
PAIRS = [
    ('c69db9e1f5cf886d4e3011313761a0f63a1d0da0', 'ca2d232b408dff5752473ab3362cb2508384175d'),
    ('18edcb9945118beb7a1b64a31a7370ecab485a79', 'd6ab26a937434d05e9fd80650def8959391de593'),
    ('68a2cd5381b8d89dc80bd16dd9cc9ca2d5c25e7f', '1aed620beb4ea049685a744f1e85c594c87e0e12'),
    ('0800c92ca5d1983df6843a5435299978901d1d02', '6f2ab8c4e49a01bdfbacbee636a8dc6ae35b6cf1'),
    ('c9664f780f861dd9f11f120c7a03ed0993671ebb', '46a4eacf9306b20cc06c1e4e80848227c560979c'),
    ('984a178d54515779a1bee100b92a6cee8777c830', '4cfb1bc470c56b19c252a1ea32a1cd906c2af3b4'),
    ('3c3cfd9e262afc9558c38f8cec57f09d10988546', '69471657246ed0c8529d88033796e369f5be789a'),
    ('6a36e2a1a017c51f8a8ad2a094566bad97c46a9b', '85696f22fc86061807093d4fc3e94f0c88cff2f6'),
    ('5787887eba0f0979a8c86d03a8e891395d5c24ad', '5c52e5b676108b51a90b851f69f930ca15fd7b67'),
    ('f0f982d193227d13e5a1c9e6bc6c8c6f7e689503', '79c418a771a8717edb4a3ed2b814fb9e89ec4a7c'),
]


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def function(path, signature):
    text = (ROOT / 'kernel_platform/common' / path).read_text()
    start = text.index(signature)
    body = text.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def host_c(text):
    compiler = shutil.which('clang')
    if not compiler:
        raise unittest.SkipTest('Host Clang unavailable; no runtime result inferred')
    with tempfile.TemporaryDirectory(prefix='coloros17-unified-test-') as temp:
        p = Path(temp)
        (p / 'case.c').write_text(text)
        compiled = subprocess.run([compiler, '-std=gnu11', '-O1', '-g', '-Wall', '-Werror',
                                   '-fsanitize=address,undefined', str(p / 'case.c'),
                                   '-o', str(p / 'case')], capture_output=True, text=True)
        if compiled.returncode:
            raise AssertionError(compiled.stderr)
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:abort_on_error=0',
                   UBSAN_OPTIONS='halt_on_error=1')
        return subprocess.run([str(p / 'case')], capture_output=True, text=True,
                              timeout=10, env=env,
                              preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)))


FIB_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
typedef uint64_t u64;
#define RTNH_F_DEAD 1
#define RTNH_F_LINKDOWN 2
struct fib_nh { unsigned fib_nh_flags; int fib_nh_weight, fib_nh_dev, fib_nh_upper_bound; };
struct fib_info { int n; struct fib_nh nh[2]; };
static int ignore_calls, race_mode, divides;
static int fib_info_num_path(struct fib_info *fi) { return fi->n; }
static int ip_ignore_linkdown(int dev) { (void)dev; return race_mode && ++ignore_calls <= 2; }
#define for_nexthops(fi) for (int i=0; i<(fi)->n; i++) for(struct fib_nh *nh=&(fi)->nh[i];nh;nh=0)
#define change_nexthops(fi) for (int i=0; i<(fi)->n; i++) for(struct fib_nh *nexthop_nh=&(fi)->nh[i];nexthop_nh;nexthop_nh=0)
#define endfor_nexthops(fi)
#define atomic_set(p, v) (*(p)=(v))
static u64 divide(u64 n, int d) { ++divides; assert(d); return (n+d/2)/d; }
#define DIV_ROUND_CLOSEST_ULL(n,d) divide(n,d)
'''
FIB_MAIN = r'''
int main(void) {
 struct fib_info fi={.n=2,.nh={{.fib_nh_weight=1},{.fib_nh_weight=1}}};
 fib_rebalance(&fi);
 assert(fi.nh[0].fib_nh_upper_bound==0x3fffffff);
 assert(fi.nh[1].fib_nh_upper_bound==0x7fffffff);
 fi.nh[0].fib_nh_flags=RTNH_F_LINKDOWN;
 fi.nh[1].fib_nh_flags=RTNH_F_LINKDOWN;
 race_mode=1; ignore_calls=0; divides=0;
 fib_rebalance(&fi);
 assert(!divides);
 assert(fi.nh[0].fib_nh_upper_bound==-1 && fi.nh[1].fib_nh_upper_bound==-1);
 return 0;
}
'''


class UnifiedBackports(unittest.TestCase):
    def test_individual_authorship_order_and_exact_relative_patches(self):
        previous = BASE
        for upstream, local in PAIRS:
            with self.subTest(upstream=upstream):
                self.assertEqual(git('show', '-s', '--format=%P', local).decode().strip(), previous)
                self.assertEqual(git('show', '-s', '--format=%an%x00%ae%x00%aI%x00%s', local),
                                 git('show', '-s', '--format=%an%x00%ae%x00%aI%x00%s', upstream))
                self.assertIn(upstream.encode(), git('show', '-s', '--format=%B', local))
                original = git('diff', upstream + '^', upstream)
                imported = git('diff', local + '^', local).replace(b'a/kernel_platform/common/', b'a/').replace(b'b/kernel_platform/common/', b'b/')
                def patch_id(data):
                    return subprocess.check_output(['git', 'patch-id', '--stable'], input=data).split()[0]
                self.assertEqual(patch_id(original), patch_id(imported))
                previous = local

    def test_selected_group_does_not_change_configuration_or_reference(self):
        paths = git('diff', '--name-only', BASE, PAIRS[-1][1]).decode().splitlines()
        self.assertEqual(len(paths), 8)
        self.assertTrue(all(p.endswith('.c') and p.startswith('kernel_platform/common/') for p in paths))

    def test_actual_fib_rebalance_normal_and_racing_linkdown(self):
        code = function('net/ipv4/fib_semantics.c', 'static void fib_rebalance(')
        result = host_c(FIB_PREFIX + code + FIB_MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_fib_missing_guard_negative_control(self):
        code = function('net/ipv4/fib_semantics.c', 'static void fib_rebalance(')
        self.assertIn('!total || ', code)
        result = host_c(FIB_PREFIX + code.replace('!total || ', '', 1) + FIB_MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_fscrypt_acquire_on_matching_key_only(self):
        signature = 'static struct fscrypt_prepared_key *\nfscrypt_find_mode_key('
        code = function('fs/crypto/keysetup.c', signature)
        prefix = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned char u8;
struct fscrypt_prepared_key { int ready; };
struct fscrypt_mode_key { struct fscrypt_mode_key *next; u8 hkdf_context, mode_num, data_unit_bits; struct fscrypt_prepared_key key; };
struct fscrypt_master_key { struct fscrypt_mode_key *mk_mode_keys; };
struct fscrypt_inode_info { u8 ci_data_unit_bits; };
static int acquires;
static void read_guard(void) {}
#define guard(kind) read_guard
#define list_for_each_entry_rcu(n,h,l) for(n=*(h);n;n=n->next)
static int fscrypt_is_key_prepared(struct fscrypt_prepared_key *k, const struct fscrypt_inode_info *ci) { (void)ci; return k->ready; }
#define smp_acquire__after_ctrl_dep() (++acquires)
'''
        main = r'''
int main(void) {
 struct fscrypt_mode_key n={.hkdf_context=1,.mode_num=2,.data_unit_bits=12,.key={.ready=1}};
 struct fscrypt_master_key mk={.mk_mode_keys=&n};
 struct fscrypt_inode_info ci={.ci_data_unit_bits=12};
 assert(fscrypt_find_mode_key(&mk,1,2,&ci)==&n.key && acquires==1);
 assert(fscrypt_find_mode_key(&mk,3,2,&ci)==NULL && acquires==1);
 n.key.ready=0;
 assert(fscrypt_find_mode_key(&mk,1,2,&ci)==NULL && acquires==1);
 return 0;
}
'''
        result = host_c(prefix + code + main)
        self.assertEqual(result.returncode, 0, result.stderr)
        negative = host_c(prefix + code.replace('smp_acquire__after_ctrl_dep();', ';') + main)
        self.assertNotEqual(negative.returncode, 0)


if __name__ == '__main__':
    unittest.main()
