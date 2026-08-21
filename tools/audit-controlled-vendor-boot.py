#!/usr/bin/env python3
"""Inventory and validate the early module closure in a vendor boot ramdisk.

This tool is deliberately read-only with respect to the extracted ramdisk,
source-built modules, and ``Module.symvers`` input.  It produces four reports:

* ``module-inventory.tsv``: one row for every vendor-boot module;
* ``dependency-report.tsv``: every MODVERSIONS import and matching provider;
* ``replacement-report.tsv``: source-built replacement contract comparisons;
* ``validation-report.txt``: concise acceptance data for the normal boot path.

The source-module inputs are used only to prove whether an exact-lineage
replacement is available.  They are never copied, stripped, signed, or packed
by this tool.  That separation keeps the inventory useful before a controlled
vendor_boot image is assembled.
"""

from __future__ import annotations

import argparse
import importlib.util
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable


def load_elf_helpers():
    """Load the project's existing ELF/MODVERSIONS reader without duplication."""

    helper_path = Path(__file__).with_name("validate-matched-wlan-vendor-dlkm.py")
    specification = importlib.util.spec_from_file_location(
        "matched_wlan_vendor_dlkm_validator", helper_path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"cannot load {helper_path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


ELF = load_elf_helpers()


def sanitize(value: str) -> str:
    return value.replace("\t", " ").replace("\n", " ").strip()


def join(values: Iterable[str]) -> str:
    return ",".join(sanitize(value) for value in values if value)


def modinfo(path: Path, field: str) -> list[str]:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return [line for line in result.stdout.splitlines() if line]


def parse_load_list(path: Path) -> list[str]:
    if not path.is_file():
        return []
    names: list[str] = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        names.append(Path(line).name)
    return names


def source_modules(roots: list[Path]) -> dict[str, Path]:
    """Map exact build outputs by module name, rejecting ambiguous inputs."""

    modules: dict[str, Path] = {}
    for root in roots:
        candidates = [root] if root.is_file() else sorted(root.glob("*.ko"))
        if not candidates:
            raise ValueError(f"{root}: no direct .ko outputs found")
        for path in candidates:
            record = ELF.module_record(path, "source", "EXACT_SOURCE_BUILD")
            previous = modules.get(record.name)
            if previous is not None and previous != path:
                if ELF.sha256(previous) != ELF.sha256(path):
                    raise ValueError(
                        f"ambiguous source output for {record.name}: {previous} and {path}"
                    )
                continue
            modules[record.name] = path
    return modules


def source_contract(stock, source_path: Path | None) -> tuple[str, int, int, str]:
    if source_path is None:
        return "NOT_PROVEN", 0, 0, ""
    source = ELF.module_record(source_path, "source", "EXACT_SOURCE_BUILD")
    import_mismatches = sum(
        stock.imports.get(symbol) != source.imports.get(symbol)
        for symbol in set(stock.imports) | set(source.imports)
    )
    export_mismatches = sum(
        stock.exports.get(symbol) != source.exports.get(symbol)
        for symbol in set(stock.exports) | set(source.exports)
    )
    status = "PASS" if not import_mismatches and not export_mismatches else "FAIL"
    return status, import_mismatches, export_mismatches, str(source_path)


def module_identity(path: Path | None) -> tuple[str, str, str]:
    """Return the loader-facing metadata needed before replacing a module."""

    if path is None:
        return "", "", ""
    return (
        join(modinfo(path, "vermagic")),
        join(modinfo(path, "signer")) or "UNSIGNED",
        join(modinfo(path, "sig_id")) or "NONE",
    )


def provider_index(records):
    providers: dict[str, list[tuple[str, str, int, str]]] = defaultdict(list)
    for symbol, values in ELF.parse_symvers(ARGS.vmlinux_symvers).items():
        for source, value in values:
            providers[symbol].append((source, "vmlinux", value, "VMLINUX"))
    for record in records.values():
        for symbol, value in record.exports.items():
            providers[symbol].append((record.name, "vendor_boot", value, "STOCK_VENDOR_BOOT"))
    return providers


def write_tsv(path: Path, header: list[str], rows: Iterable[list[str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        output.write("\t".join(header) + "\n")
        for row in rows:
            output.write("\t".join(sanitize(value) for value in row) + "\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modules-root", type=Path, required=True)
    parser.add_argument("--vmlinux-symvers", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--source-module-root", action="append", type=Path, default=[])
    parser.add_argument("--normal-load", type=Path, required=True)
    parser.add_argument("--recovery-load", type=Path, required=True)
    parser.add_argument("--fragment", default="vendor_ramdisk00")
    parser.add_argument("--source-lineage", required=True)
    return parser.parse_args()


ARGS = parse_args()


def main() -> int:
    modules_root = ARGS.modules_root.resolve()
    if not modules_root.is_dir():
        raise ValueError(f"{modules_root}: expected extracted lib/modules directory")
    if not ARGS.vmlinux_symvers.is_file():
        raise ValueError(f"{ARGS.vmlinux_symvers}: missing exact-build Module.symvers")

    normal_entries = parse_load_list(ARGS.normal_load)
    recovery_entries = parse_load_list(ARGS.recovery_load)
    normal = set(normal_entries)
    recovery = set(recovery_entries)
    records = ELF.scan_root(modules_root, "vendor_boot", "STOCK_VENDOR_BOOT")
    source_by_name = source_modules(ARGS.source_module_root)
    providers = provider_index(records)
    ARGS.out_dir.mkdir(parents=True, exist_ok=True)

    inventory_rows: list[list[str]] = []
    replacement_rows: list[list[str]] = []
    dependency_rows: list[list[str]] = []
    normal_unresolved = 0
    normal_crc_mismatches = 0
    normal_imports = 0

    for name, record in sorted(records.items()):
        filename = record.path.name
        mode = (
            "NORMAL+RECOVERY"
            if filename in normal and filename in recovery
            else "NORMAL"
            if filename in normal
            else "RECOVERY"
            if filename in recovery
            else "DORMANT"
        )
        source_path = source_by_name.get(name)
        contract, import_mismatches, export_mismatches, source_text = source_contract(
            record, source_path
        )
        signer = join(modinfo(record.path, "signer"))
        sig_id = join(modinfo(record.path, "sig_id"))
        params = join(modinfo(record.path, "parm"))
        aliases = join(modinfo(record.path, "alias"))
        firmware = join(modinfo(record.path, "firmware"))
        softdeps = join(modinfo(record.path, "softdep"))
        srcversion = join(modinfo(record.path, "srcversion"))
        vermagic = join(modinfo(record.path, "vermagic"))
        source_vermagic, source_signer, source_sig_id = module_identity(source_path)
        vermagic_contract = (
            "NOT_PROVEN"
            if source_path is None
            else "MATCH"
            if vermagic == source_vermagic
            else "DIFFERENT"
        )
        signing_contract = (
            "NOT_PROVEN"
            if source_path is None
            else "REQUIRES_PROJECT_SIGNING"
            if source_signer == "UNSIGNED"
            else "SOURCE_SIGNED"
        )
        inventory_rows.append(
            [
                name,
                filename,
                str(record.path.relative_to(modules_root.parent)),
                ARGS.fragment,
                mode,
                "UNKNOWN_NO_CHARGER_SPECIFIC_LIST",
                str(record.path.stat().st_size),
                record.sha256,
                signer or "UNSIGNED",
                sig_id or "NONE",
                vermagic,
                srcversion,
                join(record.depends),
                softdeps,
                aliases,
                firmware,
                params,
                str(len(record.imports)),
                str(len(record.exports)),
                "YES" if source_path else "NOT_PROVEN",
                source_text,
                ARGS.source_lineage if source_path else "",
                contract,
                str(import_mismatches),
                str(export_mismatches),
            ]
        )
        replacement_rows.append(
            [
                name,
                mode,
                str(record.path),
                source_text,
                contract,
                str(import_mismatches),
                str(export_mismatches),
                source_vermagic,
                source_signer,
                source_sig_id,
                vermagic_contract,
                signing_contract,
                "RETAIN_STOCK_DORMANT" if mode == "DORMANT" else "REPLACEMENT_REQUIRES_BOOT_ORDER_REVIEW",
            ]
        )
        for symbol, expected in sorted(record.imports.items()):
            matches = [entry for entry in providers.get(symbol, []) if entry[2] == expected]
            known = providers.get(symbol, [])
            if matches:
                provider_names = ",".join(sorted({entry[0] for entry in matches}))
                provider_areas = ",".join(sorted({entry[1] for entry in matches}))
                provider_crc = ",".join(sorted({f"0x{entry[2]:08x}" for entry in matches}))
                provider_provenance = ",".join(sorted({entry[3] for entry in matches}))
                result = "MATCH"
            elif known:
                provider_names = ",".join(sorted({entry[0] for entry in known}))
                provider_areas = ",".join(sorted({entry[1] for entry in known}))
                provider_crc = ",".join(sorted({f"0x{entry[2]:08x}" for entry in known}))
                provider_provenance = ",".join(sorted({entry[3] for entry in known}))
                result = "CRC_MISMATCH"
            else:
                provider_names = ""
                provider_areas = ""
                provider_crc = ""
                provider_provenance = ""
                result = "UNRESOLVED"
            dependency_rows.append(
                [
                    name,
                    mode,
                    symbol,
                    f"0x{expected:08x}",
                    provider_names,
                    provider_areas,
                    provider_crc,
                    provider_provenance,
                    result,
                ]
            )
            if filename in normal:
                normal_imports += 1
                if result == "UNRESOLVED":
                    normal_unresolved += 1
                elif result == "CRC_MISMATCH":
                    normal_crc_mismatches += 1

    write_tsv(
        ARGS.out_dir / "module-inventory.tsv",
        [
            "module",
            "filename",
            "stock_path",
            "ramdisk_fragment",
            "load_mode",
            "charger_mode",
            "size",
            "sha256",
            "signer",
            "signature_id",
            "vermagic",
            "srcversion",
            "depends",
            "softdeps",
            "aliases",
            "firmware",
            "parameters",
            "import_count",
            "export_count",
            "source_build_available",
            "source_build_path",
            "source_lineage",
            "source_contract",
            "source_import_crc_mismatches",
            "source_export_crc_mismatches",
        ],
        inventory_rows,
    )
    write_tsv(
        ARGS.out_dir / "dependency-report.tsv",
        [
            "consumer",
            "load_mode",
            "symbol",
            "expected_crc",
            "provider",
            "provider_area",
            "provider_crc",
            "provider_provenance",
            "result",
        ],
        dependency_rows,
    )
    write_tsv(
        ARGS.out_dir / "replacement-report.tsv",
        [
            "module",
            "load_mode",
            "stock_module",
            "exact_source_output",
            "contract",
            "import_crc_mismatches",
            "export_crc_mismatches",
            "source_vermagic",
            "source_signer",
            "source_signature_id",
            "vermagic_contract",
            "signing_contract",
            "recommended_action",
        ],
        replacement_rows,
    )

    source_available = sum(1 for row in inventory_rows if row[19] == "YES")
    report = [
        "CONTROLLED VENDOR_BOOT STATIC CLOSURE",
        f"modules={len(records)}",
        f"normal_modules_load_entries={len(normal_entries)}",
        f"normal_modules_distinct={len(normal)}",
        f"recovery_modules_load_entries={len(recovery_entries)}",
        f"recovery_modules_distinct={len(recovery)}",
        f"exact_source_outputs_available={source_available}",
        f"source_not_proven={len(records) - source_available}",
        f"normal_imports={normal_imports}",
        f"normal_unresolved_imports={normal_unresolved}",
        f"normal_crc_mismatches={normal_crc_mismatches}",
        "normal_closure_result="
        + ("PASS" if not normal_unresolved and not normal_crc_mismatches else "FAIL"),
        "note=charger mode has no distinct modules.load file in the stock vendor ramdisk; its module policy is not inferred.",
        f"note=source availability means an exact {ARGS.source_lineage} source-build output is present; absence means not proven, not necessarily source absent.",
    ]
    (ARGS.out_dir / "validation-report.txt").write_text("\n".join(report) + "\n", encoding="utf-8")
    print("\n".join(report))
    return 0 if not normal_unresolved and not normal_crc_mismatches else 1


if __name__ == "__main__":
    raise SystemExit(main())
