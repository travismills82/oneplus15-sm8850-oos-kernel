#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Build the bounded Camera .073 RER command-snapshot hardening against g6744.

set -euo pipefail

die() { printf 'error: %s\n' "$*" >&2; exit 2; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
contract="$repo_root/tools/controlled-v1-wlan053-kernel-contract.json"
signing_repo="${HOME}/.config/oneplus15-kernel/signing-v1"
qualified_dist=/home/travis/Android/oneplus15-sm8850-oos-wlan-cnss-053/out/wlan053-dist
out_dir="$repo_root/out/camera-rer-073"
do_clean=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --contract) contract=${2:-}; shift 2 ;;
        --signing-repo) signing_repo=${2:-}; shift 2 ;;
        --qualified-dist) qualified_dist=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        --clean) do_clean=1; shift ;;
        -h|--help) sed -n '2,12p' "$0"; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

for command in git jq llvm-objcopy modinfo openssl python3 sha256sum; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done
for path in "$contract" "$signing_repo/BUILD.bazel" \
            "$signing_repo/module-signing-v1.pem" "$signing_repo/module-signing-v1.x509" \
            "$qualified_dist/Image" "$qualified_dist/kernel_aarch64_dot_config" \
            "$qualified_dist/kernel_aarch64_Module.symvers" "$qualified_dist/System.map" \
            "$qualified_dist/modules.builtin" "$qualified_dist/vmlinux"; do
    [[ -f "$path" ]] || die "missing required input: $path"
done

release=$(jq -r .release "$contract")
stamp_id=$(jq -r .release_stamp_source_id "$contract")
stamp_epoch=$(jq -r .release_stamp_source_date_epoch "$contract")
project_cert_sha=$(jq -r .signing.project_certificate_der_sha256 "$contract")
old_source_id=5ab2a689ff87d7d28c511f1762cf41c1b90d965a
comparison_source_id=d447f713d6403f707a2910383495f4ada98cfa4d
source_file=vendor/qcom/opensource/camera-kernel/drivers/cam_sensor_module/cam_flash/cam_flash_core.c
expected_selected_source_object=a3ccb27a5514dc70f057254245d7068e971299bd

[[ $(git -C "$repo_root" rev-parse "${old_source_id}^{commit}") == "$old_source_id" ]] ||
    die "declared Canoe Camera .061 source identity is unavailable"
[[ $(git -C "$repo_root" rev-parse "${comparison_source_id}^{commit}") == "$comparison_source_id" ]] ||
    die "declared Camera .073 comparison identity is unavailable"
[[ $(git -C "$repo_root" hash-object "$source_file") == "$expected_selected_source_object" ]] ||
    die "camera flash source is not the reviewed selective .073 import"
python3 - "$repo_root/$source_file" <<'PY'
import pathlib, sys
text = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
required = (
    "flash_rer_info_u = (struct cam_flash_set_rer *)cmd_buf;",
    "count = flash_rer_info_u->count;",
    "cam_common_mem_kdup((void **)&flash_rer_info,",
    "if (count != flash_rer_info->count)",
    "cam_common_mem_free(flash_rer_info);",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"reviewed RER command-snapshot hardening is absent: {needle}")
PY

camera_root=vendor/qcom/opensource/camera-kernel
while read -r _mode _type object path; do
    [[ "$path" == "$source_file" ]] && continue
    [[ -f "$repo_root/$path" ]] || die "reviewed Camera .061 input disappeared: $path"
    [[ $(git -C "$repo_root" hash-object "$path") == "$object" ]] ||
        die "unreviewed Camera input change detected: $path"
done < <(git -C "$repo_root" ls-tree -r "$old_source_id" "$camera_root")

[[ $(sha256sum "$signing_repo/module-signing-v1.x509" | awk '{print $1}') == "$project_cert_sha" ]] ||
    die "controlled-v1 signing repository certificate changed"
[[ $(stat -c '%a' "$signing_repo/module-signing-v1.pem") == 600 ]] ||
    die "controlled-v1 private key must be mode 600"

scope_file="$repo_root/tools/controlled-v1-kernel-contract-inputs.txt"
mapfile -t contract_inputs < <(awk 'NF && $1 !~ /^#/' "$scope_file")
[[ ${#contract_inputs[@]} -gt 0 ]] || die "kernel contract input scope is empty"
git -C "$repo_root" diff --quiet -- "${contract_inputs[@]}" ||
    die "uncommitted kernel contract input change prevents qualified release reuse"
git -C "$repo_root" diff --cached --quiet -- "${contract_inputs[@]}" ||
    die "staged kernel contract input change prevents qualified release reuse"
mapfile -t untracked_contract_inputs < <(
    git -C "$repo_root" ls-files --others --exclude-standard -- "${contract_inputs[@]}"
)
[[ ${#untracked_contract_inputs[@]} -eq 0 ]] ||
    die "untracked kernel contract input prevents qualified release reuse: ${untracked_contract_inputs[0]}"

common_status="$repo_root/kernel_platform/common/workspace_status.json"
module_status="$repo_root/$camera_root/workspace_status.json"
aquery_json="$repo_root/out/camera-rer-073-kernel-aquery.json"
cleanup() { rm -f -- "$common_status" "$module_status"; }
trap cleanup EXIT HUP INT TERM
for status in "$common_status" "$module_status"; do
    [[ ! -e "$status" && ! -L "$status" ]] || die "refusing to replace workspace status: $status"
done
status_json=$(printf '{\n  "SCMVERSION": "-g%s",\n  "SOURCE_DATE_EPOCH": %s\n}\n' \
    "${stamp_id:0:12}" "$stamp_epoch")
printf '%s' "$status_json" > "$common_status"
printf '%s' "$status_json" > "$module_status"

bazel_args=(
    --override_module="oneplus15_signing=$signing_repo"
    --//vendor/qcom/opensource/camera-kernel:project_name=canoe
)
cd "$repo_root/kernel_platform"
if ((do_clean)); then tools/bazel clean; fi
tools/bazel build "${bazel_args[@]}" //common:kernel_aarch64 //soc-repo:canoe_perf_config
mkdir -p "$(dirname "$aquery_json")" "$out_dir"
tools/bazel aquery "${bazel_args[@]}" \
    'mnemonic("KernelBuild", //common:kernel_aarch64)' --output=jsonproto > "$aquery_json"

kernel_build_dir="$repo_root/kernel_platform/bazel-bin/common/kernel_aarch64"
canoe_config="$repo_root/kernel_platform/bazel-bin/soc-repo/canoe_perf_config/out_dir/.config"
python3 "$repo_root/tools/verify-controlled-v1-kernel-contract.py" \
    --contract "$contract" \
    --kernel-build-dir "$kernel_build_dir" \
    --canoe-config "$canoe_config" \
    --qualified-dist "$qualified_dist" \
    --aquery-json "$aquery_json" \
    --repo-root "$repo_root" \
    --build-input-delta "$out_dir/build-input-delta.tsv"

tools/bazel build "${bazel_args[@]}" \
    //vendor/qcom/opensource/camera-kernel:canoe_perf_camera
tools/bazel build "${bazel_args[@]}" //soc-repo:canoe_perf_dist
tools/bazel build "${bazel_args[@]}" \
    //common:kernel_aarch64_abi \
    //common:kernel_aarch64_abi_kmi_symbol_checks \
    //common:kernel_aarch64_abi_diff

rm -rf -- "$out_dir/unsigned-modules"
mkdir -p "$out_dir/unsigned-modules"
target="$repo_root/kernel_platform/bazel-bin/vendor/qcom/opensource/camera-kernel/canoe_perf_camera/camera.ko"
[[ -f "$target" ]] || die "missing canonical Camera DDK output: $target"
cp -a -- "$target" "$out_dir/unsigned-modules/camera.ko"
llvm-objcopy --strip-debug "$out_dir/unsigned-modules/camera.ko"
[[ -z $(modinfo -F signer "$out_dir/unsigned-modules/camera.ko") ]] ||
    die "Camera source output unexpectedly has a signature"
[[ $(modinfo -F vermagic "$out_dir/unsigned-modules/camera.ko") == "$release "* ]] ||
    die "Camera module does not inherit the frozen kernel release"

sha256sum "$out_dir/unsigned-modules/camera.ko" > "$out_dir/unsigned-modules-SHA256SUMS"
{
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract_sha256=%s\n' "$(sha256sum "$contract" | awk '{print $1}')"
    printf 'camera_current_generation=.061\n'
    printf 'camera_candidate_generation=.073-selective-rer-command-snapshot\n'
    printf 'camera_current_source_id=%s\n' "$old_source_id"
    printf 'camera_comparison_source_id=%s\n' "$comparison_source_id"
    printf 'selected_source_object=%s\n' "$expected_selected_source_object"
    printf 'signing_certificate_sha256=%s\n' "$project_cert_sha"
    printf 'dist=PASS\nABI=PASS\nKMI=PASS\nABI_report=EMPTY\n'
} > "$out_dir/build-contract.txt"
printf 'CONTROLLED CAMERA .073 RER BUILD PASS\n'
printf 'kernel_release=%s\n' "$release"
printf 'module=%s\n' "$out_dir/unsigned-modules/camera.ko"
