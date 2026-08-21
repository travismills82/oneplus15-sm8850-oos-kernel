#!/usr/bin/env python3
"""Validate the controlled system-DLKM load policy and WLAN handoff.

The controlled-v1 TEST3 failure analysis proved that modules.load is runtime
delivery state, not incidental packaging metadata.  This validator is
deliberately fail closed and emits the reviewable TSV shipped with a package.
"""

from __future__ import annotations

import argparse
import pathlib
import sys


def die(message: str) -> "NoReturn":
    raise SystemExit(f"error: {message}")


def module_files(root: pathlib.Path) -> dict[str, pathlib.Path]:
    result: dict[str, pathlib.Path] = {}
    for path in root.rglob("*.ko"):
        if path.name in result:
            die(f"duplicate module basename below {root}: {path.name}")
        result[path.name] = path
    return result


def load_entries(path: pathlib.Path) -> list[str]:
    if not path.is_file():
        die(f"missing modules.load: {path}")
    entries = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        entry = raw.strip()
        if not entry or entry.startswith("#"):
            continue
        entries.append(pathlib.PurePosixPath(entry).name)
    if not entries:
        die(f"modules.load is empty: {path}")
    return entries


def dep_map(path: pathlib.Path) -> dict[str, list[str]]:
    if not path.is_file():
        die(f"missing modules.dep: {path}")
    result: dict[str, list[str]] = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip():
            continue
        if ":" not in raw:
            die(f"malformed modules.dep line {number}: {raw}")
        consumer, dependencies = raw.split(":", 1)
        name = pathlib.PurePosixPath(consumer.strip()).name
        if name in result:
            die(f"duplicate modules.dep provider row: {name}")
        result[name] = [
            pathlib.PurePosixPath(value).name for value in dependencies.split()
        ]
    return result


def builtin_names(path: pathlib.Path) -> set[str]:
    if not path.is_file():
        die(f"missing modules.builtin: {path}")
    return {
        pathlib.PurePosixPath(line.strip()).name
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--system-root", required=True, type=pathlib.Path)
    parser.add_argument("--modules-builtin", required=True, type=pathlib.Path)
    parser.add_argument("--vendor-root", type=pathlib.Path)
    parser.add_argument("--required", action="append", default=[])
    parser.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    system_root = args.system_root.resolve()
    system_modules = module_files(system_root)
    system_load = load_entries(system_root / "modules.load")
    system_deps = dep_map(system_root / "modules.dep")
    builtins = builtin_names(args.modules_builtin.resolve())

    required = {pathlib.PurePosixPath(value).name for value in args.required}
    absent_required = sorted(required - set(system_load))
    if absent_required:
        die("required system modules.load entries are absent: " + ", ".join(absent_required))

    rows: list[list[str]] = []
    failures: list[str] = []
    for order, entry in enumerate(system_load, 1):
        exists = entry in system_modules
        stale_builtin = entry in builtins
        dependencies = system_deps.get(entry)
        missing_dependencies = []
        if dependencies is None:
            missing_dependencies.append("NO_MODULES_DEP_ROW")
        else:
            missing_dependencies.extend(
                dependency
                for dependency in dependencies
                if dependency not in system_modules and dependency not in builtins
            )
        status = "PASS"
        notes = []
        if not exists:
            status = "FAIL"
            notes.append("LISTED_MODULE_MISSING")
        if stale_builtin:
            status = "FAIL"
            notes.append("STALE_BUILTIN_ENTRY")
        if missing_dependencies:
            status = "FAIL"
            notes.append("MISSING_DEPENDENCY:" + ",".join(missing_dependencies))
        if status == "FAIL":
            failures.append(f"system:{entry}:{';'.join(notes)}")
        rows.append(
            [
                "system_dlkm",
                str(order),
                entry,
                "yes" if exists else "no",
                "yes" if stale_builtin else "no",
                ",".join(dependencies or []),
                status,
                ";".join(notes) if notes else "ordered load entry is present and resolved",
            ]
        )

    # The current controlled system policy intentionally preloads every module
    # it delivers.  Enforce that reviewed contract so a future partial/empty
    # modules.load cannot silently recreate the missing-wwan failure.
    unlisted = sorted(set(system_modules) - set(system_load))
    if unlisted:
        failures.append("system:unlisted modules:" + ",".join(unlisted))

    if args.vendor_root:
        vendor_root = args.vendor_root.resolve()
        vendor_modules = module_files(vendor_root)
        vendor_load = load_entries(vendor_root / "modules.load")
        vendor_deps = dep_map(vendor_root / "modules.dep")
        for required_vendor in ("cfg80211.ko", "qca_cld3_peach_v2.ko"):
            if required_vendor not in vendor_modules:
                failures.append(f"vendor:{required_vendor}:payload missing")
            if required_vendor not in vendor_load:
                failures.append(f"vendor:{required_vendor}:modules.load missing")

        cfg_order = vendor_load.index("cfg80211.ko") + 1 if "cfg80211.ko" in vendor_load else 0
        peach_order = (
            vendor_load.index("qca_cld3_peach_v2.ko") + 1
            if "qca_cld3_peach_v2.ko" in vendor_load
            else 0
        )
        peach_deps = vendor_deps.get("qca_cld3_peach_v2.ko", [])
        cfg_dependency = "cfg80211.ko" in peach_deps
        handoff_ok = bool(cfg_order and peach_order and cfg_order < peach_order and cfg_dependency)
        if not handoff_ok:
            failures.append("vendor:cfg80211-to-peach load/dependency handoff invalid")
        rows.append(
            [
                "vendor_dlkm",
                str(cfg_order),
                "cfg80211.ko",
                "yes" if "cfg80211.ko" in vendor_modules else "no",
                "no",
                "",
                "PASS" if handoff_ok else "FAIL",
                f"first cfg80211 order={cfg_order}; first Peach-v2 order={peach_order}",
            ]
        )
        rows.append(
            [
                "vendor_dlkm",
                str(peach_order),
                "qca_cld3_peach_v2.ko",
                "yes" if "qca_cld3_peach_v2.ko" in vendor_modules else "no",
                "no",
                ",".join(peach_deps),
                "PASS" if handoff_ok else "FAIL",
                "modules.dep names cfg80211.ko and load policy places cfg80211 first",
            ]
        )

    args.out.parent.mkdir(parents=True, exist_ok=True)
    header = [
        "partition",
        "load_order",
        "module",
        "payload_exists",
        "stale_builtin_entry",
        "hard_dependencies",
        "status",
        "reason",
    ]
    with args.out.open("w", encoding="utf-8", newline="") as stream:
        stream.write("\t".join(header) + "\n")
        for row in rows:
            stream.write("\t".join(row) + "\n")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    print(
        f"PASS: {len(system_load)} system load entries; "
        f"wwan={'yes' if 'wwan.ko' in system_load else 'no'}; "
        f"stale=0; missing=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
