#!/usr/bin/env python3
"""Tests for the DP's two counting modes and choice-side constraints.

    python tools/frontier-dp/test_counting.py [trees_dir]

Synthetic trees, so no network and no solver needed. The DP is cross-checked against the
C++ engine separately by crosscheck_json.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from frontier_dp import build_expanded_graph, count_frontier_dp, topo_sort

_failures = []
_run = 0


def check(name, fn):
    global _run
    _run += 1
    try:
        fn()
        print(f"  [ok  ] {name}")
    except AssertionError as exc:
        _failures.append(name)
        print(f"  [FAIL] {name}: {exc}")
    except Exception as exc:  # noqa: BLE001
        _failures.append(name)
        print(f"  [FAIL] {name}: unexpected {type(exc).__name__}: {exc}")


def node(nid, *, parents=(), req=0, ranks=1, choice=False, pre=False, row=0, col=0):
    return {
        "index": nid, "name": f"n{nid}", "type": 2 if choice else 1,
        "row": row, "col": col, "maxPoints": ranks, "req": req,
        "preFilled": pre, "parents": list(parents), "children": [],
    }


def tree(nodes):
    """Build the node dict, filling children from parents."""
    d = {n["index"]: n for n in nodes}
    for n in nodes:
        for p in n["parents"]:
            d[p]["children"].append(n["index"])
    return d


def counts(nodes, points, **kw):
    meta, par, chi = build_expanded_graph(nodes)
    order = topo_sort(meta, par, chi)
    totals, _ = count_frontier_dp(meta, par, chi, order, points, **kw)
    return totals.get(points, 0)


# --- counting modes --------------------------------------------------------

def t_one_choice_node_doubles():
    nodes = tree([node(1), node(2, parents=[1], choice=True, row=1)])
    assert counts(nodes, 2) == 1, counts(nodes, 2)
    assert counts(nodes, 2, weight_choices=True) == 2


def t_two_choice_nodes_quadruple():
    nodes = tree([node(1),
                  node(2, parents=[1], choice=True, row=1),
                  node(3, parents=[1], choice=True, row=1, col=1)])
    assert counts(nodes, 3) == 1
    assert counts(nodes, 3, weight_choices=True) == 4


def t_sets_and_builds_agree_without_choice_nodes():
    """Without choice nodes the modes must be identical, or the multiplier is leaking
    into trees that have nothing to multiply."""
    nodes = tree([node(1), node(2, parents=[1], row=1), node(3, parents=[2], row=2)])
    for pts in (1, 2, 3):
        assert counts(nodes, pts) == counts(nodes, pts, weight_choices=True), pts


# --- side constraints ------------------------------------------------------

def t_side_constraint_partitions_the_space():
    """side a + side b + excluded == unconstrained.

    The analogue of the must-have/must-not-have complement property: every build either
    takes the node as a, takes it as b, or does not take it.
    """
    nodes = tree([node(1),
                  node(2, parents=[1], choice=True, row=1),
                  node(3, parents=[1], row=1, col=1)])
    for pts in (1, 2, 3):
        base = counts(nodes, pts, weight_choices=True)
        a = counts(nodes, pts, weight_choices=True, choice_sides={2: "a"})
        b = counts(nodes, pts, weight_choices=True, choice_sides={2: "b"})
        none = counts(nodes, pts, weight_choices=True, choice_sides={2: "none"})
        assert a + b + none == base, f"{pts} points: {a}+{b}+{none} != {base}"


def t_pinning_a_side_removes_the_doubling():
    nodes = tree([node(1), node(2, parents=[1], choice=True, row=1)])
    both = counts(nodes, 2, weight_choices=True)
    pinned = counts(nodes, 2, weight_choices=True, choice_sides={2: "a"})
    assert both == 2 and pinned == 1, (both, pinned)


def t_pinned_side_forces_the_node_to_be_taken():
    """A build cannot satisfy "must have side a" by skipping the node."""
    nodes = tree([node(1), node(2, parents=[1], choice=True, row=1)])
    assert counts(nodes, 1, weight_choices=True, choice_sides={2: "a"}) == 0


def t_excluding_a_choice_node_admits_only_builds_without_it():
    nodes = tree([node(1), node(2, parents=[1], choice=True, row=1)])
    assert counts(nodes, 1, weight_choices=True, choice_sides={2: "none"}) == 1
    assert counts(nodes, 2, weight_choices=True, choice_sides={2: "none"}) == 0


def t_side_constraints_compose():
    nodes = tree([node(1),
                  node(2, parents=[1], choice=True, row=1),
                  node(3, parents=[1], choice=True, row=1, col=1)])
    assert counts(nodes, 3, weight_choices=True) == 4
    assert counts(nodes, 3, weight_choices=True, choice_sides={2: "a"}) == 2
    assert counts(nodes, 3, weight_choices=True, choice_sides={2: "a", 3: "b"}) == 1


# --- against real ingested trees, if present -------------------------------

def live_tests(trees_dir):
    import glob
    import json

    from frontier_dp import load_tree_json

    paths = sorted(glob.glob(os.path.join(trees_dir, "*.json")))
    if not paths:
        return
    print("\nreal trees:")

    def t_builds_never_below_sets():
        for path in paths[:40]:
            _, nodes = load_tree_json(path, level_cap=90)
            meta, par, chi = build_expanded_graph(nodes)
            order = topo_sort(meta, par, chi)
            cap = min(len(meta), 16)
            sets, _ = count_frontier_dp(meta, par, chi, order, cap)
            builds, _ = count_frontier_dp(meta, par, chi, order, cap, weight_choices=True)
            for pts, s in sets.items():
                assert builds.get(pts, 0) >= s, f"{path} @{pts}: {builds.get(pts)} < {s}"

    def t_side_partition_on_a_real_tree():
        path = next((p for p in paths if p.endswith("retail_11_102_spec.json")), paths[0])
        tree_json = json.load(open(path, encoding="utf-8"))
        choice_ids = [n["nodeId"] for n in tree_json["nodes"] if n["kind"] == "choice"]
        assert choice_ids, "expected choice nodes in a real spec tree"
        _, nodes = load_tree_json(path, level_cap=90)
        meta, par, chi = build_expanded_graph(nodes)
        order = topo_sort(meta, par, chi)
        pts = 12
        base = count_frontier_dp(meta, par, chi, order, pts,
                                 weight_choices=True)[0].get(pts, 0)
        for cid in choice_ids[:4]:
            got = 0
            for side in ("a", "b", "none"):
                got += count_frontier_dp(meta, par, chi, order, pts, weight_choices=True,
                                         choice_sides={cid: side})[0].get(pts, 0)
            assert got == base, f"choice {cid}: partition {got} != {base}"

    check("builds never below sets", t_builds_never_below_sets)
    check("side partition holds on a real tree", t_side_partition_on_a_real_tree)


def main():
    print("counting modes:")
    for name, fn in [
        ("one choice node doubles", t_one_choice_node_doubles),
        ("two choice nodes quadruple", t_two_choice_nodes_quadruple),
        ("no choice nodes: modes identical", t_sets_and_builds_agree_without_choice_nodes),
    ]:
        check(name, fn)

    print("\nchoice sides:")
    for name, fn in [
        ("a + b + excluded == unconstrained", t_side_constraint_partitions_the_space),
        ("pinning a side removes the doubling", t_pinning_a_side_removes_the_doubling),
        ("a pinned side forces the node to be taken",
         t_pinned_side_forces_the_node_to_be_taken),
        ("excluding admits only builds without it",
         t_excluding_a_choice_node_admits_only_builds_without_it),
        ("side constraints compose", t_side_constraints_compose),
    ]:
        check(name, fn)

    trees_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join("data", "generated", "trees")
    if os.path.isdir(trees_dir):
        live_tests(trees_dir)
    else:
        print("\nreal trees: skipped (no ingested trees found)")

    print()
    if _failures:
        print(f"{len(_failures)} of {_run} FAILED: {', '.join(_failures)}")
        return 1
    print(f"all {_run} passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
