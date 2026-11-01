#!/usr/bin/env python3
"""Unprivileged fixture tests. Never use device nodes, ADB or real partitions."""
from __future__ import annotations
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock
import zipfile

HERE=Path(__file__).resolve().parent
SPEC=importlib.util.spec_from_file_location('canary_packager',HERE/'build-coloros17-no-geas-canary.py')
M=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(M)


def digest(data):return hashlib.sha256(data).hexdigest()


class RecoveryTransaction(unittest.TestCase):
    shell=('bash',)
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='coloros115-fixture-')
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.blocks=self.root/'blocks';self.blocks.mkdir()
        self.work=self.root/'work';self.work.mkdir()
        self.data=self.root/'durable storage';self.data.mkdir()
        self.boot=b'A'*4096;self.vendor=b'V'*8192;self.newboot=b'B'*4096
        for name in ('boot_a','boot_b','vbmeta_a','vbmeta_b','init_boot_a','init_boot_b','vendor_boot_b','dtbo_b','recovery_a','recovery_b','system_dlkm_b','super'):
            value=b'S'*8192 if name=='super' else self.boot if name=='boot_b' else b'X'*4096
            (self.blocks/name).write_bytes(value)
        (self.blocks/'vendor_dlkm_b').write_bytes(self.vendor)
        (self.work/'boot.img').write_bytes(self.newboot);(self.work/'vendor_dlkm.img').write_bytes(self.vendor)
        self.original={p.name:p.read_bytes() for p in self.blocks.iterdir()}

    def run_fixture(self, mode='dry-run', problem=''):
        r=self.root;b=self.blocks;w=self.work
        values={
            'PROFILE':'fixture-no-device-access','FIRMWARE':'CPH2745_17.0.0.102(EX01)',
            'MIN_BATTERY':30,'MAX_BATTERY_TEMP':450,
            'RECOVERY':'3.7.1_16-OnePlus_15','RECOVERY_KERNEL':'qualified-recovery',
            'BOOT_BYTES':4096,'VENDOR_BYTES':8192,'PARENT_BYTES':4096,
            'BOOT_SHA':digest(self.newboot),'VENDOR_SHA':digest(self.vendor),
            'PARENT_SHA':digest(self.original['vbmeta_b']),'EXISTING_VENDOR_SHA':digest(self.vendor),
            'SUPER_PREFIX_BYTES':8192,'SUPER_PREFIX_SHA':digest(self.original['super']),
            'VENDOR_SECTORS':16,'VENDOR_START':2048,
            'COMPANIONS':' '.join(n+':'+digest(self.original[n]) for n in ('init_boot_b','vendor_boot_b','dtbo_b','system_dlkm_b')),
            'FIXTURE':str(r),'FIXTURE_BLOCKS':str(b),'FIXTURE_DATA':str(self.data),'FAULT':problem,
        }
        script='set -eu\nset -o pipefail\n'+''.join(k+'='+shlex.quote(str(v))+'\n' for k,v in values.items())
        script+='ui_print() { printf "%s\\n" "$*"; }\n. '+shlex.quote(str(M.HELPER))+'\n'
        script+=r'''
# These OS probes are fixture backends, not included in the production ZIP.
canary_partition() { test -f "$FIXTURE_BLOCKS/$1" && printf '%s\n' "$FIXTURE_BLOCKS/$1"; }
canary_property() {
 case "$1" in
 ro.boot.slot_suffix) test "$FAULT" != slot || { echo _a; return; }; echo _b ;;
 ro.boot.slot) echo b ;;
 ro.twrp.version) echo "$RECOVERY" ;;
 ro.product.device) test "$FAULT" != device || { echo other; return; }; echo infiniti ;;
 twrp.user.0.decrypt) test "$FAULT" != locked || { echo 0; return; }; echo 1 ;;
 esac
}
canary_root_uid() { echo 0; }
canary_battery_capacity() { if test "$FAULT" = battery_low; then echo 5; else echo 80; fi; }
canary_battery_temperature() {
 test "$FAULT" != battery_error || return 1
 if test "$FAULT" = battery_hot; then echo 500; else echo 270; fi
}
canary_kernel() { echo qualified-recovery; }
canary_bootconfig() {
 test "$FAULT" != boot_locked || return 1
 printf 'androidboot.vbmeta.device_state = "unlocked"\nandroidboot.verifiedbootstate = "orange"\n'
}
canary_mounts() {
 echo 'tmpfs /tmp tmpfs rw 0 0'
 test "$FAULT" != metadata_unmounted && echo '/dev/block/meta /metadata ext4 rw 0 0'
 test "$FAULT" != data_unmounted && echo '/dev/block/data /data f2fs rw,inlinecrypt 0 0'
 test "$FAULT" != mounted || printf '%s /vendor_dlkm ext4 ro 0 0\n' "$FIXTURE_BLOCKS/vendor_dlkm_b"
 :
}
canary_capacity() {
 test "$FAULT" != capacity || { echo 1; return; }
 wc -c < "$1" | tr -d '[:space:]'
}
canary_super_device() { echo 8:14; }
canary_vendor_table() {
 echo 'Targets in the device-mapper table for vendor_dlkm_b:'
 if test "$FAULT" = snapshot_mapping; then echo '0-16: snapshot, 8:14'; else echo '0-16: linear, 8:14 2048'; fi
}
canary_no_holders() { test "$FAULT" != holders; }
canary_vendor_alias_exists() { test "$FAULT" = alias; }
canary_snapshot_present() { test "$FAULT" != snapshot_unknown; }
canary_snapshot_state() {
 test "$FAULT" != snapshot_read_error || return 1
 test "$FAULT" != active_snapshot || { echo merging; return; }
 :
}
canary_snapshot_count() {
 test "$FAULT" != snapshot_count_error || return 1
 if test "$FAULT" = snapshot_records; then echo 1; else echo 0; fi
}
canary_firmware_display() {
 if test "$FAULT" = wrong_firmware; then FIRMWARE_DISPLAY=other; else FIRMWARE_DISPLAY=$FIRMWARE; fi
}
canary_data_root() { echo "$FIXTURE_DATA"; }
canary_data_filesystem() {
 case "$FAULT" in nondurable_backup) echo tmpfs ;; other_filesystem) echo /dev/block/other ;; *) echo /dev/block/data ;; esac
}
canary_resolve_path() { printf '%s\n' "$1"; }
canary_free_kb() { if test "$FAULT" = no_space; then echo 1; else echo 999999; fi; }
dd() {
 output=''
 for arg in "$@"; do case "$arg" in of=*) output=${arg#of=} ;; esac; done
 if test -n "$output"; then printf '%s\n' "$output" >> "$FIXTURE/writes.log"; fi
 case "$output" in
 "$FIXTURE_DATA"/*)
   test "$FAULT" != backup_failed || return 1
   ;;
 "$FIXTURE_BLOCKS/boot_b")
   if test ! -f "$FIXTURE/once"; then
     case "$FAULT" in
     write_failed|signal)
       : > "$FIXTURE/once"
       command dd if=/dev/zero of="$output" bs=4096 count=1 conv=fsync 2>/dev/null
       test "$FAULT" != signal || kill -TERM $$
       return 1 ;;
     bad_readback)
       : > "$FIXTURE/once"
       command dd "$@" 2>/dev/null
       printf Z | command dd of="$output" bs=1 conv=notrunc 2>/dev/null
       return 0 ;;
     esac
   fi
   ;;
 esac
 command dd "$@" 2>/dev/null
}
'''
        script+='canary_install '+shlex.quote(mode)+' '+shlex.quote(str(w))+'\n'
        return subprocess.run([*self.shell,'-c',script],capture_output=True,text=True,timeout=15)

    def assert_unchanged(self):
        self.assertEqual({p.name:p.read_bytes() for p in self.blocks.iterdir()},self.original)

    def test_full_dry_run_has_no_partition_or_backup_write(self):
        p=self.run_fixture();self.assertEqual(p.returncode,0,p.stdout+p.stderr)
        self.assertIn('DRY RUN PASS',p.stdout);self.assert_unchanged()
        self.assertFalse((self.root/'writes.log').exists())

    def test_full_install_only_vendor_then_boot_protects_parent_and_other_partitions(self):
        p=self.run_fixture('flash');self.assertEqual(p.returncode,0,p.stdout+p.stderr)
        self.assertEqual((self.blocks/'boot_b').read_bytes(),self.newboot)
        self.assertEqual((self.blocks/'vendor_dlkm_b').read_bytes(),self.vendor)
        for n,v in self.original.items():
            if n not in ('boot_b','vendor_dlkm_b'):self.assertEqual((self.blocks/n).read_bytes(),v,n)
        writes=(self.root/'writes.log').read_text().splitlines()
        targets=[x for x in writes if x.startswith(str(self.blocks)+'/')]
        self.assertEqual(targets,[str(self.blocks/'vendor_dlkm_b'),str(self.blocks/'boot_b')])
        backups=list(self.data.rglob('manifest.txt'));self.assertEqual(len(backups),1)
        self.assertIn('INSTALL_READBACK_VERIFIED',backups[0].read_text())

    def test_all_unsafe_preflights_fail_without_partition_writes(self):
        for reason in ('slot','device','boot_locked','metadata_unmounted','data_unmounted','capacity','snapshot_mapping','holders','alias','snapshot_unknown','snapshot_read_error','active_snapshot','snapshot_count_error','snapshot_records','wrong_firmware','locked','no_space','nondurable_backup','other_filesystem','battery_low','battery_hot','battery_error'):
            with self.subTest(reason=reason):
                p=self.run_fixture(problem=reason)
                self.assertNotEqual(p.returncode,0,p.stdout+p.stderr)
                self.assert_unchanged();self.assertFalse((self.root/'writes.log').exists())

    def test_wrong_parent_is_rejected_without_any_write(self):
        (self.blocks/'vbmeta_b').write_bytes(b'Y'*4096)
        p=self.run_fixture();self.assertNotEqual(p.returncode,0)
        self.assertEqual((self.blocks/'vbmeta_b').read_bytes(),b'Y'*4096)
        self.assertFalse((self.root/'writes.log').exists())

    def test_payload_corruption_is_rejected(self):
        (self.work/'boot.img').write_bytes(b'Z'*4096)
        p=self.run_fixture();self.assertNotEqual(p.returncode,0);self.assert_unchanged()

    def test_existing_vendor_and_companion_corruption_rejected(self):
        for name in ('vendor_dlkm_b','init_boot_b','vendor_boot_b','dtbo_b','system_dlkm_b'):
            with self.subTest(name=name):
                (self.blocks/name).write_bytes(b'Z'*len(self.original[name]))
                p=self.run_fixture();self.assertNotEqual(p.returncode,0)
                self.assertFalse((self.root/'writes.log').exists())
                (self.blocks/name).write_bytes(self.original[name])
        self.assert_unchanged()

    def test_super_metadata_corruption_is_rejected(self):
        (self.blocks/'super').write_bytes(b'Z'*8192)
        p=self.run_fixture();self.assertNotEqual(p.returncode,0)
        self.assertEqual((self.blocks/'super').read_bytes(),b'Z'*8192)
        self.assertFalse((self.root/'writes.log').exists())

    def test_backup_failure_never_writes_targets(self):
        p=self.run_fixture('flash','backup_failed')
        self.assertNotEqual(p.returncode,0);self.assert_unchanged()
        targets=[s for s in (self.root/'writes.log').read_text().splitlines() if s.startswith(str(self.blocks)+'/')]
        self.assertEqual(targets,[])

    def test_write_failure_restores_both_backups(self):
        p=self.run_fixture('flash','write_failed')
        self.assertNotEqual(p.returncode,0);self.assertIn('restored and verified',p.stdout);self.assert_unchanged()

    def test_readback_failure_restores_both_backups(self):
        p=self.run_fixture('flash','bad_readback')
        self.assertNotEqual(p.returncode,0);self.assertIn('restored and verified',p.stdout);self.assert_unchanged()

    def test_term_during_write_restores_both_backups(self):
        p=self.run_fixture('flash','signal')
        self.assertNotEqual(p.returncode,0);self.assertIn('restored and verified',p.stdout);self.assert_unchanged()

    def test_vbmeta_is_not_an_allowed_write_target(self):
        p=subprocess.run(['bash','-c','. "$1"; canary_write vbmeta /does/not/exist nope','test',str(M.HELPER)],capture_output=True)
        self.assertNotEqual(p.returncode,0);self.assert_unchanged()


class Packaging(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='coloros115-zip-fixture-');self.addCleanup(self.tmp.cleanup)
        self.r=Path(self.tmp.name);self.images=self.r/'images';self.images.mkdir()
        self.p=copy.deepcopy(M.profile())
        for name,size in [('boot.img',4096),('vendor_dlkm.img',8192)]:
            data=bytearray(size)
            if name=='boot.img':data[:8]=b'ANDROID!'
            else:data[1080:1082]=b'\x53\xef'
            data[-64:-60]=b'AVBf';(self.images/name).write_bytes(data)
            self.p['payloads'][name]={'bytes':size,'sha256':digest(data)}
        self.p['required_existing_vendor_sha256']=self.p['payloads']['vendor_dlkm.img']['sha256']
        self.seal=self.r/'seal.json';self.seal.write_bytes(M.encoded({'source_head':self.p['kernel_source_commit'],'kernel_release':self.p['kernel_release']}))
        self.notes=self.r/'notes.md';self.notes.write_text('Restricted fixture; not a production or physical validation artifact.\n')
        self.out=self.r/'assets'
        self.stack=contextlib.ExitStack();self.addCleanup(self.stack.close)
        self.stack.enter_context(mock.patch.object(M,'profile',return_value=self.p))
        self.stack.enter_context(mock.patch.object(M,'SEAL_SHA',M.sha(self.seal)))
        self.stack.enter_context(mock.patch.object(M,'NOTES',self.notes))

    def build(self,out=None):
        with contextlib.redirect_stdout(io.StringIO()):M.build(self.images,out or self.out,self.seal)

    def test_complete_roundtrip_and_no_vbmeta_payload(self):
        self.build()
        with zipfile.ZipFile(self.out/self.p['zip_name']) as z:
            self.assertEqual({n for n in z.namelist() if n.endswith('.img')},{'boot.img','vendor_dlkm.img'})
            self.assertIn('6.12.115',z.read('kernel-info.txt').decode())
        with contextlib.redirect_stdout(io.StringIO()):M.verify(self.out,allow_symbolic=True)

    def test_second_build_is_byte_identical(self):
        self.build();other=self.r/'second';self.build(other)
        self.assertEqual(M.sha(self.out/self.p['zip_name']),M.sha(other/self.p['zip_name']))

    def test_wrong_input_hash_is_fatal(self):
        (self.images/'boot.img').write_bytes(b'Z'*4096)
        with self.assertRaises(ValueError):self.build()

    def test_extra_vbmeta_asset_rejected(self):
        self.build();(self.out/'vbmeta.img').write_bytes(b'NO')
        with self.assertRaisesRegex(ValueError,'unexpected/missing release assets'):M.verify(self.out,allow_symbolic=True)

    def test_embedded_vbmeta_and_duplicate_path_rejected(self):
        for name in ('vbmeta.img','boot.img'):
            with self.subTest(name=name):
                out=self.r/name.replace('.','_');self.build(out)
                path=out/self.p['zip_name']
                with zipfile.ZipFile(path,'a') as z:z.writestr(name,b'NO')
                manifest=json.loads((out/'canary-manifest.json').read_text())
                manifest['zip'].update(bytes=path.stat().st_size,sha256=M.sha(path))
                (out/'canary-manifest.json').write_bytes(M.encoded(manifest));(out/'SHA256SUMS').write_bytes(M.filesums(out))
                with self.assertRaisesRegex(ValueError,'ZIP contains duplicate'):M.verify(out,allow_symbolic=True)

    def test_existing_directory_never_overwritten(self):
        self.out.mkdir()
        with self.assertRaisesRegex(ValueError,'refusing to overwrite'):self.build()

    def test_wrong_publication_commit_rejected(self):
        self.build()
        with self.assertRaises(ValueError):M.verify(self.out,commit='0'*40)

    def test_invalid_finalization_source_does_not_mutate_assets(self):
        self.build()
        before={p.name:M.sha(p) for p in self.out.iterdir()}
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaises(subprocess.CalledProcessError):
            M.finalize(self.out,'0'*40)
        self.assertEqual({p.name:M.sha(p) for p in self.out.iterdir()},before)


@unittest.skipUnless(shutil.which('busybox'),'BusyBox ash is not installed on this host')
class RecoveryAshTransaction(RecoveryTransaction):
    shell=('busybox','sh')


if __name__=='__main__':unittest.main(verbosity=2)
