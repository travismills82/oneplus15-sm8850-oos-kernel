#!/usr/bin/env python3
"""Read-only, fail-closed liblp capacity check; never resizes or writes images.

Format authority: AOSP system/core/fs_mgr/liblp/include/liblp/metadata_format.h
(Android 16). A raw super image or lpmake metadata image must include the normal
4096-byte reserved prefix and both geometry/metadata copies. Android sparse
images must first be converted independently; properties are not LP metadata.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import sys


class Invalid(ValueError):
    pass


def require(ok, message):
    if not ok:
        raise Invalid(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def name(raw):
    head, _, tail = raw.partition(b"\0")
    require(not any(tail), "nonzero bytes after partition-name terminator")
    # libsnapshot creates real LP entries as <snapshot-name>-cow. The old
    # metadata-format comment omits '-', but liblp accepts these bounded
    # names and SnapshotManager passes them to MetadataBuilder::AddPartition.
    # Keep path separators, whitespace, non-ASCII and embedded suffixes invalid.
    require(head and all(c in b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in head),
            "invalid LP name")
    return head.decode("ascii")


def geometry(f, offset):
    f.seek(offset)
    data = f.read(52)
    require(len(data) == 52, "truncated geometry")
    magic, size = struct.unpack_from("<II", data)
    require(magic == 0x616C4467 and size == 52, "unsupported LP geometry")
    require(digest(data[:8] + bytes(32) + data[40:]) == data[8:40].hex(), "geometry checksum mismatch")
    maximum, slots, block = struct.unpack_from("<III", data, 40)
    require(maximum >= 128 and maximum % 512 == 0 and maximum <= 64 * 1024 * 1024,
            "invalid metadata_max_size")
    require(1 <= slots <= 32 and block >= 512 and block % 512 == 0 and not block & (block - 1),
            "invalid slot count/logical block size")
    return data, maximum, slots, block


def metadata(f, offset, maximum):
    f.seek(offset)
    prefix = f.read(128)
    require(len(prefix) == 128, "truncated metadata header")
    magic, major, minor, size = struct.unpack_from("<IHHI", prefix)
    require(magic == 0x414C5030 and major == 10 and minor <= 2, "unsupported metadata version")
    require(size == (256 if minor >= 2 else 128), "unsupported metadata header size")
    table_size = struct.unpack_from("<I", prefix, 44)[0]
    require(size + table_size <= maximum, "metadata exceeds slot")
    f.seek(offset)
    header = f.read(size)
    tables = f.read(table_size)
    require(len(header) == size and len(tables) == table_size, "truncated metadata tables")
    require(digest(header[:12] + bytes(32) + header[44:]) == header[12:44].hex(), "header checksum mismatch")
    require(digest(tables) == header[48:80].hex(), "table checksum mismatch")
    if size == 256:
        require(not any(header[132:]), "nonzero reserved header fields")
    records = []
    occupied = []
    for index, expected in enumerate((52, 24, 48, 64)):
        start, count, width = struct.unpack_from("<III", header, 80 + 12 * index)
        require(width == expected and start + count * width <= table_size, "invalid table descriptor")
        end = start + count * width
        if count:
            require(all(end <= lo or start >= hi for lo, hi in occupied), "overlapping metadata tables")
            occupied.append((start, end))
        records.append([tables[start + i * width:start + (i + 1) * width] for i in range(count)])
    return header + tables, records


def inspect(path, slot=0):
    require(Path(path).is_file(), "metadata must be a captured regular image file, not a device or FIFO")
    with Path(path).open("rb") as f:
        first = geometry(f, 4096)
        second = geometry(f, 8192)
        require(first == second, "primary/backup geometry disagreement")
        _, maximum, slots, block = first
        require(0 <= slot < slots, "metadata slot outside geometry")
        base = 12288
        primary, records = metadata(f, base + slot * maximum, maximum)
        backup, _ = metadata(f, base + (slots + slot) * maximum, maximum)
        require(primary == backup, "primary/backup selected metadata disagreement")
    partitions, extents, groups, devices = records
    device_rows, intervals = [], {}
    for index, row in enumerate(devices):
        first_sector, alignment, alignment_offset, size, raw_name, flags = struct.unpack("<QIIQ36sI", row)
        require(size % 512 == 0 and first_sector * 512 < size, "invalid block-device capacity")
        require(alignment_offset < max(alignment, block), "invalid alignment offset")
        require(not flags & ~1, "unknown block-device flags")
        device_rows.append({"name": name(raw_name), "size": size, "first_usable_sector": first_sector,
                            "alignment": alignment, "alignment_offset": alignment_offset, "flags": flags})
        intervals[index] = []
    require(device_rows, "no LP block devices")
    require(base + 2 * slots * maximum <= device_rows[0]["first_usable_sector"] * 512,
            "metadata overlaps allocatable block-device space")
    group_rows = []
    for row in groups:
        raw_name, flags, maximum_size = struct.unpack("<36sIQ", row)
        require(not flags & ~1, "unknown group flags")
        group_rows.append({"name": name(raw_name), "maximum_size": maximum_size, "flags": flags,
                           "used_bytes": 0})
    result, used_extents = {}, set()
    for row in partitions:
        raw_name, attrs, first_extent, count, group = struct.unpack("<36sIIII", row)
        part_name = name(raw_name)
        require(part_name not in result and not attrs & ~15, "duplicate partition or unknown attributes")
        require(group < len(groups) and first_extent + count <= len(extents), "invalid partition extent/group")
        size, physical = 0, 0
        for i in range(first_extent, first_extent + count):
            require(i not in used_extents, "extent belongs to multiple partitions")
            used_extents.add(i)
            sectors, kind, target, device = struct.unpack("<QIQI", extents[i])
            require(sectors > 0 and kind in (0, 1), "unsupported/empty extent")
            require(sectors * 512 % block == 0, "extent length is not logical-block aligned")
            size += sectors * 512
            if kind == 0:
                require(device < len(devices), "extent block device missing")
                require(target * 512 % block == 0, "extent target is not logical-block aligned")
                dev = device_rows[device]
                require(target >= dev["first_usable_sector"] and (target + sectors) * 512 <= dev["size"],
                        "extent outside allocatable block-device space")
                require(all(target + sectors <= lo or target >= hi for lo, hi, _ in intervals[device]),
                        "physical extent overlap in selected metadata slot")
                intervals[device].append((target, target + sectors, part_name))
                physical += sectors * 512
            else:
                require(target == 0 and device == 0, "nonzero zero-extent target")
        group_rows[group]["used_bytes"] += size
        result[part_name] = {"size": size, "physical_bytes": physical, "attributes": attrs,
                             "group": group_rows[group]["name"], "extents": count}
    require(len(used_extents) == len(extents), "orphan metadata extents")
    for group in group_rows:
        maximum_size = group["maximum_size"]
        require(not maximum_size or group["used_bytes"] <= maximum_size, "existing group exceeds limit")
        group["headroom_bytes"] = maximum_size - group["used_bytes"] if maximum_size else None
    for index, device in enumerate(device_rows):
        device["allocated_extent_bytes"] = sum((hi - lo) * 512 for lo, hi, _ in intervals[index])
        device["unallocated_extent_bytes"] = (device["size"] - device["first_usable_sector"] * 512
                                               - device["allocated_extent_bytes"])
    header_size = struct.unpack_from("<I", primary, 8)[0]
    flags = struct.unpack_from("<I", primary, 128)[0] if header_size == 256 else 0
    return {"selected_metadata_sha256": digest(primary), "selected_slot": slot, "metadata_slots": slots,
            "metadata_header_flags": flags, "virtual_ab_flag": bool(flags & 1),
            "logical_block_size": block, "partitions": result, "groups": group_rows,
            "block_devices": device_rows}


def evaluate(report, candidates, snapshot_state):
    outcomes = []
    for partition, path in candidates:
        require(partition in report["partitions"], "candidate partition absent: " + partition)
        part = report["partitions"][partition]
        require(not any(d["flags"] for d in report["block_devices"])
                and not any(g["flags"] for g in report["groups"]),
                "retrofit slot-suffixed device/group names require explicit mapping")
        require(not part["attributes"] & (2 | 8), "slot-suffixed or disabled candidate requires explicit mapping")
        require(part["physical_bytes"] == part["size"], "candidate has zero extents without writable physical backing")
        path = Path(path)
        require(path.is_file() and path.stat().st_size > 0, "candidate must be a nonempty regular image file")
        with path.open("rb") as f:
            before = os.fstat(f.fileno())
            require(stat.S_ISREG(before.st_mode) and before.st_size > 0, "candidate must be a nonempty regular image file")
            require(f.read(4) != b"\x3a\xff\x26\xed", "Android sparse candidate must be expanded before capacity check")
            f.seek(0)
            sha = hashlib.file_digest(f, "sha256").hexdigest()
            after = os.fstat(f.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        require(identity(before) == identity(after) == identity(path.stat()), "candidate changed during inspection")
        size = before.st_size
        block = report["logical_block_size"]
        aligned = ((size + block - 1) // block) * block
        outcomes.append({"partition": partition, "image": str(path), "image_bytes": size, "image_sha256": sha,
                         "aligned_bytes": aligned, "partition_bytes": part["size"],
                         "capacity_deficit_bytes": max(0, aligned - part["size"]),
                         "fits_without_resize": aligned <= part["size"]})
    report["candidates"] = outcomes
    report["snapshot_state"] = snapshot_state
    report["capacity_status"] = "PASS" if outcomes and all(x["fits_without_resize"] for x in outcomes) else "FAIL"
    report["deployment_status"] = "NOT_AUTHORIZED"
    report["snapshot_gate"] = "PASS" if snapshot_state == "none" else "BLOCKED"
    report["scope"] = ("Selected-slot existing-partition fit only; no resizing, AVB/signing, filesystem, SELinux, "
                       "inactive-slot or live device identity approval. Snapshot state is operator-supplied and must be freshly verified.")
    return report


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--metadata", type=Path)
    ap.add_argument("--slot", type=int, default=0)
    ap.add_argument("--snapshot-state", choices=("unknown", "none", "active"), default="unknown")
    ap.add_argument("--candidate", action="append", default=[], metavar="PARTITION=IMAGE")
    args = ap.parse_args(argv)
    try:
        require(args.metadata is not None, "actual super/LP metadata was not supplied; properties and raw DLKM sizes cannot replace it")
        candidates = []
        for item in args.candidate:
            require("=" in item, "candidate must be PARTITION=IMAGE")
            partition, path = item.split("=", 1)
            require(partition and path and partition not in [p for p, _ in candidates], "invalid/duplicate candidate")
            candidates.append((partition, path))
        report = evaluate(inspect(args.metadata, args.slot), candidates, args.snapshot_state)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0 if report["capacity_status"] == "PASS" and report["snapshot_gate"] == "PASS" else 2
    except (Invalid, OSError, struct.error) as error:
        print(json.dumps({"capacity_status": "BLOCKED", "deployment_status": "NOT_AUTHORIZED",
                          "reason": str(error)}, indent=2))
        return 2


if __name__ == "__main__":
    sys.exit(main())
