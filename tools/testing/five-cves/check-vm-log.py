#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fail closed on guest failures; distinguish KASAN startup from diagnostics."""
import re
from pathlib import Path
import sys

FATAL = re.compile(
    r"BUG:|WARNING:|KASAN:|KCSAN:|Oops:|Kernel panic|possible circular locking|"
    r"RCU.*stall|refcount_t:|list_(add|del).*corrupt|blocked for more than", re.I
)
STARTUP = re.compile(r"\[\s*\d+\.\d+\]\s+kasan: KernelAddressSanitizer initialized", re.I)


def errors(text):
    lines = text.replace("\r", "").splitlines()
    result = [line for line in lines if FATAL.search(line) and not STARTUP.fullmatch(line)]
    if "FIVE_CVES_RESULT: PASS" not in lines:
        result.append("Missing exact guest PASS marker")
    if "FIVE_CVES_DMESG_END" not in lines:
        result.append("Incomplete final diagnostic capture")
    if not any(STARTUP.fullmatch(line) for line in lines):
        result.append("KASAN initialization not observed")
    return result


def main():
    if sys.argv[1:] == ["--self-test"]:
        clean = "[ 0.123456] kasan: KernelAddressSanitizer initialized\nFIVE_CVES_DMESG_END\nFIVE_CVES_RESULT: PASS\n"
        assert not errors(clean)
        bad = ("BUG: bad page", "WARNING: invalid state", "KASAN: use-after-free",
               "KCSAN: data-race", "Oops: fault", "Kernel panic - not syncing",
               "possible circular locking dependency", "RCU detected stall",
               "refcount_t: underflow", "list_del corruption", "blocked for more than 120 seconds",
               "BUG: kasan: KernelAddressSanitizer initialized")
        for line in bad:
            assert errors(clean + line + "\n"), line
        for marker in ("FIVE_CVES_RESULT: PASS", "FIVE_CVES_DMESG_END", "[ 0.123456] kasan: KernelAddressSanitizer initialized"):
            assert errors(clean.replace(marker, ""))
        print("PASS: VM log scanner, 16 positive/negative cases")
        return
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-vm-log.py SERIAL_LOG | --self-test")
    problems = errors(Path(sys.argv[1]).read_text(errors="replace"))
    if problems:
        raise SystemExit("FAIL: " + "\n".join(problems))
    print("PASS: complete guest results; KASAN active; no fatal/sanitizer/locking diagnostic")


if __name__ == "__main__":
    main()
