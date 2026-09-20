# WoW Talent Tree Manager — revival documentation

Analysis and planning for reviving TTM as a web application. Written before implementation
starts, so the decisions are made against evidence rather than recollection.

**Read [`03-plan/open-questions.md`](03-plan/open-questions.md) first** if you only read one file.
It separates what the analysis settled from what still needs a decision.

## Layout

| Directory | Contents | Nature |
|---|---|---|
| `01-current-state/` | What exists today, from reading the code | **Findings** — cited to `file:line` |
| `02-target/` | What to build | **Proposals**, except where marked verified |
| `03-plan/` | Sequencing and decisions | **Proposals** |

### 01 — Current state

| Document | Covers |
|---|---|
| [`gui-feature-inventory.md`](01-current-state/gui-feature-inventory.md) | Every feature of the Dear ImGui client, view by view, plus a prioritised web-parity checklist (MUST/SHOULD/DROP) and the gesture vocabulary that needs redesigning |
| [`engine-and-solver.md`](01-current-state/engine-and-solver.md) | The variant-counting algorithm and why it is fast; threading reality; resource limits; the verified 64-bit ceiling analysis; the existing CLI; Linux portability |
| [`data-formats.md`](01-current-state/data-formats.md) | Every persisted and shareable format, with grammars: TTM tree/skillset strings, `presets.txt`, `node_id_orders.txt`, the Blizzard loadout hash bit layout, SimC strings, local file storage |
| [`data-pipeline-and-scraper.md`](01-current-state/data-pipeline-and-scraper.md) | How talent data was acquired, the full external dependency list, and an itemised fragility analysis of how and why it broke |
| [`legacy-web-app.md`](01-current-state/legacy-web-app.md) | Post-mortem of the abandoned `Web/` app: stack, routes, DB schema, maturity, and what to salvage versus avoid |

### 02 — Target

| Document | Covers |
|---|---|
| [`explainers/frontier-sweep.html`](explainers/frontier-sweep.html) | **Interactive.** Step-by-step walkthrough of the frontier sweep: why counting in groups is exact, what plays the part of a grid cell and its neighbours, and how point barriers survive the forgetting. Computes the DP and a brute-force check live in the page |
| [`solver-performance.md`](02-target/solver-performance.md) | **Measured (2026-09-19).** The solver's real cost curve, and why counting and enumerating must be separated before the solver is exposed on a server |
| [`raidbots-live-schema.md`](02-target/raidbots-live-schema.md) | **Verified against live data (2026-09-19).** The upstream talent schema as it actually is today, including hero talents, the new `tiered`/level-gated mechanic, cross-tree prerequisites, and data-quality traps |
| [`data-model.md`](02-target/data-model.md) | Proposed JSON schemas for trees, builds and loadouts; the identifier-stability rule; interop-format keep/replace decisions; Postgres mapping |
| [`architecture.md`](02-target/architecture.md) | Container topology, the Postgres-based queue, the engine worker protocol, frontend approach, ingestion requirements, auth, and what is deliberately excluded |
| [`ui-direction.md`](02-target/ui-direction.md) | The interface should feel like it belongs to a fantasy game: what to draw from in WoW's own visual language, and which default web looks are explicitly rejected |
| [`talent-data-sources.md`](02-target/talent-data-sources.md) | Comparison of SimC, the Blizzard Game Data API, and community sources; retail vs. classic; why simc is the wrong primary source and what to use instead |

### 03 — Plan

| Document | Covers |
|---|---|
| [`open-questions.md`](03-plan/open-questions.md) | Resolved questions (so they stay resolved) and the open decisions, with recommendations |
| [`roadmap.md`](03-plan/roadmap.md) | Phase 0 de-risking spikes through Phase 5, with exit criteria per phase |

## The short version

**Why it died.** Not the scraper. The last successful data update was `2024-07-05`
(`resource_versions.txt`), at the TWW pre-patch. Raidbots' `talents.json` — the actual upstream
source — is still live today and *already publishes* `heroNodes`/`subTreeNodes`. The generator only
ever read `classNodes`/`specNodes`, and TTM's model hardcodes exactly two trees per spec (78
presets = 39 specs × 2, joined by `complementaryTreeIndex`). Hero talents had nowhere to go. It was
a data-model gap wearing a broken-scraper costume.

**What's worth keeping.** The C++ engine. It is genuinely well-designed: a topologically sorted
minimal DAG plus a `uint64_t` bitset, enumerating only strictly-increasing indices so no
deduplication pass is ever needed. Keep it as a server-side command-line worker. Porting it to
Linux is cheap — no SIMD, no MSVC intrinsics, three `<Windows.h>` includes with one real
dependency.

**The trap to avoid.** Stored and shared data is *positional*. Skillsets are a bare list of point
values matched by iterating talents in the same order on both sides, validated only by count —
while the preset generator re-indexes every node on every regeneration. Shared builds can silently
misassign points. Everything persisted must be keyed by Blizzard `nodeId`, and builds must pin a
tree revision.

**On using SimC for talent data.** Tempting, but wrong for this purpose. SimC keeps node positions
and `req_points` but **never parses `TraitEdge`** — it discards the prerequisite-arrow graph, because
validating a legal point spend is all it needs. Use Raidbots `talents.json` as the primary source
(full graph, hero trees, free, no auth), wago.tools raw DB2 CSVs as the fallback and long-term
pipeline, and simc only for spell/effect cross-checks.

**What's new since the app was maintained.** Demon Hunter has a third spec (Devourer), so 40 specs
where the code assumes 39. The level cap is 90, where the engine defaults to 70. And there is a new
`tiered` node type whose maximum ranks depend on *character level* — a mechanic the engine cannot
currently express at all.

**The number that shapes the product.** An exhaustive 30-point spec-tree solve takes **7.5 minutes
and writes 9.5 GB**; at 20 points it is 0.12 s and 1.35 MB. The engine stored every combination
purely in order to count them. A count-only mode (now implemented) drops peak memory ~176x
(1,583 MB to 9 MB) at the same speed, which makes large counts possible at all. For speed,
enumeration is the wrong tool: a frontier DP counts the same sets in **3.7 s for all 78 presets at
every budget**, versus 42 s for the engine to count one budget of one tree. Listing builds remains
the expensive operation, and the product should treat counting and listing very differently.

**Where to start.** Phase 0 spikes: build the engine on Linux, transform one spec end to end, and
settle how pre-satisfied prerequisites and level-gated ranks reach a per-tree solver. Then the data
foundation, then the engine-as-a-service, then product surface — deliberately the reverse of the
tempting order, because the data layer is where this project died the first time.

## Reading these documents

- `01-current-state/` is cited to `file:line` throughout; claims there are checkable against the
  code.
- `raidbots-live-schema.md` is reproducible — it includes the commands used.
- Everything else is a recommendation, and says so. Where a recommendation is contested or
  uncertain, it is listed in `open-questions.md` rather than asserted.
