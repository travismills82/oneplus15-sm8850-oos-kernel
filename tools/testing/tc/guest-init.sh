#!/bin/busybox sh
# SPDX-License-Identifier: GPL-2.0-only
# Disposable VM PID 1 only; never install or run on a phone or the host.
export PATH=/bin
/bin/busybox --install -s /bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
echo TC_VM_BEGIN
uname -a
/bin/qdisc-smoke
echo "TC_QDISC_SMOKE_RC=$?"
# Reuse the existing bounded, no-disk/no-network ARM64 VM runner's end marker.
echo DROIDSPACES_VM_END
while :; do sleep 60; done
