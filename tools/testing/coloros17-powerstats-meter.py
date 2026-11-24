#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Analyze saved PowerStatsServiceMeterProto without querying or changing a device."""

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import json
from pathlib import Path


# AOSP frameworks/base/core/proto/android/server/powerstatsservice.proto.
def varint(data, offset):
    value = 0
    for shift in range(0, 64, 7):
        if offset >= len(data):
            raise ValueError("truncated protobuf varint")
        octet = data[offset]
        offset += 1
        value |= (octet & 127) << shift
        if octet < 128 and value < 1 << 64:
            return value, offset
    raise ValueError("invalid protobuf varint")


def fields(data):
    offset = 0
    while offset < len(data):
        key, offset = varint(data, offset)
        number, wire = key >> 3, key & 7
        if number == 0:
            raise ValueError("invalid protobuf field")
        if wire == 0:
            value, offset = varint(data, offset)
        elif wire in (1, 2, 5):
            if wire == 2:
                size, offset = varint(data, offset)
            else:
                size = 8 if wire == 1 else 4
            if size > len(data) - offset:
                raise ValueError("truncated protobuf field")
            value = data[offset:offset + size]
            offset += size
        else:
            raise ValueError("unsupported protobuf wire type")
        yield number, value


def meter_window(data, begin_ms, end_ms):
    if not 0 < begin_ms < end_ms:
        raise ValueError("non-positive interval")
    channels, samples = {}, defaultdict(list)
    for field, record in fields(data):
        if field not in (1, 2):
            continue
        if not isinstance(record, bytes):
            raise ValueError("invalid meter record")
        decoded = dict(fields(record))
        index = decoded.get(1, 0)  # ProtoOutputStream may omit a scalar zero.
        if field == 1:
            channels[index] = decoded[2].decode()
        else:
            stamp, energy = decoded.get(2, 0), decoded.get(3, 0)
            if begin_ms <= stamp <= end_ms:
                samples[index].append((stamp, energy))

    result = []
    for index, name in sorted(channels.items()):
        series = sorted(set(samples[index]))
        if len(series) < 2:
            continue
        if any(t1 <= t0 or q1 < q0 or q1 >= 1 << 63
               for (t0, q0), (t1, q1) in zip(series, series[1:])):
            continue  # Ambiguous duplicate timestamps or counter reset.
        (start, start_energy), (end, end_energy) = series[0], series[-1]
        delta = end_energy - start_energy
        result.append({
            "channel": name,
            "first": datetime.fromtimestamp(start / 1000, timezone.utc).isoformat(),
            "last": datetime.fromtimestamp(end / 1000, timezone.utc).isoformat(),
            "samples": len(series),
            "delta_mWh": round(delta / 3_600_000, 3),
            "average_mW": round(delta / (end - start), 3),
        })
    return result


def epoch_ms(text):
    stamp = datetime.fromisoformat(text)
    if stamp.utcoffset() is None:
        raise ValueError("timestamps must specify a UTC offset")
    return int(stamp.timestamp() * 1000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("meter", type=Path)
    parser.add_argument("--start", required=True)
    parser.add_argument("--end", required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(meter_window(args.meter.read_bytes(), epoch_ms(args.start),
                                      epoch_ms(args.end)), indent=2))
    except (ValueError, TypeError, KeyError, OSError) as error:
        parser.exit(2, f"{error}\n")


if __name__ == "__main__":
    main()
