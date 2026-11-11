#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check that discarded ABI annotations leave Rust runtime/KCFI bytes intact."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

spec = importlib.util.spec_from_file_location('abi', Path(__file__).parents[1] / 'droidspaces/compare-compiled-abi.py')
abi = importlib.util.module_from_spec(spec)
spec.loader.exec_module(abi)


def allocated(elf):
    return {n: hashlib.sha256(elf.data[x[4]:x[4] + x[5]]).hexdigest()
            for n, x in elf.by_name.items() if x[2] & 2 and x[1] != 8 and not n.startswith('.discard')}


def symbols(elf):
    sec = elf.by_name['.symtab']
    strings = elf.sections[sec[6]]
    names = elf.data[strings[4]:strings[4] + strings[5]]
    section_names = {v: k for k, v in elf.by_name.items()}
    result = []
    for pos in range(sec[4], sec[4] + sec[5], sec[9]):
        n, info, other, index, value, size = struct.unpack_from('<IBBHQQ', elf.data, pos)
        section = section_names[elf.sections[index]] if 0 < index < len(elf.sections) else index
        result.append((abi.Elf.string(names, n), info, other, section, value, size))
    return result


def relocations(elf, syms):
    result = []
    for name, section in elf.by_name.items():
        if section[1] != 4:
            continue
        for pos in range(section[4], section[4] + section[5], section[9]):
            offset, info, addend = struct.unpack_from('<QQq', elf.data, pos)
            result.append((name, offset, info & 0xffffffff, syms[info >> 32], addend))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    rows = []
    for crate in ['core', 'bindings']:
        before, after = abi.Elf(a.before / (crate + '.o')), abi.Elf(a.after / (crate + '.o'))
        old, new = symbols(before), symbols(after)
        row = {'crate': crate, 'allocated_sections_and_KCFI_identical': allocated(before) == allocated(after),
               'symbol_semantics_identical': set(old) == set(new),
               'relocations_identical': relocations(before, old) == relocations(after, new)}
        rows.append(row)
    result = {'result': 'PASS' if all(all(v for k, v in row.items() if k != 'crate') for row in rows) else 'FAIL',
              'objects': rows, 'scope': 'Metadata-only change; not a comparison against legacy merged KCFI hashes'}
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(result['result'] != 'PASS')


if __name__ == '__main__':
    main()
