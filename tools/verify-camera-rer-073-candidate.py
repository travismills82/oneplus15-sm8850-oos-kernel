#!/usr/bin/env python3
"""Verify the bounded Camera RER candidate without mutating its inputs."""

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
    spec = importlib.util.spec_from_file_location("camera_contract_validator", path)
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


def disassembly(path: Path) -> str:
    return subprocess.check_output(
        ["llvm-objdump", "-dr", "--disassemble-symbols=cam_flash_pmic_pkt_parser", str(path)],
        text=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validator", required=True, type=Path)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--candidate-root", required=True, type=Path)
    parser.add_argument("--replacement", required=True, type=Path)
    parser.add_argument("--closure", required=True, type=Path)
    parser.add_argument("--import-resolution", required=True, type=Path)
    parser.add_argument("--external-edges", required=True, type=Path)
    parser.add_argument("--vendor-boot-load", required=True, type=Path)
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
        "camera": "SOURCE_REPLACEMENT",
        "camera_extension": "RE_SIGN_STOCK",
    }:
        raise SystemExit("protected-signing closure differs from reviewed Camera closure")

    changed = {name for name in baseline if sha256(baseline[name]) != sha256(candidate[name])}
    if changed != set(closure):
        raise SystemExit(f"unexpected vendor module delta: {sorted(changed.symmetric_difference(closure))}")
    if unsigned_payload(candidate["camera"].read_bytes()) != args.replacement.read_bytes():
        raise SystemExit("staged Camera pre-signature payload differs from build output")

    validator = load_validator(args.validator)
    old_camera = validator.module_record(baseline["camera"], "vendor_dlkm", "BASELINE")
    new_camera = validator.module_record(args.replacement, "vendor_dlkm", "SOURCE_REPLACEMENT")
    if old_camera.imports != new_camera.imports or old_camera.exports != new_camera.exports:
        raise SystemExit("Camera imports, exports, or CRCs changed")

    with tempfile.TemporaryDirectory(prefix="camera-extension-contract-") as temporary:
        expected_extension = Path(temporary) / "camera_extension.ko"
        shutil.copyfile(baseline["camera_extension"], expected_extension)
        subprocess.run(
            [
                "llvm-strip", "--strip-unneeded", "--wildcard",
                "--keep-symbol=__ksymtab_*", str(expected_extension),
            ],
            check=True,
        )
        if unsigned_payload(candidate["camera_extension"].read_bytes()) != expected_extension.read_bytes():
            raise SystemExit("Camera extension is not the reviewed strip-only controlled re-sign")
        old_extension = validator.module_record(
            baseline["camera_extension"], "vendor_dlkm", "BASELINE"
        )
        stripped_extension = validator.module_record(
            expected_extension, "vendor_dlkm", "STRIP_UNNEEDED"
        )
        if old_extension.imports != stripped_extension.imports or old_extension.exports != stripped_extension.exports:
            raise SystemExit("Camera extension stripping changed imports, exports, or CRCs")
        if allocated_section_contract(baseline["camera_extension"]) != allocated_section_contract(expected_extension):
            raise SystemExit("Camera extension stripping changed an allocated ELF section")
        for field in ("name", "vermagic", "depends", "alias", "parm", "firmware"):
            if module_field(baseline["camera_extension"], field) != module_field(expected_extension, field):
                raise SystemExit(f"Camera extension stripping changed modinfo field {field}")

    signature_rows: list[list[object]] = []
    replacement_rows: list[list[object]] = []
    preservation_rows: list[list[object]] = []
    for name in sorted(baseline):
        old_sha = sha256(baseline[name])
        new_sha = sha256(candidate[name])
        if name == "camera":
            reason = "Camera .073 RER userspace-command snapshot hardening"
            source_upgrade, rebuild, resign_only = "yes", "yes", "no"
        elif name == "camera_extension":
            reason = "controlled protected-export signing closure; non-allocating symbols stripped"
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
            if name == "camera"
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
        if row["consumer"] in closure or "vendor_dlkm:camera" in row["candidate_providers"]
    ]
    if not selected_import_rows or any(row["status"] != "MATCH" for row in selected_import_rows):
        raise SystemExit("Camera import/consumer CRC report is incomplete or failing")

    with args.external_edges.open(encoding="utf-8") as stream:
        external_rows = list(csv.DictReader(stream, delimiter="\t"))
    if len(external_rows) != 49:
        raise SystemExit("unexpected Camera vendor_boot protected-provider boundary")
    if any(row["consumer"] != "camera_extension" or row["new_signed_provider"] != "camera"
           for row in external_rows):
        raise SystemExit("unreviewed external Camera signed-provider edge")
    load_entries = {
        line.strip().removeprefix("/").split("/")[-1]
        for line in args.vendor_boot_load.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }
    if "camera_extension.ko" in load_entries:
        raise SystemExit("vendor_boot Camera extension is not dormant")

    old_disassembly = disassembly(baseline["camera"])
    new_disassembly = disassembly(args.replacement)
    relocation_kdup = "R_AARCH64_CALL26\tcam_common_mem_kdup"
    relocation_free = "R_AARCH64_CALL26\tcam_common_mem_free"
    if old_disassembly.count(relocation_kdup) != 3 or new_disassembly.count(relocation_kdup) != 4:
        raise SystemExit("RER private-snapshot machine-code proof failed")
    if old_disassembly.count(relocation_free) != 4 or new_disassembly.count(relocation_free) != 5:
        raise SystemExit("RER private-snapshot cleanup machine-code proof failed")

    args.out_dir.mkdir(parents=True, exist_ok=True)
    write_tsv(
        args.out_dir / "camera-replacement-manifest.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason_changed", "source_upgrade", "rebuild", "re_sign_only"],
        replacement_rows,
    )
    write_tsv(
        args.out_dir / "camera-signature-report.tsv",
        ["module", "action", "vermagic", "signer", "sig_id", "result"],
        signature_rows,
    )
    write_tsv(
        args.out_dir / "camera-preservation-report.tsv",
        ["module", "baseline_sha256", "candidate_sha256", "reason", "result"],
        preservation_rows,
    )
    write_tsv(
        args.out_dir / "camera-import-crc.tsv",
        list(selected_import_rows[0].keys()),
        [list(row.values()) for row in selected_import_rows],
    )
    write_tsv(
        args.out_dir / "camera-module-contract.tsv",
        ["module", "imports", "exports", "import_contract", "export_contract", "structural_contract"],
        [
            ["camera", len(new_camera.imports), len(new_camera.exports), "IDENTICAL", "IDENTICAL", "FUNCTION_BODY_ONLY"],
            ["camera_extension", len(old_extension.imports), len(old_extension.exports), "IDENTICAL", "IDENTICAL", "EXACT_ALLOCATED_SECTIONS"],
        ],
    )
    write_tsv(
        args.out_dir / "camera-vendor-boot-dormant-boundary.tsv",
        list(external_rows[0].keys()) + ["normal_boot_policy"],
        [list(row.values()) + ["DORMANT_NOT_REQUESTED"] for row in external_rows],
    )
    (args.out_dir / "camera-rer-machine-code.txt").write_text(
        "BASELINE\n" + old_disassembly + "\nCANDIDATE\n" + new_disassembly,
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

