#!/usr/bin/env python3
"""Package/verify exact .115 canary bytes; never builds kernels or flashes devices.

This is a separate restricted prerelease profile, not the old controlled-v1
stable-release verifier. Existing matching vbmeta is read-only and mandatory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / 'tools/releases/coloros17-linux115-no-geas.json'
HELPER = ROOT / 'tools/releases/coloros17-linux115-no-geas-installer.sh'
TEMPLATE = ROOT / 'tools/releases/coloros17-linux115-no-geas-update-binary.in'
NOTES = ROOT / 'docs/releases/coloros17.0.0.102-linux-6.12.115-no-geas-canary/release-notes.md'
SEAL_SHA = '1656d8fc35bff3999aae1d20548b258a0d0214d36996c32be096ba819539f36a'
STAMP = (2026, 11, 1, 0, 0, 0)
PAYLOADS = {'boot.img', 'vendor_dlkm.img'}
HASH = re.compile(r'[0-9a-f]{64}\Z')
COMMIT = re.compile(r'[0-9a-f]{40}\Z')


def sha(path: Path) -> str:
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def encoded(value) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def profile():
    p = json.loads(PROFILE.read_text())
    require(p['schema_version'] == 1 and p['profile'] == 'coloros17-linux115-no-geas-existing-parent-canary-v1', 'wrong canary profile')
    require(p['kernel_version'] == '6.12.115' and p['kernel_release'].startswith('6.12.115-'), 'not Linux 6.12.115')
    require(COMMIT.fullmatch(p['kernel_source_commit']), 'invalid build source identity')
    require(p['slot'] == '_b' and set(p['payloads']) == PAYLOADS, 'unexpected slot/payload boundary')
    require(p['write_scope'] == ['vendor_dlkm_b', 'boot_b'] and p['write_order'] == p['write_scope'], 'write scope/order changed')
    require(p['qualification']['zip_physical_install'] == 'NOT TESTED', 'cannot fabricate a physical ZIP result')
    require(p['qualification']['release_class'] == 'restricted experimental prerelease, not stable', 'not a stable profile')
    for row in p['payloads'].values():
        require(isinstance(row['bytes'], int) and row['bytes'] > 0 and row['bytes'] % 4096 == 0, 'invalid raw image size')
        require(HASH.fullmatch(row['sha256']), 'invalid payload SHA256')
    require(HASH.fullmatch(p['required_existing_parent_sha256']), 'invalid existing-parent prerequisite')
    require(p['required_existing_vendor_sha256'] == p['payloads']['vendor_dlkm.img']['sha256'], 'unqualified rollback vendor contract')
    return p


def contract_script(p):
    values = {
        'PROFILE': p['profile'], 'FIRMWARE': p['firmware'],
        'MIN_BATTERY': p['minimum_battery_percent'],
        'MAX_BATTERY_TEMP': p['maximum_battery_temperature_deciC'],
        'RECOVERY': p['recovery'], 'RECOVERY_KERNEL': p['recovery_kernel'],
        'BOOT_BYTES': p['payloads']['boot.img']['bytes'],
        'BOOT_SHA': p['payloads']['boot.img']['sha256'],
        'VENDOR_BYTES': p['payloads']['vendor_dlkm.img']['bytes'],
        'VENDOR_SHA': p['payloads']['vendor_dlkm.img']['sha256'],
        'PARENT_BYTES': p['required_existing_parent_bytes'],
        'PARENT_SHA': p['required_existing_parent_sha256'],
        'EXISTING_VENDOR_SHA': p['required_existing_vendor_sha256'],
        'SUPER_PREFIX_BYTES': p['required_super_prefix_bytes'],
        'SUPER_PREFIX_SHA': p['required_super_prefix_sha256'],
        'VENDOR_SECTORS': p['vendor_extent']['sectors'],
        'VENDOR_START': p['vendor_extent']['start'],
        'COMPANIONS': ' '.join(k + ':' + v for k, v in sorted(p['required_companions'].items())),
    }
    return ('# Generated exact canary constants; no runtime target overrides.\n' + ''.join('readonly '+k+'='+shlex.quote(str(v))+'\n' for k,v in values.items())).encode()


def wrapper(helper: bytes, contract: bytes):
    s = TEMPLATE.read_text()
    for name, data in [('HELPER', helper), ('CONTRACT', contract)]:
        s = s.replace('@'+name+'_SHA256@', hashlib.sha256(data).hexdigest())
    require('@HELPER_' not in s and '@CONTRACT_' not in s, 'unrendered template')
    return s.encode()


def kernel_info(p):
    return ''.join(f'{k}={p[k]}\n' for k in ('release_tag','profile','kernel_version','kernel_release','kernel_source_commit','firmware','slot')).encode()


def sums(rows):
    return ''.join(f'{digest}  {name}\n' for name,digest in sorted(rows.items())).encode()


def filesums(folder):
    return sums({p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name != 'SHA256SUMS'})


def build(images: Path, output: Path, seal: Path):
    p = profile()
    require(not output.exists(), 'refusing to overwrite a package directory')
    require(sha(seal) == SEAL_SHA, 'frozen qualification seal changed')
    frozen = json.loads(seal.read_text())
    require(frozen['source_head'] == p['kernel_source_commit'], 'frozen source mismatch')
    require(frozen['kernel_release'] == p['kernel_release'], 'frozen kernel mismatch')
    for name, row in p['payloads'].items():
        f = images / name
        require(f.is_file() and f.stat().st_size == row['bytes'] and sha(f) == row['sha256'], 'wrong frozen payload: '+name)
        with f.open('rb') as stream:
            if name == 'boot.img':
                require(stream.read(8) == b'ANDROID!', 'invalid Android boot image')
            else:
                stream.seek(1080)
                require(stream.read(2) == b'\x53\xef', 'vendor image is not ext4')
            stream.seek(-64, 2)
            require(stream.read(4) == b'AVBf', 'missing AVB footer')
    output.mkdir(parents=True)
    helper = HELPER.read_bytes(); contract = contract_script(p)
    members = {
        'installer.sh': helper, 'contract.sh': contract,
        'canary-contract.json': encoded(p), 'kernel-info.txt': kernel_info(p),
        'README.md': NOTES.read_bytes(),
        'META-INF/com/google/android/update-binary': wrapper(helper,contract),
        'META-INF/com/google/android/updater-script': b'# Installation uses the bundled restricted two-image update-binary.\n',
    }
    for name in sorted(PAYLOADS):
        shutil.copyfile(images/name, output/name)
        members[name] = (images/name).read_bytes()
    members['SHA256SUMS'] = sums({n:hashlib.sha256(data).hexdigest() for n,data in members.items()})
    archive = output/p['zip_name']
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name,data in sorted(members.items()):
            info = zipfile.ZipInfo(name, STAMP)
            info.create_system = 3
            info.external_attr = (0o755 if name.endswith('/update-binary') else 0o644) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info,data,compresslevel=6)
    manifest = {
        'schema_version':1, 'contract':p, 'publication_commit':'TAG_TARGET',
        'frozen_candidate_seal_sha256':SEAL_SHA,
        'zip':{'name':archive.name,'bytes':archive.stat().st_size,'sha256':sha(archive)},
        'installer_sha256':hashlib.sha256(helper).hexdigest(),
        'contract_script_sha256':hashlib.sha256(contract).hexdigest(),
        'payload_rebuilds':False, 'device_actions':'NONE',
        'zip_validation':'offline fixture/static tests only; no physical ZIP install',
    }
    (output/'canary-manifest.json').write_bytes(encoded(manifest))
    (output/'SHA256SUMS').write_bytes(filesums(output))
    verify(output, allow_symbolic=True)
    print('CANARY PACKAGE PASS; Linux 6.12.115; vbmeta not packaged or written')
    print('ZIP', archive, manifest['zip']['sha256'],flush=True)


def metadata(m, allow_symbolic=False):
    p = profile()
    require(m['schema_version'] == 1 and m['contract'] == p, 'manifest profile differs from reviewed source')
    require(m['frozen_candidate_seal_sha256'] == SEAL_SHA and m['payload_rebuilds'] is False, 'artifact provenance differs')
    require(m['zip']['name'] == p['zip_name'] and HASH.fullmatch(m['zip']['sha256']) and m['zip']['bytes']>0, 'invalid ZIP identity')
    for key,data in [('installer_sha256',HELPER.read_bytes()),('contract_script_sha256',contract_script(p))]:
        require(m[key] == hashlib.sha256(data).hexdigest(), 'installer/config provenance mismatch')
    commit = m['publication_commit']
    require(bool(COMMIT.fullmatch(commit)) or (allow_symbolic and commit == 'TAG_TARGET'), 'invalid publication commit')
    return p


def verify_source(p, commit, repo):
    require(commit and COMMIT.fullmatch(commit), 'exact tagged commit required for source audit')
    local_env={**os.environ,'GIT_NO_LAZY_FETCH':'1'}
    subprocess.run(['git','-C',str(repo),'cat-file','-e',commit+'^{commit}'],check=True,capture_output=True,env=local_env,timeout=30)
    subprocess.run(['git','-C',str(repo),'merge-base','--is-ancestor',p['kernel_source_commit'],commit],check=True,capture_output=True,env=local_env,timeout=30)
    tree=subprocess.check_output(['git','-C',str(repo),'rev-parse',commit+':kernel_platform/common'],text=True,env=local_env,timeout=30).strip()
    require(tree==p['common_tree'], 'published kernel source differs from tested build')
    subprocess.run(['git','-C',str(repo),'diff','--exit-code',p['kernel_source_commit'],commit,'--','kernel_platform','vendor'],check=True,capture_output=True,env=local_env,timeout=30)


def verify(folder: Path, commit=None, allow_symbolic=False, repo=None):
    manifest = json.loads((folder/'canary-manifest.json').read_text())
    p = metadata(manifest, allow_symbolic)
    expected = PAYLOADS | {p['zip_name'],'canary-manifest.json','SHA256SUMS'}
    require({f.name for f in folder.iterdir()} == expected, 'unexpected/missing release assets')
    require((folder/'SHA256SUMS').read_bytes() == filesums(folder), 'external SHA256 manifest mismatch')
    for name,row in p['payloads'].items():
        f=folder/name
        require(f.is_file() and f.stat().st_size==row['bytes'] and sha(f)==row['sha256'], 'payload mismatch: '+name)
    zpath=folder/p['zip_name']
    require(zpath.stat().st_size==manifest['zip']['bytes'] and sha(zpath)==manifest['zip']['sha256'], 'ZIP digest/size mismatch')
    with zipfile.ZipFile(zpath) as z:
        expected_members=PAYLOADS | {'installer.sh','contract.sh','canary-contract.json','kernel-info.txt','README.md','SHA256SUMS','META-INF/com/google/android/update-binary','META-INF/com/google/android/updater-script'}
        require(z.namelist()==sorted(expected_members), 'ZIP contains duplicate, unexpected or missing paths')
        entries={}
        for info in z.infolist():
            require(info.date_time==STAMP and info.create_system==3, 'ZIP metadata is not canonical')
            want=0o755 if info.filename.endswith('/update-binary') else 0o644
            require(info.external_attr >> 16 == want, 'incorrect archive permissions')
            if info.filename in PAYLOADS:
                require(info.file_size==p['payloads'][info.filename]['bytes'], 'incorrect uncompressed image size')
            else:
                require(info.file_size<=1024*1024, 'oversized metadata member')
            # Bound uncompressed lengths before reading; z.read also checks CRC.
            entries[info.filename]=z.read(info)
        expected_sums=sums({n:hashlib.sha256(data).hexdigest() for n,data in entries.items() if n!='SHA256SUMS'})
        require(entries['SHA256SUMS']==expected_sums, 'internal archive digests differ')
        helper=HELPER.read_bytes();contract=contract_script(p)
        require(entries['installer.sh']==helper and entries['contract.sh']==contract, 'embedded helper/config differs')
        require(entries['META-INF/com/google/android/update-binary']==wrapper(helper,contract), 'embedded entrypoint differs')
        require(entries['canary-contract.json']==encoded(p) and entries['kernel-info.txt']==kernel_info(p), 'embedded contract differs')
        require(entries['README.md']==NOTES.read_bytes(), 'embedded installation/coverage notes differ')
        for name in PAYLOADS:
            require(hashlib.sha256(entries[name]).hexdigest()==p['payloads'][name]['sha256'], 'embedded payload differs')
    if commit:
        require(manifest['publication_commit']==commit, 'publication commit differs')
    if repo:
        verify_source(p,commit,repo)
    print('EXACT TWO-IMAGE CANARY VERIFICATION PASS',flush=True)


def finalize(folder: Path, commit: str):
    require(COMMIT.fullmatch(commit), 'exact publication commit required')
    verify(folder,allow_symbolic=True)
    path=folder/'canary-manifest.json';m=json.loads(path.read_text())
    require(m['publication_commit']=='TAG_TARGET', 'refusing to refinalize a sealed release')
    # Reject an absent/wrong commit before changing either metadata file.
    verify_source(m['contract'],commit,ROOT)
    m['publication_commit']=commit
    path.write_bytes(encoded(m));(folder/'SHA256SUMS').write_bytes(filesums(folder))
    verify(folder,commit=commit,repo=ROOT)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    sub=ap.add_subparsers(dest='action',required=True)
    b=sub.add_parser('build');b.add_argument('--images',type=Path,required=True);b.add_argument('--out',type=Path,required=True);b.add_argument('--seal',type=Path,required=True)
    v=sub.add_parser('verify');v.add_argument('--assets',type=Path,required=True);v.add_argument('--commit');v.add_argument('--repo',type=Path);v.add_argument('--allow-symbolic',action='store_true')
    f=sub.add_parser('finalize');f.add_argument('--assets',type=Path,required=True);f.add_argument('--commit',required=True)
    m=sub.add_parser('metadata');m.add_argument('manifest',type=Path)
    a=ap.parse_args()
    try:
        if a.action=='build':build(a.images,a.out,a.seal)
        elif a.action=='verify':verify(a.assets,a.commit,a.allow_symbolic,a.repo)
        elif a.action=='finalize':finalize(a.assets,a.commit)
        else:metadata(json.loads(a.manifest.read_text()),allow_symbolic=True);print('Canary metadata PASS; not physical qualification')
    except (OSError,ValueError,KeyError,zipfile.BadZipFile,subprocess.CalledProcessError,subprocess.TimeoutExpired) as e:
        ap.exit(1, 'ERROR: '+str(e)+'\n')


if __name__=='__main__':
    main()
