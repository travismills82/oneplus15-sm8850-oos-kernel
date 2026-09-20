#!/usr/bin/env python3
"""Replay a reviewed Linux stable interval into kernel_platform/common.

Every imported commit keeps its upstream author, author date, subject and body.
The authoritative stable SHA is appended as a cherry-pick trailer.  Inventory
rows marked ALREADY_PRESENT are recorded but are not duplicated.
"""

from __future__ import annotations

import argparse
import csv
import os
import subprocess
import sys
from pathlib import Path


def run(repo: Path, args: list[str], *, data: bytes | None = None,
        env: dict[str, str] | None = None, check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        input=data,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        check=check,
    )


def git_text(repo: Path, args: list[str]) -> str:
    return run(repo, args).stdout.decode("utf-8", "replace").rstrip("\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stable-repo", required=True, type=Path)
    parser.add_argument("--target-repo", required=True, type=Path)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--start-order", type=int, default=1)
    parser.add_argument("--stop-order", type=int)
    args = parser.parse_args()

    if git_text(args.target_repo, ["status", "--porcelain"]):
        print("target worktree must be clean before replay", file=sys.stderr)
        return 2

    with args.inventory.open(encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle, delimiter="\t"))

    for row in rows:
        order = int(row["order"])
        if order < args.start_order:
            continue
        if args.stop_order is not None and order > args.stop_order:
            break
        commit = row["commit"]
        if row["action"] == "ALREADY_PRESENT":
            print(f"{order:04d} {commit[:12]} ALREADY_PRESENT")
            continue

        patch = run(args.stable_repo, ["diff", f"{commit}^", commit, "--binary"]).stdout
        applied = run(
            args.target_repo,
            ["apply", "--index", "--whitespace=nowarn",
             "--directory=kernel_platform/common", "-"],
            data=patch,
            check=False,
        )
        if applied.returncode:
            print(
                f"stopped at order {order}, {commit}:\n"
                f"{applied.stderr.decode('utf-8', 'replace')}",
                file=sys.stderr,
            )
            return 3

        if run(args.target_repo, ["diff", "--cached", "--quiet"], check=False).returncode == 0:
            print(f"stopped at order {order}, {commit}: empty staged patch", file=sys.stderr)
            return 4

        author_name = git_text(args.stable_repo, ["show", "-s", "--format=%an", commit])
        author_email = git_text(args.stable_repo, ["show", "-s", "--format=%ae", commit])
        author_date = git_text(args.stable_repo, ["show", "-s", "--format=%aI", commit])
        message = git_text(args.stable_repo, ["show", "-s", "--format=%B", commit]).rstrip()
        trailer = f"(cherry picked from commit {commit})"
        if trailer not in message:
            message = f"{message}\n\n{trailer}\n"
        else:
            message = f"{message}\n"

        env = os.environ.copy()
        env["GIT_AUTHOR_NAME"] = author_name
        env["GIT_AUTHOR_EMAIL"] = author_email
        env["GIT_AUTHOR_DATE"] = author_date
        committed = run(
            args.target_repo,
            ["commit", "--no-gpg-sign", "--file=-"],
            data=message.encode(),
            env=env,
            check=False,
        )
        if committed.returncode:
            print(committed.stderr.decode("utf-8", "replace"), file=sys.stderr)
            return 5
        new_commit = git_text(args.target_repo, ["rev-parse", "HEAD"])
        print(f"{order:04d} {commit[:12]} -> {new_commit[:12]} {row['subject']}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
