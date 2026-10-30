#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Output-free unit tests for the binary-inventory helpers."""
import csv
import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location(
    "inventory_under_test", Path(__file__).with_name("coloros17-module-inventory.py")
)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FakeElf:
    def __init__(self, data):
        self.data = data
        self.sections = [SimpleNamespace(section_type=7, offset=0, size=len(data))]

    def slice(self, offset, size):
        return self.data[offset:offset + size]


class InventoryTests(unittest.TestCase):
    def test_canonical_keeps_partition_out_of_name(self):
        self.assertEqual(module.canonical("/vendor/lib/modules/qcom-scm.ko"), "qcom_scm")

    def test_modinfo_nul_and_colon_values(self):
        parsed = module.parse_modinfo_output(
            b"filename: /stock/a.ko\0name=a\0parm=mode:description\0sig_key=AB:CD\0"
            b"alias=one\0alias=two\0depends=\0"
        )
        self.assertEqual(parsed["name"], ["a"])
        self.assertEqual(parsed["parm"], ["mode:description"])
        self.assertEqual(parsed["sig_key"], ["AB:CD"])
        self.assertEqual(parsed["alias"], ["one", "two"])
        self.assertEqual(parsed["depends"], [""])

    def test_note_build_id(self):
        self.assertEqual(module.build_id(FakeElf(struct.pack("<III", 4, 4, 3) + b"GNU\0ABCD")), "41424344")

    def test_truncated_note_is_not_silently_accepted(self):
        with self.assertRaisesRegex(ValueError, "truncated ELF note"):
            module.build_id(FakeElf(struct.pack("<III", 4, 8, 3) + b"GNU\0ABCD"))

    def test_conflicting_build_ids_are_rejected(self):
        one = struct.pack("<III", 4, 4, 3) + b"GNU\0ABCD"
        two = struct.pack("<III", 4, 4, 3) + b"GNU\0EFGH"
        with self.assertRaisesRegex(ValueError, "conflicting GNU build IDs"):
            module.build_id(FakeElf(one + two))

    def test_missing_note(self):
        self.assertEqual(module.build_id(FakeElf(b"")), "ABSENT")

    def test_slhc_is_network_not_storage(self):
        self.assertEqual(module.subsystem("slhc", []), "DATAIPA_RMNET_NETWORK")

    def test_tsv_cleaning(self):
        self.assertEqual(module.clean("x\ty\nz\r"), "x y z ")

    def test_empty_final_dependency_field_preserves_rectangular_tsv(self):
        fields = ["partition", "consumer", "dependencies"]
        row = {"partition": "system_dlkm", "consumer": "example.ko",
               "dependencies": module.depmod_dependency_field(" \t ")}
        with tempfile.TemporaryDirectory(prefix="module-inventory-test-") as directory:
            path = Path(directory) / "depmod.tsv"
            module.write_tsv(path, fields, [row])
            text = path.read_text()
        self.assertTrue(all(not line.endswith("\t") for line in text.splitlines()))
        parsed = list(csv.DictReader(text.splitlines(), delimiter="\t"))
        self.assertEqual(parsed, [{"partition": "system_dlkm", "consumer": "example.ko", "dependencies": "NONE"}])
        self.assertEqual(module.decode_depmod_dependency_field(parsed[0]["dependencies"]), [])

    def test_dependency_encoding_preserves_nonempty_loader_order(self):
        value = module.depmod_dependency_field(" /system/lib/modules/first.ko /vendor/lib/modules/second.ko ")
        self.assertEqual(value, "/system/lib/modules/first.ko /vendor/lib/modules/second.ko")
        self.assertEqual(module.decode_depmod_dependency_field(value),
                         ["/system/lib/modules/first.ko", "/vendor/lib/modules/second.ko"])

    def test_dependency_sentinel_collision_and_missing_column_fail_closed(self):
        with self.assertRaisesRegex(ValueError, "reserved NONE sentinel"):
            module.depmod_dependency_field("NONE")
        with self.assertRaisesRegex(ValueError, "missing dependency field"):
            module.decode_depmod_dependency_field("")

    def test_repository_candidates_are_path_based_not_module_name(self):
        fields = module.provenance_fields("rmnet_core", [], {}, {})
        self.assertEqual(fields["source_repository_candidates"], "UNKNOWN")
        self.assertEqual(fields["original_source_repository"], "UNVERIFIED_FOR_STOCK_BINARY")
        self.assertEqual(fields["source_repository_candidate_evidence"], "NO_LOCAL_PATH_CANDIDATE")

    def test_common_and_soc_repository_candidates_remain_distinct(self):
        candidates = module.source_repository_candidates([
            "kernel_platform/common/drivers/usb/Makefile",
            "kernel_platform/soc-repo/drivers/usb/Makefile",
        ], {})
        self.assertEqual(set(candidates.split(";")), {module.COMMON_REPOSITORY, module.SOC_REPOSITORY})

    def test_vendor_candidate_retains_oneplus_and_audited_clo_references(self):
        clo_url = "https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/datarmnet.git"
        candidates = module.source_repository_candidates([
            "vendor/qcom/opensource/datarmnet/core/rmnet_config.c",
            "vendor/qcom/opensource/datarmnet/core/Kbuild",
        ], {"Data RMNET": clo_url})
        self.assertEqual(set(candidates.split(";")), {module.MODULES_REPOSITORY, clo_url})

    def test_missing_firmware_metadata_is_not_no_dependency(self):
        fields = module.provenance_fields("bluetooth", [], {}, {})
        self.assertEqual(fields["firmware_modinfo_declarations"], "NOT_DECLARED_NOT_PROOF_NONE")
        self.assertEqual(fields["firmware_dependency_status"], "NOT_DECLARED_RUNTIME_DEPENDENCIES_UNVERIFIED")

    def test_all_declared_firmware_names_are_preserved_without_verification_claim(self):
        fields = module.provenance_fields("example", [], {"firmware": ["qca/one.bin", "qca/two.bin"]}, {})
        self.assertEqual(fields["firmware_modinfo_declarations"], "qca/one.bin;qca/two.bin")
        self.assertEqual(fields["firmware_dependency_status"], "DECLARED_FILENAMES_ONLY_RUNTIME_REQUIREMENTS_UNVERIFIED")

    def test_partial_private_boundary_review_does_not_verify_exact_source(self):
        fields = module.provenance_fields("rmnet_core", [], {}, {})
        self.assertEqual(fields["private_header_dependency_status"], "UNVERIFIED_EXACT_SOURCE")
        self.assertIn("RELATED_PARTIAL_REVIEW:", fields["private_header_boundary_review"])
        self.assertIn("coloros17-rmnet-private-abi.md", fields["private_header_boundary_review"])
        unrelated = module.provenance_fields("example", [], {}, {})
        self.assertEqual(unrelated["private_header_boundary_review"], "NO_PER_MODULE_PRIVATE_HEADER_REVIEW")


if __name__ == "__main__":
    unittest.main()
