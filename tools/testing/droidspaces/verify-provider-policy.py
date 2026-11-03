#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Independently check actual linked GKI import/export policy and precedence."""
import argparse
import csv
import importlib.util
import json
from pathlib import Path
import struct

spec = importlib.util.spec_from_file_location('elf_audit', Path(__file__).with_name('compare-compiled-abi.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--vmlinux', type=Path, required=True)
p.add_argument('--symvers', type=Path, required=True)
p.add_argument('--oracle', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
elf = module.Elf(a.vmlinux)
h = struct.unpack_from('<16sHHIQQQIHHHHHH', elf.data)
segments = [struct.unpack_from('<IIQQQQQQ', elf.data, h[5] + i * h[9]) for i in range(h[10])]
symtab = elf.by_name['.symtab']
s = elf.sections[symtab[6]]
names = elf.data[s[4]:s[4] + s[5]]
wanted = {'protected_symbol_exports', 'protected_symbol_exports_count',
          'permitted_symbol_imports', 'permitted_symbol_imports_count'}
symbols = {}
for pos in range(symtab[4], symtab[4] + symtab[5], symtab[9]):
    name, _, _, _, value, _ = struct.unpack_from('<IBBHQQ', elf.data, pos)
    name = module.Elf.string(names, name)
    if name in wanted: symbols[name] = value
assert symbols.keys() == wanted


def read_address(address, count):
    for seg in segments:
        if seg[0] == 1 and seg[3] <= address and address + count <= seg[3] + seg[5]:
            offset = seg[2] + address - seg[3]
            return elf.data[offset:offset + count]
    raise ValueError(hex(address))


def strings(name):
    count = struct.unpack('<Q', read_address(symbols[name + '_count'], 8))[0]
    pointers = struct.unpack('<' + 'Q' * count, read_address(symbols[name], count * 8))
    values = [module.Elf.string(read_address(ptr, 512), 0) for ptr in pointers]
    assert len(values) == len(set(values))
    return set(values)


def tsv(name):
    with (a.oracle / name).open() as f:
        return list(csv.DictReader(f, delimiter='\t'))


permitted, protected = strings('permitted_symbol_imports'), strings('protected_symbol_exports')
modules, exports, dependencies = tsv('module-contract.tsv'), tsv('provider-map.tsv'), tsv('module-dependencies.tsv')
assert len(modules) == 1070
assert {r['signing_contract'] for r in modules} == {'STOCK_TRUSTED', 'UNSIGNED_RETAINED'}
signed = {(r['partition'], r['module']): r['signing_contract'] == 'STOCK_TRUSTED' for r in modules}
assert sum(signed.values()) == 103
providers = {}
for r in exports:
    providers.setdefault(r['symbol'], []).append(r)
collisions = [r for r in exports if r['partition'] != 'vmlinux'
              and not signed[(r['partition'], r['provider'])] and r['symbol'] in protected]
core = {f[1]: f[0].lower() for line in a.symvers.read_text().splitlines() if (f := line.split())}
assert all(r['symbol'] == 'module_layout' for r in dependencies if r['binding'] == 'SPECIAL')


def audit(crcs, permit):
    mismatch, failure, precedence, unsigned_edges = [], [], 0, 0
    for r in dependencies:
        name, consumer = r['symbol'], (r['partition'], r['consumer'])
        if name in crcs:
            precedence += 1
            if r['expected_crc'] and crcs[name] != r['expected_crc'].lower(): mismatch.append(r)
            nonvendor = True
        else:
            pool = providers.get(name, [])
            nonvendor = bool(pool) and all(signed[(x['partition'], x['provider'])] for x in pool)
        if not signed[consumer] and r['binding'] != 'SPECIAL':
            unsigned_edges += 1
            if nonvendor and name not in permit: failure.append(r)
    return precedence, unsigned_edges, mismatch, failure


pc, uc, mismatch, failure = audit(core, permitted)
assert not collisions and not mismatch and not failure
bad = dict(core)
victim = next(r['symbol'] for r in dependencies if r['symbol'] in core and r['expected_crc'])
bad[victim] = '0x00000000' if core[victim] != '0x00000000' else '0xffffffff'
assert audit(bad, permitted)[2] and audit(core, set())[3]
result = {'result': 'PASS', 'stock_module_records': len(modules), 'vmlinux_exports': len(core),
          'compiled_permitted_imports': len(permitted), 'compiled_protected_exports': len(protected),
          'vmlinux_first_edges': pc, 'unsigned_edges_checked': uc,
          'production_provider_mismatches': len(mismatch), 'policy_failures': len(failure),
          'protected_export_collisions': len(collisions), 'negative_controls': 'PASS'}
a.out.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
