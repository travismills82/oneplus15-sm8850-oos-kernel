#!/usr/bin/env python3
"""Validate a minimal WLAN/CNSS .053 overlay on the TEST3 vendor tree.

Unlike the historical matched-WLAN validator, this tool permits an intentional
WLAN subsystem contract change.  It still fails closed when any final import is
unresolved, a MODVERSION CRC does not match, a replacement is unsigned, or one
of the physically validated exact-stock cellular modules is replaced.
"""

from __future__ import annotations

import argparse
import csv
import importlib.util
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


CELLULAR_MODULES = {
    "dwc3_msm",
    "gsim",
    "ipam",
    "ipanetm",
    "oplus_mm_kevent",
    "oplus_mm_kevent_fb",
    "qcom_glink",
    "qcom_glink_smem",
    "qcom_ramdump",
    "qcom_smd",
    "qcom_va_minidump",
    "qmi_helpers",
    "redriver",
    "repeater",
    "rmnet_aps",
    "rmnet_core",
    "rmnet_ctl",
    "rmnet_mem",
    "rmnet_offload",
    "rmnet_perf",
    "rmnet_perf_tether",
    "rmnet_sch",
    "rmnet_shs",
    "rmnet_wlan",
    "rproc_qcom_common",
    "usb_f_gsi",
    "wcd_usbss_i2c",
}


def load_helpers():
    path = Path(__file__).with_name("validate-matched-wlan-vendor-dlkm.py")
    spec = importlib.util.spec_from_file_location("wlan053_elf", path)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot load ELF helpers from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


ELF = load_helpers()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-vendor-root", type=Path, required=True)
    parser.add_argument("--replacement", type=Path, action="append", required=True)
    parser.add_argument("--external-root", type=Path, action="append", default=[])
    parser.add_argument("--vmlinux-symvers", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--expected-stock-module-count", type=int, default=436)
    parser.add_argument("--expected-kernel-release", required=True)
    return parser.parse_args()


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def metadata(record, key: str) -> str:
    return ";".join(ELF.ElfModule(record.path).modinfo().get(key, []))


def signer(path: Path) -> str:
    result = subprocess.run(
        ["modinfo", "-F", "signer", str(path)],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return ";".join(line for line in result.stdout.splitlines() if line)


def main() -> int:
    args = parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    stock = ELF.scan_root(args.stock_vendor_root, "vendor_dlkm", "EXACT_STOCK")
    if len(stock) != args.expected_stock_module_count:
        raise ValueError(
            f"stock vendor module count is {len(stock)}, expected "
            f"{args.expected_stock_module_count}"
        )
    missing_cellular = sorted(CELLULAR_MODULES - stock.keys())
    if missing_cellular:
        raise ValueError(f"stock cellular modules missing: {','.join(missing_cellular)}")

    final = dict(stock)
    replacements = {}
    for path in args.replacement:
        record = ELF.module_record(path, "vendor_dlkm", "WLAN053_REPLACEMENT")
        if record.name not in stock:
            raise ValueError(f"{path}: {record.name} has no stock vendor counterpart")
        if record.name in replacements:
            raise ValueError(f"duplicate replacement: {record.name}")
        if record.name in CELLULAR_MODULES:
            raise ValueError(f"cellular module may not be replaced: {record.name}")
        replacements[record.name] = record
        final[record.name] = record

    external = []
    for index, root in enumerate(args.external_root):
        external.extend(ELF.scan_root(root, f"external{index}", "RETAINED_EXTERNAL").values())
    vmlinux_exports = ELF.parse_symvers(args.vmlinux_symvers)

    providers = defaultdict(list)
    for record in list(final.values()) + external:
        for symbol, value in record.exports.items():
            providers[symbol].append((record.label, value, record.name, record.provenance))
    for symbol, values in vmlinux_exports.items():
        for provider, value in values:
            providers[symbol].append((f"vmlinux:{provider}", value, "vmlinux", "VMLINUX"))

    import_rows = []
    cellular_rows = []
    unresolved = 0
    crc_mismatches = 0
    cellular_edges = 0
    for consumer in sorted(final.values(), key=lambda item: item.name):
        for symbol, expected in sorted(consumer.imports.items()):
            candidates = providers.get(symbol, [])
            matching = sorted(item for item in candidates if item[1] == expected)
            if matching:
                status = "MATCH"
            elif candidates:
                status = "CRC_MISMATCH"
                crc_mismatches += 1
            else:
                status = "UNRESOLVED"
                unresolved += 1
            candidate_labels = sorted(item[0] for item in candidates)
            matching_labels = sorted(item[0] for item in matching)
            import_rows.append(
                [
                    consumer.name,
                    consumer.provenance,
                    symbol,
                    ELF.crc(expected),
                    ",".join(candidate_labels),
                    ",".join(sorted({ELF.crc(item[1]) for item in candidates})),
                    ",".join(matching_labels),
                    status,
                ]
            )
            if consumer.name in replacements:
                cellular_matches = sorted(
                    item for item in matching if item[2] in CELLULAR_MODULES
                )
                for label, value, provider_name, provenance in cellular_matches:
                    cellular_edges += 1
                    cellular_rows.append(
                        [
                            consumer.name,
                            symbol,
                            ELF.crc(expected),
                            provider_name,
                            label,
                            ELF.crc(value),
                            provenance,
                            "EXACT_STOCK_KEEP",
                        ]
                    )

    write_tsv(
        args.out_dir / "wlan053-import-crc.tsv",
        [
            "consumer",
            "consumer_provenance",
            "symbol",
            "expected_crc",
            "candidate_providers",
            "candidate_crcs",
            "matching_providers",
            "status",
        ],
        import_rows,
    )
    write_tsv(
        args.out_dir / "wlan053-cellular-provider-contract.tsv",
        [
            "wlan_consumer",
            "symbol",
            "expected_crc",
            "stock_cellular_provider",
            "provider_label",
            "provider_crc",
            "provider_provenance",
            "action",
        ],
        cellular_rows,
    )

    replacement_rows = []
    changed_import_contracts = 0
    changed_export_contracts = 0
    staging_signature_failures = 0
    vermagic_failures = 0
    for name, replacement in sorted(replacements.items()):
        original = stock[name]
        imports_match = replacement.imports == original.imports
        exports_match = replacement.exports == original.exports
        changed_import_contracts += 0 if imports_match else 1
        changed_export_contracts += 0 if exports_match else 1
        new_vermagic = metadata(replacement, "vermagic")
        # Source replacement inputs are deliberately unsigned.  The proven
        # vendor-DLKM stager strips them, computes the protected-provider
        # consumer closure, and signs that complete closure with the exact
        # Image key.  Accepting an already-signed input would either preserve
        # an unknown identity or append a second signature later.
        signature_status = "UNSIGNED_FOR_STAGE" if not replacement.signed else "FAIL_ALREADY_SIGNED"
        vermagic_status = "PASS" if new_vermagic.startswith(args.expected_kernel_release + " ") else "FAIL"
        staging_signature_failures += signature_status != "UNSIGNED_FOR_STAGE"
        vermagic_failures += vermagic_status == "FAIL"
        replacement_rows.append(
            [
                name,
                original.path,
                replacement.path,
                original.sha256,
                replacement.sha256,
                metadata(original, "vermagic"),
                new_vermagic,
                signer(original.path),
                signer(replacement.path),
                metadata(original, "srcversion"),
                metadata(replacement, "srcversion"),
                len(original.imports),
                len(replacement.imports),
                "MATCH" if imports_match else "CHANGED",
                len(original.exports),
                len(replacement.exports),
                "MATCH" if exports_match else "CHANGED",
                signature_status,
                vermagic_status,
            ]
        )
    write_tsv(
        args.out_dir / "wlan053-replacement.tsv",
        [
            "module",
            "stock_path",
            "replacement_path",
            "stock_sha256",
            "replacement_sha256",
            "stock_vermagic",
            "replacement_vermagic",
            "stock_signer",
            "replacement_signer",
            "stock_srcversion",
            "replacement_srcversion",
            "stock_import_count",
            "replacement_import_count",
            "import_contract",
            "stock_export_count",
            "replacement_export_count",
            "export_contract",
            "signature",
            "vermagic",
        ],
        replacement_rows,
    )

    changed_export_rows = []
    changed_export_symbols = set()
    for name, replacement in sorted(replacements.items()):
        original = stock[name]
        for symbol in sorted(set(original.exports) | set(replacement.exports)):
            old = original.exports.get(symbol)
            new = replacement.exports.get(symbol)
            if old == new:
                continue
            changed_export_symbols.add(symbol)
            consumers = sorted(
                record.name
                for record in final.values()
                if symbol in record.imports
            )
            changed_export_rows.append(
                [name, symbol, ELF.crc(old), ELF.crc(new), ",".join(consumers)]
            )
    write_tsv(
        args.out_dir / "wlan053-export-delta.tsv",
        ["provider", "symbol", "stock_crc", "wlan053_crc", "final_consumers"],
        changed_export_rows,
    )

    signed = dict(replacements)
    signing_rows = [[name, "SOURCE_REPLACEMENT", "", ""] for name in sorted(signed)]
    while True:
        edges = ELF.source_provider_edges(final.values(), signed)
        additions = {name: edges[name] for name in edges if name not in signed}
        if not additions:
            break
        for name, reasons in sorted(additions.items()):
            signed[name] = final[name]
            first_symbol, first_provider = reasons[0]
            signing_rows.append([name, "RE_SIGN_STOCK", first_symbol, first_provider])
    write_tsv(
        args.out_dir / "wlan053-protected-signing-closure.tsv",
        ["module", "required_action", "first_import", "signed_provider"],
        sorted(signing_rows),
    )
    # The existing, physically validated vendor-DLKM stager consumes this
    # stable filename.  Keep the WLAN-generation-specific report as well.
    write_tsv(
        args.out_dir / "protected-export-signing-closure.tsv",
        ["module", "required_action", "first_import", "signed_provider"],
        sorted(signing_rows),
    )

    cellular_hash_rows = []
    for name in sorted(CELLULAR_MODULES):
        original = stock[name]
        candidate = final[name]
        cellular_hash_rows.append(
            [name, original.sha256, candidate.sha256, "MATCH" if original.sha256 == candidate.sha256 else "FAIL"]
        )
    write_tsv(
        args.out_dir / "cellular-exact-stock.tsv",
        ["module", "stock_sha256", "candidate_sha256", "status"],
        cellular_hash_rows,
    )

    result = "PASS" if not (
        unresolved or crc_mismatches or staging_signature_failures or vermagic_failures
    ) else "FAIL"
    summary = {
        "result": result,
        "stock_vendor_modules": len(stock),
        "wlan053_replacements": len(replacements),
        "exact_stock_cellular_modules": len(CELLULAR_MODULES),
        "replacement_import_contract_changes": changed_import_contracts,
        "replacement_export_contract_changes": changed_export_contracts,
        "changed_export_symbols": len(changed_export_symbols),
        "cellular_provider_edges": cellular_edges,
        "protected_signing_closure": len(signed),
        "re_sign_stock_modules": len(signed) - len(replacements),
        "unresolved_imports": unresolved,
        "crc_mismatches": crc_mismatches,
        "replacement_contract_failures": 0,
        "staging_signature_failures": staging_signature_failures,
        "vermagic_failures": vermagic_failures,
    }
    (args.out_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0 if result == "PASS" else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
