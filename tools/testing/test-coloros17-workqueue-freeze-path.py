#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Verify the old/new workqueue freeze and thaw walks with a mixed test list."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


FUNCTIONS = ("freeze_workqueues_begin", "thaw_workqueues")

FIXTURE = r"""
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#define WQ_FREEZABLE 1
#define WARN_ON_ONCE(x) assert(!(x))
struct workqueue_struct {
    unsigned flags, mutex, saved, active, adjusts;
};
static unsigned wq_pool_mutex, per_queue_locks;
static bool workqueue_freezing;
static struct workqueue_struct workqueues[] = {
    {0, 0, 4, 4, 0}, {WQ_FREEZABLE, 0, 2, 2, 0},
    {0, 0, 1, 1, 0}, {WQ_FREEZABLE, 0, 3, 3, 0},
};
#define list_for_each_entry(wq, ignored, member) \
    for ((wq) = workqueues; (wq) < &workqueues[4]; ++(wq))
static void mutex_lock(unsigned *m)
{
    assert(!*m);
    if (m != &wq_pool_mutex) {
        assert(wq_pool_mutex);
        per_queue_locks++;
    }
    *m = 1;
}
static void mutex_unlock(unsigned *m) { assert(*m); *m = 0; }
static void wq_adjust_max_active(struct workqueue_struct *wq)
{
    assert(wq->mutex && wq_pool_mutex);
    wq->adjusts++;
    wq->active = (wq->flags & WQ_FREEZABLE) && workqueue_freezing ? 0 : wq->saved;
}
"""

CHECKS = r"""
static void run(void (*freeze)(void), void (*thaw)(void), unsigned touches)
{
    per_queue_locks = 0;
    for (unsigned pass = 0; pass < 3; pass++) {
        thaw();  /* idempotent when not freezing */
        assert(!workqueue_freezing && !wq_pool_mutex);
        freeze();
        assert(workqueue_freezing && !wq_pool_mutex);
        for (unsigned i = 0; i < 4; i++) {
            struct workqueue_struct *wq = &workqueues[i];
            assert(!wq->mutex && wq->active == (wq->flags ? 0 : wq->saved));
        }
        thaw();
        for (unsigned i = 0; i < 4; i++)
            assert(workqueues[i].active == workqueues[i].saved);
        assert(!workqueue_freezing && !wq_pool_mutex);
        assert(per_queue_locks == (pass + 1) * touches);
    }
}
int main(void)
{
    run(freeze_workqueues_begin_old, thaw_workqueues_old, 8);
    run(freeze_workqueues_begin_new, thaw_workqueues_new, 4);
    puts("WORKQUEUE_FREEZE_THAW_WALKS=PASS (3 cycles; 8 vs 4 modeled per-queue locks/cycle)");
}
"""


def bodies(path, suffix):
    source = path.read_text()
    for name in FUNCTIONS:
        declaration = f"void {name}(void)"
        match = re.search(r"^" + re.escape(declaration) + r"\n\{.*?^\}", source, re.M | re.S)
        if not match:
            raise ValueError(f"cannot find {name}")
        yield match[0].replace(declaration, f"void {name}_{suffix}(void)", 1)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("previous", type=Path)
    p.add_argument("candidate", type=Path)
    args = p.parse_args()
    source = "\n".join((FIXTURE, *bodies(args.previous, "old"),
                        *bodies(args.candidate, "new"), CHECKS))
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / "probe"
        subprocess.run([os.environ.get("CC", "cc"), "-Wall", "-Wextra", "-Werror",
                        "-O2", "-x", "c", "-", "-o", str(exe)],
                       input=source, text=True, check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
