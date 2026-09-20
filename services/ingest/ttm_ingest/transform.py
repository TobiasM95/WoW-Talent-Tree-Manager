"""Raidbots payload -> TTM tree JSON.

One spec becomes several trees: a class tree, a spec tree, and one tree per hero
sub-tree. That N-trees-per-spec shape is the whole point -- the native client
hardcoded exactly two (class + spec, joined by complementaryTreeIndex), which is why
hero talents had nowhere to go and the project stalled. See
docs/02-target/data-model.md.

Anything unexpected raises. The legacy generator inferred node types and swallowed
KeyErrors inside a CI timeout, so a silent mis-ingest looked exactly like a quiet day.
"""
from __future__ import annotations

import uuid
from dataclasses import dataclass, field
from typing import Any, Iterable

SCHEMA_VERSION = 1

# Namespace for deterministic tree ids. Re-ingesting the same tree yields the same id,
# which is what makes ingest idempotent and revisions diffable.
TTM_NAMESPACE = uuid.UUID("6f1d4c30-6c2a-5a2e-9d5b-0d1f7a2c9b44")

NODE_KINDS = {"single", "choice", "tiered", "subtree"}
ENTRY_KINDS = {"active", "passive", "tierrank", "subtree"}

# Upstream uses 0 as "no prerequisite" rather than omitting the field. Treating 0 as a
# node id invents an edge to whatever node happens to be id 0.
NO_PREREQUISITE = 0


class TransformError(RuntimeError):
    """The payload does not match what the transform knows how to read."""


@dataclass
class Anomalies:
    """Things worth reporting but not worth aborting over.

    Every entry here is a real property of live data, confirmed in
    docs/02-target/raidbots-live-schema.md -- not a shrug.
    """

    degenerate_nodes: list[str] = field(default_factory=list)
    cross_tree_edges: list[str] = field(default_factory=list)
    sibling_subtree_nodes: list[str] = field(default_factory=list)
    unresolved_prerequisites: list[str] = field(default_factory=list)

    def summary(self) -> dict[str, int]:
        return {
            "degenerateNodes": len(self.degenerate_nodes),
            "crossTreeEdges": len(self.cross_tree_edges),
            "siblingSubtreeNodes": len(self.sibling_subtree_nodes),
            "unresolvedPrerequisites": len(self.unresolved_prerequisites),
        }


def _is_degenerate(node: dict[str, Any]) -> bool:
    """Nodes with no usable entry. Live data carries a handful (e.g. Evoker 93196)."""
    entries = node.get("entries") or []
    return not entries or all(not e for e in entries)


def _ordinal_axis(values: Iterable[int]) -> dict[int, int]:
    """Map distinct coordinates to 0..n-1 in order.

    Raw posX/posY are kept on every node; this is a convenience index for layout. It is
    derived per tree rather than by dividing by a magic constant -- the legacy generator
    hardcoded `// 300` plus x-offsets of 1200/9000 and broke whenever the grid moved.
    """
    return {v: i for i, v in enumerate(sorted(set(values)))}


def _entry(raw: dict[str, Any]) -> dict[str, Any]:
    kind = raw.get("type")
    if kind is not None and kind not in ENTRY_KINDS:
        raise TransformError(
            f"unknown entry type {kind!r}. Upstream has added a type this transform "
            "does not model; check it before ingesting."
        )
    return {
        "entryId": raw.get("id"),
        "definitionId": raw.get("definitionId"),
        "spellId": raw.get("spellId"),
        "visibleSpellId": raw.get("visibleSpellId"),
        "name": raw.get("name") or "",
        "kind": kind,
        "icon": raw.get("icon"),
        "index": raw.get("index"),
        "maxRanks": raw.get("maxRanks"),
        # Raidbots carries no tooltip text. Descriptions are an unsolved input; the
        # legacy pipeline scraped Wowhead HTML for them. See open question Q4.
        "ranks": [],
    }


def _node(
    raw: dict[str, Any],
    *,
    in_tree: set[int],
    rows: dict[int, int],
    cols: dict[int, int],
    anomalies: Anomalies,
    tree_label: str,
) -> dict[str, Any]:
    kind = raw.get("type")
    if kind not in NODE_KINDS:
        raise TransformError(
            f"unknown node type {kind!r} on node {raw.get('id')} in {tree_label}. "
            "Upstream has added a node type this transform does not model."
        )

    parents = [p for p in raw.get("prev", []) if p in in_tree]
    children = [c for c in raw.get("next", []) if c in in_tree]
    dropped = (len(raw.get("prev", [])) - len(parents)) + (len(raw.get("next", [])) - len(children))
    if dropped:
        anomalies.cross_tree_edges.append(f"{tree_label}:{raw.get('id')}")

    requires = raw.get("requiresNode")
    if requires == NO_PREREQUISITE:
        requires = None
    if requires is not None and requires not in in_tree:
        # Expected: hero sub-trees are shared between two specs of a class, so a
        # prerequisite can point into the sibling spec's tree. Recorded, not an error.
        anomalies.unresolved_prerequisites.append(f"{tree_label}:{raw.get('id')}->{requires}")

    return {
        "nodeId": raw["id"],
        "localId": None,
        "kind": kind,
        "name": raw.get("name") or "",
        "maxPoints": raw.get("maxRanks"),
        # Level-gated ranks: max ranks depends on character level, which the engine's
        # static Talent::maxPoints cannot express. Carried through for the solver to
        # resolve against a level cap. See open question Q10.
        "rankLevels": raw.get("rankLevels"),
        "pointsRequired": raw.get("reqPoints", 0) or 0,
        "preFilled": bool(raw.get("freeNode", False)),
        "freeLevel": raw.get("freeLevel"),
        "entryNode": bool(raw.get("entryNode", False)),
        "row": rows[raw["posY"]],
        "col": cols[raw["posX"]],
        "pos": {"x": raw["posX"], "y": raw["posY"]},
        "parents": parents,
        "children": children,
        "requiresNode": requires,
        "subTreeId": raw.get("subTreeId"),
        "entries": [_entry(e) for e in raw.get("entries", []) if e],
    }


def _build_tree(
    *,
    spec: dict[str, Any],
    kind: str,
    raw_nodes: list[dict[str, Any]],
    anomalies: Anomalies,
    sub_tree_id: int | None = None,
    sub_tree_name: str | None = None,
    source: dict[str, Any],
) -> dict[str, Any]:
    label = f"{spec['className']}/{spec['specName']}/{kind}" + (
        f"/{sub_tree_name}" if sub_tree_name else ""
    )

    usable = []
    for raw in raw_nodes:
        if _is_degenerate(raw):
            anomalies.degenerate_nodes.append(f"{label}:{raw.get('id')}")
            continue
        usable.append(raw)

    if not usable:
        raise TransformError(f"{label} has no usable nodes")

    rows = _ordinal_axis(n["posY"] for n in usable)
    cols = _ordinal_axis(n["posX"] for n in usable)
    in_tree = {n["id"] for n in usable}

    nodes = [
        _node(raw, in_tree=in_tree, rows=rows, cols=cols, anomalies=anomalies, tree_label=label)
        for raw in usable
    ]
    nodes.sort(key=lambda n: (n["row"], n["col"], n["nodeId"]))

    key_parts = ["retail", str(spec["classId"]), str(spec["specId"]), kind]
    if sub_tree_id is not None:
        key_parts.append(str(sub_tree_id))
    key = "/".join(key_parts)

    name = {
        "class": f"{spec['className']} ({spec['specName']})",
        "spec": f"{spec['specName']} {spec['className']}",
        "hero": sub_tree_name or "Hero talents",
    }[kind]

    return {
        "schemaVersion": SCHEMA_VERSION,
        "id": str(uuid.uuid5(TTM_NAMESPACE, key)),
        "key": key,
        "kind": kind,
        "game": "retail",
        "name": name,
        "description": "",
        "classId": spec["classId"],
        "className": spec["className"],
        "specId": spec["specId"],
        "specName": spec["specName"],
        "traitTreeId": spec.get("traitTreeId"),
        "subTreeId": sub_tree_id,
        # Retail gates on points spent in this tree; classic-era trees gate on a
        # mandatory prerequisite chain per tab. Explicit so one schema can span both.
        "gating": "reqPoints",
        # The game's real point cap is not in the payload, and guessing it would be
        # fabrication. maxPointsInTree is a derived fact: the sum of all max ranks.
        "pointCap": None,
        "maxPointsInTree": sum(n["maxPoints"] or 0 for n in nodes),
        "nodeCount": len(nodes),
        "source": source,
        "nodes": nodes,
    }


def transform_spec(
    spec: dict[str, Any], *, source: dict[str, Any], anomalies: Anomalies
) -> list[dict[str, Any]]:
    """One spec entry -> its class tree, spec tree, and hero sub-trees."""
    for required in ("className", "classId", "specName", "specId"):
        if required not in spec:
            raise TransformError(f"spec entry is missing {required!r}")

    trees = [
        _build_tree(
            spec=spec, kind="class", raw_nodes=spec.get("classNodes", []),
            anomalies=anomalies, source=source,
        ),
        _build_tree(
            spec=spec, kind="spec", raw_nodes=spec.get("specNodes", []),
            anomalies=anomalies, source=source,
        ),
    ]

    # Hero sub-trees. Group by the subTreeId carried on each hero node, NOT by the
    # subtree entry's nodes[] list: that list is the union across both specs sharing
    # the sub-tree, so it names nodes absent from this spec entirely.
    by_sub_tree: dict[int, list[dict[str, Any]]] = {}
    for node in spec.get("heroNodes", []):
        sub_id = node.get("subTreeId")
        if sub_id is None:
            raise TransformError(f"hero node {node.get('id')} has no subTreeId")
        by_sub_tree.setdefault(sub_id, []).append(node)

    for selector in spec.get("subTreeNodes", []):
        for entry in selector.get("entries", []):
            sub_id = entry.get("traitSubTreeId")
            nodes = by_sub_tree.pop(sub_id, [])
            if not nodes:
                # The choice is offered but none of its nodes belong to this spec.
                anomalies.sibling_subtree_nodes.append(
                    f"{spec['className']}/{spec['specName']}:subtree {sub_id} has no nodes here"
                )
                continue
            listed = set(entry.get("nodes", []))
            extra = listed - {n["id"] for n in nodes}
            if extra:
                anomalies.sibling_subtree_nodes.append(
                    f"{spec['className']}/{spec['specName']}:subtree {sub_id} "
                    f"lists {len(extra)} node(s) belonging to the sibling spec"
                )
            trees.append(
                _build_tree(
                    spec=spec, kind="hero", raw_nodes=nodes, anomalies=anomalies,
                    sub_tree_id=sub_id, sub_tree_name=entry.get("name"), source=source,
                )
            )

    if by_sub_tree:
        raise TransformError(
            f"{spec['className']}/{spec['specName']}: hero nodes reference sub-trees "
            f"{sorted(by_sub_tree)} that no subTreeNodes entry offers"
        )

    return trees


def transform(payload, *, source: dict[str, Any]) -> tuple[list[dict[str, Any]], Anomalies]:
    anomalies = Anomalies()
    trees: list[dict[str, Any]] = []
    for spec in payload.specs:
        trees.extend(transform_spec(spec, source=source, anomalies=anomalies))

    keys = [t["key"] for t in trees]
    duplicates = {k for k in keys if keys.count(k) > 1}
    if duplicates:
        raise TransformError(f"duplicate tree keys: {sorted(duplicates)}")

    trees.sort(key=lambda t: t["key"])
    return trees, anomalies
