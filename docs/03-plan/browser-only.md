# Browser-only: hosting for free on Cloudflare

**Status:** built (2026-10-03); the first deploy runs from the release workflow.

| Phase | State |
|---|---|
| 1. Counter in TypeScript | done: exact against the Python DP, 1,769 searches, every point total |
| 2. Listing from the counter | done |
| 3. Parity suite in CI | done: 4,938 counts and 1,277 listings (594,245 builds) identical to the C++ engine |
| 4. Static data | done: `tools/site/build_data.py`, 187 trees, 2,181 icons, ~13 MB |
| 5. Pages Functions | done: `/api/popular/*` (WarcraftLogs), `/api/custom-trees` (KV) |
| 6. Frontend switch | done: every suite passes against the site under `wrangler pages dev` |
| 7. Deploy | written (`release.yml`); first run pending |
| 8. Server retired | done: API, worker, queue, Postgres and the Docker setup removed |

One change from the plan: the WarcraftLogs proxy and the project store are Pages Functions,
deployed with the site on its own origin, rather than a separate Worker.

The app runs on Cloudflare's free plans with no server of our own. The site is static
(Cloudflare Pages, a `*.pages.dev` address). Counting and listing builds run in the browser in
small TypeScript. Tree data and icons are static files built by CI. One Cloudflare Worker holds
the WarcraftLogs key and stores custom projects. The API, worker, queue and Postgres are
retired completely.

## What moves where

| Today (server)                                | Browser-only                                                     |
|-----------------------------------------------|------------------------------------------------------------------|
| `GET /trees`, `/trees/{key}`                  | static JSON per game, built by CI from the ingest                 |
| `GET /trees/{key}/counts`                     | static JSON (unfiltered counts), same build                       |
| `POST /counts`, `/counts/spread`              | the counter, in TypeScript, in a Web Worker                       |
| `POST /solve` + worker + queue, `/results`    | listing builds from the same counter, up to the sim limit         |
| `GET /solve/{id}/stats`                       | computed from the listed builds, in the browser                   |
| `GET /icons`, `/icons/{name}`                 | static image files, fetched once by CI                            |
| `POST /custom-trees`, `GET /custom-trees/{p}` | the Worker: designs stored in Workers KV under their hash         |
| `GET /popular/*` (WarcraftLogs)               | the Worker: the key stays secret there, answers cached six hours  |

Only two things need a server, and one Worker serves both:

- **WarcraftLogs**, because a secret in the browser is not a secret.
- **Custom projects**, so a link stays short. Links already name a project by the hash of its
  design (`t=custom/<hash>/...`). The design is stored in KV under that hash, so every existing
  link format keeps working. The alternative, the whole design inside the link, runs to tens
  of kilobytes for a copied retail tree and breaks in chat apps.

## The browser: as simple as possible

- **The counter** is the frontier DP, ported from Python to TypeScript: about 350 lines. It
  covers sets and builds, choice sides, required and barred talents, at-least-one and
  exactly-one groups, rank limits, and counts at every point total for shared pools.
- **Listing builds** reuses the counter. Its states already say how many builds lie behind
  each choice, so the builds can be walked out of it directly, up to the sim limit (10k by
  default, 50k at most). That needs no second engine in the browser, and the count and the
  list agree by construction.
- It runs in a Web Worker, so the page never freezes. Cancelling means terminating the worker.

## Parity: the C++ engine stays a maintained twin

The C++ engine is not part of the site. It must not be abandoned either. It remains the
independent implementation that keeps the browser counter honest, and new counting or tree
mechanics land in both.

The release CI enforces it. The parity suite runs on every ingested tree (retail, Forever) and
a set of custom fixtures (barriers, granted talents, shared pools). For several budgets and every
filter kind:

- the TypeScript counter's count equals the C++ engine's enumeration count;
- the builds the TypeScript lister produces are exactly the C++ engine's, as sets.

A feature in one and not the other fails the release. So does a feature added to the browser
whose cases the parity suite doesn't cover: each new constraint kind needs its parity case,
the way rank limits got one.

During the port, the Python DP is a third reference: the TypeScript counter must match it
exactly first. After that the Python DP retires with the server; two implementations are
enough, and fewer to keep in step.

## Phases

1. **The counter in TypeScript.** Port, then match the Python DP on every tree × several
   budgets × every filter kind, exactly.
2. **Listing builds from the counter.** Then match the C++ engine's listings as sets.
3. **The parity suite in CI.** TypeScript vs the C++ engine, built from source, in the
   release workflow. Replaces the Python cross-check.
4. **Static data.** The release CI runs the ingest (retail, Forever) and writes per-game
   indexes, one file per tree, unfiltered counts and the icons (one size; CSS scales it down).
5. **The Worker.** `/popular` ported from Python, with keys as Worker secrets and the Cache API;
   `/custom-trees` with KV (validation ported, content-addressed as now).
6. **The frontend switches over.** `api.ts` keeps its function names and calls the counter, the
   static files and the Worker instead of `/api`. The UI above it barely changes.
7. **Deploy from the release CI** with `wrangler`, using the repo secrets
   `CLOUDFLARE_ACCOUNT_ID` and `CLOUDFLARE_API_TOKEN`. Still release-only: nothing deploys or
   runs on an ordinary push.
8. **Retire the server stack.** API, worker, queue, Postgres, migrations and their Docker
   services. Kept: the ingest (runs in CI), the C++ engine and CLI (parity), and the SimC
   container (for the live tests).

## Risks

- **Counter speed in the browser.** Python counts in milliseconds; TypeScript should be
  faster, but phase 1 measures it on the largest trees and the heaviest filters.
- **Counts past 2^53.** Whole-character spaces exceed it, and some single trees might.
  The counter uses BigInt where a number could overflow, and the parity suite would catch a
  rounding error.
- **Free-tier terms change.** Pages, Workers and KV limits are as understood in October 2026.
- **WarcraftLogs budget** (3,600 points an hour) is shared by every visitor through one key;
  the cache is what makes that work.
- **The 64-slot listing limit** of the C++ engine stays. Parity cases for listing run only on
  trees the engine can list; counting parity covers every tree.
