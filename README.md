![TTM Banner](/GUI/resources/TTM_Banner.png?raw=true "TTM Banner")

# WoW Talent Tree Manager

Create, explore and share World of Warcraft talent trees and builds — including an
exhaustive build solver that enumerates every valid talent combination under
constraints.

> **Status: being rebuilt as a web application.**
> The native Windows client (v1.4.2) was discontinued in July 2024. It is preserved on the
> [`archive/native-client`](../../tree/archive/native-client) branch. Work in progress; not
> yet deployable.

## What this is

TTM's distinctive feature is its **solver**: given a talent tree and constraints (a point
budget, must-have and must-not-have talents), it enumerates *all* valid builds. It does this
with a topologically sorted minimal DAG and a `uint64_t` bitset, visiting only
strictly-increasing indices so no deduplication pass is ever needed. That engine is fast,
correct, and being kept — it is the reason this project is worth reviving.

Everything around it is being replaced.

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

## Repository layout

| Path | What |
|---|---|
| `Engine/` | **C++ solver and tree model.** Kept, being made portable. The crown jewel. |
| `CLI/` | Headless entry point to the engine; becoming the server-side worker binary. |
| `GUI/`, `AppUpdater/` | The native Dear ImGui client. Retained as the reference implementation while the web app catches up. Not actively developed. |
| `docs/` | Analysis, target architecture, and plan. |
| `services/` | New backend services (ingest, api, worker). |

The `WoW Talent Manager.sln` still builds the native client in Visual Studio 2022. A portable
CMake build for `Engine` + `CLI` is being added alongside it so the solver can run in Linux
containers.

## Branches

| Branch | Purpose |
|---|---|
| `master` | Web application development. |
| `archive/native-client` | The native client at v1.4.2, frozen. |
| `archive/server-deploy` | Old deployment/domain setup for the abandoned web attempt. |

### A note on history

This repository's history was rewritten to remove generated binary artifacts. A daily CI job
committed a 16 MB packed icon atlas 76 times, which had grown `.git` to **1.4 GB** against a
90 MB working tree. Stripping the three atlas paths and stale debug symbols reduced it to
**9.4 MB** — a 99.3% reduction — while preserving all 597 commits and 21 tags.

What was removed is *generated data*, regenerable by the preset scripts. Nothing authored was
lost. Commit SHAs changed, so an old clone cannot fast-forward; re-clone instead.

> **Careful: do not `git fetch` until the remote is rewritten.**
> `origin` still holds the old 1.4 GB history. Because the rewritten commits have different
> SHAs, a fetch re-downloads all of it into `refs/remotes/origin/*` and puts `.git` straight
> back to 1.4 GB. If that happens: `git remote remove origin`, then
> `git reflog expire --expire=now --all && git gc --prune=now`, then re-add the remote.
>
> Publishing the rewritten history means a force-push, which rewrites 21 release tags and
> breaks every existing clone. That is a deliberate decision, not a routine one — the local
> history is complete and backed up, so there is no hurry.

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
