#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Finish only this disposable VM after its complete guest result transcript.

This does not decide test success. run-vm.sh independently requires PASS and
rejects sanitizer/panic/locking diagnostics in the preserved serial log.
"""
import json
from pathlib import Path
import re
import socket
import sys
import time


def command(stream, name):
    stream.write(json.dumps({"execute": name}).encode() + b"\r\n")
    stream.flush()
    while True:
        line = stream.readline()
        if not line:
            if name == "quit":
                return
            raise RuntimeError("QMP closed before command response")
        reply = json.loads(line)
        if "error" in reply:
            raise RuntimeError(f"QMP error: {reply['error']}")
        if "return" in reply:
            return


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: qmp-finish.py PRIVATE_SOCKET SERIAL_LOG TIMEOUT")
    qmp, serial = map(Path, sys.argv[1:3])
    if not qmp.is_absolute() or qmp.parent != serial.parent:
        raise SystemExit("QMP and serial log must share the disposable directory")
    deadline = time.monotonic() + int(sys.argv[3])
    while time.monotonic() < deadline:
        if serial.exists():
            text = serial.read_text(errors="replace").replace("\r", "")
            complete = "FIVE_CVES_DMESG_END\n" in text and re.search(
                r"^FIVE_CVES_RESULT: (?:PASS|FAIL\b[^\n]*)$", text, re.M
            )
            if complete:
                with socket.socket(socket.AF_UNIX) as client:
                    client.settimeout(5)
                    client.connect(str(qmp))
                    with client.makefile("rwb") as stream:
                        greeting = json.loads(stream.readline())
                        if "QMP" not in greeting:
                            raise RuntimeError("Invalid QMP greeting")
                        command(stream, "qmp_capabilities")
                        command(stream, "quit")
                return
        time.sleep(0.2)
    raise SystemExit("No complete guest result; normal VM timeout remains fatal")


if __name__ == "__main__":
    main()
