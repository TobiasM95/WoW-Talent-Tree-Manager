"""Fetch WoW Forever's talent trees and write them in the ingest's output format.

    python services/ingest/forever_ingest.py [--source PATH_OR_URL] [--out data/generated-forever]

Then build the site's data from it, beside retail's (tools/site/build_data.py).

Written beside the retail output rather than into it, because each game is its own revision
and is promoted on its own: a Forever update must never touch what retail is serving.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import shutil
import sys
import tempfile
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ttm_ingest import forever  # noqa: E402
from ttm_ingest.transform import SCHEMA_VERSION  # noqa: E402


def fetch(source: str, cache_dir: str | None) -> tuple[bytes, str]:
    if not source.startswith(("http://", "https://")):
        with open(source, "rb") as handle:
            return handle.read(), source
    request = urllib.request.Request(source, headers={"User-Agent": "ttm-ingest (+github)"})
    with urllib.request.urlopen(request, timeout=60) as response:
        body = response.read()
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)
        digest = hashlib.sha256(body).hexdigest()
        with open(os.path.join(cache_dir, f"forever-{digest[:16]}.json"), "wb") as handle:
            handle.write(body)
    return body, source


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", default=forever.SOURCE_URL, help="local path or URL")
    parser.add_argument("--out", default="data/generated-forever")
    parser.add_argument("--cache-dir", default="data/cache")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    body, origin = fetch(args.source, args.cache_dir)
    payload = json.loads(body)
    digest = hashlib.sha256(body).hexdigest()
    source = {
        "provider": "talentsforever",
        "origin": origin,
        "digest": digest,
        "fetchedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "clientBuild": forever.client_build(payload),
        "generated": payload.get("generated"),
        "license": payload.get("license"),
        "attribution": payload.get("attribution"),
    }

    try:
        trees, warnings = forever.transform(payload, source=source)
        revision = forever.revision_for(payload)
    except forever.ForeverError as exc:
        print(f"FAILED: {exc}", file=sys.stderr)
        return 1

    nodes = sum(t["nodeCount"] for t in trees)
    print(f"client    {source['clientBuild']} (exported {source['generated']})")
    print(f"revision  {revision}")
    print(f"trees     {len(trees)} across {len({t['className'] for t in trees})} classes, {nodes} talents")
    for w in warnings:
        print(f"warning   {w}")
    if args.dry_run:
        print("dry run: nothing written")
        return 0

    out = os.path.abspath(args.out)
    parent = os.path.dirname(out) or "."
    os.makedirs(parent, exist_ok=True)
    staging = tempfile.mkdtemp(prefix=".forever-", dir=parent)
    try:
        os.makedirs(os.path.join(staging, "trees"))
        for tree in trees:
            path = os.path.join(staging, "trees", tree["key"].replace("/", "_") + ".json")
            with open(path, "w", encoding="utf-8") as handle:
                json.dump(tree, handle, indent=2, sort_keys=True, ensure_ascii=False)
                handle.write("\n")
        manifest = {
            "schemaVersion": SCHEMA_VERSION,
            "game": "forever",
            "revision": revision,
            "source": source,
            "counts": {"trees": len(trees), "nodes": nodes},
            "anomalies": {},
            "descriptions": None,
            "pointCaps": None,
            "warnings": warnings,
            "trees": [
                {"key": t["key"], "id": t["id"], "kind": t["kind"], "className": t["className"],
                 "name": t["name"], "nodeCount": t["nodeCount"], "maxPointsInTree": t["maxPointsInTree"]}
                for t in trees
            ],
        }
        with open(os.path.join(staging, "manifest.json"), "w", encoding="utf-8") as handle:
            json.dump(manifest, handle, indent=2, sort_keys=True, ensure_ascii=False)
            handle.write("\n")
        if os.path.exists(out):
            shutil.rmtree(out)
        os.rename(staging, out)
        staging = None
    finally:
        if staging and os.path.exists(staging):
            shutil.rmtree(staging)
    print(f"written   {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
