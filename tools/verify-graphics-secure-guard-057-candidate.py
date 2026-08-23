#!/usr/bin/env python3
"""Verify the bounded Graphics secure-guard candidate without mutating inputs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
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
    spec = importlib.util.spec_from_file_location("graphics_contract_validator", path)
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
                ["llvm-objcopy", f"--dump-section={name}=/dev/stdout", str(path), "/dev/null"]
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
    if closure != {
        "msm_kgsl": "SOURCE_REPLACEMENT",
        "oplus_bsp_geas_system": "RE_SIGN_STOCK",
    }:
        raise SystemExit("protected-signing closure differs from reviewed Graphics closure")

    changed = {name for name in baseline if sha256(baseline[name]) != sha256(candidate[name])}
    if changed != set(closure):
        raise SystemExit(f"unexpected vendor module delta: {sorted(changed.symmetric_difference(closure))}")

    if unsigned_payload(candidate["msm_kgsl"].read_bytes()) != args.replacement.read_bytes():
        raise SystemExit("staged KGSL pre-signature payload differs from build output")

    validator = load_validator(args.validator)
    with tempfile.TemporaryDirectory(prefix="graphics-geas-contract-") as temporary:
        expected_geas = Path(temporary) / "oplus_bsp_geas_system.ko"
        shutil.copyfile(baseline["oplus_bsp_geas_system"], expected_geas)
        subprocess.run(
            [
                "llvm-strip", "--strip-unneeded", "--wildcard",
                "--keep-symbol=__ksymtab_*", str(expected_geas),
            ],
            check=True,
        )
        if unsigned_payload(candidate["oplus_bsp_geas_system"].read_bytes()) != expected_geas.read_bytes():
            raise SystemExit("GEAS pre-signature payload is not the reviewed strip transformation")
        old_geas = validator.module_record(
            baseline["oplus_bsp_geas_system"], "vendor_dlkm", "BASELINE"
        )
        stripped_geas = validator.module_record(
            expected_geas, "vendor_dlkm", "STRIP_UNNEEDED"
        )
        if old_geas.imports != stripped_geas.imports or old_geas.exports != stripped_geas.exports:
            raise SystemExit("GEAS stripping changed imports, exports, or CRCs")
        if allocated_section_contract(baseline["oplus_bsp_geas_system"]) != allocated_section_contract(expected_geas):
            raise SystemExit("GEAS stripping changed an allocated ELF section")
        for field in ("name", "vermagic", "depends", "alias", "parm", "firmware"):
            if module_field(baseline["oplus_bsp_geas_system"], field) != module_field(expected_geas, field):
                raise SystemExit(f"GEAS stripping changed modinfo field {field}")

    old_record = validator.module_record(baseline["msm_kgsl"], "vendor_dlkm", "BASELINE")
    new_record = validator.module_record(args.replacement, "vendor_dlkm", "SOURCE_REPLACEMENT")
    if old_record.exports != new_record.exports:
        raise SystemExit("KGSL export set or CRCs changed")
    if old_record.imports != new_record.imports:
        raise SystemExit("KGSL import set or CRCs changed")

    disassembly = subprocess.check_output(
        ["llvm-objdump", "-dr", "--disassemble-symbols=kgsl_free_secure_page", str(args.replacement)],
        text=True,
    )
    for evidence in ("kgsl_free_secure_page", "cbnz", "__free_pages", "_printk"):
        if evidence not in disassembly:
            raise SystemExit(f"KGSL machine-code proof missing {evidence}")
    if disassembly.index("cbnz") > disassembly.index("__free_pages"):
        raise SystemExit("KGSL failure branch does not guard page free")

    signature_rows: list[list[object]] = []
    replacement_rows: list[list[object]] = []
    preservation_rows: list[list[object]] = []
    for name in sorted(baseline):
        old_sha = sha256(baseline[name])
        new_sha = sha256(candidate[name])
        if name == "msm_kgsl":
            reason = "Graphics .057 secure-guard unlock failure ownership fix"
            source_upgrade, rebuild, resign_only = "yes", "yes", "no"
        elif name == "oplus_bsp_geas_system":
            reason = "controlled protected-export signing closure; non-allocating symbols stripped for fixed geometry"
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
        vermagic_ok = (
            vermagic.startswith(args.expected_release + " ")
            if name == "msm_kgsl"
            else vermagic == module_field(baseline[name], "vermagic")
        )
        result = "PASS" if signer == args.expected_signer and sig_id == "PKCS#7" and vermagic_ok else "FAIL"
        if result != "PASS":
            raise SystemExit(f"{name}: signature or vermagic contract failed")
        signature_rows.append([name, closure[name], vermagic, signer, sig_id, result])
        replacement_rows.append(
            [name, old_sha, new_sha, reason, source_upgrade, rebuild, resign_only]
        )

    with args.import_resolution.open(encoding="utf-8") as stream:
        import_rows = list(csv.DictReader(stream, delimiter="\t"))
    selected_import_rows = [
        row for row in import_rows
        if row["consumer"] in closure or "vendor_dlkm:msm_kgsl" in row["candidate_providers"]
    ]
    if not selected_import_rows or any(row["status"] != "MATCH" for row in selected_import_rows):
        raise SystemExit("Graphics import/consumer CRC report is incomplete or failing")

    args.out_dir.mkdir(parents=True, exist_ok=True)
    write_tsv(
        args.out_dir / "graphics-replacement-manifest.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason_changed", "source_upgrade", "rebuild", "re_sign_only"],
        replacement_rows,
    )
    write_tsv(
        args.out_dir / "graphics-signature-report.tsv",
        ["module", "action", "vermagic", "signer", "sig_id", "result"],
        signature_rows,
    )
    write_tsv(
        args.out_dir / "graphics-preservation-report.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason", "result"],
        preservation_rows,
    )
    write_tsv(
        args.out_dir / "graphics-import-crc.tsv",
        list(selected_import_rows[0].keys()),
        [list(row.values()) for row in selected_import_rows],
    )
    write_tsv(
        args.out_dir / "graphics-module-contract.tsv",
        ["module", "imports", "exports", "import_contract", "export_contract", "structural_contract"],
        [
            ["msm_kgsl", len(new_record.imports), len(new_record.exports), "IDENTICAL", "IDENTICAL", "FUNCTION_BODY_ONLY"],
            ["oplus_bsp_geas_system", len(old_geas.imports), len(old_geas.exports), "IDENTICAL", "IDENTICAL", "EXACT_ALLOCATED_SECTIONS"],
        ],
    )
    (args.out_dir / "kgsl-secure-guard-disassembly.txt").write_text(disassembly, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
