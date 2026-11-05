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
PROFILES = {
    'kernelsu-susfs': {
        'firmware': 'CPH2745_17.0.0.102(EX01)', 'model': 'CPH2745IN',
        'label': 'KernelSU/SUSFS',
        'runtime': 'boot/FBE and SUSFS availability PASS; extended qualification pending',
    },
    'droidspaces': {
        'firmware': 'CPH2747_17.0.0.102(EX01)', 'model': 'CPH2747',
        'label': 'Droidspaces + KernelSU/SUSFS',
        'runtime': 'Android/FBE and live requirements checker PASS; full distro/hardware tests pending',
        'boot_sha256': '0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc',
        'common_tree': '6d074b67a27e4ccd1582be8178ab0bf8284870aa',
        'kernel_release': '6.12.115-android16-6-o-gd6552d3625e0-4k',
    },
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def render(tag, boot_sha, profile='kernelsu-susfs'):
    settings = PROFILES[profile]
    source = FAMILY.read_text()
    guard = source[source.index('family_prop() {'):source.index('\nresolve_boot_partition() {')]
    source = TEMPLATE.read_text().replace('@FAMILY_GUARD@', guard)
    return (source.replace('@RELEASE_TAG@', tag).replace('@BOOT_SHA256@', boot_sha)
            .replace('@FIRMWARE_DISPLAY_ID@', settings['firmware'])
            .replace('@FEATURE_LABEL@', settings['label']))


def package_files(payload, tag, commit, profile='kernelsu-susfs'):
    settings = PROFILES[profile]
    info = {'release': tag, 'firmware': settings['firmware'],
            'source_commit': commit, 'boot_sha256': sha(payload), 'boot_bytes': len(payload),
            'payload': 'boot only; original stock DLKM and AVB required',
            'runtime': settings['runtime'],
            'installer_runtime': 'host fixtures only; exact ZIP not physically installed'}
    if profile == 'droidspaces':
        info.update({'profile': profile, 'kernel_release': settings['kernel_release'],
                     'common_tree': settings['common_tree'],
                     'droidspaces': 'built-in kernel prerequisites; app/runtime installed separately',
                     'enforced_gki': 'FAIL: 44 unchanged owners plus one reviewed IPC-padding owner'})
    readme = (f'OnePlus 15 ColorOS 17.0.0.102(EX01), {settings["model"]} only. CANARY.\n'
              'Verify the published SHA256SUMS. Boot a trusted custom recovery, decrypt\n'
              '/data or mount durable external storage for the mandatory boot backup.\n'
              'Install this ZIP, then Reboot System without switching slots. It writes\n'
              'only active boot after exact stock module/firmware/AVB/snapshot guards.\n'
              'It never writes DLKM, vbmeta, init_boot, vendor_boot, recovery or DTBO.\n'
              'Rollback: restore the saved boot_<slot>.img to that same boot partition\n'
              'using recovery/fastboot. Never wipe userdata/metadata or change slots\n'
              'as a rollback substitute. Install the matching official KernelSU APK\n'
              'as an Android app update separately, not through this ZIP.\n')
    if profile == 'droidspaces':
        readme += ('Droidspaces app/runtime is separate; no APK or Linux rootfs is embedded.\n'
                   'Preserve your existing KernelSU-patched init_boot. Full distro tests\n'
                   'remain pending; this is not stable or universal device qualification.\n')
    files = {'boot.img': payload,
             'META-INF/com/google/android/update-binary': render(tag, sha(payload), profile).encode(),
             'META-INF/com/google/android/updater-script':
                 f'ui_print("ColorOS .102 boot-only {settings["label"]} canary");\n'.encode(),
             'kernel-info.json': (json.dumps(info, indent=2) + '\n').encode(),
             'README.txt': readme.encode()}
    files['checksums.sha256'] = ''.join(sha(data) + '  ' + name + '\n'
                                      for name, data in sorted(files.items())).encode()
    return files


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--boot', type=Path, required=True)
    p.add_argument('--boot-sha256', required=True)
    p.add_argument('--source-commit', required=True)
    p.add_argument('--release-tag', required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--profile', choices=PROFILES, default='kernelsu-susfs')
    args = p.parse_args()
    if not all(c.isalnum() or c in '.-_' for c in args.release_tag):
        p.error('unsafe release tag')
    payload = args.boot.read_bytes()
    if len(payload) != 100663296 or payload[:8] != b'ANDROID!' or sha(payload) != args.boot_sha256:
        p.error('payload does not match the exact qualified boot hash/size/header')
    commit = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse',
                                     args.source_commit + '^{commit}'], text=True).strip()
    settings = PROFILES[args.profile]
    if args.profile == 'droidspaces':
        tree = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse',
                                        commit + ':kernel_platform/common'], text=True).strip()
        if sha(payload) != settings['boot_sha256'] or tree != settings['common_tree']:
            p.error('Droidspaces profile requires the exact physically tested boot/source tree')
    epoch = int(subprocess.check_output(['git', '-C', str(ROOT), 'show', '-s',
                                        '--format=%ct', commit], text=True))
    timestamp = datetime.fromtimestamp(epoch, timezone.utc).timetuple()[:6]
    files = package_files(payload, args.release_tag, commit, args.profile)
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
