![TTM Banner](/GUI/resources/TTM_Banner.png?raw=true "TTM Banner")

# WoW Talent Tree Manager

Create, explore and share World of Warcraft talent trees and builds — including an
exhaustive build solver that enumerates every valid talent combination under
constraints.

> **Status: being rebuilt as a web application.**
> The native Windows client (v1.4.2) was discontinued in July 2024. It is preserved on the
> [`archive/native-client`](../../tree/archive/native-client) branch.
>
> The core loop runs end to end — count, constrain, enumerate, inspect — on live retail data
> for all 160 trees. Not yet public; the tree editor, sharing and sim analysis are not built.

## What this is

TTM's distinctive feature is its **solver**: given a talent tree and constraints (a point
budget, must-have and must-not-have talents), it enumerates *all* valid builds. It does this
with a topologically sorted minimal DAG and a `uint64_t` bitset, visiting only
strictly-increasing indices so no deduplication pass is ever needed. That engine is fast,
correct, and being kept — it is the reason this project is worth reviving.

Everything around it is being replaced.

Two games are supported: **retail** (class, spec and hero trees, with a full
SimulationCraft round trip) and **WoW Forever** (the vanilla-shaped game launching November
2026: three tabs per class sharing 51 points, planned and counted but not simmed, since
SimulationCraft does not support it). WoW Forever talent data is from
[talentsforever.com](https://talentsforever.com), read from the beta client and published under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

## Why the rewrite

The native client was discontinued when the talent data pipeline broke during the
*War Within* pre-patch. Analysis showed the actual cause was not the scraper: the upstream
data source is still live and already publishes hero talents. The problem was that TTM's data
model hardcodes exactly two trees per spec (class + spec), so hero talents had nowhere to go.

A native Windows client is also the wrong shape for this audience. The tool should be a URL.

**Read [`docs/`](docs/) before contributing.** It contains a full analysis of the existing
code, the target architecture, and the open decisions:

| Start here | |
|---|---|
| [`docs/README.md`](docs/README.md) | Index and the short version of every finding |
| [`docs/03-plan/open-questions.md`](docs/03-plan/open-questions.md) | What is settled and what still needs deciding |
| [`docs/03-plan/roadmap.md`](docs/03-plan/roadmap.md) | Phased plan with exit criteria |

## Running it

Everything is in Docker Compose. Postgres carries the data, the queue and the cache; there
is no broker and no separate cache to operate.

```bash
docker compose up -d postgres
docker compose run --rm migrate                     # schema
docker compose run --rm ingest --point-caps --descriptions
docker compose run --rm loader                      # trees + precomputed counts
docker compose run --rm sync-icons                  # optional; see docs/02-target/icons.md
docker compose up -d api worker

cd frontend && corepack enable && pnpm install && pnpm run dev   # http://localhost:5173
```

Or the production shape — Caddy serving the built frontend and proxying `/api`:

```bash
docker compose --profile web up -d --build web      # http://localhost:8081
```

The ingest fetches live talent data from Raidbots and derives point caps from DB2. The
loader refuses to promote a revision whose description coverage collapses against what is
already being served, so a run without `--descriptions` cannot quietly replace a complete
dataset with a blank one.

### Checking it

| | |
|---|---|
| `python tests/golden_counts.py build/ttm-solver` | Engine counts against known-correct values |
| `python tools/frontier-dp/test_counting.py` | The frontier DP the pre-flight gate uses |
| `python services/ingest/tests/test_ingest.py` | Transform and validation |
| `python services/ingest/tests/test_icons.py` | Icon names and fetch classification |
| `python services/api/test_api.py` | The API, against a live instance |
| `services/worker/test_worker.py` | Decoding, progress parsing, the watchdog |
| `services/worker/test_queue.py` | The lease sweeper |
| `bash services/db/smoke_test.sh` | End to end, and that the constraints bite |
| `cd frontend && pnpm test` | The real interactions in a real browser |

## Repository layout

| Path | What |
|---|---|
| `Engine/` | **C++ solver and tree model.** Kept, being made portable. The crown jewel. |
| `CLI/` | Headless entry point to the engine; becoming the server-side worker binary. |
| `GUI/`, `AppUpdater/` | The native Dear ImGui client. Retained as the reference implementation while the web app catches up. Not actively developed. |
| `docs/` | Analysis, target architecture, and plan. |
| `services/` | Backend services: ingest, api, worker, database schema and loaders. |
| `frontend/` | The web client. React + Tailwind; see [`frontend/README.md`](frontend/README.md). |
| `docker/` | Container definitions and the Caddy config. |
| `tools/frontier-dp/` | The counting DP, kept as the verified reference implementation. |
| `tests/` | Golden counts for the C++ engine. |

The `WoW Talent Manager.sln` still builds the native client in Visual Studio 2022. A portable
CMake build for `Engine` + `CLI` is being added alongside it so the solver can run in Linux
containers.

## Branches

| Branch | Purpose |
|---|---|
| `master` | Web application development. |
| `archive/native-client` | The native client at v1.4.2, frozen. |
| `archive/server-deploy` | Old deployment/domain setup for the abandoned web attempt. |
| `release` | What is about to ship. Pushing here (or tagging `v*`) runs CI. |

### CI runs on releases, not commits

Commits land constantly and most are small, so nothing runs on every push or pull request.
The checks run when something is about to ship — a push to `release`, a `v*` tag, or by hand
from the Actions tab:

| Workflow | What it proves |
|---|---|
| `release.yml` | The whole product: live ingest, the API's 48 checks, every frontend suite, and a real SimulationCraft round trip. |
| `engine-tests.yml` | Golden counts from the C++ engine, and the DP agreeing with it. The 8-minute overflow case runs on tags only. |
| `ingest-tests.yml` | The ingest against live data — also **weekly**, since upstream changes shape on Blizzard's schedule, not ours. |
| `update_presets.yml` | Legacy, manual only. Failing daily since 2024-07-05 on a retired runner image. |

Locally, the same suites are one command each: `python services/api/test_api.py` and
`pnpm run test:all` in `frontend/`.

### A note on history

This repository's history was rewritten to remove generated binary artifacts. A daily CI job
committed a 16 MB packed icon atlas 76 times, which had grown `.git` to **1.4 GB** against a
90 MB working tree. Stripping the three atlas paths and stale debug symbols reduced it to
**9.4 MB** — a 99.3% reduction — while preserving all 597 commits and 21 tags.

What was removed is *generated data*, regenerable by the preset scripts. Nothing authored was
lost. Commit SHAs changed, so an old clone cannot fast-forward; re-clone instead.

The rewrite has been published, so a fresh clone is ~10 MB. All 19 GitHub releases and their
attached `.zip` assets are unaffected — releases are keyed by tag name, and assets live in
separate blob storage, so the download links above still work.

> **If you have a clone from before September 2026**, it cannot fast-forward: every commit SHA
> changed. Re-clone rather than pull. Links to specific old commit SHAs no longer resolve, and
> the auto-generated "Source code" archives on old releases no longer contain the icon atlas
> (the prebuilt release `.zip` assets still do).

## Credits

Developed by [Tobias Mielich](https://github.com/TobiasM95).

Credits to [Dear ImGui](https://github.com/ocornut/imgui), which is the foundation of the
native GUI and very recommended, and to [Bloodmallet](https://bloodmallet.com/) for the
original idea. Also used: [libcurl](https://curl.se/libcurl/),
[stb](https://github.com/nothings/stb), [miniz](https://github.com/richgel999/miniz).

Talent data is sourced from [Raidbots](https://www.raidbots.com/). TTM is a fan project and is
not affiliated with or endorsed by Blizzard Entertainment.

## License

GPL-3.0 — see [LICENSE](LICENSE).
