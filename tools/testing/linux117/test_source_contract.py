#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Focused .117 source/control-flow checks; compiled ARM64 ABI is separate."""
import importlib.util
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]
COMMON = ROOT / 'kernel_platform/common'
BASE = '8ea149a53695e05a6b7ebe9ed720a1b33a365813'
spec = importlib.util.spec_from_file_location('host', ROOT / 'tools/driver-updates/test_coloros17_unified_kernel_backports.py')
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)


def old(path):
    return subprocess.check_output(['git', '-C', str(ROOT), 'show', BASE + ':kernel_platform/common/' + path], text=True)


def current(path):
    return (COMMON / path).read_text()


def body(text, marker):
    a = text.index(marker)
    b = text.index('{', a)
    depth, end = 1, b + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[a:end]


class Linux117SourceContract(unittest.TestCase):
    def test_feature_configuration_is_exactly_preserved(self):
        self.assertEqual(current('arch/arm64/configs/gki_defconfig'), old('arch/arm64/configs/gki_defconfig'))

    def test_no_reference_or_stock_module_policy_mutation(self):
        paths = ['kernel_platform/common/gki', 'kernel_platform/common/KernelSU',
                 'kernel_platform/common/fs/susfs.c', 'kernel_platform/common/certs',
                 'kernel_platform/soc-repo', 'vendor']
        changed = subprocess.check_output(['git', '-C', str(ROOT), 'diff', '--name-only', BASE, '--', *paths], text=True)
        self.assertEqual(changed, '')

    def test_device_type_external_definition_is_identical(self):
        marker = 'struct device_type {'
        self.assertEqual(body(current('include/linux/device.h'), marker), body(old('include/linux/device.h'), marker))

    def test_bus_contract_consumes_only_reserve_one(self):
        marker = 'struct bus_type {'
        new = body(current('include/linux/device/bus.h'), marker)
        replacement = 'ANDROID_KABI_USE(1, bool (*need_parent_lock_for)(const struct device *dev)) __kabi_reserved1;'
        self.assertEqual(new.count(replacement), 1)
        self.assertEqual(new.replace(replacement, 'ANDROID_KABI_RESERVE(1);'), body(old('include/linux/device/bus.h'), marker))

    def test_actual_parent_lock_predicate_control_flow(self):
        helper = body(current('drivers/base/base.h'), 'static inline bool device_needs_parent_lock(')
        usb = body(current('drivers/usb/core/driver.c'), 'static bool usb_need_parent_lock(')
        code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
struct device;
struct device_type { int identity; };
struct bus_type {
 bool need_parent_lock;
 union { bool (*need_parent_lock_for)(const struct device *); } __kabi_reserved1;
};
struct device { struct device *parent; const struct bus_type *bus; const struct device_type *type; };
static const struct device_type interface_type={1}, device_type={2}, stock_type={3};
static bool is_usb_interface(const struct device *d) { return d->type == &interface_type; }
'''
        code += helper + '\n' + usb
        code += r'''
int main(void) {
 struct device parent={0};
 struct bus_type usb={.need_parent_lock=true,.__kabi_reserved1={.need_parent_lock_for=usb_need_parent_lock}};
 struct bus_type legacy={.need_parent_lock=true};
 struct device d={.parent=&parent,.bus=&usb,.type=&interface_type};
 assert(device_needs_parent_lock(&d));
 d.type=&device_type; assert(!device_needs_parent_lock(&d));
 d.type=&stock_type; assert(!device_needs_parent_lock(&d));
 d.type=NULL; assert(!device_needs_parent_lock(&d));
 d.type=&interface_type; d.parent=NULL; assert(!device_needs_parent_lock(&d));
 d.parent=&parent; d.bus=&legacy; assert(!device_needs_parent_lock(&d));
 d.bus=NULL; assert(!device_needs_parent_lock(&d));
 d.bus=&usb; usb.need_parent_lock=false; assert(device_needs_parent_lock(&d));
 return 0;
}
'''
        result = host.host_c(code)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_no_stale_device_type_field_access(self):
        for path in ['drivers/base/bus.c', 'drivers/base/dd.c']:
            self.assertNotIn('type->need_parent_lock', current(path))
            self.assertIn('device_needs_parent_lock(dev)', current(path))
        self.assertNotIn('.need_parent_lock', body(current('drivers/usb/core/message.c'), 'const struct device_type usb_if_device_type ='))

    def test_irq_work_retains_wait_before_action_free(self):
        header = current('include/linux/interrupt.h')
        self.assertIn('ANDROID_KABI_IGNORE(0, struct callback_head thread_dtor);', header)
        source = current('kernel/irq/manage.c')
        stop = body(source, 'static void irq_stop_thread(')
        self.assertLess(stop.index('kthread_stop_put'), stop.index('wait_on_bit'))
        self.assertIn('IRQTF_EXIT_WORK', stop)
        free = body(source, 'static struct irqaction *__free_irq(')
        self.assertIn('irq_stop_thread(action)', free)
        self.assertIn('irq_stop_thread(action->secondary)', free)

    def test_remoteproc_recovery_state_is_retained(self):
        self.assertIn('ANDROID_KABI_IGNORE(3, bool subdevs_started);', current('include/linux/remoteproc.h'))
        self.assertIn('rproc->subdevs_started', current('drivers/remoteproc/remoteproc_core.c'))

    def test_hid_battery_offset_uses_real_reserved_storage(self):
        source = body(current('include/linux/hid.h'), 'struct hid_device {')
        self.assertNotIn('__s32 battery_report_offset;', source)
        self.assertIn('ANDROID_KABI_USE(1, __s32 battery_report_offset);', source)
        self.assertIn('#ifdef CONFIG_HID_BATTERY_STRENGTH\n\tANDROID_KABI_USE', source)
        self.assertIn('dev->battery_report_offset = field->report_offset;', current('drivers/hid/hid-input.c'))
        self.assertIn('int offset = 1 + dev->battery_report_offset / 8;', current('drivers/hid/hid-input.c'))

    def test_hiddev_keeps_real_refcount_lifetime_fix(self):
        self.assertIn('refcount_t refcount;', current('include/linux/hiddev.h'))
        text = current('drivers/hid/usbhid/hiddev.c')
        for marker in ['static int hiddev_release(', 'void hiddev_disconnect(']:
            function = body(text, marker)
            self.assertLess(function.index('mutex_unlock'), function.index('hiddev_put(hiddev)'))
        self.assertIn('refcount_inc(&hiddev->refcount)', text)
        self.assertIn('refcount_set(&hiddev->refcount, 1)', text)

    def test_conservative_sample_fix_uses_existing_tail_padding(self):
        header = body(current('drivers/cpufreq/cpufreq_governor.h'), 'struct policy_dbs_info {')
        self.assertLess(header.index('bool work_in_progress;'), header.index('ANDROID_KABI_IGNORE(0, unsigned int max_sample_load);'))
        self.assertIn('policy_dbs->max_sample_load = max_sample_load;', current('drivers/cpufreq/cpufreq_governor.c'))
        self.assertIn('policy_dbs->max_sample_load < cs_tuners->down_threshold', current('drivers/cpufreq/cpufreq_conservative.c'))

    def test_new_private_binding_types_have_no_rust_runtime_users(self):
        makefile = current('rust/Makefile')
        self.assertIn("--no-default irqaction__bindgen_ty_1 --no-default 'bus_type__bindgen_ty_1.*'", makefile)
        for source in (COMMON / 'rust').rglob('*.rs'):
            self.assertNotIn('.need_parent_lock_for', source.read_text(), str(source))
            self.assertNotIn('.thread_dtor', source.read_text(), str(source))

    def test_unsafe_rust_function_merging_remains_disabled(self):
        self.assertIn('RUSTC_FLAGS_CFI   := -Zsanitizer=kcfi -Zsanitizer-cfi-normalize-integers -Zmerge-functions=disabled', current('Makefile'))
        self.assertNotIn('filter-out -Zmerge-functions', current('rust/Makefile'))

    def test_rust_alias_rules_are_scoped_source_descriptions(self):
        rules = current('rust/kabi/coloros17_legacy_aliases.c')
        self.assertEqual(rules.count('ANDROID_KABI_TYPE_STRING('), 6)
        self.assertNotRegex(rules, r'0x[0-9a-fA-F]{8}')
        self.assertNotIn('EXPORT_SYMBOL', rules)
        self.assertIn('offsetof(struct rb_node, rb_left) == 16', rules)
        self.assertIn('sizeof(class_irq_t) == 8', rules)
        makefile = current('rust/Makefile')
        self.assertIn('--only-section=.discard.gendwarfksyms.kabi_rules', makefile)
        self.assertIn('--add-section .discard.gendwarfksyms.kabi_rules=', makefile)
        self.assertNotIn('__versions', makefile)

    def test_infrared_lifetime_and_old_structure_are_preserved(self):
        self.assertEqual(current('include/media/rc-core.h'), old('include/media/rc-core.h'))
        release = body(current('drivers/media/rc/rc-main.c'), 'static void rc_dev_release(')
        self.assertLess(release.index('ir_raw_event_free(dev)'), release.index('kfree(dev)'))
        unregister = body(current('drivers/media/rc/rc-ir-raw.c'), 'void ir_raw_event_unregister(')
        self.assertNotIn('ir_raw_event_free(dev)', unregister)
        self.assertLess(unregister.index('get_task_struct(dev->raw->thread);'),
                        unregister.index('kthread_stop(dev->raw->thread);'))
        free = body(current('drivers/media/rc/rc-ir-raw.c'), 'void ir_raw_event_free(')
        self.assertLess(free.index('timer_delete_sync'), free.index('kfree(dev->raw)'))
        self.assertIn('put_task_struct', free)
        self.assertIn('mutex_lock(&ir_raw_handler_lock)', free)
        self.assertIn('rcu_replace_pointer', current('drivers/media/rc/bpf-lirc.c'))

    def test_reviewed_futex_delta_is_exact_upstream_fix(self):
        path = 'kernel/futex/core.c'
        before = old(path)
        raw = (Path(__file__).with_name('fixtures') / 'futex_cleanup_end.c').read_text()
        marker = 'static void futex_cleanup_end('
        expected = before.replace(body(before, marker), body(raw, marker))
        self.assertEqual(current(path), expected)
        self.assertIn('key->private.__tmp = get_object_id(&mm->futex_unique_id);', current(path))
        self.assertIn('ANDROID_KABI_IGNORE(2, atomic64_t futex_unique_id)', current('include/linux/mm_types.h'))

    def test_security_and_android_specific_ordering_retained(self):
        source = current('fs/exec.c')
        begin = body(source, 'int begin_new_exec(')
        self.assertLess(begin.index('io_uring_task_cancel()'), begin.index('de_thread(me)'))
        self.assertIn('dma_buf_begin_new_exec(old_files)', begin)
        self.assertIn('smp_acquire__after_ctrl_dep()', current('fs/crypto/keysetup.c'))
        self.assertIn('nf_ct_expect_related_pair', current('net/netfilter/nf_conntrack_expect.c'))
        self.assertIn('if (strhdr->sh_type != SHT_STRTAB)', current('kernel/module/main.c'))
        self.assertIn('if (info->sechdrs[str_idx].sh_type != SHT_STRTAB)', current('kernel/module/main.c'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
