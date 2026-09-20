#!/usr/bin/env python3
"""Tests for the talent ingest.

Runnable with plain python (no pytest needed, so CI installs nothing):

    python services/ingest/tests/test_ingest.py [path/to/talents.json]

A payload path enables the live-data checks; without one those are skipped and the
synthetic tests still run.
"""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from ttm_ingest import transform as T
from ttm_ingest import validate as V
from ttm_ingest import ttm_format as FMT
from ttm_ingest import descriptions as DESC
from ttm_ingest.source import Payload

SOURCE = {"provider": "test", "origin": "test", "fetchedAt": "now", "digest": "x", "revision": 1}

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
    except Exception as exc:  # noqa: BLE001 - surface unexpected errors as failures
        _failures.append(name)
        print(f"  [FAIL] {name}: unexpected {type(exc).__name__}: {exc}")


def node(node_id, *, x=0, y=0, kind="single", ranks=1, req=0, prev=None, nxt=None,
         sub=None, requires=None, free=False, entries=None):
    n = {
        "id": node_id, "type": kind, "posX": x, "posY": y, "maxRanks": ranks,
        "reqPoints": req, "prev": prev or [], "next": nxt or [],
        "entries": entries if entries is not None else [
            {"id": node_id * 10, "type": "passive", "name": f"T{node_id}",
             "spellId": 1, "definitionId": 2, "icon": "i", "index": 100, "maxRanks": ranks}
        ],
    }
    if sub is not None:
        n["subTreeId"] = sub
    if requires is not None:
        n["requiresNode"] = requires
    if free:
        n["freeNode"] = True
    return n


def spec(**over):
    base = {
        "className": "Druid", "classId": 11, "specName": "Balance", "specId": 102,
        "traitTreeId": 793,
        "classNodes": [node(1), node(2, y=100, prev=[1])],
        "specNodes": [node(10, y=0), node(11, y=100, prev=[10])],
        "heroNodes": [], "subTreeNodes": [],
    }
    base.update(over)
    return base


def payload(specs):
    return Payload(specs=specs, digest="d", fetched_at="now", origin="test", byte_size=1_000_000)


# --------------------------------------------------------------------------
# transform
# --------------------------------------------------------------------------

def t_splits_into_class_and_spec():
    trees, _ = T.transform(payload([spec()]), source=SOURCE)
    kinds = sorted(t["kind"] for t in trees)
    assert kinds == ["class", "spec"], kinds
    assert all(t["nodeCount"] == 2 for t in trees)


def t_hero_trees_grouped_by_subtreeid():
    """nodes[] is a union across both specs sharing a sub-tree, so it must not drive
    membership -- grouping is by the subTreeId carried on each hero node."""
    s = spec(
        heroNodes=[node(50, sub=23), node(51, sub=23, y=100, prev=[50]), node(60, sub=24)],
        subTreeNodes=[{
            "id": 900, "type": "subtree", "posX": 0, "posY": 0, "entries": [
                # 999 belongs to the sibling spec and is absent here
                {"id": 1, "type": "subtree", "name": "Alpha", "traitSubTreeId": 23,
                 "nodes": [50, 51, 999]},
                {"id": 2, "type": "subtree", "name": "Beta", "traitSubTreeId": 24,
                 "nodes": [60]},
            ],
        }],
    )
    trees, anomalies = T.transform(payload([s]), source=SOURCE)
    hero = sorted((t for t in trees if t["kind"] == "hero"), key=lambda t: t["subTreeId"])
    assert len(hero) == 2, len(hero)
    assert hero[0]["subTreeId"] == 23 and hero[0]["nodeCount"] == 2, hero[0]["nodeCount"]
    assert hero[1]["subTreeId"] == 24 and hero[1]["nodeCount"] == 1
    assert hero[0]["name"] == "Alpha"
    assert anomalies.sibling_subtree_nodes, "sibling-spec node should be recorded"


def t_requires_node_zero_is_not_an_edge():
    s = spec(classNodes=[node(1, requires=0), node(2, y=100, prev=[1], requires=1)])
    trees, _ = T.transform(payload([s]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    by_id = {n["nodeId"]: n for n in tree["nodes"]}
    assert by_id[1]["requiresNode"] is None, "sentinel 0 must become null"
    assert by_id[2]["requiresNode"] == 1


def t_degenerate_nodes_dropped_and_recorded():
    s = spec(classNodes=[node(1), node(2, y=100, prev=[1]), node(3, y=200, entries=[{}])])
    trees, anomalies = T.transform(payload([s]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    assert tree["nodeCount"] == 2, tree["nodeCount"]
    assert len(anomalies.degenerate_nodes) == 1


def t_unknown_node_type_raises():
    s = spec(classNodes=[node(1, kind="brand_new_thing")])
    try:
        T.transform(payload([s]), source=SOURCE)
    except T.TransformError:
        return
    raise AssertionError("an unknown node type must not be silently accepted")


def t_unknown_entry_type_raises():
    s = spec(classNodes=[node(1, entries=[{"id": 1, "type": "wat", "name": "x"}])])
    try:
        T.transform(payload([s]), source=SOURCE)
    except T.TransformError:
        return
    raise AssertionError("an unknown entry type must not be silently accepted")


def t_edges_outside_the_tree_are_dropped():
    s = spec(classNodes=[node(1, nxt=[2, 8888]), node(2, y=100, prev=[1])])
    trees, anomalies = T.transform(payload([s]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    by_id = {n["nodeId"]: n for n in tree["nodes"]}
    assert by_id[1]["children"] == [2], by_id[1]["children"]
    assert anomalies.cross_tree_edges


def t_ids_are_deterministic():
    a, _ = T.transform(payload([spec()]), source=SOURCE)
    b, _ = T.transform(payload([spec()]), source=SOURCE)
    assert [t["id"] for t in a] == [t["id"] for t in b], "same input must yield same ids"
    assert [t["key"] for t in a] == [t["key"] for t in b]


def t_rank_levels_preserved():
    s = spec(classNodes=[
        node(1, kind="tiered", ranks=4), node(2, y=100, prev=[1]),
    ])
    s["classNodes"][0]["rankLevels"] = [{"level": 81, "maxRanks": 1}, {"level": 90, "maxRanks": 4}]
    trees, _ = T.transform(payload([s]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    tiered = next(n for n in tree["nodes"] if n["nodeId"] == 1)
    assert tiered["rankLevels"], "level-gated ranks must survive the transform"
    assert tiered["maxPoints"] == 4


# --------------------------------------------------------------------------
# engine format bridge
# --------------------------------------------------------------------------

def t_name_sanitized_to_engine_alphabet():
    """validateTalentStringFormat silently rejects a whole tree over one stray char.
    Live names include "Stampede!" and "Ride or Die!", which cost four hero trees."""
    assert FMT.sanitize_name("Stampede!") == "Stampede_"
    assert FMT.sanitize_name("Ride or Die!") == "Ride or Die_"
    allowed = FMT.sanitize_name("Wild Charge (Bear)/Cat's Grace-2")
    assert allowed == "Wild Charge (Bear)/Cat's Grace-2", allowed
    # a comma inside one name would read as a second choice alternative
    assert "," not in FMT.sanitize_name("a,b")
    # colons are escaped by clean() into an allowed marker, not dropped
    assert FMT.sanitize_name("Transcendence: Linked Spirits").startswith("Transcendence__cl__")


def t_structure_line_field_shape():
    trees, _ = T.transform(payload([spec()]), source=SOURCE)
    line = FMT.tree_to_structure_line(trees[0])
    records = [r for r in line.split(";") if r]
    assert len(records[0].split(":")) == 8, "header must have 8 fields"
    for record in records[1:]:
        count = len(record.split(":"))
        assert count == 12, f"talent record has {count} fields, engine accepts 12 or 13"


def t_tiered_ranks_resolved_against_level_cap():
    # transformed shape (maxPoints), not the upstream shape (maxRanks): this runs after
    # the transform, on a tree JSON node
    node_with_levels = {
        "maxPoints": 4,
        "rankLevels": [
            {"level": 81, "maxRanks": 1},
            {"level": 84, "maxRanks": 3},
            {"level": 90, "maxRanks": 4},
        ],
    }
    assert FMT._resolve_max_points(node_with_levels, 90) == 4
    assert FMT._resolve_max_points(node_with_levels, 84) == 3
    assert FMT._resolve_max_points(node_with_levels, 81) == 1
    # below the first threshold the engine cannot say "zero ranks"; clamp to one
    assert FMT._resolve_max_points(node_with_levels, 70) == 1
    # no cap given: the declared maximum
    assert FMT._resolve_max_points(node_with_levels, None) == 4


def t_choice_node_needs_two_alternatives():
    s = spec(classNodes=[node(1, kind="choice"), node(2, y=100, prev=[1])])
    trees, _ = T.transform(payload([s]), source=SOURCE)
    try:
        FMT.tree_to_structure_line(trees[0])
    except FMT.ConversionError:
        return
    raise AssertionError("a choice node with one entry must not be emitted as SWITCH")


# --------------------------------------------------------------------------
# descriptions
# --------------------------------------------------------------------------

def t_description_extracted_from_div():
    html = ('<table><tr><td><div class="q0">Talent</div></td></tr></table>'
            '<table><tr><td><div class="q">Deals damage.<!--cooldown:1:2 sec--></div></td></tr></table>')
    assert DESC.extract_description(html) == "Deals damage."


def t_description_extracted_from_span():
    """Some talents use <span class="q">, e.g. Shaman Surging Totem (455630). Matching
    only <div> left exactly one talent unresolved out of 3,552."""
    html = '<table><tr><td><span class="q">Modifies Damage Done +100%</span></td></tr></table>'
    assert DESC.extract_description(html) == "Modifies Damage Done +100%"


def t_missing_description_is_none_not_placeholder():
    """An absent marker must be countable, not silently become content. The legacy
    pipeline substituted "Description not available", indistinguishable from success."""
    assert DESC.extract_description('<table><tr><td>no q block</td></tr></table>') is None
    assert DESC.extract_description("") is None


def t_description_strips_markup_and_colour_codes():
    html = '<div class="q">Deals |cFFFFFFFF500|r damage.<br />Then more.</div>'
    out = DESC.extract_description(html)
    assert "|c" not in out and "<br" not in out, out
    assert "500" in out and "Then more." in out


def t_required_keys_cover_every_rank_and_alternative():
    tree = {"nodes": [
        {"maxPoints": 3, "entries": [{"spellId": 1, "definitionId": 9}]},
        {"maxPoints": 1, "entries": [{"spellId": 2, "definitionId": 8},
                                     {"spellId": 3, "definitionId": 7}]},
    ]}
    keys = DESC.required_keys([tree])
    # a 3-rank talent needs all three ranks, because the numbers change per rank
    assert (1, 9, 1) in keys and (1, 9, 2) in keys and (1, 9, 3) in keys
    # each choice alternative needs rank 1
    assert (2, 8, 1) in keys and (3, 7, 1) in keys
    assert len(keys) == 5, keys


# --------------------------------------------------------------------------
# validation
# --------------------------------------------------------------------------

def t_validate_rejects_dangling_parent():
    trees, _ = T.transform(payload([spec()]), source=SOURCE)
    trees[0]["nodes"][0]["parents"] = [4242]
    try:
        V.check_trees(trees)
    except V.ValidationError:
        return
    raise AssertionError("a parent outside the tree must be rejected")


def t_validate_rejects_asymmetric_edge():
    trees, _ = T.transform(payload([spec()]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    for n in tree["nodes"]:
        n["parents"] = []
    try:
        V.check_trees(trees)
    except V.ValidationError:
        return
    raise AssertionError("child/parent disagreement must be rejected")


def t_validate_rejects_unreachable_node():
    s = spec(classNodes=[node(1), node(2, y=100)])  # node 2 has no parent and no root link
    trees, _ = T.transform(payload([s]), source=SOURCE)
    tree = next(t for t in trees if t["kind"] == "class")
    # both are roots here, so reachability holds; force an orphan instead
    tree["nodes"][1]["parents"] = [tree["nodes"][0]["nodeId"]]
    try:
        V.check_trees(trees)
    except V.ValidationError:
        return
    raise AssertionError("a node unreachable from any root must be rejected")


def t_validate_payload_requires_fields():
    bad = spec()
    del bad["specNodes"]
    try:
        V.check_payload(payload([bad]))
    except V.ValidationError:
        return
    raise AssertionError("a missing required field must be rejected")


# --------------------------------------------------------------------------
# live payload (optional)
# --------------------------------------------------------------------------

def live_tests(path):
    import json
    from ttm_ingest.source import load_file

    p = load_file(path)
    V.check_payload(p)
    trees, anomalies = T.transform(p, source=SOURCE)
    V.check_trees(trees)

    def t_every_spec_has_all_kinds():
        by_spec: dict[tuple, set] = {}
        for t in trees:
            by_spec.setdefault((t["classId"], t["specId"]), set()).add(t["kind"])
        missing = {k: v for k, v in by_spec.items() if not {"class", "spec"} <= v}
        assert not missing, f"specs missing a class or spec tree: {list(missing)[:3]}"

    def t_hero_trees_are_two_per_spec():
        counts: dict[tuple, int] = {}
        for t in trees:
            if t["kind"] == "hero":
                counts[(t["classId"], t["specId"])] = counts.get((t["classId"], t["specId"]), 0) + 1
            counts.setdefault((t["classId"], t["specId"]), counts.get((t["classId"], t["specId"]), 0))
        odd = {k: v for k, v in counts.items() if v != 2}
        assert not odd, f"specs without exactly 2 hero trees: {list(odd.items())[:3]}"

    def t_node_conservation():
        emitted = sum(t["nodeCount"] for t in trees)
        upstream = sum(
            len(s[k]) for s in p.specs for k in ("classNodes", "specNodes", "heroNodes")
        )
        dropped = len(anomalies.degenerate_nodes)
        assert emitted == upstream - dropped, (
            f"emitted {emitted}, upstream {upstream} minus {dropped} degenerate"
        )

    def t_no_zero_prerequisites_survive():
        for t in trees:
            for n in t["nodes"]:
                assert n["requiresNode"] != 0, f"{t['key']} node {n['nodeId']} kept sentinel 0"

    def t_tiered_nodes_carry_rank_levels():
        tiered = [n for t in trees for n in t["nodes"] if n["kind"] == "tiered"]
        assert tiered, "live data should contain tiered nodes"
        assert all(n["rankLevels"] for n in tiered), "every tiered node needs rankLevels"

    print("\nlive payload:")
    for name, fn in [
        ("every spec has class and spec trees", t_every_spec_has_all_kinds),
        ("every spec has exactly 2 hero trees", t_hero_trees_are_two_per_spec),
        ("no nodes lost or invented", t_node_conservation),
        ("requiresNode sentinel 0 eliminated", t_no_zero_prerequisites_survive),
        ("tiered nodes carry rankLevels", t_tiered_nodes_carry_rank_levels),
    ]:
        check(name, fn)


def main() -> int:
    print("transform:")
    for name, fn in [
        ("splits a spec into class and spec trees", t_splits_into_class_and_spec),
        ("hero trees grouped by subTreeId, not nodes[]", t_hero_trees_grouped_by_subtreeid),
        ("requiresNode 0 becomes null", t_requires_node_zero_is_not_an_edge),
        ("degenerate nodes dropped and recorded", t_degenerate_nodes_dropped_and_recorded),
        ("unknown node type raises", t_unknown_node_type_raises),
        ("unknown entry type raises", t_unknown_entry_type_raises),
        ("edges outside the tree are dropped", t_edges_outside_the_tree_are_dropped),
        ("tree ids are deterministic", t_ids_are_deterministic),
        ("rankLevels preserved", t_rank_levels_preserved),
    ]:
        check(name, fn)

    print("\nengine format bridge:")
    for name, fn in [
        ("names sanitized to the engine's alphabet", t_name_sanitized_to_engine_alphabet),
        ("structure line has the right field shape", t_structure_line_field_shape),
        ("tiered ranks resolved against level cap", t_tiered_ranks_resolved_against_level_cap),
        ("choice node needs two alternatives", t_choice_node_needs_two_alternatives),
    ]:
        check(name, fn)

    print("\ndescriptions:")
    for name, fn in [
        ("extracted from div", t_description_extracted_from_div),
        ("extracted from span", t_description_extracted_from_span),
        ("a miss is None, never a placeholder", t_missing_description_is_none_not_placeholder),
        ("markup and colour codes stripped", t_description_strips_markup_and_colour_codes),
        ("required keys cover every rank and alternative",
         t_required_keys_cover_every_rank_and_alternative),
    ]:
        check(name, fn)

    print("\nvalidation:")
    for name, fn in [
        ("rejects dangling parent", t_validate_rejects_dangling_parent),
        ("rejects asymmetric edge", t_validate_rejects_asymmetric_edge),
        ("rejects unreachable node", t_validate_rejects_unreachable_node),
        ("rejects payload missing a field", t_validate_payload_requires_fields),
    ]:
        check(name, fn)

    path = sys.argv[1] if len(sys.argv) > 1 else None
    if path and os.path.exists(path):
        live_tests(path)
    else:
        print("\nlive payload: skipped (pass a talents.json path to enable)")

    print()
    if _failures:
        print(f"{len(_failures)} of {_run} FAILED: {', '.join(_failures)}")
        return 1
    print(f"all {_run} passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
