"""Validation, on the way in and on the way out.

The legacy pipeline had none. A crash skipped the commit step, CI went green, and the
tool silently served stale data for months -- that is how the TWW break went unnoticed
from 2024-07-05 onward. So: check the payload before transforming, check the trees
before promoting, and make every failure loud.

`check_payload` raises on anything that would produce wrong output.
`check_trees` raises on structural defects and returns warnings for things that are
merely surprising.
"""
from __future__ import annotations

from typing import Any

# 13 classes, 40 specs as of 2026-09. Demon Hunter gained a third spec (Devourer),
# so the legacy code's hardcoded 39 is already wrong -- which is exactly why this is a
# reported expectation and not an assumption baked into the transform.
EXPECTED_CLASSES = 13
EXPECTED_SPECS = 40

REQUIRED_SPEC_FIELDS = ("className", "classId", "specName", "specId", "classNodes", "specNodes")
REQUIRED_NODE_FIELDS = ("id", "type", "posX", "posY", "entries")


class ValidationError(RuntimeError):
    """The data is not fit to ingest."""


def check_payload(payload) -> list[str]:
    """Validate the upstream payload. Raises on fatal problems, returns warnings."""
    warnings: list[str] = []
    specs = payload.specs

    if not specs:
        raise ValidationError("payload contains no spec entries")

    for index, spec in enumerate(specs):
        if not isinstance(spec, dict):
            raise ValidationError(f"spec entry {index} is a {type(spec).__name__}, expected object")
        for field in REQUIRED_SPEC_FIELDS:
            if field not in spec:
                raise ValidationError(
                    f"spec entry {index} ({spec.get('className')}/{spec.get('specName')}) "
                    f"is missing {field!r}"
                )
        for bucket in ("classNodes", "specNodes", "heroNodes", "subTreeNodes"):
            for node in spec.get(bucket, []):
                for field in REQUIRED_NODE_FIELDS:
                    if field not in node:
                        raise ValidationError(
                            f"{spec['className']}/{spec['specName']} {bucket} node "
                            f"{node.get('id')} is missing {field!r}"
                        )

    # Composition changes are not fatal -- Blizzard adds specs -- but they must be seen.
    # Never hardcode the class/spec set; derive it and report drift.
    classes = {s["classId"] for s in specs}
    if len(classes) != EXPECTED_CLASSES:
        warnings.append(
            f"class count is {len(classes)}, expected {EXPECTED_CLASSES}. "
            "If Blizzard added or removed a class, update EXPECTED_CLASSES deliberately."
        )
    if len(specs) != EXPECTED_SPECS:
        warnings.append(
            f"spec count is {len(specs)}, expected {EXPECTED_SPECS}. "
            "If Blizzard added or removed a spec, update EXPECTED_SPECS deliberately."
        )

    missing_hero = [
        f"{s['className']}/{s['specName']}" for s in specs if not s.get("heroNodes")
    ]
    if missing_hero:
        warnings.append(
            f"{len(missing_hero)} spec(s) carry no hero nodes: {', '.join(missing_hero[:5])}"
            + (" ..." if len(missing_hero) > 5 else "")
        )

    return warnings


def check_trees(trees: list[dict[str, Any]]) -> list[str]:
    """Validate transformed trees. Raises on structural defects, returns warnings."""
    warnings: list[str] = []

    if not trees:
        raise ValidationError("transform produced no trees")

    for tree in trees:
        label = tree["key"]
        ids = {n["nodeId"] for n in tree["nodes"]}

        if len(ids) != len(tree["nodes"]):
            raise ValidationError(f"{label}: duplicate node ids within the tree")

        for node in tree["nodes"]:
            for parent in node["parents"]:
                if parent not in ids:
                    raise ValidationError(
                        f"{label}: node {node['nodeId']} has parent {parent} not in the tree"
                    )
            for child in node["children"]:
                if child not in ids:
                    raise ValidationError(
                        f"{label}: node {node['nodeId']} has child {child} not in the tree"
                    )
            if node["maxPoints"] is None or node["maxPoints"] < 1:
                raise ValidationError(
                    f"{label}: node {node['nodeId']} has maxPoints {node['maxPoints']}"
                )
            if not node["entries"]:
                raise ValidationError(f"{label}: node {node['nodeId']} has no entries")

        # Edges must agree in both directions, or a solver walking children sees a
        # different graph than one walking parents.
        for node in tree["nodes"]:
            for child in node["children"]:
                target = next(n for n in tree["nodes"] if n["nodeId"] == child)
                if node["nodeId"] not in target["parents"]:
                    raise ValidationError(
                        f"{label}: {node['nodeId']} lists child {child}, "
                        f"but {child} does not list it as a parent"
                    )

        roots = [n for n in tree["nodes"] if not n["parents"]]
        if not roots:
            raise ValidationError(f"{label}: no root node, the graph cannot be entered")

        # Every node must be reachable from a root, or it can never be taken.
        reachable: set[int] = set()
        stack = [n["nodeId"] for n in roots]
        by_id = {n["nodeId"]: n for n in tree["nodes"]}
        while stack:
            current = stack.pop()
            if current in reachable:
                continue
            reachable.add(current)
            stack.extend(by_id[current]["children"])
        if reachable != ids:
            orphans = sorted(ids - reachable)
            raise ValidationError(
                f"{label}: {len(orphans)} node(s) unreachable from any root: {orphans[:6]}"
            )

        if tree["kind"] == "hero" and tree["subTreeId"] is None:
            raise ValidationError(f"{label}: hero tree without a subTreeId")

        gates = {n["pointsRequired"] for n in tree["nodes"]}
        if any(g < 0 for g in gates):
            raise ValidationError(f"{label}: negative pointsRequired")

    # Composition report, so a run that silently loses a spec is visible.
    by_kind: dict[str, int] = {}
    for tree in trees:
        by_kind[tree["kind"]] = by_kind.get(tree["kind"], 0) + 1
    if by_kind.get("class", 0) != by_kind.get("spec", 0):
        warnings.append(
            f"class trees ({by_kind.get('class', 0)}) and spec trees "
            f"({by_kind.get('spec', 0)}) should come in pairs"
        )

    specs_with_hero = {
        (t["classId"], t["specId"]) for t in trees if t["kind"] == "hero"
    }
    all_specs = {(t["classId"], t["specId"]) for t in trees}
    without = all_specs - specs_with_hero
    if without:
        warnings.append(f"{len(without)} spec(s) produced no hero trees")

    return warnings
