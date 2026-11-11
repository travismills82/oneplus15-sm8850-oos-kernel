#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Reject wrong .117 bytes, firmware scope and inflated qualification claims."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('release', ROOT / 'tools/validate-coloros17-linux117-release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class Metadata(unittest.TestCase):
    def setUp(self):
        self.m = json.loads((ROOT / release.RECORD / 'manifest.json').read_text())

    def test_reviewed_record(self):
        release.validate_metadata(self.m, ROOT, release.SOURCE)

    def test_preserve_disclosed_failed_and_pending_gates(self):
        for section, key, value in (
            (None, 'stable_qualified', True), (None, 'prerelease', True),
            ('enforced_GKI', 'result', 'PASS'), ('enforced_GKI', 'owners', 45),
            ('enforced_GKI', 'unexplained_new_owners', 1),
            ('qualification', 'OTG_storage', 'PASS'),
            ('qualification', 'controlled_unplugged_power', 'PASS'),
            ('qualification', 'historical_host_tests', '202/202 PASS'),
            ('module_oracle', 'CRC_mismatches', 1), ('AVB', 'flags', 3),
        ):
            with self.subTest(section=section, key=key):
                m = copy.deepcopy(self.m)
                (m if section is None else m[section])[key] = value
                with self.assertRaises(ValueError):
                    release.validate_metadata(m, ROOT)

    def test_wrong_contract(self):
        for key, value in (
            ('source_commit', '0' * 40), ('common_tree', '0' * 40),
            ('firmware', 'CPH2745_17.0.0.102(EX01)'),
            ('write_partitions', ['boot', 'vendor_dlkm']),
            ('replacement_modules', ['bluetooth']), ('Droidspaces_userspace_embedded', True),
            ('Image_sha256', '0' * 64), ('release_commit', '../main'),
            ('kernel_release', '6.12.116-android16-6-o-test-4k'),
        ):
            with self.subTest(key=key):
                m = copy.deepcopy(self.m)
                m[key] = value
                with self.assertRaises(ValueError):
                    release.validate_metadata(m, ROOT)

    def test_finalized_commit_must_match(self):
        m = copy.deepcopy(self.m)
        m['release_commit'] = release.SOURCE
        release.validate_metadata(m, ROOT, release.SOURCE)
        m['release_commit'] = '0' * 40
        with self.assertRaises(ValueError):
            release.validate_metadata(m, ROOT, release.SOURCE)

    def test_wrong_or_extra_artifact(self):
        for name, value in (
            ('boot.img', {'sha256': '0' * 64, 'bytes': 100663296}),
            ('vendor_dlkm.img', {'sha256': '0' * 64, 'bytes': 1}),
        ):
            m = copy.deepcopy(self.m)
            m['artifacts'][name] = value
            with self.assertRaises(ValueError):
                release.validate_metadata(m, ROOT)

    def test_unreviewed_downloaded_asset_rejected(self):
        m = copy.deepcopy(self.m)
        m['release_commit'] = release.SOURCE
        for extra in ('vendor_dlkm.img', 'vbmeta.img', 'init_boot.img', 'kernel-TWRP.zip'):
            with self.subTest(extra=extra), tempfile.TemporaryDirectory() as tmp:
                folder = Path(tmp)
                for name in release.FILES | {extra}:
                    (folder / name).touch()
                with self.assertRaisesRegex(ValueError, 'unexpected/missing assets'):
                    release.validate_assets(m, folder, ROOT, release.SOURCE)

    def test_symbolic_asset_commit_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(ValueError, 'asset commit must be finalized'):
                release.validate_assets(self.m, Path(tmp), ROOT, release.SOURCE)


if __name__ == '__main__':
    unittest.main()
