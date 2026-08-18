#!/usr/bin/env bash
# Repack a vendor_boot image without changing its ramdisk payload.
#
# This is intentionally a baseline tool: it preserves every extracted vendor
# ramdisk fragment byte-for-byte, retains the stock DTB and bootconfig, and
# reconstructs the stock unsigned local AVB hash footer from the stock image's
# own geometry.  It does not sign modules, edit a cpio archive, or change a
# parent/top-level vbmeta descriptor.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  repack-controlled-vendor-boot.sh \
      --stock-image <vendor_boot.img> \
      --out-dir <directory> \
      [--unpack-tool <unpack_bootimg.py>] \
      [--mkboot-tool <mkbootimg.py>] \
      [--avbtool <avbtool>]

The generated vendor_boot.img is required to be byte-identical to the supplied
stock image.  This is the safe first controlled-vendor_boot gate: a later
module-replacement stage must use a separate, explicitly reviewed packer.
EOF
}

die() {
    printf 'error: %s\n' "$*" >&2
    exit 1
}

stock_image=
out_dir=
unpack_tool=
mkboot_tool=
avbtool=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stock-image) stock_image=${2:?missing value}; shift 2 ;;
        --out-dir) out_dir=${2:?missing value}; shift 2 ;;
        --unpack-tool) unpack_tool=${2:?missing value}; shift 2 ;;
        --mkboot-tool) mkboot_tool=${2:?missing value}; shift 2 ;;
        --avbtool) avbtool=${2:?missing value}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ -n "$stock_image" && -f "$stock_image" ]] || die "--stock-image is required"
[[ -n "$out_dir" ]] || die "--out-dir is required"

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
unpack_tool=${unpack_tool:-"$repo_root/kernel_platform/tools/mkbootimg/unpack_bootimg.py"}
mkboot_tool=${mkboot_tool:-"$repo_root/kernel_platform/tools/mkbootimg/mkbootimg.py"}
avbtool=${avbtool:-"/home/travis/Android/twrp/out/host/linux-x86/bin/avbtool"}

[[ -f "$unpack_tool" ]] || die "missing unpack tool: $unpack_tool"
[[ -f "$mkboot_tool" ]] || die "missing mkboot tool: $mkboot_tool"
[[ -x "$avbtool" ]] || die "missing avbtool: $avbtool"

mkdir -p "$out_dir"
stock_unpacked="$out_dir/stock-unpacked"
candidate_unpacked="$out_dir/candidate-unpacked"
stock_info="$out_dir/stock-avb-info.txt"
candidate_info="$out_dir/candidate-avb-info.txt"
candidate="$out_dir/vendor_boot.img"

[[ ! -e "$stock_unpacked" ]] || die "refusing to overwrite $stock_unpacked"
[[ ! -e "$candidate_unpacked" ]] || die "refusing to overwrite $candidate_unpacked"
[[ ! -e "$candidate" ]] || die "refusing to overwrite $candidate"
mkdir -p "$stock_unpacked" "$candidate_unpacked"

"$avbtool" info_image --image "$stock_image" > "$stock_info"

mapfile -d '' -t mkboot_args < <(
    python3 "$unpack_tool" --boot_img "$stock_image" --out "$stock_unpacked" \
        --format=mkbootimg -0
)
python3 "$mkboot_tool" "${mkboot_args[@]}" --vendor_boot "$candidate"

python3 - "$avbtool" "$stock_info" "$candidate" <<'PY'
import re
import subprocess
import sys
from pathlib import Path

avbtool, info_path, candidate = sys.argv[1:]
info = Path(info_path).read_text(encoding="utf-8")

def one(pattern: str, label: str) -> str:
    matches = re.findall(pattern, info, re.M)
    if len(matches) != 1:
        raise SystemExit(f"expected exactly one {label}, found {len(matches)}")
    return matches[0]

partition_size = one(r"^Image size:\s+(\d+) bytes$", "partition size")
original_size = one(r"^Original image size:\s+(\d+) bytes$", "original image size")
algorithm = one(r"^Algorithm:\s+(\S+)$", "AVB algorithm")
partition_name = one(r"^      Partition Name:\s+(\S+)$", "partition name")
hash_algorithm = one(r"^      Hash Algorithm:\s+(\S+)$", "hash algorithm")
salt = one(r"^      Salt:\s+([0-9a-fA-F]+)$", "hash salt")
rollback_index = one(r"^Rollback Index:\s+(\d+)$", "rollback index")
flags = one(r"^Flags:\s+(\d+)$", "flags")
rollback_location = one(r"^Rollback Index Location:\s+(\d+)$", "rollback index location")
if algorithm != "NONE":
    raise SystemExit(f"unsupported stock local AVB algorithm: {algorithm}")
if hash_algorithm.lower() != "sha256":
    raise SystemExit(f"unsupported stock hash algorithm: {hash_algorithm}")
if Path(candidate).stat().st_size != int(original_size):
    raise SystemExit(
        f"mkbootimg produced {Path(candidate).stat().st_size} bytes, expected {original_size}"
    )

command = [
    avbtool,
    "add_hash_footer",
    "--image", candidate,
    "--partition_name", partition_name,
    "--partition_size", partition_size,
    "--algorithm", algorithm,
    "--salt", salt,
    "--rollback_index", rollback_index,
    "--rollback_index_location", rollback_location,
    "--flags", flags,
]
for key, value in re.findall(r"^    Prop: (.+?) -> '([^']*)'$", info, re.M):
    command.extend(["--prop", f"{key}:{value}"])
subprocess.run(command, check=True)
PY

"$avbtool" verify_image --image "$candidate"
"$avbtool" info_image --image "$candidate" > "$candidate_info"
python3 "$unpack_tool" --boot_img "$candidate" --out "$candidate_unpacked" --format=info \
    > "$out_dir/candidate-boot-info.txt"

for component in bootconfig dtb vendor_ramdisk00; do
    [[ -f "$stock_unpacked/$component" ]] || die "stock extraction missing $component"
    [[ -f "$candidate_unpacked/$component" ]] || die "candidate extraction missing $component"
    cmp -s "$stock_unpacked/$component" "$candidate_unpacked/$component" ||
        die "$component changed during baseline repack"
done
cmp -s "$stock_image" "$candidate" || die "baseline repack is not byte-identical"

{
    printf 'stock_image=%s\n' "$stock_image"
    printf 'stock_sha256=%s\n' "$(sha256sum "$stock_image" | awk '{print $1}')"
    printf 'candidate_image=%s\n' "$candidate"
    printf 'candidate_sha256=%s\n' "$(sha256sum "$candidate" | awk '{print $1}')"
    printf 'image_size=%s\n' "$(stat -c '%s' "$candidate")"
    printf 'roundtrip_byte_identical=yes\n'
    printf 'dtb_sha256=%s\n' "$(sha256sum "$candidate_unpacked/dtb" | awk '{print $1}')"
    printf 'bootconfig_sha256=%s\n' "$(sha256sum "$candidate_unpacked/bootconfig" | awk '{print $1}')"
    printf 'vendor_ramdisk00_sha256=%s\n' "$(sha256sum "$candidate_unpacked/vendor_ramdisk00" | awk '{print $1}')"
    printf 'avb=stock_geometry_and_unsigned_local_footer_reproduced\n'
} > "$out_dir/manifest.txt"

# Keep the image checksum beside the manifest so a later device-side staging
# step has one authoritative payload digest to compare before any partition is
# written.  This baseline packer intentionally accepts only a byte-identical
# result, therefore this also proves that the original vendor_boot descriptor
# remains valid without changing a parent vbmeta image.
sha256sum "$candidate" > "$out_dir/SHA256SUMS"

printf 'CONTROLLED VENDOR_BOOT BASELINE PASS\n'
printf 'image=%s\n' "$candidate"
printf 'sha256=%s\n' "$(sha256sum "$candidate" | awk '{print $1}')"
printf 'byte_identical=yes\n'
