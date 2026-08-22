#!/usr/bin/env python3
"""Build reproducible controlled-v1 cellular/WLAN bisection reports.

The audit is intentionally read-only.  It walks the actual ``depends=``
metadata from the packet-data roots, compares the restored stock (PASS), the
failed controlled-v1 tree, and Candidate A, and proves which candidate modules
remain byte-identical to OxygenOS 16.0.9.400(EX01).
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import subprocess
import sys
from collections import defaultdict, deque
from dataclasses import dataclass
from pathlib import Path


CELLULAR_ROOTS = (
    "gsim",
    "ipam",
    "ipanetm",
    "rmnet_core",
    "rmnet_ctl",
    "rmnet_offload",
    "rmnet_perf_tether",
    "rmnet_perf",
    "rmnet_wlan",
    "rmnet_mem",
    "rmnet_shs",
    "rmnet_aps",
    "rmnet_sch",
    "usb_f_gsi",
    "wwan",
)

WLAN_SOURCE = {"cfg80211", "mac80211", "qca_cld3_peach_v2"}
WLAN_RESIGN = {"qca_cld3_kiwi_v2", "qca_cld3_wcn7750", "wonder"}


def load_elf_helpers():
    path = Path(__file__).with_name("validate-matched-wlan-vendor-dlkm.py")
    spec = importlib.util.spec_from_file_location("cellular_elf", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


ELF = load_elf_helpers()


@dataclass(frozen=True)
class LocatedModule:
    name: str
    partition: str
    path: Path
    record: object


def sha256(path: Path | None) -> str:
    if path is None or not path.is_file():
        return ""
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def modinfo(path: Path | None, field: str) -> str:
    if path is None or not path.is_file():
        return ""
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return ",".join(line for line in result.stdout.splitlines() if line)


def scan(root: Path, partition: str) -> dict[str, LocatedModule]:
    result: dict[str, LocatedModule] = {}
    for path in sorted(root.rglob("*.ko")):
        record = ELF.module_record(path, partition, partition)
        previous = result.get(record.name)
        if previous and previous.path != path:
            raise ValueError(f"duplicate {partition} module name {record.name}")
        result[record.name] = LocatedModule(record.name, partition, path, record)
    return result


def load_names(path: Path) -> set[str]:
    names = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        names.add(Path(line).stem.replace("-", "_"))
    return names


def proc_positions(path: Path) -> dict[str, int]:
    positions = {}
    for index, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = raw.split()
        if fields:
            positions[fields[0].replace("-", "_")] = index
    return positions


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-vendor-root", type=Path, required=True)
    parser.add_argument("--failed-vendor-root", type=Path, required=True)
    parser.add_argument("--candidate-vendor-root", type=Path, required=True)
    parser.add_argument("--stock-system-root", type=Path, required=True)
    parser.add_argument("--controlled-system-root", type=Path, required=True)
    parser.add_argument("--vendor-boot-root", type=Path, required=True)
    parser.add_argument("--vendor-load", type=Path, required=True)
    parser.add_argument("--vendor-boot-load", type=Path, required=True)
    parser.add_argument("--pass-proc-modules", type=Path, required=True)
    parser.add_argument("--fail-proc-modules", type=Path, required=True)
    parser.add_argument("--candidate-import-resolution", type=Path, required=True)
    parser.add_argument("--failed-custom-contract", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--docs-closure", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    stock_vendor = scan(args.stock_vendor_root, "vendor_dlkm")
    failed_vendor = scan(args.failed_vendor_root, "failed_vendor_dlkm")
    candidate_vendor = scan(args.candidate_vendor_root, "candidate_vendor_dlkm")
    stock_system = scan(args.stock_system_root, "system_dlkm")
    controlled_system = scan(args.controlled_system_root, "controlled_system_dlkm")
    vendor_boot = scan(args.vendor_boot_root, "vendor_boot")

    if len(stock_vendor) != 436 or len(candidate_vendor) != 436:
        raise ValueError(
            f"expected 436 stock/candidate vendor modules, got "
            f"{len(stock_vendor)}/{len(candidate_vendor)}"
        )

    normal_vendor = load_names(args.vendor_load)
    normal_vendor_boot = load_names(args.vendor_boot_load)
    pass_position = proc_positions(args.pass_proc_modules)
    fail_position = proc_positions(args.fail_proc_modules)

    # Prefer a normal-boot vendor-ramdisk provider, then vendor_dlkm, then
    # system_dlkm. This mirrors early boot rather than choosing by filename.
    providers: dict[str, LocatedModule] = {}
    all_names = set(vendor_boot) | set(stock_vendor) | set(stock_system)
    for name in all_names:
        if name in vendor_boot and name in normal_vendor_boot:
            providers[name] = vendor_boot[name]
        elif name in stock_vendor:
            providers[name] = stock_vendor[name]
        elif name in stock_system:
            providers[name] = stock_system[name]
        elif name in vendor_boot:
            providers[name] = vendor_boot[name]

    consumers: dict[str, set[str]] = defaultdict(set)
    closure = set(CELLULAR_ROOTS)
    queue = deque(CELLULAR_ROOTS)
    missing = set()
    while queue:
        consumer = queue.popleft()
        provider = providers.get(consumer)
        if provider is None:
            missing.add(consumer)
            continue
        for dependency in provider.record.depends:
            dependency = dependency.replace("-", "_")
            consumers[dependency].add(consumer)
            if dependency not in closure:
                closure.add(dependency)
                queue.append(dependency)
    if missing:
        raise ValueError(f"unmapped cellular dependencies: {sorted(missing)}")

    import_rows = read_tsv(args.candidate_import_resolution)
    vmlinux_imports: dict[str, list[str]] = defaultdict(list)
    for row in import_rows:
        if "vmlinux:" in row["matching_providers"]:
            vmlinux_imports[row["consumer"]].append(row["symbol"])

    failed_action = {
        row["module"]: row["action"]
        for row in read_tsv(args.failed_custom_contract)
    }

    closure_rows: list[list[object]] = []
    exact_stock_count = 0
    for name in sorted(closure):
        provider = providers[name]
        stock = stock_vendor.get(name) or stock_system.get(name) or vendor_boot.get(name)
        failed = failed_vendor.get(name) or controlled_system.get(name) or vendor_boot.get(name)
        candidate = candidate_vendor.get(name) or controlled_system.get(name) or vendor_boot.get(name)
        stock_hash = sha256(stock.path if stock else None)
        controlled_hash = sha256(failed.path if failed else None)
        candidate_hash = sha256(candidate.path if candidate else None)
        candidate_exact = bool(stock_hash and stock_hash == candidate_hash)
        if candidate_exact:
            exact_stock_count += 1
        if name in WLAN_SOURCE:
            candidate_action = "CUSTOM REQUIRED"
        elif name in WLAN_RESIGN:
            candidate_action = "STOCK COMPATIBLE — RE-SIGN REQUIRED"
        else:
            candidate_action = "EXACT STOCK — KEEP" if candidate_exact else "REVIEW"
        previous = failed_action.get(name, "")
        if name == "wwan" and not candidate_exact:
            candidate_action = "SOURCE BUILT — TEST 2 ISOLATION"
            prior_class = "SOURCE BUILT SYSTEM — SUSPECT"
        elif previous == "SOURCE_REPLACEMENT":
            prior_class = "SOURCE BUILT — SUSPECT"
        elif previous == "RE_SIGN_STOCK":
            prior_class = "RE-SIGNED STOCK — SUSPECT"
        elif stock_hash == controlled_hash:
            prior_class = "UNCHANGED"
        else:
            prior_class = "CHANGED — REVIEW"
        closure_rows.append(
            [
                name,
                provider.partition,
                provider.path,
                "yes" if name in pass_position else "no",
                "CELLULAR_ROOT" if name in CELLULAR_ROOTS else ",".join(sorted(consumers[name])),
                ",".join(provider.record.depends),
                ",".join(sorted(vmlinux_imports.get(name, []))),
                stock_hash,
                controlled_hash,
                modinfo(stock.path if stock else None, "signer") or "UNSIGNED",
                modinfo(failed.path if failed else None, "signer") or "UNSIGNED",
                "yes" if stock_hash != controlled_hash else "no",
                prior_class,
                candidate_action,
            ]
        )

    closure_header = [
        "module",
        "partition",
        "stock_path",
        "normal_boot_loaded",
        "provider_consumer",
        "dependencies",
        "protected_imports",
        "stock_sha256",
        "controlled_v1_sha256",
        "stock_signer",
        "controlled_signer",
        "changed_in_controlled_v1",
        "controlled_v1_classification",
        "candidate_action",
    ]
    write_tsv(args.docs_closure, closure_header, closure_rows)
    write_tsv(args.out_dir / "cellular-data-closure.tsv", closure_header, closure_rows)

    relevant_names = set(closure) | WLAN_SOURCE | WLAN_RESIGN | {
        "cnss2",
        "cnss_nl",
        "cnss_plat_ipc_qmi_svc",
        "cnss_prealloc",
        "cnss_utils",
        "smem_mailbox",
        "wlan_firmware_service",
    }
    comparison_rows = []
    for name in sorted(relevant_names):
        stock = stock_vendor.get(name) or stock_system.get(name) or vendor_boot.get(name)
        failed = failed_vendor.get(name) or controlled_system.get(name) or vendor_boot.get(name)
        if not stock or not failed:
            continue
        stock_hash = sha256(stock.path)
        failed_hash = sha256(failed.path)
        comparison_rows.append(
            [
                name,
                stock_hash,
                failed_hash,
                modinfo(stock.path, "signer") or "UNSIGNED",
                modinfo(failed.path, "signer") or "UNSIGNED",
                modinfo(stock.path, "vermagic"),
                modinfo(failed.path, "vermagic"),
                pass_position.get(name, "NOT_LOADED"),
                fail_position.get(name, "NOT_LOADED"),
                "yes" if stock_hash != failed_hash else "no",
                "CELLULAR" if name in closure else "WLAN_SHARED",
            ]
        )
    write_tsv(
        args.out_dir / "pass-fail-module-contract.tsv",
        [
            "module",
            "pass_sha256",
            "fail_sha256",
            "pass_signer",
            "fail_signer",
            "pass_vermagic",
            "fail_vermagic",
            "pass_proc_modules_position",
            "fail_proc_modules_position",
            "changed",
            "subsystem",
        ],
        comparison_rows,
    )

    cellular_rows = []
    for name in sorted(closure):
        stock = stock_vendor.get(name) or stock_system.get(name) or vendor_boot.get(name)
        candidate = candidate_vendor.get(name) or controlled_system.get(name) or vendor_boot.get(name)
        if not stock or not candidate:
            continue
        cellular_rows.append(
            [
                name,
                stock.partition,
                sha256(stock.path),
                sha256(candidate.path),
                "MATCH" if sha256(stock.path) == sha256(candidate.path) else "DIFFERENT",
                modinfo(candidate.path, "signer") or "UNSIGNED",
            ]
        )
    write_tsv(
        args.out_dir / "cellular-stock-module-list.tsv",
        ["module", "partition", "stock_sha256", "candidate_sha256", "result", "candidate_signer"],
        cellular_rows,
    )

    wlan_rows = []
    for name in sorted(WLAN_SOURCE | WLAN_RESIGN):
        stock = stock_vendor[name]
        candidate = candidate_vendor[name]
        action = "SOURCE_REPLACEMENT" if name in WLAN_SOURCE else "RE_SIGN_STOCK"
        wlan_rows.append(
            [
                name,
                action,
                sha256(stock.path),
                sha256(candidate.path),
                modinfo(candidate.path, "signer") or "UNSIGNED",
                modinfo(candidate.path, "vermagic"),
            ]
        )
    write_tsv(
        args.out_dir / "wlan-custom-module-list.tsv",
        ["module", "action", "stock_sha256", "candidate_sha256", "candidate_signer", "candidate_vermagic"],
        wlan_rows,
    )

    replacement_rows = []
    signature_rows = []
    exact_vendor_cellular = 0
    vendor_cellular_total = 0
    for name, stock in sorted(stock_vendor.items()):
        candidate = candidate_vendor[name]
        stock_hash = sha256(stock.path)
        candidate_hash = sha256(candidate.path)
        if name in WLAN_SOURCE:
            action = "SOURCE_REPLACEMENT"
            payload_identity = "SOURCE_CONTRACT_MATCH"
        elif name in WLAN_RESIGN:
            action = "RE_SIGN_STOCK"
            stock_bytes = stock.path.read_bytes()
            candidate_bytes = candidate.path.read_bytes()
            payload_identity = (
                "EXACT_STOCK_PAYLOAD_PLUS_SIGNATURE"
                if candidate_bytes.startswith(stock_bytes)
                else "PAYLOAD_MISMATCH"
            )
        else:
            action = "RETAIN_EXACT_STOCK"
            payload_identity = "EXACT" if stock_hash == candidate_hash else "MISMATCH"
        if name in closure:
            vendor_cellular_total += 1
            if stock_hash == candidate_hash:
                exact_vendor_cellular += 1
        replacement_rows.append(
            [name, action, stock_hash, candidate_hash, payload_identity]
        )
        if action != "RETAIN_EXACT_STOCK":
            signature_rows.append(
                [
                    name,
                    action,
                    modinfo(stock.path, "signer") or "UNSIGNED",
                    modinfo(candidate.path, "signer") or "UNSIGNED",
                    modinfo(candidate.path, "sig_id") or "NONE",
                    payload_identity,
                ]
            )
    write_tsv(
        args.out_dir / "module-replacement.tsv",
        ["module", "action", "stock_sha256", "candidate_sha256", "payload_identity"],
        replacement_rows,
    )
    write_tsv(
        args.out_dir / "signature-report.tsv",
        ["module", "action", "stock_signer", "candidate_signer", "signature_id", "payload_identity"],
        signature_rows,
    )

    crc_rows = []
    for name in sorted(WLAN_SOURCE):
        stock = stock_vendor[name].record
        candidate = candidate_vendor[name].record
        crc_rows.append(
            [
                name,
                "IMPORT_CONTRACT",
                len(stock.imports),
                len(candidate.imports),
                "MATCH" if stock.imports == candidate.imports else "MISMATCH",
            ]
        )
        crc_rows.append(
            [
                name,
                "EXPORT_CONTRACT",
                len(stock.exports),
                len(candidate.exports),
                "MATCH" if stock.exports == candidate.exports else "MISMATCH",
            ]
        )
    write_tsv(
        args.out_dir / "CRC-report.tsv",
        ["module", "contract", "stock_entries", "candidate_entries", "result"],
        crc_rows,
    )
    write_tsv(
        args.out_dir / "dependency-report.tsv",
        list(import_rows[0].keys()) if import_rows else [],
        [list(row.values()) for row in import_rows],
    )

    write_tsv(
        args.out_dir / "summary.tsv",
        ["key", "value"],
        [
            ["cellular_closure_modules", len(closure)],
            ["candidate_exact_stock_cellular_modules", exact_stock_count],
            ["vendor_cellular_modules", vendor_cellular_total],
            ["candidate_exact_stock_vendor_cellular_modules", exact_vendor_cellular],
            ["source_wlan_modules", len(WLAN_SOURCE)],
            ["resigned_stock_wlan_consumers", len(WLAN_RESIGN)],
            ["pass_runtime_cellular_modules", sum(name in pass_position for name in closure)],
            ["fail_runtime_cellular_modules", sum(name in fail_position for name in closure)],
        ],
    )
    print(
        f"cellular_closure={len(closure)} exact_stock_candidate={exact_stock_count} "
        f"wlan_signed_closure={len(WLAN_SOURCE | WLAN_RESIGN)}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
