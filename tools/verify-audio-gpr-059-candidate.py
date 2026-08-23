#!/usr/bin/env python3
"""Verify the final Audio .059 GPR candidate without mutating its inputs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile


SIGNATURE_MAGIC = b"~Module signature appended~\n"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module_field(path: Path, field: str) -> str:
    return subprocess.check_output(
        ["modinfo", "-F", field, str(path)], text=True
    ).strip()


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
    if header < 0:
        raise SystemExit("truncated module-signature trailer")
    _algo, _hash, id_type, signer_len, key_len, _pad, sig_len = struct.unpack(
        ">5B3sI", data[header : header + 12]
    )
    payload_end = header - signer_len - key_len - sig_len
    if id_type != 2 or payload_end < 0:
        raise SystemExit("invalid PKCS#7 module-signature trailer")
    return data[:payload_end]


def load_validator(path: Path):
    spec = importlib.util.spec_from_file_location("audio_contract_validator", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load validator: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def allocated_section_contract(path: Path) -> dict[str, tuple[str, int, str]]:
    output = subprocess.check_output(["llvm-readelf", "-SW", str(path)], text=True)
    sections: dict[str, tuple[str, int, str]] = {}
    pattern = re.compile(
        r"^\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+[0-9A-Fa-f]+\s+"
        r"[0-9A-Fa-f]+\s+([0-9A-Fa-f]+)\s+\S+\s+([A-Z]*)\s+"
    )
    for line in output.splitlines():
        match = pattern.match(line)
        if not match or "A" not in match.group(4):
            continue
        name, section_type = match.group(1), match.group(2)
        size = int(match.group(3), 16)
        if section_type == "NOBITS":
            digest = "NOBITS"
        else:
            data = subprocess.check_output(
                [
                    "llvm-objcopy",
                    f"--dump-section={name}=/dev/stdout",
                    str(path),
                    "/dev/null",
                ]
            )
            digest = hashlib.sha256(data).hexdigest()
        sections[name] = (section_type, size, digest)
    return sections


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
    source = {name for name, action in closure.items() if action == "SOURCE_REPLACEMENT"}
    resign = {name for name, action in closure.items() if action == "RE_SIGN_STOCK"}
    if source != {"gpr_dlkm"} or len(resign) != 22 or len(closure) != 23:
        raise SystemExit("protected-signing closure differs from reviewed Audio closure")
    geometry_stripped = {"wcd9378_dlkm", "wcd939x_dlkm"}
    if not geometry_stripped <= resign:
        raise SystemExit("reviewed symbol-table stripping set left the re-sign closure")
    validator = load_validator(args.validator)

    changed = {
        name for name in baseline if sha256(baseline[name]) != sha256(candidate[name])
    }
    if changed != set(closure):
        unexpected = sorted(changed.symmetric_difference(closure))
        raise SystemExit(f"unexpected vendor module delta: {unexpected}")

    replacement_bytes = args.replacement.read_bytes()
    if unsigned_payload(candidate["gpr_dlkm"].read_bytes()) != replacement_bytes:
        raise SystemExit("staged GPR pre-signature payload differs from build output")
    for name in resign:
        candidate_payload = unsigned_payload(candidate[name].read_bytes())
        if name not in geometry_stripped:
            if candidate_payload != baseline[name].read_bytes():
                raise SystemExit(f"{name}: exact-stock pre-signature payload changed")
            continue
        with tempfile.TemporaryDirectory(prefix="audio-elf-contract-") as temporary:
            expected = Path(temporary) / f"{name}.ko"
            shutil.copyfile(baseline[name], expected)
            subprocess.run(
                [
                    "llvm-strip",
                    "--strip-unneeded",
                    "--wildcard",
                    "--keep-symbol=__ksymtab_*",
                    str(expected),
                ],
                check=True,
            )
            if candidate_payload != expected.read_bytes():
                raise SystemExit(f"{name}: pre-signature payload is not the reviewed strip transformation")
            old_record = validator.module_record(
                baseline[name], "vendor_dlkm", "BASELINE"
            )
            stripped_record = validator.module_record(
                expected, "vendor_dlkm", "STRIP_UNNEEDED"
            )
            if old_record.imports != stripped_record.imports or old_record.exports != stripped_record.exports:
                raise SystemExit(f"{name}: stripping changed imports, exports, or CRCs")
            if allocated_section_contract(baseline[name]) != allocated_section_contract(expected):
                raise SystemExit(f"{name}: stripping changed an allocated ELF section")
            for field in ("name", "vermagic", "depends", "alias", "parm", "firmware"):
                if module_field(baseline[name], field) != module_field(expected, field):
                    raise SystemExit(f"{name}: stripping changed modinfo field {field}")

    signature_rows: list[list[object]] = []
    replacement_rows: list[list[object]] = []
    preservation_rows: list[list[object]] = []
    for name in sorted(baseline):
        old_sha = sha256(baseline[name])
        new_sha = sha256(candidate[name])
        if name == "gpr_dlkm":
            reason = "Audio .059 GPR teardown work cancellation"
            source_upgrade, rebuild, resign_only = "yes", "yes", "no"
        elif name in resign:
            reason = (
                "controlled protected-export signing closure; non-allocating symbols stripped for fixed geometry"
                if name in geometry_stripped
                else "controlled protected-export signing closure"
            )
            source_upgrade, rebuild, resign_only = "no", "no", "yes"
        else:
            reason = "retained byte-identical"
            source_upgrade, rebuild, resign_only = "no", "no", "no"
        preservation_rows.append(
            [name, old_sha, new_sha, reason, "PASS" if old_sha == new_sha or name in closure else "FAIL"]
        )
        if name not in closure:
            continue
        signer = module_field(candidate[name], "signer")
        sig_id = module_field(candidate[name], "sig_id")
        vermagic = module_field(candidate[name], "vermagic")
        expected_vermagic = (
            vermagic.startswith(args.expected_release + " ")
            if name == "gpr_dlkm"
            else vermagic == module_field(baseline[name], "vermagic")
        )
        signature_ok = (
            signer == args.expected_signer
            and sig_id == "PKCS#7"
            and expected_vermagic
        )
        signature_rows.append(
            [name, closure[name], vermagic, signer, sig_id, "PASS" if signature_ok else "FAIL"]
        )
        if not signature_ok:
            raise SystemExit(f"{name}: signature or vermagic contract failed")
        replacement_rows.append(
            [
                name,
                old_sha,
                new_sha,
                reason,
                source_upgrade,
                rebuild,
                resign_only,
            ]
        )

    old_record = validator.module_record(
        baseline["gpr_dlkm"], "vendor_dlkm", "BASELINE"
    )
    new_record = validator.module_record(
        args.replacement, "vendor_dlkm", "SOURCE_REPLACEMENT"
    )
    if old_record.exports != new_record.exports:
        raise SystemExit("GPR export set or CRCs changed")
    removed_imports = set(old_record.imports) - set(new_record.imports)
    added_imports = set(new_record.imports) - set(old_record.imports)
    if removed_imports or added_imports != {"cancel_work_sync"}:
        raise SystemExit(
            f"unreviewed GPR import delta: added={sorted(added_imports)} "
            f"removed={sorted(removed_imports)}"
        )
    if validator.crc(new_record.imports["cancel_work_sync"]) != "0x35480a5b":
        raise SystemExit("cancel_work_sync CRC differs from reviewed kernel contract")

    with args.import_resolution.open(encoding="utf-8") as stream:
        import_rows = list(csv.DictReader(stream, delimiter="\t"))
    selected_import_rows = [
        list(row.values())
        for row in import_rows
        if row["consumer"] == "gpr_dlkm"
        or "vendor_dlkm:gpr_dlkm" in row["candidate_providers"]
    ]
    if not selected_import_rows or any(row[-1] != "MATCH" for row in selected_import_rows):
        raise SystemExit("GPR import/consumer CRC report is incomplete or failing")

    args.out_dir.mkdir(parents=True, exist_ok=True)
    write_tsv(
        args.out_dir / "audio-replacement-manifest.tsv",
        [
            "module",
            "baseline_sha256",
            "candidate_sha256",
            "reason_changed",
            "source_upgrade",
            "rebuild",
            "re_sign_only",
        ],
        replacement_rows,
    )
    write_tsv(
        args.out_dir / "audio-signature-report.tsv",
        ["module", "action", "vermagic", "signer", "sig_id", "result"],
        signature_rows,
    )
    write_tsv(
        args.out_dir / "audio-preservation-report.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "action", "result"],
        preservation_rows,
    )
    write_tsv(
        args.out_dir / "audio-gpr-import-crc.tsv",
        list(import_rows[0].keys()),
        selected_import_rows,
    )
    contract_rows: list[list[object]] = []
    for symbol in sorted(old_record.exports):
        crc = validator.crc(old_record.exports[symbol])
        contract_rows.append(["EXPORT", symbol, crc, crc, "UNCHANGED"])
    for symbol in sorted(set(old_record.imports) | set(new_record.imports)):
        old_crc = validator.crc(old_record.imports.get(symbol))
        new_crc = validator.crc(new_record.imports.get(symbol))
        result = "ADDED_REVIEWED" if symbol == "cancel_work_sync" else "UNCHANGED"
        contract_rows.append(["IMPORT", symbol, old_crc, new_crc, result])
    write_tsv(
        args.out_dir / "audio-gpr-elf-contract.tsv",
        ["contract", "symbol", "baseline_crc", "candidate_crc", "result"],
        contract_rows,
    )
    module_contract_rows = []
    for field in ("alias", "parm", "firmware"):
        old_value = module_field(baseline["gpr_dlkm"], field)
        new_value = module_field(args.replacement, field)
        if old_value != new_value:
            raise SystemExit(f"GPR {field} contract changed")
        module_contract_rows.append([field, old_value, new_value, "UNCHANGED"])
    write_tsv(
        args.out_dir / "audio-gpr-module-contract.tsv",
        ["field", "baseline", "candidate", "result"],
        module_contract_rows,
    )

    summary = {
        "result": "PASS",
        "vendor_modules": 436,
        "source_replacements": 1,
        "re_sign_exact_stock": 22,
        "re_sign_strip_unneeded_nonallocating_only": sorted(geometry_stripped),
        "unexpected_module_changes": 0,
        "gpr_export_changes": 0,
        "gpr_import_additions": ["cancel_work_sync"],
        "gpr_import_removals": [],
        "unresolved_imports": 0,
        "crc_mismatches": 0,
        "protected_export_failures": 0,
        "signature_failures": 0,
    }
    (args.out_dir / "audio-candidate-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
