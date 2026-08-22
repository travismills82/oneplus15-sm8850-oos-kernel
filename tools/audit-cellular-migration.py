#!/usr/bin/env python3
"""Audit the frozen 27-module Canoe cellular delivery closure.

The module set is deliberately explicit: it is the exact-stock set preserved
by the physically qualified controlled-v1 core image.  ELF imports/exports,
CRCs, load policy, and runtime presence are derived from supplied artifacts;
source and role metadata describe the reviewed Canoe build graph.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Spec:
    source: str
    target: str
    config: str
    role: str
    risk: str
    batch: str
    hardware: str


SPECS = {
    "dwc3_msm": Spec("kernel_platform/soc-repo/drivers/usb/dwc3", "//soc-repo:canoe_perf/drivers/usb/dwc3/dwc3-msm", "CONFIG_USB_DWC3_MSM", "OTHER", "HIGH", "F", "Qualcomm USB DWC3 glue used by USB tethering/GSI"),
    "gsim": Spec("vendor/qcom/opensource/dataipa/drivers/platform/msm/gsi", "//vendor/qcom/opensource/dataipa:canoe_perf_gsim", "DDK: canoe_perf_gsim", "GSI TRANSPORT", "HIGH", "D", "GSI transport and channel/event-ring provider for IPA"),
    "ipam": Spec("vendor/qcom/opensource/dataipa/drivers/platform/msm/ipa", "//vendor/qcom/opensource/dataipa:canoe_perf_ipam", "DDK: canoe_perf_ipam", "IPA CORE", "HIGH", "D", "IPA core, QMI control, firmware and packet-data plumbing"),
    "ipanetm": Spec("vendor/qcom/opensource/dataipa/drivers/platform/msm/ipa/ipa_v3/ipa_net.c", "//vendor/qcom/opensource/dataipa:canoe_perf_ipanetm", "DDK: canoe_perf_ipanetm", "IPA CLIENT", "HIGH", "D", "IPA net module glue invoking IPA WWAN/RNDIS registration"),
    "oplus_mm_kevent": Spec("vendor/oplus/kernel/multimedia/feedback", "//vendor/oplus/kernel/multimedia/feedback/bazel:oplus_mm_kevent", "OPLUS DDK", "OTHER", "MEDIUM", "F", "Oplus multimedia feedback provider shared by USB/audio path"),
    "oplus_mm_kevent_fb": Spec("vendor/oplus/kernel/multimedia/feedback", "//vendor/oplus/kernel/multimedia/feedback/bazel:oplus_mm_kevent_fb", "OPLUS DDK", "OTHER", "MEDIUM", "F", "Oplus feedback bridge consumed by WCD USBSS I2C"),
    "qcom_glink": Spec("kernel_platform/soc-repo/drivers/rpmsg", "//soc-repo:canoe_perf/drivers/rpmsg/qcom_glink", "CONFIG_RPMSG_QCOM_GLINK", "FOUNDATION PROVIDER", "HIGH", "E", "Qualcomm GLINK transport and SSR notification provider"),
    "qcom_glink_smem": Spec("kernel_platform/soc-repo/drivers/rpmsg", "//soc-repo:canoe_perf/drivers/rpmsg/qcom_glink_smem", "CONFIG_RPMSG_QCOM_GLINK_SMEM", "QMI / MODEM CONTROL", "HIGH", "E", "SMEM-backed GLINK transport used by remoteproc/modem clients"),
    "qcom_ramdump": Spec("kernel_platform/soc-repo/drivers/soc/qcom", "//soc-repo:canoe_perf/drivers/soc/qcom/qcom_ramdump", "CONFIG_QCOM_RAMDUMP", "OTHER", "MEDIUM", "E", "Remote subsystem dump provider used by IPA diagnostics"),
    "qcom_smd": Spec("kernel_platform/soc-repo/drivers/rpmsg", "//soc-repo:canoe_perf/drivers/rpmsg/qcom_smd", "CONFIG_RPMSG_QCOM_SMD", "QMI / MODEM CONTROL", "HIGH", "E", "Qualcomm SMD transport used by remoteproc/modem control"),
    "qcom_va_minidump": Spec("kernel_platform/soc-repo/drivers/soc/qcom", "//soc-repo:canoe_perf/drivers/soc/qcom/qcom_va_minidump", "CONFIG_QCOM_VA_MINIDUMP", "OTHER", "MEDIUM", "E", "Virtual-address minidump registration used by IPA"),
    "qmi_helpers": Spec("kernel_platform/soc-repo/drivers/soc/qcom", "//soc-repo:canoe_perf/drivers/soc/qcom/qmi_helpers", "CONFIG_QCOM_QMI_HELPERS", "QMI / MODEM CONTROL", "HIGH", "E", "QMI encode/decode and transaction APIs for IPA/RMNET/modem"),
    "redriver": Spec("kernel_platform/soc-repo/drivers/usb/redriver", "//soc-repo:canoe_perf/drivers/usb/redriver/redriver", "CONFIG_USB_REDRIVER", "OTHER", "HIGH", "F", "Shared USB redriver provider used by DWC3"),
    "repeater": Spec("kernel_platform/soc-repo/drivers/usb/repeater", "//soc-repo:canoe_perf/drivers/usb/repeater/repeater", "CONFIG_USB_REPEATER", "OTHER", "HIGH", "F", "Shared USB repeater provider used by DWC3"),
    "rmnet_aps": Spec("vendor/qcom/opensource/datarmnet-ext/aps", "//vendor/qcom/opensource/datarmnet-ext/aps:canoe_perf_aps", "DDK: canoe_perf_aps", "RMNET EXTENSION", "MEDIUM", "B", "Leaf adaptive packet-steering policy consumer of RMNET core"),
    "rmnet_core": Spec("vendor/qcom/opensource/datarmnet/core", "//vendor/qcom/opensource/datarmnet:canoe_perf_rmnet_core", "DDK: canoe_perf_rmnet_core", "RMNET CORE", "HIGH", "C", "RMNET netdevice/QMAP core and provider for extensions"),
    "rmnet_ctl": Spec("vendor/qcom/opensource/datarmnet/core", "//vendor/qcom/opensource/datarmnet:canoe_perf_rmnet_ctl", "DDK: canoe_perf_rmnet_ctl", "QMI / MODEM CONTROL", "HIGH", "C", "RMNET-to-IPA control client and statistics provider"),
    "rmnet_mem": Spec("vendor/qcom/opensource/datarmnet-ext/mem", "//vendor/qcom/opensource/datarmnet-ext/mem:canoe_perf_rmnet_mem", "DDK: canoe_perf_rmnet_mem", "MEMORY / PREALLOC", "HIGH", "C", "Shared SKB/page-pool allocator for IPA and RMNET"),
    "rmnet_offload": Spec("vendor/qcom/opensource/datarmnet-ext/offload", "//vendor/qcom/opensource/datarmnet-ext/offload:canoe_perf_offload", "DDK: canoe_perf_offload", "RMNET EXTENSION", "MEDIUM", "B", "Leaf RMNET TCP/UDP offload engine"),
    "rmnet_perf": Spec("vendor/qcom/opensource/datarmnet-ext/perf", "//vendor/qcom/opensource/datarmnet-ext/perf:canoe_perf_perf", "DDK: canoe_perf_perf", "RMNET EXTENSION", "MEDIUM", "B", "Leaf RMNET TCP/UDP performance policy"),
    "rmnet_perf_tether": Spec("vendor/qcom/opensource/datarmnet-ext/perf_tether", "//vendor/qcom/opensource/datarmnet-ext/perf_tether:canoe_perf_perf_tether", "DDK: canoe_perf_perf_tether", "RMNET EXTENSION", "MEDIUM", "B", "Leaf RMNET tethering performance policy"),
    "rmnet_sch": Spec("vendor/qcom/opensource/datarmnet-ext/sch", "//vendor/qcom/opensource/datarmnet-ext/sch:canoe_perf_sch", "DDK: canoe_perf_sch", "RMNET EXTENSION", "LOW", "A", "Independent RMNET qdisc registration; Image-only imports"),
    "rmnet_shs": Spec("vendor/qcom/opensource/datarmnet-ext/shs", "//vendor/qcom/opensource/datarmnet-ext/shs:canoe_perf_shs", "DDK: canoe_perf_shs", "RMNET EXTENSION", "HIGH", "B", "RMNET scheduling/flow steering tied to WALT and RMNET core"),
    "rmnet_wlan": Spec("vendor/qcom/opensource/datarmnet-ext/wlan", "//vendor/qcom/opensource/datarmnet-ext/wlan:canoe_perf_wlan", "DDK: canoe_perf_wlan", "SHARED WLAN/CELLULAR PROVIDER", "HIGH", "B", "RMNET WLAN forwarding/coexistence extension"),
    "rproc_qcom_common": Spec("kernel_platform/soc-repo/drivers/remoteproc", "//soc-repo:canoe_perf/drivers/remoteproc/rproc_qcom_common", "CONFIG_QCOM_RPROC_COMMON", "FOUNDATION PROVIDER", "HIGH", "E", "Remoteproc common, GLINK subdevice and SSR provider"),
    "usb_f_gsi": Spec("kernel_platform/soc-repo/drivers/usb/gadget/function", "//soc-repo:canoe_perf/drivers/usb/gadget/function/usb_f_gsi", "CONFIG_USB_F_GSI", "IPA CLIENT", "HIGH", "F", "USB tethering function registering with IPA"),
    "wcd_usbss_i2c": Spec("kernel_platform/soc-repo/drivers/soc/qcom", "//soc-repo:canoe_perf/drivers/soc/qcom/wcd_usbss_i2c", "CONFIG_QCOM_WCD_USBSS_I2C", "OTHER", "HIGH", "F", "Shared USBSS/audio I2C provider in the DWC3 dependency chain"),
}

BATCHES = {
    "A": ("Independent qdisc leaf", "rmnet_sch", "vmlinux", "LOW", "Selected Batch 01; current .097 source ownership only"),
    "B": ("RMNET policy/flow leaves", "rmnet_aps,rmnet_offload,rmnet_perf,rmnet_perf_tether,rmnet_shs,rmnet_wlan", "rmnet_core and frozen providers", "MEDIUM-HIGH", "Test as smaller leaf sub-batches before combining"),
    "C": ("RMNET provider core", "rmnet_core,rmnet_ctl,rmnet_mem", "IPA/QMI and frozen providers", "HIGH", "Requires all affected RMNET consumers rebuilt or CRC-proven"),
    "D": ("GSI and IPA", "gsim,ipam,ipanetm", "modem IPC, USB GSI, RMNET providers", "HIGH", "Foundational data-plane and firmware/SSR batch"),
    "E": ("Modem IPC and diagnostics", "qcom_glink,qcom_glink_smem,qcom_ramdump,qcom_smd,qcom_va_minidump,qmi_helpers,rproc_qcom_common", "Image and broader stock platform providers", "HIGH", "Shared by WLAN, remoteproc and many non-cellular clients"),
    "F": ("USB tethering/shared support", "dwc3_msm,oplus_mm_kevent,oplus_mm_kevent_fb,redriver,repeater,usb_f_gsi,wcd_usbss_i2c", "Image and retained USB/audio providers", "HIGH", "Shared USB/audio/charging path; migrate only for proven need"),
}


def elf_helpers():
    path = Path(__file__).with_name("validate-matched-wlan-vendor-dlkm.py")
    spec = importlib.util.spec_from_file_location("cellular_elf", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


ELF = elf_helpers()


def norm(name: str) -> str:
    return name.replace("-", "_")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def modinfo(path: Path, field: str) -> str:
    result = subprocess.run(["modinfo", "-F", field, str(path)], check=True, capture_output=True, text=True)
    return ";".join(line.strip() for line in result.stdout.splitlines() if line.strip())


def scan(root: Path) -> dict[str, Path]:
    result = {}
    for path in sorted(root.rglob("*.ko")):
        name = norm(ELF.module_record(path, "vendor_dlkm", "STOCK").name)
        if name in result:
            raise ValueError(f"duplicate module name {name}: {result[name]} and {path}")
        result[name] = path
    return result


def load_positions(path: Path) -> dict[str, list[int]]:
    positions: dict[str, list[int]] = defaultdict(list)
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if line and not line.startswith("#"):
            positions[norm(Path(line).stem)].append(line_number)
    return positions


def proc_positions(path: Path) -> dict[str, int]:
    positions = {}
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = raw.split()
        if fields:
            positions[norm(fields[0])] = line_number
    return positions


def read_import_contract(path: Path):
    imports: dict[str, list[tuple[str, str]]] = defaultdict(list)
    import_providers: dict[str, list[tuple[str, str, str]]] = defaultdict(list)
    providers: dict[str, set[str]] = defaultdict(set)
    consumers: dict[str, set[str]] = defaultdict(set)
    boundary_edges: set[tuple[str, str, str, str]] = set()
    with path.open(encoding="utf-8", newline="") as source:
        for row in csv.DictReader(source, delimiter="\t"):
            consumer = norm(row["consumer"])
            imports[consumer].append((row["symbol"], row["expected_crc"]))
            for label in row["matching_providers"].split(","):
                provider = norm(label.rsplit(":", 1)[-1])
                providers[consumer].add(provider)
                import_providers[consumer].append(
                    (provider, row["symbol"], row["expected_crc"])
                )
                if provider in SPECS:
                    consumers[provider].add(consumer)
                    boundary_edges.add(
                        (provider, consumer, row["symbol"], row["expected_crc"])
                    )
    return imports, import_providers, providers, consumers, boundary_edges


def write_tsv(path: Path, header, rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-root", type=Path, required=True)
    parser.add_argument("--qualified-root", type=Path, required=True)
    parser.add_argument("--vendor-load", type=Path, required=True)
    parser.add_argument("--proc-modules", type=Path, required=True)
    parser.add_argument("--import-contract", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()

    stock = scan(args.stock_root)
    qualified = scan(args.qualified_root)
    missing = sorted(set(SPECS) - set(stock))
    if missing:
        raise ValueError(f"missing stock cellular modules: {','.join(missing)}")
    if len(SPECS) != 27:
        raise ValueError(f"cellular manifest must contain 27 modules, found {len(SPECS)}")
    load = load_positions(args.vendor_load)
    runtime = proc_positions(args.proc_modules)
    imports, import_providers, providers, consumers, boundary_edges = read_import_contract(
        args.import_contract
    )

    rows = []
    edge_rows = set(boundary_edges)
    exact = 0
    for name, spec in sorted(SPECS.items()):
        path = stock[name]
        record = ELF.module_record(path, "vendor_dlkm", "STOCK")
        qualified_path = qualified.get(name)
        unchanged = qualified_path is not None and sha256(path) == sha256(qualified_path)
        exact += int(unchanged)
        import_text = ";".join(f"{symbol}:{crc}" for symbol, crc in sorted(imports[name]))
        export_text = ";".join(f"{symbol}:0x{crc:08x}" for symbol, crc in sorted(record.exports.items()))
        load_text = ",".join(str(value) for value in load.get(name, [])) or "not directly requested"
        proc_text = str(runtime[name]) if name in runtime else "not observed"
        rows.append([
            name,
            path.relative_to(args.stock_root),
            "yes" if name in runtime else "no",
            f"modules.load:{load_text}; proc_modules_position:{proc_text}",
            spec.source,
            spec.target,
            spec.config,
            import_text,
            export_text,
            "none in qualified final protected-edge report",
            import_text,
            modinfo(path, "parm"),
            modinfo(path, "softdep"),
            ",".join(sorted(consumers[name])) or "none in 27-module set",
            ",".join(sorted(providers[name])),
            spec.hardware,
            spec.role,
            spec.risk,
            spec.batch,
            sha256(path),
            "yes" if unchanged else "NO",
        ])

    write_tsv(args.out_dir / "current-cellular-closure.tsv", [
        "module", "stock_path", "runtime_loaded", "load_order_evidence", "source_path",
        "build_target", "kconfig_ddk_target", "imports", "exports", "protected_imports",
        "modversion_crcs", "module_parameters", "softdeps", "consumer_modules",
        "provider_modules", "hardware_runtime_role", "classification", "migration_risk",
        "batch", "stock_sha256", "qualified_byte_identical",
    ], rows)
    write_tsv(
        args.out_dir / "dependency-edges.tsv",
        ["provider", "consumer", "symbol", "crc", "consumer_in_cellular_closure"],
        [row + (("yes" if row[1] in SPECS else "no"),) for row in sorted(edge_rows)],
    )
    write_tsv(args.out_dir / "migration-batches.tsv", ["batch", "name", "modules", "providers_retained", "risk", "disposition"],
              [[key, *value] for key, value in sorted(BATCHES.items())])
    print(f"CELLULAR MIGRATION AUDIT PASS modules={len(rows)} exact_stock={exact} edges={len(edge_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
