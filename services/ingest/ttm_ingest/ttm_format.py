"""Tree JSON -> TTM tree string.

The engine reads its bespoke `;`/`:`/`,` delimited format, and the ingest emits JSON, so
something has to bridge them. This is that bridge: it is what lets the solver run on
current-patch data, and what lets the DP be cross-checked against the engine.

The format is defined by `createTreeStringRepresentation` in Engine/src/TalentTrees.cpp;
the grammar is documented in docs/01-current-state/data-formats.md. Written to match the
C++ writer field for field:

    header:  Ver:presetName:treeType:name:treeDesc:loadoutDesc:numTalents:numSkillsets;
    talent:  idx:name[,switchName]:desc[,desc..]:type:row:col:maxPts:reqPts:preFilled:
             parents:children:icon,iconSwitch;

Note the C++ writer emits no trailing nodeID field, so Blizzard node ids are lost the
moment a tree round-trips through it. We keep our own JSON as the source of truth and
treat this format as a transport to the engine, never as storage.
"""
from __future__ import annotations

from typing import Any

TTM_VERSION = "1.4.2"  # Presets::TTM_VERSION

# TalentType in Engine/src/TalentTrees.h
TYPE_ACTIVE = 0
TYPE_PASSIVE = 1
TYPE_SWITCH = 2


class ConversionError(RuntimeError):
    """The tree cannot be expressed in the engine's format."""


def clean(text: str) -> str:
    """Escape the delimiters, matching Engine cleanString().

    Order matters only in that none of the replacements can introduce another
    delimiter, which holds because the markers are alphabetic.
    """
    return (
        (text or "")
        .replace(":", "__cl__")
        .replace("\n", "__n__")
        .replace(",", "__cm__")
        .replace(";", "__sc__")
    )


# The engine's validateTalentStringFormat restricts the name field to exactly this
# alphabet and rejects the whole tree otherwise -- silently, by skipping the line. Four
# hero trees were dropped that way before this existed, because live talent names include
# "Stampede!" and "Ride or Die!" and '!' is not in the set.
#
# Comma is deliberately excluded even though the validator permits it: inside the name
# field a comma separates a choice node's two alternatives, so a comma within a single
# name would be parsed as a second name.
_NAME_ALPHABET = set(
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789 _/()'-"
)


def sanitize_name(text: str) -> str:
    """Make a talent name safe for the engine's name field.

    Lossy on purpose. This format is a transport to the solver, not storage -- the real
    name lives in the tree JSON. Substituting rather than deleting keeps distinct names
    distinct, which matters because the engine identifies choice alternatives by name.
    """
    cleaned = clean(text or "")
    return "".join(c if c in _NAME_ALPHABET else "_" for c in cleaned) or "_"


def _talent_type(node: dict[str, Any]) -> int:
    """Map a node onto the engine's three talent types.

    `choice` becomes SWITCH, which is what the engine models: two alternatives in one
    slot. `tiered` has no equivalent -- the engine cannot express ranks whose
    availability depends on character level -- so it is carried as a plain multi-rank
    passive. That is structurally right for counting (a node taking 1..maxPoints points,
    gated by reqPoints) and drops only the level gating. See open question Q10.
    """
    kind = node.get("kind")
    if kind == "choice":
        return TYPE_SWITCH
    entries = node.get("entries") or []
    first = entries[0].get("kind") if entries else None
    return TYPE_ACTIVE if first == "active" else TYPE_PASSIVE


def _names(node: dict[str, Any], talent_type: int) -> tuple[str, str | None]:
    entries = node.get("entries") or []
    primary = (entries[0].get("name") if entries else "") or node.get("name") or "?"
    if talent_type == TYPE_SWITCH:
        if len(entries) < 2:
            raise ConversionError(
                f"node {node['nodeId']} is a choice node with {len(entries)} entries; "
                "the engine's SWITCH type needs exactly two alternatives"
            )
        return primary, (entries[1].get("name") or "?")
    return primary, None


def tree_to_structure_line(tree: dict[str, Any], *, level_cap: int | None = None) -> str:
    """Render one tree as a single TTM tree-string line.

    `preset` is always "custom" so the engine parses the talent records that follow;
    named presets are looked up from its own bundled table instead.

    `level_cap` resolves tiered nodes' rankLevels down to a concrete maxPoints. Without
    it, a tiered node contributes its full rank count, which overstates what a character
    below max level could actually spend.
    """
    nodes = tree.get("nodes") or []
    if not nodes:
        raise ConversionError(f"{tree.get('key')} has no nodes")

    # Engine talent indices are a dense 0..N-1 space and parents/children reference them,
    # so fix an order once and use it for everything. Node order is already stable from
    # the ingest (row, col, nodeId).
    index_of = {n["nodeId"]: i for i, n in enumerate(nodes)}

    records = []
    for node in nodes:
        talent_type = _talent_type(node)
        name, switch_name = _names(node, talent_type)

        max_points = _resolve_max_points(node, level_cap)
        if max_points < 1:
            raise ConversionError(
                f"node {node['nodeId']} in {tree.get('key')} resolved to {max_points} "
                "max points"
            )

        # The C++ writer indexes descriptions[size-1] unconditionally, so an empty list
        # would read out of bounds. We have no tooltip text (see Q4), so emit one entry
        # per rank using the talent name -- enough to keep the parser happy and to make
        # a dumped tree readable.
        description_count = 2 if talent_type == TYPE_SWITCH else max_points
        descriptions = ",".join(sanitize_name(name) for _ in range(description_count))

        parents = ",".join(str(index_of[p]) for p in node["parents"] if p in index_of)
        children = ",".join(str(index_of[c]) for c in node["children"] if c in index_of)

        field_name = sanitize_name(name)
        if switch_name is not None:
            field_name += "," + sanitize_name(switch_name)

        records.append(
            ":".join([
                str(index_of[node["nodeId"]]),
                field_name,
                descriptions,
                str(talent_type),
                str(node["row"] + 1),       # engine rows/cols are 1-based
                str(node["col"] + 1),
                str(max_points),
                str(node["pointsRequired"]),
                "1" if node["preFilled"] else "0",
                parents,
                children,
                "default.png,default",
            ])
        )

    header = ":".join([
        TTM_VERSION,
        "custom",
        "0",                                  # TreeType::CLASS; unused by the solver
        sanitize_name(tree.get("name") or tree.get("key") or "tree"),
        clean(tree.get("description") or ""),
        "",                                   # loadout description
        str(len(nodes)),
        "0",                                  # no skillsets
    ])

    return header + ";" + "".join(r + ";" for r in records)


def _resolve_max_points(node: dict[str, Any], level_cap: int | None) -> int:
    """Max ranks for a node, resolved against a level cap where it matters.

    Tiered nodes carry rankLevels such as [{level:81,maxRanks:1},{level:90,maxRanks:4}]:
    max ranks is a function of character level, which the engine's static maxPoints
    cannot express. Resolving it here keeps that knowledge out of the solver, at the cost
    of the level cap becoming part of the solve's input.
    """
    declared = node.get("maxPoints") or 1
    rank_levels = node.get("rankLevels")
    if not rank_levels or level_cap is None:
        return declared

    allowed = 0
    for step in rank_levels:
        if level_cap >= step["level"]:
            allowed = max(allowed, step["maxRanks"])
    # Below the first threshold the node is unavailable; the engine has no way to say
    # "zero ranks", so clamp to one and let the gate decide.
    return max(1, min(declared, allowed)) if allowed else 1


def trees_to_structure_file(trees: list[dict[str, Any]], *, level_cap: int | None = None) -> str:
    """Render several trees as a structure file, one line each.

    Line order is the caller's; `--structure-indices` selects by line number.
    """
    return "".join(tree_to_structure_line(t, level_cap=level_cap) + "\n" for t in trees)
