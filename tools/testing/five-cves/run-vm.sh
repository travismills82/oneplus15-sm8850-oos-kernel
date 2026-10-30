#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail
if [[ $# != 1 || $1 != /* ]]; then
    echo "Usage: $0 /absolute/disposable-output-directory" >&2
    exit 2
fi
out=$(realpath -- "$1")
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
initramfs=${INITRAMFS:-$out/initramfs.cpio.gz}
log=${VM_LOG:-$out/serial.log}
[[ $log == "$out/"* && ! -e $log ]] || { echo 'Refusing log overwrite/escape' >&2; exit 2; }
test -r "$out/kernel/arch/x86/boot/bzImage"
test -r "$initramfs"
test -r /dev/kvm && test -w /dev/kvm
qmp="$out/qmp.sock"
[[ ! -e $qmp && ! -L $qmp ]] || { echo 'Refusing existing QMP socket' >&2; exit 2; }
python3 "$here/qmp-finish.py" "$qmp" "$log" 900 &
watcher=$!
trap 'kill "$watcher" 2>/dev/null || true; wait "$watcher" 2>/dev/null || true' EXIT
set +e
timeout --signal=TERM --kill-after=10s 900 \
    qemu-system-x86_64 -accel kvm -cpu host -smp 2 -m 3G \
    -nographic -no-reboot -nic none \
    -qmp "unix:$qmp,server=on,wait=off" \
    -kernel "$out/kernel/arch/x86/boot/bzImage" \
    -initrd "$initramfs" \
    -append 'console=ttyS0 five_cves_vm=1 cve80668_vm=1 panic=-1 oops=panic panic_on_warn=1 kasan.fault=panic' \
    2>&1 | tee "$log"
result=${PIPESTATUS[0]}
set -e
kill "$watcher" 2>/dev/null || true
wait "$watcher" 2>/dev/null || true
trap - EXIT
if [[ -S $qmp ]]; then rm -- "$qmp"; fi
test "$result" -eq 0
grep -q '^FIVE_CVES_RESULT: PASS' "$log"
grep -q 'FIVE_CVES_WQ: PASS iterations=256' "$log"
grep -q '"test": "tcp_getsockopt_concurrency"' "$log"
grep -q '"stage": "five-cves-dm-tc"' "$log"
python3 "$here/check-vm-log.py" "$log"
sha256sum "$log"
