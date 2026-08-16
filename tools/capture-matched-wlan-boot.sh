#!/usr/bin/env bash
# Capture persistent host-side evidence across a matched-WLAN boot experiment.
#
# This tool is diagnostic only: it never writes to the phone, changes a
# property, reboots the device, or flashes a partition.  It keeps host files
# open while a separately performed staging/reboot test disconnects ADB, so the
# evidence survives a shutdown or reboot before Android is available again.

set -euo pipefail

usage() {
	cat <<'EOF'
Usage:
  tools/capture-matched-wlan-boot.sh [--serial <serial>] \
      [--out-dir <directory>] [--interval <seconds>]

Start this before staging a matched-WLAN candidate.  Leave it running through
the boot attempt; stop it with Ctrl-C after recovery/rollback is complete.

The script reads device state only.  It records a preflight snapshot, retries
live dmesg and logcat streams across ADB disconnects, and saves periodic
read-only snapshots including /sys/fs/pstore when the device is reachable.
EOF
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 2
}

adb_bin=${ADB:-adb}
serial=
out_dir=
interval=10

while [[ $# -gt 0 ]]; do
	case "$1" in
		--serial)
			serial=${2:-}
			shift 2
			;;
		--out-dir)
			out_dir=${2:-}
			shift 2
			;;
		--interval)
			interval=${2:-}
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			usage >&2
			die "unknown argument: $1"
			;;
	esac
done

command -v "$adb_bin" >/dev/null || die "adb is not available: $adb_bin"
[[ "$interval" =~ ^[1-9][0-9]*$ ]] || die "--interval must be a positive integer"

if [[ -z "$serial" ]]; then
	mapfile -t devices < <("$adb_bin" devices | awk 'NR > 1 && $2 == "device" { print $1 }')
	[[ ${#devices[@]} -eq 1 ]] || die "provide --serial when exactly one Android device is not connected"
	serial=${devices[0]}
fi

if [[ -z "$out_dir" ]]; then
	out_dir="$(pwd)/out/validation/matched-wlan-boot-$(date -u +%Y%m%dT%H%M%SZ)"
fi
[[ ! -e "$out_dir" ]] || die "output path already exists: $out_dir"
mkdir -p "$out_dir/snapshots"

events="$out_dir/events.log"
logcat_log="$out_dir/logcat-follow.txt"
dmesg_log="$out_dir/dmesg-follow.txt"
stream_errors="$out_dir/stream-errors.txt"
stopping=0
declare -a child_pids=()

adb_device() {
	"$adb_bin" -s "$serial" "$@"
}

event() {
	printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" >> "$events"
}

record_command() {
	local output=$1
	shift
	{
		printf '$'
		printf ' %q' "$@"
		printf '\n'
		local rc
		if "$@"; then
			rc=0
		else
			rc=$?
		fi
		printf '[exit=%s]\n' "$rc"
		return 0
	} >> "$output" 2>&1
}

snapshot() {
	local label=$1
	local snapshot_file="$out_dir/snapshots/${label}.txt"
	event "snapshot-start label=$label"
	{
		printf 'timestamp_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
		printf 'serial=%s\n\n' "$serial"
	} > "$snapshot_file"
	record_command "$snapshot_file" adb_device get-state
	record_command "$snapshot_file" adb_device shell getprop
	record_command "$snapshot_file" adb_device shell su -c 'uname -a; cat /proc/cmdline'
	record_command "$snapshot_file" adb_device shell su -c 'cat /proc/modules'
	record_command "$snapshot_file" adb_device shell su -c 'mount; df -h'
	record_command "$snapshot_file" adb_device shell su -c 'find /sys/fs/pstore -maxdepth 1 -type f -printf "%f %s bytes\n" 2>/dev/null; for f in /sys/fs/pstore/*; do [ -f "$f" ] || continue; echo "===== $f ====="; cat "$f"; done'
	record_command "$snapshot_file" adb_device shell su -c 'find /sys/kernel/debug -maxdepth 2 -type f -iname "*last*kmsg*" -o -iname "*ramoops*" 2>/dev/null'
	record_command "$snapshot_file" adb_device shell su -c 'dmesg'
	event "snapshot-finish label=$label"
}

stream_logcat() {
	while (( ! stopping )); do
		event "logcat-connect"
		local rc
		if "$adb_bin" -s "$serial" logcat -b all -v threadtime >> "$logcat_log" 2>> "$stream_errors"; then
			rc=0
		else
			rc=$?
		fi
		event "logcat-exit rc=$rc"
		sleep 1
	done
}

stream_dmesg() {
	while (( ! stopping )); do
		event "dmesg-follow-connect"
		local rc
		if "$adb_bin" -s "$serial" exec-out su -c 'exec dmesg -w' >> "$dmesg_log" 2>> "$stream_errors"; then
			rc=0
		else
			rc=$?
		fi
		event "dmesg-follow-exit rc=$rc"
		sleep 1
	done
}

periodic_snapshots() {
	local sequence=0
	while (( ! stopping )); do
		snapshot "periodic-$(printf '%04d' "$sequence")"
		sequence=$((sequence + 1))
		sleep "$interval"
	done
}

cleanup() {
	local rc=$?
	trap - EXIT INT TERM
	stopping=1
	event "capture-stop rc=$rc"
	for pid in "${child_pids[@]:-}"; do
		kill "$pid" 2>/dev/null || true
	done
	for pid in "${child_pids[@]:-}"; do
		wait "$pid" 2>/dev/null || true
	done
	snapshot "final" || true
	event "capture-finished"
	printf 'Capture saved in %s\n' "$out_dir"
	exit "$rc"
}

trap cleanup EXIT
trap 'exit 130' INT TERM

event "capture-start serial=$serial interval=$interval"
snapshot "preflight"
stream_logcat & child_pids+=("$!")
stream_dmesg & child_pids+=("$!")
periodic_snapshots & child_pids+=("$!")

printf 'Persistent matched-WLAN boot capture is running.\n'
printf 'output=%s\n' "$out_dir"
printf 'Leave this process running through staging and recovery; Ctrl-C stops it.\n'
wait "${child_pids[@]}"
