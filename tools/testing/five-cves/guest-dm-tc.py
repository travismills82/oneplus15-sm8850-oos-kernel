#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded DM/TC regressions; run only inside the guarded disposable VM.

This is not a deterministic reproducer for either CVE. Never run it on the
physical phone or host. The guest must have no attached host disks or NICs.
"""

import argparse
import collections
import concurrent.futures
import ctypes
import errno
import hashlib
import json
import mmap
import os
from pathlib import Path
import platform
import re
import shutil
import socket
import struct
import subprocess
import tempfile
import threading
import time


class TestFailure(RuntimeError):
    pass


def guard():
    if os.geteuid() != 0:
        raise TestFailure("requires guest root")
    if "five_cves_vm=1" not in Path("/proc/cmdline").read_text().split():
        raise TestFailure("refusing to run without exact guest command-line marker")
    if platform.release() != "6.12.114" and not platform.release().startswith("6.12.114-"):
        raise TestFailure("unexpected guest kernel release")
    if platform.machine() != "x86_64":
        raise TestFailure("this harness is restricted to the disposable x86_64 VM")
    for tool in ("dmsetup", "losetup", "ip", "tc"):
        if not shutil.which(tool):
            raise TestFailure(f"missing guest tool: {tool}")


def command(argv, expected_errors=(), timeout=20):
    environment = os.environ.copy()
    environment["LC_ALL"] = "C"
    try:
        proc = subprocess.run(argv, text=True, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, timeout=timeout, check=False,
                              env=environment)
    except subprocess.TimeoutExpired as exc:
        raise TestFailure(f"command timed out: {argv}") from exc
    if proc.returncode == 0:
        return proc
    output = (proc.stdout + "\n" + proc.stderr).lower()
    if proc.returncode > 0 and expected_errors and any(
            message.lower() in output for message in expected_errors):
        return proc
    raise TestFailure(f"command failed ({proc.returncode}): {argv}\n{output}")


def log(stage, **values):
    print(json.dumps({"stage": stage, **values}, sort_keys=True), flush=True)


def require_tmpfs(path):
    """Refuse an accidental persistent backing file even inside the guest."""
    target = str(Path(path).resolve())
    choices = []
    for line in Path("/proc/self/mountinfo").read_text().splitlines():
        left, right = line.split(" - ", 1)
        mountpoint = left.split()[4]
        mountpoint = re.sub(r"\\([0-7]{3})", lambda m: chr(int(m[1], 8)), mountpoint)
        if target == mountpoint or target.startswith(mountpoint.rstrip("/") + "/"):
            choices.append((len(mountpoint), right.split()[0]))
    if not choices or max(choices)[1] != "tmpfs":
        raise TestFailure("DM backing directory must reside on guest tmpfs")


def dm(arguments):
    return command(["dmsetup", "--noudevsync", *arguments])


def dm_test():
    require_tmpfs("/tmp")
    directory = tempfile.mkdtemp(prefix="five-cves-dm-", dir="/tmp")
    backing = Path(directory) / "backing.img"
    lower_name = f"five-cves-lower-{os.getpid()}"
    crypt_name = f"five-cves-crypt-{os.getpid()}"
    lower_device = f"/dev/mapper/{lower_name}"
    crypt_device = f"/dev/mapper/{crypt_name}"
    sectors = 65536
    loop = None
    created = []
    fd = None
    buffer = None
    cleanup_errors = []
    original_failure = None
    reads = writes = failures = 0
    try:
        with backing.open("xb") as image:
            image.truncate(sectors * 512)
        loop = command(["losetup", "--find", "--show", str(backing)]).stdout.strip()
        if not re.fullmatch(r"/dev/loop[0-9]+", loop):
            raise TestFailure(f"unexpected loop device: {loop!r}")
        linear = f"0 {sectors} linear {loop} 0"
        dm(["create", lower_name, "--table", linear])
        created.append(lower_name)
        dm(["mknodes", lower_name])
        # Public test-only key, not an encryption key copied from any device.
        key = bytes(range(64)).hex()
        table = f"0 {sectors} crypt aes-xts-plain64 {key} 0 {lower_device} 0"
        dm(["create", crypt_name, "--table", table])
        created.append(crypt_name)
        dm(["mknodes", crypt_name])
        fd = os.open(crypt_device, os.O_RDWR | os.O_DIRECT | os.O_CLOEXEC)
        # Anonymous mmap supplies page-aligned storage for O_DIRECT preadv/pwritev.
        buffer = mmap.mmap(-1, 4096)
        patterns = [hashlib.sha256(f"five-cves-{i}".encode()).digest() * 128
                    for i in range(32)]
        for i, pattern in enumerate(patterns):
            buffer[:] = pattern
            if os.pwritev(fd, [buffer], i * 4096) != 4096:
                raise TestFailure("short direct encrypted write")
            writes += 1
        os.fsync(fd)
        for i, pattern in enumerate(patterns):
            buffer[:] = bytes(4096)
            if os.preadv(fd, [buffer], i * 4096) != 4096 or buffer[:] != pattern:
                raise TestFailure("encrypted direct-read roundtrip mismatch")
            reads += 1
        with backing.open("rb", buffering=0) as image:
            if image.read(4096) == patterns[0]:
                raise TestFailure("underlying backing unexpectedly contains plaintext")
        log("dm-normal-roundtrip", direct_reads=reads, direct_writes=writes)

        dm(["suspend", lower_name])
        dm(["reload", lower_name, "--table", f"0 {sectors} error"])
        dm(["resume", lower_name])
        for i in range(64):
            for operation in ("read", "write"):
                buffer[:] = patterns[i % len(patterns)]
                try:
                    if operation == "read":
                        os.preadv(fd, [buffer], (i % 32) * 4096)
                    else:
                        os.pwritev(fd, [buffer], (i % 32) * 4096)
                except OSError as exc:
                    if exc.errno != errno.EIO:
                        raise TestFailure(f"unexpected {operation} errno: {exc.errno}") from exc
                    failures += 1
                else:
                    raise TestFailure(f"failed backing {operation} reported success")
        if failures != 128:
            raise TestFailure("incorrect failed-I/O accounting")
        log("dm-injected-error", direct_EIO=failures)

        dm(["suspend", lower_name])
        dm(["reload", lower_name, "--table", linear])
        dm(["resume", lower_name])
        for i, pattern in enumerate(patterns):
            if os.preadv(fd, [buffer], i * 4096) != 4096 or buffer[:] != pattern:
                raise TestFailure("data changed after failed writes / backing recovery")
        log("dm-restored-roundtrip", direct_reads=len(patterns), result="PASS")
    except BaseException as exc:
        original_failure = exc
    finally:
        if fd is not None:
            os.close(fd)
        if buffer is not None:
            buffer.close()
        for name in reversed(created):
            try:
                dm(["remove", name])
            except Exception as exc:
                cleanup_errors.append(str(exc))
        if loop:
            try:
                command(["losetup", "--detach", loop])
            except Exception as exc:
                cleanup_errors.append(str(exc))
        shutil.rmtree(directory)
    if cleanup_errors:
        raise TestFailure(f"DM cleanup failed: {cleanup_errors}; prior={original_failure}")
    if original_failure:
        raise original_failure


def private_netns():
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.unshare(0x40000000) != 0:  # CLONE_NEWNET
        error = ctypes.get_errno()
        raise TestFailure(f"unshare(CLONE_NEWNET): {os.strerror(error)}")
    command(["ip", "link", "set", "lo", "up"])


def action_delete_request(index, portid, sequence=1):
    """Encode this kernel's RTM_DELACTION/tcamsg action-index request.

    UAPI: rtnetlink.h RTM_DELACTION=49, tcamsg={u8,u8,u16},
    TCA_ROOT_TAB=1; pkt_cls.h TCA_ACT_KIND=1 and TCA_ACT_INDEX=3.
    act_api.c:tca_action_gd() parses TAB -> priority1 -> KIND/INDEX.
    NLM_F_ROOT is deliberately absent: this deletes exactly one index.
    """
    if not 0 < index <= 0xFFFFFFFF:
        raise TestFailure("invalid action deletion index")

    def attribute(kind, payload):
        length = 4 + len(payload)
        return struct.pack("=HH", length, kind) + payload + bytes((-length) & 3)

    action = attribute(1, b"gact\0") + attribute(3, struct.pack("=I", index))
    nested = 0x8000  # NLA_F_NESTED
    payload = struct.pack("=BBH", socket.AF_UNSPEC, 0, 0)
    payload += attribute(1 | nested, attribute(1 | nested, action))
    header = struct.pack("=IHHII", 16 + len(payload), 49, 0x01 | 0x04,
                         sequence, portid)  # REQUEST | ACK
    return header + payload


def action_delete_ack(packet, request):
    """Validate a complete single NLMSG_ERROR ACK and return its actual errno."""
    if len(packet) < 36:
        raise TestFailure("short TC deletion netlink acknowledgement")
    length, kind, flags, sequence, portid = struct.unpack_from("=IHHII", packet)
    _, _, _, requested_sequence, requested_portid = struct.unpack_from("=IHHII", request)
    if length < 36 or length > len(packet) or ((length + 3) & ~3) != len(packet):
        raise TestFailure("malformed TC deletion netlink acknowledgement length")
    if kind != 2 or sequence != requested_sequence or portid != requested_portid:
        raise TestFailure("unexpected TC deletion netlink acknowledgement identity")
    if packet[20:36] != request[:16]:
        raise TestFailure("TC deletion acknowledgement describes another request")
    result = struct.unpack_from("=i", packet, 16)[0]
    if result > 0:
        raise TestFailure("positive errno in TC deletion acknowledgement")
    return -result


def delete_gact_netlink(index=101, timeout=2):
    # One private socket/request per operation: no shared sequence or reply
    # ownership across workers. Never infer errno from iproute2's generic text.
    with socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, socket.NETLINK_ROUTE) as route:
        route.bind((0, 0))
        route.settimeout(timeout)
        request = action_delete_request(index, route.getsockname()[0])
        if route.sendto(request, (0, 0)) != len(request):
            raise TestFailure("short TC deletion netlink request")
        try:
            packet, ancillary, flags, sender = route.recvmsg(65536)
        except TimeoutError as exc:
            raise TestFailure("TC deletion netlink acknowledgement timed out") from exc
        if flags & socket.MSG_TRUNC or sender[0] != 0:
            raise TestFailure("truncated/non-kernel TC deletion acknowledgement")
        return action_delete_ack(packet, request)


def tc_test(iterations):
    private_netns()
    command(["ip", "link", "add", "cvetc0", "type", "veth", "peer", "name", "cvetc1"])
    stop = threading.Event()
    counts = collections.Counter()
    count_lock = threading.Lock()

    def tc(arguments, expected=(), timeout=20):
        result = command(["tc", *arguments], expected, timeout=timeout)
        with count_lock:
            counts["success" if result.returncode == 0 else "expected_race_errors"] += 1
        return result

    def add_filter(pref):
        return tc(["filter", "add", "dev", "cvetc0", "ingress", "protocol", "all",
                   "pref", str(pref), "matchall", "action", "gact", "drop", "index", "101"],
                  ("resource temporarily unavailable",))

    def del_filter(pref):
        return tc(["filter", "del", "dev", "cvetc0", "ingress", "pref", str(pref)])

    def delete_action(allowed=(), timeout=2):
        result = delete_gact_netlink(timeout=timeout)
        if result and result not in allowed:
            raise TestFailure(f"unexpected RTM_DELACTION errno {result}: {os.strerror(result)}")
        with count_lock:
            counts["success" if result == 0 else "expected_race_errors"] += 1
            counts[f"delete_errno_{result}"] += 1
        return result

    def binding_count(expected):
        # cls_matchall schedules destruction with tcf_queue_work(), which
        # queues RCU work. Netlink deletion can return before tcf_exts_destroy()
        # drops the action binding. Require the exact count after a bounded
        # grace/worker interval; missing actions and command errors still fail.
        deadline = time.monotonic() + 5
        output = "<no action sample completed>"
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TestFailure(f"binding count did not reach {expected} within 5s: {output}")
            output = tc(["-s", "actions", "get", "action", "gact", "index", "101"],
                        timeout=min(1, remaining)).stdout
            match = re.search(r"\bbind\s+([0-9]+)\b", output)
            if not match:
                raise TestFailure(f"missing action binding count: {output}")
            actual = int(match.group(1))
            if actual == expected:
                return
            if actual < expected:
                raise TestFailure(f"action binding count unexpectedly below {expected}: {output}")
            time.sleep(min(0.02, max(0, deadline - time.monotonic())))

    try:
        command(["ip", "link", "set", "cvetc0", "up"])
        command(["ip", "link", "set", "cvetc1", "up"])
        tc(["qdisc", "add", "dev", "cvetc0", "clsact"])
        tc(["actions", "add", "action", "gact", "drop", "index", "101"])
        for pref in (10, 11):
            result = add_filter(pref)
            if result.returncode:
                raise TestFailure("preflight filter setup unexpectedly raced")
        binding_count(2)
        if delete_action((errno.EPERM,)) != errno.EPERM:
            raise TestFailure("bound TC action deletion did not return EPERM")
        del_filter(10)
        binding_count(1)
        del_filter(11)
        binding_count(0)
        delete_action()
        if delete_action((errno.ENOENT,)) != errno.ENOENT:
            raise TestFailure("absent TC action deletion did not return ENOENT")
        log("tc-binding-lifecycle", result="PASS")

        def bind_worker(pref):
            for _ in range(iterations):
                if stop.is_set():
                    return
                if add_filter(pref).returncode == 0:
                    tc(["filter", "show", "dev", "cvetc0", "ingress", "pref", str(pref)])
                    del_filter(pref)

        def delete_worker():
            for _ in range(iterations * 2):
                if stop.is_set():
                    return
                tc(["actions", "add", "action", "gact", "drop", "index", "101"],
                   ("file exists", "resource temporarily unavailable"))
                # Validate real kernel ACK errno, not iproute2's generic
                # "Failed to delete TC action" diagnostic. Bound/absent races
                # are expected; all other errors remain fatal.
                delete_action((errno.EPERM, errno.ENOENT))

        def dump_worker():
            for _ in range(iterations * 2):
                if stop.is_set():
                    return
                result = tc(["-j", "-s", "actions", "list", "action", "gact"])
                try:
                    parsed = json.loads(result.stdout)
                except ValueError as exc:
                    raise TestFailure(f"invalid TC JSON dump: {result.stdout}") from exc
                if not isinstance(parsed, list):
                    raise TestFailure("unexpected TC action dump format")

        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
            futures = [pool.submit(bind_worker, pref) for pref in (100, 101, 102)]
            futures += [pool.submit(delete_worker), pool.submit(dump_worker)]
            try:
                for future in concurrent.futures.as_completed(futures, timeout=180):
                    future.result()
            except BaseException:
                stop.set()
                for future in futures:
                    future.cancel()
                raise
        if tc(["filter", "show", "dev", "cvetc0", "ingress"]).stdout.strip():
            raise TestFailure("TC filters remain after worker completion")
        # Matchall destruction is RCU-deferred. Wait only for EPERM (bindings
        # still draining), with a hard deadline; do not hide persistent leaks.
        deadline = time.monotonic() + 5
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TestFailure("TC action bindings did not drain within 5s")
            result = delete_action((errno.EPERM, errno.ENOENT), timeout=min(1, remaining))
            if result in (0, errno.ENOENT):
                break
            time.sleep(min(0.02, max(0, deadline - time.monotonic())))
        if delete_action((errno.ENOENT,)) != errno.ENOENT:
            raise TestFailure("TC action survived final deletion")
        if not counts["success"]:
            raise TestFailure("no TC operations completed")
        log("tc-concurrent-action-lifetime", iterations_per_binder=iterations,
            bind_workers=3, **counts, result="PASS")
    finally:
        command(["ip", "link", "delete", "cvetc0"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iterations", type=int, default=40)
    options = parser.parse_args()
    if not 1 <= options.iterations <= 300:
        parser.error("iterations must be between 1 and 300")
    try:
        guard()
        dm_test()
        tc_test(options.iterations)
    except Exception as exc:
        log("five-cves-dm-tc", result="FAIL", error=str(exc))
        return 1
    log("five-cves-dm-tc", result="PASS", scope="functional and bounded stress, not race reproduction")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
