#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fail-closed verifier for an immutable controlled-v1 kernel contract."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
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


def require_hash(label: str, path: pathlib.Path, expected: str) -> None:
    if not path.is_file():
        die(f"missing {label}: {path}")
    actual = sha256(path)
    if actual != expected:
        die(f"{label} changed: expected {expected}, got {actual}")


def require_functionally_identical(
    label: str,
    qualified: pathlib.Path,
    candidate: pathlib.Path,
    ignored_ranges: list[dict[str, object]],
) -> None:
    if not qualified.is_file() or not candidate.is_file():
        die(f"missing {label} functional-comparison input")
    qualified_data = qualified.read_bytes()
    candidate_data = candidate.read_bytes()
    if len(qualified_data) != len(candidate_data):
        die(f"{label} size changed")
    allowed: set[int] = set()
    for item in ignored_ranges:
        start = int(item["offset"])
        size = int(item["size"])
        if start < 0 or size <= 0 or start + size > len(candidate_data):
            die(f"invalid {label} metadata tolerance: {item}")
        allowed.update(range(start, start + size))
    differences = {
        index
        for index, (before, after) in enumerate(zip(qualified_data, candidate_data))
        if before != after
    }
    outside = differences - allowed
    if outside:
        die(f"{label} functional byte changed at offset {min(outside)}")
    unused = allowed - differences
    if unused:
        die(f"{label} metadata tolerance no longer describes the build at offset {min(unused)}")
    print(f"{label}_functional_differences=0")
    print(f"{label}_metadata_differences={len(differences)}")


def artifact_paths(kernel_build_dir: pathlib.Path, canoe_config: pathlib.Path) -> dict[str, pathlib.Path]:
    return {
        "config_sha256": pathlib.Path(f"{kernel_build_dir}_config/out_dir/.config"),
        "canoe_config_sha256": canoe_config,
        "module_symvers_sha256": kernel_build_dir / "Module.symvers",
        "system_map_sha256": kernel_build_dir / "System.map",
        "modules_builtin_sha256": kernel_build_dir / "modules.builtin",
        "vmlinux_sha256": kernel_build_dir / "vmlinux",
        "image_sha256": kernel_build_dir / "Image",
    }


def reconstruct_aquery_inputs(path: pathlib.Path) -> list[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    actions = [action for action in data.get("actions", []) if action.get("mnemonic") == "KernelBuild"]
    if len(actions) != 1:
        die(f"expected one KernelBuild action in {path}, found {len(actions)}")
    fragments = {int(item["id"]): item for item in data["pathFragments"]}
    artifacts = {int(item["id"]): int(item["pathFragmentId"]) for item in data["artifacts"]}
    dep_sets = {int(item["id"]): item for item in data["depSetOfFiles"]}

    def fragment_path(fragment_id: int) -> str:
        labels: list[str] = []
        while fragment_id:
            fragment = fragments[fragment_id]
            labels.append(fragment["label"])
            fragment_id = int(fragment.get("parentId", 0))
        return "/".join(reversed(labels))

    seen: set[int] = set()
    artifact_ids: set[int] = set()
    pending = [int(item) for item in actions[0].get("inputDepSetIds", [])]
    while pending:
        dep_set_id = pending.pop()
        if dep_set_id in seen:
            continue
        seen.add(dep_set_id)
        dep_set = dep_sets[dep_set_id]
        artifact_ids.update(int(item) for item in dep_set.get("directArtifactIds", []))
        pending.extend(int(item) for item in dep_set.get("transitiveDepSetIds", []))
    return sorted(fragment_path(artifacts[item]) for item in artifact_ids)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", required=True, type=pathlib.Path)
    parser.add_argument("--kernel-build-dir", required=True, type=pathlib.Path)
    parser.add_argument("--canoe-config", required=True, type=pathlib.Path)
    parser.add_argument("--qualified-dist", required=True, type=pathlib.Path)
    parser.add_argument("--aquery-json", type=pathlib.Path)
    parser.add_argument("--build-input-delta", type=pathlib.Path)
    parser.add_argument("--repo-root", type=pathlib.Path)
    args = parser.parse_args()

    contract = json.loads(args.contract.read_text(encoding="utf-8"))
    release_file = args.kernel_build_dir / "include/config/kernel.release"
    if not release_file.is_file():
        die(f"missing kernel.release: {release_file}")
    actual_release = release_file.read_text(encoding="utf-8").strip()
    if actual_release != contract["release"]:
        die(f"kernel.release changed: expected {contract['release']}, got {actual_release}")

    for label, path in artifact_paths(args.kernel_build_dir, args.canoe_config).items():
        if label in ("image_sha256", "vmlinux_sha256"):
            continue
        require_hash(label.removesuffix("_sha256"), path, contract["artifacts"][label])

    require_hash(
        "qualified Image",
        args.qualified_dist / "Image",
        contract["artifacts"]["image_sha256"],
    )
    require_hash(
        "qualified vmlinux",
        args.qualified_dist / "vmlinux",
        contract["artifacts"]["vmlinux_sha256"],
    )
    for label, filename in (("Image", "Image"), ("vmlinux", "vmlinux")):
        require_functionally_identical(
            label,
            args.qualified_dist / filename,
            args.kernel_build_dir / filename,
            contract["functional_comparison"][label]["ignored_ranges"],
        )

    signing_x509 = args.kernel_build_dir / "certs/signing_key.x509"
    require_hash(
        "module signing certificate",
        signing_x509,
        contract["signing"]["project_certificate_der_sha256"],
    )

    inputs: list[str] = []
    if args.aquery_json:
        inputs = reconstruct_aquery_inputs(args.aquery_json)
        forbidden = [path for path in inputs if path.startswith("vendor/qcom/opensource/bt-kernel/")]
        if forbidden:
            die("Bluetooth DDK source entered the KernelBuild action input set")
        allowed_roots = ("common/", "build/", "external/", "prebuilts/", "bazel-out/")
        unexpected = [path for path in inputs if not path.startswith(allowed_roots)]
        if unexpected:
            die(f"unreviewed KernelBuild input root: {unexpected[0]}")

    if args.build_input_delta:
        if not args.repo_root:
            die("--build-input-delta requires --repo-root")
        qualified = contract["qualified_branch_head"]
        completed = subprocess.run(
            ["git", "-C", str(args.repo_root), "diff", "--name-only", qualified, "HEAD"],
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        )
        changed = [line for line in completed.stdout.splitlines() if line]
        contract_prefixes = (
            "kernel_platform/common/",
            "kernel_platform/build/",
            "kernel_platform/external/",
            "kernel_platform/soc-repo/BUILD.bazel",
            "kernel_platform/soc-repo/bazel.WORKSPACE",
            "kernel_platform/soc-repo/device.bazelrc",
            "kernel_platform/soc-repo/kleaf-scripts/android_build.bzl",
            "kernel_platform/soc-repo/kleaf-scripts/modules_register.bzl",
            "kernel_platform/soc-repo/kleaf-scripts/targets/canoe.bzl",
        )
        rows = ["path\tbuild_input\tdifference\treason"]
        blockers: list[str] = []
        for path in changed:
            is_contract_input = path.startswith(contract_prefixes)
            if is_contract_input:
                blockers.append(path)
            reason = "kernel contract input" if is_contract_input else "external module source or metadata"
            rows.append(f"{path}\t{'yes' if is_contract_input else 'no'}\tyes\t{reason}")
        args.build_input_delta.parent.mkdir(parents=True, exist_ok=True)
        args.build_input_delta.write_text("\n".join(rows) + "\n", encoding="utf-8")
        if blockers:
            die(f"kernel contract input changed since qualified baseline: {blockers[0]}")

    print("KERNEL CONTRACT GUARD PASS")
    print(f"kernel_release={actual_release}")
    print(f"kernel_build_action_inputs={len(inputs)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
