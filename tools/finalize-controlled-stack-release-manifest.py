#!/usr/bin/env python3
"""Resolve a committed controlled-stack manifest to its exact tag commit."""

from __future__ import annotations

import argparse
import json
import pathlib
import re


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--template", required=True, type=pathlib.Path)
    parser.add_argument("--release-commit", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    if re.fullmatch(r"[0-9a-f]{40}", args.release_commit) is None:
        die("release commit must be a full lowercase Git object ID")
    if args.output.exists():
        die(f"refusing to overwrite output: {args.output}")

    manifest = json.loads(args.template.read_text(encoding="utf-8"))
    if manifest.get("release_commit") != "TAG_TARGET":
        die("manifest template does not contain the TAG_TARGET sentinel")
    manifest["release_commit"] = args.release_commit
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"FINALIZED RELEASE MANIFEST: {args.output}")
    print(f"release_commit={args.release_commit}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
