#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only

# Compare a candidate common-kernel provider surface against the immutable
# qualified KMI5 stock-provider contract.  This is intentionally fail closed:
# an enforced ABI report is required before the script may return success.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  tools/kmi5-stock-provider-oracle.sh \
      CANDIDATE_VMLINUX CANDIDATE_MODULE_SYMVERS \
      QUALIFIED_PROVIDER_ORACLE STOCK_MODULE_UNIVERSE \
      ABI_REPORT OUTPUT_DIRECTORY

Exit status:
  0  Existing stock KMI5 provider and ABI records are compatible.
  1  A stock KMI5 incompatibility or incomplete ABI proof was found.
  2  Inputs or tools are invalid.

The ABI report must be the short report produced by the enforced common Kleaf
ABI comparison for the same candidate.  Additions are inventoried but do not
fail the oracle.  Existing removals, signature changes, CRC-only changes, and
changed type roots do fail it.
EOF
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 2
}

[ "$#" -eq 6 ] || {
    usage >&2
    exit 2
}

candidate_vmlinux="$1"
candidate_symvers="$2"
provider_oracle="$3"
module_universe="$4"
abi_report="$5"
output_dir="$6"

for tool in awk dirname mkdir readelf sha256sum sort wc; do
    command -v "$tool" >/dev/null 2>&1 || die "required tool is unavailable: $tool"
done

for input in \
    "$candidate_vmlinux" \
    "$candidate_symvers" \
    "$provider_oracle" \
    "$module_universe" \
    "$abi_report"; do
    [ -f "$input" ] && [ -r "$input" ] || die "input is not a readable file: $input"
done

[ ! -e "$output_dir" ] || die "refusing to overwrite output: $output_dir"
mkdir -p "$output_dir"

readelf -h "$candidate_vmlinux" >/dev/null 2>&1 ||
    die "candidate vmlinux is not a readable ELF file"

provider_header="$(awk -F '\t' 'NR == 1 { print $0; exit }' "$provider_oracle")"
[ "$provider_header" = $'symbol\tbaseline_crc\tcandidate_crc\tresult\tconsumer_edges\tconsumer_count\tpartitions\tconsumers' ] ||
    die "qualified provider oracle has an unexpected schema"

module_header="$(awk -F '\t' 'NR == 1 { print $0; exit }' "$module_universe")"
case "$module_header" in
    $'module\tpartition\tstock_path\tload_mode\tclassification\t'*) ;;
    *) die "stock module universe has an unexpected schema" ;;
esac

results="$output_dir/provider-results.tsv"
affected="$output_dir/affected-stock-consumers.tsv"
signatures="$output_dir/direct-signature-changes.txt"
types="$output_dir/changed-type-roots.txt"
summary="$output_dir/summary.txt"
hashes="$output_dir/input-sha256.txt"

awk -F '\t' -v OFS='\t' '
    NR == FNR {
        if (NF >= 2 && $1 ~ /^0x[0-9a-fA-F]+$/)
            candidate[$2] = tolower($1)
        next
    }
    FNR == 1 {
        print $0
        next
    }
    {
        expected = tolower($2)
        actual = (($1 in candidate) ? candidate[$1] : "MISSING")
        if (actual == "MISSING")
            status = "MISSING"
        else if (actual == expected)
            status = "MATCH"
        else
            status = "CRC_MISMATCH"
        $3 = actual
        $4 = status
        print $0
    }
' "$candidate_symvers" "$provider_oracle" > "$results"

awk -F '\t' -v OFS='\t' '
    BEGIN { print "consumer", "provider_failures", "consumer_edges" }
    NR > 1 && $4 != "MATCH" {
        count = split($8, names, ",")
        for (i = 1; i <= count; i++) {
            if (names[i] == "")
                continue
            failures[names[i]]++
            edges[names[i]] += $5
        }
    }
    END {
        for (consumer in failures)
            print consumer, failures[consumer], edges[consumer]
    }
' "$results" | {
    IFS= read -r header
    printf '%s\n' "$header"
    sort
} > "$affected"

awk '
    /^function symbol changed from / ||
    /^variable symbol changed from / { print }
' "$abi_report" > "$signatures"

awk '/^type .* changed$/ { print }' "$abi_report" > "$types"

provider_stats="$(awk -F '\t' '
    NR > 1 {
        providers++
        edges += $5
        if ($4 == "MATCH") {
            matches++
            match_edges += $5
        } else if ($4 == "MISSING") {
            missing++
            missing_edges += $5
        } else if ($4 == "CRC_MISMATCH") {
            mismatches++
            mismatch_edges += $5
        }
    }
    END {
        printf "%d %d %d %d %d %d %d %d", providers, edges,
            matches, match_edges, missing, missing_edges,
            mismatches, mismatch_edges
    }
' "$results")"
read -r providers edges matches match_edges missing missing_edges mismatches mismatch_edges <<< "$provider_stats"

affected_consumers="$(( $(wc -l < "$affected") - 1 ))"

module_stats="$(awk -F '\t' '
    NR > 1 {
        modules++
        if ($5 == "DORMANT" || $4 == "DORMANT")
            dormant++
        else
            active++
    }
    END { printf "%d %d %d", modules, active, dormant }
' "$module_universe")"
read -r modules active_modules dormant_modules <<< "$module_stats"

abi_additions="$(awk '
    /^[0-9]+ (function|variable) symbol\(s\) added$/ { total += $1 }
    END { print total + 0 }
' "$abi_report")"
abi_removals="$(awk '
    /^[0-9]+ (function|variable) symbol\(s\) removed$/ { total += $1 }
    END { print total + 0 }
' "$abi_report")"
abi_signature_changes="$(wc -l < "$signatures")"
abi_type_roots="$(wc -l < "$types")"
abi_crc_only_changes="$(awk '
    /symbols have only CRC changes$/ {
        for (i = 1; i <= NF; i++)
            if ($i == "symbols" && i > 1 && $(i - 1) ~ /^[0-9]+$/) {
                total += $(i - 1)
                break
            }
    }
    END { print total + 0 }
' "$abi_report")"

result=PASS
if [ "$missing" -ne 0 ] ||
   [ "$mismatches" -ne 0 ] ||
   [ "$abi_removals" -ne 0 ] ||
   [ "$abi_signature_changes" -ne 0 ] ||
   [ "$abi_crc_only_changes" -ne 0 ] ||
   [ "$abi_type_roots" -ne 0 ]; then
    result=FAIL
fi

sha256sum \
    "$candidate_vmlinux" \
    "$candidate_symvers" \
    "$provider_oracle" \
    "$module_universe" \
    "$abi_report" > "$hashes"

cat > "$summary" <<EOF
KMI5 STOCK PROVIDER ORACLE
result=$result
candidate_vmlinux=$candidate_vmlinux
candidate_module_symvers=$candidate_symvers
qualified_provider_oracle=$provider_oracle
stock_module_universe=$module_universe
abi_report=$abi_report
providers=$providers
provider_edges=$edges
matching_providers=$matches
matching_edges=$match_edges
missing_providers=$missing
missing_provider_edges=$missing_edges
crc_mismatching_providers=$mismatches
crc_mismatch_edges=$mismatch_edges
affected_stock_consumers=$affected_consumers
stock_module_rows=$modules
active_module_rows=$active_modules
dormant_module_rows=$dormant_modules
abi_additions=$abi_additions
abi_removals=$abi_removals
abi_signature_changes=$abi_signature_changes
abi_crc_only_changes=$abi_crc_only_changes
abi_changed_type_roots=$abi_type_roots
EOF

cat "$summary"
[ "$result" = PASS ]
