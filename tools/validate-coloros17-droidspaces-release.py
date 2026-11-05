#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Validate this exact boot-only canary; never flash or claim stable qualification."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('coloros_packager', ROOT / 'tools/build-coloros17-boot-only-installer.py')
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)
TAG = 'coloros17.0.0.102-linux-6.12.115-droidspaces-canary'
ZIP = 'kernel-coloros-17.0.0.102-linux-6.12.115-droidspaces-TWRP.zip'
PROFILE = packager.PROFILES['droidspaces']
SOURCE = '4f960022774c379000abdb40d230ab563bc86dd7'
SCHEMA = 'coloros17-droidspaces-boot-only-v1'
STOCK = {
    'system_dlkm': '3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a',
    'vendor_dlkm': '82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47',
    'vbmeta': '750918fafedd2ffbaaceee5830f2f8a77ed1b5c8d6e149f05739dadc39c93918',
    'vendor_boot': 'd0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000',
    'dtbo': '5600107425a28b7efc85d02fd158a875904678efd426889ba91d6b4f510b0b0f',
}
FILES = {'boot.img', 'boot.img.sha256', ZIP, ZIP + '.sha256', 'manifest.json', 'README.md', 'SHA256SUMS'}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def validate_metadata(m, repo, release_commit=None):
    require(m['schema'] == SCHEMA and m['release_tag'] == TAG, 'wrong release profile/tag')
    require(m['prerelease'] is True and m['stable_qualified'] is False, 'canary must not claim stable status')
    require(m['firmware'] == PROFILE['firmware'], 'wrong exact firmware')
    require(m['source_commit'] == SOURCE and m['common_tree'] == PROFILE['common_tree'], 'wrong source identity')
    require(git(repo, 'rev-parse', SOURCE + ':kernel_platform/common') == PROFILE['common_tree'], 'source tree mismatch')
    if release_commit:
        require(re.fullmatch(r'[0-9a-f]{40}', release_commit) is not None, 'invalid release commit')
        subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', SOURCE, release_commit], check=True)
        require(git(repo, 'rev-parse', release_commit + ':kernel_platform/common') == PROFILE['common_tree'],
                'release commit changed the tested kernel')
    require(m['kernel_release'] == PROFILE['kernel_release'], 'wrong running release')
    require(m['Image_sha256'] == 'a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1', 'wrong Image')
    require(m['config_sha256'] == 'dcb3585a692d6ba12bfbe01aa647654962a8c962c68593aaa0fc7552af7f3536', 'wrong config')
    require(m['Module_symvers_sha256'] == 'e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8', 'wrong symvers')
    require(m['write_partitions'] == ['boot'] and m['replacement_modules'] == [], 'not boot-only stock-DLKM model')
    require(m['Droidspaces_userspace_embedded'] is False, 'userspace must remain separate')
    require(m['stock_contract'] == STOCK, 'stock partition contract changed')
    require(m['enforced_GKI'] == {'result': 'FAIL', 'prior_owners': 44, 'current_owners': 45,
                                  'ABI_reference_updated': False}, 'GKI status concealed/changed')
    require(m['qualification'] == {
        'Android_FBE': 'PASS', 'live_requirements_checker': 'PASS',
        'full_distro_sessions': 'NOT TESTED', 'extended_hardware': 'NOT TESTED',
        'exact_recovery_ZIP_install': 'HOST TESTS ONLY'}, 'qualification claims changed')
    require(m['module_oracle'] == {
        'modules': 1070, 'active_compatible': 1032, 'dormant': 38,
        'hard_imports': 60701, 'CRC_records': 61777, 'missing': 0,
        'CRC_mismatches': 0, 'namespace_failures': 0, 'active_blockers': 0,
        'production_provider_mismatches': 0}, 'module contract changed')
    require(m['AVB'] == {
        'algorithm': 'SHA256_RSA4096', 'flags': 0, 'rollback_index': 0,
        'rollback_index_location': 0, 'OS_property': '16', 'security_patch': '2026-10-01',
        'fingerprint': 'qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys',
        'key_sha1': '2597c218aae470a130f61162feaae70afd97f011'}, 'AVB contract changed')
    expected = FILES - {'SHA256SUMS', 'manifest.json'}
    require(set(m['artifacts']) == expected, 'unexpected release artifacts')
    for name, item in m['artifacts'].items():
        require(re.fullmatch(r'[0-9a-f]{64}', item['sha256']) is not None and
                isinstance(item['bytes'], int) and item['bytes'] > 0, 'invalid artifact identity: ' + name)
    require(m['artifacts']['boot.img'] == {'sha256': PROFILE['boot_sha256'], 'bytes': 100663296}, 'wrong tested boot')


def validate_assets(m, folder):
    require(folder.is_dir(), 'asset directory missing')
    require({p.name for p in folder.iterdir()} == FILES, 'unexpected/missing assets')
    require(all(p.is_file() and not p.is_symlink() for p in folder.iterdir()), 'invalid asset type')
    for name, item in m['artifacts'].items():
        path = folder / name
        require(path.stat().st_size == item['bytes'] and digest(path) == item['sha256'], 'artifact mismatch: ' + name)
    require(json.loads((folder / 'manifest.json').read_text()) == m, 'asset manifest differs')
    for name in ['boot.img', ZIP]:
        require((folder / (name + '.sha256')).read_text() == digest(folder / name) + '  ' + name + '\n', 'bad sidecar')
    listed = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  ([^/]+)', line)
        require(match is not None and match[2] not in listed, 'invalid/duplicate checksum record')
        listed[match[2]] = match[1]
    require(set(listed) == FILES - {'SHA256SUMS'}, 'checksum coverage mismatch')
    require(all(digest(folder / name) == value for name, value in listed.items()), 'checksum mismatch')
    boot = (folder / 'boot.img').read_bytes()
    require(boot[:8] == b'ANDROID!', 'bad boot header')
    expected = packager.package_files(boot, TAG, SOURCE, 'droidspaces')
    with zipfile.ZipFile(folder / ZIP) as archive:
        names = archive.namelist()
        require(len(names) == len(expected) and set(names) == set(expected), 'unexpected/duplicate ZIP entries')
        require(archive.testzip() is None, 'corrupt ZIP')
        for name, data in expected.items():
            info = archive.getinfo(name)
            mode = 0o100755 if name.endswith('/update-binary') else 0o100644
            require(info.external_attr >> 16 == mode and not info.flag_bits & 1, 'bad ZIP mode/encryption')
            require(info.file_size == len(data) and archive.read(name) == data, 'ZIP payload/helper/metadata differs: ' + name)
            if name == 'boot.img':
                require(info.compress_type == zipfile.ZIP_STORED, 'boot must be stored byte-for-byte')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--assets', type=Path)
    p.add_argument('--repo', type=Path, default=ROOT)
    p.add_argument('--release-commit')
    a = p.parse_args()
    m = json.loads(a.manifest.read_text())
    validate_metadata(m, a.repo, a.release_commit)
    if a.assets:
        validate_assets(m, a.assets)
    print('COLOROS_DROIDSPACES_CANARY_METADATA=PASS')
    if a.assets:
        print('EXACT_BOOT_ZIP_AND_PUBLISHED_ASSETS=PASS')
    print('STABLE_QUALIFICATION=NOT_CLAIMED; ENFORCED_GKI=FAIL(45)')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as error:
        raise SystemExit('RELEASE_REJECTED: ' + str(error))
