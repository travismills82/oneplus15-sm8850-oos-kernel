#!/usr/bin/env python3
"""Stream Linux stable patches with explicit source-commit provenance.

The output is suitable for ``git am --directory=kernel_platform/common``.
Each patch retains the upstream author, date, subject and body, and gains a
trailer naming the exact Linux stable commit represented by the patch.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


BOUNDARY = re.compile(rb"^From ([0-9a-f]{40}) Mon Sep 17 00:00:00 2001\n$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stable-repo", required=True, type=Path)
    parser.add_argument("--from-commit", required=True)
    parser.add_argument("--to-commit", required=True)
    args = parser.parse_args()

    proc = subprocess.Popen(
        [
            "git", "-C", str(args.stable_repo), "format-patch", "--stdout",
            "--no-signature", "--no-stat", "--full-index", "--no-renames",
            f"{args.from_commit}..{args.to_commit}",
        ],
        stdout=subprocess.PIPE,
    )
    assert proc.stdout is not None

    commit: str | None = None
    trailer_written = False
    output = sys.stdout.buffer
    for line in proc.stdout:
        match = BOUNDARY.match(line)
        if match:
            if commit is not None and not trailer_written:
                raise RuntimeError(f"patch {commit} contained no diff")
            commit = match.group(1).decode("ascii")
            trailer_written = False
        elif line.startswith(b"diff --git ") and commit and not trailer_written:
            output.write(f"\n(cherry picked from commit {commit})\n\n".encode())
            trailer_written = True
        output.write(line)

    returncode = proc.wait()
    if returncode:
        return returncode
    if commit is not None and not trailer_written:
        raise RuntimeError(f"patch {commit} contained no diff")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
