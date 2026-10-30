#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded helper/expectation regression tests, ONLY in the disposable VM.

No conntrack/libmnl Python package is needed: the small netlink encoder uses
the kernel UAPI. These tests exercise real FTP ALG/NAT and concurrent master
deletion, expiration and RCU dumps, not an abstract userspace lifetime model.
They are regression/stress coverage, not a proof that every race is absent.
"""

import argparse
import contextlib
import errno
import hashlib
import json
import os
from pathlib import Path
import queue
import random
import re
import selectors
import socket
import socketserver
import struct
import subprocess
import sys
import threading
import time
import traceback

PORT = 2121
DATA = bytes(range(256)) * 2048
DIGEST = hashlib.sha256(DATA).hexdigest()
PREFIX = "cve80668_"
CLIENT, ROUTER, SERVER = (PREFIX + suffix for suffix in ("c", "r", "s"))


def require_vm():
    if os.geteuid() != 0 or "cve80668_vm=1" not in Path("/proc/cmdline").read_text():
        raise SystemExit("Refusing networking operations outside guarded root VM")
    if not os.uname().release.startswith("6.12.114"):
        raise SystemExit("Refusing test against a different kernel release")


def run(*args, check=True):
    result = subprocess.run(args, check=False, text=True, capture_output=True)
    if check and result.returncode:
        print(f"Command failed: {args!r}\nstdout: {result.stdout}\nstderr: {result.stderr}",
              file=sys.stderr, flush=True)
        result.check_returncode()
    return result


def nsrun(ns, *args, check=True):
    return run("ip", "netns", "exec", ns, *args, check=check)


@contextlib.contextmanager
def netns(ns):
    old = os.open("/proc/self/ns/net", os.O_RDONLY)
    new = os.open("/run/netns/" + ns, os.O_RDONLY)
    try:
        os.setns(new, 0)
        yield
    finally:
        os.setns(old, 0)
        os.close(new)
        os.close(old)


def attribute(kind, payload):
    length = len(payload) + 4
    return struct.pack("=HH", length, kind) + payload + b"\0" * (-length % 4)


def attributes(data):
    result = {}
    offset = 0
    while offset < len(data):
        if len(data) - offset < 4:
            raise ValueError("Truncated netlink attribute header")
        length, kind = struct.unpack_from("=HH", data, offset)
        if length < 4 or offset + length > len(data):
            raise ValueError("Invalid netlink attribute length")
        result[kind & 0x3FFF] = (kind, data[offset + 4 : offset + length])
        offset += (length + 3) & ~3
    return result


class Netfilter:
    def __init__(self, ns):
        with netns(ns):
            self.sock = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, 12)
            self.sock.bind((0, 0))
        self.sock.settimeout(5)
        self.sequence = 0

    def close(self):
        self.sock.close()

    def request(self, subsystem, command, payload=b"", dump=False, family=socket.AF_INET):
        self.sequence += 1
        flags = 1 | (0x300 if dump else 4)  # REQUEST, DUMP or ACK
        body = struct.pack("!BBH", family, 0, 0) + payload
        self.sock.send(struct.pack("=IHHII", 16 + len(body), subsystem << 8 | command,
                                   flags, self.sequence, 0) + body)
        results = []
        while True:
            data = self.sock.recv(1 << 20)
            offset = 0
            while offset < len(data):
                if len(data) - offset < 16:
                    raise ValueError("Truncated netlink message")
                length, kind, _flags, sequence, _pid = struct.unpack_from("=IHHII", data, offset)
                if length < 16 or offset + length > len(data):
                    raise ValueError("Invalid netlink message length")
                value = data[offset + 16 : offset + length]
                offset += (length + 3) & ~3
                if sequence != self.sequence:
                    continue
                if kind == 2:  # NLMSG_ERROR, including successful ACK
                    error = struct.unpack_from("=i", value)[0]
                    if error:
                        raise OSError(-error, os.strerror(-error))
                    return results
                if kind == 3:  # NLMSG_DONE
                    if len(value) >= 4 and struct.unpack_from("=i", value)[0]:
                        error = -struct.unpack_from("=i", value)[0]
                        raise OSError(error, os.strerror(error))
                    return results
                results.append(value)

    def expectations(self):
        return [(value[0], attributes(value[4:]))
                for value in self.request(2, 1, dump=True)]

    def expectation_stats(self):
        totals = dict.fromkeys(("new", "create", "delete"), 0)
        for value in self.request(2, 3, dump=True, family=socket.AF_UNSPEC):
            attrs = attributes(value[4:])
            for kind, name in enumerate(totals, 1):
                totals[name] += struct.unpack("!I", attrs[kind][1])[0]
        return totals

    def expire(self, item, seconds):
        family, attrs = item
        # Existing-expectation update still requires MASTER/TUPLE/MASK.
        payload = b"".join(attribute(*attrs[k]) for k in (1, 2, 3))
        payload += attribute(4, struct.pack("!I", seconds))
        self.request(2, 0, payload, family=family)

    def flush_masters(self):
        self.request(1, 2)

    @staticmethod
    def ready_master_tuple(source_port):
        # The producer queues this tuple only AFTER its client received the
        # server's 200 reply. Merely seeing an expectation is too early: the
        # helper publishes it before the PORT packet reaches the FTP server.
        # Deleting then can legitimately reset the not-yet-ready control.
        addresses = attribute(1, socket.inet_aton("10.66.1.2"))
        addresses += attribute(2, socket.inet_aton("10.66.2.2"))
        protocol = attribute(1, bytes([socket.IPPROTO_TCP]))
        protocol += attribute(2, struct.pack("!H", source_port))
        protocol += attribute(3, struct.pack("!H", PORT))
        original = attribute(0x8001, addresses) + attribute(0x8002, protocol)
        return attribute(0x8001, original)

    def delete_ready_master(self, source_port):
        self.request(1, 2, self.ready_master_tuple(source_port))

    def master_present(self, source_port):
        try:
            return bool(self.request(1, 1, self.ready_master_tuple(source_port)))
        except OSError as error:
            if error.errno == errno.ENOENT:
                return False
            raise


def setup():
    for ns in (CLIENT, ROUTER, SERVER):
        run("ip", "netns", "add", ns)
        run("ip", "-n", ns, "link", "set", "lo", "up")
    run("ip", "link", "add", "cve_client", "type", "veth", "peer", "name", "cve_lan")
    run("ip", "link", "set", "cve_client", "netns", CLIENT)
    run("ip", "link", "set", "cve_lan", "netns", ROUTER)
    run("ip", "link", "add", "cve_wan", "type", "veth", "peer", "name", "cve_server")
    run("ip", "link", "set", "cve_wan", "netns", ROUTER)
    run("ip", "link", "set", "cve_server", "netns", SERVER)
    for ns, dev, address in ((CLIENT, "cve_client", "10.66.1.2/24"),
                             (ROUTER, "cve_lan", "10.66.1.1/24"),
                             (ROUTER, "cve_wan", "10.66.2.1/24"),
                             (SERVER, "cve_server", "10.66.2.2/24")):
        run("ip", "-n", ns, "addr", "add", address, "dev", dev)
        run("ip", "-n", ns, "link", "set", dev, "up")
    run("ip", "-n", CLIENT, "route", "add", "default", "via", "10.66.1.1")
    run("ip", "-n", SERVER, "route", "add", "default", "via", "10.66.2.1")
    nsrun(ROUTER, "sysctl", "-w", "net.ipv4.ip_forward=1")
    nsrun(ROUTER, "iptables-legacy", "-t", "raw", "-A", "PREROUTING", "-i", "cve_lan",
          "-p", "tcp", "--dport", str(PORT), "-j", "CT", "--helper", "ftp")
    nsrun(ROUTER, "iptables-legacy", "-t", "nat", "-A", "POSTROUTING", "-o", "cve_wan",
          "-j", "MASQUERADE")
    nsrun(CLIENT, "ping", "-c", "2", "-W", "2", "10.66.2.2")


def cleanup():
    for ns in (CLIENT, SERVER, ROUTER):
        pids = run("ip", "netns", "pids", ns, check=False).stdout.split()
        for pid in pids:
            try:
                os.kill(int(pid), 15)
            except ProcessLookupError:
                pass
        run("ip", "netns", "del", ns, check=False)


class FTP(socketserver.StreamRequestHandler):
    def handle(self):
        active = None
        passive = None
        self.request.settimeout(15)
        self.wfile.write(b"220 VM regression FTP\r\n")
        self.wfile.flush()
        try:
            while line := self.rfile.readline(4096):
                text = line.decode("ascii").strip()
                command, _, arg = text.partition(" ")
                if command == "USER":
                    answer = b"331 Password required\r\n"
                elif command == "PASS":
                    answer = b"230 Logged in\r\n"
                elif command == "TYPE":
                    answer = b"200 Binary mode\r\n"
                elif command == "PORT":
                    parts = [int(x) for x in arg.split(",")]
                    active = (".".join(map(str, parts[:4])), parts[4] * 256 + parts[5])
                    if active[0] != "10.66.2.1":
                        raise AssertionError("FTP NAT did not rewrite the PORT address")
                    answer = b"200 PORT accepted\r\n"
                elif command == "PASV":
                    passive = socket.socket()
                    passive.settimeout(10)
                    passive.bind(("10.66.2.2", 0))
                    passive.listen(1)
                    port = passive.getsockname()[1]
                    answer = f"227 Entering Passive Mode (10,66,2,2,{port // 256},{port % 256})\r\n".encode()
                elif command == "RETR":
                    self.wfile.write(b"150 Opening data\r\n")
                    self.wfile.flush()
                    if passive:
                        data, peer = passive.accept()
                        if peer[0] != "10.66.2.1":
                            data.close()
                            raise AssertionError("Passive FTP bypassed MASQUERADE")
                    elif active:
                        data = socket.create_connection(active, timeout=10)
                    else:
                        raise ValueError("No data channel selected")
                    with data:
                        data.sendall(DATA)
                    answer = b"226 Transfer complete\r\n"
                elif command == "QUIT":
                    answer = b"221 Goodbye\r\n"
                    self.wfile.write(answer)
                    self.wfile.flush()
                    break
                else:
                    answer = b"500 Unsupported\r\n"
                self.wfile.write(answer)
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError, TimeoutError):
            pass
        finally:
            if passive:
                passive.close()


def ftp_server():
    class Server(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True
    with Server(("10.66.2.2", PORT), FTP) as server:
        print("SERVER_READY", flush=True)
        server.serve_forever(poll_interval=0.05)


def ftp_client(passive=False):
    with socket.create_connection(("10.66.2.2", PORT), timeout=10) as control:
        stream = control.makefile("rwb", buffering=0)
        def response(prefix):
            line = stream.readline()
            if not line.startswith(prefix):
                raise AssertionError(f"Expected {prefix!r}, received {line!r}")
            return line
        def command(value, prefix):
            stream.write(value + b"\r\n")
            return response(prefix)
        response(b"220")
        command(b"USER test", b"331")
        command(b"PASS disposable", b"230")
        command(b"TYPE I", b"200")
        listener = None
        if passive:
            answer = command(b"PASV", b"227")
            numbers = [int(x) for x in re.search(rb"\(([0-9,]+)\)", answer)[1].split(b",")]
            address = (".".join(map(str, numbers[:4])), numbers[4] * 256 + numbers[5])
        else:
            listener = socket.socket()
            listener.settimeout(10)
            listener.bind(("10.66.1.2", 0))
            listener.listen(1)
            port = listener.getsockname()[1]
            command(f"PORT 10,66,1,2,{port // 256},{port % 256}".encode(), b"200")
        print(f"EXPECTATION_READY:{control.getsockname()[1]}", flush=True)
        action = sys.stdin.readline().strip()
        try:
            if action == "transfer":
                if passive:
                    data = socket.create_connection(address, timeout=10)
                stream.write(b"RETR payload\r\n")
                response(b"150")
                if listener:
                    data, _ = listener.accept()
                payload = bytearray()
                with data:
                    while chunk := data.recv(65536):
                        payload.extend(chunk)
                response(b"226")
                if len(payload) != len(DATA) or hashlib.sha256(payload).hexdigest() != DIGEST:
                    raise AssertionError("FTP/NAT data mismatch")
                print("TRANSFER_PASS", flush=True)
        finally:
            if listener:
                listener.close()


def line(process, timeout=15):
    with selectors.DefaultSelector() as selector:
        selector.register(process.stdout, selectors.EVENT_READ)
        if not selector.select(timeout):
            process.kill()
            raise TimeoutError("Timed out waiting for child readiness")
        result = process.stdout.readline().strip()
    if not result:
        raise RuntimeError(f"Child exited before readiness, rc={process.poll()}")
    return result


def child(ns, mode, passive=False):
    args = ["ip", "netns", "exec", ns, sys.executable, __file__, "--mode", mode]
    if passive:
        args.append("--passive")
    return subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)


def client():
    process = child(CLIENT, "client")
    ready = line(process)
    if not ready.startswith("EXPECTATION_READY:"):
        raise AssertionError("FTP helper setup failed")
    process.source_port = int(ready.split(":", 1)[1])
    return process


def finish(process, action="close"):
    process.stdin.write(action + "\n")
    process.stdin.flush()
    output = process.communicate(timeout=15)[0]
    if process.returncode:
        raise RuntimeError(f"FTP client failed: {process.returncode}")
    return output


def wait_for_empty(nf, seconds=5):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if not nf.expectations():
            return
        time.sleep(0.02)
    raise AssertionError("Visible expectation lookup not empty after cleanup")


def exercise(iterations):
    nf = Netfilter(ROUTER)
    try:
        for passive in (False, True):
            process = child(CLIENT, "client", passive)
            assert line(process).startswith("EXPECTATION_READY:")
            found = nf.expectations()
            if not found:
                raise AssertionError("No real expectation created by FTP helper")
            print(json.dumps({"case": "FTP_NAT", "passive": passive, "expectations": len(found)}), flush=True)
            assert "TRANSFER_PASS" in finish(process, "transfer")
            nf.flush_masters()
            wait_for_empty(nf)
        process = client()
        found = nf.expectations()
        assert found, "Expiration test did not create a real expectation"
        deleted_before = nf.expectation_stats()["delete"]
        for item in found:
            nf.expire(item, 0)
        wait_for_empty(nf)
        # Visibility alone does not prove reclamation. Do not insert, flush,
        # delete a master or perform an expectation lookup while polling the
        # independent per-CPU delete counter. GC's adaptive scan is <=60s.
        deadline = time.monotonic() + 75
        while nf.expectation_stats()["delete"] < deleted_before + len(found):
            if time.monotonic() >= deadline:
                raise AssertionError("GC did not unlink expired expectation within75 seconds")
            time.sleep(0.1)
        assert nf.master_present(process.source_port), "Master vanished before GC proof"
        print("PASS: background GC unlinked expired expectation while its master remained present", flush=True)
        finish(process)
        nf.flush_masters()
        print("PASS: expired expectations excluded from lookup/dump", flush=True)

        stop = threading.Event()
        failures = []
        counts = {"created_controls": 0, "dumps": 0, "deadline_updates": 0, "master_deletes": 0}
        lock = threading.Lock()
        ready_masters = queue.Queue()

        def background(kind):
            local = Netfilter(ROUTER)
            try:
                while not stop.is_set():
                    if kind == "dump":
                        items = local.expectations()
                        with lock:
                            counts["dumps"] += 1
                        for item in items:
                            try:
                                local.expire(item, random.choice((0, 1)))
                                with lock:
                                    counts["deadline_updates"] += 1
                            except OSError as error:
                                if error.errno not in (errno.ENOENT, errno.ETIME):
                                    raise
                    else:
                        try:
                            source_port = ready_masters.get(timeout=0.01)
                        except queue.Empty:
                            continue
                        try:
                            local.delete_ready_master(source_port)
                            with lock:
                                counts["master_deletes"] += 1
                        except OSError as error:
                            if error.errno != errno.ENOENT:
                                raise
                    stop.wait(0.002 if kind == "dump" else random.uniform(0.01, 0.05))
            except Exception:
                failures.append(traceback.format_exc())
                stop.set()
            finally:
                local.close()

        def producer():
            try:
                for _ in range(iterations):
                    if stop.is_set():
                        break
                    process = client()
                    ready_masters.put(process.source_port)
                    time.sleep(random.uniform(0.001, 0.006))
                    finish(process)
                    with lock:
                        counts["created_controls"] += 1
            except Exception:
                failures.append(traceback.format_exc())
                stop.set()

        observers = [threading.Thread(target=background, args=(kind,)) for kind in ("dump", "flush")]
        producers = [threading.Thread(target=producer) for _ in range(2)]
        for thread in observers + producers:
            thread.start()
        for thread in producers:
            thread.join(timeout=180)
            if thread.is_alive():
                failures.append("Producer failed to finish within 180 seconds")
        stop.set()
        for thread in observers:
            thread.join(timeout=10)
            if thread.is_alive():
                failures.append("Observer failed to finish")
        if failures:
            raise AssertionError("\n".join(failures))
        assert counts["created_controls"] == iterations * 2
        assert counts["dumps"] > 0 and counts["master_deletes"] > 0
        nf.flush_masters()
        wait_for_empty(nf)
        print(json.dumps({"case": "concurrent_lifetime", **counts}), flush=True)
        print(nsrun(ROUTER, "iptables-legacy", "-t", "nat", "-L", "-v", "-n").stdout, flush=True)
    finally:
        nf.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("all", "server", "client"), default="all")
    parser.add_argument("--passive", action="store_true")
    parser.add_argument("--iterations", type=int, default=40)
    parser.add_argument("--parser-selftest", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.iterations <= 1000:
        parser.error("--iterations must be between 1 and 1000")
    if args.parser_selftest:
        sample = attribute(0x8002, attribute(1, b"abcd")) + attribute(4, struct.pack("!I", 12))
        decoded = attributes(sample)
        assert decoded[4][1] == struct.pack("!I", 12)
        assert attributes(decoded[2][1])[1][1] == b"abcd"
        try:
            attributes(b"\x02\x00\x01\x00")
        except ValueError:
            pass
        else:
            raise AssertionError("Malformed attribute accepted")
        print("PASS: pure parser tests; no kernel/network operations performed")
        return
    require_vm()
    if args.mode == "server":
        ftp_server()
    elif args.mode == "client":
        ftp_client(args.passive)
    else:
        server = None
        try:
            setup()
            server = child(SERVER, "server")
            assert line(server) == "SERVER_READY"
            exercise(args.iterations)
            # Repeat teardown with outstanding helper expectations, not merely
            # empty network namespaces. Only disposable guest namespaces exist.
            for _ in range(8):
                process = client()
                probe = Netfilter(ROUTER)
                assert probe.expectations()
                probe.close()
                server.terminate()
                server.wait(timeout=5)
                process.terminate()
                process.wait(timeout=5)
                cleanup()
                setup()
                server = child(SERVER, "server")
                assert line(server) == "SERVER_READY"
            print("PASS: eight outstanding-expectation namespace teardown cycles", flush=True)
        finally:
            if server and server.poll() is None:
                server.terminate()
                server.wait(timeout=5)
            cleanup()


if __name__ == "__main__":
    main()
