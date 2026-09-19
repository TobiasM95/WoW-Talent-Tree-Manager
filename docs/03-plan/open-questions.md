# Open questions and decisions

Two lists: questions genuinely still open (need a decision or a spike), and questions the
analysis already **closed** — recorded so they don't get re-litigated later.

## Resolved by the analysis

| # | Question | Answer |
|---|---|---|
| R1 | Why did the project actually die? | A data-model gap, not a scraper bug. Raidbots' `talents.json` already publishes `heroNodes`/`subTreeNodes`; the generator only read `classNodes`/`specNodes`, and TTM hardcodes exactly two trees per spec. Last successful data update: `2024-07-05` (TWW pre-patch). |
| R2 | Does the 64-bit solver bitset block modern trees? | No, **if** each tree is solved independently. Worst case across all 79 shipped presets is 60 bits. A combined class+spec+hero solve (~75-80 bits) would need widening; nothing requires one. |
| R3 | Is the C++ engine portable to Linux/Docker? | Yes, cheaply. No SIMD or MSVC intrinsics; all bit ops are portable `uint64_t` shifts. Three `<Windows.h>` includes, one real dependency (`GlobalMemoryStatusEx`), one PPL usage, plus a CMake build to write. |
| R4 | Is the engine actually multithreaded? | No. `countConfigurationsParallel` is single-threaded — the name refers to computing all point totals in one pass. The only real parallelism is MSVC PPL in `CLI.cpp:271`, solving multiple trees concurrently. |
| R5 | Is anything in the legacy web app worth keeping? | Very little code. The engine was **never integrated** (no subprocess, no queue), and the write path is entirely stubbed. Salvage the ideas: reactflow-style rendering, the dense-id/stable-id split, copy-vs-import, WarcraftLogs popular builds. |
| R6 | Can shared builds be stored positionally? | No. Skillset strings are positional with count-only validation, and the preset generator re-indexes nodes on every regeneration. Shared builds can silently misassign points. Everything stored or shared must be nodeId-keyed. |
| R7 | Does the solver need a wall-clock limit? | Yes, and it has none today — only a combination-count guard and a memory guard. Unacceptable for a shared server; must be added. |
| R8 | Is the raidbots data source still usable? | **Yes, verified live 2026-09-19**: HTTP 200, 3.2 MB, and it already carries `heroNodes`/`subTreeNodes`, layout, and gating. See [`../02-target/raidbots-live-schema.md`](../02-target/raidbots-live-schema.md). |
| R9 | Are the engine's hardcoded class/spec enums still valid? | No. Demon Hunter now has a third spec (**Devourer**), so there are 40 specs where the legacy data had 39. Derive the class/spec set from data; never hardcode. |

## Open — needs a decision

### Q1. Scope of the first release

The native app has four major surfaces: Tree Editor (authoring custom trees), Loadout Editor
(spending points), Loadout Solver (enumeration), Sim Analysis (DPS heatmaps). Shipping all four
is a large first release.

Recommendation: **Loadout Editor + Solver first.** The solver is the unique, defensible value —
nothing else on the web does it. The Tree Editor is a niche feature (custom homebrew trees) and
Sim Analysis depends on a fragile import pipeline that needs redesign rather than porting.
Needs confirmation, because it determines roughly half the work.

### Q2. Anonymous-first, or accounts-first?

Requiring an account to view or build is the fastest way to lose the audience. But saved
loadouts, sharing, and solve-job history all imply identity.

Recommendation: anonymous core loop, with share-by-URL working without an account; accounts
purely additive (save, name, organise). Needs confirmation.

### Q3. Retail first, or retail+classic together?

The stated preference is "get a hold of retail first", which is right. The real question is
whether the *schema* should accommodate classic from day one. Classic's 3-tab / 51-point /
row-gated system is a different gating model (points-in-*this-tab* gating, strict row unlocks).

The research narrows this considerably. Classic's gating is **qualitatively different**, not just
smaller: `Talent.dbd` embeds prerequisites as direct FK columns on the talent row
(`PrereqTalent[3]`) with tier/column/tab layout, so gating is a mandatory prereq chain plus
points-in-*this-tab* thresholds — whereas in retail the edges are largely cosmetic and the real gate
is `reqPoints`. Also relevant: simc has **no genuine Classic support** (its `mop`/`tbc`/etc.
branches are frozen *retail* snapshots, and Classic Support is an open unresolved epic), and
Blizzard's Classic API namespaces appear to have no talent endpoints at all.

Recommendation: design the schema with `game` and `pointCap` per tree from the start (already in
[`../02-target/data-model.md`](../02-target/data-model.md)), and make the **gating mechanism an
explicit pluggable concept** rather than assuming edges are decorative — that is the one design
choice that decides whether one schema can span both eras. Implement and test retail only; if
classic happens, hand-import once from the `Talent.dbd` shape rather than building an automated
per-patch pipeline for it.

### Q4. Primary talent-data source — **mostly answered**

The decisive criterion was which source carries full **layout and gating** data (positions, edges,
`pointsRequired`, choice nodes, hero sub-trees), not just spell data. Answered two independent
ways — by reading simc's extraction source, and by fetching live data
([`../02-target/talent-data-sources.md`](../02-target/talent-data-sources.md),
[`../02-target/raidbots-live-schema.md`](../02-target/raidbots-live-schema.md)):

- **SimC is the wrong primary source.** It keeps `TraitNode.PosX/PosY` bucketed into a grid and
  real `req_points` from `TraitCond`, but **never parses `TraitEdge`** — it discards the
  prerequisite-arrow graph entirely, because it only needs enough to validate legal point spend.
  Good for spell/effect cross-checks; not for rendering a tree. This answers the original
  "use simc instead of a scraper" idea: no, not for layout.
- **Raidbots `talents.json` is the recommended primary.** Verified live: full graph with
  `posX`/`posY`, explicit `next[]`/`prev[]` edges, choice entries, hero sub-trees, and gating.
  Free, no auth, no pipeline cost.
- **wago.tools raw DB2 CSVs** (`TraitNode`, `TraitEdge`, `TraitCond`, `TraitSubTree`), with schemas
  tracked via WoWDBDefs, are the fallback and the long-term answer to owning the pipeline rather
  than depending on one hobby endpoint.
- **Blizzard's Game Data API** is best used as a validator and for character import, not as the
  canonical feed (OAuth overhead, unverified rate limits).

Still open: **descriptions.** Raidbots carries `spellId`, `icon` and names but *not* tooltip text,
which is why the legacy pipeline scraped Wowhead HTML (`<div class="q">`) — unversioned, and it
degraded silently to "Description not available". Descriptions are required product content, so
this needs a real answer. Candidate approaches: Blizzard's spell endpoints, wago.tools spell
description tables, or rendering from spell data. Needs a decision.

### Q5. Cross-tree prerequisites — **answered: yes, they exist**

Verified against live data ([`../02-target/raidbots-live-schema.md`](../02-target/raidbots-live-schema.md)):
96 nodes carry a real `requiresNode`, and for hero nodes the target can live in a **sibling spec's**
tree, because a hero sub-tree is shared between two specs of a class. Druid Balance's node 94608
requires 92587, which is in Druid *Guardian*'s spec nodes.

What remains open is only the **design response**, not the fact. Recommended: supply a
pre-satisfied prerequisite mask per solve job, so the engine stays strictly per-tree and needs no
algorithm change. Confirm this during spike S3 rather than investigating from scratch.

### Q10. How should tiered nodes (level-gated ranks) be modelled in the solver?

New, and not anticipated by any of the current-state analysis. Live data has exactly 40 `tiered`
nodes — one per spec — whose `maxRanks` varies by character level via `rankLevels`
(e.g. 1 rank at 81, 3 at 84, 4 at 90).

The engine's `Talent::maxPoints` is a static integer baked into the bitset slot count by
`expandTreeTalents`, so effective max ranks must be resolved against the build's level cap
*before* the tree reaches the solver. That makes the expanded slot count — and therefore the
64-bit budget — depend on level cap. Needs a decision on where that resolution happens (ingest,
API, or worker) and confirmation that the worst case still fits 64 bits at max level.

### Q6. Fresh repository, or continue this one?

`.git` is 1.4 GB against a 90 MB working tree, because CI committed a 16 MB `icons_packed.png` 76
times. A clone is painful.

Options: (a) fresh repo, archive the old one; (b) `git filter-repo` to strip the atlas blobs;
(c) live with it. Recommendation: (b) then keep the history — the native client's history is
genuinely valuable context — but (a) is defensible if a clean break is preferred. Either way,
binary assets must not go back into git.

### Q7. Licensing

Currently GPL-3. Fine for a hosted service (GPL, unlike AGPL, does not compel releasing server
modifications), but worth a deliberate choice: keeping GPL-3, relicensing, or splitting licences
between engine and web app. Only the copyright holder can decide.

### Q8. Hosting and cost envelope

Solve jobs are CPU- and memory-hungry, and enumeration caps interact directly with hosting
budget. The `--max-results` / `--mem-budget` / `--time-budget` defaults are effectively a product
decision about how generous a free solve is. Needs a target: single VPS, or something that
scales workers?

### Q9. Is Sim Analysis worth reviving at all?

It is the most complex surface and the least functional today — Raidbots URL fetch is disabled in
shipped code, and the result parser scrapes plain text by literal prefix. Reviving it means
rebuilding on SimC's JSON report.

Recommendation: defer past the first release, then reassess against actual user demand. Flagged
because it is a large piece of the native app's surface that the parity checklist should not
silently assume.
