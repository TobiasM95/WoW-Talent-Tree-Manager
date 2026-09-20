"""
Frontier DP prototype: count valid talent selections WITHOUT enumerating them.

Replicates the engine's semantics exactly so the counts can be compared against
ttm-solver output:
  - pre-filled roots are deleted; their children become roots
  - multi-rank talents expand into a rank chain; the original children attach to
    the LAST rank (so children open only when the talent is fully maxed)
  - topological order = Kahn's, ready queue sorted by pointsRequired
  - a node may be taken iff (points already spent) >= its pointsRequired
    and (it is a root or >= 1 parent is already taken)

Counts SETS (switch/choice multiplicity is a separate post-pass in the engine).
"""
import sys
from collections import defaultdict


def parse_tree(path, line_index):
    line = open(path, encoding='utf-8').read().split('\n')[line_index]
    recs = line.split(';')
    header = recs[0].split(':')
    num_talents = int(header[6])
    nodes = {}
    for rec in recs[1:]:
        if not rec.strip():
            continue
        f = rec.split(':')
        if len(f) < 11:
            continue
        idx = int(f[0])
        nodes[idx] = {
            'index': idx,
            'name': f[1],
            'type': int(f[3]),
            'row': int(f[4]),
            'col': int(f[5]),
            'maxPoints': int(f[6]),
            'req': int(f[7]),
            'preFilled': bool(int(f[8])),
            'parents': [int(x) for x in f[9].split(',') if x.strip()],
            'children': [int(x) for x in f[10].split(',') if x.strip()],
        }
    assert len(nodes) == num_talents, f"expected {num_talents} talents, parsed {len(nodes)}"
    return header, nodes


def build_expanded_graph(nodes):
    """Apply the pre-fill transform, then rank expansion. Returns adjacency on new ids."""
    # --- work on a mutable copy: parents/children as sets of original ids
    par = {i: set(n['parents']) for i, n in nodes.items()}
    chi = {i: set(n['children']) for i, n in nodes.items()}
    alive = set(nodes)

    # --- pre-fill transform (expandTreeTalents): a pre-filled ROOT is removed and
    #     each of its children becomes a root (all of that child's parent links cut).
    #     Iterate to a fixed point, since a child promoted to root may itself be pre-filled.
    changed = True
    while changed:
        changed = False
        for i in list(alive):
            if i not in alive:
                continue
            is_root = len(par[i]) == 0
            if is_root and nodes[i]['preFilled']:
                for c in list(chi[i]):
                    # cut ALL parent links of the child (engine: child->parents.clear()
                    # and removal from every parent's children list)
                    for p in list(par[c]):
                        chi[p].discard(c)
                    par[c].clear()
                alive.discard(i)
                for c in list(chi[i]):
                    pass
                chi[i] = set()
                changed = True

    # --- rank expansion: node with maxPoints M becomes a chain r0 -> r1 -> ... -> r(M-1)
    #     original children attach to r(M-1)
    new_par = defaultdict(set)
    new_chi = defaultdict(set)
    meta = {}

    def first_id(i):
        return (i, 0)

    def last_id(i):
        return (i, nodes[i]['maxPoints'] - 1)

    for i in sorted(alive):
        M = nodes[i]['maxPoints']
        for r in range(M):
            nid = (i, r)
            meta[nid] = {'req': nodes[i]['req'], 'orig': i, 'rank': r,
                         'row': nodes[i]['row'], 'col': nodes[i]['col'],
                         'name': nodes[i]['name'], 'type': nodes[i]['type']}
            if r > 0:
                new_par[nid].add((i, r - 1))
                new_chi[(i, r - 1)].add(nid)

    for i in sorted(alive):
        for c in chi[i]:
            if c not in alive:
                continue
            a, b = last_id(i), first_id(c)
            new_chi[a].add(b)
            new_par[b].add(a)

    for nid in meta:
        new_par.setdefault(nid, set())
        new_chi.setdefault(nid, set())
    return meta, dict(new_par), dict(new_chi)


def topo_sort(meta, par, chi):
    """Kahn's algorithm, ready queue kept sorted by pointsRequired (engine behaviour)."""
    indeg = {n: len(par[n]) for n in meta}
    ready = [n for n in meta if indeg[n] == 0]
    ready.sort(key=lambda n: (meta[n]['req'], meta[n]['orig'], meta[n]['rank']))
    order = []
    while ready:
        n = ready.pop(0)
        order.append(n)
        for m in sorted(chi[n]):
            indeg[m] -= 1
            if indeg[m] == 0:
                ready.append(m)
        ready.sort(key=lambda n: (meta[n]['req'], meta[n]['orig'], meta[n]['rank']))
    assert len(order) == len(meta), f"cycle? {len(order)} of {len(meta)}"
    return order


def count_frontier_dp(meta, par, chi, order, max_points, verbose=False):
    """
    Returns dict {k: number of valid selections of size exactly k} for k in 0..max_points.

    State = (frozenset of taken nodes that are still needed by unprocessed children,
             points spent).
    """
    pos = {n: i for i, n in enumerate(order)}
    # a node's taken-status is needed until its last child has been processed
    last_needed = {n: max([pos[c] for c in chi[n]], default=-1) for n in meta}

    # state -> count.  state = (frozenset live_taken, points)
    states = {(frozenset(), 0): 1}
    peak = 1

    for i, n in enumerate(order):
        req = meta[n]['req']
        parents = par[n]
        nxt = defaultdict(int)
        for (live, pts), cnt in states.items():
            # option 1: skip n
            nxt[(live, pts)] += cnt
            # option 2: take n
            if pts < max_points and pts >= req:
                if not parents or any(p in live for p in parents):
                    nxt[(live | {n}, pts + 1)] += cnt
        # prune: forget nodes no longer needed by any unprocessed child
        pruned = defaultdict(int)
        for (live, pts), cnt in nxt.items():
            keep = frozenset(x for x in live if last_needed[x] > i)
            pruned[(keep, pts)] += cnt
        states = dict(pruned)
        peak = max(peak, len(states))
        if verbose:
            print(f"  after {i+1:3d}/{len(order)} nodes: {len(states):6d} states", file=sys.stderr)

    totals = defaultdict(int)
    for (live, pts), cnt in states.items():
        totals[pts] += cnt
    return dict(totals), peak


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'Engine/resources/presets.txt'
    line_index = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    max_points = int(sys.argv[3]) if len(sys.argv) > 3 else 30

    header, nodes = parse_tree(path, line_index)
    print(f"preset: {header[1]!r}  ({len(nodes)} nodes)")
    pf = [i for i, n in nodes.items() if n['preFilled']]
    print(f"pre-filled nodes: {len(pf)}")
    print(f"sum(maxPoints): {sum(n['maxPoints'] for n in nodes.values())}")

    meta, par, chi = build_expanded_graph(nodes)
    print(f"expanded graph: {len(meta)} single-point nodes")

    order = topo_sort(meta, par, chi)
    totals, peak = count_frontier_dp(meta, par, chi, order, max_points)

    print(f"peak DP states: {peak}")
    print()
    print("points |            count")
    print("-------|-----------------")
    for k in range(1, max_points + 1):
        print(f"{k:6d} | {totals.get(k, 0):16,d}")


if __name__ == '__main__':
    main()


# ---------------------------------------------------------------------------
# Reading the ingest's tree JSON (services/ingest), as opposed to the legacy
# presets.txt. Returns the same node dict shape as parse_tree, so the DP and the
# validation harness work unchanged on current-patch data.
# ---------------------------------------------------------------------------

def _resolve_max_points(node, level_cap):
    """Tiered nodes' max ranks depend on character level; resolve against a cap.

    Must match services/ingest/ttm_ingest/ttm_format._resolve_max_points, or the DP
    and the engine are counting different trees.
    """
    declared = node.get('maxPoints') or 1
    rank_levels = node.get('rankLevels')
    if not rank_levels or level_cap is None:
        return declared
    allowed = 0
    for step in rank_levels:
        if level_cap >= step['level']:
            allowed = max(allowed, step['maxRanks'])
    return max(1, min(declared, allowed)) if allowed else 1


def load_tree_json(path, level_cap=None):
    """Load an ingested tree as (header, nodes) matching parse_tree's shape."""
    import json
    tree = json.load(open(path, encoding='utf-8'))
    ids = {n['nodeId'] for n in tree['nodes']}
    nodes = {}
    for n in tree['nodes']:
        nodes[n['nodeId']] = {
            'index': n['nodeId'],
            'name': n.get('name') or '',
            'type': 2 if n.get('kind') == 'choice' else 1,
            'row': n['row'],
            'col': n['col'],
            'maxPoints': _resolve_max_points(n, level_cap),
            'req': n['pointsRequired'],
            'preFilled': bool(n['preFilled']),
            # keep only in-tree edges, as the ingest already does
            'parents': [p for p in n['parents'] if p in ids],
            'children': [c for c in n['children'] if c in ids],
        }
    header = [tree.get('schemaVersion'), tree.get('key'), tree.get('kind'),
              tree.get('name'), '', '', len(nodes), 0]
    return header, nodes
