#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Narrow source guards; compiled ABI and VM checks are separate requirements."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]
COMMON = ROOT / 'kernel_platform/common'
BASE = 'd6552d3625e0656aeac303b878347bef208b4315'


def original(path):
    return subprocess.check_output(['git', '-C', str(ROOT), 'show', f'{BASE}:{path}'], text=True)


class DroidspacesSourceContract(unittest.TestCase):
    def test_only_reviewed_kernel_inputs_changed(self):
        changed = subprocess.check_output(
            ['git', '-C', str(ROOT), 'diff', '--name-only', BASE, '--', 'kernel_platform/common'],
            text=True).splitlines()
        self.assertEqual(set(changed), {
            'kernel_platform/common/arch/arm64/configs/gki_defconfig',
            'kernel_platform/common/include/linux/sched.h',
            'kernel_platform/common/ipc/msgutil.c',
            'kernel_platform/common/ipc/namespace.c',
            'kernel_platform/common/rust/Makefile',
        })

    def test_exact_authoritative_config_delta(self):
        path = 'kernel_platform/common/arch/arm64/configs/gki_defconfig'
        old = original(path).splitlines()
        new = (ROOT / path).read_text().splitlines()
        self.assertEqual(set(new) - set(old), {
            'CONFIG_SYSVIPC=y', 'CONFIG_POSIX_MQUEUE=y', 'CONFIG_USER_NS=y',
            'CONFIG_NETFILTER_XT_MATCH_ADDRTYPE=y', 'CONFIG_DEVTMPFS=y',
        })
        self.assertEqual(set(old) - set(new), {'# CONFIG_PID_NS is not set'})

    def test_existing_modular_architecture_retained(self):
        config = (COMMON / 'arch/arm64/configs/gki_defconfig').read_text()
        for name in ['BT', 'BT_HIDP', 'PPP', 'RFKILL']:
            self.assertRegex(config, rf'(?m)^CONFIG_{name}=m$')
        # SLHC is selected by modular PPP, not an explicit savedefconfig line.
        self.assertNotIn('CONFIG_SLHC=y', config)

    def test_no_optional_controller_or_network_expansion(self):
        config = (COMMON / 'arch/arm64/configs/gki_defconfig').read_text()
        for name in ['CGROUP_PIDS', 'CGROUP_DEVICE', 'NF_TABLES', 'MACVLAN',
                     'BRIDGE_NETFILTER', 'IP6_NF_NAT', 'DEVTMPFS_MOUNT']:
            self.assertNotRegex(config, rf'(?m)^CONFIG_{name}=[ym]$')

    def test_ipc_exports_do_not_change_lifetime_implementation(self):
        for leaf, symbol in [('msgutil.c', 'init_ipc_ns'), ('namespace.c', 'put_ipc_ns')]:
            path = 'kernel_platform/common/ipc/' + leaf
            new = (ROOT / path).read_text()
            self.assertEqual(new.count(f'EXPORT_SYMBOL_GPL({symbol});'), 1)
            restored = new.replace('#include <linux/export.h>\n', '').replace(
                f'EXPORT_SYMBOL_GPL({symbol});\n', '')
            self.assertEqual(restored, original(path))

    def test_ipc_fields_are_not_packed_or_in_vendor_reserves(self):
        text = (COMMON / 'include/linux/sched.h').read_text()
        start = text.index('ANDROID_KABI_IGNORE(0, struct sysv_sem sysvsem)')
        end = text.index('struct sched_entity', start)
        fragment = text[start:end]
        self.assertIn('ANDROID_KABI_IGNORE(1, struct sysv_shm sysvshm)', fragment)
        self.assertNotIn('__packed', fragment)
        self.assertNotIn('ANDROID_KABI_USE', fragment)
        self.assertIn('SysV IPC state must fit entirely in the pre-existing se alignment gap', text)
        self.assertIn('__alignof__(struct sysv_sem) == 0', text)
        self.assertIn('__alignof__(struct sysv_shm) == 0', text)

    def test_rust_view_does_not_expose_private_ipc_or_construct_it(self):
        text = (COMMON / 'include/linux/sched.h').read_text()
        start = text.index('#ifndef __BINDGEN__', text.index('GKI has alignment padding'))
        end = text.index('#endif', start)
        self.assertIn('ANDROID_KABI_IGNORE(0, struct sysv_sem sysvsem)', text[start:end])
        self.assertIn('ANDROID_KABI_IGNORE(1, struct sysv_shm sysvshm)', text[start:end])
        make = (COMMON / 'rust/Makefile').read_text()
        self.assertIn('$(and $(CONFIG_SYSVIPC),$(CONFIG_ARM64),$(CONFIG_GENDWARFKSYMS))', make)
        self.assertIn('--no-default sysv_sem --no-default sysv_shm', make)
        for path in (COMMON / 'rust').rglob('*.rs'):
            source = path.read_text()
            self.assertNotIn('bindings::sysv_sem', source, str(path))
            self.assertNotIn('bindings::sysv_shm', source, str(path))


    def test_no_abi_reference_or_policy_changes(self):
        output = subprocess.check_output(
            ['git', '-C', str(ROOT), 'diff', '--name-only', BASE, '--',
             'kernel_platform/common/gki', 'kernel_platform/common/kernel/module',
             'kernel_platform/common/KernelSU', 'kernel_platform/common/fs/susfs.c',
             'kernel_platform/common/include/linux/susfs*', 'vendor',
             'kernel_platform/soc-repo', 'kernel_platform/common/certs'], text=True)
        self.assertEqual(output, '')

    def test_susfs_global_mount_hiding_default_unchanged(self):
        text = (COMMON / 'fs/susfs.c').read_text()
        self.assertRegex(text, r'DEFINE_STATIC_KEY_FALSE\(susfs_is_hide_sus_mnts_for_non_su_procs_enabled\)')


if __name__ == '__main__':
    unittest.main()
