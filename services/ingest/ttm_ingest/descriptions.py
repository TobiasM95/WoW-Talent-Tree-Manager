"""Talent descriptions, fetched per (spellId, definitionId, rank).

Raidbots carries spell ids, icons and names but no tooltip text, so descriptions come
from Wowhead's internal tooltip endpoint -- the same source the original pipeline used,
verified still working on 2026-09-20:

    https://nether.wowhead.com/tooltip/spell/{spellId}?def={definitionId}&rank={n}&dataEnv=1

Per-rank text works: Cosmic Rapidity reads "13% more frequently" at rank 1 and "25%" at
rank 2. Descriptions never broke; the tree structure did.

Two deliberate differences from the original:

1. **A miss is loud.** The legacy pipeline substituted "Description not available" per
   talent, which is indistinguishable from success in aggregate. Here the caller gets a
   coverage ratio and decides; `--min-coverage` fails the run below a threshold.
2. **This is a separate stage.** Tree structure does not depend on it, so a tooltip
   outage degrades text without blocking a tree update. The cache is keyed by the tuple
   above and by nothing else, so re-running costs only the genuinely new entries.

Alternative if this source ever disappears: simc ships the same text as raw
`Spell.Description_lang` templates in spelltext_data.inc, but unrendered -- "$s1",
"$?spell[a][b]", "$lsingular:plural;" -- so using it means writing an expression
evaluator over spell effect data. A last resort, not a swap. See open question Q4.
"""
from __future__ import annotations

import json
import os
import re
import time
from dataclasses import dataclass, field
from typing import Any, Iterable

TOOLTIP_URL = (
    "https://nether.wowhead.com/tooltip/spell/{spell_id}"
    "?def={definition_id}&rank={rank}&dataEnv=1"
)

# The description sits in the tooltip HTML's last <div class="q"> block. Kept tolerant:
# this marker is unversioned and could change without notice, which is precisely why
# coverage is measured rather than assumed.
_DESC_RE = re.compile(r'<div class="q">(.*?)</div>', re.S)
_COMMENT_RE = re.compile(r"<!--.*?-->", re.S)
_TAG_RE = re.compile(r"<[^>]+>")
_COLOR_RE = re.compile(r"\|c[0-9A-Fa-f]{8}|\|r", re.I)


@dataclass
class Coverage:
    requested: int = 0
    fetched: int = 0
    from_cache: int = 0
    missing: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)

    @property
    def resolved(self) -> int:
        return self.requested - len(self.missing)

    @property
    def ratio(self) -> float:
        return self.resolved / self.requested if self.requested else 1.0

    def summary(self) -> dict[str, Any]:
        return {
            "requested": self.requested,
            "resolved": self.resolved,
            "fetched": self.fetched,
            "fromCache": self.from_cache,
            "missing": len(self.missing),
            "errors": len(self.errors),
            "coverage": round(self.ratio, 4),
        }


def extract_description(tooltip_html: str) -> str | None:
    """Pull display text out of a Wowhead tooltip payload.

    Returns None rather than a placeholder when the marker is absent, so a parsing
    failure is countable instead of silently becoming content.
    """
    if not tooltip_html:
        return None
    blocks = _DESC_RE.findall(tooltip_html)
    if not blocks:
        return None
    # The last q-block is the spell body; earlier ones can be flavour or requirements.
    text = blocks[-1]
    text = _COMMENT_RE.sub("", text)
    text = text.replace("<br />", "\n").replace("<br>", "\n")
    text = _TAG_RE.sub("", text)
    text = _COLOR_RE.sub("", text)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = re.sub(r"[ \t]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text).strip()
    return text or None


def required_keys(trees: Iterable[dict[str, Any]]) -> list[tuple[int, int, int]]:
    """Every (spellId, definitionId, rank) a set of trees needs text for.

    Choice nodes need each alternative at rank 1; multi-rank nodes need every rank,
    because the numbers in the text change per rank.
    """
    keys: set[tuple[int, int, int]] = set()
    for tree in trees:
        for node in tree.get("nodes", []):
            entries = node.get("entries") or []
            max_points = node.get("maxPoints") or 1
            for entry in entries:
                spell_id = entry.get("spellId")
                definition_id = entry.get("definitionId")
                if not spell_id or not definition_id:
                    continue
                ranks = [1] if len(entries) > 1 else range(1, max_points + 1)
                for rank in ranks:
                    keys.add((spell_id, definition_id, rank))
    return sorted(keys)


class DescriptionCache:
    """A flat JSON cache keyed by "spellId:definitionId:rank".

    Text changes only when the game changes, so the cache is the difference between a
    multi-minute run and a few seconds. Written atomically; a partial run still keeps
    what it managed to fetch.
    """

    def __init__(self, path: str):
        self.path = path
        self.data: dict[str, str] = {}
        if os.path.exists(path):
            try:
                with open(path, encoding="utf-8") as handle:
                    self.data = json.load(handle)
            except (OSError, json.JSONDecodeError):
                # A corrupt cache is a performance problem, not a correctness one.
                self.data = {}

    @staticmethod
    def key(spell_id: int, definition_id: int, rank: int) -> str:
        return f"{spell_id}:{definition_id}:{rank}"

    def get(self, spell_id: int, definition_id: int, rank: int) -> str | None:
        return self.data.get(self.key(spell_id, definition_id, rank))

    def put(self, spell_id: int, definition_id: int, rank: int, text: str) -> None:
        self.data[self.key(spell_id, definition_id, rank)] = text

    def save(self) -> None:
        os.makedirs(os.path.dirname(os.path.abspath(self.path)) or ".", exist_ok=True)
        tmp = self.path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as handle:
            json.dump(self.data, handle, sort_keys=True, ensure_ascii=False)
        os.replace(tmp, self.path)


def fetch_descriptions(
    keys: list[tuple[int, int, int]],
    *,
    cache: DescriptionCache,
    delay: float = 0.05,
    timeout: int = 20,
    retries: int = 2,
    progress: Any = None,
) -> Coverage:
    """Resolve every key, using the cache first. Mutates the cache; caller saves it."""
    import requests

    coverage = Coverage(requested=len(keys))
    session = requests.Session()
    session.headers.update({
        # Identify the client honestly rather than impersonating a browser.
        "User-Agent": "WoW-Talent-Tree-Manager/2.0 (talent tooltip ingest)",
        "Accept": "application/json",
    })

    for index, (spell_id, definition_id, rank) in enumerate(keys):
        if cache.get(spell_id, definition_id, rank) is not None:
            coverage.from_cache += 1
            continue

        url = TOOLTIP_URL.format(spell_id=spell_id, definition_id=definition_id, rank=rank)
        text: str | None = None
        last_error: str | None = None
        for attempt in range(1, retries + 2):
            try:
                response = session.get(url, timeout=timeout)
                if response.status_code == 404:
                    last_error = "404"
                    break
                response.raise_for_status()
                text = extract_description(response.json().get("tooltip", ""))
                break
            except Exception as exc:  # noqa: BLE001 - network flakiness is expected
                last_error = f"{type(exc).__name__}: {exc}"
                if attempt <= retries:
                    time.sleep(0.5 * attempt)

        label = f"{spell_id}:{definition_id}:r{rank}"
        if text:
            cache.put(spell_id, definition_id, rank, text)
            coverage.fetched += 1
        else:
            coverage.missing.append(label)
            if last_error:
                coverage.errors.append(f"{label} {last_error}")

        if delay:
            time.sleep(delay)
        if progress and index % 200 == 0:
            progress(index + 1, len(keys), coverage)

    return coverage


def apply_descriptions(trees: list[dict[str, Any]], cache: DescriptionCache) -> int:
    """Fill each entry's `ranks` from the cache. Returns how many entries got text.

    An entry with no text keeps `ranks: []` -- an empty list is honest, where a
    "Description not available" string would masquerade as content.
    """
    filled = 0
    for tree in trees:
        for node in tree.get("nodes", []):
            entries = node.get("entries") or []
            max_points = node.get("maxPoints") or 1
            for entry in entries:
                spell_id = entry.get("spellId")
                definition_id = entry.get("definitionId")
                if not spell_id or not definition_id:
                    continue
                ranks = [1] if len(entries) > 1 else list(range(1, max_points + 1))
                texts = [cache.get(spell_id, definition_id, r) for r in ranks]
                resolved = [t for t in texts if t]
                if resolved:
                    entry["ranks"] = [t or "" for t in texts]
                    filled += 1
    return filled
