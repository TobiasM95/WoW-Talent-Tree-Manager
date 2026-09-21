#!/usr/bin/env python3
"""Fill the icon cache for every icon the promoted trees refer to.

    python services/db/sync_icons.py [--size 56] [--retry-misses] [--limit N]
    docker compose run --rm sync-icons

Idempotent and resumable: it asks Postgres which names have no usable row and fetches only
those. Running it twice does nothing the second time, which matters because the first run
asks someone else's CDN for a couple of thousand files.

Not part of `load_trees.py` on purpose. Icons are keyed by name, not by tree revision, so
a new ingest almost never invalidates them -- the overlap between two revisions is close to
total. Coupling them would mean re-fetching everything on every ingest for no reason, and
it would make a CDN outage able to block a tree update.

The cache is derived data. `TRUNCATE icons` is always safe.
"""
from __future__ import annotations

import argparse
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for path in (os.path.join(REPO, "services", "ingest"), os.path.join("services", "ingest")):
    sys.path.insert(0, path)

from ttm_ingest import icons as icon_source  # noqa: E402

# A miss is cached so the sync stops asking, but not forever: upstream does add art.
RETRY_MISSES_AFTER_DAYS = 30


REFERENCED_SQL = """
    SELECT DISTINCT e->>'icon' AS name
    FROM current_trees t,
         jsonb_array_elements(t.definition->'nodes') n,
         jsonb_array_elements(n->'entries') e
    WHERE e->>'icon' IS NOT NULL
"""


def referenced(conn) -> set[str]:
    """Every normalised icon name a promoted tree refers to.

    The normalisation runs in Python, not SQL, so there is exactly one implementation of
    it. Duplicating the rule in a regex here would let the two drift, and a name that
    normalised differently on the way in and the way out would miss its own cache row.
    The set is about two thousand entries, so there is nothing to gain by pushing it down.
    """
    with conn.cursor() as cur:
        cur.execute(REFERENCED_SQL)
        raw = [row[0] for row in cur.fetchall()]

    names: set[str] = set()
    for value in raw:
        try:
            names.add(icon_source.normalise(value))
        except icon_source.IconNameError:
            # Nothing can be fetched for this; the tree still renders without the icon.
            pass
    return names


def cached(conn, size: int, *, retry_misses: bool) -> set[str]:
    """Names that already have a row worth keeping at this size."""
    with conn.cursor() as cur:
        cur.execute(
            """
            SELECT name FROM icons
            WHERE size = %s
              AND (status = 200
                   OR (NOT %s AND fetched_at >= now() - %s::interval))
            """,
            (size, retry_misses, f"{RETRY_MISSES_AFTER_DAYS} days"))
        return {row[0] for row in cur.fetchall()}


def wanted(conn, size: int, *, retry_misses: bool) -> list[str]:
    """Referenced names with no usable cache row, oldest-first by nothing in particular."""
    return sorted(referenced(conn) - cached(conn, size, retry_misses=retry_misses))


def upsert(conn, rows: list[dict]) -> None:
    """Store fetched icons. Bumps `attempts` so a name that keeps missing is visible."""
    if not rows:
        return
    with conn.cursor() as cur:
        cur.executemany(
            """
            INSERT INTO icons (name, size, status, content_type, bytes, etag, source)
            VALUES (%(name)s, %(size)s, %(status)s, %(content_type)s, %(bytes)s,
                    %(etag)s, %(source)s)
            ON CONFLICT (name, size) DO UPDATE SET
                status = EXCLUDED.status, content_type = EXCLUDED.content_type,
                bytes = EXCLUDED.bytes, etag = EXCLUDED.etag,
                source = EXCLUDED.source, fetched_at = now(),
                attempts = icons.attempts + 1
            """, rows)
    conn.commit()


def coverage(conn, size: int) -> tuple[int, int]:
    """(usable icons, names referenced). What /health reports."""
    want = referenced(conn)
    with conn.cursor() as cur:
        cur.execute("SELECT name FROM icons WHERE size = %s AND status = 200", (size,))
        have = {row[0] for row in cur.fetchall()}
    return len(want & have), len(want)


def main() -> int:
    parser = argparse.ArgumentParser(description="Fill the talent icon cache.")
    parser.add_argument("--database-url", default=os.environ.get("DATABASE_URL"))
    parser.add_argument("--base-url",
                        default=os.environ.get("TTM_ICON_BASE_URL",
                                               icon_source.DEFAULT_BASE_URL))
    parser.add_argument("--size", type=int, default=icon_source.DEFAULT_SIZE,
                        choices=icon_source.SIZES)
    parser.add_argument("--workers", type=int, default=8)
    parser.add_argument("--limit", type=int, default=0,
                        help="fetch at most N (for a smoke run)")
    parser.add_argument("--retry-misses", action="store_true",
                        help="re-ask for names upstream previously did not have")
    parser.add_argument("--batch", type=int, default=200,
                        help="commit every N icons, so an interrupted run keeps its work")
    args = parser.parse_args()

    if not args.database_url:
        print("FATAL: no database URL. Pass --database-url or set DATABASE_URL.",
              file=sys.stderr)
        return 2

    import psycopg

    with psycopg.connect(args.database_url) as conn:
        todo = wanted(conn, args.size, retry_misses=args.retry_misses)
        if args.limit:
            todo = todo[:args.limit]
        have, want = coverage(conn, args.size)
        print(f"icons: {have}/{want} cached at {args.size}px, {len(todo)} to fetch")
        if not todo:
            return 0
        print(f"source: {args.base_url}")

        started = time.monotonic()
        stored = misses = failed = unusable = 0
        for offset in range(0, len(todo), args.batch):
            chunk = todo[offset:offset + args.batch]
            rows, bad, bad_names = icon_source.fetch_many(
                chunk, args.size, base_url=args.base_url, workers=args.workers)
            upsert(conn, rows)
            stored += sum(1 for r in rows if r["status"] == 200)
            misses += sum(1 for r in rows if r["status"] != 200)
            failed += len(bad)
            unusable += len(bad_names)
            print(f"  {min(offset + len(chunk), len(todo))}/{len(todo)}"
                  f"  stored={stored} missing-upstream={misses} failed={failed}",
                  flush=True)

        have, want = coverage(conn, args.size)
        elapsed = time.monotonic() - started
        print(f"done in {elapsed:.1f}s: {have}/{want} cached"
              + (f" ({have / want:.1%})" if want else ""))
        if unusable:
            # Reported separately from network failures: re-running cannot fix a name.
            print(f"{unusable} name(s) are not fetchable at all", file=sys.stderr)
        if failed:
            # Network failures are not recorded as absence, so they are simply still
            # missing. Re-running picks them up.
            print(f"{failed} fetch(es) failed (network, not upstream 4xx); "
                  "re-run to retry", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
