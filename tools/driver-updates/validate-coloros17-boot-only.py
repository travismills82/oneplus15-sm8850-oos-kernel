#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Validate boot-only payload scope, not full AVB/KMI or runtime qualification.

Read-only inputs. Never build, sign, flash, modify modules or contact a device.
Cryptographic AVB, the complete stock-module oracle and physical testing are
separate mandatory gates documented in the firmware-specific development policy.
"""
import argparse
import json
from pathlib import Path
import struct

POLICY = Path(__file__).with_name('coloros17-boot-only-policy.json')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate_policy(policy):
    require(policy['delivery_model'] == 'boot-only-with-untouched-stock-dlkm', 'wrong deployment model')
    require(policy['allowed_image_assets'] == ['boot.img'], 'only boot.img may be delivered')
    require(policy['allowed_write_partitions'] == ['boot'], 'only boot may be written')
    require(policy['replacement_modules'] == [], 'replacement modules are forbidden')
    require(policy['disabled_stock_modules'] == [], 'stock modules must not be excluded')
    require(policy['GEAS'] == 'enabled with the original stock GPU provider', 'stock GEAS provider contract changed')
    require(policy['required_modular_config'] == dict.fromkeys(
        ('CONFIG_BT', 'CONFIG_BT_HIDP', 'CONFIG_PPP', 'CONFIG_RFKILL', 'CONFIG_SLHC'), 'm'),
        'stock GKI modular provider contract changed')
    require(policy['boot_header_cmdline'] == '', 'stock boot header command line changed')


def validate_config(path, policy):
    values = {}
    for line in path.read_text().splitlines():
        if line.startswith('CONFIG_') and '=' in line:
            key, value = line.split('=', 1)
            require(key not in values, 'duplicate configuration symbol: ' + key)
            values[key] = value
    for name, wanted in policy['required_modular_config'].items():
        require(values.get(name) == wanted, name + ' must remain ' + wanted)
    for name in ('CONFIG_MODVERSIONS', 'CONFIG_MODULE_SIG', 'CONFIG_GENDWARFKSYMS', 'CONFIG_CFI_CLANG'):
        require(values.get(name) == 'y', name + ' must remain enabled')


def validate_boot(path, policy):
    require(path.is_file() and path.name == 'boot.img', 'payload must be named boot.img')
    require(path.stat().st_size == policy['boot_partition_bytes'], 'unexpected boot partition image size')
    with path.open('rb') as stream:
        header = stream.read(4096)
    require(len(header) == 4096 and header[:8] == b'ANDROID!', 'invalid Android boot header')
    kernel_bytes, ramdisk_bytes = struct.unpack_from('<II', header, 8)
    header_size = struct.unpack_from('<I', header, 20)[0]
    version = struct.unpack_from('<I', header, 40)[0]
    require(version == policy['boot_header_version'] and header_size == 1584, 'expected Android v4 header')
    require(kernel_bytes > 0 and ramdisk_bytes == 0, 'unexpected kernel/ramdisk ownership')
    require(4096 + kernel_bytes <= path.stat().st_size, 'truncated kernel payload')
    raw = header[44:1580]
    require(b'\0' in raw, 'unterminated boot command line')
    cmdline, padding = raw.split(b'\0', 1)
    require(not padding.strip(b'\0'), 'nonzero data after boot command line')
    require(cmdline.decode('ascii') == policy['boot_header_cmdline'],
            'boot header must retain stock command line; experimental module blacklists are forbidden')
    return {'header_version': version, 'kernel_bytes': kernel_bytes,
            'ramdisk_bytes': ramdisk_bytes, 'cmdline': cmdline.decode('ascii')}


def validate_assets(folder, policy):
    allowed = {'boot.img', 'SHA256SUMS', 'boot.img.sha256', 'manifest.json', 'README.md'}
    for path in folder.rglob('*'):
        require(not path.is_symlink(), 'symlinked payload is not accepted: ' + str(path))
        require(not path.is_file() or str(path.relative_to(folder)) in allowed,
                'unreviewed extra payload/archive: ' + str(path))
    images = sorted(str(path.relative_to(folder)) for path in folder.rglob('*')
                    if path.is_file() and path.suffix.lower() == '.img')
    require(images == policy['allowed_image_assets'], 'unexpected partition image(s): ' + repr(images))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--boot', type=Path)
    p.add_argument('--config', type=Path)
    p.add_argument('--assets', type=Path)
    args = p.parse_args()
    policy = json.loads(POLICY.read_text())
    validate_policy(policy)
    result = {'policy': 'PASS', 'scope': 'payload/configuration scope only; NOT full release qualification'}
    if args.config:
        validate_config(args.config, policy)
        result['config'] = 'PASS'
    if args.assets:
        require(args.assets.is_dir(), 'missing asset directory')
        validate_assets(args.assets, policy)
        result['assets'] = 'PASS'
    if args.boot:
        result['boot'] = validate_boot(args.boot, policy)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, UnicodeError, OSError, struct.error) as error:
        raise SystemExit('BOOT_ONLY_SCOPE_REJECTED: ' + str(error))
