#!/usr/bin/env python3
"""TTM talent data ingest.

    python ingest.py --out data/generated            # fetch live
    python ingest.py --source talents.json --out ... # from a local payload
    python ingest.py --dry-run                       # validate, write nothing

Writes one JSON file per tree plus a manifest recording the source digest, the run's
revision, per-tree node counts, and every anomaly observed. Output is promoted only
after validation passes, so a bad run leaves the previous revision in place.

Exit codes: 0 success, 1 validation or transform failure, 2 bad usage.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ttm_ingest import source as source_mod
from ttm_ingest import transform as transform_mod
from ttm_ingest import validate as validate_mod
from ttm_ingest import descriptions as desc_mod
from ttm_ingest import point_caps as caps_mod


def human(n: int) -> str:
    return f"{n:,}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Ingest WoW talent data into TTM tree JSON.")
    parser.add_argument("--source", help="local payload path or URL (default: raidbots live)")
    parser.add_argument("--out", default="data/generated", help="output directory")
    parser.add_argument("--cache-dir", help="cache fetched payloads here, keyed by digest")
    parser.add_argument("--revision", type=int, help="revision number (default: derived)")
    parser.add_argument("--dry-run", action="store_true", help="validate but write nothing")
    parser.add_argument("--quiet", action="store_true", help="only report problems")
    parser.add_argument("--descriptions", action="store_true",
                        help="also fetch talent descriptions (separate stage, cached)")
    parser.add_argument("--description-cache", default="data/descriptions.json",
                        help="description cache path")
    parser.add_argument("--min-coverage", type=float, default=0.95,
                        help="fail if resolved description ratio falls below this")
    parser.add_argument("--limit-descriptions", type=int,
                        help="only resolve the first N keys (for smoke tests)")
    parser.add_argument("--point-caps", action="store_true",
                        help="derive per-tree point caps from DB2 (level-based grants)")
    parser.add_argument("--level-cap", type=int, default=90,
                        help="character level the point caps are derived for")
    parser.add_argument("--db2-build", help="pin a DB2 build instead of the live one")
    args = parser.parse_args()

    def say(*a):
        if not args.quiet:
            print(*a)

    # ---- fetch -------------------------------------------------------------
    try:
        payload = source_mod.load(args.source, cache_dir=args.cache_dir)
    except source_mod.SourceError as exc:
        print(f"FATAL: {exc}", file=sys.stderr)
        return 1

    say(f"source    {payload.origin}")
    say(f"fetched   {payload.fetched_at}  ({human(payload.byte_size)} bytes)")
    say(f"digest    {payload.digest}")
    say(f"specs     {len(payload.specs)}")

    # ---- validate the input ------------------------------------------------
    try:
        warnings = validate_mod.check_payload(payload)
    except validate_mod.ValidationError as exc:
        print(f"FATAL: upstream payload rejected: {exc}", file=sys.stderr)
        return 1
    for w in warnings:
        print(f"WARNING: {w}", file=sys.stderr)

    # ---- transform ---------------------------------------------------------
    revision = args.revision if args.revision is not None else int(payload.digest[:8], 16) % 1_000_000
    source_meta = {
        "provider": "raidbots",
        "origin": payload.origin,
        "fetchedAt": payload.fetched_at,
        "digest": payload.digest,
        "revision": revision,
    }
    try:
        trees, anomalies = transform_mod.transform(payload, source=source_meta)
    except transform_mod.TransformError as exc:
        print(f"FATAL: transform rejected the payload: {exc}", file=sys.stderr)
        return 1

    # ---- validate the output -----------------------------------------------
    try:
        tree_warnings = validate_mod.check_trees(trees)
    except validate_mod.ValidationError as exc:
        print(f"FATAL: transformed trees rejected: {exc}", file=sys.stderr)
        return 1
    for w in tree_warnings:
        print(f"WARNING: {w}", file=sys.stderr)

    kinds: dict[str, int] = {}
    for tree in trees:
        kinds[tree["kind"]] = kinds.get(tree["kind"], 0) + 1
    total_nodes = sum(t["nodeCount"] for t in trees)

    say()
    say(f"revision  {revision}")
    say(f"trees     {len(trees)}  (" + ", ".join(f"{k}: {v}" for k, v in sorted(kinds.items())) + ")")
    say(f"nodes     {human(total_nodes)}")

    observed = anomalies.summary()
    if any(observed.values()):
        say()
        say("expected anomalies (see docs/02-target/raidbots-live-schema.md):")
        for key, count in observed.items():
            if count:
                say(f"  {key:26} {count}")

    # ---- point caps: derived, not guessed --------------------------------------
    # The budget is level-based and the grant table lives in DB2, not in the raidbots
    # payload. Without this pointCap stays null and the UI cannot say "3 points left".
    caps_report = None
    if args.point_caps:
        say()
        try:
            tree_ids = {t["traitTreeId"] for t in trees if t.get("traitTreeId")}
            caps = caps_mod.derive_caps(args.level_cap, trait_tree_ids=tree_ids,
                                        build=args.db2_build, cache_dir=args.cache_dir)
            cap_warnings = caps_mod.apply_caps(trees, caps)
        except caps_mod.PointCapError as exc:
            print(f"FATAL: point caps rejected: {exc}", file=sys.stderr)
            return 1
        except Exception as exc:  # noqa: BLE001 - a source outage should not be cryptic
            print(f"FATAL: could not derive point caps: {type(exc).__name__}: {exc}",
                  file=sys.stderr)
            return 1
        caps_report = {"levelCap": args.level_cap, **caps}
        say(f"point caps at level {args.level_cap}: "
            + ", ".join(f"{k} {v}" for k, v in sorted(caps.items())))
        for w in cap_warnings:
            print(f"WARNING: {w}", file=sys.stderr)
        tree_warnings.extend(cap_warnings)

    # ---- descriptions: a separate stage on purpose ----------------------------
    # Tree structure does not depend on tooltip text, so an outage here degrades text
    # rather than blocking a tree update.
    description_report = None
    if args.descriptions:
        say()
        keys = desc_mod.required_keys(trees)
        cache = desc_mod.DescriptionCache(args.description_cache)
        if args.limit_descriptions:
            keys = keys[: args.limit_descriptions]
        say(f"descriptions: {human(len(keys))} (spellId, definitionId, rank) keys needed")

        def progress(done, total, cov):
            if not args.quiet:
                print(f"  {done}/{total}  fetched {cov.fetched}  cached {cov.from_cache}  "
                      f"missing {len(cov.missing)}", flush=True)

        coverage = desc_mod.fetch_descriptions(keys, cache=cache, progress=progress)
        cache.save()
        filled = desc_mod.apply_descriptions(trees, cache)
        description_report = coverage.summary()
        description_report["entriesFilled"] = filled
        say(f"  resolved {human(coverage.resolved)}/{human(coverage.requested)} "
            f"({coverage.ratio:.1%}), {human(coverage.fetched)} fetched, "
            f"{human(coverage.from_cache)} from cache, {human(filled)} entries filled")
        for err in coverage.errors[:5]:
            print(f"WARNING: description miss {err}", file=sys.stderr)
        if coverage.ratio < args.min_coverage:
            print(
                f"FATAL: description coverage {coverage.ratio:.1%} is below "
                f"{args.min_coverage:.1%}. The tooltip source may have changed shape -- "
                "the legacy pipeline silently substituted placeholder text here.",
                file=sys.stderr,
            )
            return 1

    if args.dry_run:
        say()
        say("dry run: nothing written")
        return 0

    # ---- write atomically: build beside the target, then swap --------------
    out = os.path.abspath(args.out)
    parent = os.path.dirname(out) or "."
    os.makedirs(parent, exist_ok=True)
    staging = tempfile.mkdtemp(prefix=".ingest-", dir=parent)
    try:
        trees_dir = os.path.join(staging, "trees")
        os.makedirs(trees_dir)
        for tree in trees:
            path = os.path.join(trees_dir, tree["key"].replace("/", "_") + ".json")
            with open(path, "w", encoding="utf-8") as handle:
                json.dump(tree, handle, indent=2, sort_keys=True, ensure_ascii=False)
                handle.write("\n")

        manifest = {
            "schemaVersion": transform_mod.SCHEMA_VERSION,
            "revision": revision,
            "source": source_meta,
            "counts": {
                "trees": len(trees),
                "nodes": total_nodes,
                "byKind": kinds,
                "specs": len({(t["classId"], t["specId"]) for t in trees}),
                "classes": len({t["classId"] for t in trees}),
            },
            "anomalies": observed,
            "descriptions": description_report,
            "pointCaps": caps_report,
            "anomalyDetail": {
                "degenerateNodes": anomalies.degenerate_nodes,
                "crossTreeEdges": anomalies.cross_tree_edges[:100],
                "siblingSubtreeNodes": anomalies.sibling_subtree_nodes,
                "unresolvedPrerequisites": anomalies.unresolved_prerequisites,
            },
            "warnings": warnings + tree_warnings,
            "trees": [
                {
                    "key": t["key"], "id": t["id"], "kind": t["kind"],
                    "className": t["className"], "specName": t["specName"],
                    "nodeCount": t["nodeCount"], "maxPointsInTree": t["maxPointsInTree"],
                }
                for t in trees
            ],
        }
        with open(os.path.join(staging, "manifest.json"), "w", encoding="utf-8") as handle:
            json.dump(manifest, handle, indent=2, sort_keys=True, ensure_ascii=False)
            handle.write("\n")

        previous = out + ".previous"
        if os.path.exists(previous):
            shutil.rmtree(previous)
        if os.path.exists(out):
            os.rename(out, previous)
        os.rename(staging, out)
        staging = None
    finally:
        if staging and os.path.exists(staging):
            shutil.rmtree(staging)

    say()
    say(f"written   {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
