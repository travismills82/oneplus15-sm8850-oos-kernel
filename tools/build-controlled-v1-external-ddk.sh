#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Build external DDK modules against an immutable, physically qualified kernel
# contract. Kleaf still supplies kernel.release, Module.symvers, and generated
# headers through kernel_build; the fixed workspace status prevents the DDK
# source repository HEAD from inventing a second vermagic identity.

set -euo pipefail

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
contract="$repo_root/tools/controlled-v1-wlan053-kernel-contract.json"
signing_repo="${HOME}/.config/oneplus15-kernel/signing-v1"
qualified_dist=/home/travis/Android/oneplus15-sm8850-oos-wlan-cnss-053/out/wlan053-dist
out_dir="$repo_root/out/bluetooth-vendor-046"
do_clean=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --contract) contract=${2:-}; shift 2 ;;
        --signing-repo) signing_repo=${2:-}; shift 2 ;;
        --qualified-dist) qualified_dist=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        --clean) do_clean=1; shift ;;
        -h|--help)
            sed -n '2,12p' "$0"
            exit 0
            ;;
        *) die "unknown argument: $1" ;;
    esac
done

for command in git jq modinfo openssl python3 sha256sum; do
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
bt_source_id=$(jq -r .subsystems.bluetooth_vendor.source_id "$contract")
[[ $(git -C "$repo_root" rev-parse "${bt_source_id}^{commit}") == "$bt_source_id" ]] ||
    die "Bluetooth module source identity is unavailable"
git -C "$repo_root" diff --quiet "$bt_source_id" HEAD -- vendor/qcom/opensource/bt-kernel ||
    die "Bluetooth source differs from the declared .046 module source identity"
[[ $(sha256sum "$signing_repo/module-signing-v1.x509" | awk '{print $1}') == "$project_cert_sha" ]] ||
    die "controlled-v1 signing repository certificate changed"
[[ $(stat -c '%a' "$signing_repo/module-signing-v1.pem") == 600 ]] ||
    die "controlled-v1 private key must be mode 600"

# A fixed release stamp is allowed only for a clean, reviewed kernel-contract
# input set. External DDK source is deliberately outside this list, but vendor
# paths are never excluded as a blanket rule.
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
bt_status="$repo_root/vendor/qcom/opensource/bt-kernel/workspace_status.json"
aquery_json="$repo_root/out/bluetooth-vendor-046-kernel-aquery.json"
cleanup() {
    rm -f -- "$common_status" "$bt_status"
}
trap cleanup EXIT HUP INT TERM
for status in "$common_status" "$bt_status"; do
    [[ ! -e "$status" && ! -L "$status" ]] || die "refusing to replace existing workspace status: $status"
done
status_json=$(printf '{\n  "SCMVERSION": "-g%s",\n  "SOURCE_DATE_EPOCH": %s\n}\n' "${stamp_id:0:12}" "$stamp_epoch")
printf '%s' "$status_json" > "$common_status"
printf '%s' "$status_json" > "$bt_status"

bazel_args=(--override_module="oneplus15_signing=$signing_repo")
cd "$repo_root/kernel_platform"
if ((do_clean)); then
    tools/bazel clean
fi

# Gate the base kernel before allowing any external module action to execute.
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
    --aquery-json "$aquery_json" \
    --repo-root "$repo_root" \
    --build-input-delta "$out_dir/build-input-delta.tsv"

tools/bazel build "${bazel_args[@]}" \
    //vendor/qcom/opensource/bt-kernel:canoe_perf_btpower \
    //vendor/qcom/opensource/bt-kernel:canoe_perf_bt_fm_swr \
    //vendor/qcom/opensource/bt-kernel:canoe_perf_btfm_slim_codec

# ABI/KMI are kernel-contract gates; the DDK source delta cannot bypass them.
tools/bazel build "${bazel_args[@]}" \
    //common:kernel_aarch64_abi \
    //common:kernel_aarch64_abi_kmi_symbol_checks \
    //common:kernel_aarch64_abi_diff

rm -rf -- "$out_dir/modules"
mkdir -p "$out_dir/modules"
for module in btpower bt_fm_swr btfm_slim_codec; do
    mapfile -t matches < <(find "$repo_root/kernel_platform/bazel-bin/vendor/qcom/opensource/bt-kernel" \
        -type f -name "${module}.ko" -print)
    [[ ${#matches[@]} -eq 1 ]] || die "expected one ${module}.ko output, found ${#matches[@]}"
    cp -a -- "${matches[0]}" "$out_dir/modules/${module}.ko"
    [[ $(modinfo -F vermagic "$out_dir/modules/${module}.ko") == "$release "* ]] ||
        die "${module}.ko does not inherit the qualified kernel release"
done

sha256sum "$out_dir/modules/"*.ko > "$out_dir/SHA256SUMS"
printf 'kernel_release=%s\n' "$release" > "$out_dir/build-contract.txt"
printf 'kernel_contract=%s\n' "$(sha256sum "$contract" | awk '{print $1}')" >> "$out_dir/build-contract.txt"
printf 'bluetooth_source_id=%s\n' "$bt_source_id" >> "$out_dir/build-contract.txt"
printf 'signing_certificate_sha256=%s\n' "$project_cert_sha" >> "$out_dir/build-contract.txt"
printf 'CONTROLLED EXTERNAL DDK BUILD PASS\n'
printf 'kernel_release=%s\n' "$release"
printf 'bluetooth_source_id=%s\n' "$bt_source_id"
printf 'output=%s\n' "$out_dir"
