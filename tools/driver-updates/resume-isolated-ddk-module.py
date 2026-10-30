#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Retry a failed isolated control using its frozen source, genuine SDKs only.

Preserves original scripts/logs/results. Never imports edited live source or
changes CRC tables, source code, production configs, or a successful module.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

spec = importlib.util.spec_from_file_location('isolated_ddk_support', Path(__file__).with_name('isolated-ddk-module-build.py'))
support = importlib.util.module_from_spec(spec)
spec.loader.exec_module(support)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run-dir', type=Path, required=True)
    p.add_argument('--attempt', required=True)
    p.add_argument('--extra-symvers', type=support.safe_path, action='append', default=[])
    p.add_argument('--jobs', type=int, default=2)
    a = p.parse_args()
    if not re.fullmatch(r'[a-z0-9-]+', a.attempt) or not 1 <= a.jobs <= 16:
        raise SystemExit('Invalid attempt/jobs')
    run = a.run_dir.resolve(); manifest = json.loads((run / 'input-manifest.json').read_text())
    package, _ = support.label_parts(manifest['label'])
    root = run / 'workspace/build/root'; out = run / 'workspace/build/kernel-output/common'
    module = root / package; built = out.parent / package
    if any((built / name).exists() for name in manifest['module_outputs']):
        raise SystemExit('Refusing to overwrite any completed module; use a fresh candidate run')
    prefix = run / ('attempt-' + a.attempt)
    script_path = prefix.with_suffix('.sh')
    if script_path.exists():
        raise SystemExit('Attempt already exists')
    old = (run / 'build.sh').read_text()
    for path in a.extra_symvers:
        if not path.is_file() or not path.name.endswith('Module.symvers'):
            raise SystemExit('Expected genuine producer Module.symvers: ' + str(path))
    # Freeze actual normal generated headers before retrying, without changing
    # their content. Preserve previous generated flags for forensic comparison.
    before = {str(f.relative_to(module)): f.read_text() for f in [*module.glob('*.cflags_shipped'), *module.glob('*.asflags_shipped')]}
    prefix.with_name(prefix.name + '-original-flags.json').write_text(json.dumps(before, indent=2) + '\n')
    generated = support.freeze_generated_flags(module, Path(manifest['prepared_worktree']) / 'kernel_platform', run, root)
    source = support.snapshot_inputs(module)
    closure = support.snapshot_closure(root, package, manifest.get('freeze_source_trees', []))
    prefix.with_name(prefix.name + '-copied-source.json').write_text(json.dumps(source, indent=2) + '\n')
    old = re.sub(r'^export KBUILD_EXTRA_SYMBOLS=.*$', 'export KBUILD_EXTRA_SYMBOLS=' + shlex.quote(' '.join(map(str,a.extra_symvers))), old, flags=re.M)
    old = re.sub(r'^export MAKEFLAGS=.*$', f'export MAKEFLAGS=-j{a.jobs}', old, flags=re.M)
    script_path.write_text(old)
    with prefix.with_suffix('.log').open('w') as log:
        result = subprocess.run(['bash', str(script_path)], stdout=log, stderr=subprocess.STDOUT)
    final = {'build_exit': result.returncode, 'label': manifest['label'], 'frozen_source_head': manifest['source_head'],
             'source_reloaded_from_live_worktree': False, 'SDK_sha256': {str(f):support.digest(f) for f in a.extra_symvers},
             'generated_inputs_sha256': generated, 'module_output_directory': str(built),
             'source_inputs_unchanged_during_retry': support.snapshot_inputs(module) == source,
             'source_closure_unchanged_during_retry': support.snapshot_closure(root, package, manifest.get('freeze_source_trees', [])) == closure,
             'compiler_inputs': support.compiler_inputs(out,root,built), 'runtime_compatibility': 'NOT CLAIMED'}
    if not final['source_inputs_unchanged_during_retry']:
        raise SystemExit('Frozen source changed during retry')
    if not final['source_closure_unchanged_during_retry']:
        raise SystemExit('Frozen source dependency closure changed during retry')
    if not result.returncode:
        final['output_sha256'] = {name:support.digest(built / name) for name in manifest['module_outputs']}
        final['output_symvers_sha256'] = support.digest(built / 'Module.symvers')
    prefix.with_suffix('.json').write_text(json.dumps(final, indent=2) + '\n')
    print(json.dumps(final, indent=2))
    raise SystemExit(result.returncode)

if __name__ == '__main__':
    main()
