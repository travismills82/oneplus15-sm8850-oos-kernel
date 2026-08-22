#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Derive the controlled-v1 Kleaf SCM identity from reviewed kernel build inputs.
# This uses Kleaf's supported common/workspace_status.json interface; it never
# modifies generated kernel headers, module vermagic, or binary artifacts.

set -euo pipefail

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

usage() {
    cat <<'EOF'
Usage:
  tools/compute-controlled-oos-source-id.sh \
      --write-workspace-status <path> [--expect-source-id <commit>] [--print-json]

The source identity is the newest committed change in the reviewed build-input
scope. It intentionally excludes documentation and package-only tooling.
If --expect-source-id is supplied, the helper fails unless the current build
inputs exactly match that identity.
EOF
}

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
scope_file="$repo_root/tools/controlled-oos-signing-v1-build-inputs.txt"
external_manifest="$repo_root/tools/controlled-oos-signing-v1-external-inputs.tsv"
status_path=
expected_source_id=
print_json=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --write-workspace-status)
            status_path=${2:-}
            [[ -n "$status_path" ]] || die "--write-workspace-status requires a path"
            shift 2
            ;;
        --expect-source-id)
            expected_source_id=${2:-}
            [[ -n "$expected_source_id" ]] || die "--expect-source-id requires a commit"
            shift 2
            ;;
        --print-json)
            print_json=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            die "unknown argument: $1"
            ;;
    esac
done

[[ -n "$status_path" ]] || die "--write-workspace-status is required"
cd "$repo_root"
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "not in a git worktree"
[[ -f "$scope_file" ]] || die "missing build-input scope: $scope_file"
[[ -f "$external_manifest" ]] || die "missing external-input manifest: $external_manifest"

mapfile -t source_paths < <(awk 'NF && $1 !~ /^#/' "$scope_file")
[[ ${#source_paths[@]} -gt 0 ]] || die "empty controlled-v1 build-input scope"
for path in "${source_paths[@]}"; do
    [[ -e "$path" ]] || die "declared build input is missing: $path"
done

# No uncommitted source input may claim an old committed identity. Known local
# symlinks are explicit external inputs and are verified below instead.
git diff --quiet -- "${source_paths[@]}" ||
    die "controlled-v1 kernel release identity cannot be used because kernel build inputs changed"
git diff --cached --quiet -- "${source_paths[@]}" ||
    die "controlled-v1 kernel release identity cannot be used because kernel build inputs changed"

is_known_local_link() {
    case "$1" in
        kernel_platform/prebuilts|kernel_platform/qcom/opensource/devicetree/oplus|kernel_platform/soc-repo/arch/arm64/boot/dts/vendor)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

mapfile -t untracked < <(git ls-files --others --exclude-standard -- "${source_paths[@]}")
for path in "${untracked[@]}"; do
    if ! is_known_local_link "$path" || [[ ! -L "$path" ]]; then
        die "untracked kernel build input: $path"
    fi
done

tree_sha256() {
    local path=$1
    (
        cd "$path"
        find -L . -type f -printf '%P\0' | LC_ALL=C sort -z | \
            while IFS= read -r -d '' file; do
                sha256sum -- "$file"
            done | sha256sum | awk '{print $1}'
    )
}

while IFS=$'\t' read -r kind path expected; do
    [[ -z "$kind" || "$kind" == \#* ]] && continue
    case "$kind" in
        file)
            [[ -f "$path" ]] || die "missing external build input: $path"
            actual=$(sha256sum -- "$path" | awk '{print $1}')
            ;;
        tree)
            [[ -d "$path" ]] || die "missing external build-input tree: $path"
            actual=$(tree_sha256 "$path")
            ;;
        *)
            die "unknown external input kind: $kind"
            ;;
    esac
    [[ "$actual" == "$expected" ]] ||
        die "controlled-v1 kernel release identity cannot be used because external input changed: $path"
done < "$external_manifest"

source_id=$(git log -1 --format=%H HEAD -- "${source_paths[@]}")
[[ -n "$source_id" ]] || die "unable to derive controlled-v1 kernel source identity"

if [[ -n "$expected_source_id" ]]; then
    expected_source_id=$(git rev-parse --verify "${expected_source_id}^{commit}") ||
        die "unknown expected source identity: $expected_source_id"
    [[ "$expected_source_id" == "$source_id" ]] &&
        git diff --quiet "$expected_source_id" HEAD -- "${source_paths[@]}" ||
        die "controlled-v1 kernel release identity cannot be reused because kernel build inputs changed"
fi

source_short=$(git rev-parse --short=12 "$source_id")
source_date_epoch=$(git show -s --format=%ct "$source_id")
status_json=$(printf '{\n  "SCMVERSION": "-g%s",\n  "SOURCE_DATE_EPOCH": %s\n}\n' \
    "$source_short" "$source_date_epoch")

case "$status_path" in
    /*) ;;
    *) status_path="$repo_root/$status_path" ;;
esac
[[ ! -e "$status_path" && ! -L "$status_path" ]] ||
    die "refusing to replace existing workspace-status input: $status_path"
mkdir -p "$(dirname "$status_path")"
printf '%s' "$status_json" > "$status_path"

if ((print_json)); then
    printf '%s' "$status_json"
else
    printf '%s\n' "$source_id"
fi
