#!/usr/bin/env python3
"""Validate the final signed WLAN .053 hybrid vendor module tree."""

from __future__ import annotations

import argparse
import csv
import importlib.util
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


CELLULAR = {
    "dwc3_msm", "gsim", "ipam", "ipanetm", "oplus_mm_kevent",
    "oplus_mm_kevent_fb", "qcom_glink", "qcom_glink_smem", "qcom_ramdump",
    "qcom_smd", "qcom_va_minidump", "qmi_helpers", "redriver", "repeater",
    "rmnet_aps", "rmnet_core", "rmnet_ctl", "rmnet_mem", "rmnet_offload",
    "rmnet_perf", "rmnet_perf_tether", "rmnet_sch", "rmnet_shs", "rmnet_wlan",
    "rproc_qcom_common", "usb_f_gsi", "wcd_usbss_i2c",
}


def helpers():
    path = Path(__file__).with_name("validate-matched-wlan-vendor-dlkm.py")
    spec = importlib.util.spec_from_file_location("wlan053_final_elf", path)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot load ELF helpers from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


ELF = helpers()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-root", type=Path, required=True)
    parser.add_argument("--candidate-root", type=Path, required=True)
    parser.add_argument("--external-root", type=Path, action="append", default=[])
    parser.add_argument("--vmlinux-symvers", type=Path, required=True)
    parser.add_argument("--expected-release", required=True)
    parser.add_argument("--expected-signer", required=True)
    parser.add_argument("--source-replacement", action="append", default=[])
    parser.add_argument("--resigned-stock", action="append", default=[])
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--expected-module-count", type=int, default=436)
    return parser.parse_args()


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def modinfo(path: Path, field: str) -> str:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)], check=False, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    return ";".join(line for line in result.stdout.splitlines() if line)


def main() -> int:
    args = parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    stock = ELF.scan_root(args.stock_root, "vendor_dlkm", "EXACT_STOCK")
    candidate = ELF.scan_root(args.candidate_root, "vendor_dlkm", "FINAL_CANDIDATE")
    if len(stock) != args.expected_module_count or len(candidate) != args.expected_module_count:
        raise ValueError(
            f"module-count mismatch: stock={len(stock)} candidate={len(candidate)} "
            f"expected={args.expected_module_count}"
        )
    if set(stock) != set(candidate):
        raise ValueError("candidate module-name inventory differs from stock")

    source = set(args.source_replacement)
    resigned = set(args.resigned_stock)
    expected_changed = source | resigned
    if source & resigned:
        raise ValueError("a module cannot be both source-replaced and re-signed stock")

    module_rows = []
    changed = set()
    signature_failures = 0
    vermagic_failures = 0
    for name in sorted(candidate):
        old = stock[name]
        new = candidate[name]
        if old.sha256 != new.sha256:
            changed.add(name)
        if name in source:
            action = "SOURCE_REPLACEMENT"
            vermagic_ok = modinfo(new.path, "vermagic").startswith(args.expected_release + " ")
            signature_ok = new.signed and modinfo(new.path, "signer") == args.expected_signer
        elif name in resigned:
            action = "RE_SIGN_STOCK"
            # Textual stock vermagic is retained intentionally.  MODVERSIONS
            # and the complete provider graph below are the compatibility gate.
            vermagic_ok = True
            signature_ok = new.signed and modinfo(new.path, "signer") == args.expected_signer
        else:
            action = "EXACT_STOCK"
            vermagic_ok = True
            signature_ok = old.sha256 == new.sha256
        signature_failures += not signature_ok
        vermagic_failures += not vermagic_ok
        module_rows.append([
            name, action, old.sha256, new.sha256,
            "CHANGED" if old.sha256 != new.sha256 else "MATCH",
            modinfo(new.path, "vermagic"), modinfo(new.path, "signer"),
            "PASS" if signature_ok else "FAIL",
            "PASS" if vermagic_ok else "FAIL",
        ])

    unexpected_changed = changed - expected_changed
    missing_changed = expected_changed - changed

    external = []
    for index, root in enumerate(args.external_root):
        external.extend(ELF.scan_root(root, f"external{index}", "RETAINED_EXTERNAL").values())
    providers = defaultdict(list)
    for record in list(candidate.values()) + external:
        for symbol, crc in record.exports.items():
            providers[symbol].append((record.label, crc))
    for symbol, values in ELF.parse_symvers(args.vmlinux_symvers).items():
        providers[symbol].extend((f"vmlinux:{provider}", crc) for provider, crc in values)

    import_rows = []
    unresolved = 0
    crc_mismatches = 0
    for consumer in sorted(candidate.values(), key=lambda record: record.name):
        for symbol, expected in sorted(consumer.imports.items()):
            choices = providers.get(symbol, [])
            matching = [item for item in choices if item[1] == expected]
            if matching:
                status = "MATCH"
            elif choices:
                status = "CRC_MISMATCH"
                crc_mismatches += 1
            else:
                status = "UNRESOLVED"
                unresolved += 1
            import_rows.append([
                consumer.name, symbol, ELF.crc(expected),
                ",".join(sorted(label for label, _crc in matching)), status,
            ])

    cellular_rows = []
    cellular_failures = 0
    for name in sorted(CELLULAR):
        if name not in stock:
            raise ValueError(f"missing cellular module: {name}")
        status = "MATCH" if stock[name].sha256 == candidate[name].sha256 else "FAIL"
        cellular_failures += status == "FAIL"
        cellular_rows.append([name, stock[name].sha256, candidate[name].sha256, status])

    write_tsv(
        args.out_dir / "vendor-dlkm-module-contract.tsv",
        ["module", "action", "stock_sha256", "candidate_sha256", "payload", "vermagic", "signer", "signature", "vermagic_contract"],
        module_rows,
    )
    write_tsv(
        args.out_dir / "vendor-dlkm-import-contract.tsv",
        ["consumer", "symbol", "expected_crc", "matching_providers", "status"],
        import_rows,
    )
    write_tsv(
        args.out_dir / "cellular-exact-stock-final.tsv",
        ["module", "stock_sha256", "candidate_sha256", "status"],
        cellular_rows,
    )

    failures = (
        unresolved + crc_mismatches + signature_failures + vermagic_failures +
        cellular_failures + len(unexpected_changed) + len(missing_changed)
    )
    summary = {
        "result": "PASS" if failures == 0 else "FAIL",
        "module_count": len(candidate),
        "changed_modules": sorted(changed),
        "expected_changed_modules": sorted(expected_changed),
        "unexpected_changed_modules": sorted(unexpected_changed),
        "expected_changes_missing": sorted(missing_changed),
        "exact_stock_cellular_modules": len(CELLULAR),
        "cellular_hash_failures": cellular_failures,
        "unresolved_imports": unresolved,
        "crc_mismatches": crc_mismatches,
        "signature_failures": signature_failures,
        "vermagic_failures": vermagic_failures,
    }
    (args.out_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
