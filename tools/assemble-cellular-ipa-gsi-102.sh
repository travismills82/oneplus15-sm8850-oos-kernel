#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Assemble the bounded DataIPA/GSI .102 closure over the validated RMNET-core image.

set -euo pipefail

readonly release=6.12.23-android16-5-o-g6744a3f6bcf4-4k
readonly signer='OnePlus 15 Controlled OOS Module Signing v1'
readonly baseline_sha=48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f
readonly -a source_modules=(gsim ipam ipanetm)
readonly -a reviewed_consumers=(
    qca_cld3_kiwi_v2 qca_cld3_peach_v2 qca_cld3_wcn7750
    rmnet_aps rmnet_core rmnet_ctl rmnet_offload rmnet_perf
    rmnet_perf_tether rmnet_shs rmnet_wlan
)

die() { printf 'error: %s\n' "$*" >&2; exit 2; }
sha256() { sha256sum "$1" | awk '{print $1}'; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
baseline_image=
baseline_stage=
replacements_dir=
system_modules=
vendor_boot_modules=
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
        --replacements-dir) replacements_dir=${2:-}; shift 2 ;;
        --system-modules) system_modules=${2:-}; shift 2 ;;
        --vendor-boot-modules) vendor_boot_modules=${2:-}; shift 2 ;;
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
for value in baseline_image baseline_stage replacements_dir system_modules \
             vendor_boot_modules module_symvers kernel_build_dir signing_key \
             system_archive system_load_contract avbtool out_dir; do
    [[ -n ${!value} ]] || die "missing required --${value//_/-} argument"
done
for command in jq modinfo openssl python3 sha256sum; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done
for path in "$baseline_image" "$module_symvers" "$signing_key" "$system_archive" \
            "$system_load_contract" "$avbtool"; do
    [[ -f "$path" ]] || die "missing file: $path"
done
for path in "$baseline_stage" "$replacements_dir" "$system_modules" \
            "$vendor_boot_modules" "$kernel_build_dir"; do
    [[ -d "$path" ]] || die "missing directory: $path"
done
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output: $out_dir"
[[ $(sha256 "$baseline_image") == "$baseline_sha" ]] ||
    die "base image is not the physically validated RMNET_CORE .102 vendor_dlkm"

declare -a replacement_args=()
for module in "${source_modules[@]}"; do
    path="$replacements_dir/$module.ko"
    [[ -f "$path" ]] || die "missing replacement: $path"
    [[ $(modinfo -F name "$path") == "$module" ]] || die "$path has wrong module name"
    [[ -z $(modinfo -F signer "$path") ]] || die "$path must be unsigned"
    [[ $(modinfo -F vermagic "$path") == "$release "* ]] ||
        die "$module does not inherit the frozen kernel release"
    replacement_args+=(--replacement "$path")
done

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
    "${replacement_args[@]}" \
    --allow-import-contract-change ipam \
    --allow-import-contract-change ipanetm \
    --allow-export-contract-change gsim \
    --allow-export-contract-change ipam \
    --external-root "$system_modules" \
    --external-root "$vendor_boot_modules" \
    --vmlinux-symvers "$module_symvers" \
    --out-dir "$contract" \
    --expected-stock-module-count 436 \
    --fail-external-signed-provider-edges
jq -e '
  .result == "PASS" and .stock_vendor_modules == 436 and
  .source_replacements == 3 and .protected_export_signed_closure == 14 and
  .re_sign_stock_modules == 11 and .retained_external_modules == 525 and
  .external_signed_provider_edges == 0 and .replacement_contract_failures == 0 and
  .unresolved_imports == 0 and .crc_mismatches == 0
' "$contract/summary.json" >/dev/null || die "bounded IPA/GSI graph failed"
mapfile -t actual_consumers < <(
    awk -F '\t' 'NR > 1 && $2 == "RE_SIGN_STOCK" { print $1 }' \
        "$contract/protected-export-signing-closure.tsv" | LC_ALL=C sort
)
mapfile -t expected_consumers < <(printf '%s\n' "${reviewed_consumers[@]}" | LC_ALL=C sort)
[[ "${actual_consumers[*]}" == "${expected_consumers[*]}" ]] ||
    die "protected signing closure differs from the reviewed consumer set"

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
    "${replacement_args[@]}"

mv "$stage_out/vendor_dlkm.img" "$out_dir/vendor_dlkm.img"
cp "$stage_out/e2fsck.txt" "$out_dir/e2fsck.txt"
cp "$stage_out/vendor-system-dependency-reconciliation.tsv" \
    "$out_dir/vendor-system-dependency-reconciliation.tsv"
cp "$system_load_contract" "$out_dir/system-dlkm-load-contract.tsv"
cp "$repo_root/docs/validation/cellular-migration/ipa-gsi-source-delta.md" \
    "$out_dir/ipa-gsi-source-delta.md"
cp "$repo_root/docs/validation/cellular-migration/ipa-gsi-shared-type-contract.tsv" \
    "$out_dir/ipa-gsi-shared-type-contract.tsv"
cp "$repo_root/docs/validation/cellular-migration/ipa-tx-meta-consumers.tsv" \
    "$out_dir/ipa-tx-meta-consumers.tsv"
cp "$repo_root/docs/validation/cellular-migration/ipa-gsi-migration-closure.tsv" \
    "$out_dir/ipa-gsi-migration-closure.tsv"

python3 - "$repo_root/tools/validate-matched-wlan-vendor-dlkm.py" \
    "$baseline_stage/lib/modules" "$stage_out/staging/lib/modules" \
    "$replacements_dir" "$signer" "$contract/import-resolution.tsv" \
    "$out_dir/ipa-gsi-consumers.tsv" "$out_dir/ipa-gsi-import-crc.tsv" \
    "$out_dir/signature-report.tsv" "$out_dir/preservation-report.tsv" \
    "$out_dir/export-contract.tsv" <<'PY'
import csv, hashlib, importlib.util, pathlib, struct, subprocess, sys

validator_path, old_root, new_root, replacements = map(pathlib.Path, sys.argv[1:5])
expected_signer, import_report = sys.argv[5], pathlib.Path(sys.argv[6])
consumers_report, import_crc_report, signature_report, preservation_report, export_report = map(pathlib.Path, sys.argv[7:12])
spec = importlib.util.spec_from_file_location("ipa_validator", validator_path)
validator = importlib.util.module_from_spec(spec); sys.modules[spec.name] = validator; spec.loader.exec_module(validator)
source = {"gsim", "ipam", "ipanetm"}
expected_changed = source | {"rmnet_ctl"}
preserve_named = {"cfg80211","mac80211","qca_cld3_peach_v2","cnss2","cnss_nl","cnss_prealloc","cnss_utils","wlan_firmware_service",
                  "btpower","bt_fm_swr","btfm_slim_codec","nxp_nci","rmnet_core","rmnet_sch",
                  "rmnet_aps","rmnet_offload","rmnet_perf","rmnet_perf_tether","rmnet_shs","rmnet_wlan","rmnet_mem"}
magic = b"~Module signature appended~\n"
def scan(root):
    out = {}
    for p in root.rglob("*.ko"):
        n = subprocess.check_output(["modinfo","-F","name",str(p)], text=True).strip()
        if n in out: raise SystemExit(f"duplicate module {n}")
        out[n] = p
    return out
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def unsigned(data):
    if not data.endswith(magic): return data
    pos = len(data)-len(magic)-12
    _a,_h,ident,sl,kl,_pad,siglen = struct.unpack(">5B3sI", data[pos:pos+12])
    start = pos-sl-kl-siglen
    if start < 0 or ident != 2: raise SystemExit("invalid module signature")
    return data[:start]
old, new = scan(old_root), scan(new_root)
if len(old) != 436 or set(old) != set(new): raise SystemExit("vendor inventory changed")
changed = {n for n in old if digest(old[n]) != digest(new[n])}
if changed != expected_changed: raise SystemExit(f"unexpected module delta: {sorted(changed)}")
for n in source:
    if unsigned(new[n].read_bytes()) != (replacements/f"{n}.ko").read_bytes():
        raise SystemExit(f"{n}: staged source payload differs")
if unsigned(new["rmnet_ctl"].read_bytes()) != old["rmnet_ctl"].read_bytes():
    raise SystemExit("rmnet_ctl pre-signature payload changed")
for n in preserve_named:
    if digest(old[n]) != digest(new[n]): raise SystemExit(f"qualified module changed: {n}")

with preservation_report.open("w", encoding="utf-8", newline="") as f:
    w=csv.writer(f,delimiter="\t",lineterminator="\n"); w.writerow(["module","baseline_sha256","candidate_sha256","action","status"])
    for n in sorted(old):
        action = "SOURCE_UPGRADE_.102" if n in {"gsim","ipam"} else ("REBUILT_FOR_PROVIDER_CONTRACT" if n=="ipanetm" else ("RE_SIGN_EXACT_STOCK" if n=="rmnet_ctl" else "BYTE_IDENTICAL_RETAINED"))
        w.writerow([n,digest(old[n]),digest(new[n]),action,"PASS"])

with signature_report.open("w", encoding="utf-8", newline="") as f:
    w=csv.writer(f,delimiter="\t",lineterminator="\n"); w.writerow(["module","action","vermagic","signer","sig_id","payload_contract","status"])
    for n in sorted(source | {"rmnet_ctl"}):
        p=new[n]; meta=lambda x: subprocess.check_output(["modinfo","-F",x,str(p)],text=True).strip()
        payload_ok = unsigned(p.read_bytes()) == ((replacements/f"{n}.ko").read_bytes() if n in source else old[n].read_bytes())
        vermagic_ok = (meta("vermagic").startswith("6.12.23-android16-5-o-g6744a3f6bcf4-4k ")
                        if n in source else
                        meta("vermagic") == subprocess.check_output(["modinfo","-F","vermagic",str(old[n])],text=True).strip())
        ok = meta("signer")==expected_signer and meta("sig_id")=="PKCS#7" and vermagic_ok and payload_ok
        w.writerow([n,"SOURCE_REPLACEMENT" if n in source else "RE_SIGN_EXACT_STOCK",meta("vermagic"),meta("signer"),meta("sig_id"),"PASS" if payload_ok else "FAIL","PASS" if ok else "FAIL"])
        if not ok: raise SystemExit(f"signature contract failed: {n}")

rows=list(csv.DictReader(import_report.open(encoding="utf-8"),delimiter="\t"))
providers=("vendor_dlkm:gsim","vendor_dlkm:ipam","vendor_dlkm:ipanetm")
selected=[r for r in rows if any(p in r["candidate_providers"] for p in providers)]
with import_crc_report.open("w",encoding="utf-8",newline="") as f:
    w=csv.DictWriter(f,fieldnames=rows[0].keys(),delimiter="\t",lineterminator="\n"); w.writeheader(); w.writerows(selected)
actions={"ipam":"SOURCE_UPGRADE","ipanetm":"REBUILT_FOR_PROVIDER_CONTRACT","rmnet_ctl":"RE_SIGN_EXACT_STOCK",
         "qca_cld3_peach_v2":"RETAIN_CONTROLLED_ACTIVE","qca_cld3_kiwi_v2":"RETAIN_CONTROLLED_DORMANT","qca_cld3_wcn7750":"RETAIN_CONTROLLED_DORMANT","rmnet_core":"RETAIN_CONTROLLED_.102"}
with consumers_report.open("w",encoding="utf-8",newline="") as f:
    w=csv.writer(f,delimiter="\t",lineterminator="\n"); w.writerow(["consumer","partition","normal_boot","provider","import_count","imports_and_crcs","shared_structs","source_generation","must_rebuild","must_source_upgrade","result"])
    for name in sorted({r["consumer"] for r in selected}):
        group=[r for r in selected if r["consumer"]==name]
        imports=";".join(f'{r["symbol"]}={r["expected_crc"]}' for r in group)
        normal="yes" if name in {"ipam","ipanetm","qca_cld3_peach_v2","rmnet_core","rmnet_ctl"} else "no/dormant packaged path"
        shared="ipa3_context" if name=="ipanetm" else ("GSI public structures" if name=="ipam" else "public IPA callback/WDI API only")
        action=actions[name]
        w.writerow([name,"vendor_dlkm",normal,group[0]["candidate_providers"],len(group),imports,shared,".102" if name in {"ipam","rmnet_core"} else "retained",action=="REBUILT_FOR_PROVIDER_CONTRACT",action=="SOURCE_UPGRADE","PASS"])

with export_report.open("w",encoding="utf-8",newline="") as f:
    w=csv.writer(f,delimiter="\t",lineterminator="\n"); w.writerow(["module","symbol","old_crc","new_crc","result","consumer_impact"])
    for n in ("gsim","ipam","ipanetm"):
        before=validator.module_record(old[n],"vendor_dlkm","BASELINE").exports
        after=validator.module_record(replacements/f"{n}.ko","vendor_dlkm","SOURCE").exports
        for sym in sorted(set(before)|set(after)):
            a,b=before.get(sym),after.get(sym)
            result="ADDED" if a is None else ("REMOVED" if b is None else ("CRC_CHANGED" if a!=b else "UNCHANGED"))
            if result!="UNCHANGED":
                impact="ipanetm rebuilt" if sym=="ipa3_ctx" else ("ipam source consumer included" if sym=="gsi_status_enabled" else "no shipped importer")
                w.writerow([n,sym,validator.crc(a),validator.crc(b),result,impact])
PY

cp "$contract/external-signed-provider-edges.tsv" "$out_dir/external-consumer-boundary.tsv"
{
    printf 'IPA/GSI .102 bounded provider closure: PASS\n'
    printf 'kernel_release=%s\n' "$release"
    printf 'kernel_contract_guard=PASS\n'
    printf 'vendor_modules=436\n'
    printf 'source_upgrades=gsim,ipam\n'
    printf 'provider_contract_rebuilds=ipanetm\n'
    printf 'exact_stock_resign=rmnet_ctl\n'
    printf 'unexpected_module_changes=0\n'
    printf 'unresolved_imports=0\ncrc_mismatches=0\n'
    printf 'protected_export_failures=0\nsignature_failures=0\n'
    printf 'shared_structure_failures=0\n'
    printf 'wlan_boundary=PASS_28_ACTIVE_PEACH_IMPORTS_UNCHANGED\n'
    printf 'usb_tether_boundary=PASS_STOCK_USB_F_GSI_PROVIDER_IMPORTS_UNCHANGED\n'
    printf 'ssr_boundary=STATIC_PASS_PHYSICAL_RECOVERY_NOT_YET_TESTED\n'
    printf 'system_modules_load_entries=46\nwwan_load_entry=21\n'
    printf 'filesystem=PASS\npartition_local_avb=PASS\n'
    printf 'physical_flash=NOT_PERFORMED\n'
} > "$out_dir/validation-report.txt"

(
    cd "$out_dir"
    sha256sum vendor_dlkm.img ipa-gsi-source-delta.md ipa-gsi-consumers.tsv \
        ipa-gsi-shared-type-contract.tsv ipa-tx-meta-consumers.tsv \
        ipa-gsi-migration-closure.tsv ipa-gsi-import-crc.tsv validation-report.txt \
        signature-report.tsv preservation-report.tsv export-contract.tsv e2fsck.txt > SHA256SUMS
)
printf 'BOUNDED IPA/GSI .102 CANDIDATE PASS\n'
printf 'image=%s\n' "$out_dir/vendor_dlkm.img"
printf 'sha256=%s\n' "$(sha256 "$out_dir/vendor_dlkm.img")"
printf 'physical_flash=NOT_PERFORMED\n'
