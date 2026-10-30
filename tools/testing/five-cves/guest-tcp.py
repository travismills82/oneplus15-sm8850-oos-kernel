#!/usr/bin/env python3
"""Bounded guest-only TCP regression; not dynamic BPF-ops retirement coverage."""

import argparse
import ctypes
import errno
import json
import os
import platform
import queue
import socket
import threading
import time


TCP_CONGESTION = getattr(socket, "TCP_CONGESTION", 13)
TCP_CC_INFO = 26
TCP_CA_NAME_MAX = 16
PATTERN = bytes(range(256))


def guest_guard():
    if os.geteuid() != 0:
        raise RuntimeError("requires root inside the disposable VM")
    with open("/proc/cmdline", encoding="ascii") as stream:
        tokens = stream.read().split()
    if "five_cves_vm=1" not in tokens:
        raise RuntimeError("refusing host/device execution: missing five_cves_vm=1")
    release = platform.release()
    if release != "6.12.114" and not release.startswith("6.12.114-"):
        raise RuntimeError("requires the patched Linux 6.12.114 guest")
    with open("/proc/sys/net/ipv4/tcp_available_congestion_control", encoding="ascii") as stream:
        available = set(stream.read().split())
    if not {"reno", "cubic"}.issubset(available):
        raise RuntimeError("guest must provide both reno and cubic")


def raw_getsockopt_checks(sock):
    libc = ctypes.CDLL(None, use_errno=True)
    getsockopt = libc.getsockopt
    getsockopt.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int,
                          ctypes.c_void_p, ctypes.c_void_p]
    getsockopt.restype = ctypes.c_int
    cases = 0
    for option in (TCP_CONGESTION, TCP_CC_INFO):
        for length in (0, 1, 4, 16, 256):
            buffer = ctypes.create_string_buffer(b"\xa5" * 272, 272)
            size = ctypes.c_uint(length)
            result = getsockopt(sock.fileno(), socket.IPPROTO_TCP, option,
                                buffer, ctypes.byref(size))
            if result != 0:
                raise OSError(ctypes.get_errno(), "length-case getsockopt failed")
            if size.value > length:
                raise AssertionError("getsockopt returned a length beyond the supplied buffer")
            if option == TCP_CONGESTION and size.value != min(length, TCP_CA_NAME_MAX):
                raise AssertionError("unexpected TCP_CONGESTION result length")
            if buffer.raw[length:] != b"\xa5" * (272 - length):
                raise AssertionError("getsockopt wrote beyond the requested buffer")
            cases += 1

    # Invalid userspace pointers exercise normal EFAULT handling, not kernel memory.
    # TCP_CC_INFO on Reno/Cubic may return no data, so use an invalid optlen there.
    for option in (TCP_CONGESTION, TCP_CC_INFO):
        buffer = ctypes.create_string_buffer(256)
        ctypes.set_errno(0)
        result = getsockopt(sock.fileno(), socket.IPPROTO_TCP, option,
                            buffer, ctypes.c_void_p(1))
        if result != -1 or ctypes.get_errno() != errno.EFAULT:
            raise AssertionError("invalid optlen did not return EFAULT")
    size = ctypes.c_uint(TCP_CA_NAME_MAX)
    ctypes.set_errno(0)
    result = getsockopt(sock.fileno(), socket.IPPROTO_TCP, TCP_CONGESTION,
                        ctypes.c_void_p(1), ctypes.byref(size))
    if result != -1 or ctypes.get_errno() != errno.EFAULT:
        raise AssertionError("invalid congestion-name buffer did not return EFAULT")
    return cases, 3


def exercise(family, seconds):
    address = "127.0.0.1" if family == socket.AF_INET else "::1"
    stats = {"family": "IPv4" if family == socket.AF_INET else "IPv6",
             "cc_switches": 0, "cc_gets": 0, "cc_info_gets": 0,
             "bytes_sent": 0, "bytes_received": 0}
    errors = queue.Queue()
    stop = threading.Event()
    barrier = threading.Barrier(5)
    sockets = []
    threads = []
    try:
        listener = socket.socket(family, socket.SOCK_STREAM)
        sockets.append(listener)
        listener.settimeout(2)
        listener.bind((address, 0))
        listener.listen(1)
        client = socket.socket(family, socket.SOCK_STREAM)
        sockets.append(client)
        client.settimeout(2)
        client.connect(listener.getsockname())
        server, _ = listener.accept()
        sockets.append(server)
        server.settimeout(2)
        stats["raw_length_cases"], stats["bad_pointer_cases"] = raw_getsockopt_checks(client)
        deadline = time.monotonic() + seconds

        def setter():
            index = 0
            while not stop.is_set() and time.monotonic() < deadline:
                client.setsockopt(socket.IPPROTO_TCP, TCP_CONGESTION,
                                  (b"reno", b"cubic")[index & 1])
                index += 1
            stats["cc_switches"] = index

        def getter():
            count = 0
            while not stop.is_set() and time.monotonic() < deadline:
                length = (1, 4, 16)[count % 3]
                name = client.getsockopt(socket.IPPROTO_TCP, TCP_CONGESTION, length)
                visible = name.split(b"\0", 1)[0]
                if len(name) != length or not visible or not any(expected.startswith(visible)
                                                                 for expected in (b"reno", b"cubic")):
                    raise AssertionError("unexpected congestion-control name")
                info = client.getsockopt(socket.IPPROTO_TCP, TCP_CC_INFO, 256)
                if len(info) > 256:
                    raise AssertionError("TCP_CC_INFO exceeded buffer")
                count += 1
            stats["cc_gets"] = count
            stats["cc_info_gets"] = count

        def sender():
            total = 0
            payload = PATTERN * 64
            try:
                while (not stop.is_set() and time.monotonic() < deadline
                       and total < 32 * 1024 * 1024):
                    client.sendall(payload)
                    total += len(payload)
                    time.sleep(0.0005)
            finally:
                stats["bytes_sent"] = total
                client.shutdown(socket.SHUT_WR)

        def receiver():
            total = 0
            while True:
                data = server.recv(65536)
                if not data:
                    break
                start = total % len(PATTERN)
                expected = (PATTERN * ((start + len(data) + 255) // 256))[start:start + len(data)]
                if data != expected:
                    raise AssertionError("loopback payload corruption")
                total += len(data)
            stats["bytes_received"] = total

        def guarded_worker(function):
            try:
                barrier.wait(timeout=3)
                function()
            except Exception as exc:  # Preserve worker failures for the controlling thread.
                errors.put(function.__name__ + ": " + repr(exc))
                stop.set()

        for function in (setter, getter, sender, receiver):
            thread = threading.Thread(target=guarded_worker, args=(function,),
                                      name=function.__name__, daemon=True)
            threads.append(thread)
            thread.start()
        barrier.wait(timeout=3)
        stop.wait(seconds)
        stop.set()
        for thread in threads:
            thread.join(timeout=3)
        if any(thread.is_alive() for thread in threads):
            raise RuntimeError("bounded TCP worker did not exit")
        if not errors.empty():
            messages = []
            while not errors.empty():
                messages.append(errors.get_nowait())
            raise RuntimeError("; ".join(messages))
        for field in ("cc_switches", "cc_gets", "cc_info_gets", "bytes_sent"):
            if stats[field] <= 0:
                raise AssertionError("no coverage for " + field)
        if stats["bytes_received"] != stats["bytes_sent"]:
            raise AssertionError("payload byte counts differ")
        return stats
    finally:
        stop.set()
        for sock in reversed(sockets):
            sock.close()
        for thread in threads:
            thread.join(timeout=1)


def main():
    guest_guard()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=float, default=2.0)
    args = parser.parse_args()
    if not 0.25 <= args.seconds <= 10:
        raise ValueError("--seconds must be between 0.25 and 10")
    results = [exercise(family, args.seconds) for family in (socket.AF_INET, socket.AF_INET6)]
    print(json.dumps({"test": "tcp_getsockopt_concurrency", "status": "PASS",
                      "kernel": platform.release(), "results": results,
                      "coverage_limit": "static Reno/Cubic; no dynamic BPF struct_ops retirement"},
                     sort_keys=True), flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(json.dumps({"test": "tcp_getsockopt_concurrency", "status": "FAIL",
                          "error": repr(error)}, sort_keys=True), flush=True)
        raise SystemExit(1)
