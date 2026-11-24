#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check that intervals never cross observed state or clock discontinuities."""

import runpy
from pathlib import Path
import unittest

intervals = runpy.run_path(Path(__file__).with_name("coloros17-runtime-history.py"))["intervals"]


class HistoryTests(unittest.TestCase):
    def test_filter_private_tags_and_measure_gauge(self):
        rows = list(intervals("""
11-24 00:00:00.000 RESET:TIME: 2026-11-24-00-00-00
11-24 00:00:01.000 033 plug=none -screen charge=1900 volt=3700
11-24 00:30:00.000 032 event="private +screen plug=ac"
11-24 01:00:01.000 032 charge=1880
""".splitlines()))
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["seconds"], 3600)
        self.assertEqual(rows[0]["screen"], "off")
        self.assertEqual(rows[0]["gauge_discharge_mAh"], 20)
        self.assertEqual(rows[0]["gauge_estimate_mWh"], 74)
        self.assertNotIn("private", str(rows))

    def test_no_bridge_between_display_plug_clock_or_gauge_changes(self):
        for event in ("042 +screen", "042 plug=usb", "042 +plugged",
                      "TIME: 2026-11-24-00:15:00", "042 charge=2600", "START"):
            with self.subTest(event=event):
                rows = list(intervals(f"""
11-24 00:00:00.000 RESET:TIME: 2026-11-24-00:00:00
11-24 00:00:01.000 043 -screen plug=none charge=2500
11-24 00:15:00.000 {event}
11-24 00:30:01.000 042 charge=2490
""".splitlines()))
                self.assertFalse(any(r["start"].endswith("00:00:01") for r in rows))

    def test_needs_known_state_and_timestamp_anchor(self):
        for start in ("11-24 00:00:01.000 043 -screen charge=2500",
                      "11-24 00:00:01.000 043 plug=none charge=2500"):
            self.assertFalse(list(intervals([
                "11-24 00:00:00.000 RESET:TIME: 2026-11-24-00:00:00",
                start, "11-24 00:30:01.000 042 charge=2490",
            ])))


if __name__ == "__main__":
    unittest.main()
