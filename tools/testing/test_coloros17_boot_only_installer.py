#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Execute the real installer in regular-file fixtures, never host block devices.

Only the fixture copy substitutes device paths/block-node tests and reviewed
stock hashes. All guards, backup/write/readback/rollback control flow executes.
These tests are not physical qualification of the recovery ZIP.
"""
import hashlib
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('packager', ROOT / 'tools/build-coloros17-boot-only-installer.py')
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Installer(unittest.TestCase):
    def case(self, mode='good', slot='_b'):
        with tempfile.TemporaryDirectory(prefix='coloros17-installer-fixture-') as tmp:
            r = Path(tmp)
            for name in ['by-name', 'mapper', 'bin', 'sdcard', 'tmp', 'metadata/ota/snapshots']:
                (r / name).mkdir(parents=True, exist_ok=True)
            payload = b'ANDROID!' + b'NEW' * 2728
            size = len(payload)
            self.assertEqual(size, 8192)
            for s in ['_a', '_b']:
                (r / ('by-name/boot' + s)).write_bytes((b'A' if s == '_a' else b'B') * size)
            protected = {
                'mapper/system_dlkm': '3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a',
                'mapper/vendor_dlkm': '82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47',
                'by-name/vbmeta': '750918fafedd2ffbaaceee5830f2f8a77ed1b5c8d6e149f05739dadc39c93918',
                'by-name/vendor_boot': 'd0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000',
                'by-name/dtbo': '5600107425a28b7efc85d02fd158a875904678efd426889ba91d6b4f510b0b0f'}
            source = packager.render('fixture-canary', sha(payload)).replace('BOOT_BYTES=100663296', 'BOOT_BYTES=8192')
            for name, digest in protected.items():
                data = ('STOCK ' + name).encode()
                for s in ['_a', '_b']:
                    (r / (name + s)).write_bytes(data)
                source = source.replace(digest, sha(data))
            (r / ('mapper/my_manifest' + slot)).write_bytes(b'manifest')
            (r / 'metadata/ota/state').write_bytes(b'')
            (r / 'mounts').write_text('/dev/block/metadata /metadata ext4 rw 0 0\n')
            (r / 'bootconfig').write_text('androidboot.vbmeta.device_state = "unlocked"\n'
                                         'androidboot.verifiedbootstate = "orange"\n'
                                         f'androidboot.slot_suffix = "{slot}"\n')
            (r / 'firmware.prop').write_text('ro.build.display.id=CPH2745_17.0.0.102(EX01)\n')
            for before, after in {
                '/dev/block/bootdevice/by-name': str(r / 'by-name-alias'),
                '/dev/block/by-name': str(r / 'by-name'),
                '/dev/block/mapper': str(r / 'mapper'),
                '/proc/bootconfig': str(r / 'bootconfig'), '/proc/mounts': str(r / 'mounts'),
                '/metadata/ota': str(r / 'metadata/ota'), '/sdcard': str(r / 'sdcard'),
                '/external_sd': str(r / 'missing-sd'), '/usb-otg': str(r / 'missing-usb'),
                '/tmp/coloros17-kernel': str(r / 'tmp/coloros17-kernel'),
                '[ -b ': '[ -f ',
            }.items():
                source = source.replace(before, after)
            scripts = {
                'getprop': '''case "$1" in
ro.product.device) echo infiniti;; ro.product.board) echo canoe;;
ro.boot.prjname) echo 24863;; ro.boot.slot_suffix) echo "$TEST_SLOT";;
ro.boot.slot) echo "${TEST_SLOT#_}";; ro.build.ab_update|ro.virtual_ab.enabled) echo true;;
esac\n''',
                'blockdev': 'stat -c %s "$2"\n',
                'lpdump': 'printf "Slot 1:\\n  Name: system_dlkm%s\\n  Name: vendor_dlkm%s\\n  Name: my_manifest%s\\n" "$TEST_SLOT" "$TEST_SLOT" "$TEST_SLOT"\n',
                'mount': 'test "$1" = -o && test "$2" = ro || exit 4\ncp "$TEST_ROOT/firmware.prop" "$4/build.prop"\n',
                'umount': 'exit 0\n',
                'df': 'if [ "$TEST_MODE" = no_backup ]; then echo "Filesystem Blocks Used Available Use% Mounted"; echo "rootfs 100 1 99 1% /"; else echo "Filesystem Blocks Used Available Use% Mounted"; echo "/dev/block/fixture 1000000 1 999999 1% /data"; fi\n',
                'dd': '''src= dst=
for arg in "$@"; do case "$arg" in if=*) src=${arg#if=};; of=*) dst=${arg#of=};; esac; done
if [ "$TEST_MODE" = backup_failure ]; then
    case "$dst" in */kernel-flash-backups/*) exit 1;; esac
fi
if [ "$dst" = "$TEST_ROOT/by-name/boot$TEST_SLOT" ] && [ "${src##*/}" = boot.img ]; then
    case "$TEST_MODE" in
        partial_write) printf BROKEN > "$dst"; exit 1;;
        bad_readback) printf BROKEN > "$dst"; exit 0;;
    esac
fi
exec /bin/dd "$@"
''',
            }
            for name, text in scripts.items():
                path = r / 'bin' / name
                path.write_text('#!/bin/sh\nset -eu\n' + text)
                path.chmod(0o755)
            if mode == 'wrong_firmware':
                (r / 'firmware.prop').write_text('ro.build.display.id=CPH2749_17.0.0.102(EX01)\n')
            elif mode == 'wrong_stock':
                (r / ('mapper/vendor_dlkm' + slot)).write_bytes(b'changed vendor')
            elif mode == 'wrong_parent':
                (r / ('by-name/vbmeta' + slot)).write_bytes(b'changed parent')
            elif mode == 'ota':
                (r / 'metadata/ota/state').write_bytes(b'active')
            elif mode == 'snapshot':
                (r / 'metadata/ota/snapshots/vendor').write_bytes(b'active')
            elif mode == 'unknown_ota':
                (r / 'metadata/ota/state').unlink()
            elif mode == 'locked':
                text = (r / 'bootconfig').read_text().replace('"unlocked"', '"locked"')
                (r / 'bootconfig').write_text(text)
            elif mode == 'slot_conflict':
                text = (r / 'bootconfig').read_text().replace(f'"{slot}"', '"_invalid"')
                (r / 'bootconfig').write_text(text)
            elif mode == 'capacity':
                (r / ('by-name/boot' + slot)).write_bytes(b'short')
            elif mode == 'alias':
                (r / 'by-name/boot_a').unlink()
                (r / 'by-name/boot_a').symlink_to(r / 'by-name/boot_b')
            elif mode == 'missing_partition':
                (r / ('mapper/vendor_dlkm' + slot)).unlink()
            archive = r / 'test.zip'
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('boot.img', payload if mode != 'corrupt_payload' else b'CORRUPT!' + payload[8:])
            script = r / 'installer.sh'
            script.write_text(source)
            before = {p: p.read_bytes() for directory in ['by-name', 'mapper']
                      for p in (r / directory).iterdir() if p.is_file()}
            env = {**os.environ, 'PATH': str(r / 'bin') + ':/usr/bin:/bin',
                   'TEST_ROOT': str(r), 'TEST_SLOT': slot, 'TEST_MODE': mode,
                   'INSTALLER_DRY_RUN': '1' if mode == 'dry_run' else '0'}
            # Host busybox standalone mode bypasses PATH mocks for applets.
            # Use bash for the regular-file simulation; separately parse the
            # unmodified production script with recovery-style busybox ash.
            result = subprocess.run(['bash', str(script), '3', '99', str(archive)],
                                    env=env, text=True, capture_output=True, timeout=20)
            expected = mode in ['good', 'dry_run']
            self.assertEqual(result.returncode == 0, expected, result.stdout + result.stderr)
            target = r / ('by-name/boot' + slot)
            for p, old in before.items():
                self.assertEqual(p.read_bytes(), payload if p == target and mode == 'good' else old,
                                 f'{mode}: unexpected write to {p}')
            if mode in ['partial_write', 'bad_readback']:
                self.assertIn('Original boot restored and hash-verified.', result.stdout)
            if mode == 'dry_run':
                self.assertFalse(list((r / 'sdcard').iterdir()))

    def test_active_b(self): self.case()
    def test_active_a(self): self.case(slot='_a')
    def test_dry_run(self): self.case('dry_run')
    def test_recovery_shell_syntax(self):
        result = subprocess.run(['busybox', 'sh', '-n'],
                                input=packager.render('syntax-canary', '0' * 64),
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
    def test_rejections_and_rollback(self):
        for mode in ['wrong_firmware', 'wrong_stock', 'wrong_parent', 'ota', 'snapshot',
                     'unknown_ota', 'locked', 'slot_conflict', 'capacity', 'alias',
                     'missing_partition', 'corrupt_payload', 'no_backup', 'backup_failure',
                     'partial_write', 'bad_readback']:
            with self.subTest(mode=mode): self.case(mode)


if __name__ == '__main__':
    unittest.main()
