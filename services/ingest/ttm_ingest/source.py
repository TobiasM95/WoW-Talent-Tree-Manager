"""Fetching and caching the upstream talent payload.

Primary source is Raidbots' live export. It is the only verified source that carries
the full graph -- positions, explicit edges, gating, choice nodes and hero sub-trees.
See docs/02-target/talent-data-sources.md for why simc is not usable here (it never
parses TraitEdge) and why wago.tools DB2 CSVs are the intended fallback.

Everything downstream works off the dict returned by `load`, so adding a second
source means writing another loader that returns the same shape.
"""
from __future__ import annotations

import hashlib
import json
import os
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Any

RAIDBOTS_LIVE = "https://www.raidbots.com/static/data/live/talents.json"

# The payload is ~3.2 MB. Anything wildly outside that is a signal, not a fluke.
MIN_PLAUSIBLE_BYTES = 500_000
MAX_PLAUSIBLE_BYTES = 64_000_000


@dataclass(frozen=True)
class Payload:
    """An upstream fetch, with enough provenance to reproduce it."""

    specs: list[dict[str, Any]]
    digest: str
    fetched_at: str
    origin: str
    byte_size: int

    @property
    def short_digest(self) -> str:
        return self.digest[:12]


def _digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def _parse(raw: bytes, origin: str) -> Payload:
    if len(raw) < MIN_PLAUSIBLE_BYTES:
        raise SourceError(
            f"payload from {origin} is only {len(raw)} bytes, expected at least "
            f"{MIN_PLAUSIBLE_BYTES}. Refusing to ingest a truncated or error response."
        )
    if len(raw) > MAX_PLAUSIBLE_BYTES:
        raise SourceError(
            f"payload from {origin} is {len(raw)} bytes, far above the expected size. "
            "Refusing to ingest until this is understood."
        )
    try:
        specs = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise SourceError(f"payload from {origin} is not valid JSON: {exc}") from exc
    if not isinstance(specs, list):
        raise SourceError(
            f"payload from {origin} is a {type(specs).__name__}, expected a list of "
            "spec entries. The upstream shape has changed."
        )
    return Payload(
        specs=specs,
        digest=_digest(raw),
        fetched_at=datetime.now(timezone.utc).isoformat(timespec="seconds"),
        origin=origin,
        byte_size=len(raw),
    )


class SourceError(RuntimeError):
    """Upstream data could not be fetched or is not what we expect.

    Always fatal. The legacy pipeline's defining failure was continuing quietly when
    the source misbehaved, so nothing here degrades gracefully.
    """


def load_file(path: str) -> Payload:
    with open(path, "rb") as handle:
        return _parse(handle.read(), origin=f"file:{path}")


def load_url(url: str = RAIDBOTS_LIVE, *, timeout: int = 120, retries: int = 3) -> Payload:
    import requests  # imported lazily so offline use needs no dependency

    last: Exception | None = None
    for attempt in range(1, retries + 1):
        try:
            response = requests.get(url, timeout=timeout)
            response.raise_for_status()
            return _parse(response.content, origin=url)
        except Exception as exc:  # network flakiness is worth retrying; bad data is not
            if isinstance(exc, SourceError):
                raise
            last = exc
            if attempt < retries:
                time.sleep(2 ** attempt)
    raise SourceError(f"could not fetch {url} after {retries} attempts: {last}") from last


def load(source: str | None = None, *, cache_dir: str | None = None) -> Payload:
    """Load a payload from a path, a URL, or the default live endpoint.

    With `cache_dir`, a successful fetch is written there under its digest so a run can
    be reproduced byte for byte later. Ingest output is only as auditable as its input.
    """
    if source and os.path.exists(source):
        return load_file(source)

    payload = load_url(source or RAIDBOTS_LIVE)

    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)
        cached = os.path.join(cache_dir, f"talents-{payload.short_digest}.json")
        if not os.path.exists(cached):
            with open(cached, "w", encoding="utf-8") as handle:
                json.dump(payload.specs, handle, separators=(",", ":"))
    return payload
