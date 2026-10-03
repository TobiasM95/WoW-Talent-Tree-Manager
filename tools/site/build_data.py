#!/usr/bin/env python3
"""The site's static data: what the API used to serve, as files.

Reads the ingest's output (retail and WoW Forever) and writes, under the site's public folder:

    data/<game>/index.json     the game's revision, data age and tree summaries
    data/trees/<key>.json      one tree, as the API's /trees/{key} returned it
    data/icons.json            every icon name that has an image, for the editor's picker
    icons/<name>.jpg           the icons, 56px (CSS scales them down), from Blizzard's renderer

    python tools/site/build_data.py --out frontend/public \\
        --game retail=data/generated --game forever=data/generated-forever --icon-cache data/icons

Icons are fetched once and kept in --icon-cache, so a rebuild only downloads new ones. A name
upstream has no image for is remembered there too (as an empty marker), so it is not asked
for again on every release.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "services", "ingest"))
from ttm_ingest import icons  # noqa: E402

SUMMARY_FIELDS = ("key", "kind", "name", "className", "specName", "subTreeId", "nodeCount",
                  "maxPointsInTree", "pointCap", "order")


def summary(tree: dict) -> dict:
    out = {f: tree.get(f) for f in SUMMARY_FIELDS}
    out["order"] = tree.get("order")
    return out


def write_json(path: str, value) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(value, handle, separators=(",", ":"), ensure_ascii=False)


def build_game(game: str, src: str, out: str) -> tuple[list[dict], set[str]]:
    manifest = json.load(open(os.path.join(src, "manifest.json"), encoding="utf-8"))
    trees = [json.load(open(p, encoding="utf-8")) for p in sorted(glob.glob(os.path.join(src, "trees", "*.json")))]
    if not trees:
        raise SystemExit(f"{game}: no trees in {src}")
    names: set[str] = set()
    for tree in trees:
        write_json(os.path.join(out, "data", "trees", *tree["key"].split("/")) + ".json", tree)
        names |= icons.icon_names(tree)
    source = manifest.get("source") or {}
    write_json(os.path.join(out, "data", game, "index.json"), {
        "game": game,
        "revision": manifest["revision"],
        "fetchedAt": source.get("fetchedAt"),
        "attribution": source.get("attribution"),
        "trees": [summary(t) for t in trees],
        "nodeCount": sum(len(t["nodes"]) for t in trees),
    })
    return trees, names


def build_icons(names: set[str], out: str, cache: str) -> list[str]:
    os.makedirs(cache, exist_ok=True)
    missing = [n for n in sorted(names) if not os.path.exists(os.path.join(cache, f"{n}.jpg"))
               and not os.path.exists(os.path.join(cache, f"{n}.none"))]
    if missing:
        rows, retry, bad = icons.fetch_many(missing, icons.DEFAULT_SIZE)
        for row in rows:
            if row["status"] == 200 and row["bytes"]:
                with open(os.path.join(cache, f"{row['name']}.jpg"), "wb") as handle:
                    handle.write(row["bytes"])
            else:
                open(os.path.join(cache, f"{row['name']}.none"), "w").close()
        if retry:
            print(f"icons: {len(retry)} failed to download this time (not cached, retried next build)")
    have = sorted(n for n in names if os.path.exists(os.path.join(cache, f"{n}.jpg")))
    target = os.path.join(out, "icons")
    os.makedirs(target, exist_ok=True)
    for name in have:
        shutil.copyfile(os.path.join(cache, f"{name}.jpg"), os.path.join(target, f"{name}.jpg"))
    return have


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, help="the site's public folder")
    parser.add_argument("--game", action="append", required=True, help="game=ingest output dir")
    parser.add_argument("--icon-cache", default=os.path.join("data", "icons"))
    parser.add_argument("--no-icons", action="store_true")
    args = parser.parse_args()

    for sub in ("data", "icons"):
        shutil.rmtree(os.path.join(args.out, sub), ignore_errors=True)
    all_names: set[str] = set()
    for spec in args.game:
        game, src = spec.split("=", 1)
        trees, names = build_game(game, src, args.out)
        all_names |= names
        print(f"{game}: {len(trees)} trees, {len(names)} icon names")
    if not args.no_icons:
        have = build_icons(all_names, args.out, args.icon_cache)
        write_json(os.path.join(args.out, "data", "icons.json"), have)
        print(f"icons: {len(have)} of {len(all_names)} have an image")
    return 0


if __name__ == "__main__":
    sys.exit(main())
