# Browser-only: hosting for free on Cloudflare

**Status:** planned (2026-10-03). Not started.

The goal is to host the app on Cloudflare's free plans with no server to run. Counting, listing
builds and talent statistics move into the browser. Tree data and icons become static files built
by CI. What is left is a single Cloudflare Worker that holds the WarcraftLogs key.

## Can it truly be browser-only?

Everything except WarcraftLogs can.

| Today (server)                         | Browser-only                                                  |
|----------------------------------------|---------------------------------------------------------------|
| `GET /trees`, `/trees/{key}`           | static JSON per game, built by CI from the ingest              |
| `GET /trees/{key}/counts`              | static JSON (precomputed unfiltered counts), same build        |
| `POST /counts`, `/counts/spread`       | the counting DP, running in the browser (WebAssembly)          |
| `POST /solve` + worker + queue         | the engine, running in the browser (WebAssembly, a Web Worker) |
| `GET /solve/{id}/results`, `/stats`    | in memory, from the listing that just ran                      |
| `GET /icons`, `/icons/{name}`          | static image files, fetched once by CI                         |
| `POST /custom-trees`, `GET /custom-trees/{p}` | the design travels in the link (compressed); optional short links in Workers KV |
| `GET /popular/*` (WarcraftLogs)        | **a Cloudflare Worker**: the key must stay secret              |

WarcraftLogs v2 needs a client secret. A secret in the browser is not a secret, so this one
feature needs a server. A Cloudflare Worker is enough: about a hundred lines, free up to 100,000
requests a day, with the key as a Worker secret and answers cached for six hours, as the API
does today.

## Parity: one engine, by construction

The requirement: the browser and the C++ engine must never drift apart, now or as the counting
features change.

The browser should therefore not get a TypeScript re-implementation. It gets **the C++
compiled to WebAssembly**, the same source the native CLI is built from. A new filter or a
rank rule is written once, in C++, and both builds have it.

The counting DP is Python today (`tools/frontier-dp/frontier_dp.py`). It moves into the engine
as C++. The Python version stays as the independent oracle that tests compare against.

Three implementations then check each other in CI:

```
C++ DP (native) ── must equal ── Python DP (oracle)
      │                                │
C++ DP (wasm) ─── must equal ─── engine enumeration (native and wasm)
```

- **native vs wasm**: same inputs, byte-identical outputs: counts, spreads, listed builds.
  This is what guarantees "the browser runs what the CLI runs".
- **C++ DP vs Python DP**: every tree, several budgets, every filter kind (required, barred,
  sides, groups, rank limits). This guards the port, and any later change to the DP.
- **DP vs enumeration**: the existing cross-check (`crosscheck_json.py`, the API's
  "every filter kind reaches the engine"), ported to run against the C++ DP.

The rule going forward: **counting and listing features land in C++ first.** The browser gets
them by recompiling. CI fails if native and wasm disagree.

## Phases

### 0. Spike: does the engine run in the browser? (decision gate)
- Build `Engine/src` (TalentTrees, TreeSolver) with Emscripten in Docker (`emscripten/emsdk`).
  The engine core needs no threads or files. Only the CLI uses threads, and the presets file's
  Windows headers get an `#ifdef`.
- From Node: solve one retail spec tree with filters and compare with `ttm-solver`.
- Measure: wasm size, time to list 10k / 50k / 2M builds, peak memory.
- **Go/no-go:** if listing the sim limit (10k–50k) takes more than a few seconds, or memory
  blows up, fall back to listing builds from the DP (walk its states backwards). That needs a
  parity suite against the engine, the same shape as above.

### 1. The DP in C++
- `Engine/src/FrontierDP.{h,cpp}`: the counting DP with every feature it has today: sets and
  builds, choice sides, required/barred, at-least-one and exactly-one groups, rank min/max,
  and the full spread (every point total).
- CLI: `--count` (one total) and `--spread` (every total), JSON out.
- Tests: C++ DP vs Python DP on every ingested tree (retail and Forever) × budgets × filter
  kinds. Runs in `engine-tests.yml` (release-only, like the rest of CI).

### 2. The engine as a browser module
- `ttm_wasm`: an Emscripten target exporting `count`, `spread`, `list` (tree JSON + constraints
  in, JSON out), built by CMake alongside `ttm-solver`.
- Runs in a **Web Worker**, so the page never freezes. Cancelling terminates the worker, which
  replaces the job queue's cancellation.
- `frontend/src/lib/engine.ts`: the same shapes `api.ts` returns today, so the UI barely
  changes.
- Parity CI: native vs wasm on fixtures, byte-identical.

### 3. Static data
- The release CI runs the ingest (retail, Forever) and writes `site/data/<game>/index.json`,
  one file per tree, and its unfiltered counts. That is about 5.5 MB raw, much less gzipped.
- Icons: fetched once per release into `site/icons/`. One size (56px; CSS scales it down).
  Cloudflare Pages allows 20,000 files per deployment, so check the icon count. Over budget,
  pack icons into one sprite sheet per class.
- A data version in `index.json` replaces `/health`'s revision.

### 4. The frontend switches over
- `api.ts` keeps its function names. They call the engine module and static files instead of
  `/api`. Everything above them (counts, Simulate, Analyse, statistics) is unchanged.
- Custom projects: the canonical design is compressed into the link, so the link is the
  project. The project id stays the hash of the design, as now. Optional later: short links
  stored in Workers KV.
- The browser-side features (saved setups, projects, SimC import) already live in the
  browser. Nothing to do.

### 5. The WarcraftLogs Worker
- `workers/popular/`: the `/popular/content` and `/popular/{spec}` logic ported from Python to
  TypeScript, with keys as Worker secrets and the Cache API for six hours. That is the only
  place the 3,600 points an hour are spent, so caching matters more with many users.
- The legality check of real builds against our tree data moves into the browser (it has the
  engine), so the Worker only fetches and summarises.

### 6. Deploy
- Cloudflare Pages for the site, a Worker for `/popular`, on your account.
- `release.yml` builds wasm, runs the parity suites, builds the site, and deploys with
  `wrangler`. It needs two GitHub secrets: a Cloudflare API token (scoped to Pages and Workers)
  and the account id. **You create these.** I cannot log into Cloudflare or GitHub settings.
- Still release-only, as agreed: nothing deploys or runs on an ordinary push.

### 7. Retire the server stack
- After parity holds in production: remove the API, worker, queue and Postgres (migrations,
  Docker services). Keep the ingest (it runs in CI), the engine and CLI (native tests, wasm
  build), the Python DP (oracle) and the SimC container (for the live tests).
- Fewer moving parts is also less to keep at parity.

## What stays the same for a player
Everything visible: Narrow, Simulate, Analyse, Forever, the editor, top players, ranks, links.
Counts may come back faster (no network round trip). Listing large sets is bounded by the
player's machine instead of ours, which is fine for the sim limit (10k–50k builds).

## Risks
- **Listing speed and memory in wasm.** The spike (phase 0) answers this before anything is
  built on it.
- **Free-tier terms change.** Pages, Workers and KV limits are as understood in October 2026.
  Check them before relying on them.
- **The 64-slot listing limit** (uint64 bitsets) carries over unchanged; custom trees over
  64 ranks can be counted but not listed, as now.
- **WarcraftLogs budget** is shared by all visitors through one key; the cache is what makes
  that work.

## Decisions needed
1. Retire the server stack after the switch (recommended), or keep it as a self-host option
   (more to keep at parity)?
2. Custom-project links: design in the link only, or also KV short links?
3. A domain: a `*.pages.dev` subdomain is free; a custom one is set up in your account.
