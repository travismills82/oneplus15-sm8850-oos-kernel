#!/bin/busybox sh
# SPDX-License-Identifier: GPL-2.0-only
# Ephemeral ARM64 VM only. Never run this as a script on Android or the host.
export PATH=/bin
/bin/busybox --install -s /bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t tmpfs tmpfs /dev
mknod /dev/null c 1 3
mknod /dev/zero c 1 5
mknod /dev/random c 1 8
mknod /dev/urandom c 1 9
mknod /dev/console c 5 1
mknod /dev/ptmx c 5 2
mknod /dev/loop-control c 10 237
mknod /dev/loop0 b 7 0
mknod /dev/fuse c 10 229
mkdir -p /dev/pts /dev/shm /dev/net /dev/block /sys/fs/cgroup /var/lib/droidspaces /data/local/Droidspaces
ln -s /dev/loop0 /dev/block/loop0
ln -s /dev/loop-control /dev/block/loop-control
mknod /dev/net/tun c 10 200
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /dev/shm
mount -t cgroup2 cgroup2 /sys/fs/cgroup
echo DROIDSPACES_VM_BEGIN
uname -a
cat /proc/cgroups
cat /sys/fs/cgroup/cgroup.controllers
/bin/droidspaces check --json
echo "DROIDSPACES_CHECK_RC=$?"
if [ -x /bin/namespace-smoke ]; then
    /bin/namespace-smoke
    echo "NAMESPACE_SMOKE_RC=$?"
fi
mkdir -p /mnt/image /mnt/lower /mnt/upper /mnt/work /mnt/overlay /mnt/devtmp
if timeout 15 losetup /dev/loop0 /ext4.img && timeout 15 mount -t ext4 /dev/loop0 /mnt/image; then
    echo persistence > /mnt/image/probe
    sync
    umount /mnt/image
    mount -t ext4 /dev/loop0 /mnt/image
    if grep -q persistence /mnt/image/probe; then echo EXT4_LOOP_REMOUNT=PASS; fi
    umount /mnt/image
    losetup -d /dev/loop0
fi
echo lower > /mnt/lower/read
if mount -t overlay overlay -o lowerdir=/mnt/lower,upperdir=/mnt/upper,workdir=/mnt/work /mnt/overlay; then
    echo upper > /mnt/overlay/write
    if grep -q lower /mnt/overlay/read && grep -q upper /mnt/upper/write; then echo OVERLAY_COPYUP=PASS; fi
    umount /mnt/overlay
fi
if mount -t devtmpfs devtmpfs /mnt/devtmp; then
    echo DEVTMPFS_MOUNT=PASS
    umount /mnt/devtmp
fi
echo DROIDSPACES_VM_END
sync
# The host runner stops this disposable VM after the end marker. Never let
# PID 1 exit (that would create an artificial kernel panic).
while :; do sleep 60; done
