#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Build the firmware-specific OnePlus 15 boot-only TWRP installer from an
# already validated boot.img. The installer deliberately retains the stock
# OxygenOS EROFS system_dlkm partition.

set -euo pipefail

readonly RELEASE_TAG='oos16.0.9.400-r6'
readonly FIRMWARE='OxygenOS 16.0.9.400(EX01)'
readonly DEVICE='OnePlus 15 / CPH2747 / Canoe'
readonly BOOT_PARTITION_BYTES='100663296'
readonly STOCK_SYSTEM_DLKM_BYTES='14131200'
readonly STOCK_SYSTEM_DLKM_BLOCKS='3450'
readonly STOCK_SYSTEM_DLKM_SHA256='18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7'
readonly STOCK_SYSTEM_DLKM_EROFS_MAGIC='e2e1f5e0'
readonly TEMPLATE_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/twrp-installer-template"
readonly REPO_ROOT="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"

usage() {
    cat <<'EOF'
Usage:
  KERNEL_RELEASE=<uname -r> tools/build-oos16.0.9.400-twrp-installer.sh \
      BOOT_IMAGE OUTPUT_ZIP

The input must be the validated OOS 16.0.9.400 boot image. The output path
must not already exist. The resulting ZIP contains only boot.img and verifies
the active stock EROFS system_dlkm partition before it writes boot.
EOF
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

file_size() {
    stat -c '%s' "$1"
}

is_decimal() {
    case "$1" in
        ''|*[!0-9]*) return 1 ;;
        *) return 0 ;;
    esac
}

[ "$#" -eq 2 ] || {
    usage >&2
    exit 2
}

BOOT_IMAGE="$1"
OUTPUT_ZIP="$2"
KERNEL_RELEASE="${KERNEL_RELEASE:-}"
[ -n "$KERNEL_RELEASE" ] ||
    die 'KERNEL_RELEASE must be set to the validated uname -r value.'

for tool in awk dd dirname git mkdir mktemp sed sha256sum stat zip; do
    command -v "$tool" >/dev/null 2>&1 || die "required tool is unavailable: $tool"
done

[ -f "$BOOT_IMAGE" ] && [ -r "$BOOT_IMAGE" ] ||
    die "boot image is not a readable regular file: $BOOT_IMAGE"
[ -d "$TEMPLATE_DIR" ] || die "installer template is missing: $TEMPLATE_DIR"
SOURCE_COMMIT_REF="${KERNEL_SOURCE_COMMIT:-HEAD}"
SOURCE_COMMIT="$(git -C "$REPO_ROOT" rev-parse --verify "${SOURCE_COMMIT_REF}^{commit}" 2>/dev/null)" ||
    die "could not resolve KERNEL_SOURCE_COMMIT '$SOURCE_COMMIT_REF' from $REPO_ROOT"

OUTPUT_DIR="$(dirname -- "$OUTPUT_ZIP")"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(CDPATH= cd -- "$OUTPUT_DIR" && pwd -P)"
OUTPUT_ZIP="$OUTPUT_DIR/$(basename -- "$OUTPUT_ZIP")"
[ ! -e "$OUTPUT_ZIP" ] || die "refusing to overwrite existing output: $OUTPUT_ZIP"

BOOT_BYTES="$(file_size "$BOOT_IMAGE")"
is_decimal "$BOOT_BYTES" && [ "$BOOT_BYTES" -gt 0 ] || die 'boot image has an invalid size'
[ $((BOOT_BYTES % 4096)) -eq 0 ] || die 'boot image is not 4096-byte aligned'
[ "$BOOT_BYTES" -le "$BOOT_PARTITION_BYTES" ] ||
    die "boot image ($BOOT_BYTES) exceeds configured boot partition ($BOOT_PARTITION_BYTES)"

[ "$(dd if="$BOOT_IMAGE" bs=8 count=1 status=none 2>/dev/null)" = 'ANDROID!' ] ||
    die 'boot image does not have an Android boot header'

BOOT_SHA256="$(sha256sum "$BOOT_IMAGE" | awk '{print $1}')"

STAGE="$(mktemp -d "${TMPDIR:-/tmp}/oneplus15-twrp-installer.XXXXXX")"
mkdir -p "$STAGE/META-INF/com/google/android"

sed \
    -e "s|@RELEASE_TAG@|$RELEASE_TAG|g" \
    -e "s|@FIRMWARE@|$FIRMWARE|g" \
    -e "s|@DEVICE@|$DEVICE|g" \
    -e "s|@BOOT_BYTES@|$BOOT_BYTES|g" \
    -e "s|@BOOT_PARTITION_BYTES@|$BOOT_PARTITION_BYTES|g" \
    -e "s|@BOOT_SHA256@|$BOOT_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BYTES@|$STOCK_SYSTEM_DLKM_BYTES|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BLOCKS@|$STOCK_SYSTEM_DLKM_BLOCKS|g" \
    -e "s|@STOCK_SYSTEM_DLKM_SHA256@|$STOCK_SYSTEM_DLKM_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_EROFS_MAGIC@|$STOCK_SYSTEM_DLKM_EROFS_MAGIC|g" \
    -e "s|@KERNEL_RELEASE@|$KERNEL_RELEASE|g" \
    -e "s|@SOURCE_COMMIT@|$SOURCE_COMMIT|g" \
    "$TEMPLATE_DIR/META-INF/com/google/android/update-binary.in" \
    > "$STAGE/META-INF/com/google/android/update-binary"
chmod 0755 "$STAGE/META-INF/com/google/android/update-binary"

sed \
    -e "s|@RELEASE_TAG@|$RELEASE_TAG|g" \
    -e "s|@FIRMWARE@|$FIRMWARE|g" \
    -e "s|@DEVICE@|$DEVICE|g" \
    -e "s|@BOOT_BYTES@|$BOOT_BYTES|g" \
    -e "s|@BOOT_PARTITION_BYTES@|$BOOT_PARTITION_BYTES|g" \
    -e "s|@BOOT_SHA256@|$BOOT_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BYTES@|$STOCK_SYSTEM_DLKM_BYTES|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BLOCKS@|$STOCK_SYSTEM_DLKM_BLOCKS|g" \
    -e "s|@STOCK_SYSTEM_DLKM_SHA256@|$STOCK_SYSTEM_DLKM_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_EROFS_MAGIC@|$STOCK_SYSTEM_DLKM_EROFS_MAGIC|g" \
    -e "s|@KERNEL_RELEASE@|$KERNEL_RELEASE|g" \
    -e "s|@SOURCE_COMMIT@|$SOURCE_COMMIT|g" \
    "$TEMPLATE_DIR/README.txt.in" > "$STAGE/README.txt"

sed \
    -e "s|@RELEASE_TAG@|$RELEASE_TAG|g" \
    -e "s|@FIRMWARE@|$FIRMWARE|g" \
    -e "s|@DEVICE@|$DEVICE|g" \
    -e "s|@BOOT_BYTES@|$BOOT_BYTES|g" \
    -e "s|@BOOT_PARTITION_BYTES@|$BOOT_PARTITION_BYTES|g" \
    -e "s|@BOOT_SHA256@|$BOOT_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BYTES@|$STOCK_SYSTEM_DLKM_BYTES|g" \
    -e "s|@STOCK_SYSTEM_DLKM_BLOCKS@|$STOCK_SYSTEM_DLKM_BLOCKS|g" \
    -e "s|@STOCK_SYSTEM_DLKM_SHA256@|$STOCK_SYSTEM_DLKM_SHA256|g" \
    -e "s|@STOCK_SYSTEM_DLKM_EROFS_MAGIC@|$STOCK_SYSTEM_DLKM_EROFS_MAGIC|g" \
    -e "s|@KERNEL_RELEASE@|$KERNEL_RELEASE|g" \
    -e "s|@SOURCE_COMMIT@|$SOURCE_COMMIT|g" \
    "$TEMPLATE_DIR/kernel-info.txt.in" > "$STAGE/kernel-info.txt"

cp "$TEMPLATE_DIR/META-INF/com/google/android/updater-script" \
    "$STAGE/META-INF/com/google/android/updater-script"
cp "$BOOT_IMAGE" "$STAGE/boot.img"

(
    cd "$STAGE"
    sha256sum \
        boot.img \
        kernel-info.txt \
        README.txt \
        META-INF/com/google/android/update-binary \
        META-INF/com/google/android/updater-script \
        > checksums.sha256
    zip -q -0 "$OUTPUT_ZIP" boot.img
    zip -q -9 "$OUTPUT_ZIP" \
        META-INF/com/google/android/update-binary \
        META-INF/com/google/android/updater-script \
        kernel-info.txt README.txt checksums.sha256
)

printf '%s  %s\n' "$(sha256sum "$OUTPUT_ZIP" | awk '{print $1}')" \
    "$(basename -- "$OUTPUT_ZIP")" > "$OUTPUT_ZIP.sha256"

printf 'Created: %s\n' "$OUTPUT_ZIP"
printf 'SHA-256: %s\n' "$(awk '{print $1}' "$OUTPUT_ZIP.sha256")"
printf 'Staging directory retained for audit: %s\n' "$STAGE"
