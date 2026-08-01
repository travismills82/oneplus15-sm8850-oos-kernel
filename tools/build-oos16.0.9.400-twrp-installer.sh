#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Build the firmware-specific OnePlus 15 TWRP installer from an already
# validated boot.img and matching flattened system_dlkm ext4 image.

set -euo pipefail

readonly RELEASE_TAG='oos16.0.9.400-r1'
readonly FIRMWARE='OxygenOS 16.0.9.400(EX01)'
readonly TEMPLATE_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/twrp-installer-template"

usage() {
    cat <<'EOF'
Usage:
  tools/build-oos16.0.9.400-twrp-installer.sh BOOT_IMAGE SYSTEM_DLKM_IMAGE OUTPUT_ZIP

The inputs must be the matched OOS 16.0.9.400-r1 boot image and flattened
ext4 system_dlkm image. The output path must not already exist.
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

[ "$#" -eq 3 ] || {
    usage >&2
    exit 2
}

BOOT_IMAGE="$1"
SYSTEM_DLKM_IMAGE="$2"
OUTPUT_ZIP="$3"

for tool in awk dd dirname mkdir mktemp sed sha256sum stat tune2fs unzip zip; do
    command -v "$tool" >/dev/null 2>&1 || die "required tool is unavailable: $tool"
done

[ -f "$BOOT_IMAGE" ] && [ -r "$BOOT_IMAGE" ] ||
    die "boot image is not a readable regular file: $BOOT_IMAGE"
[ -f "$SYSTEM_DLKM_IMAGE" ] && [ -r "$SYSTEM_DLKM_IMAGE" ] ||
    die "system_dlkm image is not a readable regular file: $SYSTEM_DLKM_IMAGE"
[ -d "$TEMPLATE_DIR" ] || die "installer template is missing: $TEMPLATE_DIR"

OUTPUT_DIR="$(dirname -- "$OUTPUT_ZIP")"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(CDPATH= cd -- "$OUTPUT_DIR" && pwd -P)"
OUTPUT_ZIP="$OUTPUT_DIR/$(basename -- "$OUTPUT_ZIP")"
[ ! -e "$OUTPUT_ZIP" ] || die "refusing to overwrite existing output: $OUTPUT_ZIP"

BOOT_BYTES="$(file_size "$BOOT_IMAGE")"
SYSTEM_DLKM_BYTES="$(file_size "$SYSTEM_DLKM_IMAGE")"
is_decimal "$BOOT_BYTES" && [ "$BOOT_BYTES" -gt 0 ] || die 'boot image has an invalid size'
is_decimal "$SYSTEM_DLKM_BYTES" && [ "$SYSTEM_DLKM_BYTES" -gt 0 ] ||
    die 'system_dlkm image has an invalid size'
[ $((BOOT_BYTES % 4096)) -eq 0 ] || die 'boot image is not 4096-byte aligned'
[ $((SYSTEM_DLKM_BYTES % 4096)) -eq 0 ] ||
    die 'system_dlkm image is not 4096-byte aligned'

[ "$(dd if="$BOOT_IMAGE" bs=8 count=1 status=none 2>/dev/null)" = 'ANDROID!' ] ||
    die 'boot image does not have an Android boot header'
tune2fs -l "$SYSTEM_DLKM_IMAGE" >/dev/null 2>&1 ||
    die 'system_dlkm image is not a flattened ext4 filesystem image'

BOOT_SHA256="$(sha256sum "$BOOT_IMAGE" | awk '{print $1}')"
SYSTEM_DLKM_SHA256="$(sha256sum "$SYSTEM_DLKM_IMAGE" | awk '{print $1}')"

STAGE="$(mktemp -d "${TMPDIR:-/tmp}/oneplus15-twrp-installer.XXXXXX")"
mkdir -p "$STAGE/META-INF/com/google/android" "$STAGE/payload"

sed \
    -e "s|@RELEASE_TAG@|$RELEASE_TAG|g" \
    -e "s|@FIRMWARE@|$FIRMWARE|g" \
    -e "s|@BOOT_BYTES@|$BOOT_BYTES|g" \
    -e "s|@SYSTEM_DLKM_BYTES@|$SYSTEM_DLKM_BYTES|g" \
    -e "s|@BOOT_SHA256@|$BOOT_SHA256|g" \
    -e "s|@SYSTEM_DLKM_SHA256@|$SYSTEM_DLKM_SHA256|g" \
    "$TEMPLATE_DIR/META-INF/com/google/android/update-binary.in" \
    > "$STAGE/META-INF/com/google/android/update-binary"
chmod 0755 "$STAGE/META-INF/com/google/android/update-binary"

sed \
    -e "s|@RELEASE_TAG@|$RELEASE_TAG|g" \
    -e "s|@FIRMWARE@|$FIRMWARE|g" \
    -e "s|@BOOT_BYTES@|$BOOT_BYTES|g" \
    -e "s|@SYSTEM_DLKM_BYTES@|$SYSTEM_DLKM_BYTES|g" \
    -e "s|@BOOT_SHA256@|$BOOT_SHA256|g" \
    -e "s|@SYSTEM_DLKM_SHA256@|$SYSTEM_DLKM_SHA256|g" \
    "$TEMPLATE_DIR/README.txt.in" > "$STAGE/README.txt"
cp "$TEMPLATE_DIR/META-INF/com/google/android/updater-script" \
    "$STAGE/META-INF/com/google/android/updater-script"
cp "$BOOT_IMAGE" "$STAGE/payload/boot.img"
cp "$SYSTEM_DLKM_IMAGE" "$STAGE/payload/system_dlkm.flatten.ext4.img"

{
    printf 'release_tag=%s\n' "$RELEASE_TAG"
    printf 'firmware=%s\n' "$FIRMWARE"
    printf 'boot_bytes=%s\nboot_sha256=%s\n' "$BOOT_BYTES" "$BOOT_SHA256"
    printf 'system_dlkm_bytes=%s\nsystem_dlkm_sha256=%s\n' \
        "$SYSTEM_DLKM_BYTES" "$SYSTEM_DLKM_SHA256"
    printf 'logical_partition_minimum_bytes=%s\n' "$SYSTEM_DLKM_BYTES"
} > "$STAGE/INSTALLER-METADATA.txt"

(
    cd "$STAGE"
    sha256sum payload/boot.img payload/system_dlkm.flatten.ext4.img > SHA256SUMS
    zip -q -0 "$OUTPUT_ZIP" payload/boot.img payload/system_dlkm.flatten.ext4.img
    zip -q -9 "$OUTPUT_ZIP" \
        META-INF/com/google/android/update-binary \
        META-INF/com/google/android/updater-script \
        README.txt INSTALLER-METADATA.txt SHA256SUMS
)

printf '%s  %s\n' "$(sha256sum "$OUTPUT_ZIP" | awk '{print $1}')" \
    "$(basename -- "$OUTPUT_ZIP")" > "$OUTPUT_ZIP.sha256"

printf 'Created: %s\n' "$OUTPUT_ZIP"
printf 'SHA-256: %s\n' "$(awk '{print $1}' "$OUTPUT_ZIP.sha256")"
printf 'Staging directory retained for audit: %s\n' "$STAGE"
