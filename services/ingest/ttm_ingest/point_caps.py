"""How many talent points a tree actually allows.

Raidbots carries the tree graph but nothing about point budgets, so `pointCap` was null
and the UI had no way to say "3 points left". The budget is level-based -- points are
granted at character levels -- and that grant table lives in DB2, not in the raidbots
payload:

    TraitTreeXTraitCurrency   trait tree -> its currencies, ordered by _Index
    TraitCurrencySource       currency   -> amount granted at a player level
    TraitCurrency             currency   -> type (2 = trait points)

Summing every grant at or below a level cap gives the cap for that currency. Observed at
build 12.1.0.69875, consistently for every class:

    _Index 1  currency 2801   31 @ level 80,  34 @ level 90   class tree
    _Index 2  currency 2800   30 @ level 80,  34 @ level 90   spec tree
    _Index 3+ currency 2986+  10 @ level 80,  13 @ level 90   hero sub-trees

The currency ids are global rather than per-class, which is what makes this a small table
rather than a per-tree lookup.

Two independent checks say the derivation is right, and both are enforced below:

- A cap must exceed the tree's highest `reqPoints` gate, or gated nodes are unreachable.
  Class gates reach 23 and spec gates 20; both caps are 34.
- A hero tree has 14 max points including exactly one free (preFilled) entry node, so 13
  are spendable -- exactly the currency amount.

This is the wago.tools DB2 route already documented as the long-term fallback source; here
it earns its place for a field raidbots simply does not carry.
"""
from __future__ import annotations

import csv
import io
import os
from collections import defaultdict
from typing import Any

WAGO_CSV = "https://wago.tools/db2/{table}/csv"

TABLES = ("TraitTreeXTraitCurrency", "TraitCurrencySource", "TraitCurrency")

# TraitCurrency.Type for talent points, as opposed to profession knowledge and the like.
TRAIT_POINT_TYPE = "2"

# Position within a trait tree's currency list.
INDEX_CLASS = 1
INDEX_SPEC = 2


class PointCapError(RuntimeError):
    """Caps could not be derived, or contradict the tree data."""


def _fetch_csv(table: str, build: str | None, cache_dir: str | None) -> list[dict[str, str]]:
    cached = None
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)
        cached = os.path.join(cache_dir, f"{table}-{build or 'live'}.csv")
        if os.path.exists(cached):
            with open(cached, encoding="utf-8") as handle:
                return list(csv.DictReader(handle))

    import requests

    url = WAGO_CSV.format(table=table)
    params = {"build": build} if build else {}
    response = requests.get(url, params=params, timeout=120,
                            headers={"User-Agent": "WoW-Talent-Tree-Manager/2.0"})
    response.raise_for_status()
    text = response.text
    if not text.strip() or "," not in text.splitlines()[0]:
        raise PointCapError(f"{table} did not return CSV; wago.tools may have changed")
    if cached:
        with open(cached, "w", encoding="utf-8", newline="") as handle:
            handle.write(text)
    return list(csv.DictReader(io.StringIO(text)))


def derive_caps(level_cap: int, *, trait_tree_ids: set[int], build: str | None = None,
                cache_dir: str | None = None) -> dict[str, int]:
    """Return {"class": n, "spec": n, "hero": n} for a level cap.

    `trait_tree_ids` scopes this to the trees the payload actually describes. DB2 holds a
    trait tree for professions, dragonriding and more, all of which have an _Index 1
    currency with unrelated amounts -- without this scoping they drown the talent values.
    """
    tree_currency = _fetch_csv("TraitTreeXTraitCurrency", build, cache_dir)
    sources = _fetch_csv("TraitCurrencySource", build, cache_dir)
    currencies = _fetch_csv("TraitCurrency", build, cache_dir)

    point_currencies = {
        int(r["ID"]) for r in currencies if r.get("Type") == TRAIT_POINT_TYPE
    }

    granted: dict[int, int] = defaultdict(int)
    for row in sources:
        try:
            cid, level, amount = (int(row["TraitCurrencyID"]), int(row["PlayerLevel"]),
                                  int(row["Amount"]))
        except (KeyError, ValueError) as exc:
            raise PointCapError(f"unreadable TraitCurrencySource row: {row}") from exc
        if level <= level_cap:
            granted[cid] += amount

    # Group each trait tree's currencies by their position, then take the value every tree
    # agrees on. Disagreement would mean the convention is not what we think it is.
    by_slot: dict[str, set[int]] = defaultdict(set)
    for row in tree_currency:
        try:
            index, cid = int(row["_Index"]), int(row["TraitCurrencyID"])
        except (KeyError, ValueError):
            continue
        if cid not in point_currencies:
            continue
        if int(row["TraitTreeID"]) not in trait_tree_ids:
            continue
        slot = ("class" if index == INDEX_CLASS
                else "spec" if index == INDEX_SPEC
                else "hero")
        by_slot[slot].add(granted.get(cid, 0))

    caps: dict[str, int] = {}
    for slot in ("class", "spec", "hero"):
        values = {v for v in by_slot.get(slot, set()) if v > 0}
        if not values:
            raise PointCapError(
                f"no talent-point currency found for {slot} trees at level {level_cap}"
            )
        if len(values) > 1:
            raise PointCapError(
                f"{slot} trees disagree on their point cap at level {level_cap}: "
                f"{sorted(values)}. The currency layout has changed and the mapping in "
                "point_caps.py needs revisiting rather than guessing."
            )
        caps[slot] = values.pop()
    return caps


def apply_caps(trees: list[dict[str, Any]], caps: dict[str, int]) -> list[str]:
    """Set pointCap on each tree, refusing values the tree itself contradicts.

    Returns warnings. Raises when a cap is impossible, because a wrong cap is worse than
    an absent one: it would make the UI reject legal builds.
    """
    warnings: list[str] = []
    for tree in trees:
        cap = caps.get(tree["kind"])
        if cap is None:
            continue

        gates = [n["pointsRequired"] for n in tree["nodes"] if n["pointsRequired"]]
        highest_gate = max(gates) if gates else 0
        free_points = sum(n["maxPoints"] or 0 for n in tree["nodes"] if n["preFilled"])
        spendable = tree["maxPointsInTree"] - free_points

        if cap <= highest_gate:
            raise PointCapError(
                f"{tree['key']}: cap {cap} does not exceed its highest gate "
                f"({highest_gate}), so gated talents would be unreachable"
            )
        if cap > spendable:
            raise PointCapError(
                f"{tree['key']}: cap {cap} exceeds the {spendable} points the tree can "
                f"absorb ({tree['maxPointsInTree']} max ranks, {free_points} granted free)"
            )
        if tree["kind"] == "hero" and cap != spendable:
            # Hero trees are meant to be fully spent; a mismatch is worth seeing.
            warnings.append(
                f"{tree['key']}: cap {cap} but {spendable} spendable points"
            )
        tree["pointCap"] = cap
    return warnings
