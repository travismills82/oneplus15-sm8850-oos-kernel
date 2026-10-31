#!/usr/bin/env python3
"""Extracted key-index tests. Host stubs are not ARM64/FBE qualification."""
import re
import subprocess
import unittest

from test_coloros17_unified_kernel_backports import ROOT, function, host_c

COMMON = ROOT / 'kernel_platform/common'
RAW = '46fb47434d1067af6076f6491fc046ba6bf912ba'
QUALIFIED = '79c418a771a8717edb4a3ed2b814fb9e89ec4a7c'


def old_source(revision, path):
    return subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                    revision + ':kernel_platform/common/' + path], text=True)


def extract(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PREFIX = r'''
#pragma GCC diagnostic ignored "-Wpointer-sign"
/* Each case extracts a subset of the shared host-only infrastructure. */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
typedef uint64_t u64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef int refcount_t;
struct rcu_head { void *next, *func; };
typedef struct { _Atomic u64 value; } atomic64_t;
static u64 atomic64_read(atomic64_t *p) { return atomic_load(&p->value); }
static u64 atomic64_inc_return(atomic64_t *p) { return atomic_fetch_add(&p->value, 1) + 1; }
static bool atomic64_try_cmpxchg_relaxed(atomic64_t *p, u64 *old, u64 n) {
 return atomic_compare_exchange_strong_explicit(&p->value, old, n,
                                               memory_order_relaxed, memory_order_relaxed);
}
#define likely(x) (x)
#define WARN_ON_ONCE(x) (x)
#define REFCOUNT_INIT(x) (x)
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define ASSOC_ARRAY_LEVEL_STEP 4
#define ASSOC_ARRAY_FAN_MASK 15
#define ASSOC_ARRAY_KEY_CHUNK_SIZE 64
#define KEY_TYPE_NET_DOMAIN 1
#define __ffs(x) __builtin_ctzl(x)
#define __ffs64(x) __builtin_ctzll(x)
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif
#define container_of(p,t,m) ((t *)((char *)(p) - offsetof(t,m)))
#define container_of_const(p,t,m) ((const t *)((const char *)(p) - offsetof(t,m)))
struct key_type { atomic64_t index_id; unsigned flags; };
'''
ENVIRONMENT = r'''
/* Minimal key fixture: actual ARM64 key geometry is checked from vmlinux. */
struct key { struct keyring_index_key index_key; };
struct net { struct key_tag *key_domain; };
struct nsproxy { struct net *net_ns; };
struct task { struct nsproxy *nsproxy; };
static struct net net;
static struct nsproxy ns = {&net};
static struct task task = {&ns};
static struct task *current = &task;
static struct key_type key_type_keyring;
static const struct key *keyring_ptr_to_key(const void *p) { return p; }
typedef struct { u64 value[2]; } siphash_key_t;
static u64 observed_type, observed_domain;
#define get_random_once(p,n) memset(p,0x5a,n)
/* Non-cryptographic fixture: verify inputs/control flow, not SipHash strength. */
static u64 siphash(const char *p, size_t n, const siphash_key_t *key) {
 u64 v=0xcbf29ce484222325ULL;
 for (size_t i=0;i<n;++i) v=(v^(unsigned char)p[i])*1099511628211ULL;
 return v;
}
static u64 siphash_3u64(u64 a,u64 b,u64 c,const siphash_key_t *key) {
 observed_type=a; observed_domain=b;
 return a*0x9e3779b97f4a7c15ULL ^ b*0xbf58476d1ce4e5b9ULL ^ c;
}
#define __read_mostly
'''


def fixture():
    header = (COMMON / 'include/linux/key.h').read_text()
    tag = extract(header, 'struct key_tag {') + ';\n'
    index = extract(header, 'struct keyring_index_key {') + ';\n'
    tag_alloc = function('include/linux/key_internal.h', 'struct key_tag_alloc {') + ';\n'
    allocation = function('security/keys/internal.h', 'struct key_alloc_state {') + ';\n'
    private = function('security/keys/internal.h', 'struct keyring_index_key_private {') + ';\n'
    getters = function('security/keys/internal.h', 'static inline struct key_alloc_state *key_alloc_state(')
    getters += function('security/keys/internal.h',
                        'static inline struct keyring_index_key_private\nkey_get_index_key(')
    return PREFIX + tag + index + tag_alloc + ENVIRONMENT + allocation + private + getters


SIGNATURES = ['static u64 key_index_id(', 'static void hash_key_type_and_desc(',
              'void key_set_index_key(', 'static unsigned long keyring_get_key_chunk(',
              'static unsigned long keyring_get_object_key_chunk(',
              'static bool keyring_compare_object(', 'static int keyring_diff_objects(']


def algorithm():
    return '\n'.join(function('security/keys/keyring.c', s) for s in SIGNATURES)


MAIN = r'''
static atomic64_t shared;
static u64 ids[16];
static void *worker(void *p) { ids[(uintptr_t)p]=key_index_id(&shared); return NULL; }
int main(void) {
 _Static_assert(sizeof(struct keyring_index_key)==40,"public index");
 _Static_assert(sizeof(struct keyring_index_key_private)==56,"private index");
 _Static_assert(sizeof(struct key_tag)==24,"public tag");
 _Static_assert(sizeof(struct key_tag_alloc)==32,"private tag");
 _Static_assert(offsetof(struct key_alloc_state,key)==0,"key allocation base");
 _Static_assert(offsetof(struct key_tag_alloc,tag)==0,"tag allocation base");
 pthread_t threads[16];
 for(uintptr_t i=0;i<16;++i) assert(!pthread_create(&threads[i],NULL,worker,(void *)i));
 for(int i=0;i<16;++i) assert(!pthread_join(threads[i],NULL));
 assert(ids[0]);
 for(int i=1;i<16;++i) assert(ids[i]==ids[0]);
 atomic_store(&shared.value,0);
 assert(key_index_id(&shared)!=ids[0]);
 struct key_type type={0};
 struct key_tag_alloc domain={.tag.usage=1};
 struct keyring_index_key_private index={.key={.type=&type,.domain_tag=&domain.tag,
   .description="shared-description",.desc_len=18}};
 key_set_index_key(&index);
 assert(index.type_id && index.domain_id && index.type_id!=index.domain_id);
 assert(observed_type==index.type_id && observed_domain==index.domain_id);
 u64 type_id=index.type_id,domain_id=index.domain_id,hash=index.key.hash;
 /* An old owned key must not dereference the retired type/tag for its IDs. */
 index.key.type=(void *)1;index.key.domain_tag=(void *)1;
 struct key_alloc_state object={.key={.index_key=index.key},
   .type_id=type_id,.domain_id=domain_id};
 struct keyring_index_key_private copied=key_get_index_key(&object.key);
 key_set_index_key(&copied);
 assert(copied.key.hash==hash && copied.type_id==type_id && copied.domain_id==domain_id);
 assert(keyring_get_key_chunk(&copied,128)==type_id);
 assert(keyring_get_key_chunk(&copied,192)==domain_id);
 assert(keyring_get_object_key_chunk(&object.key,128)==type_id);
 assert(keyring_compare_object(&object.key,&copied));
 assert(keyring_diff_objects(&object.key,&copied)==-1);
 copied.type_id^=2;
 assert(keyring_diff_objects(&object.key,&copied)==129);
 copied=key_get_index_key(&object.key);copied.domain_id^=4;
 assert(keyring_diff_objects(&object.key,&copied)==194);
 /* Exercise both automatic tag selection paths. */
 struct keyring_index_key_private selected={.key={.type=&type,.description="x",.desc_len=1}};
 key_set_index_key(&selected); assert(selected.domain_id);
 type.flags=KEY_TYPE_NET_DOMAIN;net.key_domain=&domain.tag;
 selected=(struct keyring_index_key_private){.key={.type=&type,.description="x",.desc_len=1}};
 key_set_index_key(&selected);assert(selected.key.domain_tag==&domain.tag);
 assert(selected.domain_id==domain_id);
 return 0;
}
'''


class KeyringKabi(unittest.TestCase):
    def test_actual_id_hash_chunk_copy_and_diff_algorithms(self):
        result = host_c(fixture() + algorithm() + MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_address_input_negative_control(self):
        code = algorithm()
        old = 'siphash_3u64(index_key->type_id, index_key->domain_id, acc,'
        self.assertEqual(code.count(old), 1)
        code = code.replace(old, 'siphash_3u64((u64)(uintptr_t)index_key->key.type, index_key->domain_id, acc,')
        self.assertNotEqual(host_c(fixture() + code + MAIN).returncode, 0)

    def test_dropped_cached_ids_negative_control(self):
        code = fixture().replace('.type_id = state->type_id,', '.type_id = 0,')
        self.assertNotEqual(host_c(code + algorithm() + MAIN).returncode, 0)

    def test_hash_and_id_security_algorithms_unchanged(self):
        raw = old_source(RAW, 'security/keys/keyring.c')
        for signature in [SIGNATURES[0], SIGNATURES[1], SIGNATURES[3]]:
            current = function('security/keys/keyring.c', signature)
            current = current.replace('struct keyring_index_key_private', 'struct keyring_index_key')
            current = current.replace('->key.', '->')
            self.assertEqual(current, extract(raw, signature))

    def test_public_key_tag_and_index_declarations_restored(self):
        old = old_source(QUALIFIED, 'include/linux/key.h')
        new = (COMMON / 'include/linux/key.h').read_text()
        for name in ['key_tag', 'keyring_index_key', 'key']:
            signature = 'struct ' + name + ' {'
            self.assertEqual(extract(old, signature), extract(new, signature))
        header = (COMMON / 'include/linux/key-type.h').read_text()
        self.assertIn('ANDROID_KABI_USE(1, atomic64_t index_id);', header)
        self.assertEqual(header.count('atomic64_t index_id'), 1)

    def test_all_private_assoc_array_crossings_and_ownership(self):
        for path in (COMMON / 'security/keys').glob('*.[ch]'):
            text = path.read_text()
            self.assertNotRegex(text, r'__key_link_begin\([^;]*&key->index_key')
            self.assertNotRegex(text, r'\.index_key\s*=\s*(?:key|A)->index_key')
        for signature in ['static unsigned long keyring_get_key_chunk(',
                          'static bool keyring_compare_object(',
                          'key_ref_t find_key_to_update(', 'int __key_link_begin(']:
            self.assertIn('struct keyring_index_key_private', function('security/keys/keyring.c', signature))
        unlink = function('security/keys/keyring.c', 'static int __key_unlink_begin(')
        self.assertIn('index_key = key_get_index_key(key);', unlink)
        self.assertNotIn('&key->index_key', unlink)
        allocation = function('security/keys/key.c', 'struct key *key_alloc(')
        self.assertIn('key = &allocation->key;', allocation)
        self.assertIn('allocation->type_id = index_key.type_id;', allocation)
        self.assertIn('allocation->domain_id = index_key.domain_id;', allocation)
        init = function('security/keys/key.c', 'void __init key_init(')
        self.assertIn('sizeof(struct key_alloc_state)', init)
        namespace = (COMMON / 'net/core/net_namespace.c').read_text()
        self.assertIn('domain = kzalloc(sizeof(*domain), GFP_KERNEL);', namespace)
        self.assertIn('net->key_domain = &domain->tag;', namespace)

    def test_gc_release_order_unchanged_and_full_state_wiped(self):
        current = function('security/keys/gc.c', 'static noinline void key_gc_unused_keys(')
        current = current.replace('memzero_explicit(key_alloc_state(key), sizeof(struct key_alloc_state));',
                                  'memzero_explicit(key, sizeof(*key));')
        raw = old_source(RAW, 'security/keys/gc.c')
        self.assertEqual(current, extract(raw, 'static noinline void key_gc_unused_keys('))
        gc = (COMMON / 'security/keys/gc.c').read_text()
        wipe = re.search(r'\bmemzero_explicit\([^;]+;', gc)[0]
        scaffold = fixture() + '\n#define memzero_explicit(p,n) memset(p,0,n)\n'
        main = '''
int main(void) {
 struct key_alloc_state state;
 memset(&state,0xa5,sizeof(state));
 struct key *key=&state.key;
 WIPE
 for(size_t i=0;i<sizeof(state);++i) assert(!((unsigned char *)&state)[i]);
 return 0;
}
'''
        self.assertEqual(host_c(scaffold + main.replace('WIPE', wipe)).returncode, 0)
        mutant = 'memzero_explicit(key, sizeof(*key));'
        self.assertNotEqual(host_c(scaffold + main.replace('WIPE', mutant)).returncode, 0)

    def test_owned_index_forwarding_uses_current_key(self):
        helper = function('security/keys/internal.h', 'static inline int __key_link_begin_key(')
        code = fixture() + r'''
struct assoc_array_edit { int unused; };
static u64 expected;
static int __key_link_begin(struct key *ring,const struct keyring_index_key_private *index,
                            struct assoc_array_edit **edit) {
 assert(index->type_id==expected && index->domain_id==expected+1);return 0;
}
'''
        main = r'''
int main(void) {
 struct key_alloc_state a={.type_id=10,.domain_id=11},b={.type_id=20,.domain_id=21};
 struct assoc_array_edit *edit=NULL;
 expected=10;assert(!__key_link_begin_key(NULL,&a.key,&edit));
 expected=20;assert(!__key_link_begin_key(NULL,&b.key,&edit));
 return 0;
}
'''
        self.assertEqual(host_c(code + helper + main).returncode, 0)

    def test_actual_tag_reference_and_deferred_allocation_base(self):
        code = fixture() + r'''
static void *queued;
static bool refcount_dec_and_test(refcount_t *p) { assert(*p>0);return !--*p; }
#define kfree_rcu(p,m) do { \
 assert(offsetof(struct key_tag,m)==0);assert(!queued);queued=(p); \
} while(0)
'''
        code += function('security/keys/keyring.c', 'bool key_put_tag(')
        main = r'''
int main(void) {
 struct key_tag_alloc *allocation=calloc(1,sizeof(*allocation));
 assert(allocation);allocation->tag.usage=2;
 atomic_store(&allocation->index_id.value,0x123456789ULL);
 assert(!key_put_tag(&allocation->tag) && !queued);
 assert(key_put_tag(&allocation->tag) && queued==allocation);
 assert(atomic_load(&allocation->index_id.value)==0x123456789ULL);
 /* The test grace period ends here; free the original true allocation base. */
 free(queued);
 return 0;
}
'''
        self.assertEqual(host_c(code + main).returncode, 0)


if __name__ == '__main__':
    unittest.main()
