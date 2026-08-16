#!/usr/bin/env bash
# Stage and sign a narrow source-built WLAN vendor-DLKM replacement closure.
#
# Inputs are read-only. The script copies the complete stock vendor_dlkm tree,
# replaces only validator-selected modules, strips debug information, then signs
# the complete protected-export closure with the key embedded in the matching
# Image. It intentionally retains all other stock modules byte-for-byte.

set -euo pipefail

usage() {
    cat <<'USAGE'
Usage:
  tools/stage-matched-wlan-vendor-dlkm.sh \
      --stock-vendor-root <extracted-stock-vendor-dlkm> \
      --stock-vendor-image <raw-stock-vendor_dlkm.img> \
      --kernel-build-dir <matching-kernel_aarch64-output> \
      --validation-dir <validator-output> \
      --out-dir <new-empty-output-directory> \
      --replacement <source-module.ko> [--replacement <source-module.ko> ...]

The validator output must PASS with the exact source modules supplied here.
This script does not update AVB metadata and is intentionally not a flasher.
USAGE
}

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

require_file() { [[ -f "$1" ]] || die "missing file: $1"; }
require_dir() { [[ -d "$1" ]] || die "missing directory: $1"; }

module_name() {
    local name
    name=$(modinfo -F name "$1")
    [[ -n "$name" ]] || die "could not read MODULE_NAME from $1"
    printf '%s\n' "$name"
}

has_signature() { [[ -n "$(modinfo -F signer "$1")" ]]; }

stock_root=
stock_image=
kernel_build_dir=
validation_dir=
out_dir=
declare -a replacements=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stock-vendor-root) stock_root=${2:-}; shift 2 ;;
        --stock-vendor-image) stock_image=${2:-}; shift 2 ;;
        --kernel-build-dir) kernel_build_dir=${2:-}; shift 2 ;;
        --validation-dir) validation_dir=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        --replacement) replacements+=("${2:-}"); shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown argument: $1" ;;
    esac
done

[[ -n "$stock_root" && -n "$stock_image" && -n "$kernel_build_dir" ]] || {
    usage >&2
    die "stock root, stock image, and kernel build directory are required"
}
[[ -n "$validation_dir" && -n "$out_dir" ]] || {
    usage >&2
    die "validation and output directories are required"
}
[[ ${#replacements[@]} -gt 0 ]] || die "at least one source replacement is required"

require_dir "$stock_root"
require_file "$stock_image"
require_dir "$kernel_build_dir"
require_dir "$validation_dir"
require_file "$validation_dir/summary.json"
require_file "$validation_dir/protected-export-signing-closure.tsv"
require_file "$kernel_build_dir/scripts/sign-file"
require_file "$kernel_build_dir/certs/signing_key.pem"
require_file "$kernel_build_dir/certs/signing_key.x509"
require_file "$kernel_build_dir/vmlinux"
command -v debugfs >/dev/null || die "debugfs is required"
command -v e2fsck >/dev/null || die "e2fsck is required"
command -v modinfo >/dev/null || die "modinfo is required"
[[ ! -e "$out_dir" ]] || die "output path already exists: $out_dir"

python3 - "$validation_dir/summary.json" <<'PY'
import json
import pathlib
import sys

summary = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
if summary.get("result") != "PASS":
    raise SystemExit("validator result is not PASS")
if summary.get("replacement_contract_failures") != 0:
    raise SystemExit("validator reported replacement-contract failures")
if summary.get("unresolved_imports") != 0 or summary.get("crc_mismatches") != 0:
    raise SystemExit("validator reported unresolved imports or CRC mismatches")
PY

strip_tool=$(command -v llvm-objcopy || true)
[[ -n "$strip_tool" && -x "$strip_tool" ]] || die "llvm-objcopy is required"

python3 - "$kernel_build_dir/certs/signing_key.x509" "$kernel_build_dir/vmlinux" <<'PY'
from pathlib import Path
import sys

certificate = Path(sys.argv[1]).read_bytes()
vmlinux = Path(sys.argv[2]).read_bytes()
if certificate not in vmlinux:
    raise SystemExit("the module-signing certificate is absent from the matching vmlinux")
PY

declare -A source_by_name=()
for replacement in "${replacements[@]}"; do
    require_file "$replacement"
    name=$(module_name "$replacement")
    [[ -z ${source_by_name[$name]+x} ]] || die "duplicate source replacement for $name"
    has_signature "$replacement" && die "replacement is already signed: $replacement"
    source_by_name[$name]=$replacement
done

declare -A closure_action=()
while IFS=$'\t' read -r module action _first_import _provider; do
    [[ "$module" == "module" ]] && continue
    [[ -n "$module" && -n "$action" ]] || die "malformed signing closure row"
    closure_action[$module]=$action
done < "$validation_dir/protected-export-signing-closure.tsv"

mapfile -t source_modules < <(printf '%s\n' "${!source_by_name[@]}" | LC_ALL=C sort)
mapfile -t closure_modules < <(printf '%s\n' "${!closure_action[@]}" | LC_ALL=C sort)

for module in "${source_modules[@]}"; do
    [[ ${closure_action[$module]:-} == "SOURCE_REPLACEMENT" ]] || {
        die "validator closure does not select $module as a source replacement"
    }
done
for module in "${closure_modules[@]}"; do
    case "${closure_action[$module]}" in
        SOURCE_REPLACEMENT)
            [[ -n ${source_by_name[$module]+x} ]] || {
                die "missing source replacement required by closure: $module"
            }
            ;;
        RE_SIGN_STOCK) ;;
        *) die "unknown closure action for $module: ${closure_action[$module]}" ;;
    esac
done

mkdir -p "$out_dir/staging"
cp -a "$stock_root/." "$out_dir/staging/"
stage_modules="$out_dir/staging/lib/modules"
require_dir "$stage_modules"

declare -A stage_path_by_name=()
while IFS= read -r -d '' module_path; do
    name=$(module_name "$module_path")
    [[ -z ${stage_path_by_name[$name]+x} ]] || {
        die "duplicate module name in staged stock tree: $name"
    }
    stage_path_by_name[$name]=$module_path
done < <(find "$stage_modules" -type f -name '*.ko' -print0 | sort -z)

for module in "${closure_modules[@]}"; do
    [[ -n ${stage_path_by_name[$module]+x} ]] || {
        die "closure module is absent from stock tree: $module"
    }
done

for module in "${source_modules[@]}"; do
    target=${stage_path_by_name[$module]}
    cp --preserve=mode,timestamps "${source_by_name[$module]}" "$target"
    "$strip_tool" --strip-debug "$target"
done

for module in "${closure_modules[@]}"; do
    target=${stage_path_by_name[$module]}
    has_signature "$target" && die "refusing to append a second signature to $target"
    "$kernel_build_dir/scripts/sign-file" sha1 \
        "$kernel_build_dir/certs/signing_key.pem" \
        "$kernel_build_dir/certs/signing_key.x509" \
        "$target"
    signer=$(modinfo -F signer "$target")
    [[ "$signer" == "Build time autogenerated kernel key" ]] || {
        die "unexpected signer for $module: ${signer:-<none>}"
    }
    [[ "$(modinfo -F sig_id "$target")" == "PKCS#7" ]] || {
        die "signature trailer missing for $module"
    }
done

find "$out_dir/staging" -type f -printf '%P\n' | sort > "$out_dir/staging-files.txt"
(
    cd "$out_dir/staging"
    find . -type f -print0 | sort -z | xargs -0 sha256sum
) > "$out_dir/staging-SHA256SUMS"

cp --reflink=auto "$stock_image" "$out_dir/vendor_dlkm.img"
[[ $(stat -c '%s' "$stock_image") == $(stat -c '%s' "$out_dir/vendor_dlkm.img") ]] || {
    die "candidate image size differs from the stock partition image"
}

for module in "${closure_modules[@]}"; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    debugfs -w -R "rm $relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
    debugfs -w -R "write $staged $relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
done

e2fsck -fn "$out_dir/vendor_dlkm.img" > "$out_dir/e2fsck.txt" 2>&1 || {
    sed -n '1,240p' "$out_dir/e2fsck.txt" >&2
    die "candidate vendor_dlkm filesystem validation failed"
}

mkdir -p "$out_dir/readback"
for module in "${closure_modules[@]}"; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    readback="$out_dir/readback/${module}.ko"
    debugfs -R "dump $relative $readback" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
    [[ "$(sha256sum "$staged" | awk '{print $1}')" == "$(sha256sum "$readback" | awk '{print $1}')" ]] || {
        die "read-back hash mismatch for $module"
    }
done

{
    printf 'stock_vendor_root=%s\n' "$stock_root"
    printf 'stock_vendor_image=%s\n' "$stock_image"
    printf 'stock_vendor_image_sha256=%s\n' "$(sha256sum "$stock_image" | awk '{print $1}')"
    printf 'candidate_vendor_image=%s\n' "$out_dir/vendor_dlkm.img"
    printf 'candidate_vendor_image_sha256=%s\n' "$(sha256sum "$out_dir/vendor_dlkm.img" | awk '{print $1}')"
    printf 'candidate_vendor_image_bytes=%s\n' "$(stat -c '%s' "$out_dir/vendor_dlkm.img")"
    printf 'kernel_build_dir=%s\n' "$kernel_build_dir"
    printf 'kernel_signing_certificate_sha256=%s\n' \
        "$(openssl x509 -inform DER -in "$kernel_build_dir/certs/signing_key.x509" -noout -fingerprint -sha256 | cut -d= -f2)"
    printf 'kernel_signing_certificate_present_in_vmlinux=yes\n'
    printf 'source_replacements=%s\n' "${#source_by_name[@]}"
    printf 'signed_closure_modules=%s\n' "${#closure_action[@]}"
    printf 'metadata_policy=retained_stock_metadata; module_names_and_dependencies_are_contract-matched\n'
    printf 'avb=not_updated; static_candidate_only\n'
} > "$out_dir/manifest.txt"

(
    cd "$out_dir"
    sha256sum vendor_dlkm.img manifest.txt staging-files.txt staging-SHA256SUMS e2fsck.txt
) > "$out_dir/SHA256SUMS"

printf 'STAGED MATCHED WLAN VENDOR-DLKM CANDIDATE\n'
printf 'image=%s\n' "$out_dir/vendor_dlkm.img"
printf 'manifest=%s\n' "$out_dir/manifest.txt"
printf 'signed_closure_modules=%s\n' "${#closure_action[@]}"
printf 'AVB metadata was not changed; this output is not flashable without a separately validated AVB plan.\n'
