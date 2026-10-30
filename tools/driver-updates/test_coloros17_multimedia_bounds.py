#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host models and static source contracts, NOT kernel/runtime tests.

These small models check intended range/ordering properties of the selected
multimedia fixes. They do not execute a kernel, DMA, firmware, usercopy, locking,
or reference-counting implementation and cannot qualify a device/module.
"""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
VENDOR = ROOT / "vendor/qcom/opensource"


def body(path, function):
    text = (VENDOR / path).read_text()
    match = re.search(r"\b" + re.escape(function) + r"\s*\([^;]*?\)\s*\{", text)
    if not match:
        raise AssertionError(f"Missing function {function}")
    start = match.end()
    depth = 1
    for end in range(start, len(text)):
        depth += (text[end] == "{") - (text[end] == "}")
        if not depth:
            return text[start:end]
    raise AssertionError(f"Unclosed function {function}")


def fits_backing(length, offset, size, used):
    return offset < length and size >= used and size <= length - offset


def fits_dump(length, offset, blocks):
    if offset >= length:
        return False
    for size in blocks:
        if length - offset < size:
            return False
        offset += size
    return True


class BoundsModels(unittest.TestCase):
    def test_ope_exact_fit(self):
        self.assertTrue(fits_backing(128, 32, 96, 64))

    def test_ope_backing_size_not_just_used_length(self):
        self.assertFalse(fits_backing(128, 32, 97, 64))

    def test_ope_offset_rejected_before_subtraction(self):
        self.assertFalse(fits_backing(128, 129, 1, 1))

    def test_ope_used_length_cannot_exceed_backing(self):
        self.assertFalse(fits_backing(128, 0, 64, 65))

    def test_icp_exact_fit_of_all_blocks(self):
        self.assertTrue(fits_dump(128, 16, [48, 32, 32]))

    def test_icp_second_block_must_fit(self):
        self.assertFalse(fits_dump(64, 16, [48, 1, 0]))

    def test_icp_third_block_must_fit(self):
        self.assertFalse(fits_dump(96, 16, [48, 32, 1]))

    def test_icp_offset_underflow_rejected(self):
        self.assertFalse(fits_dump(64, 65, [1]))

    def test_kgsl_split_vma_fault_keeps_page_offset(self):
        page_size = 4096
        split_pgoff, relative_fault = 3, page_size
        offset = split_pgoff * page_size + relative_fault
        self.assertEqual(offset // page_size, 4)
        self.assertNotEqual(relative_fault // page_size, offset // page_size)

    def test_fcg_actual_blob_size_is_independent(self):
        computed, claimed, actual = 128, 128, 64
        self.assertEqual(computed, claimed)
        self.assertGreater(computed, actual)


class SourceContracts(unittest.TestCase):
    def test_kgsl_memstore_token_reset(self):
        source = body("graphics-kernel/kgsl.c", "kgsl_mmap_memstore")
        self.assertIn("vma->vm_pgoff = 0;", source)

    def test_kgsl_offset_uses_wide_page_offset_before_bounds(self):
        source = body("graphics-kernel/kgsl_sharedmem.c", "kgsl_paged_vmfault")
        self.assertIn("u64 offset = ((u64)vma->vm_pgoff << PAGE_SHIFT)", source)
        self.assertLess(source.index("vma->vm_pgoff"), source.index("offset >= memdesc->size"))

    def test_jpeg_manager_lock_precedes_context_access(self):
        source = body("camera-kernel/drivers/cam_jpeg/jpeg_hw/cam_jpeg_hw_mgr.c",
                      "cam_jpeg_mgr_release_hw")
        self.assertEqual(source.count("mutex_lock(&hw_mgr->hw_mgr_mutex)"), 1)
        self.assertLess(source.index("mutex_lock(&hw_mgr->hw_mgr_mutex)"),
                        source.index("ctx_data = (struct cam_jpeg_hw_ctx_data *)"))

    def test_ife_capacity_precedes_indexed_write(self):
        source = body("camera-kernel/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c",
                      "cam_ife_hw_mgr_acquire_res_ife_out_pixel")
        self.assertLess(source.index("comp_grp->num_res >= CAM_NUM_OUT_PER_COMP_IRQ_MAX"),
                        source.index("comp_grp->res_id[comp_grp->num_res]"))

    def test_icp_all_three_blocks_are_checked_before_writes(self):
        source = body("camera-kernel/drivers/cam_icp/icp_hw/icp_hw_mgr/cam_icp_hw_mgr.c",
                      "cam_icp_mgr_hw_dump")
        self.assertEqual(source.count("if (remain_len < min_len)"), 3)
        for marker in ("CAM_ICP_DUMP_NUM_WORDS_MGR", "CAM_ICP_DUMP_NUM_WORDS_REQ"):
            section = source[source.index(marker):]
            self.assertLess(section.index("if (remain_len < min_len)"),
                            section.index("dst ="))

    def test_ope_dump_copy_uses_checked_size(self):
        source = body("camera-kernel/drivers/cam_ope/ope_hw_mgr/cam_ope_hw_mgr.c",
                      "cam_ope_dump_frame_process")
        self.assertRegex(source, r"len - cmd_desc\[i\]\.offset\) <\s*cmd_desc\[i\]\.size")
        self.assertIn("cpu_addr_u, cmd_desc[i].size)", source)

    def test_isp_dual_port_validated_before_map_use(self):
        source = body("camera-kernel/drivers/cam_isp/isp_hw_mgr/hw_utils/cam_isp_packet_parser.c",
                      "cam_isp_update_dual_config")
        self.assertIn("port_id < out_base || port_id >= out_max", source)
        self.assertLess(source.index("out_map[port_id & 0xFF] == 0xFF"),
                        source.index("hw_mgr_res = &res_list_isp_out"))

    def test_fcg_real_blob_length_reaches_both_callers(self):
        source = (VENDOR / "camera-kernel/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c").read_text()
        helper = body("camera-kernel/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c",
                      "cam_isp_blob_fcg_config_prepare")
        self.assertLess(helper.index("fcg_size > blob_size"),
                        helper.index("cam_isp_copy_fcg_config("))
        self.assertIn("prepare, blob_size, CAM_ISP_HW_TYPE_VFE", source)
        self.assertIn("prepare, blob_size, CAM_ISP_HW_TYPE_SFE", source)

    def test_context_cached_map_pointers_cleared(self):
        source = body("camera-kernel/drivers/cam_core/cam_context_utils.c",
                      "cam_context_free_mem_hw_entries")
        for field in ("out_map_entries", "in_map_entries", "hw_update_entries"):
            self.assertIn(f"ctx->req_list[i].{field} = NULL;", source)

    def test_context_prepare_checks_maps_before_parse_and_use(self):
        source = body("camera-kernel/drivers/cam_core/cam_context_utils.c",
                      "cam_context_prepare_dev_to_hw")
        self.assertLess(source.index("!req->hw_update_entries"),
                        source.index("cam_context_parse_config_cmd"))

    def test_camera_fence_identity_checked_before_reference_operation(self):
        source = body("camera-kernel/drivers/cam_sync/cam_sync_dma_fence.c",
                      "cam_dma_fence_get_put_ref")
        self.assertLess(source.index("dma_fence != expected_fence"),
                        source.index("dma_fence_get(dma_fence)"))

    def test_camera_last_put_invalidates_row(self):
        source = body("camera-kernel/drivers/cam_sync/cam_sync_dma_fence.c",
                      "cam_dma_fence_get_put_ref")
        self.assertIn("kref_put(&dma_fence->refcount, dma_fence_release)", source)
        self.assertIn("row->fence = NULL;", source)
        self.assertIn("row->state = CAM_DMA_FENCE_STATE_INVALID;", source)
        debug = source[source.index("CAM_DBG(CAM_DMA_FENCE,"):]
        self.assertNotIn("dma_fence->", debug)

    def test_camera_dead_row_monitor_has_no_live_fence_requirement(self):
        source = body("camera-kernel/drivers/cam_sync/cam_sync_dma_fence.c",
                      "__cam_dma_fence_dump_monitor_array")
        self.assertIn("fence ? kref_read(&fence->refcount) : 0", source)
        self.assertIn("cam_generic_fence_dump_monitor_array(&obj_info)", source)

    def test_camera_invalid_row_owned_reference_released_outside_lock(self):
        source = body("camera-kernel/drivers/cam_sync/cam_sync_dma_fence.c",
                      "cam_dma_fence_get_put_ref")
        error = source[source.index("monitor_dump:"):]
        self.assertIn("expected_fence && !get_or_put", error)
        self.assertLess(error.index("spin_unlock_bh("),
                        error.index("dma_fence_put(expected_fence)"))

    def test_video_disallowed_type_does_not_modify_pending_ssr(self):
        source = body("video-driver/driver/vidc/src/msm_vidc_driver.c",
                      "msm_vidc_trigger_ssr")
        self.assertLess(source.index("if (!is_ssr_type_allowed(core, ssr_type))"),
                        source.index("ssr->ssr_type = ssr_type"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
