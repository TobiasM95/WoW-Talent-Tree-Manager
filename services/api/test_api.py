#!/usr/bin/env python3
"""API tests, run against a live instance.

    docker compose up -d api
    python services/api/test_api.py [base_url]

Uses only the standard library, so it needs nothing installed locally. Asserts behaviour
and invariants, not exact counts -- those are pinned by the DP and engine suites.
"""
from __future__ import annotations

import json
import sys
import urllib.error
import urllib.request

BASE = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:8000"

_failures: list[str] = []
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


def get(path):
    with urllib.request.urlopen(BASE + path, timeout=60) as r:
        return json.loads(r.read())


def post(path, body, expect=200):
    data = json.dumps(body).encode()
    req = urllib.request.Request(
        BASE + path, data=data, headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            assert r.status == expect, f"expected {expect}, got {r.status}"
            return json.loads(r.read())
    except urllib.error.HTTPError as exc:
        assert exc.code == expect, f"expected {expect}, got {exc.code}: {exc.read()[:200]}"
        return json.loads(exc.read() or b"{}")


SPEC = "retail/11/102/spec"
HERO = "retail/11/102/hero/23"


def t_health_reports_promoted_data():
    h = get("/health")
    assert h["status"] == "ok", h
    assert h["trees"] > 0 and h["nodes"] > 0, h
    # Data age is the thing the legacy pipeline never surfaced.
    assert "dataAgeSeconds" in h, h


def t_trees_listing_covers_every_kind():
    trees = get("/trees")
    kinds = {t["kind"] for t in trees}
    assert {"class", "spec", "hero"} <= kinds, kinds
    assert len(trees) >= 160, len(trees)


def t_trees_listing_filters():
    trees = get("/trees?classId=11&specId=102")
    assert len(trees) == 4, trees          # class + spec + two hero
    assert sum(1 for t in trees if t["kind"] == "hero") == 2


def t_tree_definition_has_graph_and_text():
    tree = get(f"/trees/{SPEC}")
    assert tree["nodes"], "no nodes"
    assert any(n["parents"] for n in tree["nodes"]), "no edges"
    assert any(n["entries"][0]["ranks"] for n in tree["nodes"]), "no descriptions"


def t_precomputed_counts_are_monotone_in_nothing_but_present():
    rows = get(f"/trees/{SPEC}/counts")
    assert len(rows) > 10, len(rows)
    for r in rows:
        # A set with k choice nodes expands to 2^k builds, so builds >= sets always.
        assert r["builds"] >= r["sets"], r


def t_unfiltered_count_is_precomputed():
    r = post("/counts", {"treeKey": SPEC, "points": 20})
    assert r["source"] == "precomputed", r
    assert r["filtered"] is False
    assert r["builds"] >= r["sets"] > 0


def t_filtered_count_is_computed_and_never_wider():
    """A filter can only remove builds, never add them.

    Note it does not always *reduce* the count: the top rows of a spec tree are taken by
    every build at a realistic budget (excluding Eclipse at 20 points yields zero), so
    requiring them narrows nothing. That is a feature of the data, and it is precisely the
    "requiring this changes nothing" signal the marginals give a user.
    """
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    base = post("/counts", {"treeKey": SPEC, "points": 20})
    narrowed = post("/counts", {"treeKey": SPEC, "points": 20, "mustHave": plain[:3]})
    assert narrowed["source"] == "computed", narrowed
    assert narrowed["filtered"] is True
    assert narrowed["sets"] <= base["sets"], (narrowed["sets"], base["sets"])


def t_an_optional_talent_strictly_narrows():
    """Requiring a genuinely optional talent must reduce the count."""
    tree = get(f"/trees/{SPEC}")
    base = post("/counts", {"treeKey": SPEC, "points": 20})["sets"]
    optional = None
    for node in tree["nodes"]:
        if node["kind"] == "choice":
            continue
        excluded = post("/counts", {"treeKey": SPEC, "points": 20,
                                    "mustNotHave": [node["nodeId"]]})["sets"]
        if 0 < excluded < base:
            optional = node["nodeId"]
            break
    assert optional is not None, "expected at least one optional talent"
    required = post("/counts", {"treeKey": SPEC, "points": 20,
                                "mustHave": [optional]})["sets"]
    assert required < base, (required, base)


def t_require_and_exclude_partition_the_space():
    """The complement property, over HTTP: every build either takes the node or does not."""
    tree = get(f"/trees/{SPEC}")
    node = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"][5]
    base = post("/counts", {"treeKey": SPEC, "points": 16})["sets"]
    inc = post("/counts", {"treeKey": SPEC, "points": 16, "mustHave": [node]})["sets"]
    exc = post("/counts", {"treeKey": SPEC, "points": 16, "mustNotHave": [node]})["sets"]
    assert inc + exc == base, f"{inc} + {exc} != {base}"


def t_choice_sides_partition_the_space():
    tree = get(f"/trees/{SPEC}")
    choice = next(n["nodeId"] for n in tree["nodes"] if n["kind"] == "choice")
    base = post("/counts", {"treeKey": SPEC, "points": 14})["builds"]
    total = 0
    for side in ("a", "b", "none"):
        total += post("/counts", {"treeKey": SPEC, "points": 14,
                                  "choiceSides": {str(choice): side}})["builds"]
    assert total == base, f"{total} != {base}"


def t_gate_flips_on_result_size():
    wide = post("/counts", {"treeKey": SPEC, "points": 30})
    assert wide["listable"] is False, "a 17M-set result should not be offered for listing"
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    narrow = post("/counts", {"treeKey": SPEC, "points": 14, "mustHave": plain[:3]})
    assert narrow["listable"] is True, narrow


def t_counts_are_fast_enough_to_type_against():
    """The gate is only useful if it can run while a user paints constraints."""
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    r = post("/counts", {"treeKey": SPEC, "points": 30, "mustHave": plain[:2]})
    assert r["elapsedMs"] < 500, f"filtered count took {r['elapsedMs']}ms"


def t_unknown_node_is_rejected():
    r = post("/counts", {"treeKey": SPEC, "points": 10, "mustHave": [999999]}, expect=400)
    assert "not in" in str(r.get("detail", "")), r


def t_contradictory_filter_is_rejected():
    tree = get(f"/trees/{SPEC}")
    node = tree["nodes"][0]["nodeId"]
    r = post("/counts", {"treeKey": SPEC, "points": 10,
                         "mustHave": [node], "mustNotHave": [node]}, expect=400)
    assert "both required and excluded" in str(r.get("detail", "")), r


def t_budget_beyond_the_tree_is_rejected():
    r = post("/counts", {"treeKey": HERO, "points": 30}, expect=400)
    assert "point slots" in str(r.get("detail", "")), r


def t_unknown_tree_is_404():
    post("/counts", {"treeKey": "nope/not/real", "points": 10}, expect=404)


def main() -> int:
    print(f"api: {BASE}")
    for name, fn in [
        ("health reports promoted data", t_health_reports_promoted_data),
        ("tree listing covers every kind", t_trees_listing_covers_every_kind),
        ("tree listing filters by class and spec", t_trees_listing_filters),
        ("tree definition has graph and descriptions", t_tree_definition_has_graph_and_text),
        ("precomputed counts present, builds >= sets",
         t_precomputed_counts_are_monotone_in_nothing_but_present),
        ("unfiltered count comes from precomputed rows", t_unfiltered_count_is_precomputed),
        ("filtered count is computed and never wider",
         t_filtered_count_is_computed_and_never_wider),
        ("an optional talent strictly narrows", t_an_optional_talent_strictly_narrows),
        ("require + exclude partition the space", t_require_and_exclude_partition_the_space),
        ("choice sides partition the space", t_choice_sides_partition_the_space),
        ("listing gate flips on result size", t_gate_flips_on_result_size),
        ("counts fast enough to type against", t_counts_are_fast_enough_to_type_against),
        ("unknown node rejected", t_unknown_node_is_rejected),
        ("contradictory filter rejected", t_contradictory_filter_is_rejected),
        ("budget beyond the tree rejected", t_budget_beyond_the_tree_is_rejected),
        ("unknown tree is 404", t_unknown_tree_is_404),
    ]:
        check(name, fn)

    print()
    if _failures:
        print(f"{len(_failures)} of {_run} FAILED: {', '.join(_failures)}")
        return 1
    print(f"all {_run} passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
