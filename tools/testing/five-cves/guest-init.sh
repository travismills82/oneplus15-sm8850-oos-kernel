#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
export PATH=/bin:/sbin:/usr/bin
export PYTHON_COLORS=0
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /tmp
echo 'FIVE_CVES_VM_BEGIN'
uname -a
cat /proc/cmdline
if ! grep -qw 'five_cves_vm=1' /proc/cmdline; then
    echo 'FIVE_CVES_RESULT: FAIL missing VM guard'
    poweroff -f
    exit 1
fi
echo 1 > /proc/sys/kernel/panic_on_warn
echo 1 > /proc/sys/kernel/hung_task_panic
status=0
ip link set lo up || status=1
mkdir -p /run/netns
python3 /guest-test.py --iterations 40 || status=1
before_ct=$(cat /proc/sys/net/netfilter/nf_conntrack_count)
before_expect=$(wc -l < /proc/net/nf_conntrack_expect)
insmod /cve80668_pair.ko || status=1
rmmod cve80668_pair || status=1
after_ct=$(cat /proc/sys/net/netfilter/nf_conntrack_count)
after_expect=$(wc -l < /proc/net/nf_conntrack_expect)
echo "CVE80668_COUNTERS: conntrack=$before_ct/$after_ct expectations=$before_expect/$after_expect"
[ "$before_ct" = "$after_ct" ] && [ "$before_expect" = "$after_expect" ] || status=1
python3 /guest-tcp.py --seconds 5 || status=1
python3 /guest-dm-tc.py || status=1
insmod /five_cves_workqueue.ko guest_only=1 iterations=256 || status=1
if [ -d /sys/module/five_cves_workqueue ]; then
    rmmod five_cves_workqueue || status=1
else
    status=1
fi
echo 'FIVE_CVES_DMESG_BEGIN'
dmesg
echo 'FIVE_CVES_DMESG_END'
if [ "$status" -eq 0 ]; then
    echo 'FIVE_CVES_RESULT: PASS'
else
    echo "FIVE_CVES_RESULT: FAIL status=$status"
fi
sync
poweroff -f
