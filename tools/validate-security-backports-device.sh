#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Read-only post-flash diagnostics for the OnePlus 15 OxygenOS 16.0.9.400
# boot-only security-backport candidate. This script never flashes, reboots,
# mounts, changes a setting, or writes a device partition.

set -u -o pipefail

ADB="${ADB:-adb}"
ROOT_AVAILABLE=0

section() {
    printf '\n=== %s ===\n' "$1"
}

pass() {
    printf 'PASS: %s\n' "$*"
}

warn() {
    printf 'WARN: %s\n' "$*" >&2
}

fail() {
    printf 'FAIL: %s\n' "$*" >&2
}

run_shell() {
    local label="$1"
    local command="$2"
    local output

    printf '\n$ adb shell %s\n' "$command"
    if output="$("$ADB" shell sh -c "$command" 2>&1)"; then
        printf '%s\n' "$output"
    else
        warn "$label could not be collected"
        printf '%s\n' "$output"
    fi
}

run_root() {
    local label="$1"
    local command="$2"
    local output

    if [ "$ROOT_AVAILABLE" -ne 1 ]; then
        warn "$label requires adb shell su -c; skipping"
        return 0
    fi

    printf '\n$ adb shell su -c %s\n' "$command"
    if output="$("$ADB" shell su -c "$command" 2>&1)"; then
        printf '%s\n' "$output"
    else
        warn "$label could not be collected"
        printf '%s\n' "$output"
    fi
}

if ! command -v "$ADB" >/dev/null 2>&1; then
    fail "adb is unavailable (set ADB=/absolute/path/to/adb if needed)"
    exit 2
fi

device_state="$("$ADB" get-state 2>&1 || true)"
if [ "$device_state" != "device" ]; then
    fail "no authorized Android device is connected (adb get-state: $device_state)"
    exit 1
fi
pass "authorized Android device detected"

if "$ADB" shell su -c id >/dev/null 2>&1; then
    ROOT_AVAILABLE=1
    pass "root shell is available for diagnostic reads"
else
    warn "root shell is unavailable; privileged diagnostics will be skipped"
fi

section "Kernel identity"
run_shell "uname" "uname -a"
run_shell "kernel version" "cat /proc/version"
run_shell "Android build identity" "getprop ro.product.device; getprop ro.product.model; getprop ro.build.version.release; getprop ro.build.version.security_patch; getprop ro.boot.slot_suffix"

section "Kernel configuration and security invariants"
config_command='if [ -r /proc/config.gz ]; then (zcat /proc/config.gz 2>/dev/null || gzip -dc /proc/config.gz 2>/dev/null) | grep -E "^(CONFIG_(MODULE_SIG|MODVERSIONS|GENDWARFKSYMS|CFI_CLANG|SECCOMP|SECCOMP_FILTER|SECURITY_SELINUX|F2FS_FS|EROFS_FS|CIFS|SQUASHFS|ISO9660_FS|UDF_FS|CAN|VLAN_8021Q|USB_MON|USB_SERIAL|USB_USBNET|NET_SCH_SFQ)=|# CONFIG_(NF_TABLES|BTRFS_FS|NFS_FS) is not set)" || true; else echo "/proc/config.gz is unavailable"; fi'
run_root "kernel config" "$config_command"

section "Stock system_dlkm contract"
system_dlkm_mount="$("$ADB" shell sh -c 'awk "\$2 == \"/system_dlkm\" { print }" /proc/mounts' 2>&1 || true)"
printf '%s\n' "$system_dlkm_mount"
if printf '%s\n' "$system_dlkm_mount" | grep -q ' erofs '; then
    pass "/system_dlkm is mounted as EROFS"
elif [ -n "$system_dlkm_mount" ]; then
    fail "/system_dlkm is mounted but is not reported as EROFS"
else
    fail "/system_dlkm mount was not found"
fi
run_root "system_dlkm backing device" "ls -l /dev/block/by-name/system_dlkm* 2>/dev/null || true; blockdev --getsize64 /dev/block/by-name/system_dlkm 2>/dev/null || true"

section "Kernel error and ABI scan"
if [ "$ROOT_AVAILABLE" -eq 1 ]; then
    dmesg_output="$("$ADB" shell su -c dmesg 2>&1 || true)"
else
    dmesg_output="$("$ADB" shell dmesg 2>&1 || true)"
fi
printf '%s\n' "$dmesg_output" | tail -n 300

module_errors="$(printf '%s\n' "$dmesg_output" | grep -Ei 'Unknown symbol|disagrees about version|bad vermagic|module verification failed|protected symbol' || true)"
if [ -n "$module_errors" ]; then
    fail "possible module ABI/signature failure found; inspect these lines"
    printf '%s\n' "$module_errors"
else
    pass "no obvious module ABI/signature failure was found in dmesg"
fi

kernel_warnings="$(printf '%s\n' "$dmesg_output" | grep -Ei 'CRC|Oops|BUG:|KASAN|UBSAN|kernel panic|Call trace' || true)"
if [ -n "$kernel_warnings" ]; then
    warn "kernel warning signatures were found; review against the known stock baseline before assigning cause"
    printf '%s\n' "$kernel_warnings"
else
    pass "no Oops/BUG/KASAN/UBSAN/panic/call-trace signature was found in dmesg"
fi

section "pstore and loaded modules"
run_root "pstore" "ls -l /sys/fs/pstore 2>/dev/null || true"
run_root "loaded modules" "cat /proc/modules"

section "Radio and network state"
run_shell "network interfaces" "ip -o link show 2>/dev/null || ifconfig -a 2>/dev/null || true"
run_shell "Wi-Fi state" "cmd wifi status 2>/dev/null || dumpsys wifi 2>/dev/null | head -n 120 || true"
run_shell "Bluetooth state" "dumpsys bluetooth_manager 2>/dev/null | head -n 160 || true"
run_shell "cellular data interfaces" "ip -o link show 2>/dev/null | grep -Ei 'rmnet|ccmni|rmnet_data|rmnet_ipa' || true"

section "Storage, UFS, and swap state"
run_root "mounts" "mount"
run_root "partitions" "cat /proc/partitions"
run_root "UFS block devices" "ls -l /sys/block | grep -E 'ufs|sda|sdb|sd[a-z]' || true"
run_root "swap and ZRAM" "cat /proc/swaps; ls -l /sys/block/zram0 2>/dev/null || true; cat /sys/block/zram0/disksize 2>/dev/null || true"

section "Result"
printf '%s\n' 'This script is diagnostic only. A PASS means the specific observable check passed; it does not replace manual phone regression testing.'
printf '%s\n' 'If module ABI/signature lines or new Oops/BUG traces appear, preserve this output and restore the prior boot image rather than changing stock DLKM partitions.'
