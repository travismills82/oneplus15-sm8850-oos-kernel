#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile diagnostic bindgen output with the exact pinned Kbuild Rust flags."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, required=True)
p.add_argument('--rustc', type=Path, required=True)
p.add_argument('--bindings', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
out = a.out.resolve()
(out / 'rust/bindings').mkdir(parents=True, exist_ok=True)
shutil.copyfile(a.bindings, out / 'rust/bindings/bindings_generated.rs')
shutil.copyfile(a.build / 'rust/bindings/bindings_helpers_generated.rs',
                out / 'rust/bindings/bindings_helpers_generated.rs')
line = (a.build / 'rust/.bindings.o.cmd').read_text().splitlines()[0]
args = shlex.split(line.split(':=', 1)[1])
assert args[0].startswith('OBJTREE=') and args[1] == 'rustc'
args = [str(a.rustc.resolve())] + args[2:]
args = [('--emit=obj=' + str(out / 'bindings.o') if arg.startswith('--emit=obj=') else
         '--emit=dep-info=' + str(out / 'bindings.d') if arg.startswith('--emit=dep-info=') else
         '--emit=metadata=' + str(out / 'libbindings.rmeta') if arg.startswith('--emit=metadata=') else arg)
        for arg in args]
env = dict(os.environ, OBJTREE=str(out), RUSTC_BOOTSTRAP='1')
subprocess.run(args, cwd=a.build, env=env, check=True)
