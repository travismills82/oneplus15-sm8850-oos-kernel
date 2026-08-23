#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Assemble the combined BT .046 + NXP NFC .102 vendor-DLKM from exact,
# individually qualified module payloads. This helper never builds, signs,
# flashes, or changes boot/system-DLKM/vendor-boot/VBMeta.

set -euo pipefail

readonly release=6.12.23-android16-5-o-g6744a3f6bcf4-4k
readonly signer='OnePlus 15 Controlled OOS Module Signing v1'
readonly boot_sha=84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab
readonly system_sha=de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef
readonly baseline_vendor_sha=8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

usage() {
    sed -n '2,34p' "$0"
}

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
baseline_package=
baseline_vendor_stage=
bt_modules=
nfc_module=
system_modules=
vendor_boot_modules=
module_symvers=
avbtool=
out_dir=

while [[ $# -gt 0 ]]; do
    case "$1" in
        --baseline-package) baseline_package=${2:-}; shift 2 ;;
        --baseline-vendor-stage) baseline_vendor_stage=${2:-}; shift 2 ;;
        --bt-modules) bt_modules=${2:-}; shift 2 ;;
        --nfc-module) nfc_module=${2:-}; shift 2 ;;
        --system-modules) system_modules=${2:-}; shift 2 ;;
        --vendor-boot-modules) vendor_boot_modules=${2:-}; shift 2 ;;
        --module-symvers) module_symvers=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown argument: $1" ;;
    esac
done

for value in baseline_package baseline_vendor_stage bt_modules nfc_module \
             system_modules vendor_boot_modules module_symvers avbtool out_dir; do
    [[ -n ${!value} ]] || die "missing required --${value//_/-} argument"
done
for command in debugfs e2fsck jq modinfo openssl python3 sha256sum; do
    command -v "$command" >/dev/null || die "required command not found: $command"
done
for path in "$baseline_package" "$baseline_vendor_stage" "$bt_modules" \
            "$system_modules" "$vendor_boot_modules"; do
    [[ -d "$path" ]] || die "missing directory: $path"
done
for path in "$nfc_module" "$module_symvers" "$avbtool"; do
    [[ -f "$path" ]] || die "missing file: $path"
done
[[ -x "$avbtool" ]] || die "avbtool is not executable: $avbtool"
[[ -x "$(dirname "$avbtool")/fec" ]] || die "avbtool companion fec is missing"
[[ ! -e "$out_dir" ]] || die "refusing to overwrite output: $out_dir"

sha256() {
    sha256sum "$1" | awk '{print $1}'
}

require_hash() {
    local path=$1 expected=$2 label=$3 actual
    [[ -f "$path" ]] || die "missing $label: $path"
    actual=$(sha256 "$path")
    [[ "$actual" == "$expected" ]] || die "$label hash changed: $actual"
}

require_hash "$baseline_package/boot.img" "$boot_sha" qualified_boot
require_hash "$baseline_package/system_dlkm.img" "$system_sha" qualified_system_dlkm
require_hash "$baseline_package/vendor_dlkm.img" "$baseline_vendor_sha" qualified_vendor_dlkm

declare -A expected_module_sha=(
    [btpower]=f21baebef606e2d076827cbd87a1bcde0adfac9e785dffc9ac86a0d194c0e09f
    [bt_fm_swr]=9408e38f2d61fc97e4610a4b97ce1d9814097a385187bd205983062c37d48f21
    [btfm_slim_codec]=dde56a787da9e7925bb1ca08ffadaf837654675e3d9fef9d4b560bfae00131fc
    [nxp_nci]=51ef28ae123a7b2c0fd851491e1a13abfbd19b3b4a9a66acf3e4b997096ca9c2
)
declare -A replacement_path=(
    [btpower]="$bt_modules/btpower.ko"
    [bt_fm_swr]="$bt_modules/bt_fm_swr.ko"
    [btfm_slim_codec]="$bt_modules/btfm_slim_codec.ko"
    [nxp_nci]="$nfc_module"
)
for module in btpower bt_fm_swr btfm_slim_codec nxp_nci; do
    path=${replacement_path[$module]}
    require_hash "$path" "${expected_module_sha[$module]}" "$module"
    [[ $(modinfo -F name "$path") == "$module" ]] || die "$path has wrong module name"
    [[ $(modinfo -F vermagic "$path") == "$release "* ]] || die "$module vermagic changed"
    [[ $(modinfo -F signer "$path") == "$signer" ]] || die "$module signer changed"
done

load_contract="$baseline_package/system-dlkm-load-contract.tsv"
[[ -f "$load_contract" ]] || die "missing system-DLKM load contract"
python3 - "$load_contract" <<'PY'
import csv
import pathlib
import sys

with pathlib.Path(sys.argv[1]).open(encoding="utf-8", newline="") as stream:
    rows = [row for row in csv.DictReader(stream, delimiter="\t")
            if row.get("partition") == "system_dlkm"]
if len(rows) != 46:
    raise SystemExit(f"system modules.load count changed: {len(rows)}")
if any(row.get("status") != "PASS" for row in rows):
    raise SystemExit("system modules.load contract contains a failure")
if any(row.get("stale_builtin_entry") != "no" for row in rows):
    raise SystemExit("system modules.load contains a stale built-in entry")
wwan = [row for row in rows if row.get("module") == "wwan.ko"]
if len(wwan) != 1 or wwan[0].get("load_order") != "21":
    raise SystemExit("wwan.ko is not system modules.load entry 21")
PY

mkdir -p "$out_dir/staging"
cp -a "$baseline_vendor_stage/." "$out_dir/staging/"
stage_modules="$out_dir/staging/lib/modules"

python3 - "$stage_modules" \
    "${replacement_path[btpower]}" "${replacement_path[bt_fm_swr]}" \
    "${replacement_path[btfm_slim_codec]}" "${replacement_path[nxp_nci]}" <<'PY'
import pathlib
import shutil
import subprocess
import sys

root = pathlib.Path(sys.argv[1])
targets = {}
for path in root.rglob("*.ko"):
    name = subprocess.check_output(["modinfo", "-F", "name", str(path)], text=True).strip()
    if name in targets:
        raise SystemExit(f"duplicate staged module name: {name}")
    targets[name] = path
for source_text in sys.argv[2:]:
    source = pathlib.Path(source_text)
    name = subprocess.check_output(["modinfo", "-F", "name", str(source)], text=True).strip()
    target = targets.get(name)
    if target is None:
        raise SystemExit(f"staged vendor tree has no {name}")
    shutil.copyfile(source, target)
    shutil.copymode(source, target)
PY

prestage="$out_dir/prestage-contract"
final="$out_dir/final-contract"
python3 "$repo_root/tools/validate-matched-wlan-vendor-dlkm.py" \
    --stock-vendor-root "$baseline_vendor_stage/lib/modules" \
    --replacement "${replacement_path[btpower]}" \
    --replacement "${replacement_path[bt_fm_swr]}" \
    --replacement "${replacement_path[btfm_slim_codec]}" \
    --replacement "${replacement_path[nxp_nci]}" \
    --external-root "$system_modules" \
    --external-root "$vendor_boot_modules" \
    --vmlinux-symvers "$module_symvers" \
    --out-dir "$prestage" \
    --expected-stock-module-count 436 \
    --allow-import-contract-change bt_fm_swr \
    --fail-external-signed-provider-edges

python3 "$repo_root/tools/validate-wlan053-final-vendor-tree.py" \
    --stock-root "$baseline_vendor_stage/lib/modules" \
    --candidate-root "$stage_modules" \
    --external-root "$system_modules" \
    --external-root "$vendor_boot_modules" \
    --vmlinux-symvers "$module_symvers" \
    --expected-release "$release" \
    --expected-signer "$signer" \
    --source-replacement btpower \
    --source-replacement bt_fm_swr \
    --source-replacement btfm_slim_codec \
    --source-replacement nxp_nci \
    --out-dir "$final" \
    --expected-module-count 436

jq -e '
  .result == "PASS" and
  .stock_vendor_modules == 436 and
  .source_replacements == 4 and
  .protected_export_signed_closure == 4 and
  .re_sign_stock_modules == 0 and
  .retained_external_modules == 525 and
  .external_signed_provider_edges == 0 and
  .replacement_contract_failures == 0 and
  .unresolved_imports == 0 and
  .crc_mismatches == 0
' "$prestage/summary.json" >/dev/null || die "combined prestage contract failed"
jq -e '
  .result == "PASS" and .module_count == 436 and
  .changed_modules == ["bt_fm_swr","btfm_slim_codec","btpower","nxp_nci"] and
  .exact_stock_cellular_modules == 27 and .cellular_hash_failures == 0 and
  .unresolved_imports == 0 and .crc_mismatches == 0 and
  .signature_failures == 0 and .vermagic_failures == 0
' "$final/summary.json" >/dev/null || die "combined final module contract failed"

image_inode_metadata() {
    local image=$1 path=$2 data mode uid gid
    data=$(debugfs -R "stat $path" "$image" 2>/dev/null) || die "cannot stat $path"
    mode=$(sed -n 's/^.*Mode:  *\([0-7][0-7][0-7][0-7]\).*$/\1/p' <<<"$data")
    read -r uid gid < <(sed -n 's/^User: *\([0-9][0-9]*\).*Group: *\([0-9][0-9]*\).*$/\1 \2/p' <<<"$data")
    [[ "$mode" =~ ^[0-7]{4}$ && "$uid" =~ ^[0-9]+$ && "$gid" =~ ^[0-9]+$ ]] ||
        die "cannot parse inode metadata for $path"
    printf '%s\t%s\t%s\n' "$mode" "$uid" "$gid"
}

image_selinux_xattr_hex() {
    local image=$1 path=$2 attrs value
    attrs=$(debugfs -R "ea_list $path" "$image" 2>/dev/null) || die "cannot list xattrs for $path"
    [[ $(sed -n 's/^  \([^ ]*\) (.*/\1/p' <<<"$attrs" | wc -l) -eq 1 ]] ||
        die "unexpected xattr set for $path"
    [[ $(sed -n 's/^  \([^ ]*\) (.*/\1/p' <<<"$attrs") == security.selinux ]] ||
        die "security.selinux is missing for $path"
    value=$(debugfs -R "ea_get -x $path security.selinux" "$image" 2>/dev/null)
    value=$(sed -n 's/^security\.selinux ([0-9][0-9]*) = //p' <<<"$value" | tr -d ' ')
    [[ "$value" =~ ^([0-9A-Fa-f]{2})+$ ]] || die "invalid security.selinux xattr for $path"
    printf '%s\n' "$value"
}

debugfs_octal_literal() {
    python3 - "$1" <<'PY'
import sys
print(''.join(f'\\{byte:03o}' for byte in bytes.fromhex(sys.argv[1])))
PY
}

restore_image_metadata() {
    local image=$1 path=$2 mode=$3 uid=$4 gid=$5 selinux_hex=$6 literal
    debugfs -w -R "set_inode_field $path mode 0100${mode#0}" "$image" >/dev/null 2>&1 ||
        die "cannot restore mode for $path"
    debugfs -w -R "set_inode_field $path uid $uid" "$image" >/dev/null 2>&1 ||
        die "cannot restore uid for $path"
    debugfs -w -R "set_inode_field $path gid $gid" "$image" >/dev/null 2>&1 ||
        die "cannot restore gid for $path"
    literal=$(debugfs_octal_literal "$selinux_hex")
    debugfs -w -R "ea_set $path security.selinux \"$literal\"" "$image" >/dev/null 2>&1 ||
        die "cannot restore security.selinux for $path"
}

baseline_image="$baseline_package/vendor_dlkm.img"
candidate_image="$out_dir/vendor_dlkm.img"
cp --reflink=auto "$baseline_image" "$candidate_image"

avb_info=$($avbtool info_image --image "$baseline_image")
mapfile -t avb_fields < <(python3 - "$avb_info" <<'PY'
import re
import sys
info = sys.argv[1]
def one(pattern, label):
    match = re.search(pattern, info, re.M)
    if not match:
        raise SystemExit(f"missing {label} in baseline AVB footer")
    return match.group(1)
print(f"partition_size\t{one(r'^Image size:\s+(\d+) bytes$', 'image size')}")
print(f"original_image_size\t{one(r'^Original image size:\s+(\d+) bytes$', 'original image size')}")
print(f"partition_name\t{one(r'^      Partition Name:\s+(.+)$', 'partition name')}")
print(f"hash_algorithm\t{one(r'^      Hash Algorithm:\s+(.+)$', 'hash algorithm')}")
print(f"salt\t{one(r'^      Salt:\s+([0-9a-fA-F]+)$', 'salt')}")
print(f"block_size\t{one(r'^      Data Block Size:\s+(\d+)(?: bytes)?$', 'block size')}")
print(f"fec_num_roots\t{one(r'^      FEC num roots:\s+(\d+)$', 'FEC roots')}")
print(f"rollback_index\t{one(r'^Rollback Index:\s+(\d+)$', 'rollback index')}")
print(f"flags\t{one(r'^Flags:\s+(\d+)$', 'flags')}")
print(f"rollback_index_location\t{one(r'^Rollback Index Location:\s+(\d+)$', 'rollback location')}")
for key, value in re.findall(r"^    Prop: (.+?) -> '([^']*)'$", info, re.M):
    print(f"prop\t{key}\t{value}")
PY
)
declare -A avb_field=()
declare -a avb_props=()
for field in "${avb_fields[@]}"; do
    IFS=$'\t' read -r key value extra <<<"$field"
    if [[ $key == prop ]]; then avb_props+=("$value:$extra"); else avb_field[$key]=$value; fi
done

declare -A stage_path_by_name=()
while IFS= read -r -d '' module_path; do
    name=$(modinfo -F name "$module_path")
    stage_path_by_name[$name]=$module_path
done < <(find "$stage_modules" -type f -name '*.ko' -print0 | sort -z)

for module in btpower bt_fm_swr btfm_slim_codec nxp_nci; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    IFS=$'\t' read -r mode uid gid < <(image_inode_metadata "$candidate_image" "$relative")
    selinux=$(image_selinux_xattr_hex "$candidate_image" "$relative")
    debugfs -w -R "rm $relative" "$candidate_image" >/dev/null 2>&1
    debugfs -w -R "write $staged $relative" "$candidate_image" >/dev/null 2>&1
    restore_image_metadata "$candidate_image" "$relative" "$mode" "$uid" "$gid" "$selinux"
done

truncate -s "${avb_field[original_image_size]}" "$candidate_image"
avb_args=(
    add_hashtree_footer --image "$candidate_image"
    --partition_size "${avb_field[partition_size]}"
    --partition_name "${avb_field[partition_name]}"
    --hash_algorithm "${avb_field[hash_algorithm]}"
    --salt "${avb_field[salt]}" --block_size "${avb_field[block_size]}"
    --fec_num_roots "${avb_field[fec_num_roots]}" --algorithm NONE
    --rollback_index "${avb_field[rollback_index]}"
    --rollback_index_location "${avb_field[rollback_index_location]}"
    --flags "${avb_field[flags]}"
)
for prop in "${avb_props[@]}"; do avb_args+=(--prop "$prop"); done
PATH="$(dirname "$avbtool"):$PATH" "$avbtool" "${avb_args[@]}"
[[ $(stat -c '%s' "$candidate_image") == "${avb_field[partition_size]}" ]] ||
    die "combined image size changed"
PATH="$(dirname "$avbtool"):$PATH" "$avbtool" verify_image --image "$candidate_image" >/dev/null ||
    die "combined AVB footer verification failed"
e2fsck -fn "$candidate_image" >"$out_dir/e2fsck.txt" 2>&1 || {
    sed -n '1,200p' "$out_dir/e2fsck.txt" >&2
    die "combined ext4 validation failed"
}

mkdir -p "$out_dir/readback"
for module in btpower bt_fm_swr btfm_slim_codec nxp_nci; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    readback="$out_dir/readback/$module.ko"
    debugfs -R "dump $relative $readback" "$candidate_image" >/dev/null 2>&1
    [[ $(sha256 "$readback") == "${expected_module_sha[$module]}" ]] ||
        die "$module image read-back differs from qualified module"
done

python3 - "$baseline_vendor_stage/lib/modules" "$stage_modules" \
    "$out_dir/combined-replacement.tsv" <<'PY'
import csv
import hashlib
import pathlib
import subprocess
import sys

def modules(root):
    result = {}
    for path in pathlib.Path(root).rglob("*.ko"):
        name = subprocess.check_output(["modinfo", "-F", "name", str(path)], text=True).strip()
        result[name] = (path, hashlib.sha256(path.read_bytes()).hexdigest())
    return result

old, new = modules(sys.argv[1]), modules(sys.argv[2])
wanted = {"btpower": (".031", ".046"), "bt_fm_swr": (".031", ".046"),
          "btfm_slim_codec": (".031", ".046"), "nxp_nci": (".089", ".102")}
changed = {name for name in old if old[name][1] != new[name][1]}
if changed != set(wanted):
    raise SystemExit(f"unexpected combined delta: {sorted(changed)}")
with pathlib.Path(sys.argv[3]).open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(["module", "old_generation", "new_generation", "old_sha256",
                     "new_sha256", "status"])
    for name in sorted(wanted):
        writer.writerow([name, *wanted[name], old[name][1], new[name][1], "PASS"])
PY

cp "$load_contract" "$out_dir/system-dlkm-load-contract.tsv"
cp "$final/vendor-dlkm-module-contract.tsv" "$out_dir/vendor-dlkm-module-contract.tsv"
cp "$final/vendor-dlkm-import-contract.tsv" "$out_dir/vendor-dlkm-import-contract.tsv"
cp "$final/cellular-exact-stock-final.tsv" "$out_dir/cellular-exact-stock-final.tsv"

candidate_sha=$(sha256 "$candidate_image")
python3 - "$out_dir/manifest.json" "$candidate_sha" "${avb_field[partition_size]}" \
    "$(git -C "$repo_root" rev-parse HEAD)" <<PY
import json
import pathlib
import sys
manifest = {
  "schema_version": 2,
  "generation": "controlled-v1-wlan053-bt046-nfc102-core-candidate",
  "repository_commit": sys.argv[4],
  "kernel_contract": {
    "release": "$release",
    "boot_sha256": "$boot_sha",
    "system_dlkm_sha256": "$system_sha",
    "changed": False,
  },
  "subsystems": {
    "wlan": {"generation": ".053", "qualification": "PHYSICALLY_QUALIFIED"},
    "bluetooth_vendor": {"generation": ".046", "qualification": "CORE_PASS_OPTIONAL_EQUIPMENT_PENDING"},
    "nfc_vendor": {"generation": ".102", "qualification": "CORE_PASS_TAG_PAYMENT_PENDING"},
    "cellular": {"generation": "stock OOS 16.0.9.400(EX01)", "exact_modules": 27},
  },
  "signing": {"generation": "controlled-v1", "signer": "$signer"},
  "payloads": {
    "vendor_dlkm.img": {"sha256": sys.argv[2], "size": int(sys.argv[3])}
  },
  "vendor_module_count": 436,
  "intended_replacements": ["bt_fm_swr", "btfm_slim_codec", "btpower", "nxp_nci"],
  "system_modules_load_entries": 46,
  "wwan_load_order": 21,
  "write_scope": ["vendor_dlkm"],
  "physical_validation": "NOT_PERFORMED_COMBINED",
}
pathlib.Path(sys.argv[1]).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                                    encoding="utf-8")
PY

cat >"$out_dir/validation-report.txt" <<EOF
CONTROLLED-V1 WLAN053 + BT046 + NFC102 COMBINED STATIC CANDIDATE

kernel_release=$release
boot_changed=no
system_dlkm_changed=no
vendor_dlkm_changed=yes
vendor_modules=436
intended_replacements=4
unresolved_imports=0
crc_mismatches=0
protected_export_failures=0
signature_failures=0
duplicate_module_names=0
exact_stock_cellular_modules=27
system_modules_load_entries=46
wwan_load_order=21
ext4=PASS
partition_local_AVB=PASS
physical_validation=NOT_PERFORMED_COMBINED
EOF

find "$out_dir" -type f \( -name '*.pem' -o -name '*.key' -o -name '*.p12' -o -name '*.pfx' \) -print -quit |
    grep -q . && die "private signing material entered output"
find "$out_dir" -type f -printf '%P\n' | LC_ALL=C sort | while IFS= read -r path; do
    [[ $path == SHA256SUMS ]] || printf '%s  %s\n' "$(sha256 "$out_dir/$path")" "$path"
done >"$out_dir/SHA256SUMS"

printf 'COMBINED CONTROLLED-V1 VENDOR-DLKM PASS\n'
printf 'image=%s\n' "$candidate_image"
printf 'sha256=%s\n' "$candidate_sha"
printf 'device_writes=none\n'
