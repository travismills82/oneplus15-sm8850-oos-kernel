#!/usr/bin/env python3
"""Inventory the controlled Canoe WLAN/CNSS module generation.

The audit reads module ELF metadata without modifying its input tree.  It is
deliberately scoped to the active Peach-v2 path plus packaged modules that
share the cfg80211/CNSS contract.  Detailed import and export CRC tables are
emitted so a later source generation can be compared mechanically.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ModuleSpec:
    module: str
    filenames: tuple[str, ...]
    source_path: str
    build_target: str
    component: str
    role: str


MODULES = (
    ModuleSpec(
        "cfg80211",
        ("cfg80211.ko",),
        "kernel_platform/common/net/wireless",
        "//soc-repo:canoe_perf/net/wireless/cfg80211",
        "GKI wireless core",
        "controlled modular provider",
    ),
    ModuleSpec(
        "mac80211",
        ("mac80211.ko",),
        "kernel_platform/common/net/mac80211",
        "//soc-repo:canoe_perf/net/mac80211/mac80211",
        "GKI soft-MAC core",
        "controlled modular provider for wonder",
    ),
    ModuleSpec(
        "qca_cld3_peach_v2",
        ("qca_cld3_peach_v2.ko",),
        "vendor/qcom/opensource/wlan/qcacld-3.0",
        "//vendor/qcom/opensource/wlan/qcacld-3.0:canoe_perf_qca_cld_peach-v2",
        "qcacld-3.0 .037",
        "active Canoe WLAN driver",
    ),
    ModuleSpec(
        "cnss2",
        ("cnss2.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss2",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss2",
        "WLAN platform .037",
        "active stock CNSS2 provider",
    ),
    ModuleSpec(
        "cnss_prealloc",
        ("cnss_prealloc.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss_prealloc",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc",
        "WLAN platform .037",
        "active stock preallocation provider",
    ),
    ModuleSpec(
        "cnss_utils",
        ("cnss_utils.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss_utils",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_utils",
        "WLAN platform .037",
        "active stock CNSS utility provider",
    ),
    ModuleSpec(
        "cnss_nl",
        ("cnss_nl.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss_genl",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_nl",
        "WLAN platform .037",
        "active stock CNSS generic-netlink provider",
    ),
    ModuleSpec(
        "wlan_firmware_service",
        ("wlan_firmware_service.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss_utils",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_wlan_firmware_service",
        "WLAN platform .037",
        "active stock WLAN firmware QMI provider",
    ),
    ModuleSpec(
        "cnss_plat_ipc_qmi_svc",
        ("cnss_plat_ipc_qmi_svc.ko",),
        "vendor/qcom/opensource/wlan/platform/cnss_utils",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_plat_ipc_qmi_svc",
        "WLAN platform .037",
        "active stock CNSS platform IPC provider",
    ),
    ModuleSpec(
        "icnss2",
        ("icnss2.ko",),
        "vendor/qcom/opensource/wlan/platform/icnss2",
        "//vendor/qcom/opensource/wlan/platform:canoe_perf_icnss2",
        "WLAN platform .037",
        "packaged but not loaded on the observed Peach-v2 path",
    ),
    ModuleSpec(
        "wonder",
        ("wonder.ko",),
        "vendor/oplus/kernel/wifi/wonder",
        "//vendor/oplus/kernel/wifi:oplus_wifi",
        "Oplus WLAN companion",
        "active exact-stock payload re-signed for controlled cfg80211",
    ),
    ModuleSpec(
        "smem_mailbox",
        ("smem-mailbox.ko", "smem_mailbox.ko"),
        "vendor/qcom/opensource/data-kernel/drivers/smem-mailbox",
        "//vendor/qcom/opensource/data-kernel/drivers/smem-mailbox:canoe_perf_smem_mailbox",
        "shared data provider",
        "active exact-stock provider used by cnss_utils",
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--module-root", type=Path, required=True)
    parser.add_argument("--runtime-modules", type=Path)
    parser.add_argument("--actions-tsv", type=Path)
    parser.add_argument("--generation", required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    return parser.parse_args()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_elf_reader(repo_root: Path):
    source = repo_root / "tools/validate-matched-wlan-vendor-dlkm.py"
    spec = importlib.util.spec_from_file_location("matched_wlan_validator", source)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot load ELF reader from {source}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def modinfo(path: Path, field: str) -> list[str]:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode:
        return []
    return [line for line in result.stdout.splitlines() if line]


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def actions(path: Path | None) -> dict[str, str]:
    if path is None:
        return {}
    with path.open(encoding="utf-8", newline="") as source:
        return {row["module"]: row["action"] for row in csv.DictReader(source, delimiter="\t")}


def runtime_names(path: Path | None) -> set[str]:
    if path is None:
        return set()
    return {
        line.split()[0]
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    }


def find_module(root: Path, filenames: tuple[str, ...]) -> Path:
    for filename in filenames:
        matches = sorted(root.rglob(filename))
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            raise ValueError(f"duplicate payload name {filename}: {matches}")
    raise ValueError(f"missing module payload: {', '.join(filenames)}")


def main() -> int:
    args = parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    elf_reader = load_elf_reader(args.repo_root)
    action_by_name = actions(args.actions_tsv)
    loaded = runtime_names(args.runtime_modules)

    summary_rows: list[list[object]] = []
    import_rows: list[list[object]] = []
    export_rows: list[list[object]] = []
    parameter_rows: list[list[object]] = []
    firmware_rows: list[list[object]] = []
    alias_rows: list[list[object]] = []

    for item in MODULES:
        path = find_module(args.module_root, item.filenames)
        elf = elf_reader.ElfModule(path)
        metadata = elf.modinfo()
        imports = elf.versions()
        exports = elf.exported_crcs()
        module_name = (metadata.get("name") or [item.module])[0]
        if module_name != item.module:
            raise ValueError(f"{path}: expected module {item.module}, found {module_name}")
        source_path = args.repo_root / item.source_path
        if not source_path.exists():
            raise ValueError(f"missing mapped source: {source_path}")

        summary_rows.append(
            [
                item.module,
                args.generation,
                "yes" if item.module in loaded else "no",
                action_by_name.get(item.module, "RETAIN_STOCK"),
                item.component,
                item.role,
                item.source_path,
                item.build_target,
                path,
                path.stat().st_size,
                sha256(path),
                ";".join(metadata.get("vermagic", [])),
                ";".join(modinfo(path, "signer")),
                ";".join(metadata.get("srcversion", [])),
                ";".join(metadata.get("depends", [])),
                len(imports),
                len(exports),
                len(metadata.get("parm", [])),
                len(metadata.get("firmware", [])),
            ]
        )
        for symbol, value in sorted(imports.items()):
            import_rows.append([item.module, symbol, f"0x{value:08x}"])
        for symbol, value in sorted(exports.items()):
            export_rows.append([item.module, symbol, f"0x{value:08x}"])
        for value in metadata.get("parm", []):
            name, _, description = value.partition(":")
            parameter_rows.append([item.module, name, description])
        for value in metadata.get("firmware", []):
            firmware_rows.append([item.module, value])
        for value in metadata.get("alias", []):
            alias_rows.append([item.module, value])

    write_tsv(
        args.out_dir / "module-contract.tsv",
        [
            "module",
            "generation",
            "loaded_on_observed_canoe_runtime",
            "candidate_action",
            "component",
            "role",
            "source_path",
            "build_target",
            "module_path",
            "bytes",
            "sha256",
            "vermagic",
            "signer",
            "srcversion",
            "depends",
            "import_count",
            "export_count",
            "parameter_count",
            "firmware_declaration_count",
        ],
        summary_rows,
    )
    write_tsv(args.out_dir / "imports.tsv", ["module", "symbol", "expected_crc"], import_rows)
    write_tsv(args.out_dir / "exports.tsv", ["module", "symbol", "export_crc"], export_rows)
    write_tsv(
        args.out_dir / "module-parameters.tsv",
        ["module", "parameter", "description"],
        parameter_rows,
    )
    write_tsv(
        args.out_dir / "firmware-declarations.tsv",
        ["module", "firmware"],
        firmware_rows,
    )
    write_tsv(args.out_dir / "aliases.tsv", ["module", "alias"], alias_rows)
    print(f"modules={len(summary_rows)}")
    print(f"imports={len(import_rows)}")
    print(f"exports={len(export_rows)}")
    print(f"parameters={len(parameter_rows)}")
    print(f"firmware_declarations={len(firmware_rows)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=__import__("sys").stderr)
        raise SystemExit(2)
