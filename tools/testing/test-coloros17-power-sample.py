#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
import copy
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("power_sample",
                                               Path(__file__).with_name("coloros17-power-sample.py"))
sample = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sample)


def endpoint(uptime, level, counter, usb="false"):
    return {
        "firmware": "CPH2747_17.0.1.201(EX01)",
        "slot": "_a", "release": "test", "boot_sha256": "a" * 64,
        "boot_id": "fixture", "uptime_s": uptime,
        "sections": {
            "battery": f"AC powered: false\nUSB powered: {usb}\nWireless powered: false\n"
                       f"Dock powered: false\nlevel: {level}\nCharge counter: {counter}\nvoltage: 3900",
            "display": "mWakefulness=Asleep",
            "memory": f"workingset_nodereclaim {round(uptime)}\n",
            "suspend": f"[/sys/power/suspend_stats/success]\n{round(uptime)}",
        },
    }


class Intervals(unittest.TestCase):
    def setUp(self):
        self.before = endpoint(300, 80, 4800000)
        self.after = endpoint(3900, 79, 4740000)

    def test_unplugged_gauge_and_counters(self):
        result = sample.interval(self.before, self.after)
        self.assertEqual(result["battery"]["gauge_delta_mAh"], 60)
        self.assertEqual(result["battery"]["endpoint_estimate_mWh"], 234)
        self.assertEqual(result["battery"]["drain_pct_points_per_hour"], 1)
        self.assertEqual(result["memory_delta"]["workingset_nodereclaim"], 3600)
        self.assertEqual(result["suspend_delta"]["success"], 3600)

    def test_no_energy_if_plugged_or_gauge_increases(self):
        for end in (endpoint(3900, 79, 4740000, "true"),
                    endpoint(3900, 79, 4900000),
                    endpoint(3900, 79, -1)):
            with self.subTest(end=end):
                self.assertIsInstance(sample.interval(self.before, end)["battery"], str)

    def test_missing_values_are_not_zero(self):
        end = copy.deepcopy(self.after)
        end["sections"]["memory"] = end["sections"]["battery"] = ""
        result = sample.interval(self.before, end)
        self.assertIsInstance(result["battery"], str)
        self.assertEqual(result["memory_delta"], {})

    def test_reject_identity_changes_and_reversed_time(self):
        for key in ("slot", "firmware", "boot_sha256", "boot_id", "release"):
            end = {**self.after, key: "other"}
            with self.subTest(key=key), self.assertRaises(ValueError):
                sample.interval(self.before, end)
        with self.assertRaises(ValueError):
            sample.interval(self.after, self.before)


if __name__ == "__main__":
    unittest.main()
