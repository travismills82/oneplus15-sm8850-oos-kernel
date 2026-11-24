#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Report observed unplugged charge-counter intervals from saved BatteryStats."""

import argparse
from datetime import datetime
import json
from pathlib import Path
import re


LINE = re.compile(r"^\s*(\d\d-\d\d \d\d:\d\d:\d\d\.\d{3})\s+(.*)$")
ANCHOR = re.compile(r"^(RESET:)?TIME: (\d{4}-\d\d-\d\d-\d\d[-:]\d\d[-:]\d\d)\b")
TOKEN = re.compile(r'''(?:[^\s"]|"(?:\\.|[^"\\])*")+''')
CSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def intervals(lines, minimum_seconds=60):
    """Yield same-state, non-interpolated intervals; never expose event tags."""
    year = previous = screen = plug = voltage = first = last = None

    def finish():
        if first is None or last is None:
            return None
        start, q0, l0, v0 = first
        end, q1, l1, v1 = last
        seconds = (end - start).total_seconds()
        if seconds < minimum_seconds or seconds <= 0:
            return None
        charge = q0 - q1
        return {
            "start": start.isoformat(),
            "end": end.isoformat(),
            "screen": screen,
            "seconds": seconds,
            "start_level": l0,
            "end_level": l1,
            "gauge_discharge_mAh": charge,
            "gauge_mAh_per_hour": round(charge * 3600 / seconds, 2),
            "gauge_estimate_mWh": (
                round(charge * (v0 + v1) / 2000, 2)
                if v0 is not None and v1 is not None else None
            ),
        }

    for line in lines:
        match = LINE.match(CSI.sub("", line))
        if not match:
            continue
        prefix, payload = match.groups()
        anchor = ANCHOR.match(payload)
        if anchor:
            # A TIME record may reflect a wall clock correction. Do not
            # extrapolate or compute an interval across it.
            if row := finish():
                yield row
            first = last = previous = None
            year = anchor[2][:4]
            if anchor[1]:
                screen = plug = voltage = None
            continue
        if payload.rstrip() in {"START", "SHUTDOWN", "OVERFLOW"}:
            if row := finish():
                yield row
            first = last = previous = screen = plug = voltage = year = None
            continue
        if year is None:
            continue
        stamp = datetime.strptime(f"{year}-{prefix}", "%Y-%m-%d %H:%M:%S.%f")
        if previous is not None and stamp < previous:
            if row := finish():
                yield row
            first = last = None
            if (previous - stamp).total_seconds() > 1:
                screen = plug = voltage = year = None
                continue
        previous = stamp
        tokens = TOKEN.findall(payload)
        if not tokens or not re.fullmatch(r"\d{3}", tokens[0]):
            continue
        level = int(tokens[0])
        if level > 100:
            continue
        new_screen, new_plug, charge = screen, plug, None
        for token in tokens[1:]:
            if token in {"+screen", "-screen"}:
                new_screen = "on" if token == "+screen" else "off"
            elif token == "+plugged":
                new_plug = "unknown-plugged"
            elif token == "-plugged":
                new_plug = "none"
            elif token.startswith("plug="):
                new_plug = token[5:]
            elif re.fullmatch(r"volt=\d+", token):
                mv = int(token[5:])
                voltage = mv if 2000 < mv < 5000 else None
            elif re.fullmatch(r"charge=\d+", token):
                charge = int(token[7:])
        if (screen, plug) != (new_screen, new_plug):
            if row := finish():
                yield row
            first = last = None
        screen, plug = new_screen, new_plug
        if charge == 0:
            if row := finish():
                yield row
            first = last = None
        if plug != "none" or screen is None or not charge:
            continue
        if last is not None and charge > last[1]:
            # A gauge recalibration is not negative energy consumption.
            if row := finish():
                yield row
            first = last = None
        point = stamp, charge, level, voltage
        if first is None:
            first = point
        last = point
    if row := finish():
        yield row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("history", type=Path)
    parser.add_argument("--min-seconds", type=int, default=60)
    args = parser.parse_args()
    if args.min_seconds <= 0:
        parser.error("--min-seconds must be positive")
    with args.history.open(encoding="utf-8", errors="replace") as history:
        print(json.dumps(list(intervals(history, args.min_seconds)), indent=2))


if __name__ == "__main__":
    main()
