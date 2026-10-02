#!/usr/bin/env python3
"""Reference answers from the C++ engine, for the browser counter and lister to match.

The browser counts and lists builds in TypeScript (frontend/src/engine). The C++ engine is
the independent twin that keeps it honest: this script runs the native `ttm-solver` on every
tree under a fixed mix of searches and writes what it says, and `frontend/parity.test.mjs`
asserts the browser says exactly the same. The release CI runs both. See
docs/03-plan/browser-only.md.

    python tools/parity/engine_reference.py --solver build/ttm-solver --out parity.json \\
        data/generated/trees data/generated-forever/trees

Per tree and search it records:
  - the engine's count of sets at small budgets (it counts by enumerating, so budgets are
    kept where enumeration is quick), and
  - the engine's full listing at the largest of those budgets with at most LISTING_MAX sets.

Custom trees come from the design fixtures in tools/parity/fixtures, built through the same
validator the editor's saves go through, so barriers, granted talents and retail-style roles
are covered too.

Every search kind the browser supports needs a case here. Adding a constraint kind to the
browser without one is how the twins would quietly drift apart.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import glob
import json
import os
import random
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "services", "ingest"))
from ttm_ingest import custom, ttm_format  # noqa: E402

LEVEL_CAP = 90  # identical on both sides, or they count different trees
BUDGETS = (6, 10, 14)
LISTING_MAX = 3000
SEED = 20261003


# --- the engine's filter and output, as the worker spoke them --------------------------------

def filter_string(tree: dict, search: dict) -> str:
    """The engine's filter: positional over the tree's nodes, one value per talent.

    >0 at least that many ranks, -1 none, -2 an at-least-one member, -3 an exactly-one member,
    and an optional "/cap" for the most ranks allowed. A pinned choice side means "taken"
    (the engine lists sets, whose sides are open) and "none" means not taken.
    """
    order = [n["nodeId"] for n in tree["nodes"]]
    values = ["0"] * len(order)
    at = {nid: i for i, nid in enumerate(order)}
    for nid in search.get("mustHave", []):
        values[at[nid]] = "1"
    for nid in search.get("mustNotHave", []):
        values[at[nid]] = "-1"
    for nid, side in search.get("choiceSides", {}).items():
        values[at[int(nid)]] = "-1" if side == "none" else "1"
    for group in search.get("atLeastOneOf", []):
        for nid in group:
            values[at[nid]] = "-2"
    for group in search.get("exactlyOneOf", []):
        for nid in group:
            values[at[nid]] = "-3"
    for nid, low in search.get("rankMin", {}).items():
        values[at[int(nid)]] = str(low)
    for nid, high in search.get("rankMax", {}).items():
        values[at[int(nid)]] += f"/{high}"
    return ":".join(values)


def decode(path: str, tree: dict) -> list[dict[str, int]]:
    """The engine's listing as talent-id keyed rank maps (see the header it writes)."""
    ids = [n["nodeId"] for n in tree["nodes"]]
    out = []
    with open(path, encoding="utf-8") as handle:
        header = handle.readline().strip()
        if not header:
            return out
        bits = [(1 << b, str(ids[int(i)])) for b, i in enumerate(x for x in header.split("/") if x)]
        for line in handle:
            line = line.strip()
            if not line:
                continue
            mask = int(line.split(",", 1)[0])
            build: dict[str, int] = {}
            for value, key in bits:
                if mask & value:
                    build[key] = build.get(key, 0) + 1
            out.append(build)
    return out


def run_engine(solver: str, tree: dict, search: dict, points: int, *, listing: bool) -> tuple[int, list | None]:
    with tempfile.TemporaryDirectory() as tmp:
        structure = os.path.join(tmp, "tree.txt")
        output = os.path.join(tmp, "out.txt")
        with open(structure, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(ttm_format.tree_to_structure_line(tree, level_cap=LEVEL_CAP) + "\n")
        args = [solver, "--structure-file-path", structure, "--structure-indices", "0",
                "--target-talent-count", str(points), "--max-results", "100000000"]
        args += ["--output-file-path", output] if listing else ["--count-only"]
        if any(search.values()):
            args += ["--filter", filter_string(tree, search)]
        done = subprocess.run(args, capture_output=True, text=True, timeout=600)
        found = re.findall(r"Tree (\d+): (\d+) combinations", done.stdout)
        if not found or "INCOMPLETE" in done.stdout:
            raise RuntimeError(f"{tree['key']}: engine gave no count\n{done.stdout[-400:]}\n{done.stderr[-400:]}")
        return int(found[0][1]), (decode(output, tree) if listing else None)


# --- what to ask -------------------------------------------------------------------------------

def searches(tree: dict, rng: random.Random) -> list[tuple[str, dict]]:
    """Every search kind alone, and combined. One group of each kind: the engine's limit."""
    nodes = [n for n in tree["nodes"] if not n.get("preFilled") and n["kind"] != "subtree"]
    plain = sorted(n["nodeId"] for n in nodes if n["kind"] != "choice")
    choices = sorted(n["nodeId"] for n in nodes if n["kind"] == "choice")
    ranks = {n["nodeId"]: ttm_format._resolve_max_points(n, LEVEL_CAP) for n in nodes}
    multi = [i for i in plain if ranks[i] >= 2]
    out: list[tuple[str, dict]] = [("none", {})]
    if len(plain) >= 2:
        out.append(("mustHave", {"mustHave": rng.sample(plain, 2)}))
        out.append(("mustNotHave", {"mustNotHave": rng.sample(plain, 2)}))
    if choices:
        out.append(("side", {"choiceSides": {str(rng.choice(choices)): rng.choice(["a", "b", "none"])}}))
    if len(plain) >= 3:
        out.append(("atLeastOneOf", {"atLeastOneOf": [rng.sample(plain, 3)]}))
        out.append(("exactlyOneOf", {"exactlyOneOf": [rng.sample(plain, 2)]}))
    if multi:
        m = rng.choice(multi)
        top = ranks[m]
        out.append(("rankMin", {"rankMin": {str(m): rng.randint(2, top)}}))
        out.append(("rankMax", {"rankMax": {str(m): rng.randint(0, top - 1)}}))
        k = rng.randint(1, top - 1)
        out.append(("rankExact", {"mustHave": [m], "rankMin": {str(m): k}, "rankMax": {str(m): k}}))
    if len(plain) >= 6:
        a, b, c, d, e, f = rng.sample(plain, 6)
        mix = {"mustHave": [a], "mustNotHave": [b], "atLeastOneOf": [[c, d]], "exactlyOneOf": [[e, f]]}
        if choices:
            mix["choiceSides"] = {str(rng.choice(choices)): "a"}
        out.append(("combined", mix))
    return out


def fixture_trees() -> list[dict]:
    """Custom trees, from editor designs, through the editor's own validator and builder."""
    trees = []
    for path in sorted(glob.glob(os.path.join(os.path.dirname(__file__), "fixtures", "*.json"))):
        _, built, _ = custom.build(custom.canonical(json.load(open(path, encoding="utf-8"))))
        trees.extend(built)
    return trees


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--solver", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("dirs", nargs="*")
    args = parser.parse_args()

    trees = [json.load(open(p, encoding="utf-8")) for d in args.dirs for p in sorted(glob.glob(os.path.join(d, "*.json")))]
    trees += fixture_trees()
    rng = random.Random(SEED)
    work = [(tree, label, search) for tree in trees for label, search in searches(tree, rng)]

    def answer(item):
        tree, label, search = item
        counts: dict[int, int | None] = {}
        for b in BUDGETS:
            try:
                counts[b] = run_engine(args.solver, tree, search, b, listing=False)[0]
            except RuntimeError:
                # More points than the tree holds: the engine declines. Recorded as None,
                # and the browser must agree the budget is out of range, so a real engine
                # failure cannot hide here.
                counts[b] = None
        listable = [b for b in BUDGETS if counts[b] is not None and 0 < counts[b] <= LISTING_MAX]
        listing = None
        if listable:
            b = max(listable)
            total, builds = run_engine(args.solver, tree, search, b, listing=True)
            assert total == counts[b], (tree["key"], label, total, counts[b])
            listing = {"points": b, "builds": builds}
        return {"key": tree["key"], "tree": tree, "label": label, "search": search,
                "counts": {str(b): c for b, c in counts.items()}, "listing": listing}

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        cases = list(pool.map(answer, work))
    # Trees are written once, cases refer to them by key: a tree is large, a case is small.
    out = {"levelCap": LEVEL_CAP, "trees": {t["key"]: t for t in trees},
           "cases": [{k: v for k, v in c.items() if k != "tree"} for c in cases]}
    with open(args.out, "w", encoding="utf-8") as handle:
        json.dump(out, handle, separators=(",", ":"))
    listed = sum(1 for c in cases if c["listing"])
    print(f"{len(cases)} cases over {len(trees)} trees; {listed} with a full listing")
    return 0


if __name__ == "__main__":
    sys.exit(main())
