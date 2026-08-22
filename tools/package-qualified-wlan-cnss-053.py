#!/usr/bin/env python3
"""Freeze the exact physically qualified controlled-v1 WLAN .053 payload.

This helper promotes an already-built candidate into a sanitized qualification
package. It never rebuilds, signs, repacks, flashes, or otherwise mutates the
three tested images. The fail-closed payload hashes are the physical device
read-back hashes from the extended qualification session.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import pathlib
import shutil
import subprocess


PAYLOADS = {
    "boot.img": (
        "boot.img",
        "84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab",
    ),
    "system_dlkm.img": (
        "system_dlkm.img",
        "de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef",
    ),
    "vendor_dlkm.img": (
        "vendor_dlkm-wlan053.img",
        "8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e",
    ),
}
REQUIRED_FILES = {
    "cellular-exact-stock-final.tsv",
    "system-dlkm-load-contract.tsv",
    "vendor-dlkm-module-contract.tsv",
    "wlan053-cellular-provider-contract.tsv",
    "wlan053-firmware-contract.tsv",
    "wlan053-import-crc.tsv",
    "wlan053-replacement.tsv",
    "wlan053-source-delta-summary.json",
}
QUALIFICATION = "primary_pass_extended_core_pass_optional_environment_tests_remain"


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_file(path: pathlib.Path, label: str) -> pathlib.Path:
    resolved = path.resolve()
    if not resolved.is_file():
        die(f"missing {label}: {resolved}")
    return resolved


def copy(source: pathlib.Path, destination: pathlib.Path) -> None:
    shutil.copyfile(source, destination)
    shutil.copymode(source, destination)


def validate_load_contract(path: pathlib.Path) -> None:
    with path.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    system = [row for row in rows if row.get("partition") == "system_dlkm"]
    if len(system) != 46:
        die(f"system modules.load count changed: {len(system)}")
    if any(row.get("status") != "PASS" for row in rows):
        die("system-DLKM load contract contains a failing row")
    wwan = [row for row in system if row.get("module") == "wwan.ko"]
    if len(wwan) != 1 or wwan[0].get("load_order") != "21":
        die("wwan.ko is not the required system modules.load entry 21")
    if any(row.get("stale_builtin_entry") != "no" for row in system):
        die("system-DLKM load contract contains a stale built-in entry")


def validate_cellular_contract(path: pathlib.Path) -> None:
    with path.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if len(rows) != 27:
        die(f"exact-stock cellular closure changed: {len(rows)} modules")
    if any(row.get("status") != "MATCH" for row in rows):
        die("exact-stock cellular closure contains a non-matching module")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-dir", required=True, type=pathlib.Path)
    parser.add_argument("--qualification-record", required=True, type=pathlib.Path)
    parser.add_argument("--out-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parent.parent
    candidate = args.candidate_dir.resolve()
    qualification = require_file(args.qualification_record, "qualification record")
    output = args.out_dir.resolve()
    if not candidate.is_dir():
        die(f"candidate directory does not exist: {candidate}")
    if output.exists():
        die(f"refusing to overwrite output: {output}")
    if candidate == output:
        die("candidate and output directories must differ")

    qualification_text = qualification.read_text(encoding="utf-8")
    if "PARTIAL — CORE PASS, OPTIONAL ENVIRONMENT TESTS REMAIN" not in qualification_text:
        die("qualification record does not contain the reviewed result")
    for _, digest in PAYLOADS.values():
        if digest not in qualification_text:
            die(f"qualification record omits payload SHA-256 {digest}")

    for output_name, (candidate_name, expected) in PAYLOADS.items():
        path = require_file(candidate / candidate_name, candidate_name)
        actual = sha256(path)
        if actual != expected:
            die(f"{output_name} differs from the physically qualified payload: {actual}")
    for name in REQUIRED_FILES:
        require_file(candidate / name, name)

    validate_load_contract(candidate / "system-dlkm-load-contract.tsv")
    validate_cellular_contract(candidate / "cellular-exact-stock-final.tsv")

    source_contract = json.loads(
        require_file(candidate / "release-contract.json", "candidate release contract").read_text(
            encoding="utf-8"
        )
    )
    source_manifest = json.loads(
        require_file(candidate / "manifest.json", "candidate manifest").read_text(encoding="utf-8")
    )
    if source_contract.get("exact_stock_cellular_modules") != 27:
        die("candidate release contract does not retain 27 stock cellular modules")
    if source_contract.get("source_replacements") != [
        "cfg80211", "mac80211", "qca_cld3_peach_v2"
    ]:
        die("candidate source-replacement set differs from the qualified design")
    if source_contract.get("kernel_release") != "6.12.23-android16-5-o-g6744a3f6bcf4-4k":
        die("candidate kernel release differs from the qualified generation")

    output.mkdir(parents=True)
    for name in sorted(REQUIRED_FILES):
        copy(candidate / name, output / name)
    for output_name, (candidate_name, _) in PAYLOADS.items():
        copy(candidate / candidate_name, output / output_name)
    copy(qualification, output / "extended-qualification.md")

    commit = subprocess.check_output(
        ["git", "-C", str(repo), "rev-parse", "HEAD"], text=True
    ).strip()
    payload_contract = {
        name: {"sha256": sha256(output / name), "size": (output / name).stat().st_size}
        for name in PAYLOADS
    }
    source_contract.update(
        {
            "repository_commit": commit,
            "canonical_branch": "feature/controlled-v1-wlan053",
            "physical_validation": QUALIFICATION,
            "qualification_record": {
                "file": "extended-qualification.md",
                "sha256": sha256(qualification),
            },
            "wlan_source_version": "AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.053",
            "active_cnss_version": "AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.037-stock",
            "cellular_closure": "27_exact_stock_modules",
            "payloads": payload_contract,
            "device_writes": "none",
        }
    )
    (output / "release-contract.json").write_text(
        json.dumps(source_contract, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    source_manifest.update(
        {
            "canonical_branch": "feature/controlled-v1-wlan053",
            "physical_validation": QUALIFICATION,
            "qualification_record": "extended-qualification.md",
            "wlan_source_version": "AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.053",
            "active_cnss_version": "AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.037-stock",
            "cellular_closure": "27 exact-stock IPA/GSI/RMNET/data modules",
            "payloads": payload_contract,
            "vendor_boot": "stock unchanged and not packaged",
            "vbmeta": "stock unchanged and not packaged",
            "device_writes": "none",
        }
    )
    (output / "manifest.json").write_text(
        json.dumps(source_manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    (output / "wlan-version-contract.txt").write_text(
        "wlan_source=AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.053\n"
        "active_cnss=AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.037-stock\n"
        "source_replacements=cfg80211,mac80211,qca_cld3_peach_v2\n"
        "cfg80211=module\nmac80211=module\n",
        encoding="utf-8",
    )
    (output / "cellular-retained-stock-declaration.txt").write_text(
        "cellular_closure=IPA/GSI/RMNET/data\n"
        "module_count=27\n"
        "delivery=exact OxygenOS 16.0.9.400(EX01) stock binaries\n"
        "qualification=Visible LTE HOME, RMNET IPv4/IPv6, IP and DNS PASS\n",
        encoding="utf-8",
    )
    (output / "validation-report.txt").write_text(
        "CONTROLLED-V1 WLAN/CNSS .053 QUALIFIED BASELINE\n\n"
        f"physical_validation={QUALIFICATION}\n"
        "payload_hashes=EXACT_PHYSICAL_READBACK_MATCH\n"
        "system_modules_load_entries=46\nwwan_load_order=21\n"
        "stale_system_load_entries=0\nmissing_system_load_entries=0\n"
        "custom_wlan_modules=cfg80211,mac80211,qca_cld3_peach_v2\n"
        "active_cnss=exact_stock_037\nexact_stock_cellular_modules=27\n"
        "unresolved_imports=0\ncrc_mismatches=0\n"
        "protected_export_failures=0\nsignature_failures=0\n"
        "vendor_boot=stock_unchanged_not_packaged\n"
        "vbmeta=stock_unchanged_not_packaged\ndevice_writes=none\n",
        encoding="utf-8",
    )

    forbidden_suffixes = {".key", ".p12", ".pfx"}
    for path in output.rglob("*"):
        if path.is_file() and (
            path.suffix in forbidden_suffixes
            or "signing_key" in path.name
            or "backup" in path.name.lower()
            or "capture" in path.name.lower()
        ):
            die(f"forbidden private/backup/capture material would be packaged: {path}")

    files = sorted(path for path in output.rglob("*") if path.is_file())
    with (output / "SHA256SUMS").open("w", encoding="utf-8") as stream:
        for path in files:
            stream.write(f"{sha256(path)}  {path.relative_to(output)}\n")

    print("CONTROLLED-V1 WLAN/CNSS .053 QUALIFIED PACKAGE PASS")
    print(f"output={output}")
    print(f"physical_validation={QUALIFICATION}")
    for name, (_, expected) in PAYLOADS.items():
        print(f"{name}={expected}")
    print("device_writes=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
