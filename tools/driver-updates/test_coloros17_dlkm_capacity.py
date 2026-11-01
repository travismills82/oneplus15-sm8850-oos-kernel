#!/usr/bin/env python3
"""Synthetic format and failure-path tests; not a real-device capacity claim."""

import importlib.util
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock

SPEC = importlib.util.spec_from_file_location("capacity", Path(__file__).with_name("validate-coloros17-dlkm-capacity.py"))
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)


def fixture(*, partition_sectors=16, overlap=False, group_max=16384, duplicate=False, zero=False,
            partition_name=b"vendor_dlkm_a"):
    partition = struct.pack("<36sIIII", partition_name, 1, 0, 1, 0)
    extent = struct.pack("<QIQI", partition_sectors, 1 if zero else 0, 0 if zero else 2048, 0)
    if overlap:
        partition += struct.pack("<36sIIII", b"system_dlkm_a", 1, 1, 1, 0)
        extent += struct.pack("<QIQI", 8, 0, 2048, 0)
    if duplicate:
        partition += struct.pack("<36sIIII", b"system_dlkm_a", 1, 0, 1, 0)
    group = struct.pack("<36sIQ", b"qti_dynamic_partitions_a", 0, group_max)
    device = struct.pack("<QIIQ36sI", 2048, 1048576, 0, 4194304, b"super", 0)
    tables = partition + extent + group + device
    header = bytearray(256)
    struct.pack_into("<IHHI", header, 0, 0x414C5030, 10, 2, 256)
    struct.pack_into("<I", header, 44, len(tables))
    header[48:80] = bytes.fromhex(M.digest(tables))
    offset = 0
    for index, (data, width) in enumerate(((partition, 52), (extent, 24), (group, 48), (device, 64))):
        struct.pack_into("<III", header, 80 + 12 * index, offset, len(data) // width, width)
        offset += len(data)
    struct.pack_into("<I", header, 128, 1)
    header[12:44] = bytes.fromhex(M.digest(header))
    geometry = bytearray(struct.pack("<II32sIII", 0x616C4467, 52, bytes(32), 4096, 2, 4096))
    geometry[8:40] = bytes.fromhex(M.digest(geometry))
    image = bytearray(12288 + 4 * 4096)
    image[4096:4148] = geometry
    image[8192:8244] = geometry
    for slot in range(4):
        begin = 12288 + slot * 4096
        image[begin:begin + len(header) + len(tables)] = header + tables
    return image


class CapacityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.metadata = Path(self.tmp.name) / "super_metadata.img"
        self.candidate = Path(self.tmp.name) / "vendor_dlkm.img"
        self.candidate.write_bytes(bytes(8192))

    def inspect(self, data):
        self.metadata.write_bytes(data)
        return M.inspect(self.metadata)

    def test_real_format_fixture_and_current_capacity(self):
        report = M.evaluate(self.inspect(fixture()), [("vendor_dlkm_a", self.candidate)], "none")
        self.assertEqual(report["capacity_status"], "PASS")
        self.assertEqual(report["snapshot_gate"], "PASS")
        self.assertEqual(report["deployment_status"], "NOT_AUTHORIZED")
        self.assertEqual(report["partitions"]["vendor_dlkm_a"]["size"], 8192)

    def test_exact_deficit_no_resize(self):
        self.candidate.write_bytes(bytes(8193))
        report = M.evaluate(self.inspect(fixture()), [("vendor_dlkm_a", self.candidate)], "none")
        self.assertEqual(report["capacity_status"], "FAIL")
        self.assertEqual(report["candidates"][0]["capacity_deficit_bytes"], 4096)

    def test_real_snapshot_cow_name_is_accepted(self):
        report = self.inspect(fixture(partition_name=b"vendor_dlkm_b-cow"))
        self.assertEqual(report["partitions"]["vendor_dlkm_b-cow"]["size"], 8192)

    def test_cow_name_does_not_bypass_overlap_check(self):
        with self.assertRaisesRegex(M.Invalid, "physical extent overlap"):
            self.inspect(fixture(partition_name=b"vendor_dlkm_b-cow", overlap=True))

    def test_cow_name_does_not_bypass_snapshot_gate(self):
        report = self.inspect(fixture(partition_name=b"vendor_dlkm_b-cow"))
        result = M.evaluate(report, [("vendor_dlkm_b-cow", self.candidate)], "active")
        self.assertEqual(result["snapshot_gate"], "BLOCKED")

    def test_lp_name_still_rejects_paths_and_hidden_suffixes(self):
        for value in (b"../vendor", b"vendor/dlkm", b"vendor dlkm", b"vendor\0hidden", b"\xffvendor"):
            with self.subTest(value=value), self.assertRaises(M.Invalid):
                M.name(struct.pack("<36s", value))

    def test_unknown_and_active_snapshots_do_not_pass(self):
        for state in ("unknown", "active"):
            report = M.evaluate(self.inspect(fixture()), [("vendor_dlkm_a", self.candidate)], state)
            self.assertEqual(report["snapshot_gate"], "BLOCKED")

    def test_geometry_checksum(self):
        data = fixture()
        data[4136] ^= 1
        with self.assertRaisesRegex(M.Invalid, "geometry checksum"):
            self.inspect(data)

    def test_header_checksum(self):
        data = fixture()
        data[12288 + 128] ^= 1
        with self.assertRaisesRegex(M.Invalid, "header checksum"):
            self.inspect(data)

    def test_tables_checksum(self):
        data = fixture()
        data[12288 + 256] ^= 1
        with self.assertRaisesRegex(M.Invalid, "table checksum"):
            self.inspect(data)

    def test_valid_but_divergent_backup_rejected(self):
        data = fixture()
        changed = fixture(partition_sectors=8)
        data[12288 + 8192:12288 + 12288] = changed[12288 + 8192:12288 + 12288]
        with self.assertRaisesRegex(M.Invalid, "metadata disagreement"):
            self.inspect(data)

    def test_physical_overlap_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "physical extent overlap"):
            self.inspect(fixture(overlap=True))

    def test_duplicate_extent_ownership_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "multiple partitions"):
            self.inspect(fixture(duplicate=True))

    def test_group_limit_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "group exceeds"):
            self.inspect(fixture(group_max=4096))

    def test_wrong_partition_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "partition absent"):
            M.evaluate(self.inspect(fixture()), [("vendor_dlkm_b", self.candidate)], "none")

    def test_sparse_candidate_rejected(self):
        self.candidate.write_bytes(b"\x3a\xff\x26\xed" + bytes(100))
        with self.assertRaisesRegex(M.Invalid, "sparse candidate"):
            M.evaluate(self.inspect(fixture()), [("vendor_dlkm_a", self.candidate)], "none")

    def test_truncated_metadata_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "truncated"):
            self.inspect(fixture()[:10000])

    def test_empty_candidates_not_a_pass(self):
        report = M.evaluate(self.inspect(fixture()), [], "none")
        self.assertEqual(report["capacity_status"], "FAIL")

    def test_zero_extent_has_no_writable_capacity(self):
        with self.assertRaisesRegex(M.Invalid, "zero extents"):
            M.evaluate(self.inspect(fixture(zero=True)), [("vendor_dlkm_a", self.candidate)], "none")

    def test_unaligned_extent_rejected(self):
        with self.assertRaisesRegex(M.Invalid, "logical-block aligned"):
            self.inspect(fixture(partition_sectors=15))

    def test_empty_image_rejected(self):
        self.candidate.write_bytes(b'')
        with self.assertRaisesRegex(M.Invalid, "nonempty regular"):
            M.evaluate(self.inspect(fixture()), [("vendor_dlkm_a", self.candidate)], "none")

    def test_changed_image_during_hash_is_rejected(self):
        report = self.inspect(fixture())
        original = M.hashlib.file_digest

        def mutate(f, algorithm):
            result = original(f, algorithm)
            self.candidate.write_bytes(bytes(4096))
            return result

        with mock.patch.object(M.hashlib, 'file_digest', side_effect=mutate):
            with self.assertRaisesRegex(M.Invalid, "changed during"):
                M.evaluate(report, [("vendor_dlkm_a", self.candidate)], "none")

    def test_metadata_fifo_is_rejected_without_opening_it(self):
        fifo = Path(self.tmp.name) / 'not-an-image'
        os.mkfifo(fifo)
        with self.assertRaisesRegex(M.Invalid, "captured regular image"):
            M.inspect(fifo)


if __name__ == "__main__":
    unittest.main()
