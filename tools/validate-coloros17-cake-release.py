#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Verify the exact tested raw-boot release, without upgrading its gate claims.

Normal GitHub publication is owner-requested, not full technical qualification.
This profile is intentionally separate from the controlled-stack/ZIP profiles.
No builds, signing, source/reference updates or device operations are performed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
TAG = 'coloros17.0.0.102-linux-6.12.116-cake'
RECORD = Path('docs/releases') / TAG
SOURCE = '737f8bbabba274fec2d38cb22df9f854daebfa9b'
TREE = '481a38e99f82a476cdfa1ec1023cd9d4232dcaae'
BOOT = 'f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16'
IMAGE = '456436bea4d1d1f126b4ebfce9001253871f4b0edf1d2af07c8116f836428038'
FILES = {'boot.img', 'boot.img.sha256', 'manifest.json', 'README.md', 'SHA256SUMS'}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def validate_metadata(m, repo, release_commit=None):
    # The committed record pins every claim, including actual FAIL/pending gates.
    # An asset manifest may only resolve the symbolic tag target to its commit.
    expected = json.loads((ROOT / RECORD / 'manifest.json').read_text())
    normalized = dict(m)
    normalized['release_commit'] = 'TAG_TARGET'
    require(normalized == expected, 'metadata differs from reviewed release record')
    require(m['release_tag'] == TAG and m['source_commit'] == SOURCE and m['common_tree'] == TREE,
            'wrong release/source profile')
    require(m['prerelease'] is False and m['stable_qualified'] is False,
            'normal channel must not claim full stable qualification')
    require(m['enforced_GKI'] == {'result': 'FAIL', 'owners': 45, 'CAKE_new_owners': 0,
                                  'ABI_reference_updated': False}, 'GKI outcome changed/concealed')
    require(m['write_partitions'] == ['boot'] and not m['replacement_modules'],
            'stock-DLKM boot-only scope changed')
    require(m['artifacts']['boot.img'] == {'sha256': BOOT, 'bytes': 100663296}, 'wrong tested boot')
    require(git(repo, 'rev-parse', SOURCE + ':kernel_platform/common') == TREE, 'source tree mismatch')
    target = m['release_commit']
    require(target == 'TAG_TARGET' or re.fullmatch(r'[0-9a-f]{40}', target), 'invalid release commit')
    if release_commit:
        require(re.fullmatch(r'[0-9a-f]{40}', release_commit), 'invalid expected commit')
        require(target in ('TAG_TARGET', release_commit), 'wrong manifest release commit')
        subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', SOURCE, release_commit], check=True)
        require(git(repo, 'rev-parse', release_commit + ':kernel_platform/common') == TREE,
                'release changed the tested kernel')


def validate_assets(m, folder, repo, release_commit):
    require(release_commit and m['release_commit'] == release_commit, 'asset commit must be finalized')
    require(folder.is_dir() and {p.name for p in folder.iterdir()} == FILES, 'unexpected/missing assets')
    require(all(p.is_file() and not p.is_symlink() for p in folder.iterdir()), 'invalid asset type')
    require(json.loads((folder / 'manifest.json').read_text()) == m, 'asset manifest differs')
    require((folder / 'README.md').read_text() == subprocess.check_output(
        ['git', '-C', str(repo), 'show', release_commit + ':' + str(RECORD / 'README.md')], text=True),
        'release instructions differ from tagged source')
    boot = folder / 'boot.img'
    require(boot.stat().st_size == 100663296 and digest(boot) == BOOT, 'wrong tested boot bytes')
    require((folder / 'boot.img.sha256').read_text() == BOOT + '  boot.img\n', 'invalid boot sidecar')
    checksums = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  ([^/]+)', line)
        require(match and match[2] not in checksums, 'invalid/duplicate checksum')
        checksums[match[2]] = match[1]
    require(set(checksums) == FILES - {'SHA256SUMS'}, 'incomplete/extra checksums')
    require(all(digest(folder / name) == value for name, value in checksums.items()), 'checksum mismatch')
    with boot.open('rb') as stream:
        header = stream.read(4096)
        require(header[:8] == b'ANDROID!', 'invalid boot magic')
        kernel, ramdisk = struct.unpack_from('<II', header, 8)
        require(kernel == 43112960 and ramdisk == 0, 'kernel/ramdisk ownership changed')
        require(struct.unpack_from('<I', header, 20)[0] == 1584 and
                struct.unpack_from('<I', header, 40)[0] == 4, 'wrong boot v4 header')
        require(header[44:1580] == bytes(1536), 'nonempty boot command line')
        require(hashlib.sha256(stream.read(kernel)).hexdigest() == IMAGE, 'embedded Image mismatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--assets', type=Path)
    parser.add_argument('--repo', type=Path, default=ROOT)
    parser.add_argument('--release-commit')
    args = parser.parse_args()
    m = json.loads(args.manifest.read_text())
    validate_metadata(m, args.repo, args.release_commit)
    if args.assets:
        validate_assets(m, args.assets, args.repo, args.release_commit)
        print('EXACT_TESTED_BOOT_AND_PUBLISHED_ASSETS=PASS')
    print('RELEASE_METADATA=PASS; NORMAL_CHANNEL=OWNER_REQUESTED')
    print('FULL_STABLE_QUALIFICATION=NOT_CLAIMED; ENFORCED_GKI=FAIL(45)')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, OSError, struct.error, subprocess.CalledProcessError) as error:
        raise SystemExit('RELEASE_REJECTED: ' + str(error))
