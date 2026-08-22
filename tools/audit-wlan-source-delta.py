#!/usr/bin/env python3
"""Describe an official OnePlus WLAN snapshot delta without importing it.

The report is deliberately path/area based because OnePlus publishes each
product drop as a synchronized snapshot rather than as one commit per fix.
It does not misrepresent a file-area classification as fix provenance; exact
runtime ABI changes are established later from the built module ELF files.
"""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
from collections import Counter
from pathlib import Path


ROOT = "vendor/qcom/opensource/wlan/"
IMPORTED = (
    "fw-api/",
    "platform/",
    "qca-wifi-host-cmn/",
    "qcacld-3.0/",
)
RETAINED_GLUE = {
    "platform/target_variants.bzl",
    "platform/wlan_platform_modules.bzl",
    "qcacld-3.0/target_variants.bzl",
    "qcacld-3.0/wlan_qcacld3_modules.bzl",
}


def args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-repo", type=Path, required=True)
    parser.add_argument("--old", required=True)
    parser.add_argument("--new", required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    return parser.parse_args()


def git(repo: Path, *argv: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repo), *argv], check=True, text=True,
        stdout=subprocess.PIPE,
    ).stdout


def classify(path: str) -> str:
    lower = path.lower()
    name = Path(path).name.lower()
    if path.startswith("wlan-devicetree/"):
        return "DEVICE-SPECIFIC — DROP"
    if path.startswith("utils/"):
        return "BUILD ONLY"
    if name in {"build.bazel", "kbuild", "kconfig", "makefile"} or name.endswith((".bzl", "defconfig")):
        return "BUILD ONLY"
    if path.startswith("fw-api/") or "wmi_unified" in lower or "firmware" in lower:
        return "FIRMWARE INTERFACE"
    if any(token in lower for token in ("11be", "eht", "mlo", "multi_link", "multilink")):
        return "MLO/WIFI7"
    if any(token in lower for token in ("regulatory", "reg_", "regdb", "country", "dfs", "afc", "sar")):
        return "REGULATORY"
    if "roam" in lower:
        return "ROAMING"
    if any(token in lower for token in ("power", "suspend", "resume", "runtime_pm", "wow", "twt")):
        return "POWER/PM"
    if path.startswith("platform/") or "cnss" in lower:
        return "CNSS"
    if any(token in lower for token in ("crypto", "security", "auth", "pmf", "sae", "wpa")):
        return "SECURITY FIX"
    if any(token in lower for token in ("/dp_", "/hal_", "/hif_", "htt", "ce_", "rx_", "tx_")):
        return "DATA PATH"
    return "BUG FIX"


def decision(path: str) -> str:
    if path in RETAINED_GLUE:
        return "LOCAL BUILD GLUE ADAPTED"
    if path.startswith("wlan-devicetree/"):
        return "DROP — SHIPPING CANOE DTBO RETAINED"
    if path.startswith("utils/"):
        return "DROP — USERSPACE TEST UTILITY"
    if path.startswith(IMPORTED):
        return "IMPORT"
    return "OUT OF SCOPE"


def component(path: str) -> str:
    return path.split("/", 1)[0]


def main() -> int:
    ns = args()
    ns.out_dir.mkdir(parents=True, exist_ok=True)
    name_status = git(
        ns.reference_repo, "diff", "--name-status", "--find-renames",
        ns.old, ns.new, "--", ROOT,
    ).splitlines()
    numstat = {}
    for line in git(ns.reference_repo, "diff", "--numstat", ns.old, ns.new, "--", ROOT).splitlines():
        added, deleted, raw_path = line.split("\t", 2)
        numstat[raw_path.removeprefix(ROOT)] = (added, deleted)

    rows = []
    classes: Counter[str] = Counter()
    decisions: Counter[str] = Counter()
    statuses: Counter[str] = Counter()
    for line in name_status:
        fields = line.split("\t")
        status = fields[0]
        raw_path = fields[-1]
        path = raw_path.removeprefix(ROOT)
        category = classify(path)
        action = decision(path)
        added, deleted = numstat.get(path, ("-", "-"))
        rows.append([status, component(path), path, category, action, added, deleted])
        statuses[status[0]] += 1
        classes[category] += 1
        decisions[action] += 1

    with (ns.out_dir / "wlan053-source-delta.tsv").open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(["status", "component", "path", "classification", "decision", "added_lines", "deleted_lines"])
        writer.writerows(rows)

    with (ns.out_dir / "wlan053-dropped-device-tree.tsv").open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(["path", "classification", "reason"])
        for row in rows:
            if row[2].startswith("wlan-devicetree/"):
                writer.writerow([row[2], row[3], row[4]])

    summary = {
        "old": ns.old,
        "new": ns.new,
        "changed_paths": len(rows),
        "status_counts": dict(sorted(statuses.items())),
        "classification_counts": dict(sorted(classes.items())),
        "decision_counts": dict(sorted(decisions.items())),
        "classification_scope": "source-area heuristic; not per-fix provenance",
    }
    (ns.out_dir / "wlan053-source-delta-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
