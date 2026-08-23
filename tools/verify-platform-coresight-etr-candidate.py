#!/usr/bin/env python3
"""Verify the bounded Platform CoreSight ETR candidate without mutating inputs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
from pathlib import Path
import re
import struct
import subprocess
import sys


SIGNATURE_MAGIC = b"~Module signature appended~\n"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module_field(path: Path, field: str) -> str:
    return subprocess.check_output(["modinfo", "-F", field, str(path)], text=True).strip()


def scan_modules(root: Path) -> dict[str, Path]:
    modules: dict[str, Path] = {}
    for path in sorted(root.rglob("*.ko")):
        name = module_field(path, "name")
        if name in modules:
            raise SystemExit(f"duplicate module name: {name}")
        modules[name] = path
    return modules


def unsigned_payload(data: bytes) -> bytes:
    if not data.endswith(SIGNATURE_MAGIC):
        return data
    header = len(data) - len(SIGNATURE_MAGIC) - 12
    _algo, _hash, id_type, signer_len, key_len, _pad, sig_len = struct.unpack(
        ">5B3sI", data[header : header + 12]
    )
    payload_end = header - signer_len - key_len - sig_len
    if id_type != 2 or payload_end < 0:
        raise SystemExit("invalid PKCS#7 module-signature trailer")
    return data[:payload_end]


def load_validator(path: Path):
    spec = importlib.util.spec_from_file_location("platform_contract_validator", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load validator: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validator", required=True, type=Path)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--candidate-root", required=True, type=Path)
    parser.add_argument("--replacement", required=True, type=Path)
    parser.add_argument("--closure", required=True, type=Path)
    parser.add_argument("--import-resolution", required=True, type=Path)
    parser.add_argument("--expected-signer", required=True)
    parser.add_argument("--expected-release", required=True)
    parser.add_argument("--out-dir", required=True, type=Path)
    args = parser.parse_args()

    baseline = scan_modules(args.baseline_root)
    candidate = scan_modules(args.candidate_root)
    if len(baseline) != 436 or set(baseline) != set(candidate):
        raise SystemExit("candidate does not preserve the 436-module inventory")

    with args.closure.open(encoding="utf-8") as stream:
        closure_rows = list(csv.DictReader(stream, delimiter="\t"))
    closure = {row["module"]: row["required_action"] for row in closure_rows}
    if closure != {"coresight_tmc": "SOURCE_REPLACEMENT"}:
        raise SystemExit(f"protected-signing closure differs from reviewed closure: {closure}")

    changed = {name for name in baseline if sha256(baseline[name]) != sha256(candidate[name])}
    if changed != {"coresight_tmc"}:
        raise SystemExit(f"unexpected vendor module delta: {sorted(changed)}")
    if unsigned_payload(candidate["coresight_tmc"].read_bytes()) != args.replacement.read_bytes():
        raise SystemExit("staged CoreSight TMC pre-signature payload differs from build output")

    validator = load_validator(args.validator)
    old_record = validator.module_record(baseline["coresight_tmc"], "vendor_dlkm", "BASELINE")
    new_record = validator.module_record(args.replacement, "vendor_dlkm", "SOURCE_REPLACEMENT")
    if old_record.exports != new_record.exports:
        raise SystemExit("CoreSight TMC export set or CRCs changed")
    if old_record.imports != new_record.imports:
        raise SystemExit("CoreSight TMC import set or CRCs changed")

    old_disassembly = subprocess.check_output(
        ["llvm-objdump", "-dr", "--disassemble-symbols=tmc_etr_get_sysfs_buffer", str(baseline["coresight_tmc"])],
        text=True,
    )
    new_disassembly = subprocess.check_output(
        ["llvm-objdump", "-dr", "--disassemble-symbols=tmc_etr_get_sysfs_buffer", str(args.replacement)],
        text=True,
    )
    pattern = re.compile(r"R_AARCH64_CALL26\s+coresight_get_mode")
    if len(pattern.findall(old_disassembly)) != 1 or len(pattern.findall(new_disassembly)) != 2:
        raise SystemExit("CoreSight ETR machine-code guard call count is not 1 -> 2")

    signer = module_field(candidate["coresight_tmc"], "signer")
    sig_id = module_field(candidate["coresight_tmc"], "sig_id")
    vermagic = module_field(candidate["coresight_tmc"], "vermagic")
    if signer != args.expected_signer or sig_id != "PKCS#7":
        raise SystemExit("CoreSight TMC controlled signature contract failed")
    if not vermagic.startswith(args.expected_release + " "):
        raise SystemExit("CoreSight TMC vermagic does not inherit the frozen release")

    with args.import_resolution.open(encoding="utf-8") as stream:
        import_rows = list(csv.DictReader(stream, delimiter="\t"))
    selected_import_rows = [row for row in import_rows if row["consumer"] == "coresight_tmc"]
    if not selected_import_rows or any(row["status"] != "MATCH" for row in selected_import_rows):
        raise SystemExit("CoreSight TMC import/CRC report is incomplete or failing")

    replacement_rows = [[
        "coresight_tmc", sha256(baseline["coresight_tmc"]), sha256(candidate["coresight_tmc"]),
        "Platform .099.086 active ETR SYSFS buffer UAF guard", "yes", "yes", "no",
    ]]
    preservation_rows = []
    for name in sorted(baseline):
        old_sha, new_sha = sha256(baseline[name]), sha256(candidate[name])
        reason = "Platform .099.086 CoreSight ETR UAF guard" if name == "coresight_tmc" else "retained byte-identical"
        preservation_rows.append([name, old_sha, new_sha, reason, "PASS"])

    args.out_dir.mkdir(parents=True, exist_ok=True)
    write_tsv(
        args.out_dir / "platform-replacement-manifest.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason_changed", "source_upgrade", "rebuild", "re_sign_only"],
        replacement_rows,
    )
    write_tsv(
        args.out_dir / "platform-signature-report.tsv",
        ["module", "action", "vermagic", "signer", "sig_id", "result"],
        [["coresight_tmc", "SOURCE_REPLACEMENT", vermagic, signer, sig_id, "PASS"]],
    )
    write_tsv(
        args.out_dir / "platform-preservation-report.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason", "result"],
        preservation_rows,
    )
    write_tsv(
        args.out_dir / "platform-import-crc.tsv",
        list(selected_import_rows[0].keys()),
        [list(row.values()) for row in selected_import_rows],
    )
    write_tsv(
        args.out_dir / "platform-module-contract.tsv",
        ["module", "imports", "exports", "import_contract", "export_contract", "structural_contract"],
        [["coresight_tmc", len(new_record.imports), len(new_record.exports), "IDENTICAL", "IDENTICAL", "FUNCTION_BODY_ONLY"]],
    )
    (args.out_dir / "coresight-etr-machine-code.txt").write_text(new_disassembly, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
