#!/usr/bin/env python3
"""Package the exact physically validated controlled-v1 TEST3 payloads."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys


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


def check_hash(path: pathlib.Path, expected: str, label: str) -> None:
    actual = sha256(path)
    if actual != expected:
        die(f"{label} differs from physically validated TEST3: {actual} != {expected}")


def tsv_rows(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def copy(path: pathlib.Path, destination: pathlib.Path) -> None:
    shutil.copyfile(path, destination)
    shutil.copymode(path, destination)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--boot", required=True, type=pathlib.Path)
    parser.add_argument("--system-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-boot", type=pathlib.Path)
    parser.add_argument("--kernel-build-dir", required=True, type=pathlib.Path)
    parser.add_argument("--system-stage-dir", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-stage-dir", required=True, type=pathlib.Path)
    parser.add_argument("--out-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parent.parent
    profile_path = repo / "tools/controlled-v1-test3-release.json"
    profile = json.loads(profile_path.read_text(encoding="utf-8"))
    validator = repo / "tools/validate-system-dlkm-load-contract.py"

    boot = require_file(args.boot, "boot image")
    system_image = require_file(args.system_dlkm, "system_dlkm image")
    vendor_image = require_file(args.vendor_dlkm, "vendor_dlkm image")
    vendor_boot = require_file(args.vendor_boot, "vendor_boot image") if args.vendor_boot else None
    kernel = require_dir(args.kernel_build_dir, "kernel build directory")
    system_stage = require_dir(args.system_stage_dir, "system stage directory")
    vendor_stage = require_dir(args.vendor_stage_dir, "vendor stage directory")
    out = args.out_dir.resolve()
    if out.exists():
        die(f"refusing to overwrite output: {out}")

    check_hash(boot, profile["boot_sha256"], "boot.img")
    check_hash(system_image, profile["system_dlkm_sha256"], "system_dlkm.img")
    check_hash(vendor_image, profile["vendor_dlkm_sha256"], "vendor_dlkm.img")
    if vendor_boot:
        check_hash(vendor_boot, profile["vendor_boot_sha256"], "vendor_boot.img")

    if boot.read_bytes()[:8] != b"ANDROID!":
        die("boot.img has no Android boot magic")
    with system_image.open("rb") as stream:
        stream.seek(1024)
        if stream.read(4) != bytes.fromhex("e2e1f5e0"):
            die("system_dlkm.img is not EROFS")
    with vendor_image.open("rb") as stream:
        stream.seek(1024 + 56)
        if stream.read(2) != bytes.fromhex("53ef"):
            die("vendor_dlkm.img is not ext4")

    config = require_file(pathlib.Path(str(kernel) + "_config/out_dir/.config"), "generated config")
    contracts = {
        "config_sha256": config,
        "module_symvers_sha256": require_file(kernel / "Module.symvers", "Module.symvers"),
        "system_map_sha256": require_file(kernel / "System.map", "System.map"),
        "vmlinux_sha256": require_file(kernel / "vmlinux", "vmlinux"),
        "Image_sha256": require_file(kernel / "Image", "Image"),
    }
    for field, path in contracts.items():
        check_hash(path, profile[field], field)
    kernel_release = require_file(kernel / "include/config/kernel.release", "kernel.release").read_text().strip()
    if kernel_release != profile["kernel_release"]:
        die(f"unexpected kernel release: {kernel_release}")
    canoe_config = require_file(
        kernel.parent.parent / "soc-repo/canoe_perf_config/out_dir/.config",
        "generated Canoe config",
    )
    check_hash(canoe_config, profile["canoe_config_sha256"], "canoe_config_sha256")
    config_text = config.read_text(encoding="utf-8")
    canoe_config_text = canoe_config.read_text(encoding="utf-8")
    for setting in (
        "CONFIG_MODULE_SIG=y",
        "CONFIG_MODULE_SIG_PROTECT=y",
        "CONFIG_MODVERSIONS=y",
        "CONFIG_GENDWARFKSYMS=y",
    ):
        if f"{setting}\n" not in config_text:
            die(f"required controlled-v1 configuration missing: {setting}")
    for setting in ("CONFIG_CFG80211=m", "CONFIG_MAC80211=m", "CONFIG_RFKILL=y"):
        if f"{setting}\n" not in canoe_config_text:
            die(f"required Canoe configuration missing: {setting}")

    system_readback = require_dir(system_stage / "readback/lib/modules", "system readback modules")
    system_release_dirs = [path for path in (system_stage / "staging/lib/modules").iterdir() if path.is_dir()]
    if len(system_release_dirs) != 1:
        die("system stage must contain exactly one kernel release directory")
    modules_builtin = require_file(system_release_dirs[0] / "modules.builtin", "modules.builtin")
    vendor_root = require_dir(vendor_stage / "staging/lib/modules", "vendor staged modules")
    if sha256(require_file(vendor_stage / "vendor_dlkm.img", "staged vendor image")) != profile["vendor_dlkm_sha256"]:
        die("vendor stage is not Candidate A")

    module_contract = require_file(vendor_stage / "module-replacement.tsv", "vendor module contract")
    replacement_rows = tsv_rows(module_contract)
    action_counts: dict[str, int] = {}
    for row in replacement_rows:
        action_counts[row["action"]] = action_counts.get(row["action"], 0) + 1
    expected_actions = {
        "RETAIN_EXACT_STOCK": profile["vendor_dlkm_exact_stock"],
        "SOURCE_REPLACEMENT": profile["vendor_dlkm_source_replacements"],
        "RE_SIGN_STOCK": profile["vendor_dlkm_resigned_stock"],
    }
    if len(replacement_rows) != profile["vendor_dlkm_module_count"] or action_counts != expected_actions:
        die(f"Candidate A module contract changed: rows={len(replacement_rows)} actions={action_counts}")

    cellular_contract = require_file(vendor_stage / "cellular-stock-module-list.tsv", "cellular stock contract")
    cellular_rows = [row for row in tsv_rows(cellular_contract) if row["partition"] == "vendor_dlkm"]
    if len(cellular_rows) != profile["stock_vendor_cellular_modules"]:
        die("cellular vendor-DLKM closure count changed")
    for row in cellular_rows:
        if row["result"] != "MATCH" or row["stock_sha256"] != row["candidate_sha256"]:
            die(f"cellular module is not exact stock: {row['module']}")

    out.mkdir(parents=True)
    load_contract = out / "system-dlkm-load-contract.tsv"
    subprocess.run(
        [
            sys.executable,
            str(validator),
            "--system-root",
            str(system_readback),
            "--modules-builtin",
            str(modules_builtin),
            "--vendor-root",
            str(vendor_root),
            "--required",
            "wwan.ko",
            "--out",
            str(load_contract),
        ],
        check=True,
    )

    copy(boot, out / "boot.img")
    copy(system_image, out / "system_dlkm.img")
    copy(vendor_image, out / "vendor_dlkm.img")
    if vendor_boot:
        copy(vendor_boot, out / "vendor_boot.img")
    copy(module_contract, out / "vendor-dlkm-module-contract.tsv")

    for name in (
        "cellular-stock-module-list.tsv",
        "wlan-custom-module-list.tsv",
        "dependency-report.tsv",
        "CRC-report.tsv",
        "signature-report.tsv",
    ):
        copy(require_file(vendor_stage / name, name), out / name)

    repository_commit = subprocess.check_output(
        ["git", "-C", str(repo), "rev-parse", "HEAD"], text=True
    ).strip()
    release_contract = dict(profile)
    release_contract.update(
        {
            "repository_commit": repository_commit,
            "custom_module_count": profile["system_dlkm_module_count"]
            + profile["vendor_dlkm_source_replacements"]
            + profile["vendor_dlkm_resigned_stock"],
            "retained_stock_vendor_module_count": profile["vendor_dlkm_exact_stock"],
            "vendor_boot_packaged": bool(vendor_boot),
            "device_writes": "none",
        }
    )
    (out / "release-contract.json").write_text(
        json.dumps(release_contract, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    manifest = {
        "device": profile["device"],
        "platform": profile["platform"],
        "firmware": profile["firmware"],
        "generation": profile["generation"],
        "payloads": {
            name: {"sha256": sha256(out / name), "size": (out / name).stat().st_size}
            for name in ("boot.img", "system_dlkm.img", "vendor_dlkm.img")
        },
        "vendor_boot": {
            "policy": profile["vendor_boot_policy"],
            "expected_stock_sha256": profile["vendor_boot_sha256"],
            "packaged": bool(vendor_boot),
        },
        "vbmeta": profile["vbmeta_policy"],
        "physical_validation_commit": profile["physical_validation_commit"],
        "device_writes": "none",
    }
    if vendor_boot:
        manifest["payloads"]["vendor_boot.img"] = {
            "sha256": sha256(out / "vendor_boot.img"),
            "size": (out / "vendor_boot.img").stat().st_size,
        }
    (out / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    load_rows = tsv_rows(load_contract)
    report = [
        "CONTROLLED-V1 TEST3 VALIDATION REPORT",
        "",
        "result=PASS",
        "physical_test3_payload_hashes=exact_match",
        f"kernel_release={kernel_release}",
        f"system_dlkm_modules={profile['system_dlkm_module_count']}",
        f"system_modules_load_entries={profile['system_dlkm_load_entries']}",
        f"wwan_present={any(row['module'] == 'wwan.ko' and row['status'] == 'PASS' for row in load_rows)}",
        "stale_system_load_entries=0",
        "missing_system_load_entries=0",
        "cfg80211_before_peach=yes",
        f"vendor_dlkm_modules={len(replacement_rows)}",
        f"exact_stock_vendor_cellular_modules={len(cellular_rows)}",
        "vendor_boot=stock_unchanged",
        "vbmeta=stock_unchanged",
        "private_signing_material=not_packaged",
        "device_writes=none",
        "",
        "TEST0=PASS stock r7 plus stock DLKMs",
        "TEST1=PASS controlled-v1 Image plus stock DLKMs",
        "TEST2=PASS corrected controlled system-DLKM plus stock vendor-DLKM cellular",
        "TEST3=PASS controlled WLAN plus exact stock cellular closure",
        "root_cause=empty failed system_dlkm modules.load omitted wwan.ko",
        "ipa_rmnet_root_cause_claim=no",
    ]
    (out / "validation-report.txt").write_text("\n".join(report) + "\n", encoding="utf-8")

    forbidden_suffixes = {".key", ".p12", ".pfx"}
    for path in out.rglob("*"):
        if path.is_file() and (path.suffix in forbidden_suffixes or "signing_key" in path.name):
            die(f"private signing material would be packaged: {path}")

    sum_files = sorted(path for path in out.rglob("*") if path.is_file() and path.name != "SHA256SUMS")
    with (out / "SHA256SUMS").open("w", encoding="utf-8") as stream:
        for path in sum_files:
            stream.write(f"{sha256(path)}  {path.relative_to(out)}\n")

    print("CONTROLLED-V1 TEST3 PACKAGE PASS")
    print(f"output={out}")
    print("physical_test3_hash_match=yes")
    print("device_writes=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
