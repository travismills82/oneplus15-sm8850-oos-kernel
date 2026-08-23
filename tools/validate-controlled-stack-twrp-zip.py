#!/usr/bin/env python3
"""Fail-closed static validation for a controlled-stack TWRP ZIP."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import zipfile


EXPECTED_ENTRIES = {
    "boot.img",
    "system_dlkm.img",
    "vendor_dlkm.img",
    "manifest.json",
    "release-contract.json",
    "physical-validation.md",
    "README.txt",
    "SHA256SUMS",
    "META-INF/com/google/android/update-binary",
    "META-INF/com/google/android/updater-script",
    "tools/twrp-flash-controlled-stack",
}


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zip", type=pathlib.Path)
    parser.add_argument("--boot", required=True, type=pathlib.Path)
    parser.add_argument("--system-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-dlkm", required=True, type=pathlib.Path)
    parser.add_argument("--helper", required=True, type=pathlib.Path)
    args = parser.parse_args()

    archive_path = args.zip.resolve()
    if not archive_path.is_file():
        die(f"ZIP does not exist: {archive_path}")
    expected_payloads = {
        "boot.img": args.boot.resolve(),
        "system_dlkm.img": args.system_dlkm.resolve(),
        "vendor_dlkm.img": args.vendor_dlkm.resolve(),
        "tools/twrp-flash-controlled-stack": args.helper.resolve(),
    }
    if any(not path.is_file() for path in expected_payloads.values()):
        die("one or more comparison inputs are missing")

    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            die("ZIP integrity test failed")
        names = set(archive.namelist())
        if names != EXPECTED_ENTRIES:
            die(f"unexpected ZIP entry set: {sorted(names ^ EXPECTED_ENTRIES)}")
        for name, source in expected_payloads.items():
            if sha256_bytes(archive.read(name)) != sha256_file(source):
                die(f"ZIP member differs from qualified input: {name}")

        manifest = json.loads(archive.read("manifest.json"))
        if manifest.get("write_scope") != ["vendor_dlkm", "system_dlkm", "boot"]:
            die("manifest write scope is not the controlled three-image set")
        if manifest.get("write_order") != ["vendor_dlkm", "system_dlkm", "boot"]:
            die("manifest write order is not dependency-first with boot last")
        if "not packaged" not in manifest.get("vendor_boot", ""):
            die("manifest does not preserve vendor_boot")
        if "not packaged" not in manifest.get("vbmeta", ""):
            die("manifest does not preserve VBMeta")

        update = archive.read("META-INF/com/google/android/update-binary").decode()
        helper = archive.read("tools/twrp-flash-controlled-stack").decode()
        required_update_markers = (
            "CONTROLLED_STACK_INSTALLER_MODE:---flash",
            "--vendor-dlkm",
            "--system-dlkm",
            "--boot",
            "HELPER_SHA256=",
            "unmount_if_selected",
            "vendor_boot and VBMeta are intentionally unchanged and absent",
        )
        for marker in required_update_markers:
            if marker not in update:
                die(f"update-binary lacks required marker: {marker}")
        required_helper_markers = (
            "Dependency-first ordering",
            "require_durable_backup_destination",
            "twrp.user.0.decrypt",
            "Update state: none",
            "system_dlkm_oki is intentionally not touched",
            "flash_and_verify \"vendor_dlkm",
            "flash_and_verify \"system_dlkm",
            "flash_and_verify \"boot",
        )
        positions = []
        for marker in required_helper_markers:
            position = helper.find(marker)
            if position < 0:
                die(f"embedded helper lacks required marker: {marker}")
            positions.append(position)
        if not positions[-3] < positions[-2] < positions[-1]:
            die("embedded helper write order is not vendor_dlkm, system_dlkm, boot")

        sums = {}
        for line in archive.read("SHA256SUMS").decode().splitlines():
            digest, name = line.split("  ", 1)
            sums[name] = digest
        if set(sums) != EXPECTED_ENTRIES - {"SHA256SUMS"}:
            die("internal SHA256SUMS entry set is incomplete")
        for name, digest in sums.items():
            if sha256_bytes(archive.read(name)) != digest:
                die(f"internal SHA256SUMS mismatch: {name}")

        for executable in (
            "META-INF/com/google/android/update-binary",
            "tools/twrp-flash-controlled-stack",
        ):
            mode = archive.getinfo(executable).external_attr >> 16
            if mode & 0o111 == 0:
                die(f"ZIP executable bit is absent: {executable}")

    print("CONTROLLED STACK TWRP ZIP STATIC VALIDATION PASS")
    print(f"zip={archive_path}")
    print(f"zip_sha256={sha256_file(archive_path)}")
    print("payloads=exact_physical_match")
    print("write_order=vendor_dlkm,system_dlkm,boot")
    print("vendor_boot=unchanged")
    print("vbmeta=unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
