#!/usr/bin/env python3
"""Fail-closed metadata checks for the exact, already-tested boot canary."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('release', ROOT / 'tools/validate-coloros17-droidspaces-release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


def fixture():
    return {
        'schema': release.SCHEMA, 'release_tag': release.TAG,
        'prerelease': True, 'stable_qualified': False,
        'firmware': release.PROFILE['firmware'], 'source_commit': release.SOURCE,
        'common_tree': release.PROFILE['common_tree'],
        'kernel_release': release.PROFILE['kernel_release'],
        'Image_sha256': 'a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1',
        'config_sha256': 'dcb3585a692d6ba12bfbe01aa647654962a8c962c68593aaa0fc7552af7f3536',
        'Module_symvers_sha256': 'e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8',
        'write_partitions': ['boot'], 'replacement_modules': [],
        'Droidspaces_userspace_embedded': False, 'stock_contract': release.STOCK.copy(),
        'enforced_GKI': {'result': 'FAIL', 'prior_owners': 44, 'current_owners': 45, 'ABI_reference_updated': False},
        'qualification': {'Android_FBE': 'PASS', 'live_requirements_checker': 'PASS',
                          'full_distro_sessions': 'NOT TESTED', 'extended_hardware': 'NOT TESTED',
                          'exact_recovery_ZIP_install': 'HOST TESTS ONLY'},
        'module_oracle': {'modules': 1070, 'active_compatible': 1032, 'dormant': 38,
                          'hard_imports': 60701, 'CRC_records': 61777, 'missing': 0,
                          'CRC_mismatches': 0, 'namespace_failures': 0, 'active_blockers': 0,
                          'production_provider_mismatches': 0},
        'AVB': {'algorithm': 'SHA256_RSA4096', 'flags': 0, 'rollback_index': 0,
                'rollback_index_location': 0, 'OS_property': '16', 'security_patch': '2026-10-01',
                'fingerprint': 'qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys',
                'key_sha1': '2597c218aae470a130f61162feaae70afd97f011'},
        'artifacts': {name: {'sha256': 'a' * 64, 'bytes': 1}
                      for name in release.FILES - {'SHA256SUMS', 'manifest.json'}},
    }


class Metadata(unittest.TestCase):
    def setUp(self):
        self.m = fixture()
        self.m['artifacts']['boot.img'] = {'sha256': release.PROFILE['boot_sha256'], 'bytes': 100663296}

    def test_exact_metadata(self):
        release.validate_metadata(self.m, ROOT, release.SOURCE)

    def test_disallowed_claims(self):
        cases = [('stable_qualified', True), ('prerelease', False),
                 ('Droidspaces_userspace_embedded', True), ('write_partitions', ['boot', 'vendor_dlkm']),
                 ('replacement_modules', ['bluetooth']), ('firmware', 'CPH2745_17.0.0.102(EX01)'),
                 ('common_tree', '0' * 40), ('Image_sha256', '0' * 64)]
        for key, value in cases:
            with self.subTest(key=key):
                m = copy.deepcopy(self.m)
                m[key] = value
                with self.assertRaises(ValueError):
                    release.validate_metadata(m, ROOT)

    def test_disallowed_gate_and_artifact_changes(self):
        cases = [('enforced_GKI', 'result', 'PASS'), ('qualification', 'full_distro_sessions', 'PASS'),
                 ('module_oracle', 'CRC_mismatches', 1), ('AVB', 'flags', 3),
                 ('stock_contract', 'vendor_dlkm', '0' * 64),
                 ('artifacts', 'vendor_dlkm.img', {'sha256': '0' * 64, 'bytes': 1}),
                 ('artifacts', 'boot.img', {'sha256': '0' * 64, 'bytes': 100663296})]
        for section, key, value in cases:
            with self.subTest(section=section, key=key):
                m = copy.deepcopy(self.m)
                m[section][key] = value
                with self.assertRaises(ValueError):
                    release.validate_metadata(m, ROOT)


if __name__ == '__main__':
    unittest.main()
