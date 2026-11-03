#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Replay pinned Kbuild bindgen into a separate diagnostic output, not source."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, required=True)
p.add_argument('--bindgen', type=Path, required=True)
p.add_argument('--libclang', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--no-default', action='append', default=[])
a = p.parse_args()
line = (a.build / 'rust/bindings/.bindings_generated.rs.cmd').read_text().splitlines()[0]
args = shlex.split(line.split(':=', 1)[1].split(';', 1)[0])
assert args[0] == 'bindgen'
args[0] = str(a.bindgen.resolve())
for name in a.no_default:
    args[1:1] = ['--no-default', name]
args[args.index('-o') + 1] = str(a.out.resolve())
args = [arg for arg in args if not arg.startswith('-Wp,-MMD,')]
env = dict(os.environ, LIBCLANG_PATH=str(a.libclang.resolve()))
subprocess.run(args, cwd=a.build, env=env, check=True)
