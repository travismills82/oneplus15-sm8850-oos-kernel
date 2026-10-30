#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Prove requested Bazel config/makefile actions need no kernel/module compile.

Input is aquery --output=jsonproto --include_commandline=false deps(set(...)).
Follow actual producer/artifact edges, not the broader configured-target graph.
"""
import argparse
import json
from pathlib import Path

def prove(data, requested):
    targets = {str(v['id']): v['label'] for v in data['targets']}
    actions = data['actions']
    producers = {str(out): index for index, action in enumerate(actions) for out in action.get('outputIds', [])}
    fragments = {str(f['id']): f for f in data.get('pathFragments', [])}
    def fragment_path(ident):
        fragment = fragments[ident]
        parent = fragment.get('parentId')
        return (fragment_path(str(parent)) + '/' if parent else '') + fragment['label']
    artifact_paths = {str(a['id']): fragment_path(str(a['pathFragmentId']))
                      for a in data.get('artifacts', []) if fragments and a.get('pathFragmentId')}
    tree_producers = {artifact_paths[str(a['id'])]: producers[str(a['id'])]
                      for a in data.get('artifacts', []) if a.get('isTreeArtifact') and str(a['id']) in producers and str(a['id']) in artifact_paths}
    depsets = {str(d['id']): d for d in data.get('depSetOfFiles', [])}
    def inputs(ident, seen):
        if ident in seen:
            return set()
        seen.add(ident)
        d = depsets[ident]
        result = set(map(str, d.get('directArtifactIds', [])))
        for child in d.get('transitiveDepSetIds', []):
            result |= inputs(str(child), seen)
        return result
    todo = [i for i,a in enumerate(actions) if targets.get(str(a.get('targetId'))) in requested]
    if not todo or {targets[str(actions[i]['targetId'])] for i in todo} != set(requested):
        raise ValueError('Not every exact requested label has an action')
    needed = set()
    while todo:
        i = todo.pop()
        if i in needed:
            continue
        needed.add(i)
        for dep in actions[i].get('inputDepSetIds', []):
            for artifact in inputs(str(dep), set()):
                if artifact in producers:
                    todo.append(producers[artifact])
                elif artifact in artifact_paths:
                    # A TreeFileArtifact may have a distinct ID from the
                    # generated directory that produces it. Do not drop it.
                    path = artifact_paths[artifact]
                    todo.extend(owner for prefix,owner in tree_producers.items() if path.startswith(prefix + '/'))
    forbidden = []
    for i in sorted(needed):
        action = actions[i]
        mnemonic = action.get('mnemonic', '')
        if mnemonic in {'KernelBuild', 'KernelModule', 'KernelBuildImage', 'KernelImages'}:
            forbidden.append({'mnemonic': mnemonic, 'target': targets[str(action['targetId'])]})
    return {'requested': sorted(requested), 'required_actions': len(needed),
            'forbidden_compile_actions': forbidden, 'result': 'FAIL' if forbidden else 'PASS'}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('jsonproto', type=Path)
    p.add_argument('labels', nargs='+')
    args = p.parse_args()
    try:
        result = prove(json.loads(args.jsonproto.read_text()), args.labels)
    except (ValueError, KeyError) as error:
        raise SystemExit(f'FAIL-CLOSED: {error}') from error
    print(json.dumps(result, indent=2))
    if result['result'] != 'PASS':
        raise SystemExit(1)

if __name__ == '__main__':
    main()
