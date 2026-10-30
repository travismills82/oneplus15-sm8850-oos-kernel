#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo=$(cd -- "$here/../../.." && pwd)
if [[ $# != 1 || $1 != /* ]]; then
    echo "Usage: $0 /absolute/disposable-output-directory" >&2
    exit 2
fi
out=$(realpath -m -- "$1")
case "$out" in "$repo/kernel_platform/common"|"/"|"/home"|"/home/travis")
    echo "Unsafe test output path" >&2; exit 2;;
esac
mkdir -p -- "$out/kernel"
src="$repo/kernel_platform/common"
make -C "$src" O="$out/kernel" ARCH=x86_64 \
    KCONFIG_ALLCONFIG="$here/vm.config" allnoconfig
for symbol in KASAN KASAN_GENERIC DEBUG_LIST SYSCTL PROC_SYSCTL \
    FILE_LOCKING MODVERSIONS BASIC_MODVERSIONS ACPI \
    DM_CRYPT BLK_DEV_LOOP NET_CLS_ACT NET_ACT_GACT NET_CLS_MATCHALL \
    NET_SCH_INGRESS TCP_CONG_CUBIC VETH NET_NS; do
    grep -qx "CONFIG_${symbol}=y" "$out/kernel/.config" || {
        echo "Required VM test configuration missing: $symbol" >&2; exit 1;
    }
done
make -C "$src" O="$out/kernel" ARCH=x86_64 -j"${JOBS:-1}" bzImage modules
printf '%s\n' "VM-only bzImage: $out/kernel/arch/x86/boot/bzImage"
sha256sum "$out/kernel/arch/x86/boot/bzImage" "$out/kernel/.config"
