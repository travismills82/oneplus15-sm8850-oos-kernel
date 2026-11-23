#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Offline ARM64 VM test init only; not an Android installer.
PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mkdir -p /dev/pts /dev/shm /sys/fs/cgroup
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /dev/shm
mount -t cgroup2 cgroup2 /sys/fs/cgroup
. /container-test.sh
echo DROIDSPACES_VM_END
while :; do sleep 60; done
