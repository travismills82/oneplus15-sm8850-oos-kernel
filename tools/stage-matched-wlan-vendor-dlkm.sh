#!/usr/bin/env bash
# Stage and sign a narrow source-built WLAN vendor-DLKM replacement closure.
#
# Inputs are read-only. The script copies the complete stock vendor_dlkm tree,
# replaces only validator-selected modules, strips debug information, then signs
# the complete protected-export closure with the key embedded in the matching
# Image. It intentionally retains all other stock modules byte-for-byte.
#
# The modified ext4 payload cannot retain the stock AVB hashtree/FEC/footer:
# doing so leaves an internally inconsistent image that can fail dm-verity at
# mount time.  This script therefore regenerates the *partition-local*,
# unsigned vendor_dlkm AVB footer from the stock footer's geometry and metadata.
# It deliberately does not update any parent/top-level vbmeta descriptor; that
# remains a separate bootloader/AVB decision for a development device.

set -euo pipefail

usage() {
    cat <<'USAGE'
Usage:
  tools/stage-matched-wlan-vendor-dlkm.sh \
      --stock-vendor-root <extracted-stock-vendor-dlkm> \
      --stock-vendor-image <raw-stock-vendor_dlkm.img> \
      --kernel-build-dir <matching-kernel_aarch64-output> \
      --signing-key <declared-controlled-signing-private-pem> \
      --system-dlkm-staging-archive <matching-system_dlkm_staging_archive.tar.gz> \
      --avbtool <path-to-avbtool> \
      --validation-dir <validator-output> \
      --out-dir <new-empty-output-directory> \
      --replacement <source-module.ko> [--replacement <source-module.ko> ...]

The validator output must PASS with the exact source modules supplied here.
The system-DLKM staging archive must come from the same build.  Its flat
module set and modules.builtin file are used to reconcile vendor modules.dep:
dependencies on a provider compiled into vmlinux are removed, while every
other /system/lib/modules dependency must still exist in the supplied system
image.
The script regenerates the partition-local unsigned AVB hashtree/FEC/footer
using the stock footer's geometry. It does not update parent/top-level vbmeta
metadata and is intentionally not a flasher.
USAGE
}

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

require_file() { [[ -f "$1" ]] || die "missing file: $1"; }
require_dir() { [[ -d "$1" ]] || die "missing directory: $1"; }

module_name() {
    local name
    name=$(modinfo -F name "$1")
    [[ -n "$name" ]] || die "could not read MODULE_NAME from $1"
    printf '%s\n' "$name"
}

has_signature() { [[ -n "$(modinfo -F signer "$1")" ]]; }

# debugfs write creates a fresh inode.  That is not equivalent to replacing a
# file through the mounted Android filesystem: it loses the original mode and
# security.selinux xattr.  vendor_modprobe is confined to vendor_file, so an
# unlabeled replacement can be present and signed yet remain unloadable.
image_inode_metadata() {
    local image=$1 path=$2 stat mode uid gid

    stat=$(debugfs -R "stat $path" "$image" 2>/dev/null) || {
        die "could not inspect inode metadata for $path"
    }
    mode=$(sed -n 's/^.*Mode:  *\([0-7][0-7][0-7][0-7]\).*$/\1/p' <<<"$stat")
    read -r uid gid < <(sed -n 's/^User: *\([0-9][0-9]*\).*Group: *\([0-9][0-9]*\).*$/\1 \2/p' <<<"$stat")
    [[ "$mode" =~ ^[0-7]{4}$ && "$uid" =~ ^[0-9]+$ && "$gid" =~ ^[0-9]+$ ]] || {
        die "could not parse inode metadata for $path"
    }
    printf '%s\t%s\t%s\n' "$mode" "$uid" "$gid"
}

image_selinux_xattr_hex() {
    local image=$1 path=$2 attrs value

    attrs=$(debugfs -R "ea_list $path" "$image" 2>/dev/null) || {
        die "could not inspect extended attributes for $path"
    }
    # Refuse to silently drop an attribute the stager does not know how to
    # reproduce.  Current stock vendor modules have exactly this xattr.
    [[ $(sed -n 's/^  \([^ ]*\) (.*/\1/p' <<<"$attrs" | wc -l) -eq 1 ]] || {
        die "unexpected extended-attribute set for $path"
    }
    [[ $(sed -n 's/^  \([^ ]*\) (.*/\1/p' <<<"$attrs") == security.selinux ]] || {
        die "security.selinux is missing for $path"
    }
    value=$(debugfs -R "ea_get -x $path security.selinux" "$image" 2>/dev/null) || {
        die "could not read security.selinux for $path"
    }
    value=$(sed -n 's/^security\.selinux ([0-9][0-9]*) = //p' <<<"$value" | tr -d ' ')
    [[ "$value" =~ ^([0-9A-Fa-f]{2})+$ ]] || die "invalid security.selinux value for $path"
    printf '%s\n' "$value"
}

debugfs_octal_literal() {
    python3 - "$1" <<'PY'
import sys

raw = bytes.fromhex(sys.argv[1])
print(''.join(f'\\{byte:03o}' for byte in raw))
PY
}

restore_image_metadata() {
    local image=$1 path=$2 mode=$3 uid=$4 gid=$5 selinux_hex=$6 selinux_literal

    debugfs -w -R "set_inode_field $path mode 0100${mode#0}" "$image" >/dev/null 2>&1 || {
        die "could not restore mode for $path"
    }
    debugfs -w -R "set_inode_field $path uid $uid" "$image" >/dev/null 2>&1 || {
        die "could not restore uid for $path"
    }
    debugfs -w -R "set_inode_field $path gid $gid" "$image" >/dev/null 2>&1 || {
        die "could not restore gid for $path"
    }
    selinux_literal=$(debugfs_octal_literal "$selinux_hex")
    debugfs -w -R "ea_set $path security.selinux \"$selinux_literal\"" "$image" >/dev/null 2>&1 || {
        die "could not restore security.selinux for $path"
    }
}

stock_root=
stock_image=
kernel_build_dir=
signing_key=
system_dlkm_staging_archive=
avbtool=
validation_dir=
out_dir=
declare -a replacements=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stock-vendor-root) stock_root=${2:-}; shift 2 ;;
        --stock-vendor-image) stock_image=${2:-}; shift 2 ;;
        --kernel-build-dir) kernel_build_dir=${2:-}; shift 2 ;;
        --signing-key) signing_key=${2:-}; shift 2 ;;
        --system-dlkm-staging-archive) system_dlkm_staging_archive=${2:-}; shift 2 ;;
        --avbtool) avbtool=${2:-}; shift 2 ;;
        --validation-dir) validation_dir=${2:-}; shift 2 ;;
        --out-dir) out_dir=${2:-}; shift 2 ;;
        --replacement) replacements+=("${2:-}"); shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown argument: $1" ;;
    esac
done

[[ -n "$stock_root" && -n "$stock_image" && -n "$kernel_build_dir" && -n "$signing_key" &&
   -n "$system_dlkm_staging_archive" && -n "$avbtool" ]] || {
    usage >&2
    die "stock root, stock image, kernel build directory, signing key, system-DLKM archive, and avbtool are required"
}
[[ -n "$validation_dir" && -n "$out_dir" ]] || {
    usage >&2
    die "validation and output directories are required"
}
[[ ${#replacements[@]} -gt 0 ]] || die "at least one source replacement is required"

require_dir "$stock_root"
require_file "$stock_image"
require_dir "$kernel_build_dir"
require_file "$signing_key"
require_file "$system_dlkm_staging_archive"
require_dir "$validation_dir"
require_file "$validation_dir/summary.json"
require_file "$validation_dir/protected-export-signing-closure.tsv"
require_file "$kernel_build_dir/scripts/sign-file"
require_file "$kernel_build_dir/certs/signing_key.x509"
require_file "$kernel_build_dir/vmlinux"
require_file "$kernel_build_dir/include/config/kernel.release"
require_file "$kernel_build_dir/modules.builtin"
require_file "$avbtool"
avbtool_dir=$(dirname "$avbtool")
[[ -x "$avbtool_dir/fec" ]] || die "the avbtool companion fec binary is required"
PATH="$avbtool_dir:$PATH"
export PATH
command -v debugfs >/dev/null || die "debugfs is required"
command -v e2fsck >/dev/null || die "e2fsck is required"
command -v modinfo >/dev/null || die "modinfo is required"
[[ ! -e "$out_dir" ]] || die "output path already exists: $out_dir"

read_avb_footer() {
    local image=$1
    "$avbtool" info_image --image "$image"
}

avb_info=$(read_avb_footer "$stock_image")
mapfile -t avb_fields < <(python3 - "$avb_info" <<'PY'
import re
import sys

info = sys.argv[1]

def one(pattern, label):
    match = re.search(pattern, info, re.M)
    if not match:
        raise SystemExit(f"missing {label} in stock AVB footer")
    return match.group(1)

print(f"partition_size\t{one(r'^Image size:\s+(\d+) bytes$', 'image size')}")
print(f"original_image_size\t{one(r'^Original image size:\s+(\d+) bytes$', 'original image size')}")
print(f"partition_name\t{one(r'^      Partition Name:\s+(.+)$', 'partition name')}")
print(f"hash_algorithm\t{one(r'^      Hash Algorithm:\s+(.+)$', 'hash algorithm')}")
print(f"salt\t{one(r'^      Salt:\s+([0-9a-fA-F]+)$', 'salt')}")
print(f"block_size\t{one(r'^      Data Block Size:\s+(\d+)(?: bytes)?$', 'data block size')}")
print(f"fec_num_roots\t{one(r'^      FEC num roots:\s+(\d+)$', 'FEC roots')}")
print(f"rollback_index\t{one(r'^Rollback Index:\s+(\d+)$', 'rollback index')}")
print(f"flags\t{one(r'^Flags:\s+(\d+)$', 'flags')}")
print(f"rollback_index_location\t{one(r'^Rollback Index Location:\s+(\d+)$', 'rollback index location')}")
for key, value in re.findall(r"^    Prop: (.+?) -> '([^']*)'$", info, re.M):
    print(f"prop\t{key}\t{value}")
PY
)

declare -A avb_field=()
declare -a avb_props=()
for field in "${avb_fields[@]}"; do
    IFS=$'\t' read -r key value extra <<< "$field"
    if [[ "$key" == prop ]]; then
        avb_props+=("$value:$extra")
    else
        avb_field[$key]=$value
    fi
done
for key in partition_size original_image_size partition_name hash_algorithm salt block_size fec_num_roots rollback_index flags rollback_index_location; do
    [[ -n ${avb_field[$key]:-} ]] || die "stock AVB footer did not provide $key"
done

python3 - "$validation_dir/summary.json" <<'PY'
import json
import pathlib
import sys

summary = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
if summary.get("result") != "PASS":
    raise SystemExit("validator result is not PASS")
if summary.get("replacement_contract_failures") != 0:
    raise SystemExit("validator reported replacement-contract failures")
if summary.get("unresolved_imports") != 0 or summary.get("crc_mismatches") != 0:
    raise SystemExit("validator reported unresolved imports or CRC mismatches")
PY

strip_tool=$(command -v llvm-objcopy || true)
[[ -n "$strip_tool" && -x "$strip_tool" ]] || die "llvm-objcopy is required"

python3 - "$kernel_build_dir/certs/signing_key.x509" "$kernel_build_dir/vmlinux" <<'PY'
from pathlib import Path
import sys

certificate = Path(sys.argv[1]).read_bytes()
vmlinux = Path(sys.argv[2]).read_bytes()
if certificate not in vmlinux:
    raise SystemExit("the module-signing certificate is absent from the matching vmlinux")
PY

kernel_release=$(<"$kernel_build_dir/include/config/kernel.release")
[[ -n "$kernel_release" ]] || die "matching kernel release is empty"

# The staging key is an output of the exact Image build, not a global or
# historical default.  Derive the signing identity from that public output so
# the protected-export closure cannot silently be signed with an unrelated
# autogenerated key.  Kbuild records the X.509 common name in modinfo's
# signer field.
kernel_signing_certificate_der_sha256=$(sha256sum "$kernel_build_dir/certs/signing_key.x509" | awk '{print $1}')
kernel_signing_subject=$(openssl x509 -inform DER \
    -in "$kernel_build_dir/certs/signing_key.x509" -noout -subject -nameopt compat |
    sed -n 's#^subject=/CN=\([^/]*\).*#\1#p')
[[ -n "$kernel_signing_subject" ]] || {
    die "could not derive the module-signing certificate common name"
}
signing_key_public=$(openssl pkey -in "$signing_key" -pubout 2>/dev/null |
    openssl pkey -pubin -outform DER 2>/dev/null | sha256sum | awk '{print $1}') || {
    die "could not read the declared controlled signing key"
}
signing_cert_public=$(openssl x509 -inform DER -in "$kernel_build_dir/certs/signing_key.x509" \
    -pubkey -noout 2>/dev/null |
    openssl pkey -pubin -outform DER 2>/dev/null | sha256sum | awk '{print $1}') || {
    die "could not read the matching Image signing certificate"
}
[[ "$signing_key_public" == "$signing_cert_public" ]] || {
    die "declared signing key does not match the certificate embedded in vmlinux"
}

# Do not use the stock system-DLKM inventory to interpret cross-partition
# dependencies.  The candidate Image has deliberately moved some providers
# (for example rfkill) into vmlinux, so a stock modules.dep path can name a
# file that no longer exists in the matching system_dlkm image.  The archive
# is a build output, not an input image to mutate.
system_dlkm_reference="$out_dir/system-dlkm-reference"
mkdir -p "$system_dlkm_reference"
tar -xzf "$system_dlkm_staging_archive" -C "$system_dlkm_reference"
system_modules_root="$system_dlkm_reference/flatten/lib/modules"
system_modules_builtin="$system_dlkm_reference/lib/modules/$kernel_release/modules.builtin"
require_dir "$system_modules_root"
require_file "$system_modules_builtin"
cmp -s "$system_modules_builtin" "$kernel_build_dir/modules.builtin" || {
    die "system-DLKM archive modules.builtin does not match the kernel build"
}

declare -A source_by_name=()
for replacement in "${replacements[@]}"; do
    require_file "$replacement"
    name=$(module_name "$replacement")
    [[ -z ${source_by_name[$name]+x} ]] || die "duplicate source replacement for $name"
    has_signature "$replacement" && die "replacement is already signed: $replacement"
    vermagic=$(modinfo -F vermagic "$replacement")
    [[ "$vermagic" == "$kernel_release "* ]] || {
        die "replacement $name does not match kernel release $kernel_release: ${vermagic:-<none>}"
    }
    source_by_name[$name]=$replacement
done

declare -A closure_action=()
while IFS=$'\t' read -r module action _first_import _provider; do
    [[ "$module" == "module" ]] && continue
    [[ -n "$module" && -n "$action" ]] || die "malformed signing closure row"
    closure_action[$module]=$action
done < "$validation_dir/protected-export-signing-closure.tsv"

mapfile -t source_modules < <(printf '%s\n' "${!source_by_name[@]}" | LC_ALL=C sort)
mapfile -t closure_modules < <(printf '%s\n' "${!closure_action[@]}" | LC_ALL=C sort)

for module in "${source_modules[@]}"; do
    [[ ${closure_action[$module]:-} == "SOURCE_REPLACEMENT" ]] || {
        die "validator closure does not select $module as a source replacement"
    }
done
for module in "${closure_modules[@]}"; do
    case "${closure_action[$module]}" in
        SOURCE_REPLACEMENT)
            [[ -n ${source_by_name[$module]+x} ]] || {
                die "missing source replacement required by closure: $module"
            }
            ;;
        RE_SIGN_STOCK) ;;
        *) die "unknown closure action for $module: ${closure_action[$module]}" ;;
    esac
done

mkdir -p "$out_dir/staging"
cp -a "$stock_root/." "$out_dir/staging/"
stage_modules="$out_dir/staging/lib/modules"
require_dir "$stage_modules"

declare -A stage_path_by_name=()
while IFS= read -r -d '' module_path; do
    name=$(module_name "$module_path")
    [[ -z ${stage_path_by_name[$name]+x} ]] || {
        die "duplicate module name in staged stock tree: $name"
    }
    stage_path_by_name[$name]=$module_path
done < <(find "$stage_modules" -type f -name '*.ko' -print0 | sort -z)

# Android's flattened vendor module loader consumes this text file directly.
# Keep all stock ordering and vendor dependencies, but make it describe the
# candidate system-DLKM/Image boundary exactly.  A missing non-built-in system
# provider is a hard error: silently retaining it would create a boot-time
# modprobe failure, and silently dropping it would hide a real dependency.
modules_dep_reconciliation="$out_dir/vendor-system-dependency-reconciliation.tsv"
python3 - "$stage_modules/modules.dep" "$system_modules_root" \
    "$system_modules_builtin" "$modules_dep_reconciliation" <<'PY'
from pathlib import Path, PurePosixPath
import os
import sys

dep_path = Path(sys.argv[1])
system_modules_root = Path(sys.argv[2])
builtin_path = Path(sys.argv[3])
report_path = Path(sys.argv[4])

if not dep_path.is_file():
    raise SystemExit(f"missing staged modules.dep: {dep_path}")

builtin_names = {
    PurePosixPath(line.strip()).name
    for line in builtin_path.read_text(encoding="utf-8").splitlines()
    if line.strip().endswith(".ko")
}
if not builtin_names:
    raise SystemExit(f"no built-in modules found in {builtin_path}")

system_module_names = {
    path.name
    for path in system_modules_root.rglob("*.ko")
    if path.is_file()
}
if not system_module_names:
    raise SystemExit(f"no flattened system modules found in {system_modules_root}")

lines = dep_path.read_text(encoding="utf-8").splitlines(keepends=True)
reconciled = []
rows = [("module", "system_dependency", "provider_state", "action")]
pruned = 0
retained = 0

for line in lines:
    if not line.strip() or line.lstrip().startswith("#"):
        reconciled.append(line)
        continue
    try:
        target, raw_dependencies = line.rstrip("\n").split(":", 1)
    except ValueError as exc:
        raise SystemExit(f"malformed modules.dep row: {line!r}") from exc

    dependencies = raw_dependencies.split()
    kept = []
    for dependency in dependencies:
        path = PurePosixPath(dependency)
        if str(path.parent) != "/system/lib/modules":
            kept.append(dependency)
            continue

        name = path.name
        if name in builtin_names:
            rows.append((target, dependency, "BUILTIN", "PRUNED_BUILTIN"))
            pruned += 1
        elif name in system_module_names:
            rows.append((target, dependency, "SYSTEM_DLKM", "RETAINED"))
            retained += 1
            kept.append(dependency)
        else:
            raise SystemExit(
                f"{target} requires absent system provider {dependency}; "
                "it is neither in the matching flattened system_dlkm nor built into vmlinux"
            )

    reconciled.append(f"{target}:" + (f" {' '.join(kept)}" if kept else "") + "\n")

temporary = dep_path.with_name(dep_path.name + ".reconciled")
temporary.write_text("".join(reconciled), encoding="utf-8")
os.chmod(temporary, dep_path.stat().st_mode)
temporary.replace(dep_path)

report_path.write_text(
    "\n".join("\t".join(row) for row in rows) + "\n",
    encoding="utf-8",
)
print(f"modules.dep reconciliation: pruned_builtin_edges={pruned} retained_system_edges={retained}")
PY

for module in "${closure_modules[@]}"; do
    [[ -n ${stage_path_by_name[$module]+x} ]] || {
        die "closure module is absent from stock tree: $module"
    }
done

for module in "${source_modules[@]}"; do
    target=${stage_path_by_name[$module]}
    cp --preserve=mode,timestamps "${source_by_name[$module]}" "$target"
    "$strip_tool" --strip-debug "$target"
done

for module in "${closure_modules[@]}"; do
    target=${stage_path_by_name[$module]}
    has_signature "$target" && die "refusing to append a second signature to $target"
    "$kernel_build_dir/scripts/sign-file" sha1 \
        "$signing_key" \
        "$kernel_build_dir/certs/signing_key.x509" \
        "$target"
    signer=$(modinfo -F signer "$target")
    [[ "$signer" == "$kernel_signing_subject" ]] || {
        die "unexpected signer for $module: ${signer:-<none>} (expected $kernel_signing_subject)"
    }
    [[ "$(modinfo -F sig_id "$target")" == "PKCS#7" ]] || {
        die "signature trailer missing for $module"
    }
done

find "$out_dir/staging" -type f -printf '%P\n' | sort > "$out_dir/staging-files.txt"
(
    cd "$out_dir/staging"
    find . -type f -print0 | sort -z | xargs -0 sha256sum
) > "$out_dir/staging-SHA256SUMS"

cp --reflink=auto "$stock_image" "$out_dir/vendor_dlkm.img"
[[ $(stat -c '%s' "$stock_image") == $(stat -c '%s' "$out_dir/vendor_dlkm.img") ]] || {
    die "candidate image size differs from the stock partition image"
}

declare -A image_mode_by_module=()
declare -A image_uid_by_module=()
declare -A image_gid_by_module=()
declare -A image_selinux_by_module=()
for module in "${closure_modules[@]}"; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    metadata=$(image_inode_metadata "$out_dir/vendor_dlkm.img" "$relative")
    IFS=$'\t' read -r image_mode_by_module[$module] image_uid_by_module[$module] image_gid_by_module[$module] <<< "$metadata"
    image_selinux_by_module[$module]=$(image_selinux_xattr_hex "$out_dir/vendor_dlkm.img" "$relative")
done

modules_dep_relative=/lib/modules/modules.dep
modules_dep_stage="$stage_modules/modules.dep"
metadata=$(image_inode_metadata "$out_dir/vendor_dlkm.img" "$modules_dep_relative")
IFS=$'\t' read -r image_modules_dep_mode image_modules_dep_uid image_modules_dep_gid <<< "$metadata"
image_modules_dep_selinux=$(image_selinux_xattr_hex "$out_dir/vendor_dlkm.img" "$modules_dep_relative")

for module in "${closure_modules[@]}"; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    debugfs -w -R "rm $relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
    debugfs -w -R "write $staged $relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
    restore_image_metadata "$out_dir/vendor_dlkm.img" "$relative" \
        "${image_mode_by_module[$module]}" \
        "${image_uid_by_module[$module]}" \
        "${image_gid_by_module[$module]}" \
        "${image_selinux_by_module[$module]}"
done

# modules.dep is executable policy input for vendor_modprobe, so replace it in
# the image with the same inode metadata safeguards as signed modules.
debugfs -w -R "rm $modules_dep_relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
debugfs -w -R "write $modules_dep_stage $modules_dep_relative" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
restore_image_metadata "$out_dir/vendor_dlkm.img" "$modules_dep_relative" \
    "$image_modules_dep_mode" "$image_modules_dep_uid" "$image_modules_dep_gid" \
    "$image_modules_dep_selinux"

# The copied image still contains the stock hashtree/FEC/footer.  Discard that
# trailing integrity data before computing a new footer from the modified ext4
# payload.  The candidate is private output, never the verified stock image.
truncate -s "${avb_field[original_image_size]}" "$out_dir/vendor_dlkm.img"
avb_args=(
    add_hashtree_footer
    --image "$out_dir/vendor_dlkm.img"
    --partition_size "${avb_field[partition_size]}"
    --partition_name "${avb_field[partition_name]}"
    --hash_algorithm "${avb_field[hash_algorithm]}"
    --salt "${avb_field[salt]}"
    --block_size "${avb_field[block_size]}"
    --fec_num_roots "${avb_field[fec_num_roots]}"
    --algorithm NONE
    --rollback_index "${avb_field[rollback_index]}"
    --rollback_index_location "${avb_field[rollback_index_location]}"
    --flags "${avb_field[flags]}"
)
for prop in "${avb_props[@]}"; do
    avb_args+=(--prop "$prop")
done
"$avbtool" "${avb_args[@]}"
[[ $(stat -c '%s' "$out_dir/vendor_dlkm.img") == "${avb_field[partition_size]}" ]] || {
    die "regenerated vendor_dlkm size does not match the stock partition size"
}
candidate_avb_info=$(read_avb_footer "$out_dir/vendor_dlkm.img")
candidate_root_digest=$(python3 - "$candidate_avb_info" <<'PY'
import re
import sys
match = re.search(r'^      Root Digest:\s+([0-9a-fA-F]+)$', sys.argv[1], re.M)
if not match:
    raise SystemExit('missing candidate AVB root digest')
print(match.group(1))
PY
)
stock_root_digest=$(python3 - "$avb_info" <<'PY'
import re
import sys
match = re.search(r'^      Root Digest:\s+([0-9a-fA-F]+)$', sys.argv[1], re.M)
if not match:
    raise SystemExit('missing stock AVB root digest')
print(match.group(1))
PY
)
[[ "$candidate_root_digest" != "$stock_root_digest" ]] || {
    die "regenerated AVB root digest unexpectedly equals the stock digest"
}
"$avbtool" verify_image --image "$out_dir/vendor_dlkm.img" >/dev/null || {
    die "regenerated AVB footer failed avbtool verification"
}

e2fsck -fn "$out_dir/vendor_dlkm.img" > "$out_dir/e2fsck.txt" 2>&1 || {
    sed -n '1,240p' "$out_dir/e2fsck.txt" >&2
    die "candidate vendor_dlkm filesystem validation failed"
}

mkdir -p "$out_dir/readback"
for module in "${closure_modules[@]}"; do
    staged=${stage_path_by_name[$module]}
    relative=${staged#"$out_dir/staging"}
    readback="$out_dir/readback/${module}.ko"
    debugfs -R "dump $relative $readback" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
    [[ "$(sha256sum "$staged" | awk '{print $1}')" == "$(sha256sum "$readback" | awk '{print $1}')" ]] || {
        die "read-back hash mismatch for $module"
    }
    metadata=$(image_inode_metadata "$out_dir/vendor_dlkm.img" "$relative")
    IFS=$'\t' read -r mode uid gid <<< "$metadata"
    [[ "$mode" == "${image_mode_by_module[$module]}" &&
       "$uid" == "${image_uid_by_module[$module]}" &&
       "$gid" == "${image_gid_by_module[$module]}" ]] || {
        die "read-back inode metadata mismatch for $module"
    }
    [[ "$(image_selinux_xattr_hex "$out_dir/vendor_dlkm.img" "$relative")" == "${image_selinux_by_module[$module]}" ]] || {
        die "read-back security.selinux mismatch for $module"
    }
done

modules_dep_readback="$out_dir/readback/modules.dep"
debugfs -R "dump $modules_dep_relative $modules_dep_readback" "$out_dir/vendor_dlkm.img" >/dev/null 2>&1
cmp -s "$modules_dep_stage" "$modules_dep_readback" || {
    die "read-back modules.dep does not match the reconciled staged metadata"
}
metadata=$(image_inode_metadata "$out_dir/vendor_dlkm.img" "$modules_dep_relative")
IFS=$'\t' read -r mode uid gid <<< "$metadata"
[[ "$mode" == "$image_modules_dep_mode" &&
   "$uid" == "$image_modules_dep_uid" &&
   "$gid" == "$image_modules_dep_gid" ]] || {
    die "read-back inode metadata mismatch for modules.dep"
}
[[ "$(image_selinux_xattr_hex "$out_dir/vendor_dlkm.img" "$modules_dep_relative")" == "$image_modules_dep_selinux" ]] || {
    die "read-back security.selinux mismatch for modules.dep"
}

modules_dep_pruned_builtin_edges=$(awk -F '\t' 'NR > 1 && $4 == "PRUNED_BUILTIN" { count++ } END { print count + 0 }' "$modules_dep_reconciliation")
modules_dep_retained_system_edges=$(awk -F '\t' 'NR > 1 && $4 == "RETAINED" { count++ } END { print count + 0 }' "$modules_dep_reconciliation")

{
    printf 'stock_vendor_root=%s\n' "$stock_root"
    printf 'stock_vendor_image=%s\n' "$stock_image"
    printf 'stock_vendor_image_sha256=%s\n' "$(sha256sum "$stock_image" | awk '{print $1}')"
    printf 'candidate_vendor_image=%s\n' "$out_dir/vendor_dlkm.img"
    printf 'candidate_vendor_image_sha256=%s\n' "$(sha256sum "$out_dir/vendor_dlkm.img" | awk '{print $1}')"
    printf 'candidate_vendor_image_bytes=%s\n' "$(stat -c '%s' "$out_dir/vendor_dlkm.img")"
    printf 'stock_vendor_avb_root_digest=%s\n' "$stock_root_digest"
    printf 'candidate_vendor_avb_root_digest=%s\n' "$candidate_root_digest"
    printf 'vendor_avb_footer=regenerated_hashtree_fec_from_modified_ext4_payload\n'
    printf 'kernel_build_dir=%s\n' "$kernel_build_dir"
    printf 'kernel_release=%s\n' "$kernel_release"
    printf 'system_dlkm_staging_archive=%s\n' "$system_dlkm_staging_archive"
    printf 'system_dlkm_staging_archive_sha256=%s\n' "$(sha256sum "$system_dlkm_staging_archive" | awk '{print $1}')"
    printf 'modules_dep_reconciliation=matching_system_dlkm_and_vmlinux_builtin_boundary\n'
    printf 'modules_dep_pruned_builtin_edges=%s\n' "$modules_dep_pruned_builtin_edges"
    printf 'modules_dep_retained_system_edges=%s\n' "$modules_dep_retained_system_edges"
    printf 'kernel_signing_certificate_der_sha256=%s\n' "$kernel_signing_certificate_der_sha256"
    printf 'kernel_signing_certificate_subject=%s\n' "$kernel_signing_subject"
    printf 'kernel_signing_certificate_present_in_vmlinux=yes\n'
    printf 'source_replacements=%s\n' "${#source_by_name[@]}"
    printf 'signed_closure_modules=%s\n' "${#closure_action[@]}"
    printf 'metadata_policy=stock_module_and_modules.dep_mode_uid_gid_and_security.selinux_preserved; module_names_and_dependencies_are_contract-matched\n'
    printf 'avb=partition-local_footer_regenerated; parent_top_level_vbmeta_not_updated\n'
} > "$out_dir/manifest.txt"

(
    cd "$out_dir"
    sha256sum vendor_dlkm.img manifest.txt staging-files.txt staging-SHA256SUMS \
        vendor-system-dependency-reconciliation.tsv e2fsck.txt
) > "$out_dir/SHA256SUMS"

printf 'STAGED MATCHED WLAN VENDOR-DLKM CANDIDATE\n'
printf 'image=%s\n' "$out_dir/vendor_dlkm.img"
printf 'manifest=%s\n' "$out_dir/manifest.txt"
printf 'signed_closure_modules=%s\n' "${#closure_action[@]}"
printf 'The vendor_dlkm partition-local AVB footer was regenerated; parent/top-level vbmeta was not updated.\n'
