#!/usr/bin/env python3
"""Read-only source-contract checks, not compiled KABI or physical validation."""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMMON = ROOT / 'kernel_platform/common'
BASE = 'f116d21665429dc9d6ba88d7b004f4de53796bf8'


def baseline(path):
    return subprocess.check_output(
        ['git', '-C', str(ROOT), 'show', BASE + ':kernel_platform/common/' + path],
        text=True)


class Integration(unittest.TestCase):
    def test_builtin_configuration(self):
        text = (COMMON / 'arch/arm64/configs/gki_defconfig').read_text()
        self.assertIn('CONFIG_KSU=y\n', text)
        self.assertIn('CONFIG_KSU_SUSFS=y\n', text)
        self.assertNotIn('CONFIG_KSU=m', text)

    def test_no_unrelated_defconfig_delta(self):
        text = (COMMON / 'arch/arm64/configs/gki_defconfig').read_text()
        self.assertEqual(''.join(x for x in text.splitlines(True) if 'CONFIG_KSU' not in x),
                         baseline('arch/arm64/configs/gki_defconfig'))

    def test_native_driver_wiring(self):
        self.assertIn('source "drivers/kernelsu/Kconfig"',
                      (COMMON / 'drivers/Kconfig').read_text())
        self.assertRegex((COMMON / 'drivers/Makefile').read_text(),
                         r'obj-\$\(CONFIG_KSU\)\s*\+= kernelsu/')
        self.assertIn('obj-$(CONFIG_KSU_SUSFS) += susfs.o',
                      (COMMON / 'fs/Makefile').read_text())

    def test_no_dynamic_bridge_or_network_fetch(self):
        text = (COMMON / 'drivers/kernelsu/Kbuild').read_text()
        self.assertNotIn('kernelsu-objs += hook/arm64/syscall_hook.o', text)
        self.assertNotIn('git) fetch', text)
        self.assertIn('KSU_VERSION := 32716', text)

    def test_resolved_uapi_is_source(self):
        directory = COMMON / 'drivers/kernelsu/include/uapi'
        self.assertTrue(directory.is_dir())
        self.assertFalse(directory.is_symlink())
        self.assertTrue(list(directory.glob('*.h')))

    def test_thread_flags_do_not_overlap_arm64_registry(self):
        original = baseline('arch/arm64/include/asm/thread_info.h')
        core = set(map(int, re.findall(r'^#define TIF_\w+\s+(\d+)\b', original, re.M)))
        susfs = (COMMON / 'include/linux/susfs_def.h').read_text()
        extension = set(map(int, re.findall(r'^#define TIF_\w+\s+(\d+)\b', susfs, re.M)))
        self.assertEqual(extension, {33, 34, 35})
        self.assertFalse(core & extension)
        self.assertLess(max(extension), 64)

    def test_prior_task_flag_repair_unchanged(self):
        for path in ['include/linux/sched.h', 'include/linux/sched/signal.h']:
            self.assertEqual((COMMON / path).read_text(), baseline(path))

    def test_original_mount_unmount_bit_retained(self):
        self.assertIn('MNT_WRITE_HOLD|MNT_MARKED|MNT_INTERNAL|MNT_UMOUNT',
                      (COMMON / 'fs/namespace.c').read_text())

    def test_smaps_rollup_lock_retry_algorithm_unchanged(self):
        def function(text):
            start = text.index('static int show_smaps_rollup(')
            end = text.index('\nstatic ', start + 1)
            return text[start:end]
        self.assertEqual(function((COMMON / 'fs/proc/task_mmu.c').read_text()),
                         function(baseline('fs/proc/task_mmu.c')))

    def test_no_certificate_or_reference_mutation(self):
        changed = subprocess.check_output(
            ['git', '-C', str(ROOT), 'diff', '--name-only', BASE, '--',
             'kernel_platform/common/certs', 'kernel_platform/common/gki',
             'kernel_platform/common/android'], text=True)
        self.assertEqual(changed, '')

    def test_authorized_uname_spoofing_without_other_identity_changes(self):
        text = (COMMON / 'arch/arm64/configs/gki_defconfig').read_text()
        # Its bool defaults to y when KSU_SUSFS=y. Keep the normalized
        # savedefconfig form rather than adding a redundant default selection.
        self.assertNotIn('# CONFIG_KSU_SUSFS_SPOOF_UNAME is not set', text)
        kconfig = (COMMON / 'drivers/kernelsu/Kconfig').read_text()
        uname = kconfig.split('config KSU_SUSFS_SPOOF_UNAME\n', 1)[1].split('\nconfig ', 1)[0]
        self.assertIn('depends on KSU_SUSFS', uname)
        self.assertIn('default y', uname)
        for name in ['SPOOF_CMDLINE_OR_BOOTCONFIG',
                     'HIDE_KSU_SUSFS_SYMBOLS', 'PRIORITY_UID_FILTER']:
            self.assertIn('# CONFIG_KSU_SUSFS_' + name + ' is not set', text)

    def test_remote_memory_filter_uses_resolved_vma_and_releases_page(self):
        text = (COMMON / 'mm/memory.c').read_text()
        text = text[text.index('static int __access_remote_vm('):]
        gup = text.index('struct page *page = get_user_page_vma_remote')
        guard = text.index('SUSFS_IS_INODE_SUS_MAP')
        self.assertLess(gup, guard)
        self.assertIn('if (!IS_ERR_OR_NULL(page))\n\t\t\t\tput_page(page);', text)
        fallback = text.index('/* GUP failure may leave only the VM_IO/VM_PFNMAP path. */')
        self.assertLess(text.index('vma = vma_lookup(mm, addr);'), fallback)
        self.assertLess(fallback, text.index('bytes = vma->vm_ops->access'))


if __name__ == '__main__':
    unittest.main()
