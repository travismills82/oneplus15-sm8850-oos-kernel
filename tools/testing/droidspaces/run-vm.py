#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run disposable ARM64 feature probes with no host disks or external network."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import selectors
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--qemu', type=Path, required=True)
p.add_argument('--kernel', type=Path, required=True)
p.add_argument('--initramfs', type=Path, required=True)
p.add_argument('--log', type=Path, required=True)
a = p.parse_args()
assert a.kernel.read_bytes()[:4] != b'\x7fELF', 'Use the actual ARM64 Image'
command = [str(a.qemu), '-machine', 'virt,gic-version=3', '-cpu', 'max',
           '-accel', 'tcg,thread=multi', '-smp', '2', '-m', '1536', '-display', 'none',
           '-monitor', 'none', '-serial', 'stdio', '-nic', 'none', '-no-reboot',
           '-kernel', str(a.kernel), '-initrd', str(a.initramfs),
           '-append', 'console=ttyAMA0 rdinit=/init panic=1']
def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()

identity = {'kernel_sha256': sha(a.kernel), 'initramfs_sha256': sha(a.initramfs),
            'qemu_sha256': sha(a.qemu), 'command': command, 'physical_device': False}
process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
selector = selectors.DefaultSelector()
selector.register(process.stdout, selectors.EVENT_READ)
deadline, completed, capture = time.monotonic() + 180, False, bytearray()
try:
    with a.log.open('xb') as output:
        output.write(('DROIDSPACES_VM_INPUTS ' + json.dumps(identity) + '\n').encode())
        while time.monotonic() < deadline:
            ready = selector.select(timeout=1)
            for key, _ in ready:
                data = os.read(key.fileobj.fileno(), 65536)
                if data:
                    capture.extend(data)
                    output.write(data)
                    output.flush()
                else:
                    selector.unregister(key.fileobj)
            if b'DROIDSPACES_VM_END' in capture:
                completed = True
                break
            if process.poll() is not None:
                break
finally:
    if process.poll() is None:
        process.terminate()
    process.wait(timeout=10)
    selector.close()
print(f'VM_END_MARKER={completed}; log={a.log}')
assert completed and b'Kernel panic' not in capture, 'VM probe did not complete cleanly'
