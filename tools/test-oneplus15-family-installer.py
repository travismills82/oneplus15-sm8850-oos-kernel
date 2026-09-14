#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only

"""Exercise the fail-closed OnePlus 15 family property guard offline."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UPDATER = ROOT / "tools/twrp-installer-template/META-INF/com/google/android/update-binary.in"

CASES: tuple[tuple[str, bool, dict[str, str]], ...] = (
    (
        "CPH2747",
        True,
        {"ro.product.model": "CPH2747", "ro.product.device": "infiniti", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "marketing-name",
        True,
        {"ro.product.model": "OnePlus 15", "ro.product.device": "infiniti", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_b"},
    ),
    (
        "CPH2745-fingerprint",
        True,
        {"ro.product.model": "OnePlus 15", "ro.product.device": "infiniti", "ro.product.board": "canoe", "ro.build.fingerprint": "OnePlus/CPH2745IN/OP611FL1:16/test", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "CPH2749",
        True,
        {"ro.product.model": "CPH2749", "ro.product.device": "infiniti", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "PLK110",
        True,
        {"ro.product.model": "PLK110", "ro.product.device": "infiniti", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "missing-regional-model",
        True,
        {"ro.product.model": "unknown", "ro.product.device": "infiniti", "ro.hardware": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "audited-stock-alias",
        True,
        {"ro.product.model": "OnePlus 15", "ro.product.device": "OP611FL1", "ro.product.board": "canoe", "ro.boot.prjname": "24863", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "wrong-device",
        False,
        {"ro.product.model": "Other Phone", "ro.product.device": "other", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "same-soc-wrong-phone",
        False,
        {"ro.product.model": "Other SM8850", "ro.product.device": "other", "ro.product.board": "canoe", "ro.hardware": "sm8850", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "conflicting-platform",
        False,
        {"ro.product.model": "OnePlus 15", "ro.product.device": "infiniti", "ro.product.board": "not-canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "model-only",
        False,
        {"ro.product.model": "CPH2747", "ro.product.board": "canoe", "ro.boot.slot_suffix": "_a"},
    ),
    (
        "missing-platform",
        False,
        {"ro.product.model": "OnePlus 15", "ro.product.device": "infiniti", "ro.boot.slot_suffix": "_a"},
    ),
)


def extract_guard(source: str) -> str:
    start = source.index("family_prop() {")
    end = source.index("\nresolve_boot_partition() {", start)
    return source[start:end]


def main() -> int:
    guard = extract_guard(UPDATER.read_text(encoding="utf-8"))
    harness = """#!/bin/sh
set -u
ui_print() { :; }
abort_install() { printf '%s\\n' "$*" >&2; exit 42; }
getprop() {
    awk -F= -v wanted="$1" '$1 == wanted { print substr($0, index($0, "=") + 1); exit }' "$PROPFILE"
}
""" + guard + "\ndetect_oneplus15_family\n"

    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="op15-family-guard-") as directory:
        root = Path(directory)
        script = root / "guard.sh"
        script.write_text(harness, encoding="utf-8")
        script.chmod(0o755)
        for name, expected_pass, properties in CASES:
            propfile = root / f"{name}.properties"
            propfile.write_text(
                "".join(f"{key}={value}\n" for key, value in properties.items()),
                encoding="utf-8",
            )
            result = subprocess.run(
                ["sh", str(script)],
                env={"PATH": "/usr/bin:/bin", "PROPFILE": str(propfile)},
                capture_output=True,
                text=True,
                check=False,
            )
            passed = result.returncode == 0
            if passed != expected_pass:
                failures.append(
                    f"{name}: expected {'PASS' if expected_pass else 'FAIL'}, "
                    f"rc={result.returncode}, stderr={result.stderr.strip()!r}"
                )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"ONEPLUS15_FAMILY_PROPERTY_MATRIX=PASS ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
