"""Talent icon fetching.

Icons arrive as *names* in the talent payload ("ability_demonhunter_feldevastation"),
never as images. Turning a name into pixels means asking a CDN, and this module is the
only place that knows how.

Two decisions worth stating:

**Blizzard's own render service is the default source**, not a third-party aggregator.
`render.worldofwarcraft.com` is first-party, has no terms of service of its own to fall
foul of, and serves exactly the sizes the game uses. The trade-off is that it offers only
18/36/56 pixels -- 64 and up are 403 -- so 56 is the largest icon this service can have.
That is also the size every comparable tool uses, and a canvas scales it with CSS.

**One file per name, never a packed atlas.** The native app shipped an atlas. For a web
client that is the wrong shape: it cannot be cached per icon, it has to be regenerated
whenever any single icon changes, and drawing one tree means downloading all of them.

Nothing here writes into the repository. See docs/02-target/icons.md.
"""
from __future__ import annotations

import hashlib
import re
import urllib.error
import urllib.request
from typing import Any, Iterable

# Blizzard's own icon renderer. Overridable, because a deployment may want a mirror, an
# internal cache, or no icon source at all.
DEFAULT_BASE_URL = "https://render.worldofwarcraft.com/us/icons"

# The only sizes the default source serves; 64 and above answer 403.
SIZES = (18, 36, 56)
DEFAULT_SIZE = 56

# Upstream paths are built from these names, so the alphabet is enforced rather than
# trusted: a name with a slash or a dot in it would address something else entirely.
NAME_RE = re.compile(r"^[a-z0-9_]{2,128}$")

# A 200 response shorter than this is an error page, not an icon. The smallest real icon
# observed at 18px is just under 1 KB; 256 bytes is comfortably below anything genuine
# and comfortably above a stub.
MIN_BYTES = 256
MAX_BYTES = 262_144

USER_AGENT = (
    "ttm-icon-sync/1.0 (WoW Talent Tree Manager; "
    "https://github.com/TobiasM95/WoW-Talent-Tree-Manager)"
)


# Upstream mostly gives bare names, but not always: one entry in the live payload carries
# `ability_druid_mangle.tga`, a raw asset filename. The icon behind it exists, so the
# extension is stripped rather than treated as a broken name.
ASSET_EXTENSIONS = (".tga", ".blp", ".png", ".jpg", ".jpeg")


class IconError(RuntimeError):
    """The icon could not be fetched in a way worth caching as a success."""


class IconNameError(IconError):
    """The name itself is unusable, so no amount of retrying will help.

    Separate from IconError because the caller reports them differently: a network failure
    says "re-run to retry" and a malformed name would make that advice a lie.
    """


def icon_names(definition: dict[str, Any]) -> set[str]:
    """Every icon name a tree definition refers to.

    Icons live on entries, not nodes: a choice node has two entries with two different
    icons, and rendering it needs both.
    """
    names: set[str] = set()
    for node in definition.get("nodes", []):
        for entry in node.get("entries", []):
            name = entry.get("icon")
            if not name:
                continue
            try:
                names.add(normalise(name))
            except IconNameError:
                # A name nothing can be done with is not worth failing an ingest over;
                # the tree still renders, just without that one icon.
                continue
    return names


def normalise(name: str) -> str:
    """Lowercase, strip any asset extension, and validate. Raises IconNameError.

    Validation happens here rather than at the HTTP boundary so that a bad name from the
    transform is caught during a sync, not when a browser asks for it. The same function
    is used by the ingest and by the API, so a name normalises to the same thing on the
    way in and on the way out -- otherwise a lookup would miss its own cache row.
    """
    candidate = str(name).strip().lower()
    for extension in ASSET_EXTENSIONS:
        if candidate.endswith(extension):
            candidate = candidate[: -len(extension)]
            break
    if not NAME_RE.match(candidate):
        raise IconNameError(f"not a usable icon name: {name!r}")
    return candidate


def icon_url(name: str, size: int = DEFAULT_SIZE, base_url: str = DEFAULT_BASE_URL) -> str:
    if size not in SIZES:
        raise IconError(f"size {size} is not one of {SIZES}")
    return f"{base_url.rstrip('/')}/{size}/{normalise(name)}.jpg"


def etag_for(payload: bytes) -> str:
    """A content hash, so the API can answer a conditional GET without the bytes.

    Truncated: 16 hex characters is 64 bits, which is far more than enough to distinguish
    a few thousand images and keeps the header short.
    """
    return hashlib.sha256(payload).hexdigest()[:16]


def fetch(name: str, size: int = DEFAULT_SIZE, *, base_url: str = DEFAULT_BASE_URL,
          timeout: float = 20.0, opener=None) -> dict[str, Any]:
    """Fetch one icon. Returns a row ready to upsert; never raises on a 404.

    A missing icon is data, not an error: upstream genuinely does not have images for
    every name the talent payload uses, and a row recording that is what stops the sync
    asking again on every run. Network failures *do* raise, because those are worth
    retrying and must not be cached as absence.
    """
    url = icon_url(name, size, base_url)
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    fetcher = opener.open if opener is not None else urllib.request.urlopen

    try:
        with fetcher(request, timeout=timeout) as response:
            payload = response.read(MAX_BYTES + 1)
            content_type = response.headers.get("Content-Type", "")
            status = getattr(response, "status", 200) or 200
    except urllib.error.HTTPError as exc:
        # 4xx is an answer: this name has no image. 5xx is upstream having a bad day and
        # must not be recorded as absence, or a blip would blank icons until someone
        # noticed and cleared the cache by hand.
        if 500 <= exc.code:
            raise IconError(f"{url}: upstream {exc.code}") from exc
        return {"name": normalise(name), "size": size, "status": exc.code,
                "content_type": None, "bytes": None, "etag": None, "source": url}
    except urllib.error.URLError as exc:
        raise IconError(f"{url}: {exc.reason}") from exc

    if status != 200:
        return {"name": normalise(name), "size": size, "status": status,
                "content_type": None, "bytes": None, "etag": None, "source": url}
    if not content_type.startswith("image/"):
        raise IconError(f"{url}: expected an image, got {content_type!r}")
    if not (MIN_BYTES <= len(payload) <= MAX_BYTES):
        raise IconError(f"{url}: {len(payload)} bytes is not a plausible icon")

    return {
        "name": normalise(name),
        "size": size,
        "status": 200,
        "content_type": content_type.split(";")[0].strip(),
        "bytes": payload,
        "etag": etag_for(payload),
        "source": url,
    }


def fetch_many(names: Iterable[str], size: int = DEFAULT_SIZE, *,
               base_url: str = DEFAULT_BASE_URL, workers: int = 8,
               timeout: float = 20.0, on_result=None,
               opener=None) -> tuple[list[dict], list[str], list[str]]:
    """Fetch many icons concurrently. Returns (rows, retryable failures, bad names).

    Three buckets, not two: a name that can never resolve must not be reported as
    something a re-run would fix.

    Concurrency is modest on purpose. This is someone else's CDN being asked for a couple
    of thousand small files; eight at a time finishes in well under a minute and is not a
    load anyone would notice.
    """
    from concurrent.futures import ThreadPoolExecutor

    rows: list[dict] = []
    failed: list[str] = []
    unusable: list[str] = []
    ordered = sorted(set(names))

    def one(name: str):
        try:
            return name, fetch(name, size, base_url=base_url, timeout=timeout,
                               opener=opener), None
        except IconNameError as exc:
            return name, None, exc
        except IconError as exc:
            return name, None, exc

    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        for name, row, error in pool.map(one, ordered):
            if row is not None:
                rows.append(row)
            elif isinstance(error, IconNameError):
                unusable.append(name)
            else:
                failed.append(name)
            if on_result is not None:
                on_result(name, row, error)
    return rows, failed, unusable
