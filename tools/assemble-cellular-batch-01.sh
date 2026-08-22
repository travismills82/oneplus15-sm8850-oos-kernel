#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Assemble a vendor-DLKM-only rmnet_sch source-ownership candidate.

set -euo pipefail

readonly release=6.12.23-android16-5-o-g6744a3f6bcf4-4k
readonly signer='OnePlus 15 Controlled OOS Module Signing v1'
readonly baseline_vendor_sha=3ed964f345e6f5040c70ef7c0c083c1fc4bab536b6a522ca83c61b20be032ed4

die() { printf 'error: %s\n' "$*" >&2; exit 2; }
sha256() { sha256sum "$1" | awk '{print $1}'; }

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
baseline_image=
baseline_stage=
replacement=
system_modules=
vendor_boot_modules=
module_symvers=
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
        --module-symvers) module_symvers=${2:-}; shift 2 ;;
        --system-load-contract) system_load_contract=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        *) die "unknown argument: $1" ;;
    esac
done
for value in baseline_image baseline_stage replacement system_modules \
             vendor_boot_modules module_symvers system_load_contract avbtool out_dir; do
    [[ -n ${!value} ]] || die "missing required --${value//_/-} argument"
done
for command in debugfs e2fsck jq modinfo python3 sha256sum; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done
for path in "$baseline_image" "$replacement" "$module_symvers" \
            "$system_load_contract" "$avbtool"; do
    [[ -f "$path" ]] || die "missing file: $path"
done
for path in "$baseline_stage" "$system_modules" "$vendor_boot_modules"; do
    [[ -d "$path" ]] || die "missing directory: $path"
done
[[ -x "$avbtool" && -x "$(dirname "$avbtool")/fec" ]] ||
    die "avbtool and companion fec must be executable"
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output: $out_dir"
[[ $(sha256 "$baseline_image") == "$baseline_vendor_sha" ]] ||
    die "baseline vendor-DLKM is not the physically qualified combined core image"
[[ $(modinfo -F name "$replacement") == rmnet_sch ]] || die "replacement is not rmnet_sch"
[[ $(modinfo -F vermagic "$replacement") == "$release "* ]] || die "replacement vermagic changed"
[[ $(modinfo -F signer "$replacement") == "$signer" ]] || die "replacement signer changed"

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

mkdir -p "$out_dir/staging"
cp -a "$baseline_stage/." "$out_dir/staging/"
stage_modules="$out_dir/staging/lib/modules"
stage_target=$(python3 - "$stage_modules" <<'PY'
import pathlib, subprocess, sys
matches=[]
for path in pathlib.Path(sys.argv[1]).rglob("*.ko"):
    name=subprocess.check_output(["modinfo","-F","name",str(path)],text=True).strip()
    if name == "rmnet_sch": matches.append(path)
if len(matches) != 1: raise SystemExit(f"expected one staged rmnet_sch, found {len(matches)}")
print(matches[0])
PY
)
cp "$replacement" "$stage_target"

contract="$out_dir/module-contract"
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
  .source_replacements == 1 and .protected_export_signed_closure == 1 and
  .re_sign_stock_modules == 0 and .retained_external_modules == 525 and
  .external_signed_provider_edges == 0 and .replacement_contract_failures == 0 and
  .unresolved_imports == 0 and .crc_mismatches == 0
' "$contract/summary.json" >/dev/null || die "Batch 01 module graph failed"

python3 - "$baseline_stage/lib/modules" "$stage_modules" \
    "$out_dir/batch01-replacement.tsv" "$out_dir/cellular-preservation.tsv" <<'PY'
import csv, hashlib, pathlib, subprocess, sys

cellular={"dwc3_msm","gsim","ipam","ipanetm","oplus_mm_kevent","oplus_mm_kevent_fb",
"qcom_glink","qcom_glink_smem","qcom_ramdump","qcom_smd","qcom_va_minidump",
"qmi_helpers","redriver","repeater","rmnet_aps","rmnet_core","rmnet_ctl","rmnet_mem",
"rmnet_offload","rmnet_perf","rmnet_perf_tether","rmnet_sch","rmnet_shs","rmnet_wlan",
"rproc_qcom_common","usb_f_gsi","wcd_usbss_i2c"}
def scan(root):
    result={}
    for path in pathlib.Path(root).rglob("*.ko"):
        name=subprocess.check_output(["modinfo","-F","name",str(path)],text=True).strip()
        if name in result: raise SystemExit(f"duplicate module {name}")
        result[name]=(path,hashlib.sha256(path.read_bytes()).hexdigest())
    return result
old,new=scan(sys.argv[1]),scan(sys.argv[2])
if len(old)!=436 or set(old)!=set(new): raise SystemExit("vendor inventory changed")
changed={name for name in old if old[name][1]!=new[name][1]}
if changed!={"rmnet_sch"}: raise SystemExit(f"unexpected vendor delta: {sorted(changed)}")
with pathlib.Path(sys.argv[3]).open("w",encoding="utf-8",newline="") as stream:
    w=csv.writer(stream,delimiter="\t",lineterminator="\n")
    w.writerow(["module","old_sha256","new_sha256","action","status"])
    w.writerow(["rmnet_sch",old["rmnet_sch"][1],new["rmnet_sch"][1],"SOURCE_REPLACEMENT_CURRENT_.097","PASS"])
with pathlib.Path(sys.argv[4]).open("w",encoding="utf-8",newline="") as stream:
    w=csv.writer(stream,delimiter="\t",lineterminator="\n")
    w.writerow(["module","old_sha256","new_sha256","action","status"])
    for name in sorted(cellular):
        action="SOURCE_REPLACEMENT" if name=="rmnet_sch" else "EXACT_STOCK_RETAINED"
        status="PASS" if (name=="rmnet_sch" or old[name][1]==new[name][1]) else "FAIL"
        w.writerow([name,old[name][1],new[name][1],action,status])
        if status!="PASS": raise SystemExit(f"cellular preservation failure: {name}")
PY

image_inode_metadata() {
    local image=$1 path=$2 data mode uid gid
    data=$(debugfs -R "stat $path" "$image" 2>/dev/null) || die "cannot stat $path"
    mode=$(sed -n 's/^.*Mode:  *\([0-7][0-7][0-7][0-7]\).*$/\1/p' <<<"$data")
    read -r uid gid < <(sed -n 's/^User: *\([0-9][0-9]*\).*Group: *\([0-9][0-9]*\).*$/\1 \2/p' <<<"$data")
    [[ "$mode" =~ ^[0-7]{4}$ && "$uid" =~ ^[0-9]+$ && "$gid" =~ ^[0-9]+$ ]] || die "bad inode metadata"
    printf '%s\t%s\t%s\n' "$mode" "$uid" "$gid"
}

image_selinux_xattr_hex() {
    local image=$1 path=$2 attrs value
    attrs=$(debugfs -R "ea_list $path" "$image" 2>/dev/null) || die "cannot list xattrs"
    [[ $(sed -n 's/^  \([^ ]*\) (.*/\1/p' <<<"$attrs") == security.selinux ]] || die "unexpected xattrs"
    value=$(debugfs -R "ea_get -x $path security.selinux" "$image" 2>/dev/null)
    value=$(sed -n 's/^security\.selinux ([0-9][0-9]*) = //p' <<<"$value" | tr -d ' ')
    [[ "$value" =~ ^([0-9A-Fa-f]{2})+$ ]] || die "invalid SELinux xattr"
    printf '%s\n' "$value"
}

# avbtool verifies the embedded partition-name descriptor by resolving the
# sibling name vendor_dlkm.img, so retain the canonical partition filename.
candidate="$out_dir/vendor_dlkm.img"
cp --reflink=auto "$baseline_image" "$candidate"
relative=${stage_target#"$out_dir/staging"}
IFS=$'\t' read -r mode uid gid < <(image_inode_metadata "$candidate" "$relative")
selinux=$(image_selinux_xattr_hex "$candidate" "$relative")
debugfs -w -R "rm $relative" "$candidate" >/dev/null 2>&1
debugfs -w -R "write $stage_target $relative" "$candidate" >/dev/null 2>&1
debugfs -w -R "set_inode_field $relative mode 0100${mode#0}" "$candidate" >/dev/null 2>&1
debugfs -w -R "set_inode_field $relative uid $uid" "$candidate" >/dev/null 2>&1
debugfs -w -R "set_inode_field $relative gid $gid" "$candidate" >/dev/null 2>&1
literal=$(python3 - "$selinux" <<'PY'
import sys
print(''.join(f'\\{byte:03o}' for byte in bytes.fromhex(sys.argv[1])))
PY
)
debugfs -w -R "ea_set $relative security.selinux \"$literal\"" "$candidate" >/dev/null 2>&1

avb_info=$($avbtool info_image --image "$baseline_image")
mapfile -t avb_fields < <(python3 - "$avb_info" <<'PY'
import re,sys
i=sys.argv[1]
def one(p):
    m=re.search(p,i,re.M)
    if not m: raise SystemExit(f"missing AVB field: {p}")
    return m.group(1)
print("partition_size\t"+one(r'^Image size:\s+(\d+) bytes$'))
print("original_image_size\t"+one(r'^Original image size:\s+(\d+) bytes$'))
print("partition_name\t"+one(r'^      Partition Name:\s+(.+)$'))
print("hash_algorithm\t"+one(r'^      Hash Algorithm:\s+(.+)$'))
print("salt\t"+one(r'^      Salt:\s+([0-9a-fA-F]+)$'))
print("block_size\t"+one(r'^      Data Block Size:\s+(\d+)(?: bytes)?$'))
print("fec_num_roots\t"+one(r'^      FEC num roots:\s+(\d+)$'))
print("rollback_index\t"+one(r'^Rollback Index:\s+(\d+)$'))
print("flags\t"+one(r'^Flags:\s+(\d+)$'))
print("rollback_index_location\t"+one(r'^Rollback Index Location:\s+(\d+)$'))
for k,v in re.findall(r"^    Prop: (.+?) -> '([^']*)'$",i,re.M): print(f"prop\t{k}\t{v}")
PY
)
declare -A avb_field=(); declare -a avb_props=()
for field in "${avb_fields[@]}"; do
    IFS=$'\t' read -r key value extra <<<"$field"
    if [[ $key == prop ]]; then avb_props+=("$value:$extra"); else avb_field[$key]=$value; fi
done
truncate -s "${avb_field[original_image_size]}" "$candidate"
avb_args=(add_hashtree_footer --image "$candidate" --partition_size "${avb_field[partition_size]}"
  --partition_name "${avb_field[partition_name]}" --hash_algorithm "${avb_field[hash_algorithm]}"
  --salt "${avb_field[salt]}" --block_size "${avb_field[block_size]}"
  --fec_num_roots "${avb_field[fec_num_roots]}" --algorithm NONE
  --rollback_index "${avb_field[rollback_index]}"
  --rollback_index_location "${avb_field[rollback_index_location]}" --flags "${avb_field[flags]}")
for prop in "${avb_props[@]}"; do avb_args+=(--prop "$prop"); done
PATH="$(dirname "$avbtool"):$PATH" "$avbtool" "${avb_args[@]}"
PATH="$(dirname "$avbtool"):$PATH" "$avbtool" verify_image --image "$candidate" >/dev/null || die "AVB verify failed"
e2fsck -fn "$candidate" >"$out_dir/e2fsck.txt" 2>&1 || die "ext4 validation failed"
debugfs -R "dump $relative $out_dir/rmnet_sch-readback.ko" "$candidate" >/dev/null 2>&1
[[ $(sha256 "$out_dir/rmnet_sch-readback.ko") == $(sha256 "$replacement") ]] || die "image read-back mismatch"

cp "$system_load_contract" "$out_dir/system-dlkm-load-contract.tsv"
sha256sum "$candidate" "$replacement" "$out_dir/rmnet_sch-readback.ko" > "$out_dir/SHA256SUMS"
{
    printf 'CELLULAR BATCH 01 STATIC CANDIDATE PASS\n'
    printf 'kernel_release=%s\n' "$release"
    printf 'vendor_modules=436\nchanged_modules=1\nchanged_module=rmnet_sch\n'
    printf 'retained_stock_cellular_modules=26\n'
    printf 'unresolved_imports=0\ncrc_mismatches=0\nprotected_export_failures=0\nsignature_failures=0\n'
    printf 'system_modules_load_entries=46\nwwan_load_order=21\n'
    printf 'vendor_dlkm_sha256=%s\n' "$(sha256 "$candidate")"
    printf 'physical_validation=NOT_PERFORMED\ndevice_writes=none\n'
} > "$out_dir/validation-report.txt"
printf 'CELLULAR BATCH 01 ASSEMBLY PASS\n'
printf 'vendor_dlkm=%s\n' "$candidate"
printf 'sha256=%s\n' "$(sha256 "$candidate")"
