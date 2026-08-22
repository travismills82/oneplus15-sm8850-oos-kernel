#!/usr/bin/env python3
"""Validate a signed WLAN vendor-DLKM replacement closure.

This tool is deliberately read-only with respect to its module inputs.  It
overlays source-built module replacements onto a complete stock ``vendor_dlkm``
tree, then inspects the actual ELF ``__versions`` and ``__ksymtab``/``__kcrctab``
tables.  The resulting reports answer three separate questions:

* Does every source-built replacement preserve the stock module ABI contract?
* Do all modules in the resulting vendor-DLKM tree resolve their versioned
  imports against vmlinux, the candidate tree, or an explicitly retained
  external module root?
* Which remaining stock vendor-DLKM modules must be re-signed when a source
  replacement becomes a signed provider under CONFIG_MODULE_SIG_PROTECT?

It does not sign modules, change module metadata, or construct an image.  A
packager must first strip modules, then sign the source replacements and the
reported re-sign envelope with the exact key embedded in the matching Image.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ELF_MAGIC = b"\x7fELF"
ELFCLASS64 = 2
ELFDATA2LSB = 1
MODULE_SIGNATURE_MAGIC = b"~Module signature appended~\n"


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    offset: int
    size: int
    link: int
    entsize: int


class ElfModule:
    """Minimal ELF64 reader for the module metadata that the loader uses."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:4] != ELF_MAGIC:
            raise ValueError(f"{path}: not an ELF file")
        if self.data[4] != ELFCLASS64 or self.data[5] != ELFDATA2LSB:
            raise ValueError(f"{path}: expected little-endian ELF64")

        header = struct.unpack_from("<16sHHIQQQIHHHHHH", self.data, 0)
        self.sh_offset = header[6]
        self.sh_entry_size = header[11]
        self.sh_count = header[12]
        self.shstr_index = header[13]
        if self.sh_entry_size != 64:
            raise ValueError(f"{path}: unexpected section header size")
        if self.shstr_index >= self.sh_count:
            raise ValueError(f"{path}: invalid section-string-table index")

        raw_headers = []
        for index in range(self.sh_count):
            offset = self.sh_offset + index * self.sh_entry_size
            raw_headers.append(struct.unpack_from("<IIQQQQIIQQ", self.data, offset))
        shstr = raw_headers[self.shstr_index]
        shstr_data = self.slice(shstr[4], shstr[5])
        self.sections = [
            Section(
                index=index,
                name=self.c_string(shstr_data, raw[0]),
                offset=raw[4],
                size=raw[5],
                link=raw[6],
                entsize=raw[9],
            )
            for index, raw in enumerate(raw_headers)
        ]

    def slice(self, offset: int, size: int) -> bytes:
        end = offset + size
        if offset < 0 or end > len(self.data):
            raise ValueError(f"{self.path}: truncated ELF section")
        return self.data[offset:end]

    @staticmethod
    def c_string(data: bytes, offset: int) -> str:
        if offset >= len(data):
            return ""
        return data[offset:].split(b"\0", 1)[0].decode("ascii", "replace")

    def section_or_none(self, name: str) -> Section | None:
        matches = [section for section in self.sections if section.name == name]
        if len(matches) > 1:
            raise ValueError(f"{self.path}: multiple {name} sections")
        return matches[0] if matches else None

    def modinfo(self) -> dict[str, list[str]]:
        section = self.section_or_none(".modinfo")
        values: dict[str, list[str]] = defaultdict(list)
        if not section:
            return values
        for entry in self.slice(section.offset, section.size).split(b"\0"):
            text = entry.decode("ascii", "replace")
            if "=" not in text:
                continue
            key, value = text.split("=", 1)
            values[key].append(value)
        return values

    def versions(self) -> dict[str, int]:
        section = self.section_or_none("__versions")
        if not section:
            return {}
        entry_size = 64  # struct modversion_info: u64 CRC + 56-byte name
        if section.size % entry_size:
            raise ValueError(f"{self.path}: malformed __versions section")
        versions: dict[str, int] = {}
        data = self.slice(section.offset, section.size)
        for offset in range(0, len(data), entry_size):
            crc = struct.unpack_from("<Q", data, offset)[0] & 0xFFFFFFFF
            name = self.c_string(data[offset + 8 : offset + entry_size], 0)
            if not name:
                continue
            if name in versions and versions[name] != crc:
                raise ValueError(f"{self.path}: conflicting CRC for {name}")
            versions[name] = crc
        return versions

    def exported_crcs(self) -> dict[str, int]:
        symtab = next((section for section in self.sections if section.name == ".symtab"), None)
        if not symtab or symtab.entsize != 24:
            raise ValueError(f"{self.path}: missing usable .symtab")
        if symtab.link >= len(self.sections):
            raise ValueError(f"{self.path}: invalid symbol-string-table reference")
        strtab = self.sections[symtab.link]
        strings = self.slice(strtab.offset, strtab.size)
        symbols = []
        data = self.slice(symtab.offset, symtab.size)
        for offset in range(0, len(data), symtab.entsize):
            name_offset, _info, _other, section_index, value, _size = struct.unpack_from(
                "<IBBHQQ", data, offset
            )
            symbols.append((self.c_string(strings, name_offset), section_index, value))

        exports: dict[str, int] = {}
        for section_name, crc_section_name in (
            ("__ksymtab", "__kcrctab"),
            ("__ksymtab_gpl", "__kcrctab_gpl"),
            ("__ksymtab_gpl_future", "__kcrctab_gpl_future"),
        ):
            ksymtab = self.section_or_none(section_name)
            kcrctab = self.section_or_none(crc_section_name)
            if not ksymtab and not kcrctab:
                continue
            if not ksymtab or not kcrctab:
                raise ValueError(f"{self.path}: incomplete {section_name} export tables")
            entries = []
            for name, section_index, value in symbols:
                if (
                    section_index == ksymtab.index
                    and name != section_name
                    and name.startswith("__ksymtab_")
                ):
                    entries.append((value, name[len("__ksymtab_") :]))
            entries.sort()
            if not entries:
                raise ValueError(f"{self.path}: no symbols in {section_name}")
            if kcrctab.size != 4 * len(entries):
                raise ValueError(
                    f"{self.path}: {crc_section_name} has {kcrctab.size} bytes for "
                    f"{len(entries)} exports"
                )
            crc_data = self.slice(kcrctab.offset, kcrctab.size)
            for index, (_value, name) in enumerate(entries):
                crc = struct.unpack_from("<I", crc_data, 4 * index)[0]
                if name in exports and exports[name] != crc:
                    raise ValueError(f"{self.path}: conflicting export CRC for {name}")
                exports[name] = crc
        return exports


@dataclass(frozen=True)
class ModuleRecord:
    area: str
    name: str
    path: Path
    provenance: str
    imports: dict[str, int]
    exports: dict[str, int]
    depends: tuple[str, ...]
    signed: bool
    sha256: str

    @property
    def label(self) -> str:
        return f"{self.area}:{self.name}"


def crc(value: int | None) -> str:
    return "" if value is None else f"0x{value:08x}"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def module_record(path: Path, area: str, provenance: str) -> ModuleRecord:
    elf = ElfModule(path)
    modinfo = elf.modinfo()
    names = modinfo.get("name", [])
    if len(set(names)) > 1:
        raise ValueError(f"{path}: conflicting module names: {names}")
    name = names[0] if names else path.stem.replace("-", "_")
    depends = tuple(
        dependency
        for value in modinfo.get("depends", [])
        for dependency in value.split(",")
        if dependency
    )
    return ModuleRecord(
        area=area,
        name=name,
        path=path,
        provenance=provenance,
        imports=elf.versions(),
        exports=elf.exported_crcs(),
        depends=depends,
        signed=elf.data.endswith(MODULE_SIGNATURE_MAGIC),
        sha256=sha256(path),
    )


def scan_root(root: Path, area: str, provenance: str) -> dict[str, ModuleRecord]:
    if not root.is_dir():
        raise ValueError(f"{root}: expected a module root directory")
    modules: dict[str, ModuleRecord] = {}
    for path in sorted(root.rglob("*.ko")):
        record = module_record(path, area, provenance)
        if record.name in modules:
            previous = modules[record.name]
            raise ValueError(
                f"{root}: duplicate module name {record.name}: {previous.path} and {path}"
            )
        modules[record.name] = record
    return modules


def parse_symvers(path: Path) -> dict[str, list[tuple[str, int]]]:
    if not path.is_file():
        raise ValueError(f"{path}: missing Module.symvers")
    exports: dict[str, list[tuple[str, int]]] = defaultdict(list)
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = line.split("\t")
        if len(fields) < 3 or not fields[0].startswith("0x"):
            continue
        try:
            value = int(fields[0], 16) & 0xFFFFFFFF
        except ValueError as exc:
            raise ValueError(f"{path}:{line_number}: invalid CRC") from exc
        exports[fields[1]].append((fields[2], value))
    return exports


def write_tsv(path: Path, header: Iterable[str], rows: Iterable[Iterable[str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        output.write("\t".join(header) + "\n")
        for row in rows:
            output.write("\t".join(str(value) for value in row) + "\n")


def source_provider_edges(
    consumers: Iterable[ModuleRecord], signed_modules: dict[str, ModuleRecord]
) -> dict[str, list[tuple[str, str]]]:
    by_symbol: dict[str, list[tuple[str, int]]] = defaultdict(list)
    for provider in signed_modules.values():
        for symbol, value in provider.exports.items():
            by_symbol[symbol].append((provider.name, value))

    edges: dict[str, list[tuple[str, str]]] = {}
    for consumer in consumers:
        matches = []
        for symbol, expected in consumer.imports.items():
            providers = sorted(
                provider for provider, actual in by_symbol.get(symbol, []) if actual == expected
            )
            if providers:
                matches.append((symbol, ",".join(providers)))
        if matches:
            edges[consumer.name] = matches
    return edges


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--stock-vendor-root",
        type=Path,
        required=True,
        help="read-only extracted stock vendor_dlkm module root",
    )
    parser.add_argument(
        "--replacement",
        type=Path,
        action="append",
        required=True,
        help="stripped source-built replacement .ko; repeat for every replacement",
    )
    parser.add_argument(
        "--external-root",
        type=Path,
        action="append",
        default=[],
        help="read-only retained module root, such as vendor_boot or system_dlkm",
    )
    parser.add_argument(
        "--vmlinux-symvers",
        type=Path,
        required=True,
        help="Module.symvers from the exact matching Image build",
    )
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument(
        "--expected-stock-module-count",
        type=int,
        help="fail if the stock vendor root does not contain this many modules",
    )
    parser.add_argument(
        "--fail-external-signed-provider-edges",
        action="store_true",
        help="treat retained external consumers of new signed providers as fatal",
    )
    parser.add_argument(
        "--allow-import-contract-change",
        action="append",
        default=[],
        metavar="MODULE",
        help=(
            "allow an explicitly reviewed source replacement to change its import set; "
            "all resulting imports must still resolve with matching CRCs"
        ),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    stock = scan_root(args.stock_vendor_root, "vendor_dlkm", "STOCK_VENDOR")
    if args.expected_stock_module_count is not None and len(stock) != args.expected_stock_module_count:
        raise ValueError(
            f"stock vendor module count is {len(stock)}, expected {args.expected_stock_module_count}"
        )

    final = dict(stock)
    replacements: dict[str, ModuleRecord] = {}
    for path in args.replacement:
        record = module_record(path, "vendor_dlkm", "SOURCE_REPLACEMENT")
        if record.name not in stock:
            raise ValueError(f"{path}: replacement {record.name} has no stock vendor counterpart")
        if record.name in replacements:
            raise ValueError(f"duplicate replacement for {record.name}")
        replacements[record.name] = record
        final[record.name] = record

    external: list[ModuleRecord] = []
    for index, root in enumerate(args.external_root):
        external.extend(scan_root(root, f"external{index}", "RETAINED_EXTERNAL").values())
    vmlinux_exports = parse_symvers(args.vmlinux_symvers)

    replacement_rows = []
    contract_failures = 0
    allowed_import_changes = set(args.allow_import_contract_change)
    unknown_allowed = allowed_import_changes - set(replacements)
    if unknown_allowed:
        raise ValueError(
            "allowed import-contract module is not a replacement: "
            + ", ".join(sorted(unknown_allowed))
        )
    for name, replacement in sorted(replacements.items()):
        original = stock[name]
        imports_match = replacement.imports == original.imports
        exports_match = replacement.exports == original.exports
        import_change_allowed = name in allowed_import_changes
        if (not imports_match and not import_change_allowed) or not exports_match:
            contract_failures += 1
        replacement_rows.append(
            [
                name,
                original.path,
                replacement.path,
                original.sha256,
                replacement.sha256,
                original.path.stat().st_size,
                replacement.path.stat().st_size,
                "MATCH" if imports_match else "MISMATCH",
                "YES" if import_change_allowed else "NO",
                "MATCH" if exports_match else "MISMATCH",
            ]
        )
    write_tsv(
        args.out_dir / "replacement-contracts.tsv",
        [
            "module",
            "stock_path",
            "replacement_path",
            "stock_sha256",
            "replacement_sha256",
            "stock_bytes",
            "replacement_bytes",
            "imports",
            "import_change_explicitly_allowed",
            "exports",
        ],
        replacement_rows,
    )

    providers: dict[str, list[tuple[str, int]]] = defaultdict(list)
    for record in list(final.values()) + external:
        for symbol, value in record.exports.items():
            providers[symbol].append((record.label, value))
    for symbol, values in vmlinux_exports.items():
        for provider, value in values:
            providers[symbol].append((f"vmlinux:{provider}", value))

    import_rows = []
    unresolved = 0
    crc_mismatches = 0
    for consumer in sorted(final.values(), key=lambda module: module.name):
        for symbol, expected in sorted(consumer.imports.items()):
            candidates = providers.get(symbol, [])
            matching = sorted(label for label, value in candidates if value == expected)
            actual_values = sorted({crc(value) for _label, value in candidates})
            if matching:
                status = "MATCH"
            elif candidates:
                status = "CRC_MISMATCH"
                crc_mismatches += 1
            else:
                status = "UNRESOLVED"
                unresolved += 1
            import_rows.append(
                [
                    consumer.name,
                    consumer.provenance,
                    symbol,
                    crc(expected),
                    ",".join(sorted(label for label, _value in candidates)),
                    ",".join(actual_values),
                    ",".join(matching),
                    status,
                ]
            )
    write_tsv(
        args.out_dir / "import-resolution.tsv",
        [
            "consumer",
            "consumer_provenance",
            "symbol",
            "expected_crc",
            "candidate_providers",
            "candidate_crcs",
            "matching_providers",
            "status",
        ],
        import_rows,
    )

    signed = dict(replacements)
    closure_rows = [
        [name, "SOURCE_REPLACEMENT", "", ""] for name in sorted(replacements)
    ]
    while True:
        edges = source_provider_edges(final.values(), signed)
        additions = {
            name: edges[name]
            for name in edges
            if name not in signed
        }
        if not additions:
            break
        for name, reasons in sorted(additions.items()):
            signed[name] = final[name]
            first_symbol, first_provider = reasons[0]
            closure_rows.append([name, "RE_SIGN_STOCK", first_symbol, first_provider])
    write_tsv(
        args.out_dir / "protected-export-signing-closure.tsv",
        ["module", "required_action", "first_import", "signed_provider"],
        sorted(closure_rows),
    )

    external_edges = source_provider_edges(external, signed)
    external_rows = []
    for record in sorted(external, key=lambda module: (module.area, module.name)):
        for symbol, providers_for_symbol in external_edges.get(record.name, []):
            external_rows.append([record.area, record.name, record.path, symbol, providers_for_symbol])
    write_tsv(
        args.out_dir / "external-signed-provider-edges.tsv",
        ["external_area", "consumer", "path", "import", "new_signed_provider"],
        external_rows,
    )

    module_rows = []
    for record in sorted(final.values(), key=lambda module: module.name):
        action = "SOURCE_REPLACEMENT" if record.name in replacements else (
            "RE_SIGN_STOCK" if record.name in signed else "RETAIN_STOCK"
        )
        module_rows.append(
            [
                record.name,
                record.path,
                record.provenance,
                record.path.stat().st_size,
                record.sha256,
                len(record.imports),
                len(record.exports),
                "yes" if record.signed else "no",
                ",".join(record.depends),
                action,
            ]
        )
    write_tsv(
        args.out_dir / "candidate-modules.tsv",
        [
            "module",
            "path",
            "provenance",
            "bytes",
            "sha256",
            "imports",
            "exports",
            "input_signed",
            "depends",
            "required_staging_action",
        ],
        module_rows,
    )

    summary = {
        "stock_vendor_modules": len(stock),
        "source_replacements": len(replacements),
        "protected_export_signed_closure": len(signed),
        "re_sign_stock_modules": len(signed) - len(replacements),
        "retained_external_modules": len(external),
        "external_signed_provider_edges": len(external_rows),
        "replacement_contract_failures": contract_failures,
        "allowed_import_contract_changes": sorted(allowed_import_changes),
        "unresolved_imports": unresolved,
        "crc_mismatches": crc_mismatches,
        "result": "PASS"
        if not (contract_failures or unresolved or crc_mismatches)
        and not (args.fail_external_signed_provider_edges and external_rows)
        else "FAIL",
    }
    (args.out_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0 if summary["result"] == "PASS" else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
