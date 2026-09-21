#!/usr/bin/env python3
"""Load ingested tree JSON into Postgres and precompute build counts.

    python services/db/load_trees.py [--trees data/generated] [--level-cap 90]

Two jobs in one pass:

1. Insert the revision and its trees, then mark the revision promoted. A revision that
   fails partway is never promoted, so `current_trees` keeps serving the previous one --
   the same staged-then-swapped discipline the ingest uses on disk.

2. Precompute, for every tree and every point budget, both counts:

   - `set_count`  -- distinct selections, choice-node sides unresolved. One row of
     enumerator output per set.
   - `build_count` -- distinct builds, sides resolved: sum over sets of 2^(choice nodes).
     The user-facing number.

   The frontier DP does both in milliseconds and the answers are fixed for a tree
   revision, so computing them here is what turns the count into a free pre-flight gate at
   request time instead of a job. See docs/02-target/architecture.md.

Counts are stored as numeric, not bigint: nine class trees exceed 2^31 (shaman_class_
elemental reaches 37,296,642,700), which is exactly what overflowed the engine's 32-bit
counter.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tools", "frontier-dp"))
sys.path.insert(0, os.path.join("tools", "frontier-dp"))

DEFAULT_LEVEL_CAP = 90


def load_dp():
    """Import the frontier DP, which lives in tools/ as a verified reference."""
    from frontier_dp import (  # noqa: PLC0415
        build_expanded_graph, count_frontier_dp, load_tree_json, topo_sort,
    )
    return load_tree_json, build_expanded_graph, topo_sort, count_frontier_dp


# How much worse than what is already being served a revision may be before promotion is
# refused. Coverage moves a little between ingests as upstream adds spells faster than
# tooltips; a wholesale collapse is a different thing entirely.
COVERAGE_TOLERANCE = 0.02


def _currently_serving(cur) -> tuple[int, float] | None:
    """(revision, description coverage) of the revision being served right now, if any."""
    cur.execute(
        """
        SELECT revision, description_coverage FROM ingest_runs
        WHERE promoted_at IS NOT NULL AND description_coverage IS NOT NULL
        ORDER BY promoted_at DESC LIMIT 1
        """)
    row = cur.fetchone()
    return (int(row[0]), float(row[1])) if row else None


def _coverage_regression(serving, new_coverage) -> str | None:
    """Describe a description-coverage regression against what is being served, or None.

    This exists because of a mistake that is very easy to make: running the ingest without
    `--descriptions` produces a perfectly valid revision in which every entry has empty
    text, and promoting it silently replaces a fully-described dataset with a blank one.
    Nothing downstream would complain -- the trees are correct, the counts are correct, and
    only a user reading a talent would notice.

    That is the exact failure mode that killed the original pipeline: not a crash, but quiet
    degradation that nothing was watching for. So the loader compares against what is
    already being served and refuses to make it worse without being told to.
    """
    if serving is None:
        return None
    previous_revision, previous = serving
    current = 0.0 if new_coverage is None else float(new_coverage)
    if current >= previous - COVERAGE_TOLERANCE:
        return None
    detail = ("no descriptions at all -- was the ingest run without --descriptions?"
              if new_coverage is None else f"coverage fell to {current:.1%}")
    return (f"description coverage {previous:.1%} -> {current:.1%} "
            f"(serving revision {previous_revision}): {detail}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Load TTM trees into Postgres.")
    parser.add_argument("--database-url", default=os.environ.get("DATABASE_URL"))
    parser.add_argument("--trees", default=os.path.join("data", "generated"),
                        help="ingest output directory (containing trees/ and manifest.json)")
    parser.add_argument("--level-cap", type=int, default=DEFAULT_LEVEL_CAP,
                        help="level cap to resolve tiered node ranks against")
    parser.add_argument("--skip-counts", action="store_true",
                        help="load trees without precomputing build counts")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--allow-coverage-drop", action="store_true",
                        help="promote even if description coverage is worse than the "
                             "revision currently being served")
    args = parser.parse_args()

    if not args.database_url:
        print("FATAL: no database URL. Pass --database-url or set DATABASE_URL.",
              file=sys.stderr)
        return 2

    manifest_path = os.path.join(args.trees, "manifest.json")
    tree_paths = sorted(glob.glob(os.path.join(args.trees, "trees", "*.json")))
    if not os.path.exists(manifest_path) or not tree_paths:
        print(f"FATAL: no ingest output in {args.trees}. Run the ingest first.",
              file=sys.stderr)
        return 2

    with open(manifest_path, encoding="utf-8") as handle:
        manifest = json.load(handle)
    revision = manifest["revision"]
    source = manifest["source"]
    descriptions = manifest.get("descriptions") or {}
    point_caps = manifest.get("pointCaps") or {}

    trees = [json.load(open(p, encoding="utf-8")) for p in tree_paths]
    print(f"revision  {revision}")
    print(f"trees     {len(trees)}")
    print(f"digest    {source['digest'][:16]}...")
    if point_caps:
        print("caps      " + ", ".join(f"{k} {v}" for k, v in sorted(point_caps.items())))

    # ---- counts, before touching the database ------------------------------
    # Computed first so a DP failure aborts before anything is inserted.
    counts: dict[str, dict[int, tuple[int, int]]] = {}
    if not args.skip_counts:
        load_tree_json, build_graph, topo, count_dp = load_dp()
        print(f"counting  (level cap {args.level_cap})")
        for path, tree in zip(tree_paths, trees):
            _, nodes = load_tree_json(path, level_cap=args.level_cap)
            meta, par, chi = build_graph(nodes)
            order = topo(meta, par, chi)
            sets, _ = count_dp(meta, par, chi, order, len(meta))
            builds, _ = count_dp(meta, par, chi, order, len(meta), weight_choices=True)
            counts[tree["key"]] = {
                p: (sets[p], builds.get(p, 0))
                for p in sets
                if p > 0 and sets[p] > 0
            }
        rows = sum(len(v) for v in counts.values())
        biggest = max((max(b for _, b in v.values()) for v in counts.values() if v), default=0)
        print(f"          {rows:,} count rows, largest {biggest:,} builds")

    if args.dry_run:
        print("dry run: nothing written")
        return 0

    import psycopg

    with psycopg.connect(args.database_url) as conn:
        with conn.cursor() as cur:
            # Read what is being served *before* touching anything. The upsert below
            # overwrites this revision's own coverage and clears its promoted_at, so after
            # it there is nothing left to compare against -- and re-ingesting the same
            # upstream build reuses the same revision number, which is the common case.
            serving = _currently_serving(cur)

            # Re-running the same revision updates it in place rather than deleting it.
            # Deleting would cascade into trees, which solve_jobs references -- a reload
            # must never destroy a user's job. promoted_at is cleared here and set again
            # only once every statement below has succeeded.
            cur.execute(
                """
                INSERT INTO ingest_runs (
                    revision, source_provider, source_origin, source_digest, fetched_at,
                    tree_count, node_count, anomalies, description_coverage, warnings
                ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                ON CONFLICT (revision) DO UPDATE SET
                    source_provider = EXCLUDED.source_provider,
                    source_origin   = EXCLUDED.source_origin,
                    source_digest   = EXCLUDED.source_digest,
                    fetched_at      = EXCLUDED.fetched_at,
                    tree_count      = EXCLUDED.tree_count,
                    node_count      = EXCLUDED.node_count,
                    anomalies       = EXCLUDED.anomalies,
                    description_coverage = EXCLUDED.description_coverage,
                    warnings        = EXCLUDED.warnings,
                    promoted_at     = NULL
                """,
                (
                    revision, source["provider"], source["origin"], source["digest"],
                    source["fetchedAt"], len(trees),
                    sum(t["nodeCount"] for t in trees),
                    json.dumps(manifest.get("anomalies") or {}),
                    descriptions.get("coverage"),
                    json.dumps(manifest.get("warnings") or []),
                ),
            )

            cur.executemany(
                """
                INSERT INTO trees (
                    id, revision, key, kind, game, gating, class_id, spec_id,
                    class_name, spec_name, sub_tree_id, name, definition,
                    point_cap, max_points_in_tree, node_count
                ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                ON CONFLICT (id, revision) DO UPDATE SET
                    key = EXCLUDED.key, kind = EXCLUDED.kind, game = EXCLUDED.game,
                    gating = EXCLUDED.gating, class_id = EXCLUDED.class_id,
                    spec_id = EXCLUDED.spec_id, class_name = EXCLUDED.class_name,
                    spec_name = EXCLUDED.spec_name, sub_tree_id = EXCLUDED.sub_tree_id,
                    name = EXCLUDED.name, definition = EXCLUDED.definition,
                    point_cap = EXCLUDED.point_cap,
                    max_points_in_tree = EXCLUDED.max_points_in_tree,
                    node_count = EXCLUDED.node_count
                """,
                [
                    (
                        t["id"], revision, t["key"], t["kind"], t["game"], t["gating"],
                        t["classId"], t["specId"], t["className"], t["specName"],
                        t["subTreeId"], t["name"], json.dumps(t),
                        t["pointCap"], t["maxPointsInTree"], t["nodeCount"],
                    )
                    for t in trees
                ],
            )
            print(f"inserted  {len(trees)} trees")

            if counts:
                # Counts are derived and nothing references them, so replacing is safe.
                cur.execute("DELETE FROM tree_counts WHERE tree_revision = %s", (revision,))
                by_key = {t["key"]: t["id"] for t in trees}
                cur.executemany(
                    """
                    INSERT INTO tree_counts
                        (tree_id, tree_revision, points, set_count, build_count, level_cap)
                    VALUES (%s, %s, %s, %s, %s, %s)
                    """,
                    [
                        (by_key[key], revision, points, str(set_n), str(build_n),
                         args.level_cap)
                        for key, totals in counts.items()
                        for points, (set_n, build_n) in sorted(totals.items())
                    ],
                )
                print(f"inserted  {sum(len(v) for v in counts.values()):,} count rows")

            # Only now is the revision fit to serve -- if it is actually an improvement
            # on what is being served already.
            regression = _coverage_regression(serving, descriptions.get("coverage"))
            if regression and not args.allow_coverage_drop:
                # Everything, not just the promotion. Re-ingesting the same upstream build
                # reuses the revision number, so the upsert above has already cleared the
                # good revision's promoted_at and coverage -- committing the load while
                # withholding the promotion would un-promote the very data it protects.
                conn.rollback()
                print(f"REFUSED to promote revision {revision}: {regression}",
                      file=sys.stderr)
                print("  Nothing was written: the whole load is rolled back, so the "
                      "previous revision keeps being served.", file=sys.stderr)
                print("  Re-run the ingest with --descriptions, or pass "
                      "--allow-coverage-drop if the drop is intended.", file=sys.stderr)
                return 1
            if regression:
                print(f"WARNING: {regression} (promoting anyway, as asked)",
                      file=sys.stderr)
            cur.execute(
                "UPDATE ingest_runs SET promoted_at = now() WHERE revision = %s",
                (revision,),
            )
        conn.commit()

    print(f"promoted  revision {revision}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
