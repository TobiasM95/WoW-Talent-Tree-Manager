![TTM Banner](/GUI/resources/TTM_Banner.png?raw=true "TTM Banner")

# WoW Talent Tree Manager

Create, explore and share World of Warcraft talent trees and builds — including an
exhaustive build solver that enumerates every valid talent combination under
constraints.

> **Status: a web application, hosted for free as a static site on Cloudflare Pages.**
> The native Windows client (v1.4.2) was discontinued in July 2024. It is preserved on the
> [`archive/native-client`](../../tree/archive/native-client) branch.

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
SimulationCraft does not support it). A third, **Custom**, is your own trees, designed in the
tree editor or copied from either game and changed. WoW Forever talent data is from
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

There is no server of ours. The site is static: tree data and icons are files, and counting and
listing builds run in the browser (`frontend/src/engine`). Two Cloudflare Pages Functions
(`frontend/functions/`) do what needs a server: WarcraftLogs, whose key must stay secret, and
saved custom projects, in Workers KV. See [`docs/03-plan/browser-only.md`](docs/03-plan/browser-only.md).

```bash
# the data: live talent data (Raidbots, DB2 point caps, descriptions) and WoW Forever
pip install -r services/ingest/requirements.txt
python services/ingest/ingest.py --point-caps --descriptions
python services/ingest/forever_ingest.py
python tools/site/build_data.py --out frontend/public \
  --game retail=data/generated --game forever=data/generated-forever

# the site, served the way Cloudflare serves it, functions included
cd frontend && corepack enable && pnpm install
pnpm run build && pnpm exec wrangler pages dev dist --port 8081      # http://localhost:8081
```

For top players, put `WCL_CLIENT_ID` and `WCL_CLIENT_SECRET` in `frontend/.dev.vars`
(git-ignored). For quick UI work, `pnpm run dev` serves on :5173 with `/api` proxied to
`wrangler pages dev` on :8788.

### Checking it

| | |
|---|---|
| `cd frontend && pnpm run test:all` | Every frontend suite, in a real browser, against the site on :8081 |
| `python tools/parity/engine_reference.py ...` + `node parity.test.mjs` | The browser engine against the C++ engine: counts and full listings, every tree and filter |
| `python tests/golden_counts.py build/ttm-solver` | The C++ engine against known-correct counts |
| `python tools/frontier-dp/test_counting.py` | The Python counting DP, the original reference |
| `python services/ingest/tests/test_ingest.py` | Transform and validation |

## Repository layout

| Path | What |
|---|---|
| `Engine/` | **The C++ solver and tree model.** The browser engine's maintained twin: the release CI requires the two to agree. |
| `CLI/` | Headless entry point to the engine, used by the parity suite and the golden counts. |
| `GUI/`, `AppUpdater/` | The native Dear ImGui client. Retained as the reference implementation. Not actively developed. |
| `docs/` | Analysis, target architecture, and plan. |
| `services/ingest/` | Live talent data into the tree format the site serves. Runs in CI. |
| `frontend/` | The site: React + Tailwind, the browser engine, the Pages Functions. See [`frontend/README.md`](frontend/README.md). |
| `tools/parity/` | The engine parity suite's reference generator and fixtures. |
| `tools/site/` | Builds the site's static data and icons from the ingest. |
| `tools/frontier-dp/` | The Python counting DP: the original reference, still used by the engine's tests. |
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
| `release.yml` | The browser engine agrees with the C++ engine; live data ingests; the site builds and every frontend suite passes against it, with a real SimulationCraft round trip; then it **publishes to Cloudflare Pages**. |
| `engine-tests.yml` | Golden counts from the C++ engine, and the Python DP agreeing with it. The 8-minute overflow case runs on tags only. |
| `ingest-tests.yml` | The ingest against live data, also **weekly**, since upstream changes shape on Blizzard's schedule, not ours. |
| `update_presets.yml` | Legacy, manual only. |

Locally, the same suites are `pnpm run test:all` in `frontend/`, against the site on :8081.

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
