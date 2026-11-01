#!/usr/bin/env python3
"""Offline diagnostic policy. Never writes to a source population or device.

Package vendor_dlkm without GEAS; preserve vendor_boot bytes. The separate
oracle projection excludes dormant vendor_boot GEAS only because the boot
command line's standard kernel module_blacklist denies those exact names.
No CRC, ELF metadata, signature, or kernel-loader modification is performed.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
POLICY = json.loads((HERE / 'coloros17-no-geas-policy.json').read_text())
DISABLED = frozenset(POLICY['disabled_canonical_names'])


def sha(path):
    with path.open('rb') as handle:
        return hashlib.file_digest(handle, 'sha256').hexdigest()


def command_line(original):
    """Merge a single standard parameter, preserving unrelated blacklists."""
    words = original.split()
    keys = [w for w in words if w.startswith('module_blacklist=')]
    if len(keys) > 1:
        raise ValueError('Ambiguous duplicate module_blacklist parameters')
    names = set(keys[0].partition('=')[2].split(',')) if keys else set()
    names.discard('')
    words = [w for w in words if not w.startswith('module_blacklist=')]
    return ' '.join(words + ['module_blacklist=' + ','.join(sorted(names | DISABLED))])


def filter_metadata(text, kind):
    result = []
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            result.append(line)
            continue
        tokens = stripped.replace(':', ' ').split()
        names = [Path(t).name.removesuffix('.ko').replace('-', '_') for t in tokens]
        if kind == 'modules.dep':
            if names[0] in DISABLED:
                continue
            if DISABLED & set(names[1:]):
                raise ValueError('Retained dependency on excluded GEAS: ' + line)
        elif kind.startswith('modules.load'):
            if names[0] in DISABLED:
                continue
        elif DISABLED & set(names):
            raise ValueError('Unreviewed GEAS reference in ' + kind + ': ' + line)
        result.append(line)
    return ''.join(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--population', type=Path, required=True)
    parser.add_argument('--gpu', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stock-command-line', required=True)
    args = parser.parse_args()
    if args.out.exists():
        raise ValueError('Output already exists; refusing to overwrite')
    spec = importlib.util.spec_from_file_location('no_geas_elf', ROOT / 'tools/validate-matched-wlan-vendor-dlkm.py')
    elf = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = elf
    spec.loader.exec_module(elf)
    modules = {}
    removed_exports = set()
    for path in sorted(args.population.rglob('*.ko')):
        module = elf.ElfModule(path)
        name = module.modinfo()['name'][0]
        modules[path] = (name, module)
        if name in DISABLED:
            removed_exports |= module.exported_symbols().keys()
    affected = [(p, m) for p, (n, m) in modules.items() if n in DISABLED]
    if len(modules) != 1070 or len(affected) != 6:
        raise ValueError('Population differs from audited 1070 / six GEAS locations')
    for path, (name, module) in modules.items():
        if name in DISABLED:
            continue
        deps = {n.replace('-', '_') for raw in module.modinfo().get('depends', []) for n in raw.split(',')}
        imports = {s.name for s in module.undefined_symbols()} | module.versions().keys()
        if deps & DISABLED or imports & removed_exports:
            raise ValueError('Reverse GEAS dependency: ' + str(path))
    gpu = elf.ElfModule(args.gpu)
    if gpu.modinfo().get('name') != ['msm_kgsl'] or 'get_gpu_bw_data' in gpu.exported_symbols():
        raise ValueError('Expected genuine updated GPU without unaccepted GEAS getter')
    package = args.out / 'packaged-population'
    shutil.copytree(args.population, package)
    rollback = args.out / 'original-geas'
    changed = []
    for path, module in affected:
        rel = path.relative_to(args.population)
        backup = rollback / rel
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, backup)
        assert sha(path) == sha(backup)
        if rel.parts[0] == 'vendor_dlkm':
            (package / rel).unlink()
            changed.append({'path': str(rel), 'action': 'excluded', 'original_sha256': sha(path)})
        elif rel.parts[0] != 'vendor_boot':
            raise ValueError('Unaudited GEAS partition')
    dest = package / 'vendor_dlkm/lib/modules/msm_kgsl.ko'
    shutil.copy2(args.gpu, dest)
    changed.append({'path': str(dest.relative_to(package)), 'action': 'GPU replacement', 'sha256': sha(dest)})
    for path in sorted((package / 'vendor_dlkm/lib/modules').glob('modules.*')):
        old = path.read_text()
        new = filter_metadata(old, path.name)
        if path.name == 'modules.blocklist':
            new += '\n# Diagnostic ColorOS 17 no-GEAS GPU experiment\n'
            new += ''.join('blocklist ' + n + '\n' for n in sorted(DISABLED))
        if new != old:
            path.write_text(new)
            changed.append({'path': str(path.relative_to(package)), 'action': 'load/dependency policy', 'sha256': sha(path)})
    projection = args.out / 'enabled-oracle-population'
    shutil.copytree(package, projection)
    for name in DISABLED:
        (projection / 'vendor_boot/lib/modules' / (name + '.ko')).unlink()
    for part in ('vendor_boot', 'system_dlkm'):
        for path in (args.population / part).rglob('*'):
            if path.is_file():
                assert sha(path) == sha(package / path.relative_to(args.population))
    result = {'policy': POLICY, 'boot_command_line': command_line(args.stock_command_line),
              'source_population': str(args.population), 'packaged_population': str(package),
              'enabled_oracle_population': str(projection), 'changes': changed,
              'physical_module_files': len(list(package.rglob('*.ko'))),
              'enabled_module_records': len(list(projection.rglob('*.ko'))),
              'excluded_canonical_modules': sorted(DISABLED), 'excluded_records': 6,
              'vendor_boot_unmodified': True, 'system_dlkm_unmodified': True,
              'runtime': 'NOT TESTED; standard module_blacklist required in the actual boot image'}
    (args.out / 'policy-manifest.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
