#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Assemble the bounded Camera .073 RER candidate over qualified Graphics057.

set -euo pipefail

readonly release=6.12.23-android16-5-o-g6744a3f6bcf4-4k
readonly signer='OnePlus 15 Controlled OOS Module Signing v1'
readonly baseline_sha=a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0

die() { printf 'error: %s\n' "$*" >&2; exit 2; }
sha256() { sha256sum "$1" | awk '{print $1}'; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
baseline_image=
baseline_stage=
replacement=
system_modules=
vendor_boot_modules=
vendor_boot_load=
module_symvers=
kernel_build_dir=
signing_key=
system_archive=
system_load_contract=
avbtool=
out_dir=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --baseline-image) baseline_image=${2:-}; shift 2 ;;
        --baseline-stage) baseline_stage=${2:-}; shift 2 ;;
        --replacement) replacement=${2:-}; shift 2 ;;
        --system-modules) system_modules=${2:-}; shift 2 ;;
        --vendor-boot-modules) vendor_boot_modules=${2:-}; shift 2 ;;
        --vendor-boot-load) vendor_boot_load=${2:-}; shift 2 ;;
        --module-symvers) module_symvers=${2:-}; shift 2 ;;
        --kernel-build-dir) kernel_build_dir=${2:-}; shift 2 ;;
        --signing-key) signing_key=${2:-}; shift 2 ;;
        --system-dlkm-staging-archive) system_archive=${2:-}; shift 2 ;;
        --system-load-contract) system_load_contract=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        *) die "unknown argument: $1" ;;
    esac
done
for value in baseline_image baseline_stage replacement system_modules \
             vendor_boot_modules vendor_boot_load module_symvers kernel_build_dir \
             signing_key system_archive system_load_contract avbtool out_dir; do
    [[ -n ${!value} ]] || die "missing required --${value//_/-} argument"
done
for path in "$baseline_image" "$replacement" "$vendor_boot_load" \
            "$module_symvers" "$signing_key" "$system_archive" \
            "$system_load_contract" "$avbtool"; do
    [[ -f "$path" ]] || die "missing file: $path"
done
for path in "$baseline_stage" "$system_modules" "$vendor_boot_modules" \
            "$kernel_build_dir"; do
    [[ -d "$path" ]] || die "missing directory: $path"
done
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output: $out_dir"
[[ $(sha256 "$baseline_image") == "$baseline_sha" ]] ||
    die "base image is not the physically qualified Graphics057 vendor_dlkm"
[[ $(modinfo -F name "$replacement") == camera ]] || die "replacement is not camera"
[[ -z $(modinfo -F signer "$replacement") ]] || die "Camera build output must be unsigned"
[[ $(modinfo -F vermagic "$replacement") == "$release "* ]] ||
    die "Camera build output does not inherit the frozen kernel release"

python3 - "$system_load_contract" <<'PY'
import csv, pathlib, sys
rows = [r for r in csv.DictReader(pathlib.Path(sys.argv[1]).open(encoding="utf-8"), delimiter="\t")
        if r.get("partition") == "system_dlkm"]
if len(rows) != 46 or any(r.get("status") != "PASS" for r in rows):
    raise SystemExit("system modules.load 46-entry contract failed")
wwan = [r for r in rows if r.get("module") == "wwan.ko"]
if len(wwan) != 1 or wwan[0].get("load_order") != "21":
    raise SystemExit("wwan.ko is not modules.load entry 21")
if any(r.get("stale_builtin_entry") != "no" for r in rows):
    raise SystemExit("system modules.load contains stale built-in entries")
PY

contract="$out_dir/module-contract"
mkdir -p "$contract"
python3 "$repo_root/tools/validate-matched-wlan-vendor-dlkm.py" \
    --stock-vendor-root "$baseline_stage/lib/modules" \
    --replacement "$replacement" \
    --external-root "$system_modules" \
    --external-root "$vendor_boot_modules" \
    --vmlinux-symvers "$module_symvers" \
    --out-dir "$contract" \
    --expected-stock-module-count 436
jq -e '
  .result == "PASS" and .stock_vendor_modules == 436 and
  .source_replacements == 1 and .protected_export_signed_closure == 2 and
  .re_sign_stock_modules == 1 and .retained_external_modules == 525 and
  .allowed_import_contract_changes == [] and .allowed_export_contract_changes == [] and
  .external_signed_provider_edges == 49 and .replacement_contract_failures == 0 and
  .unresolved_imports == 0 and .crc_mismatches == 0
' "$contract/summary.json" >/dev/null || die "bounded Camera module graph failed"

python3 - "$contract/external-signed-provider-edges.tsv" "$vendor_boot_load" <<'PY'
import csv, pathlib, sys
rows = list(csv.DictReader(pathlib.Path(sys.argv[1]).open(encoding="utf-8"), delimiter="\t"))
if len(rows) != 49 or any(r["consumer"] != "camera_extension" or
                          r["new_signed_provider"] != "camera" for r in rows):
    raise SystemExit("unreviewed external Camera signed-provider boundary")
entries = {line.strip().split("/")[-1] for line in pathlib.Path(sys.argv[2]).read_text().splitlines()
           if line.strip() and not line.lstrip().startswith("#")}
if "camera_extension.ko" in entries:
    raise SystemExit("vendor_boot Camera extension is not dormant")
PY

stage_out="$out_dir/staged-candidate"
"$repo_root/tools/stage-matched-wlan-vendor-dlkm.sh" \
    --stock-vendor-root "$baseline_stage" \
    --stock-vendor-image "$baseline_image" \
    --kernel-build-dir "$kernel_build_dir" \
    --signing-key "$signing_key" \
    --system-dlkm-staging-archive "$system_archive" \
    --avbtool "$avbtool" \
    --validation-dir "$contract" \
    --out-dir "$stage_out" \
    --strip-unneeded-resign camera_extension \
    --replacement "$replacement"

cp --reflink=auto "$stage_out/vendor_dlkm.img" "$out_dir/vendor_dlkm.img"
cp "$stage_out/e2fsck.txt" "$out_dir/e2fsck.txt"
cp "$stage_out/e2fsck-repair.txt" "$out_dir/e2fsck-repair.txt"
cp "$stage_out/e2fsck-pre-avb.txt" "$out_dir/e2fsck-pre-avb.txt"
cp "$stage_out/manifest.txt" "$out_dir/staging-manifest.txt"
cp "$stage_out/vendor-system-dependency-reconciliation.tsv" \
    "$out_dir/vendor-system-dependency-reconciliation.tsv"
cp "$system_load_contract" "$out_dir/system-dlkm-load-contract.tsv"
cp "$repo_root/docs/validation/camera-modernization/source-delta.md" \
    "$out_dir/camera-073-source-delta.md"
cp "$repo_root/docs/validation/camera-modernization/current-camera-closure.tsv" \
    "$out_dir/current-camera-closure.tsv"
cp "$repo_root/docs/validation/camera-modernization/candidate-ranking.tsv" \
    "$out_dir/camera-candidate-ranking.tsv"

python3 "$repo_root/tools/verify-camera-rer-073-candidate.py" \
    --validator "$repo_root/tools/validate-matched-wlan-vendor-dlkm.py" \
    --baseline-root "$baseline_stage/lib/modules" \
    --candidate-root "$stage_out/staging/lib/modules" \
    --replacement "$replacement" \
    --closure "$contract/protected-export-signing-closure.tsv" \
    --import-resolution "$contract/import-resolution.tsv" \
    --external-edges "$contract/external-signed-provider-edges.tsv" \
    --vendor-boot-load "$vendor_boot_load" \
    --expected-signer "$signer" \
    --expected-release "$release" \
    --out-dir "$out_dir"

"$avbtool" verify_image --image "$out_dir/vendor_dlkm.img" >/dev/null ||
    die "candidate AVB/footer validation failed"
python3 - "$repo_root/tools/controlled-v1-wlan053-kernel-contract.json" \
    "$repo_root/out/camera-rer-073/build-contract.txt" \
    "$out_dir/release-contract.json" <<'PY'
import json, pathlib, sys
kernel = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
build = dict(line.split("=", 1) for line in pathlib.Path(sys.argv[2]).read_text().splitlines() if "=" in line)
manifest = {
    "schema_version": 2,
    "name": "controlled-v1-camera073-selective-rer-snapshot",
    "qualification": "STATIC_PASS_PHYSICAL_NOT_TESTED",
    "kernel_contract": {
        "release": kernel["release"],
        "release_stamp_source_id": kernel["release_stamp_source_id"],
        "config_sha256": kernel["artifacts"]["config_sha256"],
        "module_symvers_sha256": kernel["artifacts"]["module_symvers_sha256"],
        "image_sha256": kernel["artifacts"]["image_sha256"],
        "vmlinux_sha256": kernel["artifacts"]["vmlinux_sha256"],
    },
    "subsystems": {
        "wlan": ".053 physically qualified and retained",
        "bluetooth_vendor": ".046 core-qualified and retained",
        "nfc_vendor": ".102 core-qualified and retained",
        "cellular": ".102 core physically qualified and retained",
        "audio": ".059 selective GPR physically qualified and retained",
        "graphics": ".057 selective secure-guard physically qualified and retained",
        "camera": {
            "baseline_generation": ".061",
            "candidate_generation": ".073 selective RER command snapshot",
            "baseline_source_id": build["camera_current_source_id"],
            "comparison_source_id": build["camera_comparison_source_id"],
        },
    },
    "signing": {
        "generation": "controlled-v1",
        "certificate_sha256": build["signing_certificate_sha256"],
    },
}
pathlib.Path(sys.argv[3]).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
PY

{
    printf 'Camera .073 selective RER candidate: PASS\n'
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract_guard=PASS\nconfig=IDENTICAL\nModule.symvers=IDENTICAL\n'
    printf 'Image_functional_contract=IDENTICAL\nvmlinux_functional_contract=IDENTICAL\n'
    printf 'dist=PASS\nABI=PASS_EMPTY\nKMI=PASS\n'
    printf 'vendor_modules=436\nsource_upgrades=1:camera\n'
    printf 'exact_stock_resign=1:camera_extension\nunexpected_module_changes=0\n'
    printf 'camera_export_changes=0\ncamera_import_changes=0\n'
    printf 'unresolved_imports=0\ncrc_mismatches=0\n'
    printf 'protected_export_failures=0\nsignature_failures=0\n'
    printf 'vendor_boot_dormant_camera_extension_edges=49:REVIEWED\n'
    printf 'system_modules_load_entries=46\nwwan_load_entry=21\n'
    printf 'filesystem=PASS\npartition_local_avb=PASS\nphysical_flash=NOT_PERFORMED\n'
} > "$out_dir/validation-report.txt"

(
    cd "$out_dir"
    sha256sum vendor_dlkm.img release-contract.json camera-replacement-manifest.tsv \
        camera-import-crc.tsv camera-module-contract.tsv camera-signature-report.tsv \
        camera-preservation-report.tsv camera-vendor-boot-dormant-boundary.tsv \
        system-dlkm-load-contract.tsv validation-report.txt e2fsck-repair.txt \
        e2fsck-pre-avb.txt e2fsck.txt > SHA256SUMS
)
printf 'CONTROLLED CAMERA .073 RER STATIC CANDIDATE PASS\n'
printf 'image=%s\n' "$out_dir/vendor_dlkm.img"
printf 'sha256=%s\n' "$(sha256 "$out_dir/vendor_dlkm.img")"
printf 'physical_flash=NOT_PERFORMED\n'

