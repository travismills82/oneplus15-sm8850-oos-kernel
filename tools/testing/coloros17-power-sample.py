#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Read-only ColorOS endpoint capture and same-boot interval comparison."""

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import shlex
import subprocess


SCRIPT = r'''
section() { printf '\n@@%s\n' "$1"; }
read_files() {
    for file in "$@"; do
        [ -r "$file" ] || continue
        printf '\n[%s]\n' "$file"
        timeout 2 cat "$file" 2>/dev/null || echo unavailable
    done
}
section identity
getprop ro.product.model
getprop ro.build.display.id
getprop ro.boot.slot_suffix
uname -r
cat /proc/sys/kernel/random/boot_id
cat /proc/uptime
section boot
slot="$(getprop ro.boot.slot_suffix)"
case "$slot" in
    _a|_b) sha256sum "/dev/block/by-name/boot$slot" ;;
esac
section battery
dumpsys battery | grep -E '^[[:space:]]+(AC powered|USB powered|Wireless powered|Dock powered|Charge counter|level|voltage|temperature):'
section display
dumpsys power | grep -E '^[[:space:]]+mWakefulness='
section thermal
dumpsys thermalservice | grep -E '^[[:space:]]*Thermal Status:'
section suspend
read_files /sys/power/mem_sleep /sys/power/pm_async /sys/power/suspend_stats/* \
    /sys/kernel/wakeup_reasons/last_resume_reason /sys/kernel/wakeup_reasons/last_suspend_time
section wakeups
read_files /sys/kernel/debug/wakeup_sources
for p in /sys/class/wakeup/wakeup*; do
    [ -r "$p/name" ] || continue
    read_files "$p"/name "$p"/active_count "$p"/event_count "$p"/wakeup_count "$p"/total_time_ms
done
section qcom_sleep
read_files /sys/power/system_sleep/stats /sys/kernel/debug/qcom_sleep_stats \
    /sys/kernel/debug/rpmh_stats /sys/kernel/debug/qcom_stats/* /sys/kernel/debug/qcom_sleep_stats/*
section scheduler
read_files /proc/stat /proc/schedstat /proc/softirqs /proc/interrupts \
    /proc/pressure/cpu /sys/module/workqueue/parameters/power_efficient
section cpufreq
for p in /sys/devices/system/cpu/cpufreq/policy*; do
    read_files "$p"/related_cpus "$p"/scaling_driver "$p"/scaling_governor \
        "$p"/scaling_min_freq "$p"/scaling_max_freq "$p"/cpuinfo_min_freq \
        "$p"/cpuinfo_max_freq "$p"/stats/time_in_state "$p"/walt/up_rate_limit_us \
        "$p"/walt/down_rate_limit_us
done
section cpuidle
for d in /sys/devices/system/cpu/cpu[0-9]*/cpuidle/state[0-9]*; do
    read_files "$d"/name "$d"/usage "$d"/time "$d"/disable
done
section gpu
for d in /sys/class/devfreq/*kgsl*; do
    read_files "$d"/governor "$d"/trans_stat "$d"/cur_freq "$d"/device/power/runtime_status
done
read_files /sys/class/kgsl/kgsl-3d0/gpu_clock_stats /sys/class/kgsl/kgsl-3d0/ifpc_count
# Avoid KGSL gpubusy: on some drivers reading it resets its sampling counters.
section storage
read_files /proc/diskstats /sys/class/scsi_host/host*/power/runtime_status \
    /sys/class/scsi_host/host*/power/runtime_active_time /sys/class/scsi_host/host*/power/runtime_suspended_time
grep ' /data ' /proc/mounts
section memory
read_files /proc/vmstat /proc/meminfo /proc/swaps /proc/pressure/memory /proc/pressure/io \
    /sys/kernel/mm/lru_gen/enabled /sys/block/zram*/comp_algorithm /sys/block/zram*/mm_stat \
    /sys/block/zram*/io_stat /sys/block/zram*/bd_stat
section network
read_files /proc/net/dev
section workqueue
read_files /sys/kernel/tracing/trace_stat/workqueue /sys/kernel/debug/tracing/trace_stat/workqueue
section powerstats
dumpsys -l | grep -Ei 'power\.stats|powerstats' || true
section providers
awk '{ print $1 }' /proc/modules
section firmware_settings
grep -o 'cgroup\.memory=[^ ]*' /proc/cmdline || true
section end
'''


def execute(adb, serial):
    result = subprocess.run(
        [adb, "-s", serial, "shell", "-T", "su -c " + shlex.quote(SCRIPT)],
        capture_output=True, text=True, check=True, timeout=100,
    )
    parts = re.split(r"^@@([a-z_]+)\s*$", result.stdout, flags=re.M)
    if parts[0].strip() or len(parts) % 2 != 1:
        raise ValueError("capture produced unexpected preamble or markers")
    sections = {key: body.strip() for key, body in zip(parts[1::2], parts[2::2])}
    if "end" not in sections:
        raise ValueError("incomplete capture")
    model, firmware, slot, release, boot_id, uptime = sections["identity"].splitlines()
    actual, *device = sections["boot"].split()
    if (model, firmware) != ("CPH2747", "CPH2747_17.0.1.201(EX01)"):
        raise ValueError("wrong device/firmware")
    if slot not in ("_a", "_b") or device != ["/dev/block/by-name/boot" + slot]:
        raise ValueError("slot mismatch")
    if not re.fullmatch(r"[0-9a-f]{64}", actual) or not re.fullmatch(r"[0-9a-f-]{36}", boot_id):
        raise ValueError("unreadable boot identity")
    return {
        "capture_utc": datetime.now(timezone.utc).isoformat(),
        "firmware": firmware, "slot": slot, "release": release,
        "boot_sha256": actual, "boot_id": boot_id, "uptime_s": float(uptime.split()[0]),
        "sections": sections,
    }


def battery_fields(snapshot):
    return dict(re.findall(r"^\s*([\w ]+):\s*(\w+)\s*$",
                           snapshot["sections"]["battery"], re.M))


def interval(before, after):
    for key in ("firmware", "slot", "release", "boot_sha256", "boot_id"):
        if before[key] != after[key]:
            raise ValueError("interval crosses a different boot or artifact: " + key)
    elapsed = after["uptime_s"] - before["uptime_s"]
    if elapsed <= 0:
        raise ValueError("invalid endpoint order")
    early, late = map(battery_fields, (before, after))
    report = {"artifact": before["boot_sha256"], "elapsed_s": elapsed,
              "display_endpoints": [s["sections"]["display"] for s in (before, after)],
              "battery": "unavailable (possibly plugged or missing counters)"}
    unplugged = all(fields.get(rail + " powered") == "false" for fields in (early, late)
                    for rail in ("AC", "USB", "Wireless", "Dock"))
    if unplugged:
        try:
            level0, level1 = (int(item["level"]) for item in (early, late))
            q0, q1 = (int(item["Charge counter"]) for item in (early, late))
            v0, v1 = (int(item["voltage"]) for item in (early, late))
            if 0 < q1 <= q0 and 0 <= level1 <= level0 <= 100 and all(2000 < v < 5000 for v in (v0, v1)):
                mah = (q0 - q1) / 1000
                report["battery"] = {
                    "drain_pct_points_per_hour": (level0 - level1) * 3600 / elapsed,
                    "gauge_delta_mAh": mah,
                    "gauge_mAh_per_hour": mah * 3600 / elapsed,
                    "endpoint_estimate_mWh": mah * (v0 + v1) / 2000,
                    "note": "fuel-gauge estimate; not rail energy or proof of continuous idle/unplugged state",
                }
        except (KeyError, ValueError):
            pass
    for topic, pattern in {
        "memory": r"^(workingset_nodereclaim|workingset_refault_file|pgscan_kswapd|pgsteal_kswapd|pswpin|pswpout) (\d+)$",
        "suspend": r"^\[/sys/power/suspend_stats/(\w+)\]\n(\d+)$",
    }.items():
        a, b = (dict(re.findall(pattern, item["sections"][topic], re.M))
                for item in (before, after))
        report[topic + "_delta"] = {k: int(b[k]) - int(a[k]) for k in sorted(a.keys() & b.keys())
                                    if int(b[k]) >= int(a[k])}
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)
    capture = sub.add_parser("capture")
    capture.add_argument("--adb", default="adb")
    capture.add_argument("--serial", required=True)
    capture.add_argument("--boot-sha256", required=True)
    capture.add_argument("--output", type=Path, required=True)
    diff = sub.add_parser("interval")
    diff.add_argument("start", type=Path)
    diff.add_argument("end", type=Path)
    args = p.parse_args()
    try:
        if args.command == "capture":
            snap = execute(args.adb, args.serial)
            if snap["boot_sha256"] != args.boot_sha256:
                raise ValueError("does not match the expected boot image")
            with args.output.open("x") as stream:
                json.dump(snap, stream, indent=2)
                stream.write("\n")
        else:
            print(json.dumps(interval(json.loads(args.start.read_text()),
                                      json.loads(args.end.read_text())), indent=2))
    except (ValueError, KeyError, subprocess.SubprocessError, OSError) as error:
        p.exit(2, f"{error}\n")


if __name__ == "__main__":
    main()
