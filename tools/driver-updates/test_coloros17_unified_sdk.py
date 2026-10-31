#!/usr/bin/env python3
"""Synthetic SDK pin/negative tests. No kernel, Bazel or phone operation."""
import hashlib
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from test_isolated_ddk_tools import B


class UnifiedSDK(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = self.root / 'sdk.json'
        self.data = {'schema_version': 1, 'kernel_source_commit': 'a' * 40,
                     'common_tree': 'b' * 40, 'inputs': {}}
        for name in ('frozen_symvers', 'prepare_archive', 'environment', 'vmlinux_symvers',
                     'localversion', 'kernel_config', 'Image', 'build_log', 'build_exit'):
            p = self.root / name
            p.write_text('0\n' if name == 'build_exit' else 'synthetic fixture ' + name)
            record = {'path': str(p), 'sha256': B.digest(p)}
            if name in ('build_log', 'build_exit'):
                self.data[name] = record
            else:
                self.data['inputs'][name] = record

    def seal(self):
        self.manifest.write_text(json.dumps(self.data))
        return B.digest(self.manifest)

    def test_complete_explicitly_pinned_fixture(self):
        data, inputs = B.read_kernel_sdk(self.manifest, self.seal())
        self.assertEqual(data['common_tree'], 'b' * 40)
        self.assertEqual(len(inputs), 7)

    def test_manifest_hash_required(self):
        self.seal()
        for wrong in (None, '', 'c' * 64):
            with self.subTest(hash=wrong), self.assertRaises(ValueError):
                B.read_kernel_sdk(self.manifest, wrong)

    def test_changed_input_rejected(self):
        expected = self.seal()
        (self.root / 'frozen_symvers').write_text('changed')
        with self.assertRaisesRegex(ValueError, 'artifact changed'):
            B.read_kernel_sdk(self.manifest, expected)

    def test_failed_normal_build_rejected(self):
        (self.root / 'build_exit').write_text('1\n')
        self.data['build_exit']['sha256'] = B.digest(self.root / 'build_exit')
        with self.assertRaisesRegex(ValueError, 'did not succeed'):
            B.read_kernel_sdk(self.manifest, self.seal())

    def test_missing_image_rejected(self):
        del self.data['inputs']['Image']
        with self.assertRaisesRegex(ValueError, 'SDK inputs'):
            B.read_kernel_sdk(self.manifest, self.seal())

    def test_source_identity_not_shell_or_ref_expression(self):
        self.data['kernel_source_commit'] = 'HEAD:kernel_platform/common'
        with self.assertRaisesRegex(ValueError, 'source identity'):
            B.read_kernel_sdk(self.manifest, self.seal())

    def test_relative_or_shell_path_rejected(self):
        for value in ('relative/path', '/tmp/a;id'):
            self.data['inputs']['Image']['path'] = value
            with self.subTest(path=value), self.assertRaises(ValueError):
                B.read_kernel_sdk(self.manifest, self.seal())

    def test_incompatible_common_tree_still_rejected(self):
        args = SimpleNamespace(source_worktree=self.root, prepared_worktree=None,
                               expected_head='d' * 40, label='//vendor/a:canoe_perf_a',
                               kernel_sdk_manifest=self.manifest,
                               kernel_sdk_manifest_sha256=self.seal())
        def git(worktree, *command):
            if command == ('rev-parse', 'HEAD'):
                return 'd' * 40
            if command == ('rev-parse', 'HEAD:kernel_platform/common'):
                return 'c' * 40
            if command == ('rev-parse', 'a' * 40 + ':kernel_platform/common'):
                return 'b' * 40
            raise AssertionError(command)
        with patch.object(B, 'git', git), self.assertRaisesRegex(ValueError, 'Common source'):
            B.plan(args)

    def test_standalone_transition_selected_from_verified_environment(self):
        relative = 'bazel-out/host-real/bin/build/kernel/hermetic-tools'
        (self.root / relative).mkdir(parents=True)
        (self.root / 'bazel-out/host-other/bin/build/kernel/hermetic-tools').mkdir(parents=True)
        env = self.root / 'env.sh'
        env.write_text('KLEAF_HERMETIC_BASE=${KLEAF_HERMETIC_BASE:-' + relative + '}\n')
        self.assertEqual(B.standalone_hermetic_base(env, self.root), relative)

    def test_standalone_transition_rejects_escape(self):
        env = self.root / 'env.sh'
        env.write_text('KLEAF_HERMETIC_BASE=${KLEAF_HERMETIC_BASE:-bazel-out/../bad}\n')
        with self.assertRaises(ValueError):
            B.standalone_hermetic_base(env, self.root)


if __name__ == '__main__':
    unittest.main()
