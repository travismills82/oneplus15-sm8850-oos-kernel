#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exact CAKE feature scope; compiled ABI, module and VM tests are separate."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]
BASE = 'c2e2f95d3c3c4ece99591320ebee1dc69a6f2ed4'
CONFIG = 'kernel_platform/common/arch/arm64/configs/gki_defconfig'


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True)


class TrafficControlSourceContract(unittest.TestCase):
    def test_exact_config_change(self):
        old = git('show', f'{BASE}:{CONFIG}')
        new = (ROOT / CONFIG).read_text()
        self.assertEqual(new, old.replace('CONFIG_NET_SCH_FQ_CODEL=y\n',
                         'CONFIG_NET_SCH_FQ_CODEL=y\nCONFIG_NET_SCH_CAKE=y\n'))

    def test_all_three_built_in(self):
        lines = (ROOT / CONFIG).read_text().splitlines()
        for symbol in ['CAKE', 'FQ', 'FQ_CODEL']:
            self.assertEqual(lines.count(f'CONFIG_NET_SCH_{symbol}=y'), 1)
            self.assertNotIn(f'CONFIG_NET_SCH_{symbol}=m', lines)

    def test_only_authoritative_kernel_input_changed(self):
        changed = git('diff', '--name-only', BASE, '--', 'kernel_platform').splitlines()
        self.assertEqual(changed, [CONFIG])

    def test_stock_module_policy_not_changed(self):
        self.assertEqual(git('diff', '--name-only', BASE, '--', 'vendor',
                             'kernel_platform/soc-repo',
                             'kernel_platform/common/kernel/module'), '')

    def test_existing_default_qdisc_not_overridden(self):
        config = (ROOT / CONFIG).read_text()
        self.assertNotIn('CONFIG_NET_SCH_DEFAULT=y', config)
        self.assertNotIn('CONFIG_DEFAULT_NET_SCH=', config)

    def test_scheduler_uses_existing_upstream_implementation(self):
        self.assertEqual(git('diff', '--name-only', BASE, '--',
                             'kernel_platform/common/net/sched',
                             'kernel_platform/common/include'), '')

    def test_runtime_probe_is_network_namespace_isolated(self):
        source = Path(__file__).with_name('qdisc-smoke.c').read_text()
        main = source[source.index('int main(void)'):]
        self.assertLess(main.index('unshare(CLONE_NEWNET)'), main.index('socket('))
        self.assertIn('INADDR_LOOPBACK', source)
        self.assertIn('packets >= 16', source)
        self.assertIn('change_qdisc("cake_missing", 0) == -ENOENT', main)

    def test_reference_and_security_integrations_not_modified(self):
        self.assertEqual(git('diff', '--name-only', BASE, '--',
                             'kernel_platform/common/gki',
                             'kernel_platform/common/KernelSU',
                             'kernel_platform/common/fs/susfs.c',
                             'kernel_platform/common/include/linux/susfs*',
                             'kernel_platform/common/certs'), '')


if __name__ == '__main__':
    unittest.main()
