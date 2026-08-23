#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Build the bounded Platform .099.086 CoreSight ETR UAF guard against g6744.

set -euo pipefail

die() { printf 'error: %s\n' "$*" >&2; exit 2; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
contract="$repo_root/tools/controlled-v1-wlan053-kernel-contract.json"
signing_repo="${HOME}/.config/oneplus15-kernel/signing-v1"
qualified_dist=/home/travis/Android/oneplus15-sm8850-oos-wlan-cnss-053/out/wlan053-dist
out_dir="$repo_root/out/platform-coresight-etr-086"
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
baseline_source_id=a83e6fbd78e30d7abc5223b69fbdf3afb02f0b2c
current_official_id=fc30e54174d254ff7f33622a9278e4435f6718d2
comparison_official_id=0cfd948c0d9507d0bc04706f6f919a25787c2d89
selected_commit=db1b06b53dcf37388f95105123ba36a854724d34
source_root=kernel_platform/soc-repo/drivers/hwtracing/coresight
source_file="$source_root/coresight-tmc-etr.c"
expected_selected_source_object=c9f5070de6d7684b787c1f485860badd58479f88

[[ $(git -C "$repo_root" rev-parse "${baseline_source_id}^{commit}") == "$baseline_source_id" ]] ||
    die "Camera-qualified source baseline is unavailable"
[[ $(git -C "$repo_root" hash-object "$source_file") == "$expected_selected_source_object" ]] ||
    die "CoreSight ETR source is not the reviewed .099.086 fix"
python3 - "$repo_root/$source_file" <<'PY'
import pathlib, sys
text = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
needle = "if (coresight_get_mode(csdev) == CS_MODE_SYSFS)\n\t\tgoto out;"
if text.count(needle) != 1:
    raise SystemExit("reviewed CoreSight ETR active-buffer guard is absent")
PY

while read -r _mode _type object path; do
    [[ "$path" == "$source_file" ]] && continue
    [[ -f "$repo_root/$path" ]] || die "qualified CoreSight input disappeared: $path"
    [[ $(git -C "$repo_root" hash-object "$path") == "$object" ]] ||
        die "unreviewed CoreSight input change detected: $path"
done < <(git -C "$repo_root" ls-tree -r "$baseline_source_id" "$source_root")

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
module_status="$repo_root/kernel_platform/soc-repo/workspace_status.json"
aquery_json="$repo_root/out/platform-coresight-etr-086-kernel-aquery.json"
cleanup() { rm -f -- "$common_status" "$module_status"; }
trap cleanup EXIT HUP INT TERM
for status in "$common_status" "$module_status"; do
    [[ ! -e "$status" && ! -L "$status" ]] || die "refusing to replace workspace status: $status"
done
status_json=$(printf '{\n  "SCMVERSION": "-g%s",\n  "SOURCE_DATE_EPOCH": %s\n}\n' \
    "${stamp_id:0:12}" "$stamp_epoch")
printf '%s' "$status_json" > "$common_status"
printf '%s' "$status_json" > "$module_status"

bazel_args=(--override_module="oneplus15_signing=$signing_repo")
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

module_target='//soc-repo:canoe_perf/drivers/hwtracing/coresight/coresight-tmc'
tools/bazel build "${bazel_args[@]}" "$module_target"
tools/bazel build "${bazel_args[@]}" //soc-repo:canoe_perf_dist
tools/bazel build "${bazel_args[@]}" \
    //common:kernel_aarch64_abi \
    //common:kernel_aarch64_abi_kmi_symbol_checks \
    //common:kernel_aarch64_abi_diff

rm -rf -- "$out_dir/unsigned-modules"
mkdir -p "$out_dir/unsigned-modules"
target="$repo_root/kernel_platform/bazel-bin/soc-repo/canoe_perf/drivers/hwtracing/coresight/coresight-tmc/coresight-tmc.ko"
[[ -f "$target" ]] || die "missing canonical CoreSight TMC DDK output: $target"
cp -a -- "$target" "$out_dir/unsigned-modules/coresight-tmc.ko"
llvm-objcopy --strip-debug "$out_dir/unsigned-modules/coresight-tmc.ko"
[[ -z $(modinfo -F signer "$out_dir/unsigned-modules/coresight-tmc.ko") ]] ||
    die "CoreSight TMC source output unexpectedly has a signature"
[[ $(modinfo -F vermagic "$out_dir/unsigned-modules/coresight-tmc.ko") == "$release "* ]] ||
    die "CoreSight TMC does not inherit the frozen kernel release"

sha256sum "$out_dir/unsigned-modules/coresight-tmc.ko" > "$out_dir/unsigned-modules-SHA256SUMS"
{
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract_sha256=%s\n' "$(sha256sum "$contract" | awk '{print $1}')"
    printf 'platform_current_generation=.099.064\n'
    printf 'platform_candidate_generation=.099.086-selective-coresight-etr-uaf-guard\n'
    printf 'platform_current_source_id=%s\n' "$current_official_id"
    printf 'platform_candidate_source_id=%s\n' "$comparison_official_id"
    printf 'selected_source_commit=%s\n' "$selected_commit"
    printf 'selected_source_object=%s\n' "$expected_selected_source_object"
    printf 'signing_certificate_sha256=%s\n' "$project_cert_sha"
    printf 'dist=PASS\nABI=PASS\nKMI=PASS\nABI_report=EMPTY\n'
} > "$out_dir/build-contract.txt"
printf 'CONTROLLED PLATFORM .099.086 CORESIGHT ETR BUILD PASS\n'
printf 'kernel_release=%s\n' "$release"
printf 'module=%s\n' "$out_dir/unsigned-modules/coresight-tmc.ko"
