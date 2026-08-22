#!/usr/bin/env python3
"""Package the statically validated minimal Canoe WLAN .053 candidate.

This is deliberately a non-flashing boundary.  It refuses the rejected full
CNSS overlay and accepts only the three-source replacement design proven to
fit the existing vendor-DLKM partition while retaining all 27 stock cellular
modules byte-for-byte.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys


BASELINE = {
    "boot_sha256": "25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313",
    "system_dlkm_sha256": "edebc94818e6fa4e214d58fd82fe46f6c513fc9850b3e7b77caf076a12270f05",
    "vendor_dlkm_sha256": "24e66015a3e4ea3583f895d529008d8c7c3706d7bc506ef550df936935127b80",
}
SOURCE_REPLACEMENTS = {"cfg80211", "mac80211", "qca_cld3_peach_v2"}
RESIGNED_STOCK = {"qca_cld3_kiwi_v2", "qca_cld3_wcn7750", "wonder"}


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


def tsv_rows(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def copy(path: pathlib.Path, destination: pathlib.Path) -> None:
    shutil.copyfile(path, destination)
    shutil.copymode(path, destination)


def check_magic(path: pathlib.Path, offset: int, expected: bytes, label: str) -> None:
    with path.open("rb") as stream:
        stream.seek(offset)
        actual = stream.read(len(expected))
    if actual != expected:
        die(f"{label} has unexpected image magic: {actual.hex()}")


def summary_pass(path: pathlib.Path, label: str) -> dict[str, object]:
    data = json.loads(require_file(path, label).read_text(encoding="utf-8"))
    if data.get("result") != "PASS":
        die(f"{label} is not PASS")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--boot", required=True, type=pathlib.Path)
    parser.add_argument("--system-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--kernel-build-dir", required=True, type=pathlib.Path)
    parser.add_argument("--system-stage-dir", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-stage-dir", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-validation-dir", required=True, type=pathlib.Path)
    parser.add_argument("--wlan-contract-dir", required=True, type=pathlib.Path)
    parser.add_argument("--source-delta-dir", required=True, type=pathlib.Path)
    parser.add_argument("--firmware-contract", required=True, type=pathlib.Path)
    parser.add_argument("--out-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parent.parent
    boot = require_file(args.boot, "boot image")
    system_image = require_file(args.system_dlkm, "system_dlkm image")
    vendor_image = require_file(args.vendor_dlkm, "vendor_dlkm image")
    kernel = require_dir(args.kernel_build_dir, "kernel build directory")
    system_stage = require_dir(args.system_stage_dir, "system stage")
    vendor_stage = require_dir(args.vendor_stage_dir, "vendor stage")
    vendor_validation = require_dir(args.vendor_validation_dir, "final vendor validation")
    wlan_contract = require_dir(args.wlan_contract_dir, "minimal WLAN contract")
    source_delta = require_dir(args.source_delta_dir, "source delta")
    firmware_contract = require_file(args.firmware_contract, "firmware contract")
    out = args.out_dir.resolve()
    if out.exists():
        die(f"refusing to overwrite output: {out}")

    check_magic(boot, 0, b"ANDROID!", "boot.img")
    check_magic(system_image, 1024, bytes.fromhex("e2e1f5e0"), "system_dlkm.img")
    check_magic(vendor_image, 1024 + 56, bytes.fromhex("53ef"), "vendor_dlkm.img")
    if sha256(require_file(vendor_stage / "vendor_dlkm.img", "staged vendor image")) != sha256(vendor_image):
        die("vendor image differs from the validated staged image")

    config = require_file(pathlib.Path(str(kernel) + "_config/out_dir/.config"), "generated config")
    canoe_config = require_file(
        kernel.parent.parent / "soc-repo/canoe_perf_config/out_dir/.config",
        "generated Canoe config",
    )
    for setting in (
        "CONFIG_MODULE_SIG=y", "CONFIG_MODULE_SIG_PROTECT=y",
        "CONFIG_MODVERSIONS=y", "CONFIG_GENDWARFKSYMS=y",
    ):
        if f"{setting}\n" not in config.read_text(encoding="utf-8"):
            die(f"required controlled-v1 setting missing: {setting}")
    canoe_text = canoe_config.read_text(encoding="utf-8")
    for setting in ("CONFIG_CFG80211=m", "CONFIG_MAC80211=m", "CONFIG_RFKILL=y"):
        if f"{setting}\n" not in canoe_text:
            die(f"required Canoe setting missing: {setting}")

    release = require_file(kernel / "include/config/kernel.release", "kernel.release").read_text().strip()
    source_scope = require_file(
        repo / "tools/controlled-oos-signing-v1-build-inputs.txt",
        "controlled source-input scope",
    )
    source_paths = [
        line.strip()
        for line in source_scope.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]
    source_id = subprocess.check_output(
        ["git", "-C", str(repo), "log", "-1", "--format=%H", "HEAD", "--", *source_paths],
        text=True,
    ).strip()
    if not source_id or f"-g{source_id[:12]}-4k" not in release:
        die(f"kernel release does not encode current source identity: {release}")
    kernel_files = {
        "config_sha256": config,
        "canoe_config_sha256": canoe_config,
        "module_symvers_sha256": require_file(kernel / "Module.symvers", "Module.symvers"),
        "system_map_sha256": require_file(kernel / "System.map", "System.map"),
        "vmlinux_sha256": require_file(kernel / "vmlinux", "vmlinux"),
        "Image_sha256": require_file(kernel / "Image", "Image"),
    }

    minimal = summary_pass(wlan_contract / "summary.json", "minimal WLAN contract summary")
    final = summary_pass(vendor_validation / "summary.json", "final vendor validation summary")
    required_minimal = {
        "wlan053_replacements": 3,
        "exact_stock_cellular_modules": 27,
        "unresolved_imports": 0,
        "crc_mismatches": 0,
        "changed_export_symbols": 0,
    }
    for field, expected in required_minimal.items():
        if minimal.get(field) != expected:
            die(f"minimal WLAN contract changed: {field}={minimal.get(field)!r}")
    if set(final.get("changed_modules", [])) != SOURCE_REPLACEMENTS | RESIGNED_STOCK:
        die("final vendor changed-module set differs from the reviewed minimal design")
    for field in (
        "cellular_hash_failures", "unresolved_imports", "crc_mismatches",
        "signature_failures", "vermagic_failures",
    ):
        if final.get(field) != 0:
            die(f"final vendor validation failure: {field}={final.get(field)!r}")
    if final.get("module_count") != 436 or final.get("exact_stock_cellular_modules") != 27:
        die("final vendor module/cellular inventory changed")

    system_root = require_dir(system_stage / "readback/lib/modules", "system readback modules")
    release_dirs = [path for path in (system_stage / "staging/lib/modules").iterdir() if path.is_dir()]
    if len(release_dirs) != 1:
        die("system stage must contain one release directory")
    modules_builtin = require_file(release_dirs[0] / "modules.builtin", "modules.builtin")
    vendor_root = require_dir(vendor_stage / "staging/lib/modules", "vendor staged modules")

    out.mkdir(parents=True)
    load_contract = out / "system-dlkm-load-contract.tsv"
    subprocess.run(
        [
            sys.executable, str(repo / "tools/validate-system-dlkm-load-contract.py"),
            "--system-root", str(system_root), "--modules-builtin", str(modules_builtin),
            "--vendor-root", str(vendor_root), "--required", "wwan.ko",
            "--out", str(load_contract),
        ],
        check=True,
    )
    load_rows = tsv_rows(load_contract)
    system_load_rows = [row for row in load_rows if row["partition"] == "system_dlkm"]
    if len(system_load_rows) != 46:
        die(f"system modules.load count changed: {len(system_load_rows)}")
    if not any(row["module"] == "wwan.ko" and row["status"] == "PASS" for row in system_load_rows):
        die("wwan.ko is absent from the system-DLKM load contract")
    if any(row["status"] != "PASS" for row in load_rows):
        die("system-DLKM load contract contains a failing row")

    copy(boot, out / "boot.img")
    copy(system_image, out / "system_dlkm.img")
    copy(vendor_image, out / "vendor_dlkm-wlan053.img")
    copy(firmware_contract, out / "wlan053-firmware-contract.tsv")
    for source, name in (
        (wlan_contract / "wlan053-replacement.tsv", "wlan053-replacement.tsv"),
        (wlan_contract / "wlan053-import-crc.tsv", "wlan053-import-crc.tsv"),
        (wlan_contract / "wlan053-cellular-provider-contract.tsv", "wlan053-cellular-provider-contract.tsv"),
        (vendor_validation / "vendor-dlkm-module-contract.tsv", "vendor-dlkm-module-contract.tsv"),
        (vendor_validation / "cellular-exact-stock-final.tsv", "cellular-exact-stock-final.tsv"),
        (source_delta / "wlan053-source-delta-summary.json", "wlan053-source-delta-summary.json"),
    ):
        copy(require_file(source, name), out / name)

    payloads = {
        name: {"sha256": sha256(out / name), "size": (out / name).stat().st_size}
        for name in ("boot.img", "system_dlkm.img", "vendor_dlkm-wlan053.img")
    }
    release_contract = {
        "generation": "controlled-v1-wlan-cnss-053",
        "repository_commit": subprocess.check_output(
            ["git", "-C", str(repo), "rev-parse", "HEAD"], text=True
        ).strip(),
        "kernel_source_id": source_id,
        "kernel_release": release,
        "kernel_contract": {field: sha256(path) for field, path in kernel_files.items()},
        "baseline_test3": BASELINE,
        "payloads": payloads,
        "source_replacements": sorted(SOURCE_REPLACEMENTS),
        "resigned_stock_modules": sorted(RESIGNED_STOCK),
        "vendor_modules": 436,
        "exact_stock_cellular_modules": 27,
        "vendor_boot": "stock unchanged and not packaged",
        "vbmeta": "stock unchanged and not packaged",
        "physical_validation": "pending",
        "device_writes": "none",
    }
    (out / "release-contract.json").write_text(
        json.dumps(release_contract, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (out / "manifest.json").write_text(
        json.dumps(
            {
                "device": "CPH2747",
                "platform": "canoe",
                "firmware": "OOS_16.0.9.400_EX01",
                "generation": "controlled-v1-wlan-cnss-053",
                "payloads": payloads,
                "write_scope": ["vendor_dlkm", "system_dlkm", "boot"],
                "write_order": ["vendor_dlkm", "system_dlkm", "boot"],
                "vendor_boot": "stock unchanged",
                "vbmeta": "stock unchanged",
                "physical_validation": "pending",
                "device_writes": "none",
            },
            indent=2,
            sort_keys=True,
        ) + "\n",
        encoding="utf-8",
    )
    report = [
        "WLAN/CNSS .053 MINIMAL CANDIDATE VALIDATION",
        "",
        "static_result=PASS",
        "physical_validation=PENDING",
        f"kernel_release={release}",
        "runtime_source_replacements=cfg80211,mac80211,qca_cld3_peach_v2",
        "active_cnss=exact_stock_037",
        "full_cnss_053_candidate=REJECTED_PARTITION_CAPACITY",
        "exact_stock_cellular_modules=27",
        "vendor_modules=436",
        "unresolved_imports=0",
        "crc_mismatches=0",
        "protected_export_failures=0",
        "signature_failures=0",
        "vermagic_failures=0",
        "changed_export_symbols=0",
        "system_modules_load_entries=46",
        "wwan_present=yes",
        "stale_system_load_entries=0",
        "missing_system_load_entries=0",
        "firmware_contract=PASS_NO_NEW_PEACH_REQUESTS",
        "vendor_boot=stock_unchanged",
        "vbmeta=stock_unchanged",
        "device_writes=none",
    ]
    (out / "wlan053-validation.txt").write_text("\n".join(report) + "\n", encoding="utf-8")

    forbidden_suffixes = {".key", ".p12", ".pfx"}
    for path in out.rglob("*"):
        if path.is_file() and (path.suffix in forbidden_suffixes or "signing_key" in path.name):
            die(f"private signing material would be packaged: {path}")
    files = sorted(path for path in out.rglob("*") if path.is_file() and path.name != "SHA256SUMS")
    with (out / "SHA256SUMS").open("w", encoding="utf-8") as stream:
        for path in files:
            stream.write(f"{sha256(path)}  {path.relative_to(out)}\n")

    print("WLAN/CNSS .053 PACKAGE STATIC PASS")
    print(f"output={out}")
    print(f"kernel_release={release}")
    print("physical_validation=pending")
    print("device_writes=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
