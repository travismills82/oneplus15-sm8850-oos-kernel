#!/bin/busybox sh
# SPDX-License-Identifier: GPL-2.0-only
# Source in a disposable ARM64 test VM after proc/sysfs/cgroup mounts.
# Never run on Android or the host. Use an offline VM with no physical NIC.
coloros_container_test() (
    export DS_NO_PROXY=1
    root=/mnt/test-rootfs
    name=efficiency-smoke
    [ -x "$root/bin/busybox" ] && [ -r "$root/etc/alpine-release" ] || exit 1
    mkdir -p "$root"/sbin "$root"/tmp /var/lib/Droidspaces || exit 1
    printf '#!/bin/sh\nwhile :; do /bin/busybox sleep 60; done\n' > "$root"/sbin/fixture-init || exit 1
    chmod 755 "$root"/sbin/fixture-init || exit 1
    /bin/droidspaces version || exit 1
    check="$(timeout 20 /bin/droidspaces check --json)" || exit 1
    printf '%s\n' "$check"
    printf '%s\n' "$check" | grep -Eq '"requirements_met":[[:space:]]*1' || exit 1
    timeout 22 /bin/droidspaces --name="$name" --rootfs="$root" --net=none \
        --init=/sbin/fixture-init start || exit 1
    trap 'timeout 10 /bin/droidspaces --name="$name" stop >/dev/null 2>&1 || true' EXIT
    host_pidns="$(readlink /proc/self/ns/pid)" || exit 1
    guest_pidns="$(timeout 10 /bin/droidspaces --name="$name" run /bin/busybox readlink /proc/self/ns/pid)" || exit 1
    printf '%s\n' "$guest_pidns" | grep -Eq '^pid:\[[0-9]+\]$' || exit 1
    [ "$guest_pidns" != "$host_pidns" ] || exit 1
    timeout 15 /bin/droidspaces --name="$name" run /bin/sh -c '
        /bin/busybox cat /etc/alpine-release &&
        /bin/busybox dd if=/dev/zero of=/tmp/payload bs=1048576 count=8 &&
        /bin/busybox sha256sum /tmp/payload &&
        test -r /proc/self/cgroup &&
        /bin/busybox rm /tmp/payload' || exit 1
    timeout 22 /bin/droidspaces --name="$name" stop || exit 1
    trap - EXIT
    [ "$(timeout 5 /bin/droidspaces --name="$name" pid)" = NONE ]
)

if coloros_container_test; then
    echo COLOROS_DROIDSPACES_CONTAINER=PASS
else
    echo COLOROS_DROIDSPACES_CONTAINER=FAIL
fi
