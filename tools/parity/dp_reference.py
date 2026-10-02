"""Reference counts from the Python frontier DP, for the browser counter to match exactly.

The engine parity suite (engine_reference.py) checks counts at a few budgets, because the
engine counts by enumerating. The DP counts every point total at once, so this checks the
browser's full spreads too, which shared pools depend on. Kept while the Python DP exists.

    python tools/parity/dp_reference.py <out.json> data/generated/trees data/generated-forever/trees
    node counter.test.mjs <out.json>        (from frontend/)
"""
import glob
import json
import os
import random
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "frontier-dp"))
from frontier_dp import build_expanded_graph, topo_sort, _resolve_max_points, count_spread  # noqa: E402

LEVEL_CAP = 90


def graph_of(tree):
    ids = {n["nodeId"] for n in tree["nodes"]}
    nodes = {
        n["nodeId"]: {
            "index": n["nodeId"], "name": n.get("name") or "",
            "type": 2 if n.get("kind") == "choice" else 1,
            "row": n["row"], "col": n["col"],
            "maxPoints": _resolve_max_points(n, LEVEL_CAP),
            "req": n["pointsRequired"], "preFilled": bool(n["preFilled"]),
            "parents": [p for p in n["parents"] if p in ids],
            "children": [c for c in n["children"] if c in ids],
        }
        for n in tree["nodes"]
    }
    meta, par, chi = build_expanded_graph(nodes)
    order = topo_sort(meta, par, chi)
    modelled = {m["orig"] for m in meta.values()}
    return (meta, par, chi, order), nodes, modelled


def searches(tree, nodes, modelled, rng):
    """A spread of searches: none, each filter kind alone, and some combined."""
    usable = [i for i in sorted(modelled)]
    plain = [i for i in usable if nodes[i]["type"] == 1]
    choices = [i for i in usable if nodes[i]["type"] == 2]
    multi = [i for i in plain if nodes[i]["maxPoints"] >= 2]
    out = [("none", {})]
    if len(plain) >= 2:
        out.append(("mustHave", {"mustHave": rng.sample(plain, 2)}))
        out.append(("mustNotHave", {"mustNotHave": rng.sample(plain, 2)}))
    if choices:
        c = rng.choice(choices)
        out.append(("side", {"choiceSides": {str(c): rng.choice(["a", "b", "none"])}}))
    if len(plain) >= 3:
        out.append(("atLeastOneOf", {"atLeastOneOf": [rng.sample(plain, 3)]}))
        out.append(("exactlyOneOf", {"exactlyOneOf": [rng.sample(plain, 2)]}))
    if multi:
        m = rng.choice(multi)
        top = nodes[m]["maxPoints"]
        low = rng.randint(1, top)
        out.append(("rankMin", {"rankMin": {str(m): low}}))
        if top >= 2:
            out.append(("rankMax", {"rankMax": {str(m): rng.randint(0, top - 1)}}))
            k = rng.randint(1, top - 1)
            out.append(("rankExact", {"rankMin": {str(m): k}, "rankMax": {str(m): k}}))
    if len(plain) >= 6:
        a, b, c, d, e, f = rng.sample(plain, 6)
        mix = {"mustHave": [a], "mustNotHave": [b], "atLeastOneOf": [[c, d]], "exactlyOneOf": [[e, f]]}
        if choices:
            mix["choiceSides"] = {str(rng.choice(choices)): "a"}
        out.append(("combined", mix))
        # Two groups of each kind: counting allows it, listing does not.
        if len(plain) >= 10:
            g = rng.sample(plain, 8)
            out.append(("twoGroupsEach", {"atLeastOneOf": [g[0:2], g[2:4]], "exactlyOneOf": [g[4:6], g[6:8]]}))
    return out


def main():
    out_path, *dirs = sys.argv[1:]
    rng = random.Random(20261003)
    cases = []
    started = time.time()
    for d in dirs:
        for path in sorted(glob.glob(os.path.join(d, "*.json"))):
            tree = json.load(open(path, encoding="utf-8"))
            args, nodes, modelled = graph_of(tree)
            slots = len(args[0])
            for label, search in searches(tree, nodes, modelled, rng):
                filters = dict(
                    require=set(search.get("mustHave", [])), exclude=set(search.get("mustNotHave", [])),
                    choice_sides={int(k): v for k, v in search.get("choiceSides", {}).items()},
                    at_least_one_of=search.get("atLeastOneOf", []), exactly_one_of=search.get("exactlyOneOf", []),
                    rank_min={int(k): v for k, v in search.get("rankMin", {}).items()},
                    rank_max={int(k): v for k, v in search.get("rankMax", {}).items()},
                )
                sets = count_spread(*args, slots, **filters)
                builds = count_spread(*args, slots, weight_choices=True, **filters)
                cases.append({
                    "tree": os.path.relpath(path, ROOT).replace("\\", "/"), "label": label, "search": search,
                    "slots": slots, "sets": [str(x) for x in sets], "builds": [str(x) for x in builds],
                })
    json.dump(cases, open(out_path, "w"), separators=(",", ":"))
    print(f"{len(cases)} cases over {len({c['tree'] for c in cases})} trees in {time.time() - started:.1f}s")


main()
