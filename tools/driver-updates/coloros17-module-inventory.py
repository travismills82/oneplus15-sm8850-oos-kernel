#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Inventory exact ColorOS .102 stock modules without changing binary inputs.

The existing loader-oriented ELF parser supplies actual ksymtab exports and
MODVERSIONS data. Source/build-file associations are candidates only: neither
name matching, vermagic, a build ID nor a matching CRC proves source identity.
No device access, image extraction, module loading or signature signing occurs.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.dont_write_bytecode = True

EXPECTED_PARTITIONS = {"system_dlkm": 103, "vendor_dlkm": 461, "vendor_boot": 506}
METADATA_NAMES = (
    "modules.load", "modules.load.recovery", "modules.dep", "modules.alias",
    "modules.softdep", "modules.blocklist", "system_dlkm.modules.blocklist",
    "modules.builtin", "modules.builtin.modinfo", "modules.options",
)
ROOTS = {
    "system_dlkm": "modules/system_dlkm",
    "vendor_dlkm": "modules/vendor_dlkm",
    "vendor_boot": "vendor-boot-ramdisk",
}
IMAGE_PATHS = {
    "system_dlkm": "system/system_dlkm.img",
    "vendor_dlkm": "vendor/vendor_dlkm.img",
    "vendor_boot": "critical/vendor_boot.img",
}
FIRMWARE_HASHES = {
    "system_dlkm": "3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a",
    "vendor_dlkm": "82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47",
    "vendor_boot": "d0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000",
}
COMMON_REPOSITORY = "https://android.googlesource.com/kernel/common"
SOC_REPOSITORY = "https://github.com/OnePlusOSS/android_kernel_oneplus_sm8850.git"
MODULES_REPOSITORY = "https://github.com/OnePlusOSS/android_kernel_modules_and_devicetree_oneplus_sm8850.git"
# Repository associations identify public comparison sources only. They do not
# attribute a shipping binary to that source or choose a firmware release tag.
CLO_PATH_REFERENCES = {
    "vendor/qcom/opensource/wlan/qcacld-3.0/": "WLAN qcacld",
    "vendor/qcom/opensource/wlan/qca-wifi-host-cmn/": "WLAN host-cmn",
    "vendor/qcom/opensource/wlan/platform/": "WLAN platform",
    "vendor/qcom/opensource/wlan/fw-api/": "WLAN fw-api",
    "vendor/qcom/opensource/video-driver/": "Video",
    "vendor/qcom/opensource/touch-drivers/": "Touch",
    "vendor/qcom/opensource/spu-kernel/": "SPU",
    "vendor/qcom/opensource/securemsm-kernel/": "SecureMSM",
    "vendor/qcom/opensource/mm-drivers/": "MM drivers",
    "vendor/qcom/opensource/graphics-kernel/": "Graphics",
    "vendor/qcom/opensource/dsp-kernel/": "DSP",
    "vendor/qcom/opensource/display-drivers/": "Display",
    "vendor/qcom/opensource/datarmnet-ext/": "Data RMNET ext",
    "vendor/qcom/opensource/datarmnet/": "Data RMNET",
    "vendor/qcom/opensource/dataipa/": "Data IPA",
    "vendor/qcom/opensource/camera-kernel/": "Camera",
    "vendor/qcom/opensource/audio-kernel/": "Audio",
    "vendor/qcom/opensource/bt-kernel/": "Bluetooth",
    "vendor/qcom/opensource/data-kernel/": "Data kernel",
    "vendor/qcom/opensource/fingerprint/": "Fingerprint",
    "vendor/qcom/opensource/eva-kernel/": "EVA",
    "vendor/qcom/opensource/mmrm-driver/": "MMRM",
    "vendor/qcom/opensource/synx-kernel/": "Synx",
    "vendor/nxp/opensource/driver/": "NXP",
}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def clean(value: object) -> str:
    return str(value).replace("\t", " ").replace("\r", " ").replace("\n", " ")


def canonical(name: str) -> str:
    return Path(name).name.removesuffix(".ko").replace("-", "_")


def build_id(elf) -> str:
    ids = []
    for section in elf.sections:
        if section.section_type != 7:
            continue
        data = elf.slice(section.offset, section.size)
        pos = 0
        while pos < len(data):
            if not any(data[pos:]):
                break
            if len(data) - pos < 12:
                raise ValueError("truncated ELF note header")
            namesz, descsz, kind = struct.unpack_from("<III", data, pos)
            pos += 12
            end_name = pos + namesz
            pos_desc = pos + ((namesz + 3) & ~3)
            end_desc = pos_desc + descsz
            end_note = pos_desc + ((descsz + 3) & ~3)
            if end_note > len(data):
                raise ValueError("truncated ELF note payload")
            if data[pos:end_name].rstrip(b"\0") == b"GNU" and kind == 3:
                ids.append(data[pos_desc:end_desc].hex())
            pos = end_note
    if len(set(ids)) > 1:
        raise ValueError("conflicting GNU build IDs")
    return ids[0] if ids else "ABSENT"


def parse_modinfo_output(output: bytes) -> dict[str, list[str]]:
    result = defaultdict(list)
    for record in output.split(b"\0"):
        # kmod's NUL mode emits metadata as key=value, but filename: value.
        # Values themselves (parameters/signature key IDs) may contain colons.
        text = record.decode("utf-8", "replace")
        separator = "=" if "=" in text else ":"
        if separator in text:
            key, value = text.split(separator, 1)
            result[key.strip()].append(value.strip())
    return result


def modinfo(path: Path) -> dict[str, list[str]]:
    output = subprocess.run(
        ["modinfo", "-0", str(path)], check=True, capture_output=True
    ).stdout
    return parse_modinfo_output(output)


def get_one(metadata, name, default="") -> str:
    return ";".join(metadata.get(name, [])) or default


def source_indexes(repo: Path):
    files = subprocess.run(
        ["git", "ls-files", "-z"], cwd=repo, check=True, capture_output=True
    ).stdout.decode().split("\0")
    by_file = defaultdict(set)
    by_target = defaultdict(set)
    targets = re.compile(r"(?<![A-Za-z0-9_])([A-Za-z0-9_-]+)\.(?:o|ko)(?![A-Za-z0-9_])")
    for filename in files:
        path = Path(filename)
        if not filename.startswith(("kernel_platform/common/", "kernel_platform/soc-repo/", "vendor/")):
            continue
        if path.suffix in {".c", ".rs"}:
            by_file[canonical(path.stem)].add(filename)
        if path.name not in {"Makefile", "Kbuild", "BUILD.bazel", "modules.bzl"}:
            continue
        source = repo / path
        if not source.is_file():
            continue
        text = source.read_text(errors="replace")
        for name in targets.findall(text):
            by_target[canonical(name)].add(filename)
    return by_file, by_target


def reference_repositories(repo: Path) -> dict[str, str]:
    """Read previously audited official URLs, without network/source guessing."""
    path = repo / "docs/driver-updates/coloros17-upstream-refs.tsv"
    with path.open() as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    result = {}
    for row in rows:
        if row["subsystem"] not in CLO_PATH_REFERENCES.values():
            continue
        url = row["repository"]
        if not url.startswith("https://git.codelinaro.org/clo/la/platform/vendor/"):
            raise ValueError("comparison source is not an audited official CodeLinaro URL")
        if row["subsystem"] in result:
            raise ValueError("ambiguous comparison repository mapping")
        result[row["subsystem"]] = url
    if set(CLO_PATH_REFERENCES.values()) != set(result):
        raise ValueError("missing audited CodeLinaro repository mapping")
    return result


def source_repository_candidates(paths: list[str], repositories: dict[str, str]) -> str:
    candidates = set()
    for path in paths:
        if path.startswith("kernel_platform/common/"):
            candidates.add(COMMON_REPOSITORY)
        elif path.startswith("kernel_platform/soc-repo/"):
            candidates.add(SOC_REPOSITORY)
        elif path.startswith("vendor/"):
            candidates.add(MODULES_REPOSITORY)
            for prefix, reference in CLO_PATH_REFERENCES.items():
                if path.startswith(prefix):
                    candidates.add(repositories[reference])
    return ";".join(sorted(candidates)) or "UNKNOWN"


def provenance_fields(name: str, paths: list[str], metadata, repositories) -> dict[str, str]:
    firmware = get_one(metadata, "firmware")
    review = "NO_PER_MODULE_PRIVATE_HEADER_REVIEW"
    if name.startswith("rmnet_") or name == "ipam":
        review = "RELATED_PARTIAL_REVIEW:docs/driver-updates/coloros17-rmnet-private-abi.md"
    elif name == "dwc3_msm":
        review = "RELATED_PARTIAL_REVIEW:docs/driver-updates/coloros17-build-and-packaging-gates.md#private-abi-hard-gate-dwc3"
    return {
        "original_source_repository": "UNVERIFIED_FOR_STOCK_BINARY",
        "source_repository_candidates": source_repository_candidates(paths, repositories),
        "source_repository_candidate_evidence": "LOCAL_PATH_ASSOCIATION_ONLY_NOT_EXACT_SOURCE" if paths else "NO_LOCAL_PATH_CANDIDATE",
        "firmware_modinfo_declarations": firmware or "NOT_DECLARED_NOT_PROOF_NONE",
        "firmware_dependency_status": "DECLARED_FILENAMES_ONLY_RUNTIME_REQUIREMENTS_UNVERIFIED" if firmware else "NOT_DECLARED_RUNTIME_DEPENDENCIES_UNVERIFIED",
        "private_header_dependency_status": "UNVERIFIED_EXACT_SOURCE",
        "private_header_boundary_review": review,
    }


def subsystem(name: str, paths: list[str]) -> str:
    """Review routing, not a source-match or runtime-functionality claim."""
    joined = " ".join(paths).lower()
    rules = (
        ("WLAN", r"qca_cld|cnss|wlan|wificap|wifi_|^mac80211$|^cfg80211$", "/wlan/"),
        ("BLUETOOTH", r"^bluetooth$|^hidp$|^rfcomm$|^bt|^hci_uart$|^rfkill$", "/bt-kernel/"),
        ("GPU", r"kgsl|adreno|gpucc|gpufreq", "/graphics-kernel/"),
        ("DISPLAY_DRM", r"drm|dsi|display|dispcc|panel|hdmi|lt9611", "/display-drivers/"),
        ("CAMERA", r"camera|camcc|cambist|leds_qcom_flash", "/camera-kernel/"),
        ("AUDIO", r"audio|^snd|^wcd|^wsa|^swr|^lpass|^gpr|^spf|^q6_", "/audio-kernel/"),
        ("VIDEO_HFI", r"msm_video|hfi|videocc|msm_eva|evacc", "/video-driver/"),
        ("DSP", r"adsprpc|fastrpc|adsp|cdsp|qcom_q6|^pdr|qcom_pdr", "/dsp-kernel/"),
        ("DATAIPA_RMNET_NETWORK", r"^ipa|^rmnet|^qmi|network_|netlink|^ppp|^pptp$|^slhc$|^l2tp", "/datarmnet"),
        ("SECUREMSM_SPU", r"^si_core|qcom_scm|smcinvoke|qsee|secure|^spcom|^spss|qcedev|qcrypto|^tme|hdcp", "/securemsm-kernel/"),
        ("TOUCH_FINGERPRINT", r"^oplus_.*(?:_tp_|_fp_|touch|synaptics)|goodix|^st_fts|^qbt|^atmel_mxt|^qts$", "/touchpanel/"),
        ("USB_TYPEC", r"usb|dwc3|typec|ucsi|altmode|redriver|eusb|nb7vpq|^cdc_|^r815|^asix|ax88179|^aqc111", "/usb/"),
        ("UFS_STORAGE", r"ufs|sdhci|^mmc|storage_", "/ufs/"),
        ("POWER_THERMAL", r"thermal|charger|^oplus_chg|wireless_pen|^qcom_lpm|^rpm|^cpufreq|^cpuidle|^clk|regulator|^pmic|battery|^icc|^qnoc|power|devfreq|^bcl", "/thermal/"),
        ("MEMORY_DMA_IOMMU_SYNC", r"zram|zsmalloc|qpace|iommu|smmu|dma|synx|fence|mmrm|mem_|memlat|memleak|^llcc|^mpam|^slc|^msm_shared", "/synx-kernel/"),
    )
    for label, expression, fragment in rules:
        if re.search(expression, name) or fragment in joined:
            return label
    return "OPLUS_OTHER" if name.startswith("oplus_") else "PLATFORM_OR_GKI_OTHER"


def read_hashes(paths: list[Path]) -> dict[Path, str]:
    result = {}
    for path in paths:
        for line in path.read_text().splitlines():
            expected, filename = line.split(None, 1)
            if not re.fullmatch(r"[0-9a-f]{64}", expected):
                raise ValueError(f"invalid hash record in {path.name}")
            target = Path(filename.strip().removeprefix("*"))
            if target in result and result[target] != expected:
                raise ValueError("conflicting stock binary hashes")
            result[target] = expected
    return result


def write_tsv(path: Path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({key: clean(row.get(key, "")) for key in fields})


def depmod_dependency_field(dependencies: str) -> str:
    """Encode an empty final TSV field without dropping its column."""
    value = dependencies.strip()
    if value == "NONE":
        raise ValueError("depmod loader path collides with reserved NONE sentinel")
    return value or "NONE"


def decode_depmod_dependency_field(value: str) -> list[str]:
    """NONE means an empty dependency list, never a module named NONE."""
    if not value:
        raise ValueError("missing dependency field; expected NONE for no dependencies")
    return [] if value == "NONE" else value.split()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--oracle", type=Path, required=True, help="frozen static-oracle-01 directory")
    parser.add_argument("--firmware-images", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    repo, baseline, oracle = args.repo.resolve(), args.baseline.resolve(), args.oracle.resolve()
    out = args.output_dir.resolve()
    if out != repo / "docs/driver-updates":
        raise ValueError("outputs must stay in this repository's docs/driver-updates")
    helper_path = repo / "tools/validate-matched-wlan-vendor-dlkm.py"
    spec = importlib.util.spec_from_file_location("coloros17_elf_inventory", helper_path)
    if spec is None or spec.loader is None:
        raise ValueError("missing existing ELF helper")
    elf_helper = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = elf_helper
    spec.loader.exec_module(elf_helper)
    with (oracle / "stock-module-contract.tsv").open() as stream:
        contract = list(csv.DictReader(stream, delimiter="\t"))
    if Counter(row["partition"] for row in contract) != Counter(EXPECTED_PARTITIONS):
        raise ValueError("not the exact 1070-file ColorOS .102 population")
    if Counter(row["classification"] for row in contract) != {"COMPATIBLE": 1032, "DORMANT": 38}:
        raise ValueError("unexpected baseline load classification")
    expected = read_hashes([baseline / "module-sha256.txt", baseline / "vendor-boot-module-sha256.txt"])
    source_files, source_targets = source_indexes(repo)
    repositories = reference_repositories(repo)
    inventory, exports, sections, parameters, load_records, metadata_files, dependency_rows, depmod_paths = [], [], [], [], [], [], [], []
    image_rows = []
    for partition, relative in IMAGE_PATHS.items():
        image = args.firmware_images / relative
        actual = digest(image)
        if actual != FIRMWARE_HASHES[partition]:
            raise ValueError(f"not the exact stock .102 {partition} image")
        image_rows.append({"partition": partition, "firmware_path": relative, "bytes": image.stat().st_size, "sha256": actual})
    for index, item in enumerate(sorted(contract, key=lambda r: (r["partition"], r["stock_path"])), 1):
        partition = item["partition"]
        relative = Path(item["stock_path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("unsafe stock-relative module path")
        path = baseline / ROOTS[partition] / relative
        sha = digest(path)
        if expected.get(path) != sha:
            raise ValueError(f"stock binary hash mismatch: {partition}:{relative}")
        elf = elf_helper.ElfModule(path)
        if struct.unpack_from("<H", elf.data, 18)[0] != 183:
            raise ValueError("stock module is not AArch64")
        info = modinfo(path)
        name = get_one(info, "name")
        if name != item["module"]:
            raise ValueError(f"canonical module name mismatch: {partition}:{relative}")
        versions = elf.versions()
        undefined = elf.undefined_symbols()
        actual_exports = elf.exported_symbols()
        names = {section.name for section in elf.sections}
        comment = elf.section_or_none(".comment")
        compiler_text = ";".join(
            entry.decode("utf-8", "replace") for entry in
            (elf.slice(comment.offset, comment.size).split(b"\0") if comment else []) if entry
        )
        source_paths = sorted(source_files.get(name, set()))
        build_paths = sorted(source_targets.get(name, set()))
        routing = subsystem(name, source_paths + build_paths)
        row = {
            "index": index, "partition": partition, "stock_path": str(relative), "module": name,
            "sha256": sha, "bytes": path.stat().st_size, "compression": "UNCOMPRESSED_ELF",
            "load_mode": item["load_mode"], "baseline_classification": item["classification"],
            "signing_contract": item["signing_contract"], "signer": get_one(info, "signer", "UNSIGNED"),
            "signature_id": get_one(info, "sig_id", "NONE"),
            "sig_key": get_one(info, "sig_key", "NONE"), "sig_hashalgo": get_one(info, "sig_hashalgo", "NONE"),
            "vermagic": get_one(info, "vermagic"), "srcversion": get_one(info, "srcversion", "ABSENT"),
            "scmversion_metadata": get_one(info, "scmversion", "ABSENT"),
            "module_version": get_one(info, "version", "ABSENT"), "license": get_one(info, "license"),
            "depends": get_one(info, "depends"), "import_namespaces": get_one(info, "import_ns"),
            "alias_count": len(info.get("alias", [])), "undefined_elf_symbols": len(undefined),
            "hard_required": sum(not symbol.weak for symbol in undefined),
            "weak_imports": sum(symbol.weak for symbol in undefined), "version_records": len(versions),
            "exports": len(actual_exports), "gpl_exports": sum(r.export_class == "EXPORT_SYMBOL_GPL" for r in actual_exports.values()),
            "gnu_build_id": build_id(elf), "BTF": ".BTF" in names, "DWARF": ".debug_info" in names,
            "compiler_comment": compiler_text or "ABSENT",
            "section_count": len(elf.sections), "subsystem_review_bucket": routing,
            "source_file_candidates": ";".join(source_paths), "build_file_candidates": ";".join(build_paths),
            "source_match": "EXACT_REVISION_NOT_VERIFIED", "stock_source_revision": "UNKNOWN",
            **provenance_fields(name, source_paths + build_paths, info, repositories),
            "protected_export_failures_in_baseline_oracle": item["protected_export_failures"],
        }
        if row["vermagic"] != item["vermagic"]:
            raise ValueError("vermagic drift from frozen contract")
        inventory.append(row)
        for export in sorted(actual_exports.values(), key=lambda record: record.name):
            exports.append({"partition": partition, "module": name, "stock_path": str(relative), "symbol": export.name,
                            "crc": f"0x{export.crc:08x}", "export_class": export.export_class,
                            "namespace": export.namespace, "provider_sha256": sha})
        for section in elf.sections:
            sections.append({"partition": partition, "module": name, "section": section.name, "type": section.section_type,
                             "flags": f"0x{section.flags:x}", "bytes": section.size, "alignment": section.alignment})
        for parameter in info.get("parm", []):
            parameters.append({"partition": partition, "module": name, "parameter_and_description": parameter})
        for dependency in get_one(info, "depends").split(","):
            if dependency:
                dependency_rows.append({"partition": partition, "module": name, "dependency": dependency, "source": "ELF_modinfo_depends"})
    for partition, root in ROOTS.items():
        for filename in METADATA_NAMES:
            path = baseline / root / "lib/modules" / filename
            present = path.is_file()
            metadata_files.append({"partition": partition, "stock_path": f"lib/modules/{filename}", "present": present,
                                   "evidence_scope": "EXTRACTED_BASELINE", "bytes": path.stat().st_size if present else 0,
                                   "sha256": digest(path) if present else "NOT_CAPTURED_OR_ABSENT"})
            if present and filename == "modules.dep":
                for position, raw in enumerate(path.read_text().splitlines(), 1):
                    if not raw.strip():
                        continue
                    consumer, separator, dependencies = raw.partition(":")
                    if not separator:
                        raise ValueError("malformed stock modules.dep record")
                    depmod_paths.append({"metadata_partition": partition, "line": position,
                                         "consumer_loader_path": consumer,
                                         "dependency_loader_paths_in_order": depmod_dependency_field(dependencies)})
            if not present or not filename.startswith("modules.load"):
                continue
            for position, raw in enumerate(path.read_text().splitlines(), 1):
                text = raw.strip()
                if text and not text.startswith("#"):
                    load_records.append({"partition": partition, "list": filename, "line": position,
                                         "requested_path": text, "canonical_name": canonical(text)})
    totals = {field: sum(int(row[field]) for row in inventory) for field in
              ("undefined_elf_symbols", "hard_required", "weak_imports", "version_records")}
    if totals != {"undefined_elf_symbols": 60707, "hard_required": 60701, "weak_imports": 6, "version_records": 61777}:
        raise ValueError(f"ELF import counts drifted: {totals}")
    counts = Counter(row["subsystem_review_bucket"] for row in inventory)
    names = defaultdict(list)
    for row in inventory:
        names[row["module"]].append(row)
    duplicates = [{"module": name, "copies": len(rows), "partitions": ";".join(row["partition"] for row in rows),
                   "distinct_binary_hashes": len({row["sha256"] for row in rows}), "sha256s": ";".join(row["sha256"] for row in rows)}
                  for name, rows in sorted(names.items()) if len(rows) > 1]
    results = {"modules": len(inventory), "unique_names": len(names), "partitions": EXPECTED_PARTITIONS,
               "BTF_present": sum(row["BTF"] for row in inventory), "DWARF_present": sum(row["DWARF"] for row in inventory),
               "source_revision_verified": 0, "export_records": len(exports), "section_records": len(sections),
               "module_parameter_records": len(parameters), "duplicate_names": len(duplicates),
               "duplicate_names_with_different_bytes": sum(row["distinct_binary_hashes"] > 1 for row in duplicates),
               "firmware_declarations_present": sum(row["firmware_modinfo_declarations"] != "NOT_DECLARED_NOT_PROOF_NONE" for row in inventory),
               "repository_candidates_present": sum(row["source_repository_candidates"] != "UNKNOWN" for row in inventory),
               "private_header_dependencies_verified": 0, **totals}
    if args.check_only:
        print(json.dumps(results, indent=2, sort_keys=True))
        return 0
    out.mkdir(parents=True, exist_ok=True)
    datasets = (
        ("coloros17-module-inventory.tsv", inventory), ("coloros17-module-exports.tsv", exports),
        ("coloros17-module-sections.tsv", sections), ("coloros17-module-parameters.tsv", parameters),
        ("coloros17-module-dependencies.tsv", dependency_rows), ("coloros17-module-loads.tsv", load_records),
        ("coloros17-module-loading-metadata.tsv", metadata_files), ("coloros17-module-duplicates.tsv", duplicates),
        ("coloros17-stock-images.tsv", image_rows),
        ("coloros17-module-depmod-paths.tsv", depmod_paths),
    )
    for filename, rows in datasets:
        if rows:
            write_tsv(out / filename, list(rows[0]), rows)
    text = [
        "# ColorOS 17.0.0.102 exact stock-module inventory", "", "Audit date: 2026-10-30. Binary inventory only; no module update or physical test is claimed.", "",
        "Generated from the exact released .114 security source's stock oracle and the original .102 binary set. The stock framework is Android 17 / SDK 37; vendor is Android 16 / SDK 36. This is a mixed platform contract, not proof of another firmware's compatibility.", "",
        "## Population", "", "| Partition | Files |", "|---|---:|",
        *[f"| {partition} | {count} |" for partition, count in EXPECTED_PARTITIONS.items()],
        f"| Total | {len(inventory)} |", "",
        f"There are {len(names)} distinct canonical names, not 1070 distinct drivers. {len(duplicates)} names have copies in multiple partitions; {results['duplicate_names_with_different_bytes']} of these have different binary bytes. Copies must remain partition-qualified. The frozen normal/recovery load contract has 1032 compatible entries and 38 dormant entries; these are not the observed 691 runtime-loaded modules.", "",
        "The two unequal-name groups are zram and zsmalloc. Each has a signed system_dlkm GKI variant and matching unsigned vendor_dlkm/vendor_boot QCOM variants. A name-only replacement/provider model would conceal this difference. The other cross-partition copies are byte-identical; do not assume that permits changing only one normal/recovery load path.", "",
        "[Complete 1070-row inventory](coloros17-module-inventory.tsv) includes canonical name, binary hash/size, signature metadata, vermagic, version/source hash metadata, imports, exports, build ID, private type-section availability, source/build-file/repository candidates, explicit stock-source confidence, firmware declarations/dependency confidence, private-header review status and load classification.", "",
        "## Source ownership and confidence", "", "| Review bucket | Partition-qualified files |", "|---|---:|",
        *[f"| {name} | {count} |" for name, count in sorted(counts.items())], "",
        "Buckets route review; they are not exact source attribution. Source/build-file name associations are explicitly candidates. All 1070 stock source revisions remain EXACT_REVISION_NOT_VERIFIED: a matching filename, release tag, vermagic, srcversion, export CRC or source-equivalent function does not prove a matching shipping implementation. Do not apply historical OxygenOS .601 source lineage as ColorOS .102 binary provenance.", "",
        "Every row explicitly marks original_source_repository=UNVERIFIED_FOR_STOCK_BINARY. source_repository_candidates maps existing local source/build-path candidates to manifest-declared Google Common, OnePlus SoC or OnePlus modules repositories and the official CodeLinaro URLs previously audited in [the ref inventory](coloros17-upstream-refs.tsv). Repository ownership/availability is verified at that reference level, not as the origin of the stock binary. Rows without a local path association retain UNKNOWN. Generic build lists can yield multiple candidates; neither a candidate URL nor its release tag is a source-match or security-applicability claim.", "",
        f"Firmware: {results['firmware_declarations_present']} partition-qualified module records have explicit ELF modinfo firmware declarations; their exact declared strings are preserved per row. Other rows use NOT_DECLARED_NOT_PROOF_NONE. firmware_dependency_status remains unverified for every module: MODULE_FIRMWARE metadata need not enumerate dynamically requested firmware, secure-world dependencies, calibration, HAL/user-space contracts or DT-selected files. No firmware dependency is inferred absent and no firmware compatibility is approved by this inventory.", "",
        "Private headers: every row retains private_header_dependency_status=UNVERIFIED_EXACT_SOURCE. Applicable RMNET and DWC3 rows link related partial manual boundary reviews; these are not complete per-module include/dependency closures or proof of shipping layout equivalence. Other rows explicitly state NO_PER_MODULE_PRIVATE_HEADER_REVIEW. BTF availability, matching export CRCs and name-based public source associations cannot establish private-header compatibility; complete dependency/config/layout review remains a replacement-module gate.", "",
        "All 103 GKI modules record 6.12.69-android16-6-g242e1a07f878-ab15865446-4k and scmversion g242e1a07f878. All 967 vendor/vendor_boot files record 6.12.69-android16-6-o-gb79517a1cbef-4k; 491 of those copies additionally record scmversion g4808b15dc936. These are source-search anchors, not proof of a complete source/config/toolchain match. Per-binary compiler-comment metadata is preserved where available.", "",
        "All 1070 compiler comments identify Android clang 19.0.1, build 14043575 / r536225, LLVM commit b3a530ec6537146650e42be89f1089e9a3588460. Rust Binder additionally identifies rustc 1.82.0-dev (f6e511eec 2024-10-15), Android Rust toolchain linux-glibc-15545352. A replacement toolchain must be reviewed for compiler-generated Rust identity, CFI, DWARF and module ABI effects rather than chosen only by version proximity.", "",
        f"Private type evidence: {results['BTF_present']} ELF files contain .BTF; {results['DWARF_present']} contain .debug_info. The complete section inventory records availability per exact binary; absent BTF/DWARF is not evidence of layout compatibility. All binaries are uncompressed AArch64 ELF inputs.", "",
        "## Import, provider and load metadata", "",
        "The independent loader-oriented parser counts 60707 named ELF undefined symbols: 60701 hard and 6 weak. All 61777 MODVERSIONS records are accounted for, including special module-layout entries. Export records come only from actual __ksymtab/__ksymtab_gpl and associated CRC/namespace tables, never arbitrary global ELF definitions. This inventory is not a replacement for the complete candidate provider oracle.", "",
        "Companions: [actual exports](coloros17-module-exports.tsv), [ELF sections](coloros17-module-sections.tsv), [parameters](coloros17-module-parameters.tsv), [ELF dependency names](coloros17-module-dependencies.tsv), [depmod paths/order](coloros17-module-depmod-paths.tsv), [ordered stock load requests](coloros17-module-loads.tsv), [metadata hashes/presence](coloros17-module-loading-metadata.tsv), [cross-partition copies](coloros17-module-duplicates.tsv), and [raw stock images](coloros17-stock-images.tsv). modules.dep uses explicit system/vendor/ramdisk paths; module names alone do not decide runtime precedence. Metadata missing from the extracted baseline is marked NOT_CAPTURED_OR_ABSENT, not asserted absent from the original partition or synthesized.", "",
        "In the depmod-paths TSV, dependency_loader_paths_in_order=NONE explicitly represents an empty dependency list. Decode NONE to [] before interpreting loader paths; otherwise split the field on whitespace in its recorded order. This preserves a rectangular four-column table without a trailing empty field/tab. NONE is a reserved encoding, not a module request; a literal matching input is rejected rather than silently conflated. This changes only the audit representation, never stock modules.dep bytes or dependency semantics.", "",
        "## Signing and deployment gates", "",
        "All 103 system_dlkm files are trusted-signed stock GKI modules; all 967 vendor_dlkm/vendor_boot files are retained unsigned under the qualified Android policy. Actual signed-module metadata uses PKCS#7 / SHA256 and public key identifier 7B:47:78:24:F0:27:CB:C7:E5:84:37:5F:41:33:60:F2:C0:96:2E:BC. Signature metadata is inventory, not a fresh cryptographic signature-verification claim. Current configuration keeps MODULE_SIG, MODULE_SIG_PROTECT, MODVERSIONS, GENDWARFKSYMS and trimming; MODULE_SIG_FORCE is not enabled, as in the qualified firmware contract. The current production signing-hash setting is sha1; do not confuse it with the actual stock module signature algorithms listed individually.", "",
        "The stock private signing key is not provided by the public certificate. Rebuilt protected GKI providers require a legitimately trusted signing/dependency closure. Unsigned vendor providers remain subject to permitted imports, protected exports, namespaces and CRCs. Do not re-sign retained stock files, spoof vermagic/CRCs, force loading or disable validation. A new matched-module deployment needs a new kernel/module/AVB/package qualification; existing boot-only release artifacts stay unchanged.", "",
        "The Downloads raw system_dlkm/vendor_dlkm/vendor_boot hashes match the previously protected stock partitions. The baseline images named *-verity.img are filesystem-only mounted views and have different lengths/hashes; they are not interchangeable raw partition payloads. No binaries, firmware, private signing keys or raw device property dump are committed here.", "",
        "## Reproduction", "", "```sh", "python3 tools/driver-updates/coloros17-module-inventory.py \\", "  --baseline <immutable-.102-baseline> \\", "  --oracle <frozen-five-CVE-static-oracle-01> \\", "  --firmware-images <CPH2745IN_17.0.0.102> \\", "  --output-dir docs/driver-updates", "```", "",
        "Use --check-only for an output-free repeat of all binary/hash/count checks. No property dump is consumed or published by the generator. It never reads a serial, PIN, credential or private key. No device is accessed and no source revision is inferred from a release name.", "",
    ]
    (out / "coloros17-module-inventory.md").write_text("\n".join(text), encoding="utf-8")
    print(json.dumps(results, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
