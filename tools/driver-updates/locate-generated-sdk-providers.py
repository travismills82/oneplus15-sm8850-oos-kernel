#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Map actual object undefined names to genuine cached producer-table paths.

Never creates/edits a CRC table. Output arguments name complete existing SDK
files, not a runtime replacement set. Audit natural imports/private ABI against
stock after compilation; source-equivalent symbols do not prove compatibility.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--object',type=Path,required=True)
    p.add_argument('--cache-root',type=Path,default=Path('/home/travis/Android/stock_kernel/out/coloros1700102-612114-five-cves/kernel_platform/bazel-bin'))
    p.add_argument('--vmlinux-symvers',type=Path,default=Path('/home/travis/Android/kernel-test-backups/coloros1700102-6.12.114-five-cves-20261029/static-oracle-01/vmlinux-export-records.tsv'))
    p.add_argument('--out-json',type=Path,required=True)
    p.add_argument('--out-args',type=Path,required=True)
    a=p.parse_args()
    if a.out_json.exists() or a.out_args.exists():raise SystemExit('Refusing to overwrite SDK discovery evidence')
    nm='/home/travis/Android/stock_kernel/kernel_platform/prebuilts/clang/host/linux-x86/clang-r536225/bin/llvm-nm'
    undefined={row.split()[-1] for row in subprocess.check_output([nm,'-u',str(a.object)],text=True).splitlines() if row.split()}
    vm={row.split('\t')[1] for row in a.vmlinux_symvers.read_text().splitlines() if row}
    # Defined by generated module metadata, not a kernel-export requirement.
    special={'__this_module'}
    missing=undefined-vm-special
    tables={};found=set()
    for path in a.cache_root.resolve().rglob('*_Module.symvers'):
        if '.runfiles/' in str(path):continue
        exports=[]
        for row in path.read_text().splitlines():
            f=row.split('\t')
            if len(f)>=4 and f[1] in missing:
                exports.append({'symbol':f[1],'crc':f[0],'provider':f[2],'class':f[3],'namespace':f[4] if len(f)>4 else ''});found.add(f[1])
        if exports:tables[str(path)]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'actual_used_exports':exports}
    result={'object':str(a.object),'actual_undefined':len(undefined),'kernel_resolved':len(undefined&vm),'special_references':sorted(undefined&special),
            'non_kernel_symbols':sorted(missing),'uncached_symbols':sorted(missing-found),'genuine_generated_tables':tables,
            'runtime_compatibility':'NOT CLAIMED; do not deploy SDK provider modules'}
    a.out_json.parent.mkdir(parents=True,exist_ok=True)
    a.out_json.write_text(json.dumps(result,indent=2)+'\n')
    a.out_args.write_text('\n'.join('--extra-symvers '+path for path in tables)+'\n')
    print(json.dumps({'actual_undefined':len(undefined),'SDK_tables':len(tables),'uncached_symbols':result['uncached_symbols'],'json':str(a.out_json)},indent=2))

if __name__=='__main__':main()
