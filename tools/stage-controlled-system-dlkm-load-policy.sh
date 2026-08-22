#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Repack an already-built controlled system_dlkm archive with the stock
# OxygenOS module-load policy filtered to the modules that still exist.  This
# is a packaging-only operation: it never rebuilds Image or a .ko file.

set -euo pipefail

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

usage() {
    cat <<'EOF'
Usage:
  tools/stage-controlled-system-dlkm-load-policy.sh \
    --staging-archive <system_dlkm_staging_archive.tar.gz> \
    --stock-modules-load <stock-system-dlkm/modules.load> \
    --base-image <controlled-system_dlkm.img> \
    --build-image <build_image> \
    --avbtool <avbtool> \
    --fsck-erofs <fsck.erofs> \
    --out-dir <directory>

The base image supplies the reviewed partition size and AVB descriptor
parameters. The output contains the same module binaries as the input staging
archive; only modules.load and filesystem/AVB packing metadata are regenerated.
EOF
}

staging_archive=
stock_modules_load=
base_image=
build_image=
avbtool=
fsck_erofs=
out_dir=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --staging-archive) staging_archive=${2:-}; shift 2 ;;
        --stock-modules-load) stock_modules_load=${2:-}; shift 2 ;;
        --base-image) base_image=${2:-}; shift 2 ;;
        --build-image) build_image=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --fsck-erofs) fsck_erofs=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown argument: $1" ;;
    esac
done

for path in "$staging_archive" "$stock_modules_load" "$base_image" \
            "$build_image" "$avbtool" "$fsck_erofs"; do
    [[ -f "$path" ]] || die "required input is missing: $path"
done
[[ -n "$out_dir" ]] || die "--out-dir is required"
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output directory: $out_dir"

staging_archive=$(realpath "$staging_archive")
stock_modules_load=$(realpath "$stock_modules_load")
base_image=$(realpath "$base_image")
build_image=$(realpath "$build_image")
avbtool=$(realpath "$avbtool")
fsck_erofs=$(realpath "$fsck_erofs")

mkdir -p "$out_dir/staging" "$out_dir/readback"
tar -xzf "$staging_archive" -C "$out_dir/staging"
modules_root="$out_dir/staging/flatten/lib/modules"
[[ -d "$modules_root" ]] || die "staging archive has no flattened module tree"

module_count=$(find "$modules_root" -maxdepth 1 -type f -name '*.ko' | wc -l)
((module_count > 0)) || die "staging archive contains no flattened modules"

declare -A available=()
while IFS= read -r path; do
    available[$(basename "$path")]=1
done < <(find "$modules_root" -maxdepth 1 -type f -name '*.ko' | sort)

: > "$out_dir/modules.load"
while IFS= read -r stock_entry; do
    stock_entry=${stock_entry##*/}
    [[ -n "$stock_entry" && ${stock_entry:0:1} != '#' ]] || continue
    if [[ -n ${available[$stock_entry]:-} ]]; then
        printf '%s\n' "$stock_entry" >> "$out_dir/modules.load"
    fi
done < "$stock_modules_load"

load_count=$(wc -l < "$out_dir/modules.load")
[[ "$load_count" -eq "$module_count" ]] ||
    die "stock policy covers $load_count of $module_count controlled modules"
cp -- "$out_dir/modules.load" "$modules_root/modules.load"

release_modules_dir=$(find "$out_dir/staging/lib/modules" -mindepth 1 -maxdepth 1 \
    -type d -print -quit)
[[ -n "$release_modules_dir" ]] || die "staging archive has no release module directory"
validator="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/validate-system-dlkm-load-contract.py"
[[ -x "$validator" ]] || die "load-contract validator is unavailable: $validator"
python3 "$validator" \
    --system-root "$modules_root" \
    --modules-builtin "$release_modules_dir/modules.builtin" \
    --required wwan.ko \
    --out "$out_dir/system-dlkm-load-contract.tsv"

avb_info=$($avbtool info_image --image "$base_image") ||
    die "unable to read base system_dlkm AVB footer"
field() {
    local label=$1
    local value
    value=$(sed -n "s/^${label}:[[:space:]]*//p" <<< "$avb_info" | head -1)
    [[ -n "$value" ]] || die "base AVB footer is missing: $label"
    printf '%s\n' "$value"
}

partition_size=$(field 'Image size')
partition_size=${partition_size%% *}
partition_name=$(field '      Partition Name')
hash_algorithm=$(field '      Hash Algorithm')
salt=$(field '      Salt')
fec_num_roots=$(field '      FEC num roots')
rollback_index=$(field 'Rollback Index')
rollback_index_location=$(field 'Rollback Index Location')
flags=$(field 'Flags')
[[ "$partition_name" == system_dlkm ]] ||
    die "base AVB descriptor authenticates $partition_name, not system_dlkm"

cat > "$out_dir/file_contexts" <<'EOF'
/system_dlkm(/.*)? u:object_r:system_dlkm_file:s0
EOF
cat > "$out_dir/system_dlkm.props" <<EOF
fs_type=erofs
use_dynamic_partition_size=true
mount_point=system_dlkm
selinux_fc=$(realpath "$out_dir/file_contexts")
erofs_default_compressor=lz4hc,9
erofs_big_pcluster=true
timestamp=0
EOF

tool_dir=$(dirname "$build_image")
export PATH="$tool_dir:$PATH"
"$build_image" \
    "$out_dir/staging/flatten" \
    "$out_dir/system_dlkm.props" \
    "$out_dir/system_dlkm.img" \
    /dev/null

$avbtool add_hashtree_footer \
    --image "$out_dir/system_dlkm.img" \
    --partition_size "$partition_size" \
    --partition_name "$partition_name" \
    --hash_algorithm "$hash_algorithm" \
    --salt "$salt" \
    --fec_num_roots "$fec_num_roots" \
    --algorithm NONE \
    --rollback_index "$rollback_index" \
    --rollback_index_location "$rollback_index_location" \
    --flags "$flags"

[[ $(stat -c '%s' "$out_dir/system_dlkm.img") -eq "$partition_size" ]] ||
    die "candidate image does not match the reviewed partition size"
$avbtool verify_image --image "$out_dir/system_dlkm.img" >/dev/null ||
    die "candidate system_dlkm AVB footer failed verification"
$fsck_erofs --xattrs "$out_dir/system_dlkm.img" > "$out_dir/fsck.erofs.txt" 2>&1 ||
    die "candidate system_dlkm failed EROFS validation"
$fsck_erofs --extract="$out_dir/readback" "$out_dir/system_dlkm.img" \
    >> "$out_dir/fsck.erofs.txt" 2>&1 ||
    die "candidate system_dlkm readback extraction failed"

cmp -s "$out_dir/modules.load" "$out_dir/readback/lib/modules/modules.load" ||
    die "modules.load readback differs"
while IFS= read -r source; do
    name=$(basename "$source")
    readback="$out_dir/readback/lib/modules/$name"
    [[ -f "$readback" ]] || die "readback is missing $name"
    cmp -s "$source" "$readback" || die "module payload changed while packing: $name"
done < <(find "$modules_root" -maxdepth 1 -type f -name '*.ko' | sort)

{
    printf 'base_image_sha256=%s\n' "$(sha256sum "$base_image" | awk '{print $1}')"
    printf 'candidate_image_sha256=%s\n' "$(sha256sum "$out_dir/system_dlkm.img" | awk '{print $1}')"
    printf 'candidate_image_size=%s\n' "$(stat -c '%s' "$out_dir/system_dlkm.img")"
    printf 'module_count=%s\n' "$module_count"
    printf 'modules_load_entries=%s\n' "$load_count"
    printf 'modules_load_policy=stock_order_filtered_to_controlled_modules\n'
    printf 'module_payload_changes=0\n'
    printf 'avb=partition_local_hashtree_fec_regenerated_parent_vbmeta_unchanged\n'
} > "$out_dir/manifest.txt"

python3 "$validator" \
    --system-root "$out_dir/readback/lib/modules" \
    --modules-builtin "$release_modules_dir/modules.builtin" \
    --required wwan.ko \
    --out "$out_dir/system-dlkm-load-contract.tsv"

(
    cd "$out_dir"
    sha256sum system_dlkm.img modules.load system-dlkm-load-contract.tsv \
        manifest.txt fsck.erofs.txt > SHA256SUMS
)

printf 'CONTROLLED SYSTEM_DLKM LOAD POLICY STAGED\n'
cat "$out_dir/manifest.txt"
