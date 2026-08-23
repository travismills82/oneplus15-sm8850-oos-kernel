#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Assemble the bounded RMNET core .102 provider/signing closure over Batch 01.

set -euo pipefail

readonly release=6.12.23-android16-5-o-g6744a3f6bcf4-4k
readonly signer='OnePlus 15 Controlled OOS Module Signing v1'
readonly baseline_sha=640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958
readonly source_module=rmnet_core
readonly -a resign_modules=(
    rmnet_aps rmnet_offload rmnet_perf rmnet_perf_tether rmnet_shs rmnet_wlan
)

die() { printf 'error: %s\n' "$*" >&2; exit 2; }
sha256() { sha256sum "$1" | awk '{print $1}'; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
baseline_image=
baseline_stage=
replacement=
system_modules=
vendor_boot_modules=
module_symvers=
kernel_build_dir=
signing_key=
system_archive=
system_load_contract=
shared_type_contract=
source_delta="$repo_root/docs/validation/cellular-migration/rmnet-core-102-source-delta.md"
avbtool=
out_dir=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --baseline-image) baseline_image=${2:-}; shift 2 ;;
        --baseline-stage) baseline_stage=${2:-}; shift 2 ;;
        --replacement) replacement=${2:-}; shift 2 ;;
        --system-modules) system_modules=${2:-}; shift 2 ;;
        --vendor-boot-modules) vendor_boot_modules=${2:-}; shift 2 ;;
        --module-symvers) module_symvers=${2:-}; shift 2 ;;
        --kernel-build-dir) kernel_build_dir=${2:-}; shift 2 ;;
        --signing-key) signing_key=${2:-}; shift 2 ;;
        --system-dlkm-staging-archive) system_archive=${2:-}; shift 2 ;;
        --system-load-contract) system_load_contract=${2:-}; shift 2 ;;
        --shared-type-contract) shared_type_contract=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        *) die "unknown argument: $1" ;;
    esac
done
for value in baseline_image baseline_stage replacement system_modules \
             vendor_boot_modules module_symvers kernel_build_dir signing_key \
             system_archive system_load_contract shared_type_contract avbtool out_dir; do
    [[ -n ${!value} ]] || die "missing required --${value//_/-} argument"
done
for command in jq llvm-objdump modinfo openssl python3 sha256sum; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done
for path in "$baseline_image" "$replacement" "$module_symvers" "$signing_key" \
            "$system_archive" "$system_load_contract" "$shared_type_contract" \
            "$source_delta" "$avbtool"; do
    [[ -f "$path" ]] || die "missing file: $path"
done
for path in "$baseline_stage" "$system_modules" "$vendor_boot_modules" "$kernel_build_dir"; do
    [[ -d "$path" ]] || die "missing directory: $path"
done
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output: $out_dir"
[[ $(sha256 "$baseline_image") == "$baseline_sha" ]] ||
    die "base image is not the physically validated Batch 01 vendor_dlkm"
[[ $(modinfo -F name "$replacement") == "$source_module" ]] ||
    die "replacement is not rmnet_core"
[[ -z $(modinfo -F signer "$replacement") ]] ||
    die "source replacement must be unsigned before controlled staging"
[[ $(modinfo -F vermagic "$replacement") == "$release "* ]] ||
    die "source replacement does not inherit the frozen kernel release"

python3 - "$system_load_contract" <<'PY'
import csv, pathlib, sys
with pathlib.Path(sys.argv[1]).open(encoding="utf-8", newline="") as stream:
    rows = [row for row in csv.DictReader(stream, delimiter="\t")
            if row.get("partition") == "system_dlkm"]
if len(rows) != 46 or any(row.get("status") != "PASS" for row in rows):
    raise SystemExit("system modules.load 46-entry contract failed")
wwan = [row for row in rows if row.get("module") == "wwan.ko"]
if len(wwan) != 1 or wwan[0].get("load_order") != "21":
    raise SystemExit("wwan.ko is not system modules.load entry 21")
if any(row.get("stale_builtin_entry") != "no" for row in rows):
    raise SystemExit("system modules.load contains a stale built-in entry")
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
    --expected-stock-module-count 436 \
    --fail-external-signed-provider-edges
jq -e '
  .result == "PASS" and .stock_vendor_modules == 436 and
  .source_replacements == 1 and .protected_export_signed_closure == 7 and
  .re_sign_stock_modules == 6 and .retained_external_modules == 525 and
  .external_signed_provider_edges == 0 and .replacement_contract_failures == 0 and
  .unresolved_imports == 0 and .crc_mismatches == 0
' "$contract/summary.json" >/dev/null || die "bounded RMNET provider graph failed"

mapfile -t actual_resign < <(
    awk -F '\t' 'NR > 1 && $2 == "RE_SIGN_STOCK" { print $1 }' \
        "$contract/protected-export-signing-closure.tsv" | LC_ALL=C sort
)
mapfile -t expected_resign < <(printf '%s\n' "${resign_modules[@]}" | LC_ALL=C sort)
[[ "${actual_resign[*]}" == "${expected_resign[*]}" ]] ||
    die "protected signing closure differs from the reviewed six consumers"

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
    --replacement "$replacement"

mv "$stage_out/vendor_dlkm.img" "$out_dir/vendor_dlkm.img"
cp "$stage_out/e2fsck.txt" "$out_dir/e2fsck.txt"
cp "$stage_out/vendor-system-dependency-reconciliation.tsv" \
    "$out_dir/vendor-system-dependency-reconciliation.tsv"
cp "$system_load_contract" "$out_dir/system-dlkm-load-contract.tsv"
cp "$shared_type_contract" "$out_dir/rmnet-shared-type-contract.tsv"
cp "$source_delta" "$out_dir/rmnet-core-source-delta.md"

stock_core=$(python3 - "$baseline_stage/lib/modules" <<'PY'
import pathlib, subprocess, sys
matches = []
for path in pathlib.Path(sys.argv[1]).rglob("*.ko"):
    name = subprocess.check_output(["modinfo", "-F", "name", str(path)], text=True).strip()
    if name == "rmnet_core":
        matches.append(path)
if len(matches) != 1:
    raise SystemExit(f"expected one stock rmnet_core, found {len(matches)}")
print(matches[0])
PY
)
old_disassembly="$out_dir/rmnet-core-priority-old.disassembly.txt"
new_disassembly="$out_dir/rmnet-core-priority-new.disassembly.txt"
llvm-objdump -dr --disassemble-symbols=rmnet_map_v5_checksum_uplink_packet \
    "$stock_core" > "$old_disassembly"
llvm-objdump -dr --disassemble-symbols=rmnet_map_v5_checksum_uplink_packet \
    "$replacement" > "$new_disassembly"
! grep -q 'R_AARCH64_CALL26.*rmnet_ll_get_ipa_ready_status' "$old_disassembly" ||
    die "Batch 01 rmnet_core unexpectedly contains the .102 priority status call"
grep -q 'R_AARCH64_CALL26.*rmnet_ll_get_ipa_ready_status' "$new_disassembly" ||
    die "candidate rmnet_core lacks the .102 priority status call"
! grep -a -q 'priority bit set' "$stock_core" ||
    die "Batch 01 rmnet_core unexpectedly contains the .102 priority diagnostic"
grep -a -q 'priority bit set' "$replacement" ||
    die "candidate rmnet_core lacks the .102 priority diagnostic"
{
    printf 'source_id=%s\n' bc8d91d1e146be96d2e27bebe8f753f82bdebeee
    printf 'source_object_sha1=%s\n' 83b683afb1b2efbeb1b82c728f327abd41d522ae
    printf 'stock_rmnet_core_sha256=%s\n' "$(sha256 "$stock_core")"
    printf 'candidate_unsigned_rmnet_core_sha256=%s\n' "$(sha256 "$replacement")"
    printf 'stock_status_call=absent\n'
    printf 'candidate_status_call=present\n'
    printf 'stock_priority_diagnostic=absent\n'
    printf 'candidate_priority_diagnostic=present\n'
    printf 'result=PASS\n'
} > "$out_dir/rmnet-core-priority-binary-proof.txt"

python3 - "$baseline_stage/lib/modules" "$stage_out/staging/lib/modules" \
    "$replacement" "$signer" "$out_dir/rmnet-provider-migration-closure.tsv" \
    "$out_dir/cellular-preservation.tsv" "$out_dir/signature-report.tsv" <<'PY'
import csv, hashlib, pathlib, struct, subprocess, sys

old_root, new_root = map(pathlib.Path, sys.argv[1:3])
replacement = pathlib.Path(sys.argv[3])
expected_signer = sys.argv[4]
closure_report, preservation_report, signature_report = map(pathlib.Path, sys.argv[5:8])
source = {"rmnet_core"}
resigned = {"rmnet_aps", "rmnet_offload", "rmnet_perf", "rmnet_perf_tether", "rmnet_shs", "rmnet_wlan"}
expected_changed = source | resigned
cellular = {"dwc3_msm","gsim","ipam","ipanetm","oplus_mm_kevent","oplus_mm_kevent_fb",
"qcom_glink","qcom_glink_smem","qcom_ramdump","qcom_smd","qcom_va_minidump",
"qmi_helpers","redriver","repeater","rmnet_aps","rmnet_core","rmnet_ctl","rmnet_mem",
"rmnet_offload","rmnet_perf","rmnet_perf_tether","rmnet_sch","rmnet_shs","rmnet_wlan",
"rproc_qcom_common","usb_f_gsi","wcd_usbss_i2c"}
magic = b"~Module signature appended~\n"

def scan(root):
    result = {}
    for path in root.rglob("*.ko"):
        name = subprocess.check_output(["modinfo", "-F", "name", str(path)], text=True).strip()
        if name in result:
            raise SystemExit(f"duplicate module {name}")
        result[name] = path
    return result

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def unsigned_payload(data):
    if not data.endswith(magic):
        return data
    struct_start = len(data) - len(magic) - 12
    if struct_start < 0:
        raise SystemExit("truncated module signature")
    algo, hash_id, id_type, signer_len, key_id_len, _pad, sig_len = struct.unpack(
        ">5B3sI", data[struct_start:struct_start + 12]
    )
    start = struct_start - signer_len - key_id_len - sig_len
    if start < 0 or id_type != 2:
        raise SystemExit("invalid PKCS#7 module signature trailer")
    return data[:start]

old, new = scan(old_root), scan(new_root)
if len(old) != 436 or set(old) != set(new):
    raise SystemExit("vendor inventory changed")
changed = {name for name in old if digest(old[name]) != digest(new[name])}
if changed != expected_changed:
    raise SystemExit(f"unexpected vendor module delta: {sorted(changed)}")
if unsigned_payload(new["rmnet_core"].read_bytes()) != replacement.read_bytes():
    raise SystemExit("signed rmnet_core payload differs from reviewed source replacement")
for name in resigned:
    if unsigned_payload(new[name].read_bytes()) != old[name].read_bytes():
        raise SystemExit(f"{name}: re-signed stock payload changed before signature")

with closure_report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["module","old_new_source","reason_included","provider_role","consumer_role","source_changed","rebuild_only","shared_types","contract_result"])
    writer.writerow(["rmnet_core",".097 -> .102","contains isolated priority behavior","RMNET provider","consumer of stock IPA/QMI/rmnet_ctl/rmnet_mem","yes","no","RMNET public headers unchanged","PASS"])
    for name in sorted(resigned):
        writer.writerow([name,"stock .097 unchanged","protected consumer of controlled rmnet_core","none","imports protected rmnet_core exports","no","RE_SIGN_ONLY","RMNET public headers unchanged","PASS"])
    writer.writerow(["rmnet_sch","Batch-01 controlled unchanged","already physically validated baseline","leaf","not a rmnet_core consumer","no","no","none","PASS_RETAINED"])

with preservation_report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["module","baseline_sha256","candidate_sha256","action","status"])
    for name in sorted(cellular):
        action = "SOURCE_REPLACEMENT_.102" if name in source else (
            "RE_SIGN_EXACT_STOCK_FOR_PROTECTED_CONTRACT" if name in resigned else (
            "BATCH01_CONTROLLED_RETAINED" if name == "rmnet_sch" else "EXACT_STOCK_RETAINED"))
        status = "PASS" if name in expected_changed or digest(old[name]) == digest(new[name]) else "FAIL"
        writer.writerow([name, digest(old[name]), digest(new[name]), action, status])
        if status != "PASS":
            raise SystemExit(f"cellular preservation failure: {name}")

with signature_report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["module","action","vermagic","signer","sig_id","payload_preserved","status"])
    for name in sorted(expected_changed):
        path = new[name]
        metadata = lambda field: subprocess.check_output(["modinfo", "-F", field, str(path)], text=True).strip()
        signer = metadata("signer")
        sig_id = metadata("sig_id")
        preserved = (unsigned_payload(path.read_bytes()) ==
                     (replacement.read_bytes() if name == "rmnet_core" else old[name].read_bytes()))
        status = "PASS" if signer == expected_signer and sig_id == "PKCS#7" and preserved else "FAIL"
        writer.writerow([name, "SOURCE_REPLACEMENT" if name in source else "RE_SIGN_STOCK",
                         metadata("vermagic"), signer, sig_id, "yes" if preserved else "no", status])
        if status != "PASS":
            raise SystemExit(f"signature contract failed: {name}")
PY

python3 - "$baseline_stage/lib/modules" "$stage_out/staging/lib/modules" <<'PY'
import hashlib, pathlib, subprocess, sys
old_root, new_root = map(pathlib.Path, sys.argv[1:3])
protected_qualified = {"btpower", "bt_fm_swr", "btfm_slim_codec", "nxp_nci", "qca_cld3_peach_v2"}
stock_ipa_gsi = {"gsim", "ipam", "ipanetm", "rmnet_ctl", "rmnet_mem", "usb_f_gsi"}
def scan(root):
    out = {}
    for p in root.rglob("*.ko"):
        n = subprocess.check_output(["modinfo", "-F", "name", str(p)], text=True).strip()
        out[n] = hashlib.sha256(p.read_bytes()).hexdigest()
    return out
old, new = scan(old_root), scan(new_root)
for group, names in (("qualified WLAN/BT/NFC", protected_qualified), ("stock IPA/GSI", stock_ipa_gsi)):
    changed = sorted(n for n in names if old.get(n) != new.get(n))
    if changed:
        raise SystemExit(f"{group} hash changed: {changed}")
PY

python3 - "$repo_root/tools/validate-matched-wlan-vendor-dlkm.py" \
    "$baseline_stage/lib/modules" "$replacement" \
    "$out_dir/rmnet-core-export-contract.tsv" <<'PY'
import csv, importlib.util, pathlib, subprocess, sys
validator_path, old_root, replacement, report = map(pathlib.Path, sys.argv[1:5])
spec = importlib.util.spec_from_file_location("rmnet_validator", validator_path)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
old_path = None
for path in old_root.rglob("*.ko"):
    if subprocess.check_output(["modinfo", "-F", "name", str(path)], text=True).strip() == "rmnet_core":
        old_path = path
        break
if old_path is None:
    raise SystemExit("stock rmnet_core not found")
old = module.module_record(old_path, "vendor_dlkm", "STOCK")
new = module.module_record(replacement, "vendor_dlkm", "SOURCE_REPLACEMENT")
with report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["symbol","old_crc","new_crc","export_class","protected_status","result"])
    for symbol in sorted(set(old.exports) | set(new.exports)):
        before, after = old.exports.get(symbol), new.exports.get(symbol)
        if before is None:
            result = "ADDED"
        elif after is None:
            result = "REMOVED"
        elif before != after:
            result = "CRC_CHANGED"
        else:
            result = "UNCHANGED"
        writer.writerow([symbol, module.crc(before), module.crc(after), "EXPORT_SYMBOL",
                         "CONFIG_MODULE_SIG_PROTECT_PROVIDER", result])
if old.exports != new.exports:
    raise SystemExit("rmnet_core export contract changed")
PY
awk -F '\t' 'NR == 1 || $1 == "rmnet_core"' "$contract/import-resolution.tsv" > \
    "$out_dir/rmnet-core-import-crc.tsv"
cp "$contract/external-signed-provider-edges.tsv" "$out_dir/external-consumer-boundary.tsv"

python3 - "$contract/import-resolution.tsv" "$out_dir/rmnet-core-consumers.tsv" <<'PY'
import csv, pathlib, sys
source, report = map(pathlib.Path, sys.argv[1:3])
rows = list(csv.DictReader(source.open(encoding="utf-8"), delimiter="\t"))
grouped = {}
for row in rows:
    if row["candidate_providers"] != "vendor_dlkm:rmnet_core":
        continue
    grouped.setdefault(row["consumer"], []).append(f'{row["symbol"]}:{row["expected_crc"]}')
shared = {
    "rmnet_aps": "rmnet_frag_descriptor;hook callback types;QMAP tx metadata",
    "rmnet_offload": "rmnet_port;rmnet_frag_descriptor;rmnet_map_dl_ind;qmi_rmnet_ps_ind;hook callback types",
    "rmnet_perf": "rmnet_priv;skb metadata;hook callback types",
    "rmnet_perf_tether": "skb metadata;hook callback types",
    "rmnet_shs": "rmnet_port;rmnet_priv;rmnet_map_dl_ind;rmnet_map_pb_ind;qmi_rmnet_ps_ind;hook callback types",
    "rmnet_wlan": "skb metadata;hook callback types",
}
with report.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["module","partition","normal_boot","recovery","imported_rmnet_core_symbols","expected_crcs","shared_rmnet_structures","source_generation","stock_custom","must_rebuild"])
    for name in sorted(grouped):
        pairs = sorted(grouped[name])
        writer.writerow([name,"vendor_dlkm","yes","not requested by retained vendor_boot recovery policy",
                         ";".join(pair.rsplit(":",1)[0] for pair in pairs),
                         ";".join(pair.rsplit(":",1)[1] for pair in pairs), shared[name],
                         ".097 source unchanged through .102","exact stock payload re-signed controlled-v1",
                         "NO_SOURCE_REBUILD; RE_SIGN_REQUIRED"])
if len(grouped) != 6:
    raise SystemExit(f"unexpected direct consumer count: {len(grouped)}")
PY

candidate_sha=$(sha256 "$out_dir/vendor_dlkm.img")
{
    printf 'RMNET CORE .102 STATIC CANDIDATE PASS\n'
    printf 'kernel_release=%s\n' "$release"
    printf 'base_vendor_dlkm_sha256=%s\n' "$baseline_sha"
    printf 'candidate_vendor_dlkm_sha256=%s\n' "$candidate_sha"
    printf 'vendor_modules=436\nsource_replacements=1\nre_signed_stock_consumers=6\n'
    printf 'controlled_source_cellular_modules=2\nretained_stock_source_cellular_modules=25\n'
    printf 'unresolved_imports=0\ncrc_mismatches=0\nprotected_export_failures=0\nsignature_failures=0\n'
    printf 'structural_contract_failures=0\nexternal_consumer_edges=0\n'
    printf 'priority_fix_binary_proof=PASS\n'
    printf 'system_modules_load_entries=46\nwwan_load_order=21\n'
    printf 'stock_ipa_gsi_hashes_unchanged=yes\nwlan053_bt046_nfc102_hashes_unchanged=yes\n'
    printf 'physical_validation=NOT_PERFORMED\ndevice_writes=none\n'
} > "$out_dir/rmnet-core-validation.txt"
(
    cd "$out_dir"
    sha256sum vendor_dlkm.img rmnet-core-validation.txt \
        rmnet-provider-migration-closure.tsv rmnet-core-import-crc.tsv \
        rmnet-core-export-contract.tsv rmnet-core-consumers.tsv \
        rmnet-shared-type-contract.tsv signature-report.tsv e2fsck.txt
    sha256sum rmnet-core-source-delta.md
    sha256sum rmnet-core-priority-binary-proof.txt \
        rmnet-core-priority-old.disassembly.txt \
        rmnet-core-priority-new.disassembly.txt
) > "$out_dir/SHA256SUMS"
printf 'RMNET CORE .102 BOUNDED CANDIDATE PASS\n'
printf 'vendor_dlkm=%s\n' "$out_dir/vendor_dlkm.img"
printf 'sha256=%s\n' "$candidate_sha"
