#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Assemble a non-flashing controlled-v1 OxygenOS kernel stack.  This tool is
# deliberately a packaging boundary: it only reads validated inputs and writes
# a new release directory.  It contains no adb, fastboot, dd, TWRP, or block
# device operation.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  tools/package-controlled-oos-stack.sh \
      --boot <boot.img> \
      --vendor-boot <lossless-vendor_boot.img> \
      --system-dlkm <system_dlkm.erofs.img> \
      --vendor-dlkm <vendor_dlkm.ext4.img> \
      --kernel-build-dir <kernel_aarch64-output> \
      --vendor-validation-dir <matched-vendor-dlkm-validation-dir> \
      --vendor-stage-dir <staged-vendor-dlkm-output-dir> \
      --vendor-boot-validation-dir <vendor-boot-validation-dir> \
      --retained-stock-contract <tsv> \
      --retained-dependency-report <tsv> \
      --validation-report <txt> \
      --vendor-boot-baseline-sha256 <sha256> \
      --kernel-source-id <40-hex-source-id> \
      [--kernel-commit <git-sha>] [--out-dir <directory>]

The default output is out/controlled-oos-signing-v1.  The output directory
must not exist.  This helper cannot flash a partition and deliberately does
not package private signing material or vbmeta.
EOF
}

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

require_file() { [[ -f "$1" ]] || die "missing regular file: $1"; }
require_dir() { [[ -d "$1" ]] || die "missing directory: $1"; }
sha256() { sha256sum "$1" | awk '{print $1}'; }
size_bytes() { stat -c '%s' "$1"; }

magic_hex() {
    od -An -tx1 -N8 "$1" | tr -d ' \n'
}

require_magic() {
    local label=$1 file=$2 expected=$3 actual
    actual=$(magic_hex "$file")
    [[ "$actual" == "$expected" ]] || die "$label has unexpected header magic: $actual"
}

require_erofs() {
    local actual
    actual=$(od -An -tx4 -j 1024 -N4 "$1" | tr -d ' \n')
    [[ "$actual" == e0f5e1e2 ]] || die "system_dlkm is not an EROFS image"
}

require_ext4() {
    local actual
    actual=$(od -An -tx2 -j $((1024 + 56)) -N2 "$1" | tr -d ' \n')
    [[ "$actual" == ef53 ]] || die "vendor_dlkm is not an ext4 image"
}

boot=
vendor_boot=
system_dlkm=
vendor_dlkm=
kernel_build_dir=
vendor_validation_dir=
vendor_stage_dir=
vendor_boot_validation_dir=
retained_stock_contract=
retained_dependency_report=
validation_report=
vendor_boot_baseline_sha256=
kernel_source_id=
kernel_commit=
out_dir=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --boot) boot=${2:-}; shift 2 ;;
        --vendor-boot) vendor_boot=${2:-}; shift 2 ;;
        --system-dlkm) system_dlkm=${2:-}; shift 2 ;;
        --vendor-dlkm) vendor_dlkm=${2:-}; shift 2 ;;
        --kernel-build-dir) kernel_build_dir=${2:-}; shift 2 ;;
        --vendor-validation-dir) vendor_validation_dir=${2:-}; shift 2 ;;
        --vendor-stage-dir) vendor_stage_dir=${2:-}; shift 2 ;;
        --vendor-boot-validation-dir) vendor_boot_validation_dir=${2:-}; shift 2 ;;
        --retained-stock-contract) retained_stock_contract=${2:-}; shift 2 ;;
        --retained-dependency-report) retained_dependency_report=${2:-}; shift 2 ;;
        --validation-report) validation_report=${2:-}; shift 2 ;;
        --vendor-boot-baseline-sha256) vendor_boot_baseline_sha256=${2:-}; shift 2 ;;
        --kernel-source-id) kernel_source_id=${2:-}; shift 2 ;;
        --kernel-commit) kernel_commit=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown argument: $1" ;;
    esac
done

for value in "$boot" "$vendor_boot" "$system_dlkm" "$vendor_dlkm" \
             "$kernel_build_dir" "$vendor_validation_dir" \
             "$vendor_stage_dir" \
             "$vendor_boot_validation_dir" "$retained_stock_contract" \
             "$retained_dependency_report" "$validation_report" \
             "$vendor_boot_baseline_sha256" "$kernel_source_id"; do
    [[ -n "$value" ]] || { usage >&2; die "a required argument is missing"; }
done

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
out_dir=${out_dir:-"$repo_root/out/controlled-oos-signing-v1"}
kernel_commit=${kernel_commit:-"$(git -C "$repo_root" rev-parse HEAD)"}

for command in awk cp find git grep modinfo od openssl python3 sed sha256sum sort stat xargs; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done

require_file "$boot"
require_file "$vendor_boot"
require_file "$system_dlkm"
require_file "$vendor_dlkm"
require_dir "$kernel_build_dir"
# Kleaf's kernel_build target and kernel_config target have separate output
# trees.  The former owns Image/vmlinux/Module.symvers; the latter is the
# authoritative generated .config used to build them.
kernel_config="${kernel_build_dir}_config/out_dir/.config"
require_file "$kernel_config"
require_file "$kernel_build_dir/include/config/kernel.release"
require_file "$kernel_build_dir/Module.symvers"
require_file "$kernel_build_dir/System.map"
require_file "$kernel_build_dir/vmlinux"
require_file "$kernel_build_dir/Image"
require_file "$kernel_build_dir/certs/signing_key.x509"
require_dir "$vendor_validation_dir"
require_file "$vendor_validation_dir/summary.json"
require_file "$vendor_validation_dir/protected-export-signing-closure.tsv"
require_dir "$vendor_stage_dir"
require_file "$vendor_stage_dir/vendor_dlkm.img"
require_file "$vendor_stage_dir/manifest.txt"
require_file "$vendor_stage_dir/vendor-system-dependency-reconciliation.tsv"
require_dir "$vendor_boot_validation_dir"
require_file "$vendor_boot_validation_dir/module-inventory.tsv"
require_file "$vendor_boot_validation_dir/dependency-report.tsv"
require_file "$vendor_boot_validation_dir/validation-report.txt"
require_file "$retained_stock_contract"
require_file "$retained_dependency_report"
require_file "$validation_report"
[[ "$vendor_boot_baseline_sha256" =~ ^[0-9a-f]{64}$ ]] ||
    die "vendor_boot baseline SHA-256 must be lowercase hexadecimal"
[[ "$kernel_source_id" =~ ^[0-9a-f]{40}$ ]] ||
    die "kernel source identity must be a full lowercase 40-hex identifier"
[[ ! -e "$out_dir" ]] || die "refusing to overwrite existing output: $out_dir"

require_magic boot "$boot" 414e44524f494421
require_magic vendor_boot "$vendor_boot" 564e4452424f4f54
require_erofs "$system_dlkm"
require_ext4 "$vendor_dlkm"
[[ "$(sha256 "$vendor_dlkm")" == "$(sha256 "$vendor_stage_dir/vendor_dlkm.img")" ]] ||
    die "vendor_dlkm does not match the validated staged vendor image"

kernel_release=$(<"$kernel_build_dir/include/config/kernel.release")
[[ -n "$kernel_release" ]] || die "kernel release is empty"
short_source_id=${kernel_source_id:0:12}
[[ "$kernel_release" == *"-g${short_source_id}-4k" ]] ||
    die "kernel release does not encode the declared source identity: $kernel_release"
toolchain_id=$(sed -n 's/^CONFIG_CC_VERSION_TEXT="\(.*\)"$/\1/p' "$kernel_config")
[[ -n "$toolchain_id" ]] || die "generated config does not identify the compiler toolchain"
for setting in \
    'CONFIG_MODULE_SIG=y' \
    'CONFIG_MODULE_SIG_PROTECT=y' \
    'CONFIG_MODVERSIONS=y' \
    'CONFIG_GENDWARFKSYMS=y'; do
    grep -qx "$setting" "$kernel_config" ||
        die "controlled-v1 config requirement is absent: $setting"
done

project_cert="$repo_root/kernel_platform/common/certs/controlled_oos_signing_v1.pem"
stock_cert="$repo_root/kernel_platform/common/certs/oos16_0_9_400_stock_gki_module_signing.pem"
trusted_bundle="$repo_root/kernel_platform/common/certs/controlled_oos_signing_v1_trusted.pem"
for certificate in "$project_cert" "$stock_cert" "$trusted_bundle"; do
    require_file "$certificate"
done
project_cert_der_sha256=$(openssl x509 -in "$project_cert" -outform DER | sha256sum | awk '{print $1}')
stock_cert_der_sha256=$(openssl x509 -in "$stock_cert" -outform DER | sha256sum | awk '{print $1}')
kernel_signing_der_sha256=$(sha256 "$kernel_build_dir/certs/signing_key.x509")
[[ "$kernel_signing_der_sha256" == "$project_cert_der_sha256" ]] ||
    die "Image build signing certificate is not controlled-v1"
project_signer=$(openssl x509 -in "$project_cert" -noout -subject -nameopt compat |
    sed -n 's#^subject=/CN=\([^/]*\).*#\1#p')
[[ -n "$project_signer" ]] || die "could not derive the controlled-v1 certificate common name"
trusted_bundle_sha256=$(sha256 "$trusted_bundle")
[[ $(openssl crl2pkcs7 -nocrl -certfile "$trusted_bundle" | openssl pkcs7 -print_certs -noout | grep -c '^subject=') == 2 ]] ||
    die "controlled-v1 trusted certificate bundle must contain exactly two certificates"

[[ $(sha256 "$vendor_boot") == "$vendor_boot_baseline_sha256" ]] ||
    die "vendor_boot is not the approved lossless baseline"

python3 - "$vendor_validation_dir/summary.json" "$kernel_release" \
    "$vendor_stage_dir/manifest.txt" <<'PY'
import json
import pathlib
import sys

summary = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
if summary.get("result") != "PASS":
    raise SystemExit("vendor-DLKM validation result is not PASS")
for field in ("replacement_contract_failures", "unresolved_imports", "crc_mismatches"):
    if summary.get(field) != 0:
        raise SystemExit(f"vendor-DLKM validation reports {field}={summary.get(field)}")

manifest = pathlib.Path(sys.argv[3])
if manifest.is_file():
    values = dict(
        line.split("=", 1)
        for line in manifest.read_text(encoding="utf-8").splitlines()
        if "=" in line
    )
    if values.get("kernel_release") != sys.argv[2]:
        raise SystemExit("vendor-DLKM manifest kernel release does not match Image")
PY

python3 - "$retained_stock_contract" <<'PY'
import csv
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
rows = list(csv.DictReader(path.open(encoding="utf-8"), delimiter="\t"))
if not rows:
    raise SystemExit("retained-stock contract has no module rows")
required = {"module", "partition", "classification", "unresolved_imports", "crc_mismatches", "protected_export_failures"}
if not required.issubset(rows[0]):
    raise SystemExit("retained-stock contract is missing required columns")
for row in rows:
    if row["classification"] == "BLOCKER":
        raise SystemExit(f"retained stock module is a blocker: {row['partition']}:{row['module']}")
    # A DORMANT module is not named by any supported normal or recovery
    # modules.load path.  Keep its raw compatibility evidence for review, but
    # do not turn a deliberately prohibited module into an active contract.
    if row["classification"] == "DORMANT":
        continue
    for key in ("unresolved_imports", "crc_mismatches", "protected_export_failures"):
        if row[key] != "0":
            raise SystemExit(f"retained stock module contract failure: {row['partition']}:{row['module']} {key}={row[key]}")
PY

mkdir -p "$out_dir/module-inventory/vendor-dlkm" \
         "$out_dir/module-inventory/vendor-boot" \
         "$out_dir/module-inventory/retained-stock"
out_dir=$(cd "$out_dir" && pwd)

cp -- "$boot" "$out_dir/boot.img"
cp -- "$vendor_boot" "$out_dir/vendor_boot.img"
cp -- "$system_dlkm" "$out_dir/system_dlkm.img"
cp -- "$vendor_dlkm" "$out_dir/vendor_dlkm.img"

# Only portable, reviewable output evidence belongs in a package.  In
# particular, neither the temporary staging tree nor signing_key.pem is ever
# copied here.
for file in summary.json protected-export-signing-closure.tsv \
            replacement-contracts.tsv import-resolution.tsv \
            external-signed-provider-edges.tsv candidate-modules.tsv; do
    [[ ! -f "$vendor_validation_dir/$file" ]] ||
        cp -- "$vendor_validation_dir/$file" "$out_dir/module-inventory/vendor-dlkm/$file"
done
for file in vendor-system-dependency-reconciliation.tsv staging-files.txt \
            staging-SHA256SUMS e2fsck.txt; do
    [[ ! -f "$vendor_stage_dir/$file" ]] ||
        cp -- "$vendor_stage_dir/$file" "$out_dir/module-inventory/vendor-dlkm/$file"
done
if [[ -f "$vendor_stage_dir/manifest.txt" ]]; then
    awk -F= '
        BEGIN {
            allowed["stock_vendor_image_sha256"] = 1
            allowed["candidate_vendor_image_sha256"] = 1
            allowed["candidate_vendor_image_bytes"] = 1
            allowed["stock_vendor_avb_root_digest"] = 1
            allowed["candidate_vendor_avb_root_digest"] = 1
            allowed["vendor_avb_footer"] = 1
            allowed["kernel_release"] = 1
            allowed["system_dlkm_staging_archive_sha256"] = 1
            allowed["modules_dep_reconciliation"] = 1
            allowed["modules_dep_pruned_builtin_edges"] = 1
            allowed["modules_dep_retained_system_edges"] = 1
            allowed["kernel_signing_certificate_der_sha256"] = 1
            allowed["kernel_signing_certificate_subject"] = 1
            allowed["kernel_signing_certificate_present_in_vmlinux"] = 1
            allowed["source_replacements"] = 1
            allowed["signed_closure_modules"] = 1
            allowed["metadata_policy"] = 1
            allowed["avb"] = 1
        }
        allowed[$1] { print }
    ' "$vendor_stage_dir/manifest.txt" \
        > "$out_dir/module-inventory/vendor-dlkm/manifest.txt"
fi
cp -- "$vendor_boot_validation_dir/module-inventory.tsv" \
    "$out_dir/module-inventory/vendor-boot/module-inventory.tsv"
cp -- "$vendor_boot_validation_dir/dependency-report.tsv" \
    "$out_dir/module-inventory/vendor-boot/dependency-report.tsv"
cp -- "$vendor_boot_validation_dir/validation-report.txt" \
    "$out_dir/module-inventory/vendor-boot/validation-report.txt"
[[ ! -f "$vendor_boot_validation_dir/replacement-report.tsv" ]] ||
    cp -- "$vendor_boot_validation_dir/replacement-report.tsv" \
        "$out_dir/module-inventory/vendor-boot/replacement-report.tsv"
cp -- "$retained_stock_contract" \
    "$out_dir/module-inventory/retained-stock/retained-stock-contract.tsv"
cp -- "$retained_stock_contract" "$out_dir/retained-stock-contract.tsv"
cp -- "$retained_dependency_report" "$out_dir/dependency-report.tsv"
cp -- "$validation_report" "$out_dir/validation-report.txt"

custom_module_count=$(awk -F '\t' 'NR > 1 { count++ } END { print count + 0 }' \
    "$vendor_validation_dir/protected-export-signing-closure.tsv")
retained_stock_module_count=$(awk -F '\t' 'NR > 1 { count++ } END { print count + 0 }' \
    "$retained_stock_contract")

custom_module_contract="$out_dir/custom-module-contract.tsv"
printf 'module\taction\tsha256\tvermagic\tsigner\tsignature_id\n' > "$custom_module_contract"
source_example=$(awk -F '\t' 'NR > 1 && $2 == "SOURCE_REPLACEMENT" { print $1; exit }' \
    "$vendor_validation_dir/protected-export-signing-closure.tsv")
[[ -n "$source_example" ]] || die "controlled closure has no source replacement"
source_example_vermagic=$(modinfo -F vermagic "$vendor_stage_dir/readback/$source_example.ko")
[[ "$source_example_vermagic" == "$kernel_release "* ]] ||
    die "source replacement example does not match $kernel_release"
expected_vermagic_tail=${source_example_vermagic#* }
while IFS=$'\t' read -r module action _first_import _signed_provider; do
    [[ "$module" == module ]] && continue
    [[ -n "$module" && -n "$action" ]] || die "malformed controlled module closure row"
    staged_module="$vendor_stage_dir/readback/$module.ko"
    require_file "$staged_module"
    staged_vermagic=$(modinfo -F vermagic "$staged_module")
    staged_signer=$(modinfo -F signer "$staged_module")
    staged_sig_id=$(modinfo -F sig_id "$staged_module")
    case "$action" in
        SOURCE_REPLACEMENT)
            [[ "$staged_vermagic" == "$kernel_release $expected_vermagic_tail" ]] ||
                die "source replacement $module does not match $kernel_release"
            ;;
        RE_SIGN_STOCK)
            [[ "$staged_vermagic" == *" $expected_vermagic_tail" ]] ||
                die "re-signed stock module $module has an incompatible vermagic contract"
            ;;
        *) die "unknown controlled module action for $module: $action" ;;
    esac
    [[ "$staged_signer" == "$project_signer" ]] ||
        die "controlled module $module has unexpected signer: ${staged_signer:-<none>}"
    [[ "$staged_sig_id" == PKCS#7 ]] ||
        die "controlled module $module has no PKCS#7 signature"
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$module" "$action" "$(sha256 "$staged_module")" "$staged_vermagic" \
        "$staged_signer" "$staged_sig_id" >> "$custom_module_contract"
done < "$vendor_validation_dir/protected-export-signing-closure.tsv"

cat > "$out_dir/module-signing-report.txt" <<EOF
CONTROLLED-V1 MODULE SIGNING REPORT
kernel_release=$kernel_release
kernel_source_id=$kernel_source_id
project_certificate_der_sha256=$kernel_signing_der_sha256
stock_certificate_der_sha256=$stock_cert_der_sha256
trusted_bundle_sha256=$trusted_bundle_sha256
trusted_bundle_certificates=2
CONFIG_MODULE_SIG=y
CONFIG_MODULE_SIG_PROTECT=y
CONFIG_MODVERSIONS=y
CONFIG_GENDWARFKSYMS=y
controlled_vendor_modules=$custom_module_count
controlled_vendor_signature_failures=0
EOF

python3 - "$out_dir/release-contract.json" \
    "$kernel_commit" "$kernel_source_id" "$kernel_release" "$toolchain_id" \
    "$(sha256 "$kernel_config")" \
    "$(sha256 "$kernel_build_dir/Module.symvers")" \
    "$(sha256 "$kernel_build_dir/System.map")" \
    "$(sha256 "$kernel_build_dir/vmlinux")" \
    "$(sha256 "$kernel_build_dir/Image")" \
    "$kernel_signing_der_sha256" "$trusted_bundle_sha256" \
    "$(sha256 "$out_dir/boot.img")" \
    "$(sha256 "$out_dir/system_dlkm.img")" \
    "$(sha256 "$out_dir/vendor_dlkm.img")" \
    "$(sha256 "$out_dir/vendor_boot.img")" \
    "$custom_module_count" "$retained_stock_module_count" \
    "$stock_cert_der_sha256" <<'PY'
import json
import pathlib
import sys

keys = (
    "kernel_commit", "kernel_source_id", "kernel_release", "toolchain_id",
    "config_sha256", "module_symvers_sha256",
    "system_map_sha256", "vmlinux_sha256", "Image_sha256",
    "module_signing_certificate_sha256", "system_trusted_bundle_sha256",
    "boot_sha256", "system_dlkm_sha256", "vendor_dlkm_sha256", "vendor_boot_sha256",
    "custom_module_count", "retained_stock_module_count", "stock_module_certificate_sha256",
)
values = dict(zip(keys, sys.argv[2:], strict=True))
for key in ("custom_module_count", "retained_stock_module_count"):
    values[key] = int(values[key])
pathlib.Path(sys.argv[1]).write_text(json.dumps(values, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

cat > "$out_dir/manifest.txt" <<EOF
device=CPH2747
platform=canoe
firmware=OOS_16.0.9.400_EX01
generation=controlled-v1
delivery_mode=controlled_hybrid_wlan
kernel_commit=$kernel_commit
kernel_source_id=$kernel_source_id
kernel_release=$kernel_release
toolchain_id=$toolchain_id
module_signing_certificate_der_sha256=$kernel_signing_der_sha256
stock_module_certificate_der_sha256=$stock_cert_der_sha256
system_trusted_bundle_sha256=$trusted_bundle_sha256
vbmeta=unchanged_not_included
device_writes=none

boot_path=boot.img
boot_size=$(size_bytes "$out_dir/boot.img")
boot_sha256=$(sha256 "$out_dir/boot.img")

vendor_boot_path=vendor_boot.img
vendor_boot_size=$(size_bytes "$out_dir/vendor_boot.img")
vendor_boot_sha256=$(sha256 "$out_dir/vendor_boot.img")
vendor_boot_policy=lossless_stock_baseline

system_dlkm_path=system_dlkm.img
system_dlkm_filesystem=erofs
system_dlkm_size=$(size_bytes "$out_dir/system_dlkm.img")
system_dlkm_sha256=$(sha256 "$out_dir/system_dlkm.img")

vendor_dlkm_path=vendor_dlkm.img
vendor_dlkm_filesystem=ext4
vendor_dlkm_size=$(size_bytes "$out_dir/vendor_dlkm.img")
vendor_dlkm_sha256=$(sha256 "$out_dir/vendor_dlkm.img")
vendor_dlkm_policy=matched_hybrid_wlan_closure

release_contract=release-contract.json
module_inventory=module-inventory/
module_signing_report=module-signing-report.txt
retained_stock_contract=retained-stock-contract.tsv
custom_module_contract=custom-module-contract.tsv
dependency_report=dependency-report.tsv
validation_report=validation-report.txt
EOF

if find "$out_dir" -type f \( -name '*.key' -o -name '*.p12' -o -name '*signing*.pem' \) -print -quit | grep -q .; then
    die "private signing material would be included in the package"
fi

(
    cd "$out_dir"
    find boot.img vendor_boot.img system_dlkm.img vendor_dlkm.img \
         manifest.txt release-contract.json module-signing-report.txt \
         retained-stock-contract.tsv custom-module-contract.tsv \
         dependency-report.tsv validation-report.txt module-inventory \
         -type f -print0 | sort -z | xargs -0 sha256sum
) > "$out_dir/SHA256SUMS"

printf 'CONTROLLED-V1 OOS STACK PACKAGE PASS\n'
printf 'output=%s\n' "$out_dir"
printf 'kernel_commit=%s\n' "$kernel_commit"
printf 'kernel_source_id=%s\n' "$kernel_source_id"
printf 'kernel_release=%s\n' "$kernel_release"
printf 'device_writes=none\n'
