#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Classify the full unabridged small report; never turn GKI FAIL into PASS."""
import argparse
import json
from pathlib import Path
import re


def owners(path):
    result = {}
    for block in re.split(r'\n\s*\n', path.read_text().strip()):
        title = block.splitlines()[0]
        assert title.startswith(('function symbol', 'variable symbol', 'type ')), title
        assert title not in result, title
        result[title] = block
    return result


p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline', type=Path, required=True)
p.add_argument('--candidate', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
old, new = owners(a.baseline), owners(a.candidate)
removed = sorted(old.keys() - new.keys())
changed = [key for key in old if key in new and old[key] != new[key]]
added = {key: new[key] for key in new.keys() - old.keys()}
expected = "type 'struct task_struct' changed"
reviewed = (not removed and not changed and set(added) == {expected} and
            added[expected].splitlines()[1:] == [
                "  member 'union { struct sysv_sem sysvsem; unsigned char __kabi_ignored0; }' was added",
                "  member 'union { struct sysv_shm sysvshm; unsigned char __kabi_ignored1; }' was added"])
report = {'enforced_GKI_result': 'FAIL', 'baseline_owner_count': len(old),
          'candidate_owner_count': len(new), 'removed_owners': removed,
          'changed_prior_owners': changed, 'new_owners': added,
          'delta_review': 'REVIEWED C-PRIVATE IPC ADDITION; OLD LAYOUT/CRCS REQUIRE SEPARATE PROOF'
                          if reviewed else 'UNEXPLAINED; STOP'}
a.out.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
assert reviewed, 'Unexplained enforced GKI difference'
