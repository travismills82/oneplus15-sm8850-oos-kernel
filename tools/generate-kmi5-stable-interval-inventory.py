#!/usr/bin/env python3
"""Generate a reproducible pre-scan for one Linux stable point interval.

The report deliberately calls header/export changes review candidates rather
than claiming ABI safety.  Patch applicability is tested against the selected
monorepo baseline with the stable paths rooted at kernel_platform/common.
"""

from __future__ import annotations

import argparse
import csv
import subprocess
from pathlib import Path


SENSITIVE_PREFIXES = {
    "block/": "block",
    "drivers/block/": "block",
    "drivers/cpufreq/": "cpufreq",
    "drivers/gpu/drm/": "DRM",
    "drivers/md/": "device-mapper",
    "drivers/net/": "networking",
    "drivers/scsi/": "SCSI/UFS",
    "drivers/thermal/": "thermal",
    "drivers/ufs/": "UFS",
    "drivers/usb/": "USB",
    "fs/crypto/": "fscrypt",
    "fs/f2fs/": "F2FS",
    "include/linux/blk": "block ABI",
    "include/linux/cpufreq": "cpufreq ABI",
    "include/linux/device-mapper": "device-mapper ABI",
    "include/linux/fscrypt": "fscrypt ABI",
    "include/linux/usb": "USB ABI",
    "include/net/": "networking ABI",
    "kernel/bpf/": "BPF",
    "kernel/power/": "power",
    "net/": "networking",
}

KNOWN_HIGH_RISK = {
    "e258ecf0c2a8f621ca2d08bbce6ceff4820cc762": (
        "PRIVATE_OEM_ABI_RISK",
        "Known OnePlus FBE-sensitive DM queue-state change; preserve upstream "
        "commit and require separate semantic reconciliation plus physical oracle",
    ),
    "19ca4528666990be376ac3eb6fe667b03db5324d": (
        "PRIVATE_OEM_ABI_RISK",
        "Known OnePlus FBE-sensitive DM suspend change; preserve upstream commit "
        "and require separate semantic reconciliation plus physical oracle",
    ),
}


def run(repo: Path, args: list[str], *, data: bytes | None = None,
        check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        input=data,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=check,
    )


def text(repo: Path, args: list[str]) -> str:
    return run(repo, args).stdout.decode("utf-8", "replace").rstrip("\n")


def classify_subsystem(files: list[str]) -> str:
    found: list[str] = []
    for name in files:
        for prefix, label in SENSITIVE_PREFIXES.items():
            if name.startswith(prefix) and label not in found:
                found.append(label)
    if found:
        return ",".join(found)
    first = files[0].split("/", 1)[0] if files else "metadata"
    return first


def classify_abi(commit: str, files: list[str], patch: str) -> tuple[str, str]:
    if commit in KNOWN_HIGH_RISK:
        return KNOWN_HIGH_RISK[commit]
    exported = "EXPORT_SYMBOL" in patch or "EXPORT_TRACEPOINT_SYMBOL" in patch
    public_header = any(
        name.startswith(("include/", "arch/arm64/include/")) for name in files
    )
    vendor_hook = any("vendor_hook" in name or "trace/hooks" in name for name in files)
    storage = any(
        name.startswith(("fs/crypto/", "fs/f2fs/", "block/", "drivers/md/",
                         "drivers/scsi/", "drivers/ufs/"))
        for name in files
    )
    if vendor_hook:
        return "KMI5_SHIM_REQUIRED_REVIEW", "Android vendor-hook surface changed"
    if exported or public_header:
        return "ABI_REVIEW_REQUIRED", "Export or module-visible header changed"
    if storage:
        return "PRIVATE_OEM_ABI_RISK", "Boot/FBE-sensitive implementation changed"
    return "INTERNAL_ONLY_CANDIDATE", "No exported symbol or public header detected"


def applicability(baseline: Path, patch: bytes) -> tuple[str, str, str]:
    common_prefix = "kernel_platform/common"
    forward = run(
        baseline,
        ["apply", "--check", f"--directory={common_prefix}", "-"],
        data=patch,
        check=False,
    )
    reverse = run(
        baseline,
        ["apply", "--reverse", "--check", f"--directory={common_prefix}", "-"],
        data=patch,
        check=False,
    )
    if reverse.returncode == 0 and forward.returncode != 0:
        return "YES", "ALREADY_PRESENT", "Patch reverses cleanly from baseline"
    if forward.returncode == 0 and reverse.returncode != 0:
        return "NO", "IMPORT", "Patch applies cleanly to baseline snapshot"
    if forward.returncode == 0 and reverse.returncode == 0:
        return "REVIEW", "MANUAL_REVIEW", "Patch applies in both directions"
    reason = forward.stderr.decode("utf-8", "replace").splitlines()
    detail = reason[0] if reason else "Patch does not apply cleanly"
    return "REVIEW", "MANUAL_REVIEW", detail


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stable-repo", required=True, type=Path)
    parser.add_argument("--baseline-repo", required=True, type=Path)
    parser.add_argument("--from-commit", required=True)
    parser.add_argument("--to-commit", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--metadata-only",
        action="store_true",
        help="use one batched log scan and defer patch applicability to replay",
    )
    args = parser.parse_args()

    metadata: list[tuple[str, str, str, list[str]]] = []
    if args.metadata_only:
        marker = "__KMI5_COMMIT__"
        raw = text(
            args.stable_repo,
            [
                "log", "--reverse", "--topo-order", "--no-merges",
                f"--format={marker}%H%x1f%an <%ae>%x1f%s", "--name-only",
                f"{args.from_commit}..{args.to_commit}",
            ],
        )
        current: tuple[str, str, str, list[str]] | None = None
        for line in raw.splitlines():
            if line.startswith(marker):
                if current is not None:
                    metadata.append(current)
                commit, author, subject = line[len(marker):].split("\x1f", 2)
                current = (commit, author, subject, [])
            elif line and current is not None:
                current[3].append(line)
        if current is not None:
            metadata.append(current)
    else:
        commits = text(
            args.stable_repo,
            ["rev-list", "--reverse", "--topo-order", "--no-merges",
             f"{args.from_commit}..{args.to_commit}"],
        ).splitlines()
        for commit in commits:
            metadata.append((
                commit,
                text(args.stable_repo, ["show", "-s", "--format=%an <%ae>", commit]),
                text(args.stable_repo, ["show", "-s", "--format=%s", commit]),
                text(
                    args.stable_repo,
                    ["diff-tree", "--no-commit-id", "--name-only", "-r", commit],
                ).splitlines(),
            ))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, delimiter="\t", lineterminator="\n")
        writer.writerow([
            "order", "commit", "author", "subject", "files", "subsystem",
            "abi_classification", "already_present", "action", "reason",
        ])
        for index, (commit, author, subject, files) in enumerate(metadata, start=1):
            if args.metadata_only:
                patch = b""
                patch_text = ""
            else:
                patch = run(args.stable_repo, ["diff", f"{commit}^", commit, "--binary"]).stdout
                patch_text = patch.decode("utf-8", "replace")
            abi_class, abi_reason = classify_abi(commit, files, patch_text)
            if args.metadata_only:
                already, action = "REVIEW", "IMPORT_PENDING"
                apply_reason = "Applicability is evaluated in ordered replay"
            else:
                already, action, apply_reason = applicability(args.baseline_repo, patch)
            reason = f"{abi_reason}; {apply_reason}"
            writer.writerow([
                index,
                commit,
                author,
                subject,
                ",".join(files),
                classify_subsystem(files),
                abi_class,
                already,
                action,
                reason,
            ])

    print(f"wrote {len(metadata)} commits to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
