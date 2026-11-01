#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Package an existing, exact-hash boot payload; never build/repack a kernel."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / 'tools/coloros17-boot-only-installer/update-binary.in'
FAMILY = ROOT / 'tools/twrp-installer-template/META-INF/com/google/android/update-binary.in'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def render(tag, boot_sha):
    source = FAMILY.read_text()
    guard = source[source.index('family_prop() {'):source.index('\nresolve_boot_partition() {')]
    source = TEMPLATE.read_text().replace('@FAMILY_GUARD@', guard)
    return source.replace('@RELEASE_TAG@', tag).replace('@BOOT_SHA256@', boot_sha)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--boot', type=Path, required=True)
    p.add_argument('--boot-sha256', required=True)
    p.add_argument('--source-commit', required=True)
    p.add_argument('--release-tag', required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    if not all(c.isalnum() or c in '.-_' for c in args.release_tag):
        p.error('unsafe release tag')
    payload = args.boot.read_bytes()
    if len(payload) != 100663296 or payload[:8] != b'ANDROID!' or sha(payload) != args.boot_sha256:
        p.error('payload does not match the exact qualified boot hash/size/header')
    commit = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse',
                                     args.source_commit + '^{commit}'], text=True).strip()
    epoch = int(subprocess.check_output(['git', '-C', str(ROOT), 'show', '-s',
                                        '--format=%ct', commit], text=True))
    timestamp = datetime.fromtimestamp(epoch, timezone.utc).timetuple()[:6]
    info = {'release': args.release_tag, 'firmware': 'CPH2745_17.0.0.102(EX01)',
            'source_commit': commit, 'boot_sha256': sha(payload), 'boot_bytes': len(payload),
            'payload': 'boot only; original stock DLKM and AVB required',
            'runtime': 'boot/FBE and SUSFS availability PASS; extended qualification pending',
            'installer_runtime': 'host fixtures only; exact ZIP not physically installed'}
    readme = ('OnePlus 15 ColorOS 17.0.0.102(EX01), CPH2745IN only. CANARY.\n'
              'Verify the published SHA256SUMS. Boot a trusted custom recovery, decrypt\n'
              '/data or mount durable external storage for the mandatory boot backup.\n'
              'Install this ZIP, then Reboot System without switching slots. It writes\n'
              'only active boot after exact stock module/firmware/AVB/snapshot guards.\n'
              'It never writes DLKM, vbmeta, init_boot, vendor_boot, recovery or DTBO.\n'
              'Rollback: restore the saved boot_<slot>.img to that same boot partition\n'
              'using recovery/fastboot. Never wipe userdata/metadata or change slots\n'
              'as a rollback substitute. Install the matching official KernelSU APK\n'
              'as an Android app update separately, not through this ZIP.\n')
    files = {'boot.img': payload,
             'META-INF/com/google/android/update-binary': render(args.release_tag, sha(payload)).encode(),
             'META-INF/com/google/android/updater-script': b'ui_print("ColorOS .102 boot-only KernelSU/SUSFS canary");\n',
             'kernel-info.json': (json.dumps(info, indent=2) + '\n').encode(),
             'README.txt': readme.encode()}
    files['checksums.sha256'] = ''.join(sha(data) + '  ' + name + '\n'
                                      for name, data in sorted(files.items())).encode()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.out, 'x') as archive:
        for name, data in sorted(files.items()):
            entry = zipfile.ZipInfo(name, timestamp)
            entry.create_system = 3
            entry.external_attr = (0o100755 if name.endswith('update-binary') else 0o100644) << 16
            entry.compress_type = zipfile.ZIP_STORED if name == 'boot.img' else zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
    print(json.dumps({'zip': str(args.out), 'sha256': sha(args.out.read_bytes()),
                      'boot_sha256': sha(payload)}, indent=2))


if __name__ == '__main__':
    main()
