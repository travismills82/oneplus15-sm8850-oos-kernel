#!/usr/bin/env python3
"""Host contracts for GPU queue/API preparation, not device qualification."""
from pathlib import Path
import subprocess
import unittest
from test_coloros17_unified_kernel_backports import host_c

ROOT = Path(__file__).resolve().parents[2]


def body(file, signature):
    text = (ROOT / 'vendor/qcom/opensource/graphics-kernel' / file).read_text()
    start = text.index(signature)
    pos = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}')
        pos += 1
    return text[start:pos]


PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
struct work_struct { void (*fn)(struct work_struct *); };
struct workqueue_struct { int id; };
struct kgsl_device { struct { bool wake_on_touch; } pwrctrl; int state; };
struct adreno_device { struct work_struct input_work; };
struct kgsl_sharedmem_bind_op { int ref; struct work_struct work; };
static struct workqueue_struct stock_queue={1}, percpu_queue={2};
struct workqueue_struct *system_wq=&stock_queue, *system_percpu_wq=&percpu_queue;
static struct adreno_device adreno;
static struct work_struct *expected_work;
static struct kgsl_sharedmem_bind_op *binding;
static int calls;
#define KGSL_STATE_SLUMBER 3
#define ADRENO_DEVICE(d) (&adreno)
#define INIT_WORK(w,f) ((w)->fn=(f))
static void kref_get(int *ref) { ++*ref; }
static void kgsl_sharedmem_bind_worker(struct work_struct *work) { (void)work; }
static int queue_work(struct workqueue_struct *queue, struct work_struct *work) {
 assert(queue==system_wq && work==expected_work);
 if (binding) assert(binding->ref==1 && work->fn==kgsl_sharedmem_bind_worker);
 ++calls; return 1;
}
'''
MAIN = r'''
int main(void) {
 struct kgsl_device device={.state=KGSL_STATE_SLUMBER};
 expected_work=&adreno.input_work;
 adreno_touch_wake(&device); assert(calls==1);
 device.pwrctrl.wake_on_touch=true;
 adreno_touch_wake(&device); assert(calls==1);
 device.pwrctrl.wake_on_touch=false; device.state=0;
 adreno_touch_wake(&device); assert(calls==1);
 struct kgsl_sharedmem_bind_op op={0};
 binding=&op; expected_work=&op.work;
 kgsl_sharedmem_bind_ranges(&op);
 assert(calls==2 && op.ref==1);
 return 0;
}
'''


class StockGPUProvider(unittest.TestCase):
    def source(self):
        return body('adreno.c', 'void adreno_touch_wake(') + '\n' + body('kgsl_vbo.c', 'void kgsl_sharedmem_bind_ranges(')

    def test_actual_queue_bodies_preserve_guards_work_and_reference(self):
        result = host_c(PREFIX + self.source() + MAIN)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_wrong_provider_negative_control(self):
        code = self.source()
        self.assertEqual(code.count('queue_work(system_wq,'), 2)
        result = host_c(PREFIX + code.replace('queue_work(system_wq,', 'queue_work(system_percpu_wq,') + MAIN)
        self.assertNotEqual(result.returncode, 0)

    def test_synx_exact_reviewed_upstream_header_and_wrapper(self):
        for relative, expected in (
            ('msm/synx/synx_api.h', '1410dcca04cc1c8558f9302392d443b948be6bec'),
            ('msm/synx/synx_compat.c', '2e9f320a7315f13521fb746d43c5226c1123e874'),
        ):
            path = ROOT / 'vendor/qcom/opensource/synx-kernel' / relative
            actual = subprocess.check_output(['git', 'hash-object', str(path)], text=True).strip()
            self.assertEqual(actual, expected)


if __name__ == '__main__':
    unittest.main()
