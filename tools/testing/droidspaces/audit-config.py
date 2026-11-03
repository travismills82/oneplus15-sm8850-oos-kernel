#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Gate the effective config delta, not merely defconfig requests."""
import argparse
import json
from pathlib import Path
import re


def config(path):
    values = {}
    for line in path.read_text().splitlines():
        if match := re.fullmatch(r'(CONFIG_\w+)=(.*)', line):
            values[match[1]] = match[2]
        elif match := re.fullmatch(r'# (CONFIG_\w+) is not set', line):
            values[match[1]] = 'n'
    return values


p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline', type=Path, required=True)
p.add_argument('--candidate', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
old, new = config(a.baseline), config(a.candidate)
changed = {key: {'baseline': old.get(key, 'absent'), 'candidate': new.get(key, 'absent')}
           for key in old.keys() | new.keys() if old.get(key) != new.get(key)}
allowed = {
    'SYSVIPC': 'y', 'SYSVIPC_SYSCTL': 'y', 'SYSVIPC_COMPAT': 'y',
    'POSIX_MQUEUE': 'y', 'POSIX_MQUEUE_SYSCTL': 'y', 'IPC_NS': 'y',
    'USER_NS': 'y', 'PID_NS': 'y', 'NETFILTER_XT_MATCH_ADDRTYPE': 'y',
    'DEVTMPFS': 'y', 'DEVTMPFS_MOUNT': 'n', 'DEVTMPFS_SAFE': 'n',
}
assert all(key in {'CONFIG_' + name for name in allowed} for key in changed), changed
assert all(new.get('CONFIG_' + name) == value for name, value in allowed.items())
features = [
    'NAMESPACES', 'UTS_NS', 'IPC_NS', 'PID_NS', 'USER_NS', 'NET_NS',
    'CGROUPS', 'MEMCG', 'MEMCG_V1', 'CGROUP_SCHED', 'CFS_BANDWIDTH',
    'CGROUP_CPUACCT', 'CGROUP_FREEZER', 'CGROUP_PIDS', 'CGROUP_DEVICE',
    'SECCOMP', 'SECCOMP_FILTER', 'BLK_DEV_LOOP', 'EXT4_FS', 'OVERLAY_FS',
    'FUSE_FS', 'TUN', 'VETH', 'BRIDGE', 'NETFILTER', 'NF_CONNTRACK',
    'NETFILTER_XTABLES', 'NETFILTER_XT_MATCH_ADDRTYPE', 'IP_NF_IPTABLES',
    'IP_NF_NAT', 'IP_NF_TARGET_MASQUERADE', 'NF_TABLES', 'MACVLAN',
    'IP6_NF_NAT', 'DEVTMPFS', 'DEVTMPFS_MOUNT', 'PROC_FS', 'SYSFS',
    'UNIX98_PTYS', 'SYSVIPC', 'POSIX_MQUEUE', 'CHECKPOINT_RESTORE',
    'FHANDLE', 'KSU', 'KSU_SUSFS', 'KSU_SUSFS_SPOOF_UNAME',
    'BT', 'BT_HIDP', 'PPP', 'RFKILL', 'SLHC',
    'MODULE_SIG', 'MODVERSIONS', 'GENDWARFKSYMS', 'CFI_CLANG',
    'SECURITY_SELINUX', 'SYSTEM_TRUSTED_KEYS', 'CMDLINE', 'CMDLINE_EXTEND',
]
report = {'result': 'PASS', 'changes': changed,
          'features': {name: {'baseline': old.get('CONFIG_' + name, 'absent'),
                              'candidate': new.get('CONFIG_' + name, 'absent')}
                       for name in features}}
a.out.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
print(json.dumps(changed, indent=2, sort_keys=True))
