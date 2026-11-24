#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# ARM64 QEMU init only. Never use as an on-device command.
set -eu
PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH
mount -t proc proc /proc
grep -qw coloros_test_guest=1 /proc/cmdline
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mkdir -p /tmp/freezer
mount -t cgroup -o freezer freezer /tmp/freezer
mkdir /tmp/freezer/probe
sleep 600 &
worker=$!
echo "$worker" > /tmp/freezer/probe/tasks
for i in 1 2 3; do
    echo FROZEN > /tmp/freezer/probe/freezer.state
    for attempt in 1 2 3 4 5; do
        [ "$(cat /tmp/freezer/probe/freezer.state)" = FROZEN ] && break
        sleep 1
    done
    [ "$(cat /tmp/freezer/probe/freezer.state)" = FROZEN ]
    echo THAWED > /tmp/freezer/probe/freezer.state
    [ "$(cat /tmp/freezer/probe/freezer.state)" = THAWED ]
    kill -0 "$worker"
done
echo CGROUP_V1_FREEZE_THAW=PASS
kill "$worker"
wait "$worker" || :

if [ -w /sys/power/pm_test ] && grep -qw freezer /sys/power/pm_test &&
   grep -qw mem /sys/power/state; then
    echo freezer > /sys/power/pm_test
    for i in 1 2; do
        echo mem > /sys/power/state
        echo "PM_FREEZER_TEST_$i=PASS"
    done
    echo none > /sys/power/pm_test
else
    echo PM_FREEZER_TEST=UNAVAILABLE
fi
echo DROIDSPACES_VM_END
while :; do sleep 60; done
