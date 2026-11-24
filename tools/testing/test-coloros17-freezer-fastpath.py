#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise real freeze_task control flow with synthetic scheduler states."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def body(path, name):
    source = path.read_text()
    match = re.search(r"^bool freeze_task\(struct task_struct \*p\)\n\{.*?^\}",
                      source, re.M | re.S)
    if not match:
        raise ValueError("freeze_task C body not found")
    return match[0].replace("bool freeze_task(", f"bool {name}(", 1)


HARNESS = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#define PF_KTHREAD 1
#define TASK_NORMAL 3
struct task_struct { unsigned flags; bool z, eligible, direct; };
static unsigned freezer_lock, eligibility, set_state, fake, wake;
#define spin_lock_irqsave(l, f) do { assert(*(l) == 0); *(l) = 1; (f) = 0; } while (0)
#define spin_unlock_irqrestore(l, f) do { assert(*(l) == 1 && (f) == 0); *(l) = 0; } while (0)
static bool frozen(struct task_struct *p) { assert(freezer_lock); return p->z; }
static bool freezing(struct task_struct *p) { assert(freezer_lock); eligibility++; return p->eligible; }
static bool __freeze_task(struct task_struct *p) { assert(freezer_lock); set_state++; return p->direct; }
static void fake_signal_wake_up(struct task_struct *p) { assert(!p->flags && freezer_lock); fake++; }
static void wake_up_state(struct task_struct *p, int s) { assert(p->flags && s == TASK_NORMAL && freezer_lock); wake++; }
"""

CHECKS = r"""
int main(void)
{
    unsigned callbacks_avoided = 0;
    for (unsigned i = 0; i < 16; i++) {
        struct task_struct p = { !!(i & 8), !!(i & 4), !!(i & 2), !!(i & 1) };
        eligibility = set_state = fake = wake = 0;
        bool previous = freeze_previous(&p);
        unsigned count = eligibility, sets = set_state, signals = fake, wakes = wake;
        assert(!freezer_lock);
        eligibility = set_state = fake = wake = 0;
        bool next = freeze_candidate(&p);
        assert(!freezer_lock && previous == next);
        assert(sets == set_state && signals == fake && wakes == wake);
        assert(eligibility == (p.z ? 0 : count));
        callbacks_avoided += count - eligibility;
    }
    assert(callbacks_avoided == 8);
    puts("FREEZER_TRUTH_TABLE_AND_SHORT_CIRCUIT=PASS (16 cases, 8 eligibility calls avoided)");
}
"""


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("previous", type=Path)
    p.add_argument("candidate", type=Path)
    args = p.parse_args()
    source = "\n".join((HARNESS, body(args.previous, "freeze_previous"),
                        body(args.candidate, "freeze_candidate"), CHECKS))
    with tempfile.TemporaryDirectory() as tmp:
        executable = Path(tmp) / "probe"
        subprocess.run([os.environ.get("CC", "cc"), "-Wall", "-Wextra", "-Werror",
                        "-O2", "-x", "c", "-", "-o", str(executable)],
                       input=source, text=True, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
