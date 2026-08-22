#!/usr/bin/env python3
"""Audit retained stock modules against an exact controlled-v1 Image.

The controlled stack intentionally remains hybrid while source coverage grows.
This tool makes that boundary explicit: it reads stock module trees, indexes
the *candidate* provider trees plus vmlinux Module.symvers, and refuses an
active retained module with an unresolved import, a CRC mismatch, an
incompatible non-release vermagic suffix, an unknown signature, or a protected
provider edge that was not included in the signed replacement closure.

It is read-only with respect to all module inputs.  It writes a concise
``retained-stock-contract.tsv`` and an import-level companion report suitable
for packaging with a controlled-v1 release.
"""

from __future__ import annotations

import argparse
import csv
import importlib.util
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


def load_elf_helpers():
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


def die(message: str) -> None:
    raise ValueError(message)


def split_assignment(value: str, label: str) -> tuple[str, Path]:
    if "=" not in value:
        die(f"{label} must be PARTITION=PATH: {value}")
    partition, raw_path = value.split("=", 1)
    if not partition or not raw_path:
        die(f"{label} must be PARTITION=PATH: {value}")
    path = Path(raw_path).resolve()
    return partition, path


def split_load_assignment(value: str) -> tuple[str, str, Path]:
    if "=" not in value or ":" not in value.split("=", 1)[0]:
        die(f"--load-list must be PARTITION:MODE=PATH: {value}")
    key, raw_path = value.split("=", 1)
    partition, mode = key.split(":", 1)
    if not partition or not mode or not raw_path:
        die(f"--load-list must be PARTITION:MODE=PATH: {value}")
    return partition, mode, Path(raw_path).resolve()


def split_module_assignment(value: str, label: str) -> tuple[str, str]:
    if ":" not in value:
        die(f"{label} must be PARTITION:MODULE: {value}")
    partition, module = value.split(":", 1)
    if not partition or not module:
        die(f"{label} must be PARTITION:MODULE: {value}")
    return partition, module.removesuffix(".ko")


def sanitize(value: object) -> str:
    return str(value).replace("\t", " ").replace("\n", " ").strip()


def modinfo(path: Path, field: str) -> str:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return ",".join(line for line in result.stdout.splitlines() if line)


def certificate_cn(path: Path) -> str:
    result = subprocess.run(
        ["openssl", "x509", "-in", str(path), "-noout", "-subject", "-nameopt", "compat"],
        check=True,
        capture_output=True,
        text=True,
    )
    prefix = "subject=/CN="
    subject = result.stdout.strip()
    if not subject.startswith(prefix):
        die(f"could not derive common name from certificate: {path}")
    return subject[len(prefix) :].split("/", 1)[0]


def parse_load_list(path: Path) -> set[str]:
    if not path.is_file():
        die(f"missing modules.load input: {path}")
    entries: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entries.add(Path(line).name.removesuffix(".ko"))
    return entries


def read_controlled_modules(value: str) -> dict[str, set[str]]:
    """Parse PARTITION=protected-export-signing-closure.tsv."""

    partition, path = split_assignment(value, "--controlled-module-list")
    if not path.is_file():
        die(f"missing controlled-module list: {path}")
    modules: set[str] = set()
    with path.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source, delimiter="\t")
        if not reader.fieldnames or "module" not in reader.fieldnames:
            die(f"controlled-module list has no module column: {path}")
        for row in reader:
            name = (row.get("module") or "").strip()
            if name:
                modules.add(name)
    return {partition: modules}


def add_records(
    provider_index: dict[str, list[tuple[str, str, int, bool]]],
    partition: str,
    root: Path,
    controlled: set[str],
) -> None:
    if not root.is_dir():
        die(f"candidate provider root is not a directory: {root}")
    records = ELF.scan_root(root, partition, "CANDIDATE_PROVIDER")
    for record in records.values():
        for symbol, value in record.exports.items():
            provider_index[symbol].append(
                (partition, record.name, value, record.name in controlled)
            )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vmlinux-symvers", type=Path, required=True)
    parser.add_argument("--kernel-release", required=True)
    parser.add_argument("--expected-vermagic", required=True)
    parser.add_argument("--stock-cert", type=Path, required=True)
    parser.add_argument("--project-cert", type=Path, required=True)
    parser.add_argument(
        "--stock-root",
        action="append",
        default=[],
        metavar="PARTITION=PATH",
        help="read-only stock tree to classify; may be repeated",
    )
    parser.add_argument(
        "--candidate-root",
        action="append",
        default=[],
        metavar="PARTITION=PATH",
        help="candidate module tree used as a provider map; may be repeated",
    )
    parser.add_argument(
        "--load-list",
        action="append",
        default=[],
        metavar="PARTITION:MODE=PATH",
        help="a supported modules.load list; may be repeated",
    )
    parser.add_argument(
        "--controlled-module-list",
        action="append",
        default=[],
        metavar="PARTITION=TSV",
        help="signed source/re-sign closure; these modules are not retained stock",
    )
    parser.add_argument(
        "--fully-controlled-partition",
        action="append",
        default=[],
        metavar="PARTITION",
        help="partition whose candidate contains no retained stock binaries; may be repeated",
    )
    parser.add_argument(
        "--forbidden-load",
        action="append",
        default=[],
        metavar="PARTITION:MODULE",
        help="a retained module intentionally prohibited from supported load lists",
    )
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--dependency-out", type=Path, required=True)
    parser.add_argument("--summary-out", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not args.vmlinux_symvers.is_file():
        die(f"missing exact Image Module.symvers: {args.vmlinux_symvers}")
    if not args.stock_cert.is_file() or not args.project_cert.is_file():
        die("stock and project public certificates are required")
    if not args.stock_root or not args.candidate_root:
        die("at least one --stock-root and --candidate-root are required")
    if args.out.exists() or args.dependency_out.exists() or args.summary_out.exists():
        die("refusing to overwrite retained-stock contract output")

    stock_roots = dict(split_assignment(value, "--stock-root") for value in args.stock_root)
    candidate_roots = dict(split_assignment(value, "--candidate-root") for value in args.candidate_root)
    if set(stock_roots) - set(candidate_roots):
        die("every stock partition needs a matching candidate provider root")
    for partition, root in stock_roots.items():
        if not root.is_dir():
            die(f"stock module root is not a directory: {partition}={root}")

    controlled: dict[str, set[str]] = defaultdict(set)
    for value in args.controlled_module_list:
        for partition, names in read_controlled_modules(value).items():
            controlled[partition].update(names)
    for partition in args.fully_controlled_partition:
        if partition not in stock_roots:
            die(f"fully controlled partition has no stock root: {partition}")
        records = ELF.scan_root(stock_roots[partition], partition, "STOCK_INVENTORY")
        controlled[partition].update(records)

    load_modes: dict[str, dict[str, set[str]]] = defaultdict(lambda: defaultdict(set))
    for value in args.load_list:
        partition, mode, path = split_load_assignment(value)
        for module in parse_load_list(path):
            load_modes[partition][module].add(mode)

    forbidden = {split_module_assignment(value, "--forbidden-load") for value in args.forbidden_load}
    for partition, module in forbidden:
        if load_modes.get(partition, {}).get(module):
            die(f"forbidden dormant module is present in a supported load list: {partition}:{module}")

    stock_signer = certificate_cn(args.stock_cert)
    project_signer = certificate_cn(args.project_cert)
    expected_tail = args.expected_vermagic.partition(" ")[2]
    if not expected_tail:
        die("expected vermagic must include a release and feature suffix")

    providers: dict[str, list[tuple[str, str, int, bool]]] = defaultdict(list)
    for symbol, values in ELF.parse_symvers(args.vmlinux_symvers).items():
        for source, value in values:
            providers[symbol].append(("vmlinux", source, value, False))
    for partition, root in candidate_roots.items():
        add_records(providers, partition, root, controlled[partition])

    rows: list[dict[str, str]] = []
    dependency_rows: list[dict[str, str]] = []
    totals = defaultdict(int)

    for partition, root in sorted(stock_roots.items()):
        records = ELF.scan_root(root, partition, "RETAINED_STOCK")
        for record in sorted(records.values(), key=lambda item: item.name):
            if record.name in controlled[partition]:
                continue
            modes = sorted(load_modes.get(partition, {}).get(record.name, set()))
            load_mode = "+".join(modes) if modes else "DORMANT"
            active = bool(modes)
            signer = modinfo(record.path, "signer") or "UNSIGNED"
            sig_id = modinfo(record.path, "sig_id") or "NONE"
            vermagic = modinfo(record.path, "vermagic")
            release_token, separator, tail = vermagic.partition(" ")
            has_crcs = "module_layout" in record.imports
            if vermagic == args.expected_vermagic:
                vermagic_contract = "MATCH"
            elif has_crcs and separator and tail == expected_tail:
                vermagic_contract = "MODVERSIONS_RELEASE_TOKEN_DIFFERENT"
            else:
                vermagic_contract = "INCOMPATIBLE"

            if signer == "UNSIGNED":
                signing_contract = "UNSIGNED_RETAINED"
            elif signer == stock_signer:
                signing_contract = "STOCK_TRUSTED"
            elif signer == project_signer:
                signing_contract = "PROJECT_TRUSTED"
            else:
                signing_contract = "UNKNOWN_SIGNER"

            unresolved = 0
            mismatches = 0
            protected_failures = 0
            for symbol, expected_crc in sorted(record.imports.items()):
                candidates = providers.get(symbol, [])
                matching = [entry for entry in candidates if entry[2] == expected_crc]
                if matching:
                    result = "MATCH"
                elif candidates:
                    result = "CRC_MISMATCH"
                    mismatches += 1
                else:
                    result = "UNRESOLVED"
                    unresolved += 1
                # A symbol is a protected-provider edge only when every
                # matching runtime provider is in the controlled signing
                # closure.  Hybrid vendor_dlkm deliberately retains exact
                # stock providers for some duplicate GKI module names (for
                # example zsmalloc); a parallel controlled system_dlkm copy
                # must not turn that valid stock-to-stock dependency into a
                # false protected-export failure.
                controlled_provider = bool(matching) and all(
                    entry[3] for entry in matching
                )
                if controlled_provider:
                    protected_failures += 1
                dependency_rows.append(
                    {
                        "partition": partition,
                        "consumer": record.name,
                        "load_mode": load_mode,
                        "symbol": symbol,
                        "expected_crc": f"0x{expected_crc:08x}",
                        "matching_provider": ",".join(
                            sorted(f"{entry[0]}:{entry[1]}" for entry in matching)
                        ),
                        "candidate_crcs": ",".join(
                            sorted({f"0x{entry[2]:08x}" for entry in candidates})
                        ),
                        "result": result,
                        "controlled_provider": "yes" if controlled_provider else "no",
                    }
                )

            if not active:
                classification = "DORMANT"
            elif (
                unresolved
                or mismatches
                or protected_failures
                or vermagic_contract == "INCOMPATIBLE"
                or signing_contract == "UNKNOWN_SIGNER"
            ):
                classification = "BLOCKER"
            else:
                classification = "COMPATIBLE"

            totals[classification] += 1
            rows.append(
                {
                    "module": record.name,
                    "partition": partition,
                    "stock_path": str(record.path.relative_to(root)),
                    "load_mode": load_mode,
                    "classification": classification,
                    "vermagic": vermagic,
                    "vermagic_contract": vermagic_contract,
                    "signature": signer,
                    "signature_id": sig_id,
                    "signing_contract": signing_contract,
                    "imports": str(len(record.imports)),
                    "unresolved_imports": str(unresolved),
                    "crc_mismatches": str(mismatches),
                    "protected_export_failures": str(protected_failures),
                    "release_token": release_token,
                    "reason": (
                        "not in a supported modules.load path"
                        if not active
                        else "all imports and CRCs resolve against the controlled candidate"
                        if classification == "COMPATIBLE"
                        else "active retained module violates the controlled Image contract"
                    ),
                }
            )

    header = [
        "module", "partition", "stock_path", "load_mode", "classification", "vermagic",
        "vermagic_contract", "signature", "signature_id", "signing_contract", "imports",
        "unresolved_imports", "crc_mismatches", "protected_export_failures", "release_token",
        "reason",
    ]
    dependency_header = [
        "partition", "consumer", "load_mode", "symbol", "expected_crc", "matching_provider",
        "candidate_crcs", "result", "controlled_provider",
    ]
    for path in (args.out, args.dependency_out, args.summary_out):
        path.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=header, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    with args.dependency_out.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=dependency_header, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(dependency_rows)
    args.summary_out.write_text(
        "\n".join(
            [
                "RETAINED STOCK MODULE CONTRACT",
                f"kernel_release={args.kernel_release}",
                f"expected_vermagic={args.expected_vermagic}",
                f"compatible={totals['COMPATIBLE']}",
                f"dormant={totals['DORMANT']}",
                f"blocker={totals['BLOCKER']}",
                f"result={'PASS' if not totals['BLOCKER'] else 'FAIL'}",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    print(args.summary_out.read_text(encoding="utf-8"), end="")
    return 0 if not totals["BLOCKER"] else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
