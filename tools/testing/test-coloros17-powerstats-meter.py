#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
import runpy
from pathlib import Path
import unittest

window = runpy.run_path(Path(__file__).with_name("coloros17-powerstats-meter.py"))["meter_window"]


def varint(x):
    return (bytes([x]) if x < 128
            else bytes([(x & 127) | 128]) + varint(x >> 7))


def blob(tag, b):
    return bytes([(tag << 3) | 2]) + varint(len(b)) + b


def sample(t, q):
    return blob(2, b"\x10" + varint(t) + b"\x18" + varint(q))


class MeterTests(unittest.TestCase):
    header = blob(1, blob(2, b"gpu"))

    def test_rail_units_and_boundaries(self):
        source = self.header + sample(1, 0) + sample(2000, 100) + sample(6000, 3_600_100)
        rows = window(source, 1000, 7000)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["delta_mWh"], 1)
        self.assertEqual(rows[0]["average_mW"], 900)
        self.assertEqual(rows[0]["samples"], 2)

    def test_reset_and_conflicting_sample_are_rejected(self):
        for series in ((sample(3000, 99)), (sample(2000, 101))):
            self.assertFalse(window(self.header + sample(2000, 100) + series, 1000, 4000))

    def test_reject_truncation(self):
        with self.assertRaises(ValueError):
            window(self.header + b"\x12\x05\x10", 1, 100)


if __name__ == "__main__":
    unittest.main()
