#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Build the bounded Graphics .057 secure-guard ownership fix against frozen g6744.

set -euo pipefail

die() { printf 'error: %s\n' "$*" >&2; exit 2; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
contract="$repo_root/tools/controlled-v1-wlan053-kernel-contract.json"
signing_repo="${HOME}/.config/oneplus15-kernel/signing-v1"
qualified_dist=/home/travis/Android/oneplus15-sm8850-oos-wlan-cnss-053/out/wlan053-dist
out_dir="$repo_root/out/graphics-secure-guard-057"
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
source_id=d447f713d6403f707a2910383495f4ada98cfa4d
source_file=vendor/qcom/opensource/graphics-kernel/kgsl_sharedmem.c
expected_selected_source_object=ac4999291959c8a9f49994e8a6e9f378dce64eb7

[[ $(git -C "$repo_root" rev-parse "${old_source_id}^{commit}") == "$old_source_id" ]] ||
    die "declared Canoe Graphics .038 source identity is unavailable"
[[ $(git -C "$repo_root" rev-parse "${source_id}^{commit}") == "$source_id" ]] ||
    die "declared Graphics .057 comparison identity is unavailable"
[[ $(git -C "$repo_root" hash-object "$source_file") == "$expected_selected_source_object" ]] ||
    die "KGSL source is not the reviewed selective .057 import"
python3 - "$repo_root/$source_file" <<'PY'
import pathlib, sys
text = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
needle = '''\tret = kgsl_unlock_sgt(&sgt);
\tif (ret)
\t\t/*
\t\t * Unlock of the secure page failed. This page will be
\t\t * stuck in secure side forever and is unrecoverable.
\t\t * Give up on this page and don't free it.
\t\t */
\t\tpr_err("kgsl_unlock_sgt failed ret %d\\n", ret);
\telse
\t\t__free_page(page);'''
if text.count(needle) != 1:
    raise SystemExit("reviewed KGSL secure-guard ownership fix is absent")
PY

graphics_root=vendor/qcom/opensource/graphics-kernel
while read -r _mode _type object path; do
    [[ "$path" == "$source_file" ]] && continue
    if [[ "$path" == "$graphics_root/config/canoe_perf_gpuconf" ]]; then
        [[ $(git -C "$repo_root" hash-object "$path") == \
           e19edaca7a981138699edc6c007ab6ec50e792f3 ]] ||
            die "qualified Canoe Graphics config changed"
        continue
    fi
    [[ "$path" == "$graphics_root/config/"*"_perf_gpuconf" ]] && continue
    [[ "$path" == "$graphics_root/config/pitti_gki_gpuconf" ]] && continue
    [[ -f "$repo_root/$path" ]] || die "reviewed Graphics .038 input disappeared: $path"
    [[ $(git -C "$repo_root" hash-object "$path") == "$object" ]] ||
        die "unreviewed Graphics input change detected: $path"
done < <(git -C "$repo_root" ls-tree -r "$old_source_id" "$graphics_root")

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
module_status="$repo_root/$graphics_root/workspace_status.json"
aquery_json="$repo_root/out/graphics-secure-guard-057-kernel-aquery.json"
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

tools/bazel build "${bazel_args[@]}" \
    //vendor/qcom/opensource/graphics-kernel:canoe_perf_msm_kgsl
tools/bazel build "${bazel_args[@]}" //soc-repo:canoe_perf_dist
tools/bazel build "${bazel_args[@]}" \
    //common:kernel_aarch64_abi \
    //common:kernel_aarch64_abi_kmi_symbol_checks \
    //common:kernel_aarch64_abi_diff

rm -rf -- "$out_dir/unsigned-modules"
mkdir -p "$out_dir/unsigned-modules"
target="$repo_root/kernel_platform/bazel-bin/vendor/qcom/opensource/graphics-kernel/canoe_perf_msm_kgsl/msm_kgsl.ko"
[[ -f "$target" ]] || die "missing canonical KGSL DDK output: $target"
cp -a -- "$target" "$out_dir/unsigned-modules/msm_kgsl.ko"
llvm-objcopy --strip-debug "$out_dir/unsigned-modules/msm_kgsl.ko"
[[ -z $(modinfo -F signer "$out_dir/unsigned-modules/msm_kgsl.ko") ]] ||
    die "KGSL source output unexpectedly has a signature"
[[ $(modinfo -F vermagic "$out_dir/unsigned-modules/msm_kgsl.ko") == "$release "* ]] ||
    die "KGSL does not inherit the frozen kernel release"

sha256sum "$out_dir/unsigned-modules/msm_kgsl.ko" > "$out_dir/unsigned-modules-SHA256SUMS"
{
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract_sha256=%s\n' "$(sha256sum "$contract" | awk '{print $1}')"
    printf 'graphics_current_generation=.038\n'
    printf 'graphics_candidate_generation=.057-selective-secure-guard-unlock\n'
    printf 'graphics_current_source_id=%s\n' "$old_source_id"
    printf 'graphics_candidate_source_id=%s\n' "$source_id"
    printf 'selected_source_object=%s\n' "$expected_selected_source_object"
    printf 'signing_certificate_sha256=%s\n' "$project_cert_sha"
    printf 'dist=PASS\nABI=PASS\nKMI=PASS\nABI_report=EMPTY\n'
} > "$out_dir/build-contract.txt"
printf 'CONTROLLED GRAPHICS .057 SECURE-GUARD BUILD PASS\n'
printf 'kernel_release=%s\n' "$release"
printf 'module=%s\n' "$out_dir/unsigned-modules/msm_kgsl.ko"
