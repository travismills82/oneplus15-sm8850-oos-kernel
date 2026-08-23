#!/usr/bin/env python3
"""Build a deterministic TWRP ZIP from exact physically qualified payloads.

The archive embeds the reviewed hardened controlled-stack helper. It writes
vendor_dlkm, system_dlkm, then boot last; vendor_boot and VBMeta are excluded.
This builder never flashes a device and refuses payloads not named by the
physical baseline manifest and report.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
import zipfile


HARDENED_HELPER_COMMIT = "3f499bfd1f7152ea27b27935be22ff73581709a1"
HARDENED_HELPER_SHA256 = "84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50"
KERNEL_RELEASE = "6.12.23-android16-5-o-g6744a3f6bcf4-4k"
ZIP_TIMESTAMP = (2026, 8, 23, 0, 0, 0)
EXECUTABLES = {
    "META-INF/com/google/android/update-binary",
    "tools/twrp-flash-controlled-stack",
}


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


def read_json(path: pathlib.Path, label: str) -> dict:
    try:
        value = json.loads(require_file(path, label).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        die(f"invalid {label}: {error}")
    if not isinstance(value, dict):
        die(f"{label} is not a JSON object")
    return value


def verify_magic(path: pathlib.Path, label: str) -> None:
    with path.open("rb") as stream:
        if label == "boot":
            magic = stream.read(8)
            if magic != b"ANDROID!":
                die("boot image has invalid Android header magic")
        elif label == "system_dlkm":
            stream.seek(1024)
            if stream.read(4) != bytes.fromhex("e2e1f5e0"):
                die("system_dlkm image is not EROFS")
        elif label == "vendor_dlkm":
            stream.seek(1024 + 56)
            if stream.read(2) != bytes.fromhex("53ef"):
                die("vendor_dlkm image is not ext4")


def render(template: pathlib.Path, values: dict[str, str]) -> bytes:
    text = require_file(template, "installer template").read_text(encoding="utf-8")
    for key, value in values.items():
        text = text.replace(f"@{key}@", value)
    if "@" in text:
        unresolved = sorted({part.split("@", 1)[0] for part in text.split("@")[1::2]})
        die(f"installer template has unresolved placeholders: {unresolved}")
    return text.encode()


def zip_member(archive: zipfile.ZipFile, name: str, data: bytes) -> None:
    info = zipfile.ZipInfo(name, ZIP_TIMESTAMP)
    info.create_system = 3
    mode = 0o755 if name in EXECUTABLES else 0o644
    info.external_attr = (mode & 0xFFFF) << 16
    info.compress_type = zipfile.ZIP_STORED if name.endswith(".img") else zipfile.ZIP_DEFLATED
    archive.writestr(info, data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--boot", required=True, type=pathlib.Path)
    parser.add_argument("--system-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--baseline-manifest", required=True, type=pathlib.Path)
    parser.add_argument("--physical-report", required=True, type=pathlib.Path)
    parser.add_argument("--helper-source", required=True, type=pathlib.Path)
    parser.add_argument("--helper-repo", required=True, type=pathlib.Path)
    parser.add_argument("--out-dir", required=True, type=pathlib.Path)
    parser.add_argument(
        "--release-tag",
        default="controlled-v1-g6744-wlan053-bt046-nfc102-cellular102-audio059-graphics057-camera073-platform086",
    )
    args = parser.parse_args()

    if re.fullmatch(r"[A-Za-z0-9._-]+", args.release_tag) is None:
        die("release tag must contain only letters, numbers, dot, underscore, or hyphen")

    repo = pathlib.Path(__file__).resolve().parent.parent
    template = repo / "tools/twrp-controlled-stack-installer-template"
    boot = require_file(args.boot, "boot image")
    system = require_file(args.system_dlkm, "system_dlkm image")
    vendor = require_file(args.vendor_dlkm, "vendor_dlkm image")
    baseline_path = require_file(args.baseline_manifest, "baseline manifest")
    report = require_file(args.physical_report, "physical validation report")
    helper = require_file(args.helper_source, "controlled-stack helper")
    helper_repo = args.helper_repo.resolve()
    output = args.out_dir.resolve()
    if output.exists():
        die(f"refusing to overwrite output: {output}")

    baseline = read_json(baseline_path, "baseline manifest")
    qualification = str(baseline.get("qualification", ""))
    if not qualification.startswith("NORMAL_RUNTIME_PHYSICALLY_VALIDATED"):
        die("baseline manifest is not physically qualified")
    contract = baseline.get("kernel_contract", {})
    if contract.get("release") != KERNEL_RELEASE:
        die("baseline kernel release is not the qualified g6744 contract")

    expected = {
        "boot.img": contract.get("boot_sha256"),
        "system_dlkm.img": contract.get("system_dlkm_sha256"),
        "vendor_dlkm.img": baseline.get("vendor_dlkm_sha256"),
    }
    payloads = {
        "boot.img": boot,
        "system_dlkm.img": system,
        "vendor_dlkm.img": vendor,
    }
    for name, path in payloads.items():
        actual = sha256(path)
        if not isinstance(expected[name], str) or actual != expected[name]:
            die(f"{name} does not match the physical baseline: {actual}")
        if path.stat().st_size <= 0 or path.stat().st_size % 4096:
            die(f"{name} size is empty or not 4096-byte aligned")
    verify_magic(boot, "boot")
    verify_magic(system, "system_dlkm")
    verify_magic(vendor, "vendor_dlkm")

    report_text = report.read_text(encoding="utf-8")
    if "PASS — NORMAL-RUNTIME COMPATIBILITY VALIDATED" not in report_text:
        die("physical report does not contain the reviewed PASS result")
    for digest in expected.values():
        if digest not in report_text:
            die(f"physical report omits payload SHA-256 {digest}")

    if not helper_repo.is_dir():
        die(f"helper repository does not exist: {helper_repo}")
    helper_head = subprocess.check_output(
        ["git", "-C", str(helper_repo), "rev-parse", "HEAD"], text=True
    ).strip()
    lineage = subprocess.run(
        [
            "git", "-C", str(helper_repo), "merge-base", "--is-ancestor",
            HARDENED_HELPER_COMMIT, helper_head,
        ],
        check=False,
    )
    if lineage.returncode != 0:
        die("helper repository does not contain the hardened backup-guard lineage")
    if sha256(helper) != HARDENED_HELPER_SHA256:
        die("controlled-stack helper differs from the reviewed hardened helper")

    output.mkdir(parents=True)
    for name, source in payloads.items():
        shutil.copyfile(source, output / name)
    shutil.copyfile(baseline_path, output / "release-contract.json")
    shutil.copyfile(report, output / "physical-validation.md")

    payload_metadata = {
        name: {"sha256": sha256(path), "size": path.stat().st_size}
        for name, path in payloads.items()
    }
    manifest = {
        "schema_version": 1,
        "release_tag": args.release_tag,
        "device": "OnePlus 15 CPH2747 Canoe",
        "firmware": "OxygenOS 16.0.9.400(EX01)",
        "kernel_release": KERNEL_RELEASE,
        "source_commit": baseline.get("tested_source", {}).get("commit"),
        "qualification": qualification,
        "payloads": payload_metadata,
        "write_scope": ["vendor_dlkm", "system_dlkm", "boot"],
        "write_order": ["vendor_dlkm", "system_dlkm", "boot"],
        "vendor_boot": "stock unchanged and not packaged",
        "vbmeta": "stock unchanged and not packaged",
        "active_slot_change": "none",
        "twrp_helper": {
            "required_lineage": HARDENED_HELPER_COMMIT,
            "source_head": helper_head,
            "sha256": HARDENED_HELPER_SHA256,
            "backup_guard": "fail closed unless decrypted writable durable destination exists",
        },
        "coverage_boundaries": {
            "bluetooth": "core qualified; optional equipment tests remain",
            "nfc": "core qualified; tag and payment transaction remain environment dependent",
            "platform_etr_resize_trigger": "not observed",
        },
    }
    manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    release_contract_bytes = (output / "release-contract.json").read_bytes()
    physical_report_bytes = (output / "physical-validation.md").read_bytes()
    helper_bytes = helper.read_bytes()

    values = {
        "RELEASE_TAG": args.release_tag,
        "BOOT_BYTES": str(boot.stat().st_size),
        "BOOT_SHA256": sha256(boot),
        "SYSTEM_DLKM_BYTES": str(system.stat().st_size),
        "SYSTEM_DLKM_SHA256": sha256(system),
        "VENDOR_DLKM_BYTES": str(vendor.stat().st_size),
        "VENDOR_DLKM_SHA256": sha256(vendor),
        "HELPER_SHA256": HARDENED_HELPER_SHA256,
    }
    update_binary = render(
        template / "META-INF/com/google/android/update-binary.in", values
    )
    updater_script = require_file(
        template / "META-INF/com/google/android/updater-script", "updater-script"
    ).read_bytes()
    readme = (
        f"{args.release_tag}\n\n"
        "Physically qualified OnePlus 15 controlled stack.\n"
        f"Kernel: {KERNEL_RELEASE}\n\n"
        "TWRP writes the active slot in this order:\n"
        "  1. vendor_dlkm\n  2. system_dlkm\n  3. boot (last)\n\n"
        "Selected DLKM mount points are safely unmounted before validation.\n"
        "Every selected partition is backed up and every write is read-back verified.\n"
        "vendor_boot, VBMeta, system_dlkm_oki, and slot metadata are never changed.\n"
        "TWRP must have user 0 decrypted or mounted durable external storage.\n"
    ).encode()

    members = {
        "boot.img": boot.read_bytes(),
        "system_dlkm.img": system.read_bytes(),
        "vendor_dlkm.img": vendor.read_bytes(),
        "manifest.json": manifest_bytes,
        "release-contract.json": release_contract_bytes,
        "physical-validation.md": physical_report_bytes,
        "README.txt": readme,
        "META-INF/com/google/android/update-binary": update_binary,
        "META-INF/com/google/android/updater-script": updater_script,
        "tools/twrp-flash-controlled-stack": helper_bytes,
    }
    sums = "".join(
        f"{hashlib.sha256(data).hexdigest()}  {name}\n"
        for name, data in sorted(members.items())
    ).encode()
    members["SHA256SUMS"] = sums

    archive_name = f"{args.release_tag}-twrp.zip"
    archive_path = output / archive_name
    with zipfile.ZipFile(archive_path, "w", allowZip64=True) as archive:
        for name in sorted(members):
            zip_member(archive, name, members[name])
    with zipfile.ZipFile(archive_path) as archive:
        bad = archive.testzip()
        if bad is not None:
            die(f"ZIP integrity failure at {bad}")

    (output / "manifest.json").write_bytes(manifest_bytes)
    (output / "README.txt").write_bytes(readme)
    zip_hash = sha256(archive_path)
    (output / f"{archive_name}.sha256").write_text(
        f"{zip_hash}  {archive_name}\n", encoding="utf-8"
    )
    (output / "validation-report.txt").write_text(
        "CONTROLLED STACK TWRP ZIP BUILD PASS\n"
        f"release_tag={args.release_tag}\n"
        f"zip={archive_name}\nzip_sha256={zip_hash}\n"
        "payload_hashes=EXACT_PHYSICAL_MATCH\n"
        "write_scope=vendor_dlkm,system_dlkm,boot\n"
        "write_order=vendor_dlkm,system_dlkm,boot\n"
        "vendor_boot=unchanged_not_packaged\nvbmeta=unchanged_not_packaged\n"
        "backup_guard=HARDENED\nreadback_verification=REQUIRED\n"
        "live_twrp_dry_run=NOT_PERFORMED\n",
        encoding="utf-8",
    )
    outside = sorted(path for path in output.iterdir() if path.is_file() and path.name != "SHA256SUMS")
    (output / "SHA256SUMS").write_text(
        "".join(f"{sha256(path)}  {path.name}\n" for path in outside),
        encoding="utf-8",
    )

    print("CONTROLLED STACK TWRP ZIP BUILD PASS")
    print(f"output={output}")
    print(f"zip={archive_path}")
    print(f"zip_sha256={zip_hash}")
    for name, metadata in payload_metadata.items():
        print(f"{name}={metadata['sha256']}")
    print("device_writes=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
