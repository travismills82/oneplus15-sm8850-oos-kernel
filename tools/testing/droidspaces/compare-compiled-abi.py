#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compare existing export records, KCFI IDs and compiled BTF member layouts."""
import argparse
import json
import mmap
from pathlib import Path
import struct


class Elf:
    def __init__(self, path):
        self.stream = path.open('rb')
        self.data = mmap.mmap(self.stream.fileno(), 0, access=mmap.ACCESS_READ)
        h = struct.unpack_from('<16sHHIQQQIHHHHHH', self.data)
        assert h[0][:6] == b'\x7fELF\x02\x01' and h[2] == 183
        self.sections = [struct.unpack_from('<IIQQQQIIQQ', self.data, h[6] + i * h[11])
                         for i in range(h[12])]
        strings = self.sections[h[13]]
        names = self.data[strings[4]:strings[4] + strings[5]]
        self.by_name = {self.string(names, s[0]): s for s in self.sections}

    @staticmethod
    def string(data, offset):
        return data[offset:data.find(b'\0', offset)].decode()

    def section(self, name):
        s = self.by_name[name]
        return self.data[s[4]:s[4] + s[5]]

    def kcfi(self):
        syms = self.by_name['.symtab']
        s = self.sections[syms[6]]
        names = self.data[s[4]:s[4] + s[5]]
        result = {}
        for pos in range(syms[4], syms[4] + syms[5], syms[9]):
            name, _, _, _, value, _ = struct.unpack_from('<IBBHQQ', self.data, pos)
            name = self.string(names, name)
            if name.startswith('__kcfi_typeid_'):
                result[name[len('__kcfi_typeid_'):]] = value
        return result


class Btf:
    def __init__(self, elf):
        data = elf.section('.BTF')
        magic, version, _, length, off, size, soff, ssize = struct.unpack_from('<HBBIIIII', data)
        assert magic == 0xeb9f and version == 1
        self.strings = data[length + soff:length + soff + ssize]
        self.types = [None]
        pos, end = length + off, length + off + size
        while pos < end:
            name, info, value = struct.unpack_from('<III', data, pos)
            pos += 12
            kind, n, flag = (info >> 24) & 31, info & 0xffff, info >> 31
            t = {'name': Elf.string(self.strings, name), 'kind': kind, 'value': value}
            extra = {1: 4, 3: 12, 4: 12 * n, 5: 12 * n, 6: 8 * n,
                     13: 8 * n, 14: 4, 15: 12 * n, 17: 4, 19: 12 * n}.get(kind, 0)
            if kind in [4, 5]:
                t['members'] = []
                for i in range(n):
                    mname, tid, bits = struct.unpack_from('<III', data, pos + i * 12)
                    width = bits >> 24 if flag else 0
                    bits = bits & 0xffffff if flag else bits
                    t['members'].append((Elf.string(self.strings, mname), tid, bits, width))
            elif kind == 3:
                t['array'] = struct.unpack_from('<III', data, pos)
            self.types.append(t)
            pos += extra
        assert pos == end

    def resolve(self, tid):
        while tid and self.types[tid]['kind'] in [8, 9, 10, 11, 18]:
            tid = self.types[tid]['value']
        return tid

    def size(self, tid):
        tid = self.resolve(tid)
        if not tid: return 0
        t = self.types[tid]
        if t['kind'] == 2: return 8
        if t['kind'] == 3: return self.size(t['array'][0]) * t['array'][2]
        return t['value'] if t['kind'] in [1, 4, 5, 6, 16, 19] else 0

    def identity(self, tid):
        tid = self.resolve(tid)
        if not tid: return 'void'
        t = self.types[tid]
        if t['kind'] == 2: return 'ptr:' + self.identity(t['value'])
        if t['kind'] == 3: return f"array[{t['array'][2]}]:" + self.identity(t['array'][0])
        return str(t['kind']) + ':' + t['name']

    def layout(self, name):
        candidates = [i for i, t in enumerate(self.types) if t and t['kind'] == 4 and t['name'] == name]
        assert candidates, name
        tid = max(candidates, key=lambda i: self.types[i]['value'])
        result = {}

        def fields(index, base):
            t = self.types[self.resolve(index)]
            for member, typ, offset, width in t['members']:
                typ = self.resolve(typ)
                if not member and self.types[typ]['kind'] in [4, 5]:
                    fields(typ, base + offset)
                elif member:
                    record = [base + offset, width, self.size(typ), self.identity(typ)]
                    if member in result: assert result[member] == record, member
                    result[member] = record
        fields(tid, 0)
        return {'size': self.types[tid]['value'], 'members': result}


def symvers(path):
    records = {}
    for line in path.read_text().splitlines():
        f = line.split()
        if f: records[(f[1], f[2])] = [f[0]] + f[3:]
    return records


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--candidate', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    old, new = symvers(a.baseline / 'Module.symvers'), symvers(a.candidate / 'Module.symvers')
    changes = [{'symbol': k[0], 'provider': k[1], 'old': v, 'new': new.get(k)}
               for k, v in old.items() if new.get(k) != v]
    before, after = Elf(a.baseline / 'vmlinux'), Elf(a.candidate / 'vmlinux')
    ob, nb = Btf(before), Btf(after)
    layouts, errors = {}, []
    roots = ['task_struct', 'thread_info', 'cred', 'inode', 'file', 'file_operations',
             'super_block', 'vfsmount', 'mount', 'address_space', 'vm_area_struct',
             'mm_struct', 'nsproxy', 'pid_namespace', 'user_namespace', 'signal_struct', 'sighand_struct']
    for root in roots:
        old_layout, new_layout = ob.layout(root), nb.layout(root)
        changed = {k: [v, new_layout['members'].get(k)]
                   for k, v in old_layout['members'].items() if new_layout['members'].get(k) != v}
        added = {k: v for k, v in new_layout['members'].items() if k not in old_layout['members']}
        layouts[root] = {'old_size': old_layout['size'], 'new_size': new_layout['size'],
                         'changed_existing_members': changed, 'new_members': added}
        if old_layout['size'] != new_layout['size'] or changed: errors.append(root)
        if root == 'task_struct':
            assert new_layout['members']['sysvsem'][0] == 168 * 8
            assert new_layout['members']['sysvshm'][0] == 176 * 8
            assert new_layout['members']['se'][0] == 192 * 8
    ok, nk = before.kcfi(), after.kcfi()
    kcfi_changes = {k: [v, nk[k]] for k, v in ok.items() if k in nk and nk[k] != v}
    additions = [{'symbol': key[0], 'provider': key[1], 'record': new[key]}
                 for key in sorted(new.keys() - old.keys())]
    report = {'existing_export_records': len(old), 'export_changes': changes,
              'new_export_records': len(additions), 'new_exports': additions, 'layouts': layouts,
              'layout_failures': errors, 'shared_kcfi_records': len(ok.keys() & nk.keys()),
              'changed_kcfi_records': kcfi_changes,
              'result': 'PASS' if not changes and not errors and not kcfi_changes else 'FAIL'}
    a.out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'layouts'}, indent=2))
    raise SystemExit(report['result'] != 'PASS')


if __name__ == '__main__':
    main()
