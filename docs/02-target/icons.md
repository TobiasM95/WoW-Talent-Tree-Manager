# Icons

Talent icons arrive from upstream as *names* — `ability_demonhunter_feldevastation` — never
as images. This document covers where the images come from, why the pipeline is shaped the
way it is, and the position taken on assets nobody here owns.

## The position on ownership

Talent icons are Blizzard's artwork. Nothing in this project claims otherwise, and the
pipeline is built so that the repository never contains any of it:

- **No icon bytes in the repository.** Not in `git`, not in a Docker image, not in the
  frontend bundle. The `icons` table is a runtime cache, filled by a deliberate command
  (`docker compose run --rm sync-icons`) that a deployment chooses to run.
- **Fetched from Blizzard's own service**, `render.worldofwarcraft.com`, not from a
  third-party aggregator's CDN. First-party means there is no intermediary's terms of
  service in the way, and no one else's bandwidth being spent.
- **Not redistributed.** The cache serves the operator's own deployment. It is derived
  data: `TRUNCATE icons` is always safe, and the cache can be rebuilt or never built.
- **Entirely optional.** The application is designed to work with an empty `icons` table.
  A talent without an icon renders by name. `/health` reports `iconCoverage` as
  information, not as a failure.

Set `TTM_ICON_BASE_URL` to a mirror, or simply never run the sync. The difference is
cosmetic, which is the point: a tool for counting talent builds should not depend on
someone else's art to function.

This is separate from, and more conservative than, the UI direction — see
[`ui-direction.md`](ui-direction.md), which covers not imitating the game's own visual
identity in original artwork.

## One file per name, never an atlas

The native app shipped a packed icon atlas. For a web client that is the wrong shape in
three ways:

| | Atlas | One file per name |
|---|---|---|
| Browser caching | one blob; any change invalidates all of it | per icon, independently |
| Drawing one tree | download every icon in the game | download the ~60 that tree uses |
| Adding an icon | regenerate and redeploy the atlas | insert one row |

Icons are also immutable under a name, which is what makes the per-file version cheap:
`Cache-Control: public, max-age=31536000, immutable` plus an ETag means a returning visitor
makes **no icon requests at all**.

## Sizes

Blizzard's renderer serves 18, 36 and 56 pixels. 64 and above answer `403`, so 56 is the
largest icon available and the pipeline states that rather than pretending otherwise. It is
also the size every comparable tool uses; a canvas scales it with CSS, and a retina display
gets a slightly soft icon rather than a missing one.

`size` is part of the `icons` primary key, so adding 36 later is a sync run, not a
migration.

## The pipeline

```
raidbots payload   -->  ingest       -->  tree definition   (icon NAMES, normalised)
                                            |
                                            v
current_trees  -->  sync_icons  -->  icons table  -->  GET /icons/{name}
                     (fetch)          (bytes)           (immutable, ETag)
```

**Normalised once, in one place.** `ttm_ingest.icons.normalise` lowercases, strips an asset
extension and validates against `^[a-z0-9_]{2,128}$`. The ingest uses it so stored
definitions are clean, and the API uses it so a name from a client resolves to the same
row — a second implementation would let the two drift and a lookup would miss its own cache
entry. It is also the reason `/icons/foo` and `/icons/foo.jpg` are the same icon.

The extension-stripping is not hypothetical: the live payload carries
`ability_druid_mangle.tga`, a raw asset filename. The icon behind it exists, so the
extension is stripped rather than the icon dropped.

**The alphabet is enforced, not trusted**, in code *and* as a table constraint. Upstream
paths are built from these names, and a name containing `/` or `..` would address something
else entirely.

## Caching absence

Not every name has art. 19 of 2,094 names answer `403` from Blizzard's renderer today —
mostly icons added to the game more recently than the render service's index.

A `4xx` is therefore **stored**, as a row with `status <> 200` and no bytes. Without that
negative cache the sync would re-ask upstream for the same missing images on every run,
forever.

A `5xx` or a network error is **not** stored. Recording a transient outage as "this icon
does not exist" would blank icons for a month, and nothing would look broken enough for
anyone to investigate. Misses are retried after 30 days anyway, since upstream does add art.

Three outcomes, reported separately, because the advice differs:

| Outcome | Cached? | What to do |
|---|---|---|
| `200` | yes, with bytes | nothing |
| `4xx` | yes, as absence | nothing; retried in 30 days |
| `5xx` / network | no | re-run the sync |
| unusable name | no | fix the transform; a re-run cannot help |

That last row is why `IconNameError` is a separate exception. Reporting a malformed name as
"re-run to retry" would be advice that can never work.

## Why Postgres, not a volume

2,094 icons at ~2.3 KB each is about **5 MB**. At that size the interesting question is not
performance but operational surface: a table needs no extra mount, no separate backup path,
and no second thing to reason about when a container is replaced. The API already has a
connection pool. This is the same "do a lot in Postgres" call the queue makes.

If the cache ever grew to a size where that stopped being true — every size, every game
version, a few hundred megabytes — a volume or object store would be the answer. It is not
close.

## Running it

```bash
docker compose run --rm sync-icons                 # fill what is missing
docker compose run --rm sync-icons --limit 20      # smoke run
docker compose run --rm sync-icons --retry-misses  # re-ask for the 403s
docker compose run --rm sync-icons --size 36       # a second size
python services/ingest/tests/test_icons.py         # no network needed
```

Idempotent and resumable. It asks Postgres which names have no usable row and fetches only
those, committing every 200, so an interrupted run keeps its work. A full cold fill is
about 105 seconds at eight concurrent requests — modest on purpose, since this is someone
else's CDN being asked for a couple of thousand small files.

Deliberately **not** part of `load_trees.py`: icons are keyed by name rather than by tree
revision, so the overlap between two revisions is close to total. Coupling them would
re-fetch everything on every ingest for no reason, and would let a CDN outage block a
talent-data update.
