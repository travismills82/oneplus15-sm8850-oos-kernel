#!/usr/bin/env python3
"""Unprivileged scope fixtures; no signed image, real partition or device use."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('boot_only_scope', HERE/'validate-coloros17-boot-only.py')
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)


class BootOnlyScope(unittest.TestCase):
    def setUp(self):
        self.policy = json.loads(M.POLICY.read_text())
        self.temp = tempfile.TemporaryDirectory(prefix='boot-only-scope-fixture-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def image(self, *, cmdline='', version=4, size=None, ramdisk=0):
        path = self.root/'boot.img'
        header = bytearray(4096)
        header[:8] = b'ANDROID!'
        struct.pack_into('<II', header, 8, 64, ramdisk)
        struct.pack_into('<I', header, 20, 1584)
        struct.pack_into('<I', header, 40, version)
        header[44:44+len(cmdline)] = cmdline.encode()
        with path.open('wb') as f:
            f.write(header)
            f.truncate(size if size is not None else self.policy['boot_partition_bytes'])
        return path

    def config(self, override=None, extra=''):
        values={**self.policy['required_modular_config'], **dict.fromkeys(
            ('CONFIG_MODVERSIONS','CONFIG_MODULE_SIG','CONFIG_GENDWARFKSYMS','CONFIG_CFI_CLANG'),'y')}
        values.update(override or {})
        path=self.root/'config'
        path.write_text(''.join(k+'='+v+'\n' for k,v in values.items())+extra)
        return path

    def test_committed_policy(self):
        M.validate_policy(self.policy)

    def test_replacement_modules_rejected(self):
        p=copy.deepcopy(self.policy);p['replacement_modules']=['msm_kgsl.ko']
        with self.assertRaises(ValueError):M.validate_policy(p)

    def test_partition_expansion_rejected(self):
        p=copy.deepcopy(self.policy);p['allowed_write_partitions'].append('vbmeta')
        with self.assertRaises(ValueError):M.validate_policy(p)

    def test_GEAS_exclusion_rejected(self):
        p=copy.deepcopy(self.policy);p['disabled_stock_modules']=['oplus_bsp_geas_data']
        with self.assertRaises(ValueError):M.validate_policy(p)

    def test_stock_configuration(self):
        M.validate_config(self.config(), self.policy)

    def test_builtin_conversion_rejected(self):
        for key in self.policy['required_modular_config']:
            with self.subTest(key=key),self.assertRaises(ValueError):
                M.validate_config(self.config({key:'y'}),self.policy)

    def test_security_disabling_rejected(self):
        for key in ('CONFIG_MODVERSIONS','CONFIG_MODULE_SIG','CONFIG_GENDWARFKSYMS','CONFIG_CFI_CLANG'):
            with self.subTest(key=key),self.assertRaises(ValueError):
                M.validate_config(self.config({key:'n'}),self.policy)

    def test_duplicate_config_rejected(self):
        with self.assertRaises(ValueError):
            M.validate_config(self.config(extra='CONFIG_BT=y\n'),self.policy)

    def test_expected_boot_header_scope(self):
        self.assertEqual(M.validate_boot(self.image(),self.policy)['cmdline'],'')

    def test_no_GEAS_container_rejected(self):
        with self.assertRaises(ValueError):
            M.validate_boot(self.image(cmdline='module_blacklist=oplus_bsp_geas_data'),self.policy)

    def test_unexpected_header_version_rejected(self):
        with self.assertRaises(ValueError):M.validate_boot(self.image(version=3),self.policy)

    def test_unexpected_ramdisk_rejected(self):
        with self.assertRaises(ValueError):M.validate_boot(self.image(ramdisk=32),self.policy)

    def test_wrong_partition_size_rejected(self):
        with self.assertRaises(ValueError):M.validate_boot(self.image(size=8192),self.policy)

    def test_truncated_kernel_rejected(self):
        p=self.image()
        with p.open('r+b') as f:f.seek(8);f.write(struct.pack('<I',0xffffffff))
        with self.assertRaises(ValueError):M.validate_boot(p,self.policy)

    def test_allowed_payload_set(self):
        self.image();(self.root/'SHA256SUMS').write_text('fixture only\n')
        M.validate_assets(self.root,self.policy)

    def test_extra_image_rejected(self):
        self.image();(self.root/'vendor_dlkm.img').touch()
        with self.assertRaises(ValueError):M.validate_assets(self.root,self.policy)

    def test_mixed_case_image_rejected(self):
        self.image();(self.root/'VBMETA.IMG').touch()
        with self.assertRaises(ValueError):M.validate_assets(self.root,self.policy)

    def test_hidden_module_archive_or_payload_rejected(self):
        self.image()
        for name in ('driver.ko','modules.tar','kernel.zip','payload.bin'):
            p=self.root/name;p.touch()
            with self.subTest(name=name),self.assertRaises(ValueError):M.validate_assets(self.root,self.policy)
            p.unlink()

    def test_symlink_rejected(self):
        self.image();(self.root/'README.md').symlink_to('/dev/null')
        with self.assertRaises(ValueError):M.validate_assets(self.root,self.policy)

    def test_nested_payload_rejected(self):
        self.image();(self.root/'hidden').mkdir();(self.root/'hidden/boot.img').touch()
        with self.assertRaises(ValueError):M.validate_assets(self.root,self.policy)


if __name__ == '__main__':unittest.main()
