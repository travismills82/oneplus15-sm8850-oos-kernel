#!/usr/bin/env python3
"""Fail-closed metadata, package, and release-commit verification."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import zipfile


EXPECTED_ZIP_ENTRIES = {
    "META-INF/com/google/android/update-binary",
    "META-INF/com/google/android/updater-script",
    "README.txt",
    "SHA256SUMS",
    "boot.img",
    "manifest.json",
    "physical-validation.md",
    "release-contract.json",
    "system_dlkm.img",
    "tools/twrp-flash-controlled-stack",
    "vendor_dlkm.img",
}
ALLOWED_RELEASE_COMMIT_PATHS = (
    ".github/workflows/controlled-stack-release-validate.yml",
    "docs/controlled-stack-release-process.md",
    "docs/releases/",
    "tools/finalize-controlled-stack-release-manifest.py",
    "tools/verify-controlled-stack-release.py",
)


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_object(path: pathlib.Path, label: str) -> dict:
    if not path.is_file():
        die(f"missing {label}: {path}")
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        die(f"invalid {label}: {error}")
    if not isinstance(value, dict):
        die(f"{label} is not a JSON object")
    return value


def require_equal(label: str, actual: object, expected: object) -> None:
    if actual != expected:
        die(f"{label} mismatch: expected {expected!r}, got {actual!r}")


def require_sha256(label: str, value: object) -> str:
    if not isinstance(value, str) or re.fullmatch(r"[0-9a-f]{64}", value) is None:
        die(f"{label} is not a lowercase SHA-256 digest")
    return value


def verify_metadata(manifest: dict, signoff: dict, allow_symbolic: bool) -> None:
    release_id = manifest.get("release_id")
    if not isinstance(release_id, str) or re.fullmatch(r"[A-Za-z0-9._-]+", release_id) is None:
        die("release ID is missing or unsafe")
    require_equal("signoff release ID", signoff.get("release_id"), release_id)
    for label, value in (
        ("device", manifest.get("device", {}).get("model")),
        ("firmware", manifest.get("device", {}).get("firmware")),
        ("kernel release", manifest.get("kernel", {}).get("release")),
    ):
        if not isinstance(value, str) or not value.strip():
            die(f"{label} is missing")
    tested_source = manifest.get("physically_tested_source_head")
    if not isinstance(tested_source, str) or re.fullmatch(r"[0-9a-f]{40}", tested_source) is None:
        die("physically tested source is not a full Git object ID")
    require_equal("signoff tested source", signoff.get("physically_tested_source_head"), tested_source)
    helper_hash = require_sha256("helper hash", manifest.get("twrp_helper_sha256"))
    require_equal("write scope", manifest.get("write_scope"), ["vendor_dlkm", "system_dlkm", "boot"])
    require_equal("write order", manifest.get("write_order"), ["vendor_dlkm", "system_dlkm", "boot"])
    require_equal(
        "unchanged partition contract",
        set(manifest.get("not_modified", [])),
        {"vendor_boot", "VBMeta", "system_dlkm_oki", "slot metadata"},
    )
    zip_contract = manifest.get("release_zip", {})
    zip_hash = require_sha256("ZIP hash", zip_contract.get("sha256"))
    zip_name = zip_contract.get("filename")
    if not isinstance(zip_name, str) or pathlib.Path(zip_name).name != zip_name or not zip_name.endswith("-TWRP.zip"):
        die("release ZIP filename is unsafe or not a TWRP ZIP")
    if not isinstance(zip_contract.get("size"), int) or zip_contract["size"] <= 0:
        die("release ZIP size is invalid")
    require_equal("physical ZIP installation flag", zip_contract.get("physically_installed"), True)
    require_equal("signoff ZIP hash", signoff.get("release_zip_sha256"), zip_hash)
    require_equal("runtime source changes", manifest.get("runtime_source_changes_in_release_commit"), 0)
    require_equal("kernel input guard", manifest.get("kernel_input_guard"), "PASS")
    require_equal("payload entry set", set(manifest.get("payloads", {})), {"boot.img", "system_dlkm.img", "vendor_dlkm.img"})
    for name, metadata in manifest["payloads"].items():
        require_sha256(f"{name} hash", metadata.get("sha256"))
        if not isinstance(metadata.get("size"), int) or metadata["size"] <= 0:
            die(f"{name} size is invalid")

    required_passes = (
        "overall_pre_release_status",
        "exact_payload_hashes",
        "single_zip_twrp_installation",
        "post_write_partition_readback",
        "android_boot",
        "module_contract_scan",
        "kernel_fault_scan",
        "physical_zip_installation",
        "embedded_payload_hashes",
        "readback_validation",
    )
    for field in required_passes:
        require_equal(f"signoff {field}", signoff.get(field), "PASS")
    for field, result in signoff.get("subsystems", {}).items():
        require_equal(f"subsystem {field}", result, "PASS")
    require_equal("load entries", signoff.get("module_load_contract", {}).get("entries"), 46)
    require_equal("wwan entry", signoff.get("module_load_contract", {}).get("wwan_ko_entry"), 21)
    require_equal("handoff", signoff.get("subsystems", {}).get("wifi_cellular_handoff"), "PASS")
    transient = signoff.get("handoff_transient_packet_loss", {})
    require_equal("handoff transient observed", transient.get("observed"), True)
    require_equal("handoff packets lost", transient.get("packets_lost"), 1)
    require_equal("handoff stable IP", transient.get("stabilized_ip_test"), "5/5 PASS")
    require_equal("handoff stable DNS", transient.get("stabilized_dns_test"), "5/5 PASS")
    require_equal("handoff classification", transient.get("classification"), "NON_BLOCKING_TRANSIENT")
    require_equal("release approval", signoff.get("release_approval"), "APPROVED")
    require_equal("signoff kernel release", signoff.get("kernel_release"), manifest["kernel"]["release"])

    release_commit = manifest.get("release_commit")
    if release_commit == "TAG_TARGET" and allow_symbolic:
        pass
    elif not isinstance(release_commit, str) or re.fullmatch(r"[0-9a-f]{40}", release_commit) is None:
        die("release commit is neither a finalized object ID nor an allowed TAG_TARGET sentinel")


def verify_internal_sums(archive: zipfile.ZipFile) -> None:
    sums: dict[str, str] = {}
    for line in archive.read("SHA256SUMS").decode().splitlines():
        digest, name = line.split("  ", 1)
        sums[name] = digest
    require_equal("internal checksum entry set", set(sums), EXPECTED_ZIP_ENTRIES - {"SHA256SUMS"})
    for name, expected in sums.items():
        require_equal(f"internal checksum {name}", sha256_bytes(archive.read(name)), expected)


def verify_zip(path: pathlib.Path, manifest: dict) -> None:
    if not path.is_file():
        die(f"release ZIP is missing: {path}")
    require_equal("release ZIP filename", path.name, manifest["release_zip"]["filename"])
    require_equal("release ZIP size", path.stat().st_size, manifest["release_zip"]["size"])
    require_equal("release ZIP SHA-256", sha256_file(path), manifest["release_zip"]["sha256"])
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            die("release ZIP integrity test failed")
        require_equal("ZIP entry set", set(archive.namelist()), EXPECTED_ZIP_ENTRIES)
        internal_manifest = json.loads(archive.read("manifest.json"))
        require_equal("internal write scope", internal_manifest.get("write_scope"), ["vendor_dlkm", "system_dlkm", "boot"])
        require_equal("internal write order", internal_manifest.get("write_order"), ["vendor_dlkm", "system_dlkm", "boot"])
        require_equal("internal package release tag", internal_manifest.get("release_tag"), manifest["release_zip"]["package_release_tag"])
        for name, metadata in manifest["payloads"].items():
            expected = metadata["sha256"]
            require_equal(f"embedded {name}", sha256_bytes(archive.read(name)), expected)
            require_equal(f"internal manifest {name}", internal_manifest.get("payloads", {}).get(name, {}).get("sha256"), expected)
        require_equal("embedded helper", sha256_bytes(archive.read("tools/twrp-flash-controlled-stack")), manifest["twrp_helper_sha256"])
        verify_internal_sums(archive)


def verify_external_sums(path: pathlib.Path, required: dict[str, str]) -> None:
    if not path.is_file():
        die(f"missing external SHA256SUMS: {path}")
    sums: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        digest, name = line.split("  ", 1)
        sums[name] = digest
    for name, expected in required.items():
        require_equal(f"external checksum {name}", sums.get(name), expected)


def verify_release_commit(repo: pathlib.Path, commit: str, manifest: dict) -> None:
    if re.fullmatch(r"[0-9a-f]{40}", commit) is None:
        die("--release-commit must be a full lowercase Git object ID")
    resolved = subprocess.check_output(
        ["git", "-C", str(repo), "rev-parse", f"{commit}^{{commit}}"], text=True
    ).strip()
    require_equal("resolved release commit", resolved, commit)
    require_equal("manifest release commit", manifest.get("release_commit"), commit)
    parents = subprocess.check_output(
        ["git", "-C", str(repo), "show", "-s", "--format=%P", commit], text=True
    ).split()
    require_equal("release commit parent count", len(parents), 1)
    changed = subprocess.check_output(
        ["git", "-C", str(repo), "diff-tree", "--no-commit-id", "--name-only", "-r", commit], text=True
    ).splitlines()
    unexpected = [
        path for path in changed
        if not any(path == allowed or path.startswith(allowed) for allowed in ALLOWED_RELEASE_COMMIT_PATHS)
    ]
    if unexpected:
        die(f"release commit contains a non-release path: {unexpected[0]}")
    print(f"release_commit_paths={len(changed)}")
    print("image_producing_source_changes=0")
    print("vendor_runtime_source_changes=0")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=pathlib.Path)
    parser.add_argument("--signoff", required=True, type=pathlib.Path)
    parser.add_argument("--zip", type=pathlib.Path)
    parser.add_argument("--sha256sums", type=pathlib.Path)
    parser.add_argument("--release-commit")
    parser.add_argument("--repo", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument("--allow-symbolic-release-commit", action="store_true")
    args = parser.parse_args()

    manifest = read_object(args.manifest, "release manifest")
    signoff = read_object(args.signoff, "validation signoff")
    verify_metadata(manifest, signoff, args.allow_symbolic_release_commit)
    if args.zip:
        verify_zip(args.zip, manifest)
    if args.release_commit:
        verify_release_commit(args.repo.resolve(), args.release_commit, manifest)
    if args.sha256sums:
        if not args.zip:
            die("--sha256sums requires --zip")
        required = {
            args.zip.name: sha256_file(args.zip),
            "release-manifest.json": sha256_file(args.manifest),
            "validation-signoff.json": sha256_file(args.signoff),
        }
        verify_external_sums(args.sha256sums, required)
    print("CONTROLLED STACK RELEASE PREFLIGHT PASS")
    print(f"release_id={manifest['release_id']}")
    print(f"zip_verified={'yes' if args.zip else 'metadata_only'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
