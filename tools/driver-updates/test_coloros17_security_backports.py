#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host tests of extracted driver code, not kernel/hardware qualification.

The SG-tail fixture executes the actual guarded walk and rejection statements,
not the entire hash/DMA path. Kernel locking, IRQ delivery, workqueue teardown,
GPR ordering and firmware behavior remain separate source/runtime gates.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SEC = ROOT / 'vendor/qcom/opensource/securemsm-kernel/crypto-qti'
DSP = ROOT / 'vendor/qcom/opensource/dsp-kernel/dsp'
AUDIO = ROOT / 'vendor/qcom/opensource/audio-kernel/asoc/codecs'


def function(text, name):
    match = re.search(r'^\w[^\n]*\b' + re.escape(name) + r'\([^;]*?\)\s*\{', text, re.M)
    if not match:
        raise ValueError('function not found: ' + name)
    start = text.index('{', match.start())
    depth = 0
    for end in range(start, len(text)):
        if text[end] == '{':
            depth += 1
        elif text[end] == '}':
            depth -= 1
            if depth == 0:
                return text[match.start():end + 1]
    raise ValueError('unterminated function: ' + name)


def run_c(code, *, should_pass=True):
    compiler = shutil.which('clang')
    if not compiler:
        raise unittest.SkipTest('clang unavailable; sanitizer test not performed')
    with tempfile.TemporaryDirectory(prefix='coloros17-security-test-') as directory:
        source, binary = Path(directory) / 'test.c', Path(directory) / 'test'
        source.write_text(code)
        command = [compiler, '-std=gnu11', '-O1', '-g', '-fsanitize=address,undefined',
                   '-fno-omit-frame-pointer', str(source), '-o', str(binary)]
        build = subprocess.run(command, text=True, capture_output=True)
        if build.returncode:
            raise AssertionError(build.stdout + build.stderr)
        environment = os.environ.copy()
        environment['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
        result = subprocess.run([str(binary)], text=True, capture_output=True,
                                env=environment, timeout=15)
        if should_pass and result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        if not should_pass and not result.returncode:
            raise AssertionError('mutation was not detected')


class SecurityBackports(unittest.TestCase):
    def test_actual_qce_count_and_exhaustion_mutation(self):
        body = function((SEC / 'qce50.c').read_text(), 'count_sg')
        prefix = '''
#include <assert.h>
#include <stddef.h>
struct scatterlist { unsigned length; struct scatterlist *next; };
static struct scatterlist *sg_next(struct scatterlist *s) { return s->next; }
'''
        tests = '''
int main(void) {
 struct scatterlist b = {16, NULL}, a = {16, &b};
 assert(count_sg(NULL, 0) == 0);
 assert(count_sg(NULL, 1) == 0);
 assert(count_sg(&a, 1) == 1);
 assert(count_sg(&a, 16) == 1);
 assert(count_sg(&a, 17) == 2);
 assert(count_sg(&a, 32) == 2);
 /* Exhaustion returns a partial count, NOT proof of a DMA-safe request. */
 assert(count_sg(&a, 33) == 2);
 return 0;
}
'''
        self.assertIn('nbytes > 0 && sg;', body)
        run_c(prefix + body + tests)
        run_c(prefix + body.replace('nbytes > 0 && sg;', 'nbytes > 0;') + tests,
              should_pass=False)

    def test_actual_sha_tail_walk_and_rejection_mutations(self):
        body = function((SEC / 'qcrypto.c').read_text(), '_sha_update')
        match = re.search(r'\twhile \(len < nbytes && sg_last\) \{.*?\n\t\}', body, re.S)
        self.assertIsNotNone(match)
        guard = '\t\t\tif (!sg_last)\n\t\t\t\treturn -EINVAL;'
        self.assertIn(guard, body)
        self.assertIn('\t\tif (!sg_last)\n\t\t\treturn -EINVAL;', body)
        prefix = '''
#include <assert.h>
#include <errno.h>
#include <stddef.h>
struct scatterlist { unsigned length; struct scatterlist *next; int end; };
static struct scatterlist *sg_next(struct scatterlist *s) { return s->next; }
static void sg_mark_end(struct scatterlist *s) { s->end = 1; }
static int tail(struct scatterlist *sg_last, unsigned len, unsigned nbytes) {
'''
        suffix = '''
 sg_mark_end(sg_last);
 return 0;
}
int main(void) {
 struct scatterlist b = {16, NULL, 0}, a = {16, &b, 0};
 assert(tail(&a, 0, 8) == 0 && a.end);
 assert(tail(&a, 0, 24) == 0 && b.end);
 assert(tail(&a, 0, 40) == -EINVAL);
 assert(tail(NULL, 0, 8) == -EINVAL);
 assert(tail(NULL, 8, 8) == -EINVAL);
 return 0;
}
'''
        walk = match.group(0)
        run_c(prefix + walk + '\n' + guard + suffix)
        run_c(prefix + walk.replace(' && sg_last', '') + '\n' + guard + suffix,
              should_pass=False)
        run_c(prefix + walk + suffix, should_pass=False)

    def test_actual_fastrpc_registration_states_and_unwind(self):
        body = function((DSP / 'fastrpc.c').read_text(), 'fastrpc_driver_register')
        scaffold = '''
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#define DSP_CREATE_COMPLETE 2
#define DSP_EXIT_START 3
#define pr_err(...) ((void)0)
#define pr_info(...) ((void)0)
#define dev_warn(...) ((void)0)
struct fastrpc_device { int marker; };
struct device { int marker; };
struct list_head { struct fastrpc_user *first; };
struct fastrpc_user { int tgid_frpc, state; struct fastrpc_device *device; struct list_head fastrpc_drivers; };
struct fastrpc_channel_ctx { int lock; void *dev; struct list_head users; };
struct fastrpc_domain { struct fastrpc_channel_ctx *cctx; };
struct fastrpc_driver { struct device *device; int handle; struct list_head hn; int (*probe)(struct fastrpc_device *); };
static struct { struct fastrpc_domain *domain; int fastrpc_domains_table; } g_frpc;
static int gets, puts, added, probes, get_error;
#define hash_for_each(table, i, domain, node) for ((i)=0; (i)<1 && ((domain)=g_frpc.domain); ++(i))
#define list_for_each_entry(user, head, member) for ((user)=(head)->first; (user); (user)=NULL)
#define spin_lock_irqsave(lock, flags) do { ++*(lock); (flags)=0; } while (0)
#define spin_unlock_irqrestore(lock, flags) do { assert(*(lock)==1); --*(lock); (void)(flags); } while (0)
#define atomic_read(p) (*(p))
static int fastrpc_file_get(struct fastrpc_user *u) { (void)u; ++gets; return get_error; }
static void fastrpc_file_put(struct fastrpc_user *u, bool work) { (void)u; assert(!work); ++puts; }
static void list_add_tail(struct list_head *h, struct list_head *l) { (void)h; (void)l; ++added; }
static int probe(struct fastrpc_device *d) { assert(d && d->marker==17); ++probes; return 0; }
'''
        tests = '''
int main(void) {
 struct fastrpc_device device = {17};
 struct fastrpc_user user = {.tgid_frpc=42,.device=&device};
 struct fastrpc_channel_ctx channel = {.users={&user}};
 struct fastrpc_domain domain = {&channel};
 struct fastrpc_driver driver = {.handle=42,.probe=probe};
 g_frpc.domain=&domain;
 assert(fastrpc_driver_register(NULL)==-EINVAL);
 for (int state=0;state<=4;state++) {
  user.state=state; gets=puts=added=probes=0;
  int r=fastrpc_driver_register(&driver);
  assert(gets==1 && puts==1 && channel.lock==0);
  if(state==2) { assert(r==0 && added==1 && probes==1 && driver.device==(struct device *)&device); }
  else { assert(r==-ESRCH && !added && !probes && driver.device==NULL); }
 }
 user.state=2; get_error=-ENOENT; gets=puts=added=probes=0;
 assert(fastrpc_driver_register(&driver)==-ESRCH);
 assert(gets==1 && puts==0 && !added && !probes && channel.lock==0);
 return 0;
}
'''
        run_c(scaffold + body + tests)
        run_c(scaffold + body.replace('dsp_state != DSP_CREATE_COMPLETE',
                                     'dsp_state >= DSP_EXIT_START') + tests,
              should_pass=False)

    def test_qce_irq_and_qcrypto_queue_source_contracts(self):
        qce = (SEC / 'qce50.c').read_text()
        irq = function(qce, 'qce_core_irq_init')
        self.assertIn('request_irq(pce_dev->core_irq, qce_core_irq_handler,', irq)
        self.assertIn('IRQF_TRIGGER_RISING, "qcedev", pce_dev)', irq)
        self.assertNotIn('enable_irq(', irq)
        self.assertNotIn('IRQF_NO_AUTOEN', irq)
        self.assertNotIn('IRQ_NOAUTOEN', qce)
        qcrypto = (SEC / 'qcrypto.c').read_text()
        self.assertIn('queue_work(system_wq, &pengine->bw_reaper_ws);',
                      function(qcrypto, 'qcrypto_bw_reaper_timer_callback'))
        self.assertIn('queue_work(system_wq, &pengine->bw_allocate_ws);',
                      function(qcrypto, 'qcrypto_ce_bw_allocate_req'))
        remove = function(qcrypto, '_qcrypto_remove_engine')
        for work in ('bw_reaper_ws', 'bw_allocate_ws'):
            self.assertIn('cancel_work_sync(&pengine->' + work + ');', remove)
            self.assertIn('INIT_WORK(&pengine->' + work + ',', qcrypto)
        self.assertIn('del_timer_sync(&pengine->bw_reaper_timer);', remove)

    def test_actual_audio_timeout_cleanup_and_mutation(self):
        body = function((AUDIO / 'audio-ext-clk-up.c').read_text(), 'audio_ext_clk_prepare')
        scaffold = '''
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#define CONFIG_AUDIO_PRM 1
#define HZ 100
#define DEFINE_RATELIMIT_STATE(name,a,b) int name
#define __ratelimit(p) ((void)(p),0)
#define pr_debug(...) ((void)0)
#define pr_err_ratelimited(...) ((void)0)
#define pr_err(...) ((void)0)
#define AUDIO_EXT_CLK_LPASS 2
#define AUDIO_EXT_CLK_LPASS_MAX 22
struct clk_hw {int dummy;};
struct clk_cfg {unsigned clk_id;};
struct pinctrl_info {void *pinctrl,*active,*base;};
struct audio_ext_clk_priv {int clk_src; unsigned enable; struct clk_cfg prm_clk_cfg; struct {struct pinctrl_info pnctrl_info;} audio_clk;};
static struct audio_ext_clk_priv priv;
static int enables, disables, fail, pin_calls, io_calls;
static struct audio_ext_clk_priv *to_audio_clk(struct clk_hw *hw) { (void)hw;return &priv; }
static int audio_prm_set_lpass_clk_cfg(struct clk_cfg *c,int enable) { (void)c;if(enable){++enables;return fail;} ++disables;return -EIO; }
static int pinctrl_select_state(void *p,void *s) { (void)p;(void)s;++pin_calls;return 0; }
static void iowrite32(int v,void *p) { (void)v;(void)p;++io_calls; }
'''
        tests = '''
int main(void) {
 struct clk_hw hw={0};
 int errors[]={-ETIMEDOUT,-EINVAL,-ENODEV,0};
 for(unsigned i=0;i<sizeof(errors)/sizeof(errors[0]);i++) {
  priv=(struct audio_ext_clk_priv){.clk_src=AUDIO_EXT_CLK_LPASS};
  enables=disables=pin_calls=io_calls=0;fail=errors[i];
  int ret=audio_ext_clk_prepare(&hw);
  assert(enables==1 && ret==fail);
  assert(disables==(fail==-ETIMEDOUT));
  assert(priv.enable==(fail==0));
 }
 priv=(struct audio_ext_clk_priv){.clk_src=AUDIO_EXT_CLK_LPASS,.enable=1};
 enables=disables=0;fail=-ETIMEDOUT;
 assert(audio_ext_clk_prepare(&hw)==0 && !enables && !disables);
 priv=(struct audio_ext_clk_priv){.clk_src=0};
 assert(audio_ext_clk_prepare(&hw)==0 && !enables && !disables);
 return 0;
}
'''
        run_c(scaffold + body + tests)
        run_c(scaffold + body.replace('if (ret == -ETIMEDOUT)',
                                     'if (ret == -EINVAL)') + tests,
              should_pass=False)
        no_prm = '''
int main(void) {
 struct clk_hw hw={0};
 priv=(struct audio_ext_clk_priv){.clk_src=AUDIO_EXT_CLK_LPASS};
 assert(audio_ext_clk_prepare(&hw)==-EPERM);
 assert(!enables && !disables && !priv.enable);
 return 0;
}
'''
        run_c(scaffold.replace('#define CONFIG_AUDIO_PRM 1', '') + body + no_prm)


if __name__ == '__main__':
    unittest.main(verbosity=2)
