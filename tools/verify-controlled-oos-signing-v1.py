#!/usr/bin/env python3
"""Verify controlled-v1 module-signing certificates in a built vmlinux."""

from __future__ import annotations

import argparse
import base64
from pathlib import Path


def pem_der(path: Path) -> bytes:
    lines = path.read_text(encoding="ascii").splitlines()
    payload = [line for line in lines if not line.startswith("---")]
    return base64.b64decode("".join(payload), validate=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vmlinux", type=Path, required=True)
    parser.add_argument("--signing-x509", type=Path, required=True)
    parser.add_argument("--project-cert", type=Path, required=True)
    parser.add_argument("--stock-cert", type=Path, required=True)
    args = parser.parse_args()

    vmlinux = args.vmlinux.read_bytes()
    signing = args.signing_x509.read_bytes()
    project = pem_der(args.project_cert)
    stock = pem_der(args.stock_cert)

    if signing != project:
        raise SystemExit("Kbuild signing_key.x509 differs from controlled-v1 project certificate")
    for name, certificate in (("controlled-v1", project), ("stock OOS", stock)):
        if certificate not in vmlinux:
            raise SystemExit(f"{name} certificate is absent from vmlinux trusted-key payload")

    print("trusted certificates present in vmlinux: stock-oos, controlled-v1")


if __name__ == "__main__":
    main()
