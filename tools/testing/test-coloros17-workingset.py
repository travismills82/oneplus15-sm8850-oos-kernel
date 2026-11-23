#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile the real count_shadow_nodes() with bounded, controlled inputs."""

import argparse
from pathlib import Path
import subprocess
import tempfile


HARNESS = r'''
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
typedef struct { int unused; } lv_t;
struct lruvec { int unused; } local_vec;
struct shrinker { int unused; };
struct shrink_control { void *memcg; int nid; };
struct list_lru { bool memcg_aware; };
static struct list_lru shadow_nodes;
#define SHRINK_EMPTY (ULONG_MAX - 1)
#define PAGE_SHIFT 12
#define XA_CHUNK_SHIFT 6
#define NR_LRU_LISTS 4
#define NR_LRU_BASE 0
#define NR_SLAB_RECLAIMABLE_B 10
#define NR_SLAB_UNRECLAIMABLE_B 11
#define NODE_DATA(nid) (nid)
static unsigned long live_nodes, pages_in_node;
static unsigned long local_pages, local_slab;
static unsigned int flushes;
static unsigned long list_lru_shrink_count(struct list_lru *l, struct shrink_control *sc) { return live_nodes; }
static unsigned long node_present_pages(int nid) { return pages_in_node; }
static void mem_cgroup_flush_stats_ratelimited(void *memcg) { ++flushes; }
static struct lruvec *mem_cgroup_lruvec(void *memcg, int nid) { return &local_vec; }
static unsigned long lruvec_page_state_local(struct lruvec *l, int index) {
    return index == 0 ? local_pages : index >= 10 ? local_slab : 0;
}
'''

CASES = r'''
int main(void) {
    struct shrink_control sc = {.memcg = &local_vec};
    pages_in_node = 8000;
    local_pages = 96;
    local_slab = 8 << PAGE_SHIFT;
    live_nodes = 0;
    assert(count_shadow_nodes(NULL, &sc) == SHRINK_EMPTY && !flushes);
    live_nodes = 600;
    assert(count_shadow_nodes(NULL, &sc) == 0 && !flushes);
    live_nodes = 1100;
    assert(count_shadow_nodes(NULL, &sc) == 100 && !flushes);
    shadow_nodes.memcg_aware = true;
#ifdef CONFIG_MEMCG
    assert(count_shadow_nodes(NULL, &sc) == 1086 && flushes == 1);
#else
    assert(count_shadow_nodes(NULL, &sc) == 100 && !flushes);
#endif
    sc.memcg = NULL;
    assert(count_shadow_nodes(NULL, &sc) == 100);
    live_nodes = 1000;
    assert(count_shadow_nodes(NULL, &sc) == 0);
    puts("empty, nokmem node cap, active memcg local cap, null memcg and boundary: PASS");
    return 0;
}
'''


def check(path, compiler):
    text = path.read_text()
    start = text.index("static unsigned long count_shadow_nodes(")
    end = text.index("\nstatic enum lru_status", start)
    code = text[start:end]
    with tempfile.TemporaryDirectory(prefix="workingset-") as tmp:
        source, output = (Path(tmp) / n for n in ("test.c", "test"))
        source.write_text(HARNESS + code + CASES)
        for target in ([], ["-DCONFIG_MEMCG=1"]):
            subprocess.run([compiler, "-O2", "-Wall", "-no-pie", "-Wno-unused-function",
                            *target, str(source), "-o", str(output)], check=True)
            subprocess.run([str(output)], check=True)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("workingset", type=Path)
    p.add_argument("--cc", default="cc")
    args = p.parse_args()
    check(args.workingset, args.cc)
