#!/usr/bin/env python3
"""Package a vendor-only Bluetooth .046 overlay on qualified WLAN .053.

The kernel contract, boot image, system-DLKM, WLAN modules, and stock cellular
closure are immutable inputs.  Only the three explicitly reviewed Bluetooth
vendor modules may differ.  This tool never builds, signs, repacks, or flashes.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import pathlib
import shutil
import subprocess
import sys


RELEASE = "6.12.23-android16-5-o-g6744a3f6bcf4-4k"
BOOT_SHA256 = "84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab"
SYSTEM_SHA256 = "de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef"
BASE_VENDOR_SHA256 = "8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e"
BT_SOURCE_ID = "8906fd47be43616ee8ed532ae571ecbe30dced49"
SIGNER = "OnePlus 15 Controlled OOS Module Signing v1"
REPLACEMENTS = {"btpower", "bt_fm_swr", "btfm_slim_codec"}


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_file(path: pathlib.Path, label: str) -> pathlib.Path:
    path = path.resolve()
    if not path.is_file():
        die(f"missing {label}: {path}")
    return path


def require_dir(path: pathlib.Path, label: str) -> pathlib.Path:
    path = path.resolve()
    if not path.is_dir():
        die(f"missing {label}: {path}")
    return path


def copy(source: pathlib.Path, destination: pathlib.Path) -> None:
    shutil.copyfile(source, destination)
    shutil.copymode(source, destination)


def load_elf_helpers(repo: pathlib.Path):
    helper = repo / "tools/validate-matched-wlan-vendor-dlkm.py"
    spec = importlib.util.spec_from_file_location("bt046_package_elf", helper)
    if spec is None or spec.loader is None:
        die(f"cannot load ELF helpers: {helper}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def modinfo(path: pathlib.Path, field: str) -> str:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)], check=True, text=True,
        stdout=subprocess.PIPE,
    )
    return ",".join(line for line in result.stdout.splitlines() if line)


def validate_load_contract(path: pathlib.Path) -> None:
    with path.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    system = [row for row in rows if row.get("partition") == "system_dlkm"]
    if len(system) != 46:
        die(f"system modules.load count changed: {len(system)}")
    if any(row.get("status") != "PASS" for row in system):
        die("system modules.load contract has a failing entry")
    wwan = [row for row in system if row.get("module") == "wwan.ko"]
    if len(wwan) != 1 or wwan[0].get("load_order") != "21":
        die("wwan.ko is not system modules.load entry 21")
    if any(row.get("stale_builtin_entry") != "no" for row in system):
        die("system modules.load contract has a stale built-in entry")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-package", type=pathlib.Path, required=True)
    parser.add_argument("--baseline-vendor-stage", type=pathlib.Path, required=True)
    parser.add_argument("--candidate-vendor-stage", type=pathlib.Path, required=True)
    parser.add_argument("--candidate-vendor-image", type=pathlib.Path, required=True)
    parser.add_argument("--vendor-validation-dir", type=pathlib.Path, required=True)
    parser.add_argument("--kernel-contract", type=pathlib.Path, required=True)
    parser.add_argument("--out-dir", type=pathlib.Path, required=True)
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parent.parent
    baseline = require_dir(args.baseline_package, "qualified WLAN053 package")
    baseline_stage = require_dir(args.baseline_vendor_stage, "qualified vendor staging tree")
    candidate_stage = require_dir(args.candidate_vendor_stage, "Bluetooth vendor staging tree")
    candidate_image = require_file(args.candidate_vendor_image, "Bluetooth vendor image")
    validation = require_dir(args.vendor_validation_dir, "Bluetooth vendor validation")
    contract_path = require_file(args.kernel_contract, "kernel contract")
    output = args.out_dir.resolve()
    if output.exists():
        die(f"refusing to overwrite output: {output}")

    boot = require_file(baseline / "boot.img", "qualified boot")
    system = require_file(baseline / "system_dlkm.img", "qualified system_dlkm")
    old_vendor = require_file(baseline / "vendor_dlkm.img", "qualified vendor_dlkm")
    for path, expected, label in (
        (boot, BOOT_SHA256, "boot"),
        (system, SYSTEM_SHA256, "system_dlkm"),
        (old_vendor, BASE_VENDOR_SHA256, "baseline vendor_dlkm"),
    ):
        actual = sha256(path)
        if actual != expected:
            die(f"{label} is not the qualified WLAN053 payload: {actual}")

    load_contract = require_file(
        baseline / "system-dlkm-load-contract.tsv", "system-DLKM load contract"
    )
    validate_load_contract(load_contract)

    summary_path = require_file(validation / "summary.json", "vendor validation summary")
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    if summary.get("result") != "PASS":
        die("vendor validation did not pass")
    if set(summary.get("changed_modules", [])) != REPLACEMENTS:
        die("vendor validation changed-module set is not the three Bluetooth modules")
    for field in (
        "unresolved_imports", "crc_mismatches", "signature_failures",
        "vermagic_failures", "cellular_hash_failures",
    ):
        if summary.get(field) != 0:
            die(f"vendor validation reports {field}={summary.get(field)}")
    if summary.get("exact_stock_cellular_modules") != 27:
        die("vendor validation does not preserve the 27-module cellular closure")

    contract = json.loads(contract_path.read_text(encoding="utf-8"))
    if contract.get("release") != RELEASE:
        die("kernel contract release changed")
    bt_contract = contract.get("subsystems", {}).get("bluetooth_vendor", {})
    if bt_contract.get("source_id") != BT_SOURCE_ID or not str(
        bt_contract.get("generation", "")
    ).endswith(".046"):
        die("Bluetooth module source identity is not the reviewed .046 import")

    elf = load_elf_helpers(repo)
    old_modules = elf.scan_root(baseline_stage, "baseline", "QUALIFIED_WLAN053")
    new_modules = elf.scan_root(candidate_stage, "candidate", "BT046")
    if set(old_modules) != set(new_modules) or len(new_modules) != 436:
        die("candidate vendor module inventory differs from the 436-module baseline")
    changed = {
        name for name in old_modules if old_modules[name].sha256 != new_modules[name].sha256
    }
    if changed != REPLACEMENTS:
        die(f"candidate has an unexpected module delta: {sorted(changed)}")

    output.mkdir(parents=True)
    copy(boot, output / "boot.img")
    copy(system, output / "system_dlkm.img")
    copy(candidate_image, output / "vendor_dlkm.img")
    copy(load_contract, output / "system-dlkm-load-contract.tsv")
    for name in (
        "summary.json", "vendor-dlkm-module-contract.tsv",
        "vendor-dlkm-import-contract.tsv", "cellular-exact-stock-final.tsv",
    ):
        copy(require_file(validation / name, name), output / name)

    module_rows = []
    for name in sorted(REPLACEMENTS):
        old = old_modules[name]
        new = new_modules[name]
        added = sorted(set(new.imports) - set(old.imports))
        removed = sorted(set(old.imports) - set(new.imports))
        changed_crc = sorted(
            symbol for symbol in set(old.imports) & set(new.imports)
            if old.imports[symbol] != new.imports[symbol]
        )
        path = new.path
        vermagic = modinfo(path, "vermagic")
        signer = modinfo(path, "signer")
        if not vermagic.startswith(RELEASE + " ") or signer != SIGNER:
            die(f"{name} does not satisfy release/signing contract")
        module_rows.append({
            "module": name,
            "old_sha256": old.sha256,
            "new_sha256": new.sha256,
            "old_imports": len(old.imports),
            "new_imports": len(new.imports),
            "added_imports": ",".join(
                f"{symbol}=0x{new.imports[symbol]:08x}" for symbol in added
            ),
            "removed_imports": ",".join(removed),
            "changed_import_crcs": ",".join(changed_crc),
            "old_exports": len(old.exports),
            "new_exports": len(new.exports),
            "exports_unchanged": "yes" if old.exports == new.exports else "no",
            "depends": modinfo(path, "depends"),
            "aliases": modinfo(path, "alias"),
            "firmware": modinfo(path, "firmware"),
            "vermagic": vermagic,
            "signer": signer,
            "status": "PASS",
        })
    contract_tsv = output / "bluetooth-module-contract.tsv"
    with contract_tsv.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, delimiter="\t", fieldnames=list(module_rows[0]))
        writer.writeheader()
        writer.writerows(module_rows)

    payloads = {
        name: {"sha256": sha256(output / name), "size": (output / name).stat().st_size}
        for name in ("boot.img", "system_dlkm.img", "vendor_dlkm.img")
    }
    repository_commit = subprocess.check_output(
        ["git", "-C", str(repo), "rev-parse", "HEAD"], text=True
    ).strip()
    release_contract = {
        "schema_version": 2,
        "generation": "controlled-v1-wlan053-bt046",
        "repository_commit": repository_commit,
        "kernel_contract": {
            "release": RELEASE,
            "source_id": contract["kernel_action_source_id"],
            "release_stamp_source_id": contract["release_stamp_source_id"],
            **contract["artifacts"],
        },
        "subsystems": contract["subsystems"],
        "signing": contract["signing"],
        "payloads": payloads,
        "vendor_module_count": 436,
        "bluetooth_replacements": sorted(REPLACEMENTS),
        "exact_stock_cellular_modules": 27,
        "system_modules_load_entries": 46,
        "wwan_load_order": 21,
        "validation": {
            "unresolved_imports": 0,
            "crc_mismatches": 0,
            "protected_export_failures": 0,
            "signature_failures": 0,
            "kernel_contract_guard": "PASS",
            "physical_validation": "NOT_PERFORMED",
        },
        "device_writes": "none",
    }
    (output / "release-contract.json").write_text(
        json.dumps(release_contract, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    manifest = {
        "device": "CPH2747",
        "platform": "canoe",
        "firmware": "OOS_16.0.9.400_EX01",
        "generation": "controlled-v1-wlan053-bt046",
        "kernel_release": RELEASE,
        "wlan": "AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.053",
        "bluetooth_vendor": "AU_TECHPACK_BTFM.LA.2.0.R1.00.00.00.000.046",
        "cellular": "stock OOS 16.0.9.400(EX01); 27 exact modules",
        "signing_generation": contract["signing"]["generation"],
        "payloads": payloads,
        "vendor_boot": "stock unchanged and not packaged",
        "vbmeta": "stock unchanged and not packaged",
        "write_scope": ["vendor_dlkm"],
        "device_writes": "none",
    }
    (output / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (output / "validation-report.txt").write_text(
        "CONTROLLED-V1 WLAN053 + BLUETOOTH VENDOR .046 STATIC CANDIDATE\n\n"
        "kernel_contract_guard=PASS\nImage_functional_differences=0\n"
        "config_differences=0\nModule.symvers_differences=0\n"
        "ABI=PASS_EMPTY\nKMI=PASS\nBluetooth_replacements=3\n"
        "unresolved_imports=0\ncrc_mismatches=0\n"
        "protected_export_failures=0\nsignature_failures=0\n"
        "WLAN053_payloads_retained=yes\nexact_stock_cellular_modules=27\n"
        "system_modules_load_entries=46\nwwan_load_order=21\n"
        "physical_validation=NOT_PERFORMED\ndevice_writes=none\n",
        encoding="utf-8",
    )

    for path in output.rglob("*"):
        if path.is_file() and (
            path.suffix.lower() in {".key", ".p12", ".pfx"}
            or "signing_key" in path.name
            or "backup" in path.name.lower()
            or "capture" in path.name.lower()
        ):
            die(f"forbidden private/device material would be packaged: {path}")
    files = sorted(path for path in output.rglob("*") if path.is_file())
    with (output / "SHA256SUMS").open("w", encoding="utf-8") as stream:
        for path in files:
            stream.write(f"{sha256(path)}  {path.relative_to(output)}\n")

    print("CONTROLLED-V1 WLAN053 + BLUETOOTH .046 PACKAGE PASS")
    print(f"output={output}")
    for name, metadata in payloads.items():
        print(f"{name}={metadata['sha256']}")
    print("device_writes=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
