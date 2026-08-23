#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Build the isolated RMNET core .102 provider against the frozen g6744 contract.

set -euo pipefail

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
contract="$repo_root/tools/controlled-v1-wlan053-kernel-contract.json"
signing_repo="${HOME}/.config/oneplus15-kernel/signing-v1"
qualified_dist=/home/travis/Android/oneplus15-sm8850-oos-wlan-cnss-053/out/wlan053-dist
out_dir="$repo_root/out/cellular-rmnet-core-102"
do_clean=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --contract) contract=${2:-}; shift 2 ;;
        --signing-repo) signing_repo=${2:-}; shift 2 ;;
        --qualified-dist) qualified_dist=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        --clean) do_clean=1; shift ;;
        -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

for command in git jq llvm-objcopy modinfo openssl pahole python3 sha256sum; do
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
source_id=bc8d91d1e146be96d2e27bebe8f753f82bdebeee
old_source_id=5ab2a689ff87d7d28c511f1762cf41c1b90d965a
source_file=vendor/qcom/opensource/datarmnet/core/rmnet_map_data.c
[[ $(git -C "$repo_root" rev-parse "${source_id}^{commit}") == "$source_id" ]] ||
    die "declared RMNET .102 source identity is unavailable"
[[ $(git -C "$repo_root" hash-object "$source_file") == \
   $(git -C "$repo_root" rev-parse "$source_id:$source_file") ]] ||
    die "rmnet_map_data.c is not the exact declared .102 source object"
mapfile -t shared_headers < <(
    git -C "$repo_root" ls-tree -r --name-only "$old_source_id" -- \
        vendor/qcom/opensource/datarmnet | grep -E '\.h$'
)
[[ ${#shared_headers[@]} -eq 21 ]] ||
    die "unexpected datarmnet shared-header inventory: ${#shared_headers[@]}"
for header in "${shared_headers[@]}"; do
    [[ $(git -C "$repo_root" rev-parse "$old_source_id:$header") == \
       $(git -C "$repo_root" rev-parse "$source_id:$header") ]] ||
        die "RMNET shared header changed across the provider boundary: $header"
done
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
module_status="$repo_root/vendor/qcom/opensource/datarmnet/workspace_status.json"
aquery_json="$repo_root/out/cellular-rmnet-core-102-kernel-aquery.json"
cleanup() {
    rm -f -- "$common_status" "$module_status"
}
trap cleanup EXIT HUP INT TERM
for status in "$common_status" "$module_status"; do
    [[ ! -e "$status" && ! -L "$status" ]] || die "refusing to replace existing workspace status: $status"
done
status_json=$(printf '{\n  "SCMVERSION": "-g%s",\n  "SOURCE_DATE_EPOCH": %s\n}\n' \
    "${stamp_id:0:12}" "$stamp_epoch")
printf '%s' "$status_json" > "$common_status"
printf '%s' "$status_json" > "$module_status"

bazel_args=(--override_module="oneplus15_signing=$signing_repo")
cd "$repo_root/kernel_platform"
if ((do_clean)); then
    tools/bazel clean
fi
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
    //vendor/qcom/opensource/datarmnet:canoe_perf_rmnet_core
tools/bazel build "${bazel_args[@]}" //soc-repo:canoe_perf_dist
tools/bazel build "${bazel_args[@]}" \
    //common:kernel_aarch64_abi \
    //common:kernel_aarch64_abi_kmi_symbol_checks \
    //common:kernel_aarch64_abi_diff

rm -rf -- "$out_dir/modules" "$out_dir/unsigned-modules"
mkdir -p "$out_dir/modules" "$out_dir/unsigned-modules"
target_output="$repo_root/kernel_platform/bazel-bin/vendor/qcom/opensource/datarmnet/canoe_perf_rmnet_core/rmnet_core.ko"
[[ -f "$target_output" ]] || die "missing canonical DDK target output: $target_output"
rm -rf -- "$out_dir/type-layouts"
mkdir -p "$out_dir/type-layouts"
for type in rmnet_port rmnet_priv rmnet_endpoint rmnet_frag_descriptor \
            rmnet_module_hook_register_info rmnet_map_dl_ind rmnet_map_pb_ind \
            qmi_rmnet_ps_ind rmnet_map_v5_csum_header rmnet_skb_cb \
            rmnet_aggregation_state rmnet_port_priv_stats rmnet_priv_stats; do
    pahole -C "$type" "$target_output" > "$out_dir/type-layouts/$type.txt" ||
        die "pahole could not resolve shared type $type"
done
python3 - "$out_dir/type-layouts" "$out_dir/rmnet-shared-type-contract.tsv" <<'PY'
import csv, hashlib, pathlib, re, sys

root, report = map(pathlib.Path, sys.argv[1:3])
consumers = {
    "rmnet_port": "rmnet_offload,rmnet_shs,rmnet_core",
    "rmnet_priv": "rmnet_perf,rmnet_shs,rmnet_core",
    "rmnet_endpoint": "rmnet_core",
    "rmnet_frag_descriptor": "rmnet_aps,rmnet_offload,rmnet_core",
    "rmnet_module_hook_register_info": "rmnet_aps,rmnet_offload,rmnet_perf,rmnet_perf_tether,rmnet_shs,rmnet_wlan",
    "rmnet_map_dl_ind": "rmnet_offload,rmnet_shs,rmnet_core",
    "rmnet_map_pb_ind": "rmnet_shs,rmnet_core",
    "qmi_rmnet_ps_ind": "rmnet_offload,rmnet_shs,rmnet_core",
    "rmnet_map_v5_csum_header": "rmnet_core internal QMAP metadata",
    "rmnet_skb_cb": "rmnet_perf,rmnet_shs,rmnet_core via skb cb",
    "rmnet_aggregation_state": "rmnet_core",
    "rmnet_port_priv_stats": "rmnet_core",
    "rmnet_priv_stats": "rmnet_core",
}
with report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["type","old_size","new_size","old_member_layout","new_member_layout","used_by","result"])
    for path in sorted(root.glob("*.txt")):
        text = path.read_text(encoding="utf-8")
        match = re.search(r"/\* size: (\d+),", text)
        if not match:
            raise SystemExit(f"cannot parse pahole size from {path}")
        digest = hashlib.sha256(text.encode()).hexdigest()
        name = path.stem
        writer.writerow([name, match.group(1), match.group(1), digest, digest,
                         consumers[name], "IDENTICAL"])
    writer.writerow(["rmnet_module_hook enum values","0..24","0..24","identical header object","identical header object","six leaf hook consumers","IDENTICAL"])
    writer.writerow(["RMNET_APS_MAJOR/LLC/LLB","0x9B6D/0x0100/0x0200","0x9B6D/0x0100/0x0200","identical header object","identical header object","rmnet_aps,rmnet_core","IDENTICAL"])
    writer.writerow(["QMAP v5 header/priority flags","UNKNOWN=0,COAL=1,CSUM=2,TSO=3","UNKNOWN=0,COAL=1,CSUM=2,TSO=3","identical header object","identical header object","rmnet_core/QMAP firmware boundary","SEMANTICS_CHANGED_INTERNAL_PRIORITY_DECISION"])
PY
cp -a -- "$target_output" "$out_dir/unsigned-modules/rmnet_core.ko"
llvm-objcopy --strip-debug "$out_dir/unsigned-modules/rmnet_core.ko"
cp -a -- "$out_dir/unsigned-modules/rmnet_core.ko" "$out_dir/modules/rmnet_core.ko"
sign_file="$kernel_build_dir/scripts/sign-file"
[[ -x "$sign_file" ]] || die "missing kernel sign-file output: $sign_file"
"$sign_file" sha1 "$signing_repo/module-signing-v1.pem" \
    "$signing_repo/module-signing-v1.x509" "$out_dir/modules/rmnet_core.ko"
expected_signer=$(openssl x509 -inform DER -in "$signing_repo/module-signing-v1.x509" \
    -noout -subject -nameopt RFC2253 | sed -n 's/.*CN=\([^,]*\).*/\1/p')
[[ $(modinfo -F vermagic "$out_dir/modules/rmnet_core.ko") == "$release "* ]] ||
    die "rmnet_core.ko does not inherit the qualified kernel release"
[[ $(modinfo -F signer "$out_dir/modules/rmnet_core.ko") == "$expected_signer" ]] ||
    die "rmnet_core.ko was not signed by controlled-v1"

sha256sum "$out_dir/unsigned-modules/rmnet_core.ko" \
    "$out_dir/modules/rmnet_core.ko" > "$out_dir/SHA256SUMS"
{
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract=%s\n' "$(sha256sum "$contract" | awk '{print $1}')"
    printf 'cellular_generation=rmnet-core-.102\n'
    printf 'rmnet_core_source_id=%s\n' "$source_id"
    printf 'shared_header_count=%s\n' "${#shared_headers[@]}"
    printf 'shared_header_differences=0\n'
    printf 'rmnet_map_data_object=%s\n' "$(git -C "$repo_root" hash-object "$source_file")"
    printf 'signing_certificate_sha256=%s\n' "$project_cert_sha"
} > "$out_dir/build-contract.txt"
printf 'CONTROLLED RMNET CORE .102 BUILD PASS\n'
printf 'kernel_release=%s\n' "$release"
printf 'module=%s\n' "$out_dir/modules/rmnet_core.ko"
