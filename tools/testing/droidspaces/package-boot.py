#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline boot-only package after real compiled, oracle and VM evidence gates.

Never flashes, rebuilds, commits, modifies an ABI reference, or overwrites a
frozen candidate. The AVB contract is copied from the exact qualified image.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, required=True)
p.add_argument('--evidence', type=Path, required=True)
p.add_argument('--baseline', type=Path, required=True)
p.add_argument('--tools-workspace', type=Path, required=True)
a = p.parse_args()
S = Path(__file__).resolve().parents[3]
B, E, BASE, H = a.build.resolve(), a.evidence.resolve(), a.baseline.resolve(), a.tools_workspace.resolve()
T = H / 'prebuilts/kernel-build-tools/linux-x86/bin/avbtool'
KEY = H / 'tools/mkbootimg/gki/testdata/testkey_rsa4096.pem'
commands = []


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def run(name, argv):
    argv = [str(x) for x in argv]
    result = subprocess.run(argv, capture_output=True, text=True)
    (E / (name + '.log')).write_text(result.stdout + result.stderr)
    commands.append({'name': name, 'argv': argv, 'exit': result.returncode})
    assert result.returncode == 0, (name, result.stderr[-2000:])
    return result.stdout


def passed_json(name):
    value = json.loads((E / name).read_text())
    assert value['result'] == 'PASS', name
    return value


assert sha(BASE) == '3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5'
assert 'Build completed successfully' in (E / 'common-build-06.log').read_text()
abi_result = passed_json('compiled-abi-final.json')
config_result = passed_json('config-audit.json')
provider = passed_json('provider-policy.json')
assert not abi_result['export_changes'] and not abi_result['layout_failures']
assert not abi_result['changed_kcfi_records']
summary = dict(line.split('=', 1) for line in
               (E / 'stock-oracle/module-summary.txt').read_text().splitlines() if '=' in line)
for key, value in {'compatible': '1032', 'dormant': '38', 'blocker': '0',
                   'hard_required': '60701', 'crc_matches': '61777', 'missing': '0',
                   'crc_mismatches': '0', 'namespace_failures': '0', 'result': 'PASS'}.items():
    assert summary[key] == value, (key, summary.get(key))
for name, count in [('driver-host-tests-final.log', 202), ('source-guards.log', 9),
                    ('kernelsu-source-guards.log', 10)]:
    log = (E / name).read_text()
    assert f'Ran {count} tests' in log and log.rstrip().endswith('OK'), name
vm = (E / 'vm/candidate-final.log').read_text()
vm_inputs = json.loads(vm.splitlines()[0].removeprefix('DROIDSPACES_VM_INPUTS '))
assert vm_inputs['kernel_sha256'] == sha(B / 'arch/arm64/boot/Image')
assert vm_inputs['initramfs_sha256'] == sha(E / 'vm/initramfs.cpio')
checker = next(json.loads(line.strip()) for line in vm.splitlines() if line.startswith('{"root":'))
assert checker['requirements_met'] == 1
for marker in ['NAMESPACE_SMOKE_RC=0', 'SYSV_SEM_UNDO_100_FORKS=PASS',
               'SYSV_SHM_FORK_CLEANUP=PASS', 'SECCOMP_FILTER_OPERATION=PASS',
               'EXT4_LOOP_REMOUNT=PASS', 'OVERLAY_COPYUP=PASS', 'DEVTMPFS_MOUNT=PASS',
               'DROIDSPACES_VM_END']:
    assert marker in vm, marker
assert not re.search(r'SMOKE_FAIL|Kernel panic|BUG:|WARNING:|Oops:', vm)
assert (E / 'enforced-gki-final.log').is_file()
gki_delta = json.loads((E / 'enforced-gki-delta.json').read_text())
assert gki_delta['baseline_owner_count'] == 44 and gki_delta['candidate_owner_count'] == 45
assert not gki_delta['removed_owners'] and not gki_delta['changed_prior_owners']
assert gki_delta['delta_review'].startswith('REVIEWED C-PRIVATE IPC ADDITION;')
for name, argv in [
    ('clang-version', [H / 'prebuilts/clang/host/linux-x86/clang-r536225/bin/clang', '--version']),
    ('linker-version', [H / 'prebuilts/clang/host/linux-x86/clang-r536225/bin/ld.lld', '--version']),
    ('rustc-version', [H / 'prebuilts/rust/linux-x86/1.82.0.p2/bin/rustc', '--version']),
    ('bindgen-version', [H / 'prebuilts/clang-tools/linux-x86/bin/bindgen', '--version']),
]:
    run(name, argv)
head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=S, text=True).strip()
assert head == 'd6552d3625e0656aeac303b878347bef208b4315'
common_files = ['arch/arm64/configs/gki_defconfig', 'include/linux/sched.h',
                'ipc/msgutil.c', 'ipc/namespace.c', 'rust/Makefile']
paths = ['kernel_platform/common/' + path for path in common_files]
patch = subprocess.check_output(['git', 'diff', '--binary', 'HEAD', '--', *paths], cwd=S)
(E / 'kernel-source.patch').write_bytes(patch)
with tempfile.TemporaryDirectory(prefix='ds-source-index-', dir=E) as temp:
    env = dict(os.environ, GIT_INDEX_FILE=str(Path(temp) / 'index'))
    subprocess.run(['git', 'read-tree', 'HEAD'], cwd=S, env=env, check=True)
    subprocess.run(['git', 'add', '--', *paths], cwd=S, env=env, check=True)
    tree = subprocess.check_output(['git', 'write-tree'], cwd=S, env=env, text=True).strip()
    common_tree = subprocess.check_output(['git', 'rev-parse', tree + ':kernel_platform/common'],
                                          cwd=S, text=True).strip()

F, C = E / 'frozen', E / 'candidate'
assert not F.exists() and not C.exists(), 'Never overwrite a frozen artifact set'
F.mkdir()
C.mkdir()
for source, name in [(B / 'arch/arm64/boot/Image', 'Image'), (B / 'vmlinux', 'vmlinux'),
                     (B / 'Module.symvers', 'Module.symvers'),
                     (B / 'vmlinux.symvers', 'vmlinux.symvers'), (B / '.config', 'kernel.config'),
                     (S / 'kernel_platform/bazel-bin/common/kernel_aarch64_abi_dump/abi.stg', 'abi.stg')]:
    shutil.copyfile(source, F / name)
info = run('baseline-avb-info', [T, 'info_image', '--image', BASE])
run('baseline-avb-verify', [T, 'verify_image', '--image', BASE, '--key', KEY])
salt = re.search(r'^\s+Salt:\s+([0-9a-f]+)$', info, re.M)[1]
props = dict(re.findall(r"^\s+Prop: ([^ ]+) -> '([^']*)'$", info, re.M))
assert props == {'com.android.build.boot.os_version': '16',
                 'com.android.build.boot.fingerprint': 'qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys',
                 'com.android.build.boot.security_patch': '2026-10-01'}
for pattern in [r'^Flags:\s+0$', r'^Rollback Index:\s+0$',
                r'^Rollback Index Location:\s+0$', r'^Algorithm:\s+SHA256_RSA4096$']:
    assert re.search(pattern, info, re.M), pattern
run('baseline-unpack', ['python3', H / 'tools/mkbootimg/unpack_bootimg.py',
                       '--boot_img', BASE, '--out', E / 'baseline-unpack'])
assert (E / 'baseline-unpack/ramdisk').stat().st_size == 0
boot = C / 'boot.img'
run('candidate-mkbootimg', ['python3', H / 'tools/mkbootimg/mkbootimg.py',
    '--header_version', '4', '--kernel', F / 'Image', '--ramdisk', E / 'baseline-unpack/ramdisk',
    '--cmdline', '', '--output', boot])
args = [T, 'add_hash_footer', '--image', boot, '--partition_name', 'boot',
        '--partition_size', '100663296', '--algorithm', 'SHA256_RSA4096', '--key', KEY,
        '--hash_algorithm', 'sha256', '--salt', salt, '--rollback_index', '0',
        '--rollback_index_location', '0', '--flags', '0']
for key, value in props.items():
    args += ['--prop', key + ':' + value]
run('candidate-avb-sign', args)
run('candidate-avb-verify', [T, 'verify_image', '--image', boot, '--key', KEY])
new_info = run('candidate-avb-info', [T, 'info_image', '--image', boot])
assert "Public key (sha1):        2597c218aae470a130f61162feaae70afd97f011" in new_info
run('candidate-unpack', ['python3', H / 'tools/mkbootimg/unpack_bootimg.py',
                        '--boot_img', boot, '--out', E / 'candidate-unpack'])
assert sha(E / 'candidate-unpack/kernel') == sha(F / 'Image')
assert (E / 'candidate-unpack/ramdisk').stat().st_size == 0
run('boot-only-scope', ['python3', '-B', S / 'tools/driver-updates/validate-coloros17-boot-only.py',
                      '--boot', boot, '--config', F / 'kernel.config'])
manifest = {
    'status': 'EXPERIMENTAL; ARM64 VM TESTED; PHONE NOT TESTED; NOT A STABLE RELEASE',
    'firmware': 'CPH2745_17.0.0.102(EX01)', 'device': 'OnePlus 15 SM8850',
    'branch': 'experiment/coloros17-linux115-droidspaces', 'base_commit': head,
    'kernel_common_snapshot_tree': common_tree, 'uncommitted_source': True,
    'kernel_source_patch_sha256': sha(E / 'kernel-source.patch'),
    'kernel_release': (B / 'include/config/kernel.release').read_text().strip(),
    'qualified_rollback_boot_sha256': sha(BASE),
    'delivery_model': 'boot.img only; original stock ColorOS DLKM, stock KGSL and GEAS',
    'boot_sha256': sha(boot), 'boot_bytes': boot.stat().st_size,
    'frozen': {path.name: {'sha256': sha(path), 'bytes': path.stat().st_size} for path in F.iterdir()},
    'AVB': {'verification': 'PASS', 'algorithm': 'SHA256_RSA4096', 'flags': 0,
            'rollback_index': 0, 'rollback_index_location': 0, 'properties': props,
            'signer': 'same qualified development/test boot key, not OEM authority'},
    'common_build': 'PASS', 'strict_KMI_symbol_list': 'PASS', 'ABI_generation': 'PASS',
    'enforced_GKI': 'FAIL; inherited deviations plus reviewed C-private IPC padding additions; reference unchanged',
    'compiled_ABI': abi_result, 'provider_policy': provider, 'stock_module_oracle': summary,
    'enforced_GKI_delta_classification': gki_delta,
    'checker_source': 'b01a81197f57115da465af6ef196f65a2bd10c27',
    'checker_results_ARM64_VM': checker, 'config_audit': config_result,
    'VM_fixture_sha256': {name: sha(E / name) for name in [
        'vm/initramfs.cpio', 'vm/namespace-smoke', 'vm/candidate-final.log',
        'upstream-checker/droidspaces-v7.1.0/aarch64/droidspaces',
        'qemu-local/root/usr/bin/qemu-system-aarch64']},
    'KernelSU_source': '28db74f6f4692e97006ce9ebd8b19b9cc614b5b7',
    'SUSFS_source': '68f62fae75ada3959f816841a3c64299383521ed',
    'toolchain_sha256': {str(path.relative_to(H)): sha(path) for path in [
        H / 'prebuilts/clang/host/linux-x86/clang-r536225/bin/clang.real',
        H / 'prebuilts/clang/host/linux-x86/clang-r536225/bin/ld.lld',
        H / 'prebuilts/rust/linux-x86/1.82.0.p2/bin/rustc',
        H / 'prebuilts/clang-tools/linux-x86/bin/bindgen']},
    'SUSFS_policy': 'unchanged; global mount hiding must be off for Droidspaces attempts; officially unsupported',
    'DLKM_changed': False, 'init_boot_changed': False, 'vbmeta_changed': False,
    'main_changed': False, 'device_actions': 'NONE', 'committed': False, 'published': False,
}
(C / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
(C / 'boot.img.sha256').write_text(sha(boot) + '  boot.img\n')
(C / 'SHA256SUMS').write_text(''.join(sha(C / name) + '  ' + name + '\n'
    for name in ['boot.img', 'boot.img.sha256', 'manifest.json']))
(F / 'SHA256SUMS').write_text(''.join(sha(path) + '  ' + path.name + '\n'
                                   for path in sorted(F.iterdir()) if path.is_file()))
(E / 'packaging-commands.json').write_text(json.dumps(commands, indent=2) + '\n')
print(json.dumps({'boot': str(boot), 'sha256': sha(boot), 'Image_sha256': sha(F / 'Image'),
                  'bytes': boot.stat().st_size, 'common_tree': common_tree}, indent=2))
