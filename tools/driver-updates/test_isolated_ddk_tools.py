#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host-only infrastructure tests. No kernel/Bazel/device operation is run."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest

def load(name):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

B=load('isolated-ddk-module-build')
C=load('prove-ddk-preparation-closure')

class BuilderInputs(unittest.TestCase):
    def test_local_label(self):self.assertEqual(B.label_parts('//vendor/a:canoe_perf_x'),('vendor/a','canoe_perf_x'))
    def test_reject_common_build(self):
        for label in ['//common:x','//common/drivers:x','@external//foo:x','//vendor/../common:x']:
            with self.subTest(label=label),self.assertRaises(ValueError):B.label_parts(label)
    def test_reject_noncanonical_label(self):
        for label in ['//vendor//a:x','//vendor/./a:x','//vendor/a/:x','//vendor/a:x//y','//external/a:x']:
            with self.subTest(label=label),self.assertRaises(ValueError):B.label_parts(label)
    def test_reject_shell_path(self):
        for path in ['/tmp/a b','/tmp/$(id)','/tmp/a;id','/tmp/a`id`']:
            with self.assertRaises(ValueError):B.safe_path(path)
    def test_source_snapshot_detects_changed_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'x.c').write_text('old');a=B.snapshot_inputs(p);(p/'x.c').write_text('new');self.assertNotEqual(a,B.snapshot_inputs(p))
    def test_source_snapshot_ignores_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'x.c').write_text('old');a=B.snapshot_inputs(p);(p/'x.o').write_bytes(b'ELF');self.assertEqual(a,B.snapshot_inputs(p))
    def test_source_snapshot_includes_config_variants(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            for name in ['Kconfig.vendor','Makefile.vendor','build.config.canoe','canoe.defconfig']:(p/name).write_text('config')
            self.assertEqual(len(B.snapshot_inputs(p)),4)
    def test_dependency_closure_detects_sibling_source_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'vendor/wlan/driver').mkdir(parents=True);(p/'vendor/wlan/shared').mkdir()
            (p/'vendor/wlan/driver/x.c').write_text('driver');(p/'vendor/wlan/shared/y.h').write_text('before')
            before=B.snapshot_closure(p,'vendor/wlan/driver',['vendor/wlan'])
            (p/'vendor/wlan/shared/y.h').write_text('after')
            self.assertNotEqual(before,B.snapshot_closure(p,'vendor/wlan/driver',['vendor/wlan']))
    def test_actual_module_commands_exclude_inherited_host_commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);out=p/'output/common';out.mkdir(parents=True);module=p/'output/vendor/test';module.mkdir(parents=True);root=p/'root';root.mkdir()
            source=root/'real.h';source.write_text('actual source')
            (out/'.historical-host.cmd').write_text('deps_host := \\\n /not/present/historical.h \\\n\n')
            (module/'.actual.o.cmd').write_text('deps_actual := \\\n '+str(source)+' \\\n\n')
            actual=B.compiler_inputs(out,root,module)
            self.assertEqual(actual['unresolved_dependency_names'],[])
            self.assertEqual(set(actual['actual_dependency_sha256']),{str(source)})
    def test_missing_forced_header_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);m=p/'module';m.mkdir();(m/'x.cflags_shipped').write_text('-include $(ROOT_DIR)/bazel-out/x/header.h\n')
            with self.assertRaises(ValueError):B.freeze_generated_flags(m,p/'prepared',p/'run',p/'root')
    def test_unused_include_directory_is_not_generated_input(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);m=p/'module';m.mkdir();(m/'x.cflags_shipped').write_text('-I$(ROOT_DIR)/bazel-out/unused/include\n')
            self.assertEqual(B.freeze_generated_flags(m,p/'prepared',p/'run',p/'root'),{})
    def test_existing_include_directory_is_not_recursively_copied(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);m=p/'module';m.mkdir();d=p/'prepared/bazel-out/runfiles';d.mkdir(parents=True);(d/'unconsumed.h').write_text('unused')
            (m/'x.cflags_shipped').write_text('-I$(ROOT_DIR)/bazel-out/runfiles\n')
            self.assertEqual(B.freeze_generated_flags(m,p/'prepared',p/'run',p/'root'),{})
            self.assertFalse((p/'run/generated-inputs').exists())
    def test_generated_header_bytes_preserved(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);m=p/'module';m.mkdir();src=p/'prepared/bazel-out/x/header.h';src.parent.mkdir(parents=True);src.write_bytes(b'#define REAL_GENERATOR 1\n')
            (m/'x.cflags_shipped').write_text('-include $(ROOT_DIR)/bazel-out/x/header.h\n')
            result=B.freeze_generated_flags(m,p/'prepared',p/'run',p/'root')
            self.assertEqual(len(result),1);self.assertEqual((p/'run/generated-inputs/bazel-out/x/header.h').read_bytes(),src.read_bytes())
    def test_unchanged_flag_file_keeps_mtime_for_incremental_retry(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);m=p/'module';m.mkdir();flags=m/'x.cflags_shipped';flags.write_text('-I/already/frozen/source\n')
            os.utime(flags,ns=(1000000000000,1000000000000))
            B.freeze_generated_flags(m,p/'prepared',p/'run',p/'root')
            self.assertEqual(flags.stat().st_mtime_ns,1000000000000)

class ActionClosure(unittest.TestCase):
    def data(self, mnemonic='FileWrite'):
        return {'targets':[{'id':1,'label':'//x:config'},{'id':2,'label':'//x:kernel'}],
                'actions':[{'targetId':1,'mnemonic':'Config','outputIds':[1],'inputDepSetIds':[1]},
                           {'targetId':2,'mnemonic':mnemonic,'outputIds':[2]}],
                'depSetOfFiles':[{'id':1,'directArtifactIds':[2]}]}
    def test_safe_preparation(self):self.assertEqual(C.prove(self.data(),['//x:config'])['result'],'PASS')
    def test_reject_kernel_compile(self):self.assertEqual(C.prove(self.data('KernelBuild'),['//x:config'])['result'],'FAIL')
    def test_reject_module_compile(self):self.assertEqual(C.prove(self.data('KernelModule'),['//x:config'])['result'],'FAIL')
    def test_missing_target_fails(self):
        with self.assertRaises(ValueError):C.prove(self.data(),['//x:missing'])
    def test_transitive_depset(self):
        d=self.data('KernelBuild');d['depSetOfFiles']=[{'id':1,'transitiveDepSetIds':[2]},{'id':2,'directArtifactIds':[2]}]
        self.assertEqual(C.prove(d,['//x:config'])['result'],'FAIL')
    def test_tree_file_does_not_hide_kernel_compile(self):
        d=self.data('KernelBuild');d['depSetOfFiles'][0]['directArtifactIds']=[3]
        d['pathFragments']=[{'id':1,'label':'generated'},{'id':2,'label':'member.h','parentId':1}]
        d['artifacts']=[{'id':2,'pathFragmentId':1,'isTreeArtifact':True},{'id':3,'pathFragmentId':2}]
        self.assertEqual(C.prove(d,['//x:config'])['result'],'FAIL')

if __name__=='__main__':unittest.main()
