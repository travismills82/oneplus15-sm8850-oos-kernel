#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only

"""Validate a firmware-specific OnePlus 15 boot-only TWRP archive."""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import stat
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath


EXPECTED_ENTRIES = {
    "META-INF/com/google/android/update-binary",
    "META-INF/com/google/android/updater-script",
    "README.txt",
    "boot.img",
    "checksums.sha256",
    "kernel-info.txt",
}
CHECKSUM_ENTRIES = EXPECTED_ENTRIES - {"checksums.sha256"}
FORBIDDEN_PAYLOAD_NAMES = {
    "system_dlkm.img",
    "system_dlkm.flatten.ext4.img",
    "system_dlkm_oki.img",
    "vendor_dlkm.img",
    "vendor_boot.img",
    "dtbo.img",
    "vbmeta.img",
    "vbmeta_system.img",
    "vbmeta_vendor.img",
}


def fail(message: str) -> None:
    raise ValueError(message)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_key_values(text: str, source: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for line_number, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.strip()
        if not line:
            continue
        if "=" not in line:
            fail(f"{source}:{line_number}: expected key=value")
        key, value = line.split("=", 1)
        if not key or key in result:
            fail(f"{source}:{line_number}: invalid or duplicate key {key!r}")
        result[key] = value
    return result


def parse_checksums(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    pattern = re.compile(r"^([0-9a-f]{64})  ([^\r\n]+)$")
    for line_number, line in enumerate(text.splitlines(), 1):
        match = pattern.fullmatch(line)
        if not match:
            fail(f"checksums.sha256:{line_number}: malformed checksum record")
        digest, name = match.groups()
        if name in result:
            fail(f"checksums.sha256:{line_number}: duplicate path {name}")
        result[name] = digest
    return result


def validate_zip(args: argparse.Namespace) -> None:
    archive = Path(args.zip).resolve()
    boot = Path(args.boot).resolve()
    if not archive.is_file() or not boot.is_file():
        fail("ZIP and boot inputs must be readable regular files")

    boot_hash = sha256_file(boot)
    boot_size = boot.stat().st_size
    if boot_size != args.boot_bytes:
        fail(f"boot size {boot_size} does not match expected {args.boot_bytes}")
    with boot.open("rb") as stream:
        if stream.read(8) != b"ANDROID!":
            fail("boot input does not contain an Android boot header")

    with zipfile.ZipFile(archive) as package:
        bad_member = package.testzip()
        if bad_member:
            fail(f"ZIP CRC validation failed for {bad_member}")
        infos = package.infolist()
        names = [info.filename for info in infos]
        if len(names) != len(set(names)):
            fail("ZIP contains duplicate member names")
        if set(names) != EXPECTED_ENTRIES:
            missing = sorted(EXPECTED_ENTRIES - set(names))
            extra = sorted(set(names) - EXPECTED_ENTRIES)
            fail(f"unexpected ZIP member set; missing={missing}, extra={extra}")
        for info in infos:
            path = PurePosixPath(info.filename)
            if path.is_absolute() or ".." in path.parts:
                fail(f"unsafe ZIP path: {info.filename}")
            mode = info.external_attr >> 16
            if stat.S_ISLNK(mode):
                fail(f"ZIP member must not be a symlink: {info.filename}")
        update_info = package.getinfo("META-INF/com/google/android/update-binary")
        update_mode = update_info.external_attr >> 16
        if not update_mode & stat.S_IXUSR:
            fail("update-binary is not executable")

        contents = {name: package.read(name) for name in names}

    if contents["boot.img"] != boot.read_bytes():
        fail("embedded boot.img is not byte-identical to the release boot input")
    if sha256_bytes(contents["boot.img"]) != boot_hash:
        fail("embedded boot.img checksum mismatch")

    checksums = parse_checksums(contents["checksums.sha256"].decode("utf-8"))
    if set(checksums) != CHECKSUM_ENTRIES:
        fail("internal checksum path set is incomplete or unexpected")
    for name in sorted(CHECKSUM_ENTRIES):
        actual = sha256_bytes(contents[name])
        if checksums[name] != actual:
            fail(f"internal checksum mismatch for {name}")

    info_text = contents["kernel-info.txt"].decode("utf-8")
    info = parse_key_values(info_text, "kernel-info.txt")
    expected_info = {
        "release_tag": args.release_tag,
        "firmware": args.firmware,
        "build_display_id": args.build_display_id,
        "device": "OnePlus 15 / CPH2747 / Canoe",
        "kernel_release": args.kernel_release,
        "source_commit": args.source_commit,
        "boot_bytes": str(args.boot_bytes),
        "boot_partition_bytes": str(args.boot_bytes),
        "boot_sha256": boot_hash,
        "system_dlkm_contract": "stock_oxygenos_erofs",
        "stock_system_dlkm_bytes": "14131200",
        "stock_system_dlkm_blocks": "3450",
        "stock_system_dlkm_erofs_magic": "e2e1f5e0",
        "stock_system_dlkm_sha256": args.stock_system_dlkm_sha256,
        "installer_writes": "boot_active_slot_only",
    }
    if info != expected_info:
        fail(f"kernel-info contract mismatch: expected={expected_info}, actual={info}")

    update_text = contents[
        "META-INF/com/google/android/update-binary"
    ].decode("utf-8")
    updater_text = contents[
        "META-INF/com/google/android/updater-script"
    ].decode("utf-8")
    readme_text = contents["README.txt"].decode("utf-8")
    combined_text = (
        update_text + "\n" + updater_text + "\n" + readme_text + "\n" + info_text
    )
    if re.search(r"@[A-Z0-9_]+@", combined_text):
        fail("archive contains an unresolved template placeholder")
    for expected in (
        args.release_tag,
        args.firmware,
        args.build_display_id,
        args.kernel_release,
        args.source_commit,
        boot_hash,
        args.stock_system_dlkm_sha256,
    ):
        if expected not in combined_text:
            fail(f"archive metadata is missing required value: {expected}")

    for name in FORBIDDEN_PAYLOAD_NAMES:
        if name in EXPECTED_ENTRIES:
            fail(f"forbidden payload unexpectedly allowed: {name}")
    forbidden_commands = (
        "lpmake",
        "lpadd",
        "lpunpack",
        "resize2fs",
        "fastboot flash",
        "avbctl disable",
        "disable-verity",
        "disable-verification",
    )
    for command in forbidden_commands:
        if command in update_text:
            fail(f"update-binary contains forbidden operation: {command}")
    for target in ("SYSTEM_DLKM_BLOCK", "vendor_dlkm", "vendor_boot", "dtbo", "vbmeta"):
        if re.search(rf"\bof=.*{re.escape(target)}", update_text, re.IGNORECASE):
            fail(f"update-binary contains a forbidden write target: {target}")

    with tempfile.TemporaryDirectory(prefix="op15-boot-only-zip-") as directory:
        script = Path(directory) / "update-binary"
        script.write_bytes(contents["META-INF/com/google/android/update-binary"])
        result = subprocess.run(
            ["sh", "-n", os.fspath(script)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode:
            fail(f"update-binary shell syntax failed: {result.stderr.strip()}")

    print("BOOT_ONLY_TWRP_ZIP_VALIDATION=PASS")
    print(f"zip={archive}")
    print(f"zip_sha256={sha256_file(archive)}")
    print(f"boot_sha256={boot_hash}")
    print(f"release_tag={args.release_tag}")
    print(f"kernel_release={args.kernel_release}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("zip")
    parser.add_argument("--boot", required=True)
    parser.add_argument("--release-tag", required=True)
    parser.add_argument("--firmware", required=True)
    parser.add_argument("--build-display-id", required=True)
    parser.add_argument("--kernel-release", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--stock-system-dlkm-sha256", required=True)
    parser.add_argument("--boot-bytes", type=int, default=100663296)
    args = parser.parse_args()
    try:
        validate_zip(args)
    except (OSError, UnicodeError, ValueError, zipfile.BadZipFile) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
