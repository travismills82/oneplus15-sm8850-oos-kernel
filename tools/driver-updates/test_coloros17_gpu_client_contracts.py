#!/usr/bin/env python3
"""Firmware-bound client declarations, not stock runtime qualification."""
from pathlib import Path
import re
import unittest
from test_coloros17_unified_kernel_backports import host_c

ROOT = Path(__file__).resolve().parents[2]
GPU = ROOT / 'vendor/qcom/opensource/graphics-kernel'


class GPUClientContracts(unittest.TestCase):
    def mtrack(self, replacement=None):
        source = (GPU / 'kgsl_coloros_mtrack.h').read_text()
        source = re.sub(r'^#include .*$', '', source, flags=re.M)
        if replacement:
            source = source.replace('MTRACK_GPU,', replacement)
        return r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
typedef int pid_t;
typedef unsigned short umode_t;
struct proc_ops;
''' + source + r'''
int main(void) {
 assert(MTRACK_DMABUF==0 && MTRACK_GPU==1 && MTRACK_ZRAM==2);
 assert(MTRACK_ERM==3 && MTRACK_UXMEM_POOL==4 && MTRACK_MAX==5);
 assert(MTRACK_GPU_TOTAL==3 && MTRACK_GPU_PROC_KERNEL==4);
 assert(MTRACK_ZRAM_ORIG==5 && MTRACK_ZRAM_SAMEPAGES==8);
 assert(sizeof(enum mtrack_type)==4 && sizeof(enum mtrack_subtype)==4);
 assert(sizeof(struct mtrack_debugger)==3*sizeof(void *));
 assert(offsetof(struct mtrack_debugger, mem_usage)==0);
 assert(offsetof(struct mtrack_debugger, pid_mem_usage)==sizeof(void *));
 assert(offsetof(struct mtrack_debugger, dump_usage_stat)==2*sizeof(void *));
 return 0;
}
'''

    def test_real_mtrack_header_registry_and_callbacks(self):
        result = host_c(self.mtrack())
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_old_wrong_gpu_slot_negative_control(self):
        self.assertNotEqual(host_c(self.mtrack('MTRACK_GPU = 2,')).returncode, 0)

    def test_vmid_preserves_existing_numbers(self):
        source = (ROOT / 'kernel_platform/soc-repo/include/soc/qcom/secure_buffer.h').read_text()
        enum = re.search(r'enum vmid \{.*?\};', source, re.S).group(0)
        result = host_c('#include <assert.h>\n' + enum + r'''
int main(void) {
 assert(sizeof(enum vmid)==4);
 assert(VMID_TZ==1 && VMID_HLOS==3 && VMID_CP_PIXEL==10);
 assert(VMID_OEMVM==0x31 && VMID_GVM1==0x34);
 assert(VMID_LAST==0x35 && VMID_INVAL==-1);
 return 0;
}
''')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_only_required_client_header_and_log_dependency(self):
        source = (GPU / 'kgsl.c').read_text()
        self.assertIn('#include "kgsl_coloros_mtrack.h"', source)
        for file in ('kgsl.c', 'kgsl_sharedmem.c'):
            content = (GPU / file).read_text()
            self.assertNotIn('#include <mm_osvelte/common.h>', content)
            self.assertIn('#include "kgsl_osvelte_log.h"', content)
        self.assertIn('pr_info("osvelte: " fmt', (GPU / 'kgsl_osvelte_log.h').read_text())


if __name__ == '__main__':
    unittest.main()
