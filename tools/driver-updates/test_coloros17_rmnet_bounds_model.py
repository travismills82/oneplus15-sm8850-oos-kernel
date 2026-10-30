#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline RMNET boundary models, NOT a kernel/SKB/module runtime test.

Scalar models express review requirements, not proof that driver code enforces
them. Separate static SourceContracts inspect real function bodies and guard
ordering, without executing kernel code or proving control-flow correctness.
One test compiles the exact source checksum-workaround predicate with its source
declarations on the host. No network, device, kernel module or firmware is used.
Only a temporary host executable is created and removed.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from functools import lru_cache
from pathlib import Path

sys.dont_write_bytecode = True
REPO = Path(__file__).resolve().parents[2]
CORE = Path("vendor/qcom/opensource/datarmnet/core")
MAP_SIZE, CSUM_SIZE, COAL_SIZE = 4, 4, 28


@lru_cache(maxsize=None)
def source_limit(name: str) -> int:
    source = (REPO / CORE / "rmnet_map.h").read_text()
    match = re.search(r"^#define\s+" + name + r"\s+\((\d+)\)\s*$", source, re.M)
    if match is None:
        raise ValueError(f"review source limit changed: {name}")
    return int(match.group(1))


def header_bytes(chunks: list[bytes], offset: int, length: int) -> bytes | None:
    """Logical checked-read model; does not implement skb_header_pointer()."""
    total = sum(map(len, chunks))
    if offset < 0 or length < 0 or offset > total or length > total - offset:
        return None
    return b"".join(chunks)[offset:offset + length]


def basic_bounds(frame: int, declared: int, pad: int, trailer: int = 0) -> bool:
    """The ea47 basic MAP/padding/trailer inequality, not an IP validator."""
    return frame >= MAP_SIZE and pad <= declared and frame >= MAP_SIZE + declared + trailer


def legacy_fragment_fit(total: int, head: int, fragments: list[int], packet: int | None = None) -> bool:
    """Legacy frag0 span requirement; not a model of SKB page ownership/pulls."""
    if total < MAP_SIZE or head or not fragments or fragments[0] < MAP_SIZE:
        return False
    required = total if packet is None else packet
    return MAP_SIZE <= required <= total and required <= fragments[0]


def count_bounds(counts: list[int], *, narrow_legacy: bool = False) -> bool:
    """Model both aggregate widths; do not invent a per-NLO count limit."""
    if len(counts) > source_limit("RMNET_MAP_V5_MAX_NLOS"):
        return False
    total = 0
    for count in counts:
        if not 0 <= count <= 255:
            raise ValueError("wire packet count is not u8")
        total += count
        if narrow_legacy:
            total &= 255
        if total > source_limit("RMNET_MAP_V5_MAX_PACKETS"):
            return False
    return True


def payload_bounds(hlen: int, available: int, nlos: list[tuple[int, int]]) -> bool:
    """The ea47 shared-header/cumulative-payload predicate for active NLOs."""
    total = 0
    for packet_len, count in nlos:
        if not count:
            continue
        if packet_len <= hlen:
            return False
        total += (packet_len - hlen) * count
        if hlen + total > available:
            return False
    return True


def nonlinear_gro_bounds(hlen: int, available: int, packet_len: int, count: int,
                         *, require_nonzero: bool) -> bool:
    """Review the original nonlinear fast-path inequality and proposed guard."""
    return (not require_nonzero or count != 0) and packet_len > hlen and (
        hlen + (packet_len - hlen) * count <= available
    )


def declaration(source: str, kind: str, name: str) -> str:
    match = re.search(r"\b" + kind + r"\s+" + name +
                      r"\s*\{[^}]*\}\s*(?:__aligned\(1\))?\s*;", source)
    if match is None:
        raise ValueError(f"source declaration changed: {name}")
    return match.group(0)


def checksum_workaround_function(source: str) -> str:
    match = re.search(r"\bbool\s+rmnet_map_v5_csum_buggy\([^)]*\)\s*\{", source)
    if match is None:
        raise ValueError("checksum-workaround source function changed")
    end, depth = match.end(), 1
    while depth and end < len(source):
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    if depth:
        raise ValueError("unterminated checksum-workaround function")
    return source[match.start():end]


def strip_comments_and_literals(source: str) -> str:
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda match: " " * len(match.group(0)), source, flags=re.S)


def block_end(code: str, start: int) -> int:
    if code[start] != "{":
        raise AssertionError("expected a source block")
    depth = 1
    pos = start + 1
    while pos < len(code) and depth:
        depth += (code[pos] == "{") - (code[pos] == "}")
        pos += 1
    if depth:
        raise AssertionError("unterminated source block")
    return pos


def source_function(source: str, name: str) -> str:
    code = strip_comments_and_literals(source)
    match = re.search(r"\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", code)
    if match is None:
        raise AssertionError(f"missing source function: {name}")
    end = block_end(code, match.end() - 1)
    return re.sub(r"\s+", "", code[match.start():end])


def require_guard(body: str, condition: str, action: str, before: str) -> None:
    needle = "if(" + condition + ")"
    start = body.find(needle)
    if start < 0:
        raise AssertionError(f"required source guard missing: {needle}")
    statement = start + len(needle)
    end = block_end(body, statement) if body[statement] == "{" else body.index(";", statement) + 1
    if action not in body[statement:end]:
        raise AssertionError(f"guard does not perform expected rejection: {action}")
    access = body.find(before)
    if access < 0 or end > access:
        raise AssertionError(f"source guard must precede access: {before}")


def check_deaggregate(body: str) -> None:
    require_guard(body, "skb->len<sizeof(*maph)", "returnNULL;", "rmnet_map_data_ptr(skb)")
    require_guard(body,
                  "skb_is_nonlinear(skb)&&(skb_headlen(skb)||!skb_shinfo(skb)->nr_frags||skb_frag_size(skb_shinfo(skb)->frags)<sizeof(*maph))",
                  "returnNULL;", "rmnet_map_data_ptr(skb)")
    for access in ("next_hdr)->header_type", "skb_append_pagefrags("):
        require_guard(body,
                      "skb_is_nonlinear(skb)&&packet_len>skb_frag_size(skb_shinfo(skb)->frags)",
                      "returnNULL;", access)
    assert "pskb_may_pull(" not in body, "head-moving pull changes the existing frag0 clone contract"


def check_ingress_fragment(body: str) -> None:
    require_guard(body,
                  "skb_is_nonlinear(skb)&&(skb_headlen(skb)||!skb_shinfo(skb)->nr_frags||skb_frag_size(skb_shinfo(skb)->frags)<skb->len)",
                  "gotofree_skb;", "rmnet_map_data_ptr(skb)")


def check_next_header(body: str) -> None:
    condition = "skb->len<sizeof(structrmnet_map_header)+sizeof(structrmnet_map_v5_csum_header)"
    for access in ("rmnet_map_get_next_hdr_type(skb)", "rmnet_map_get_csum_valid(skb)"):
        require_guard(body, condition, "return-EINVAL;", access)


def check_coal_header(body: str) -> None:
    condition = "skb->len<sizeof(structrmnet_map_header)+sizeof(*coal_hdr)"
    for access in ("rmnet_map_data_ptr(skb)", "coal_hdr->virtual_channel_id", "coal_hdr->num_nlos", "coal_hdr->nl_pairs["):
        require_guard(body, condition, "return-EINVAL;", access)


def check_counter(body: str) -> None:
    assert "unsignedintpkts=0;" in body, "aggregate packet counter must be unsigned int"
    require_guard(body, "pkts>RMNET_MAP_V5_MAX_PACKETS", "return-EINVAL;", "priv->stats.coal.coal_pkts+=pkts;")
    assert body.index("unsignedintpkts=0;") < body.index("pkts+=pkt;")


def check_nonlinear_gro(body: str) -> None:
    condition = "if(gro&&coal_hdr.num_nlos==1&&coal_hdr.csum_valid&&coal_hdr.nl_pairs[0].num_packets)"
    start = body.find(condition)
    assert start >= 0, "nonlinear GRO fast path lacks nonzero packet-count condition"
    pos = start + len(condition)
    block = body[pos:block_end(body, pos)]
    require_guard(block,
                  "gro_pkt_len<=gro_hlen||gro_hlen+(u32)(gro_pkt_len-gro_hlen)*coal_hdr.nl_pairs[0].num_packets>coal_desc->len",
                  "return;", "list_add_tail(")


def check_prepass(body: str, *, nonlinear: bool) -> None:
    assert "u32total_data_len=0;" in body, "cumulative byte counter must remain u32"
    prefix = "coal_hdr." if nonlinear else "coal_hdr->"
    loop = "for(nlo=0;nlo<" + prefix + "num_nlos;nlo++){"
    starts = [match.start() for match in re.finditer(re.escape(loop), body)]
    assert len(starts) == 2, "expected separate validation and delivery NLO loops"
    start = starts[0] + len(loop) - 1
    end = block_end(body, start)
    prepass = body[start:end]
    assert "if(!num_packets)continue;" in prepass, "empty NLO semantics changed"
    require_guard(prepass, "pkt_len<=hlen", "return;", "pkt_len-=hlen;")
    accumulation = "total_data_len+=(u32)pkt_len*num_packets;"
    assert accumulation in prepass, "whole-frame cumulative prevalidation missing"
    length = "coal_desc->len" if nonlinear else "coal_skb->len"
    # The byte bound is the final branch inside this loop, not an access after
    # it. Check the actual guarded block and reject before the delivery loop.
    bound = "if(hlen+total_data_len>" + length + "){"
    bound_start = prepass.find(bound)
    assert bound_start > prepass.index(accumulation), "cumulative bound absent or before accumulation"
    bound_body = prepass[bound_start + len(bound) - 1:]
    assert "return;" in bound_body, "cumulative prepass failure must reject"
    delivery = "__rmnet_frag_segment_data(" if nonlinear else "__rmnet_map_segment_coal_skb("
    assert delivery not in prepass and "list_add_tail(" not in prepass and "__skb_queue_tail(" not in prepass
    assert end <= starts[1] < body.index(delivery), "segmented delivery precedes complete prevalidation"
    # Independently bounded fast paths precede the nonlinear loop. The linear
    # checksum workaround is an explicit reuse-only exception; it does not use
    # an NLO-derived payload/GSO length and is checked by the exact host test.
    if not nonlinear:
        assert body.index("if(gro&&") >= end, "linear GRO must follow prepass"
    else:
        before = body[:starts[0]]
        buggy = "if(rmnet_map_v5_csum_buggy(&coal_hdr)&&!zero_csum){"
        pos = before.index(buggy) + len(buggy) - 1
        workaround = before[pos:block_end(before, pos)]
        require_guard(workaround,
                      "buggy_hlen>coal_desc->len||buggy_pkt_len<=buggy_hlen||buggy_pkt_len>coal_desc->len",
                      "return;", "list_add_tail(")
        check_nonlinear_gro(before)


class SourceContracts(unittest.TestCase):
    """Static lexical regressions in real source; NOT kernel execution."""

    @classmethod
    def setUpClass(cls):
        cls.map_source = (REPO / CORE / "rmnet_map_data.c").read_text()
        cls.frag_source = (REPO / CORE / "rmnet_descriptor.c").read_text()
        cls.ingress_source = (REPO / CORE / "rmnet_handlers.c").read_text()

    def test_deaggregate_guard_before_data_pointer(self):
        check_deaggregate(source_function(self.map_source, "rmnet_map_deaggregate"))

    def test_next_header_guard_before_dispatch_and_checksum_access(self):
        check_next_header(source_function(self.map_source, "rmnet_map_process_next_hdr_packet"))

    def test_complete_coal_header_guard_before_field_reads(self):
        check_coal_header(source_function(self.map_source, "rmnet_map_data_check_coal_header"))

    def test_both_actual_aggregate_counters_are_wide(self):
        check_counter(source_function(self.map_source, "rmnet_map_data_check_coal_header"))
        check_counter(source_function(self.frag_source, "rmnet_frag_data_check_coal_header"))

    def test_nonlinear_actual_gro_guard_and_byte_bound(self):
        check_nonlinear_gro(source_function(self.frag_source, "rmnet_frag_segment_coal_data"))

    def test_linear_actual_prepass_before_segment_delivery(self):
        check_prepass(source_function(self.map_source, "rmnet_map_segment_coal_skb"), nonlinear=False)

    def test_nonlinear_actual_prepass_and_bounded_fast_paths(self):
        check_prepass(source_function(self.frag_source, "rmnet_frag_segment_coal_data"), nonlinear=True)

    def test_upstream_ingress_guards_before_actual_reads_and_subtraction(self):
        body = source_function(self.ingress_source, "__rmnet_map_ingress_handler")
        require_guard(body, "skb->len<sizeof(*qmap)", "gotofree_skb;", "rmnet_map_data_ptr(skb)")
        check_ingress_fragment(body)
        require_guard(body, "pad>ntohs(qmap->pkt_len)", "gotofree_skb;", "len=ntohs(qmap->pkt_len)-pad;")

    def test_in_memory_negative_controls_reject_removed_guards(self):
        cases = [
            (check_deaggregate, source_function(self.map_source, "rmnet_map_deaggregate"),
             "if(skb->len<sizeof(*maph))returnNULL;", ""),
            (check_deaggregate, source_function(self.map_source, "rmnet_map_deaggregate"),
             "skb_headlen(skb)||!skb_shinfo(skb)->nr_frags||", ""),
            (check_deaggregate, source_function(self.map_source, "rmnet_map_deaggregate"),
             "if(skb_is_nonlinear(skb)&&packet_len>skb_frag_size(skb_shinfo(skb)->frags))returnNULL;", ""),
            (check_ingress_fragment, source_function(self.ingress_source, "__rmnet_map_ingress_handler"),
             "skb_frag_size(skb_shinfo(skb)->frags)<skb->len", "false"),
            (check_next_header, source_function(self.map_source, "rmnet_map_process_next_hdr_packet"),
             "if(skb->len<sizeof(structrmnet_map_header)+sizeof(structrmnet_map_v5_csum_header))return-EINVAL;", ""),
            (check_coal_header, source_function(self.map_source, "rmnet_map_data_check_coal_header"),
             "if(skb->len<sizeof(structrmnet_map_header)+sizeof(*coal_hdr))", "if(false)"),
            (check_counter, source_function(self.map_source, "rmnet_map_data_check_coal_header"),
             "unsignedintpkts=0;", "u8pkts=0;"),
            (check_counter, source_function(self.frag_source, "rmnet_frag_data_check_coal_header"),
             "unsignedintpkts=0;", "u8pkts=0;"),
            (check_nonlinear_gro, source_function(self.frag_source, "rmnet_frag_segment_coal_data"),
             "&&coal_hdr.nl_pairs[0].num_packets)", ")"),
        ]
        for checker, body, old, new in cases:
            with self.subTest(checker=checker.__name__, replacement=old):
                self.assertIn(old, body)
                with self.assertRaises(AssertionError):
                    checker(body.replace(old, new, 1))
        for source, name, nonlinear in (
            (self.map_source, "rmnet_map_segment_coal_skb", False),
            (self.frag_source, "rmnet_frag_segment_coal_data", True),
        ):
            body = source_function(source, name)
            with self.subTest(prepass=name):
                with self.assertRaises(AssertionError):
                    check_prepass(body.replace("total_data_len+=(u32)pkt_len*num_packets;", "", 1), nonlinear=nonlinear)


class BoundsModels(unittest.TestCase):
    def test_reviewed_wire_limits(self):
        self.assertEqual(source_limit("RMNET_MAP_V5_MAX_NLOS"), 6)
        self.assertEqual(source_limit("RMNET_MAP_V5_MAX_PACKETS"), 48)

    def test_linear_and_split_truncated_headers(self):
        for size in (MAP_SIZE, MAP_SIZE + CSUM_SIZE, MAP_SIZE + COAL_SIZE):
            for length in range(size + 1):
                data = bytes(range(length))
                for split in range(length + 1):
                    for chunks in ([data], [data[:split], data[split:]]):
                        with self.subTest(size=size, length=length, split=split):
                            result = header_bytes(chunks, 0, size)
                            self.assertEqual(result is not None, length >= size)

    def test_next_header_cannot_be_read_from_main_header_only(self):
        data = bytes(MAP_SIZE)
        self.assertIsNone(header_bytes([data], MAP_SIZE, 1))
        self.assertIsNone(header_bytes([data], MAP_SIZE, CSUM_SIZE))
        self.assertIsNone(header_bytes([data], MAP_SIZE, COAL_SIZE))

    def test_earlier_main_header_guard_is_required_before_deaggregation(self):
        for length in range(MAP_SIZE):
            self.assertIsNone(header_bytes([bytes(length)], 0, MAP_SIZE))

    def test_legacy_fragment_guards_reject_unsupported_spans(self):
        self.assertFalse(legacy_fragment_fit(32, 4, [28], packet=16))
        self.assertFalse(legacy_fragment_fit(32, 0, [], packet=16))
        self.assertFalse(legacy_fragment_fit(32, 0, [3, 29], packet=16))
        self.assertFalse(legacy_fragment_fit(32, 0, [15, 17], packet=16))
        self.assertFalse(legacy_fragment_fit(3, 0, [3], packet=3))
        self.assertFalse(legacy_fragment_fit(16, 0, [16], packet=17))

    def test_legacy_multi_fragment_aggregate_allows_each_fitting_frame(self):
        # The aggregate need not be in frag0; only the current cloned frame.
        self.assertTrue(legacy_fragment_fit(32, 0, [16, 16], packet=16))
        self.assertTrue(legacy_fragment_fit(16, 0, [16], packet=16))
        # Direct/deaggregated ingress receives one entire frame, not aggregate.
        self.assertFalse(legacy_fragment_fit(32, 0, [16, 16]))
        self.assertTrue(legacy_fragment_fit(32, 0, [32]))

    def test_padding_underflow_and_trailer_boundaries(self):
        self.assertFalse(basic_bounds(100, 20, 21))
        self.assertTrue(basic_bounds(24, 20, 20))  # zero IP payload needs a separate policy
        self.assertTrue(basic_bounds(28, 20, 0, trailer=4))
        self.assertFalse(basic_bounds(27, 20, 0, trailer=4))
        self.assertTrue(basic_bounds(65539, 65535, 63))

    def test_wide_aggregate_stops_representable_wrap(self):
        self.assertTrue(count_bounds([1, 255], narrow_legacy=True))
        self.assertFalse(count_bounds([1, 255]))
        self.assertTrue(count_bounds([48, 208], narrow_legacy=True))
        self.assertFalse(count_bounds([48, 208]))

    def test_aggregate_boundaries_and_empty_unused_nlos(self):
        self.assertTrue(count_bounds([48, 0, 0, 0, 0, 0]))
        self.assertTrue(count_bounds([8, 8, 8, 8, 8, 8]))
        self.assertFalse(count_bounds([48, 1]))
        self.assertFalse(count_bounds([0] * 7))
        for first in range(49):
            for second in range(256):
                self.assertEqual(count_bounds([first, second]), first + second <= 48)

    def test_nonempty_packet_length_cannot_underflow(self):
        for hlen in (28, 40, 48, 60, 120):
            for length in (0, hlen - 1, hlen):
                self.assertFalse(payload_bounds(hlen, 4096, [(length, 1)]))
            self.assertTrue(payload_bounds(hlen, hlen + 1, [(hlen + 1, 1)]))

    def test_cumulative_payload_boundary_and_late_rejection(self):
        # Shared header occurs once: 40 + 10*2 + 20*3 = 120, not 5*40.
        nlos = [(50, 2), (60, 3)]
        self.assertTrue(payload_bounds(40, 120, nlos))
        self.assertFalse(payload_bounds(40, 119, nlos))
        self.assertFalse(payload_bounds(40, 120, [(50, 2), (40, 1)]))
        self.assertTrue(payload_bounds(40, 120, [(0, 0), *nlos, (0, 0)]))

    def test_nonlinear_zero_count_gro_needs_explicit_guard(self):
        self.assertTrue(nonlinear_gro_bounds(40, 50, 50, 0, require_nonzero=False))
        self.assertFalse(nonlinear_gro_bounds(40, 50, 50, 0, require_nonzero=True))
        self.assertTrue(nonlinear_gro_bounds(40, 50, 50, 1, require_nonzero=True))

    def test_cumulative_byte_arithmetic_fits_u32(self):
        maximum = source_limit("RMNET_MAP_V5_MAX_NLOS") * 65535 * 255 + 131070
        self.assertLess(maximum, 2**32)

    def test_exact_checksum_workaround_predicate_on_host(self):
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("host C compiler unavailable; exact predicate NOT TESTED")
        header = (REPO / CORE / "rmnet_map.h").read_text()
        source = (REPO / CORE / "rmnet_map_data.c").read_text()
        parts = ["#include <stdbool.h>\n#include <stdint.h>\n",
                 "typedef uint8_t u8; typedef uint16_t __be16;\n",
                 "#define __aligned(n) __attribute__((aligned(n)))\n",
                 f"#define RMNET_MAP_V5_MAX_NLOS {source_limit('RMNET_MAP_V5_MAX_NLOS')}\n"]
        parts += [declaration(header, "enum", name) for name in
                  ("rmnet_map_v5_close_type", "rmnet_map_v5_close_value")]
        parts += [declaration(header, "struct", name) for name in
                  ("rmnet_map_header", "rmnet_map_v5_csum_header",
                   "rmnet_map_v5_nl_pair", "rmnet_map_v5_coal_header")]
        parts.append(checksum_workaround_function(source))
        parts.append(r"""
_Static_assert(sizeof(struct rmnet_map_header) == 4, "MAP size");
_Static_assert(sizeof(struct rmnet_map_v5_csum_header) == 4, "csum size");
_Static_assert(sizeof(struct rmnet_map_v5_nl_pair) == 4, "pair size");
_Static_assert(sizeof(struct rmnet_map_v5_coal_header) == 28, "coal size");
int main(void) {
    struct rmnet_map_v5_coal_header h = {0};
    for (unsigned int n = 0; n < 8; ++n)
        for (unsigned int p = 0; p < 256; ++p)
            for (unsigned int t = 0; t < 16; ++t)
                for (unsigned int v = 0; v < 16; ++v) {
                    h.num_nlos = n; h.nl_pairs[0].num_packets = p;
                    h.close_type = t; h.close_value = v;
                    bool expected = n == 1 && p == 1 &&
                        (t == RMNET_MAP_COAL_CLOSE_COAL ||
                         (t == RMNET_MAP_COAL_CLOSE_HW &&
                          (v == RMNET_MAP_COAL_CLOSE_HW_PKT ||
                           v == RMNET_MAP_COAL_CLOSE_HW_BYTE ||
                           v == RMNET_MAP_COAL_CLOSE_HW_TIME)));
                    if (rmnet_map_v5_csum_buggy(&h) != expected) return 1;
                }
    return 0;
}
""")
        with tempfile.TemporaryDirectory(prefix="rmnet-host-predicate-") as directory:
            binary = str(Path(directory) / "predicate")
            result = subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Wextra",
                                     "-Werror", "-x", "c", "-o", binary, "-"],
                                    input="\n".join(parts), text=True, capture_output=True,
                                    timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([binary], capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, "exact predicate mismatch")


if __name__ == "__main__":
    if not __debug__:
        raise SystemExit("ERROR: do not disable static contract assertions with Python -O")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=REPO)
    args, remaining = parser.parse_known_args()
    REPO = args.repo.resolve()
    print("HOST MODELS ONLY: not kernel, SKB, module, ABI, race or device validation", flush=True)
    for name in ("rmnet_map.h", "rmnet_map_data.c", "rmnet_descriptor.c", "rmnet_handlers.c"):
        path = REPO / CORE / name
        print(f"SOURCE_SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()} {CORE / name}", flush=True)
    unittest.main(argv=[sys.argv[0], *remaining])
