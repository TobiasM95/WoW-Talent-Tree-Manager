# Revival roadmap

Status: proposal. Ordered to **de-risk the unknowns before building product surface**. The
tempting order (scaffold the web app first, integrate the engine last) is exactly backwards: the
engine integration and the data ingestion are the two things that can still fail in surprising
ways, and the legacy project died at the data layer while its UI was in good shape.

## Phase 0 — Spikes (de-risk first)

Small, throwaway, answer-a-question-only work. Nothing here ships.

| # | Spike | Question it answers | Done when |
|---|---|---|---|
| S1 | Build the engine + CLI on Linux via CMake in Docker | Is the port as cheap as the analysis says? | `ttm-solver` solves a `presets.txt` preset in a container and matches the Windows build's output |
| S2 | Transform one spec into the target tree JSON, hero sub-trees included | Does the transformation hold up end to end? | A complete JSON tree for one spec, hero sub-trees included, validated against the game |
| S3 | Cross-tree prerequisites + tiered nodes | How do we feed pre-satisfied prerequisites and level-gated ranks to a per-tree solver? (Q5, Q10) | A worker contract that handles both, with the 64-bit budget confirmed at max level |
| S4 | Blizzard hash round-trip | Can we import/export live in-game strings with the current codec? | An in-game export string imports correctly and re-exports byte-identically |

S1 and S2 are independent and can run in parallel.

**S2 is already largely de-risked**: the source was verified live and its full schema documented in
[`../02-target/raidbots-live-schema.md`](../02-target/raidbots-live-schema.md), so this is
transformation work, not discovery.

**S3 is now the highest-risk item.** Both of its inputs are confirmed to exist in live data —
cross-tree `requiresNode` targets, and `tiered` nodes whose max ranks depend on character level —
and both affect the solver's input, so they shape the worker contract. Neither requires an
algorithm redesign if handled as pre-resolution before the solve, which is what this spike should
establish.

Rationale for S4: the codec zero-fills the 128-bit `tree_hash` and pins the version byte to `1`.
Whether real current-patch strings still round-trip is unverified, and Blizzard-string interop is
a headline feature.

## Phase 1 — Data foundation

Build the pipeline before the product, because everything downstream is shaped by the tree JSON.

- Postgres schema: `trees` (revisioned), `builds`, `loadouts`, plus constraints and real FKs.
- Ingest container: fetch → **validate against schema** → transform → write a new tree revision →
  promote only on success. Alert loudly on failure; track `last_successful_ingest_at`.
- Primary source: Raidbots `talents.json`; fallback: wago.tools raw DB2 CSVs (`TraitNode`,
  `TraitEdge`, `TraitCond`, `TraitSubTree`). Write the transform against an internal
  source-agnostic intermediate so switching is a swap, not a rewrite. Do **not** use simc as the
  layout source — it never parses `TraitEdge`. See
  [`../02-target/talent-data-sources.md`](../02-target/talent-data-sources.md).
- All 39 specs × (class + spec + hero sub-trees) ingested and spot-checked against the live game.
- Icon pipeline: individual files, content-hashed, served behind a cache. No packed atlas, no
  binaries in git.
- Importers for the legacy formats (TTM tree string, TTM skillset string, Blizzard hash, SimC
  string) so existing users' saved data isn't orphaned — read-only, one-way, into the new JSON.

Exit criteria: a fresh ingest run reproduces every spec's trees correctly, and a deliberately
malformed upstream payload fails the run loudly instead of writing bad data.

## Phase 2 — Engine as a service

- CMake build retained alongside the `.vcxproj` files, so the native app still builds.
- Replace `<Windows.h>` dependencies: `--mem-budget` argument instead of `GlobalMemoryStatusEx`;
  drop `SHGetKnownFolderPath`/`%APPDATA%` entirely; `std::thread` instead of PPL.
- Add what the worker contract needs: NDJSON streaming output (incrementally flushed), a progress
  file driven by the existing `runningCount`, meaningful exit codes, and the **wall-clock
  `--time-budget` that does not exist today**.
- `solve_jobs` queue in Postgres with `FOR UPDATE SKIP LOCKED`, lease-based crash recovery, and
  `request_hash` dedup/caching.
- Worker container with hard CPU/memory limits.
- API pre-validates tree size (the 64-slot ceiling) before spawning, so an oversized tree is a
  clean 4xx and never a terminated worker.

Exit criteria: a solve submitted over HTTP returns streamed, paginated results; an identical
second request is served from cache without recomputation; a deliberately oversized or
pathological job is capped cleanly rather than taking down a container.

## Phase 3 — Core product (Loadout Editor + Solver)

The first genuinely user-facing phase.

- Tree canvas (SVG), pan/zoom, per-class theming, talent tooltips with descriptions.
- Point spending: left-click adds, right-click removes (the convention players expect), choice-node
  toggling, gating and prerequisite validation, level cap, point-budget display.
- Hero sub-tree selection.
- Loadout management: multiple named builds per tree.
- Solver UI: constraint painting (must-have / must-not-have / one-of), point-total filter,
  asynchronous job submission with progress, paginated results, transfer results into a loadout.
- Import/export: Blizzard hash, SimC string, and `ttm1.` share codes.
- Anonymous use works end to end; share-by-URL works without an account.

Exit criteria: a player can land on the site, build a spec, solve it under constraints, and share
a link — without signing in.

## Phase 4 — Accounts and persistence

- Session cookies, server-side sessions, Battle.net OAuth plus optional email/password.
- Saved trees/loadouts, workspace, public/private sharing.
- Stale-build detection: a build pins `treeRevision`, so surface "this build predates patch X" and
  offer migration.

## Phase 5 — Beyond parity

Only now, once the foundation holds:

- Tree Editor (custom/homebrew trees) — the native app's authoring surface.
- Sim Analysis rebuilt on SimC's JSON report (open question Q9).
- Classic support (open question Q3).
- Popular builds from WarcraftLogs — the one genuinely good idea in the legacy web app.
- Engine improvements, which are far easier once it is under test in CI with a stable contract.

## Sequencing notes

- **Test the engine contract in CI from Phase 2 onward.** The legacy project had zero tests, and
  the trickiest logic (gating validation, hash bit-packing, divider placement) is exactly the kind
  that fails silently. A golden-file test comparing solver output for a fixed set of presets is
  cheap and would catch most regressions.
- **Keep the native client building.** It is the reference implementation and the only way to
  verify the rewrite's correctness for a long while. Do not delete `GUI/` yet.
- **Retire `Web/` at the start of Phase 1**, not before — read it for salvageable ideas first (see
  [`../01-current-state/legacy-web-app.md`](../01-current-state/legacy-web-app.md)), then delete
  it in one commit so it stops being ambiguous whether it is live.
- Decide open questions Q1, Q2, Q4 and Q6 before Phase 1 starts; they change what gets built. Q3,
  Q7, Q8, Q9 can wait.
