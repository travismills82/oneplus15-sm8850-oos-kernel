#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build one normally configured DDK target against frozen qualified outputs.

Does not run Bazel, build a kernel, sign/package modules, or assert runtime
compatibility. SDK tables must be real unmodified producer outputs; separately
audit every actual compiled import against the retained production providers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tarfile

FROZEN = Path('/home/travis/Android/kernel-test-backups/coloros1700102-6.12.114-five-cves-20261029')
QUALIFIED = Path('/home/travis/Android/stock_kernel/out/coloros1700102-612114-five-cves')
COMMON_TREE = '818ee1b0ff65d6d475afeeba55bf4099cfcbfc22'
SYMVERS_SHA = '6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1'
QUALIFIED_INPUT_HASHES = {
    'prepare_archive': '28cd2f46c9aed946aaa4ee0729fea01791c13e0e554e932813acf6a37e2620dd',
    'environment': '8c4e24504cadcde362a8b714c281c686459c863e685285cddb9b2000006c5243',
    'vmlinux_symvers': 'fa6d325e5e5ce232ca7622176829ce7c4bcf7cf7ea47070bf6f48b265da02efb',
    'localversion': '8c1bf64610fb6c40cad3b5236d33c243048834777601b8eb17dbbab6c86534e0',
}
SAFE_PATH = re.compile(r'^[A-Za-z0-9_./+:-]+$')
SOURCE_ROOTS = {'vendor', 'soc-repo', 'common-modules', 'qcom', 'oplus'}
TOOL_SOURCE_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as handle:
        while chunk := handle.read(1024 * 1024):
            h.update(chunk)
    return h.hexdigest()

def git(worktree, *args):
    return subprocess.check_output(['git', '-C', str(worktree), *args], text=True).strip()

def label_parts(label):
    match = re.fullmatch(r'//([A-Za-z0-9_./+-]+):([A-Za-z0-9_./+-]+)', label)
    if not match:
        raise ValueError('Expected a local //package:target DDK label')
    package, target = match.groups()
    if package != Path(package).as_posix() or target != Path(target).as_posix():
        raise ValueError('Non-canonical label path')
    if any(p in ('', '.', '..') for p in [*Path(package).parts, *Path(target).parts]) or package == 'common' or package.startswith('common/'):
        raise ValueError('Common-kernel changes require a separately planned kernel build')
    if Path(package).parts[0] not in SOURCE_ROOTS:
        raise ValueError('Unsupported DDK source root; refusing to write through a shared Bazel external tree')
    return package, target

def safe_path(value):
    path = Path(value).absolute().resolve(strict=False)
    if not SAFE_PATH.fullmatch(str(path)):
        raise ValueError('Kbuild input paths must not contain whitespace/shell metacharacters')
    return path

def snapshot_inputs(module):
    """Hash copied code/build definitions, not generated object files."""
    result = {}
    for path in module.rglob('*'):
        if path.is_file() and (path.suffix in {'.c', '.h', '.bzl', '.bazel', '.S', '.inc', '.defconfig'}
                               or path.name in {'Kconfig', 'Makefile', 'Kbuild'}
                               or path.name.startswith(('Kconfig.', 'Makefile.', 'build.config'))
                               or path.name.endswith(('.cflags_shipped', '.asflags_shipped'))):
            result[str(path.relative_to(module))] = digest(path)
    return result

def snapshot_closure(root, package, additional_trees):
    """Record every explicitly copied source tree, not only its DDK package."""
    return {tree: snapshot_inputs(root / tree)
            for tree in sorted({package, *additional_trees})}

def freeze_generated_flags(module, prepared_k, run, root):
    """Copy real Bazel-generated flag inputs; rewrite paths, never their bytes."""
    captured = {}
    for path in [*module.glob('*.cflags_shipped'), *module.glob('*.asflags_shipped')]:
        text = path.read_text()
        references = set(re.findall(r'(?:\$\(ROOT_DIR\)|' + re.escape(str(root)) + r')/(bazel-out/[A-Za-z0-9_./+-]+)', text))
        for relative in sorted(references, key=len, reverse=True):
            original = prepared_k / relative
            if not original.exists():
                forced = any(re.search(r'(?:-include|-imacros)\s+' + re.escape(prefix + relative) + r'(?:\s|$)', text)
                             for prefix in ['$(ROOT_DIR)/', str(root) + '/'])
                if forced:
                    raise ValueError(f'Missing normal generated forced header; build its producing target first: {original}')
                # Normal DDK includes source and generated -I alternatives;
                # an unused nonexistent search directory is not an input.
                continue
            copied = run / 'generated-inputs' / relative
            if original.is_dir():
                # A generated -I directory can contain complete runfiles/tool
                # trees. Never recursively copy it. Its actually consumed
                # headers are recorded from compiler depfiles after the build.
                continue
            copied.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(original, copied)
            captured[str(copied)] = digest(copied)
            text = text.replace('$(ROOT_DIR)/' + relative, str(copied))
            text = text.replace(str(root) + '/' + relative, str(copied))
        rewritten = text.replace('$(ROOT_DIR)', str(root))
        if rewritten != path.read_text():
            path.write_text(rewritten)
    return captured

def compiler_inputs(out, root, module_output=None):
    """Read actual compiler dependency lists; report unresolved names explicitly."""
    names = set()
    # modules_prepare includes historical host-tool .cmd files. They were not
    # compiled by this module run and must not be mislabelled actual inputs.
    commands = module_output if module_output is not None else out.parent
    for command in commands.rglob('*.cmd'):
        lines = command.read_text(errors='replace').splitlines()
        collecting = False
        for line in lines:
            if re.match(r'^deps_.*\s*:=', line):
                collecting = True
                continue
            if collecting and not line.strip():
                collecting = False
            if collecting:
                stripped = line.strip().removesuffix('\\').strip()
                if stripped and '$(' not in stripped and ' ' not in stripped:
                    names.add(stripped)
    hashes, unresolved = {}, []
    for name in sorted(names):
        options = [Path(name)] if Path(name).is_absolute() else [out / name, root / 'common' / name, root / name]
        found = next((p for p in options if p.is_file()), None)
        if found:
            hashes[str(found.absolute())] = digest(found)
        else:
            unresolved.append(name)
    return {'command_scope': str(commands), 'actual_dependency_sha256': hashes,
            'unresolved_dependency_names': unresolved}

def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-worktree', type=safe_path, required=True)
    p.add_argument('--prepared-worktree', type=safe_path, help='normal prepared Bazel config/makefile workspace; defaults to source')
    p.add_argument('--expected-head', required=True)
    p.add_argument('--label', required=True)
    p.add_argument('--module-out', action='append', required=True, help='exact expected .ko path relative to the DDK package')
    p.add_argument('--run-dir', type=safe_path, required=True, help='fresh private output directory outside source')
    p.add_argument('--extra-symvers', type=safe_path, action='append', default=[])
    p.add_argument('--freeze-source-tree', action='append', default=[], help='also copy a package/dependency source subtree, e.g. vendor/qcom/opensource/wlan')
    p.add_argument('--jobs', type=int, default=4)
    p.add_argument('--plan-only', action='store_true', help='validate inputs and print plan without writes/compilation')
    return p

def plan(args):
    source = args.source_worktree
    prepared = args.prepared_worktree or source
    package, target = label_parts(args.label)
    if not re.fullmatch(r'[0-9a-f]{40}', args.expected_head) or git(source, 'rev-parse', 'HEAD') != args.expected_head:
        raise ValueError('Source HEAD differs from the explicitly reviewed revision')
    if git(source, 'rev-parse', 'HEAD:kernel_platform/common') != COMMON_TREE or git(source, 'diff', 'HEAD', '--', 'kernel_platform/common'):
        raise ValueError('Common source/config differs from frozen kernel source: separate kernel build required')
    if git(prepared, 'rev-parse', 'HEAD:kernel_platform/common') != COMMON_TREE or git(prepared, 'diff', 'HEAD', '--', 'kernel_platform/common'):
        raise ValueError('Preparation workspace common tree differs from the frozen SDK')
    if not 1 <= args.jobs <= 16:
        raise ValueError('jobs must be1..16; coordinate parallel module builds')
    if args.run_dir.exists() or args.run_dir.is_symlink():
        raise ValueError('Refusing to overwrite an existing run directory')
    if args.run_dir.is_relative_to(source) or args.run_dir.is_relative_to(prepared):
        raise ValueError('Run directory must be outside source worktrees')
    for out in args.module_out:
        if Path(out).is_absolute() or '..' in Path(out).parts or out != Path(out).as_posix() or not re.fullmatch(r'[A-Za-z0-9_./+-]+\.ko', out):
            raise ValueError('Invalid relative module output')
    if len(args.module_out) != len(set(args.module_out)):
        raise ValueError('Duplicate module output')
    for tree in args.freeze_source_tree:
        if Path(tree).is_absolute() or '..' in Path(tree).parts or tree != Path(tree).as_posix() or not re.fullmatch(r'[A-Za-z0-9_./+-]+', tree) or not Path(tree).parts or Path(tree).parts[0] not in SOURCE_ROOTS:
            raise ValueError('Invalid source freeze subtree')
        if not (source / 'kernel_platform' / tree).is_dir():
            raise ValueError('Requested source freeze subtree does not exist')
    if source != prepared:
        # Reusing normal preparation for another worktree is valid only when
        # its build/config definitions agree. Body/header changes still need
        # the caller's ABI review; this is not a runtime compatibility gate.
        for tree in {package, *args.freeze_source_tree}:
            base = source / 'kernel_platform' / tree
            for path in base.rglob('*'):
                if path.is_file() and (path.suffix in {'.bzl', '.bazel', '.defconfig'} or path.name in {'Kconfig', 'Makefile', 'Kbuild'} or path.name.startswith('build.config')):
                    counterpart = prepared / 'kernel_platform' / tree / path.relative_to(base)
                    if not counterpart.is_file() or path.read_bytes() != counterpart.read_bytes():
                        raise ValueError(f'Independent worktree needs its own normal preparation: {path}')
    k = prepared / 'kernel_platform'
    generated = k / 'bazel-bin' / package / (target + '_makefiles/makefiles')
    config = k / 'bazel-bin' / package / (target + '_config/out_dir')
    inputs = {'frozen_symvers': FROZEN / 'production/Module.symvers',
              'prepare_archive': QUALIFIED / 'kernel_platform/bazel-bin/common/kernel_aarch64_modules_prepare/modules_prepare_outdir.tar.gz',
              'environment': QUALIFIED / 'kernel_platform/bazel-bin/common/kernel_aarch64_env.sh',
              'vmlinux_symvers': QUALIFIED / 'kernel_platform/bazel-bin/common/kernel_aarch64/vmlinux.symvers',
              'localversion': QUALIFIED / 'kernel_platform/bazel-bin/common/kernel_aarch64_config/out_dir/localversion',
              'ddk_config': config / '.config', 'ddk_makefile': generated / 'Makefile'}
    for path in inputs.values():
        if not path.is_file():
            raise ValueError(f'Missing normal prepared input: {path}')
    if digest(inputs['frozen_symvers']) != SYMVERS_SHA:
        raise ValueError('Frozen qualified Module.symvers changed')
    for name, expected in QUALIFIED_INPUT_HASHES.items():
        if digest(inputs[name]) != expected:
            raise ValueError(f'Qualified prebuilt input changed: {name}')
    for path in args.extra_symvers:
        if not path.is_file() or not path.name.endswith('Module.symvers'):
            raise ValueError(f'SDK input must be an existing real producer Module.symvers: {path}')
    hermetic = list((k / 'bazel-out').glob('*/bin/build/kernel/hermetic-tools'))
    hermetic = [p for p in hermetic if p.is_dir()]
    if len(hermetic) != 1:
        raise ValueError(f'Expected one prepared hermetic-tools directory, found{len(hermetic)}')
    return {'source': source, 'prepared': prepared, 'package': package, 'target': target,
            'generated': generated, 'config': config, 'inputs': inputs,
            'hermetic': str(hermetic[0].relative_to(k)), 'run': args.run_dir}

def execute(args, p):
    run = p['run']; sandbox = run / 'workspace/build'
    root = sandbox / 'root'; out = sandbox / 'kernel-output/common'
    root.mkdir(parents=True); out.mkdir(parents=True)
    source_k = p['source'] / 'kernel_platform'; prepared_k = p['prepared'] / 'kernel_platform'
    (root / 'common').mkdir()
    for item in (source_k / 'common').iterdir():
        (root / 'common' / item.name).symlink_to(item)
    package = p['package']
    def reference(relative):
        original, destination = source_k / relative, root / relative
        if relative == package or relative in args.freeze_source_tree:
            shutil.copytree(original, destination, symlinks=False)
        elif package.startswith(relative + '/') or any(tree.startswith(relative + '/') for tree in args.freeze_source_tree):
            destination.mkdir()
            for child in original.iterdir():
                reference(relative + '/' + child.name)
        else:
            destination.symlink_to(original)
    for name in ['vendor', 'soc-repo', 'common-modules', 'qcom', 'oplus']:
        if (source_k / name).exists():
            reference(name)
    for name in ['prebuilts', 'build', 'bazel-out']:
        (root / name).symlink_to(prepared_k / name)
    execroot = (prepared_k / 'bazel-out').resolve().parent
    (root / 'external').symlink_to(execroot / 'external')
    # Host gendwarfksyms RPATH is relative to the private output's ancestor.
    sdk_prebuilts = run / 'prebuilts'
    if sdk_prebuilts.is_symlink():
        if sdk_prebuilts.resolve() != (prepared_k / 'prebuilts').resolve():
            raise ValueError('Sibling prebuilts points to another toolchain')
    elif sdk_prebuilts.exists():
        raise ValueError('Unexpected sibling prebuilts path')
    else:
        sdk_prebuilts.symlink_to(prepared_k / 'prebuilts')
    module = root / package
    shutil.copytree(p['generated'], module, dirs_exist_ok=True)
    with tarfile.open(p['inputs']['prepare_archive']) as archive:
        archive.extractall(out, filter='data')
    shutil.copy2(p['config'] / '.config', out / '.config')
    shutil.copytree(p['config'] / 'include', out / 'include', dirs_exist_ok=True)
    for key, destination in [('localversion', 'localversion'), ('frozen_symvers', 'Module.symvers'), ('vmlinux_symvers', 'vmlinux.symvers')]:
        shutil.copy2(p['inputs'][key], out / destination)
    auto = out / 'include/config/auto.conf.cmd'
    auto.write_text(auto.read_text().replace('${ROOT_DIR}', str(root)))
    generated_inputs = freeze_generated_flags(module, prepared_k, run, root)
    copied_inputs = snapshot_inputs(module)
    (run / 'copied-source-inputs.json').write_text(json.dumps(copied_inputs, indent=2) + '\n')
    copied_closure = snapshot_closure(root, package, args.freeze_source_tree)
    (run / 'copied-source-closure.json').write_text(json.dumps(copied_closure, indent=2) + '\n')
    hashes = {str(v): digest(v) for v in p['inputs'].values()}
    hashes.update({str(v): digest(v) for v in args.extra_symvers})
    manifest = {'source_head': args.expected_head, 'label': args.label, 'module_outputs': args.module_out,
                'builder_source_sha256': TOOL_SOURCE_SHA256,
                'common_tree': COMMON_TREE, 'input_sha256': hashes, 'source_status': git(p['source'], 'status', '--short'),
                'configuration_inputs_edited': False, 'kernel_Image_built': False,
                'copied_source_inputs': len(copied_inputs), 'generated_input_sha256': generated_inputs,
                'freeze_source_trees': args.freeze_source_tree,
                'source_worktree': str(p['source']), 'prepared_worktree': str(p['prepared']),
                'module_output_directory': str(sandbox / 'kernel-output' / package),
                'runtime_compatibility': 'NOT CLAIMED; audit actual imports/private ABI/load policy separately'}
    (run / 'input-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    q = shlex.quote
    script = f'''#!/bin/bash
set -euo pipefail
cd {q(str(root))}
export BUILD_WORKSPACE_DIRECTORY= BAZEL_TEST=
export KLEAF_HERMETIC_BASE={q(p['hermetic'])}
set +u
source {q(str(p['inputs']['environment']))}
set -u
export ROOT_DIR={q(str(root))} OUT_DIR={q(str(out))} COMMON_OUT_DIR={q(str(run))}
export KCONFIG_EXT_PREFIX=$(sed -n 's/^ifneq "$(KCONFIG_EXT_PREFIX)" "\\(.*\\)"/\\1/p' {q(str(auto))})
export MAKEFLAGS=-j{args.jobs}
export KCPPFLAGS={q('-ffile-prefix-map=' + str(root) + '=/proc/self/cwd -ffile-prefix-map=' + str(source_k) + '=/proc/self/cwd')}
export KBUILD_EXTRA_SYMBOLS={q(' '.join(map(str,args.extra_symvers)))}
clang --version
make -C {q(str(root / 'common'))} O={q(str(out))} M={q('../' + package)} LLVM=1 KERNEL_SRC={q(str(root / 'common'))} VPATH={q(str(root / 'common'))} modules
'''
    (run / 'build.sh').write_text(script)
    with (run / 'build.log').open('w') as log:
        result = subprocess.run(['bash', str(run / 'build.sh')], stdout=log, stderr=subprocess.STDOUT)
    manifest['build_exit'] = result.returncode
    (run / 'compiler-inputs.json').write_text(json.dumps(compiler_inputs(out, root, sandbox / 'kernel-output' / package), indent=2) + '\n')
    if snapshot_inputs(module) != copied_inputs:
        raise ValueError('Copied source/generated build inputs changed during compilation')
    if snapshot_closure(root, package, args.freeze_source_tree) != copied_closure:
        raise ValueError('Explicitly frozen source dependency closure changed during compilation')
    if result.returncode == 0:
        built = sandbox / 'kernel-output' / package
        actual = sorted(str(path.relative_to(built)) for path in built.rglob('*.ko'))
        if actual != sorted(args.module_out):
            raise ValueError(f'Unexpected module output population: {actual}')
        manifest['output_sha256'] = {name: digest(built / name) for name in actual}
        manifest['output_symvers_sha256'] = digest(built / 'Module.symvers')
    (run / 'result.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))
    if result.returncode:
        raise SystemExit(result.returncode)

def main():
    args = parser().parse_args()
    try:
        p = plan(args)
        if args.plan_only:
            print(json.dumps({k: {n: str(v) for n,v in value.items()} if isinstance(value,dict) else str(value) for k,value in p.items()}, indent=2))
        else:
            execute(args, p)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(f'FAIL-CLOSED: {error}') from error

if __name__ == '__main__':
    main()
