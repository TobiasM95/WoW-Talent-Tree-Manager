"""Player-designed trees: validate a project from the editor and build it into tree records.

A project has one style, as the games do. **Classic**: one to three tabs gated by row, each
with its own budget or all sharing one pool, like WoW Forever's 51. **Retail**: one class
tree, one to four spec trees and up to six hero trees, each with its own budget and gated by
barriers; a hero tree names the specs that may take it. The output is the same tree format both ingests write, so the counting DP, the
solver, the worker and the canvas take custom trees without a special case.

**Content-addressed.** A project's id is the hash of its canonical form, so the same design
always has the same id, saving an edit writes a new version, and a key never changes meaning.
That is what makes every cache keyed by tree key safe without invalidation, and a shared link
to a custom tree reproduce exactly the tree it was made from.

**Validation is the whole job here.** Everything else in the pipeline trusts the tree format;
the ingests earn that trust by failing loudly on bad upstream data, and this has to earn it
from input a person typed. So every property the solver relies on is checked: ids unique,
edges within the tree and acyclic, ranks and gates in range, choice nodes with exactly two
alternatives. Limits are set well above any real tree and well below anything that could hurt
the service.
"""

from __future__ import annotations

import hashlib
import json
import re
import uuid
from typing import Any

from .transform import SCHEMA_VERSION, TTM_NAMESPACE

MAX_TREES = 3  # classic: vanilla's three tabs
MAX_SPECS = 4  # retail: Druid's four
MAX_HEROES = 6
MAX_NODES = 150
MAX_RANKS = 9
MAX_ROW = 30
MAX_COL = 20
MAX_GATE = 300
MAX_POOL = 300
MAX_NAME = 80
MAX_TEXT = 600
PITCH = 600
ENGINE_SLOTS = 64
ICON = re.compile(r"^[a-z0-9_\-]{1,100}$")


class CustomTreeError(ValueError):
    """The project cannot be served, and the message says why in a person's words."""


def _text(value: Any, where: str, *, limit: int = MAX_NAME, required: bool = True) -> str:
    if value is None or (isinstance(value, str) and not value.strip()):
        if required:
            raise CustomTreeError(f"{where} needs a name")
        return ""
    if not isinstance(value, str):
        raise CustomTreeError(f"{where} must be text")
    value = value.strip()
    if len(value) > limit:
        raise CustomTreeError(f"{where} is longer than {limit} characters")
    return value


def _int(value: Any, where: str, low: int, high: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not low <= value <= high:
        raise CustomTreeError(f"{where} must be a whole number from {low} to {high}")
    return value


def _entry(raw: Any, where: str, max_rank: int) -> dict[str, Any]:
    if not isinstance(raw, dict):
        raise CustomTreeError(f"{where} is not a talent")
    icon = raw.get("icon")
    if icon is not None:
        icon = str(icon).lower().removesuffix(".jpg")
        if not ICON.match(icon):
            raise CustomTreeError(f"{where}: {raw.get('icon')!r} is not an icon name")
    kind = raw.get("kind", "passive")
    if kind not in ("passive", "active"):
        raise CustomTreeError(f"{where}: kind must be passive or active")
    ranks = raw.get("ranks") or []
    if not isinstance(ranks, list) or len(ranks) > max_rank:
        raise CustomTreeError(f"{where}: at most one description per rank")
    return {
        "name": _text(raw.get("name"), where),
        "icon": icon,
        "kind": kind,
        "ranks": [_text(r, f"{where} rank {i + 1}", limit=MAX_TEXT, required=False) for i, r in enumerate(ranks)],
    }


def canonical(project: dict[str, Any]) -> dict[str, Any]:
    """Validate a project and reduce it to exactly what defines it, in a fixed order."""
    if not isinstance(project, dict):
        raise CustomTreeError("a project is an object with a name and trees")
    name = _text(project.get("name"), "The project", limit=60)
    style = project.get("style", "classic")
    if style not in ("classic", "retail"):
        raise CustomTreeError("a project's style is retail or classic")
    retail = style == "retail"
    pool = project.get("sharedPointCap")
    if pool is not None:
        if retail:
            raise CustomTreeError("a retail-style project gives each tree its own budget, not a shared pool")
        pool = _int(pool, "The shared point pool", 1, MAX_POOL)
    trees_in = project.get("trees")
    if not isinstance(trees_in, list):
        raise CustomTreeError("a project's trees are a list")
    roles = _roles(trees_in) if retail else []
    if not retail and not 1 <= len(trees_in) <= MAX_TREES:
        raise CustomTreeError(f"a classic project has one to {MAX_TREES} trees")
    spec_count = roles.count("spec")

    seen_ids: set[int] = set()
    trees: list[dict[str, Any]] = []
    for t_index, raw_tree in enumerate(trees_in):
        if not isinstance(raw_tree, dict):
            raise CustomTreeError(f"tree {t_index + 1} is not a tree")
        tree_name = _text(raw_tree.get("name"), f"Tree {t_index + 1}", limit=60)
        per_row = raw_tree.get("pointsPerRow")
        if per_row is not None:
            if retail:
                raise CustomTreeError(f"{tree_name}: a retail-style tree gates with barriers, not points per row")
            per_row = _int(per_row, f"{tree_name}: points per row", 0, 50)
        barriers = _barriers(raw_tree.get("barriers"), tree_name)
        if barriers and not retail:
            raise CustomTreeError(f"{tree_name}: a classic tree gates by row, not with barriers")
        raw_nodes = raw_tree.get("nodes")
        if not isinstance(raw_nodes, list) or not 1 <= len(raw_nodes) <= MAX_NODES:
            raise CustomTreeError(f"{tree_name} has 1 to {MAX_NODES} talents")

        nodes: list[dict[str, Any]] = []
        cells: dict[tuple[int, int], str] = {}
        ids: set[int] = set()
        for raw in raw_nodes:
            if not isinstance(raw, dict):
                raise CustomTreeError(f"{tree_name}: a talent is not an object")
            nid = _int(raw.get("nodeId"), f"{tree_name}: a talent id", 1, 2**31 - 1)
            if nid in seen_ids:
                raise CustomTreeError(f"{tree_name}: talent id {nid} is used twice")
            seen_ids.add(nid)
            ids.add(nid)
            label = _text(raw.get("name"), f"{tree_name}: talent {nid}")
            where = f"{tree_name}: {label}"
            max_rank = _int(raw.get("maxPoints"), f"{where}: ranks", 1, MAX_RANKS)
            row = _int(raw.get("row"), f"{where}: row", 0, MAX_ROW)
            col = _int(raw.get("col"), f"{where}: column", 0, MAX_COL)
            if (row, col) in cells:
                raise CustomTreeError(f"{where} shares a cell with {cells[(row, col)]}")
            cells[(row, col)] = label
            gate = raw.get("pointsRequired")
            if gate is None:
                gate = _inherited(row, per_row, barriers)
            gate = _int(gate, f"{where}: points required", 0, MAX_GATE)
            kind = raw.get("kind", "single")
            if kind not in ("single", "choice"):
                raise CustomTreeError(f"{where}: kind must be single or choice")
            entries = raw.get("entries") or [{"name": label}]
            if not isinstance(entries, list):
                raise CustomTreeError(f"{where}: entries must be a list")
            if kind == "choice" and len(entries) != 2:
                raise CustomTreeError(f"{where}: a choice node has exactly two alternatives")
            if kind == "single" and len(entries) != 1:
                raise CustomTreeError(f"{where}: a single talent has exactly one entry")
            if kind == "choice" and max_rank != 1:
                raise CustomTreeError(f"{where}: a choice node has one rank")
            granted = raw.get("granted", False)
            if not isinstance(granted, bool):
                raise CustomTreeError(f"{where}: granted is yes or no")
            parents = raw.get("parents") or []
            if not isinstance(parents, list) or len(parents) > 8:
                raise CustomTreeError(f"{where}: parents must be a list of up to 8 talents")
            node = {
                "nodeId": nid,
                "name": label,
                "kind": kind,
                "maxPoints": max_rank,
                "row": row,
                "col": col,
                "pointsRequired": gate,
                "parents": sorted({_int(p, f"{where}: a parent", 1, 2**31 - 1) for p in parents}),
                "entries": [_entry(e, f"{where} alternative {i + 1}", max_rank) for i, e in enumerate(entries)],
            }
            if granted:
                # Free, like retail's starting talents. Only when set, so older ids hold.
                node["granted"] = True
            nodes.append(node)

        by_id = {n["nodeId"]: n for n in nodes}
        for n in nodes:
            for p in n["parents"]:
                if p == n["nodeId"]:
                    raise CustomTreeError(f"{tree_name}: {n['name']} requires itself")
                if p not in ids:
                    raise CustomTreeError(f"{tree_name}: {n['name']} requires a talent that is not in the tree")
        _acyclic(nodes, by_id, tree_name)

        cap = raw_tree.get("pointCap")
        slots = sum(n["maxPoints"] for n in nodes)
        cap = slots if cap is None else _int(cap, f"{tree_name}: point budget", 1, slots)
        nodes.sort(key=lambda n: (n["row"], n["col"], n["nodeId"]))
        tree = {"name": tree_name, "pointCap": cap, "pointsPerRow": per_row, "nodes": nodes}
        if barriers:
            # Only when present, so a project saved before barriers existed keeps its id.
            tree["barriers"] = barriers
        if retail:
            tree["role"] = roles[t_index]
            if roles[t_index] == "hero":
                tree["specs"] = _hero_specs(raw_tree.get("specs"), spec_count, tree_name)
        trees.append(tree)

    if retail:
        spec_names = [t["name"] for t in trees if t["role"] == "spec"]
        if len(set(spec_names)) != len(spec_names):
            raise CustomTreeError("each spec tree needs its own name: the planner picks a spec by it")
        # Classic stays unmarked, so every project saved before styles existed keeps its id.
        return {"name": name, "style": "retail", "sharedPointCap": None, "trees": trees}
    return {"name": name, "sharedPointCap": pool, "trees": trees}


def _roles(trees_in: list[Any]) -> list[str]:
    """Retail's shape, in order: one class tree, one to four specs, up to six hero trees."""
    roles = [t.get("role") if isinstance(t, dict) else None for t in trees_in]
    if any(r not in ("class", "spec", "hero") for r in roles):
        raise CustomTreeError("each tree of a retail-style project is a class, spec or hero tree")
    if roles.count("class") != 1 or roles[0] != "class":
        raise CustomTreeError("a retail-style project has exactly one class tree, first")
    specs, heroes = roles.count("spec"), roles.count("hero")
    if not 1 <= specs <= MAX_SPECS:
        raise CustomTreeError(f"a retail-style project has one to {MAX_SPECS} spec trees")
    if heroes > MAX_HEROES:
        raise CustomTreeError(f"a retail-style project has at most {MAX_HEROES} hero trees")
    if roles != ["class"] + ["spec"] * specs + ["hero"] * heroes:
        raise CustomTreeError("a retail-style project lists its class tree, then its specs, then its hero trees")
    return roles


def _hero_specs(raw: Any, spec_count: int, tree_name: str) -> list[int]:
    """Which specs may take a hero tree, by their place among the spec trees. Default: all."""
    if raw is None:
        return list(range(spec_count))
    if not isinstance(raw, list) or not raw:
        raise CustomTreeError(f"{tree_name}: a hero tree belongs to at least one spec")
    out = sorted({_int(s, f"{tree_name}: a spec", 0, spec_count - 1) for s in raw})
    return out


def _barriers(raw: Any, tree_name: str) -> list[dict[str, int]]:
    """Retail's gates: a line above a row, and the points to spend before crossing it.

    A barrier holds for every row from its own down, until a later one raises it, which is how
    retail's 8- and 20-point lines work. Sorted by row, one per row, never lowering the gate.
    """
    if raw is None:
        return []
    if not isinstance(raw, list) or len(raw) > MAX_ROW:
        raise CustomTreeError(f"{tree_name}: barriers must be a list of up to {MAX_ROW}")
    out: dict[int, int] = {}
    for b in raw:
        if not isinstance(b, dict):
            raise CustomTreeError(f"{tree_name}: a barrier is a row and a number of points")
        row = _int(b.get("row"), f"{tree_name}: a barrier's row", 1, MAX_ROW)
        if row in out:
            raise CustomTreeError(f"{tree_name}: two barriers above row {row + 1}")
        out[row] = _int(b.get("points"), f"{tree_name}: the barrier above row {row + 1}", 1, MAX_GATE)
    rows = sorted(out)
    for above, below in zip(rows, rows[1:]):
        if out[below] <= out[above]:
            raise CustomTreeError(f"{tree_name}: the barrier above row {below + 1} must ask for more than the one above it")
    return [{"row": r, "points": out[r]} for r in rows]


def _inherited(row: int, per_row: int | None, barriers: list[dict[str, int]]) -> int:
    """The gate a talent takes from where it sits: its row's points, or the barriers above it."""
    crossed = max((b["points"] for b in barriers if b["row"] <= row), default=0)
    return max(crossed, row * per_row if per_row is not None else 0)


def _acyclic(nodes: list[dict[str, Any]], by_id: dict[int, dict[str, Any]], tree_name: str) -> None:
    """Refuse a loop: a talent cannot require, however indirectly, itself."""
    state: dict[int, int] = {}  # 1 visiting, 2 done

    def visit(nid: int, path: list[str]) -> None:
        if state.get(nid) == 2:
            return
        if state.get(nid) == 1:
            raise CustomTreeError(f"{tree_name}: {' -> '.join(path + [by_id[nid]['name']])} loops back on itself")
        state[nid] = 1
        for p in by_id[nid]["parents"]:
            visit(p, path + [by_id[nid]["name"]])
        state[nid] = 2

    for n in nodes:
        visit(n["nodeId"], [])


def project_id(canon: dict[str, Any]) -> str:
    blob = json.dumps(canon, sort_keys=True, separators=(",", ":"), ensure_ascii=False)
    return hashlib.sha256(blob.encode()).hexdigest()[:16]


def build(canon: dict[str, Any]) -> tuple[str, list[dict[str, Any]], list[str]]:
    """The project's id, its trees in the shared tree format, and any warnings."""
    pid = project_id(canon)
    warnings: list[str] = []
    out: list[dict[str, Any]] = []
    spec_names = [t["name"] for t in canon["trees"] if t.get("role") == "spec"]
    for order, t in enumerate(canon["trees"]):
        # Retail-style trees are served as retail's own kinds, so the planner takes a custom
        # project exactly as it takes a real spec: choose a spec, get its class, spec and
        # hero trees. A hero tree names the specs that may take it.
        role = t.get("role")
        children: dict[int, list[int]] = {n["nodeId"]: [] for n in t["nodes"]}
        for n in t["nodes"]:
            for p in n["parents"]:
                children[p].append(n["nodeId"])
        slots = sum(n["maxPoints"] for n in t["nodes"])
        if slots > ENGINE_SLOTS:
            warnings.append(
                f"{t['name']} has {slots} ranks; counting works, but listing builds needs {ENGINE_SLOTS} or fewer"
            )
        nodes = [
            {
                "nodeId": n["nodeId"],
                "localId": None,
                "kind": n["kind"],
                # The name the designer gave it, for choice nodes too. Deriving "A / B" here,
                # as retail's data does, meant an opened project saved back as a different one.
                "name": n["name"],
                "maxPoints": n["maxPoints"],
                "rankLevels": None,
                "pointsRequired": n["pointsRequired"],
                "preFilled": bool(n.get("granted")),
                "freeLevel": None,
                "entryNode": not n["parents"],
                "row": n["row"],
                "col": n["col"],
                "pos": {"x": n["col"] * PITCH, "y": n["row"] * PITCH},
                "parents": n["parents"],
                "children": sorted(children[n["nodeId"]]),
                "requiresNode": None,
                "subTreeId": None,
                "entries": [
                    {
                        "entryId": n["nodeId"] * 2 + i if n["kind"] == "choice" else n["nodeId"],
                        "definitionId": None,
                        "spellId": None,
                        "visibleSpellId": None,
                        "name": e["name"],
                        "kind": e["kind"],
                        "icon": e["icon"],
                        "index": i,
                        "maxRanks": n["maxPoints"],
                        "ranks": e["ranks"],
                    }
                    for i, e in enumerate(n["entries"])
                ],
            }
            for n in t["nodes"]
        ]
        key = f"custom/{pid}/{order}"
        out.append({
            "schemaVersion": SCHEMA_VERSION,
            "id": str(uuid.uuid5(TTM_NAMESPACE, key)),
            "key": key,
            "kind": role or "tab",
            "game": "custom",
            "name": t["name"],
            "description": "",
            "classId": None,
            "className": canon["name"],
            "specId": None,
            "specName": t["name"] if role == "spec" else None,
            "heroSpecs": [spec_names[i] for i in t["specs"]] if role == "hero" else None,
            "traitTreeId": None,
            # Hero trees are told apart by sub-tree id throughout; a project's own will do.
            "subTreeId": order + 1 if role == "hero" else None,
            "gating": "reqPoints",
            "pointCap": t["pointCap"] if canon["sharedPointCap"] is None else min(t["pointCap"], canon["sharedPointCap"]),
            "maxPointsInTree": slots,
            "nodeCount": len(nodes),
            "sharedPointCap": canon["sharedPointCap"],
            "pointsPerRow": t["pointsPerRow"],
            "barriers": t.get("barriers") or [],
            "order": order,
            "project": pid,
            "fullNodeOrder": None,
            "subTreeSelector": None,
            "source": {"provider": "custom"},
            "nodes": nodes,
        })
    return pid, out, warnings


def editable(canon_tree_records: list[dict[str, Any]], name: str, pool: int | None) -> dict[str, Any]:
    """A stored project back in the editor's shape, so a saved version can be edited again."""
    retail = any(t.get("kind", "tab") != "tab" for t in canon_tree_records)
    spec_names = [t["name"] for t in sorted(canon_tree_records, key=lambda t: t["order"]) if t.get("kind") == "spec"]
    return {
        "name": name,
        **({"style": "retail"} if retail else {}),
        "sharedPointCap": pool,
        "trees": [
            {
                **({"role": t["kind"]} if retail else {}),
                **({"specs": [spec_names.index(s) for s in t.get("heroSpecs") or []]} if t.get("kind") == "hero" else {}),
                "name": t["name"],
                "pointCap": t["pointCap"],
                "pointsPerRow": t.get("pointsPerRow"),
                "barriers": t.get("barriers") or [],
                "nodes": [
                    {
                        "nodeId": n["nodeId"],
                        "name": n["name"],
                        "kind": n["kind"],
                        "maxPoints": n["maxPoints"],
                        "row": n["row"],
                        "col": n["col"],
                        "pointsRequired": n["pointsRequired"],
                        "granted": bool(n.get("preFilled")),
                        "parents": n["parents"],
                        "entries": [
                            {"name": e["name"], "icon": e["icon"], "kind": e["kind"], "ranks": e["ranks"]}
                            for e in n["entries"]
                        ],
                    }
                    for n in t["nodes"]
                ],
            }
            for t in sorted(canon_tree_records, key=lambda t: t["order"])
        ],
    }
