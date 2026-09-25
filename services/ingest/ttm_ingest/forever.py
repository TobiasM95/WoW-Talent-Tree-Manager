"""WoW Forever talent trees, from talentsforever.com's data export.

WoW Forever keeps vanilla's shape: three trees per class, one pool of 51 points shared across
them, and five points in a tree to open each next row. The export is read from the Forever
beta client's own data files (build 1.60.1.70009 at the time of writing) and published under
CC BY 4.0 -- free to reuse and adapt, with a credit and a link back, which the site shows
wherever this data is.

The output is the same tree format the retail ingest writes, so everything downstream -- the
database, the counting DP, the engine, the API and the canvas -- takes these trees unchanged:

  - a talent's row gate is its row's point requirement: (row - 1) x 5;
  - a prerequisite arrow is a parent edge, which the solver already requires at full rank;
  - an active talent is an ``active`` entry, so it draws square;
  - rank tooltips become the entry's per-rank descriptions.

**Node ids are derived, because the export has none.** Stable ids are what make share links
and saved setups survive a data update, so each talent's id is a hash of its class, tree and
name: it survives a talent moving on the grid (common while a beta is tuned), and does not
survive a rename. The alternatives -- position, or list order -- break on far more ordinary
changes. Collisions are checked, not assumed away.

The point rules are not in the export. They are vanilla's, which every source describing
Forever gives (51 points, 5 per row); they are constants here and recorded on each tree, so a
change is one edit and every consumer reads it from the data.
"""

from __future__ import annotations

import hashlib
import re
import uuid
from typing import Any

from .transform import SCHEMA_VERSION, TTM_NAMESPACE

SOURCE_URL = "https://talentsforever.com/data.json"
ATTRIBUTION = "talentsforever.com"
ATTRIBUTION_URL = "https://talentsforever.com"
LICENSE = "CC-BY-4.0"

POINT_POOL = 51
POINTS_PER_ROW = 5
# The engine enumerates a tree in one 64-bit set, one bit per rank. Counting has no such
# limit, but every tree should stay solvable end to end.
ENGINE_SLOTS = 64

# Blizzard's class ids, so a Forever tree names its class the way retail data does.
CLASS_IDS = {
    "Warrior": 1, "Paladin": 2, "Hunter": 3, "Rogue": 4, "Priest": 5,
    "Shaman": 7, "Mage": 8, "Warlock": 9, "Druid": 11,
}

# Grid pitch in the same coordinate space retail trees use, so the canvas lays both out alike.
PITCH = 600

# Derived ids live above every retail node id (which are six-digit) so the two can never meet
# in one lookup table, and below 2^31 so they fit an int column.
ID_BASE = 100_000_000
ID_SPAN = 2_000_000_000


class ForeverError(Exception):
    """The export is not shaped the way this transform relies on."""


def _slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def node_id(class_name: str, tree_name: str, talent_name: str) -> int:
    digest = hashlib.sha1(f"forever/{class_name}/{tree_name}/{talent_name}".encode()).digest()
    return ID_BASE + int.from_bytes(digest[:8], "big") % ID_SPAN


def client_build(payload: dict[str, Any]) -> str | None:
    """The newest client build the export mentions anywhere, e.g. "1.60.1.70009".

    The export does not carry it as a field; it appears in notes on individual records ("as in
    the game's files, build 1.60.1.70009"). Recorded for provenance, not relied on.
    """
    import json

    builds = re.findall(r"build\s+(\d+\.\d+\.\d+\.\d+)", json.dumps(payload))
    return max(builds, key=lambda b: int(b.rsplit(".", 1)[1])) if builds else None


def revision_for(payload: dict[str, Any]) -> int:
    """A revision from the export's date, in a range of its own: 2026-09-24 -> 1720260924.

    Revisions are one sequence across games, so Forever's sit well clear of retail's (six
    digits). The export's own `generated` date orders them, so a newer export always promotes
    over an older one, and re-reading the same export updates its revision in place.
    """
    generated = str(payload.get("generated") or "")
    if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", generated):
        raise ForeverError(f"the export's generated date is {generated!r}, expected YYYY-MM-DD")
    return 1_700_000_000 + int(generated.replace("-", ""))


def _need(obj: dict[str, Any], field: str, where: str) -> Any:
    if field not in obj or obj[field] is None:
        raise ForeverError(f"{where}: missing {field!r}")
    return obj[field]


def transform(payload: dict[str, Any], *, source: dict[str, Any]) -> tuple[list[dict[str, Any]], list[str]]:
    """Every class's three trees, in the ingest's tree format, and any warnings."""
    if payload.get("license") != LICENSE:
        # Reusing the data rests on the licence; if it changes, a person has to look.
        raise ForeverError(f"licence is {payload.get('license')!r}, expected {LICENSE}")
    classes = _need(payload, "talents", "export")
    if not isinstance(classes, dict) or not classes:
        raise ForeverError("export has no classes")

    trees: list[dict[str, Any]] = []
    warnings: list[str] = []
    seen_ids: dict[int, str] = {}

    for class_name, cls in classes.items():
        if class_name not in CLASS_IDS:
            raise ForeverError(f"unknown class {class_name!r}")
        tabs = _need(cls, "trees", class_name)
        if len(tabs) != 3:
            raise ForeverError(f"{class_name} has {len(tabs)} trees, expected 3")

        for order, tab in enumerate(tabs):
            tree_name = _need(tab, "name", class_name)
            label = f"{class_name}/{tree_name}"
            raw = _need(tab, "talents", label)
            if not raw:
                raise ForeverError(f"{label} has no talents")

            by_name: dict[str, dict[str, Any]] = {}
            cells: dict[tuple[int, int], str] = {}
            nodes: list[dict[str, Any]] = []
            for t in raw:
                name = _need(t, "name", label)
                where = f"{label}/{name}"
                row, col, max_rank = _need(t, "row", where), _need(t, "col", where), _need(t, "max", where)
                if not (isinstance(row, int) and 1 <= row <= 11 and isinstance(col, int) and 1 <= col <= 4):
                    raise ForeverError(f"{where}: grid position {row},{col} is off the tree")
                if not (isinstance(max_rank, int) and 1 <= max_rank <= 9):
                    raise ForeverError(f"{where}: max rank {max_rank!r}")
                if (row, col) in cells:
                    raise ForeverError(f"{where}: shares its cell with {cells[(row, col)]}")
                if name in by_name:
                    raise ForeverError(f"{where}: talent name appears twice in the tree")
                cells[(row, col)] = name

                nid = node_id(class_name, tree_name, name)
                if nid in seen_ids:
                    raise ForeverError(f"{where}: derived id collides with {seen_ids[nid]}")
                seen_ids[nid] = where

                desc = t.get("desc") or []
                node = {
                    "nodeId": nid,
                    "localId": None,
                    "kind": "single",
                    "name": name,
                    "maxPoints": max_rank,
                    "rankLevels": None,
                    "pointsRequired": (row - 1) * POINTS_PER_ROW,
                    "preFilled": False,
                    "freeLevel": None,
                    "entryNode": False,  # decided below, once prerequisites are known
                    "row": row - 1,
                    "col": col - 1,
                    "pos": {"x": (col - 1) * PITCH, "y": (row - 1) * PITCH},
                    "parents": [],
                    "children": [],
                    "requiresNode": None,
                    "subTreeId": None,
                    "entries": [{
                        "entryId": nid,
                        "definitionId": None,
                        "spellId": None,
                        "visibleSpellId": None,
                        "name": name,
                        "kind": "passive" if t.get("passive", True) else "active",
                        "icon": t.get("icon"),
                        "index": None,
                        "maxRanks": max_rank,
                        "ranks": [str(d) for d in desc][:max_rank],
                    }],
                }
                by_name[name] = node
                nodes.append((node, t.get("req")))

            # Prerequisite arrows name their parent; resolve within the tree.
            for node, req in nodes:
                if not req:
                    continue
                parent = by_name.get(req)
                if parent is None:
                    raise ForeverError(f"{label}/{node['name']}: requires {req!r}, not in the tree")
                # Above, or beside: vanilla trees have sideways arrows (Holy Shock -> Divine
                # Precision, Mind Flay -> Improved Mind Flay). Only a prerequisite *below* a
                # talent is impossible, since its row would open later than the talent's.
                if parent["row"] > node["row"]:
                    raise ForeverError(f"{label}/{node['name']}: requires {req!r}, which is below it")
                if node["nodeId"] in parent["parents"]:
                    raise ForeverError(f"{label}/{node['name']}: requires {req!r}, which requires it back")
                node["parents"].append(parent["nodeId"])
                parent["children"].append(node["nodeId"])

            flat = [n for n, _ in nodes]
            for n in flat:
                n["entryNode"] = n["row"] == 0 and not n["parents"]
            flat.sort(key=lambda n: (n["row"], n["col"]))

            slots = sum(n["maxPoints"] for n in flat)
            if slots > ENGINE_SLOTS:
                warnings.append(f"{label}: {slots} ranks exceed the engine's {ENGINE_SLOTS}; counting works, listing will not")

            key = f"forever/{_slug(class_name)}/{_slug(tree_name)}"
            trees.append({
                "schemaVersion": SCHEMA_VERSION,
                "id": str(uuid.uuid5(TTM_NAMESPACE, key)),
                "key": key,
                "kind": "tab",
                "game": "forever",
                "name": tree_name,
                "description": "",
                "classId": CLASS_IDS[class_name],
                "className": class_name,
                "specId": None,
                "specName": None,
                "traitTreeId": None,
                "subTreeId": None,
                "gating": "reqPoints",
                # One tree can hold at most the whole pool, and at most what it has ranks for.
                "pointCap": min(POINT_POOL, slots),
                "maxPointsInTree": slots,
                "nodeCount": len(flat),
                # The rules the source does not state, recorded where every consumer reads them.
                "sharedPointCap": POINT_POOL,
                "pointsPerRow": POINTS_PER_ROW,
                "order": order,
                "icon": tab.get("icon"),
                "attribution": {"name": ATTRIBUTION, "url": ATTRIBUTION_URL, "license": LICENSE},
                "fullNodeOrder": None,
                "subTreeSelector": None,
                "source": source,
                "nodes": flat,
            })

    return trees, warnings
