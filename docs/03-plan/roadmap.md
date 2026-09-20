# Revival roadmap

Status: proposal. Ordered to **de-risk the unknowns before building product surface**. The
tempting order (scaffold the web app first, integrate the engine last) is exactly backwards: the
engine integration and the data ingestion are the two things that can still fail in surprising
ways, and the legacy project died at the data layer while its UI was in good shape.

## Phase 0 — Spikes (de-risk first)

Small, throwaway, answer-a-question-only work. Nothing here ships.

| # | Spike | Question it answers | Done when |
|---|---|---|---|
| S1 | ~~Build the engine + CLI on Linux via CMake in Docker~~ | ~~Is the port as cheap as the analysis says?~~ | **DONE** — byte-for-byte identical output (same sha256) from gcc 12 and MSVC v143 on `druid_restoration`. See the commit and [`../02-target/solver-performance.md`](../02-target/solver-performance.md). |
| S2 | ~~Transform one spec into the target tree JSON, hero sub-trees included~~ | ~~Does the transformation hold up end to end?~~ | **DONE, and generalised past one spec** — all 40 specs transform into 160 trees (40 class, 40 spec, 80 hero), 4,567 nodes, with validation and tests. See [`../../services/ingest/`](../../services/ingest/README.md). |
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

- ~~Postgres schema: `trees` (revisioned), `builds`, `loadouts`, plus constraints and real FKs~~
  **DONE** ([`../../services/db/README.md`](../../services/db/README.md)). Revisioned trees,
  nodeId-keyed builds pinning a revision, precomputed `tree_counts`, and the
  `FOR UPDATE SKIP LOCKED` job queue with dedup. Loaded via a promote-on-success loader; a
  partial load leaves `current_trees` serving the previous revision. Smoke test asserts the
  constraints actually reject bad data.
- ~~Ingest: fetch → validate → transform → write a new revision → promote only on success~~
  **DONE** ([`../../services/ingest/`](../../services/ingest/README.md)). Fatal on unknown node
  or entry types, missing fields, broken graphs or duplicate keys; warns on class/spec roster
  drift rather than hardcoding it. Output is staged and swapped in only after validation, so a
  bad run leaves the previous revision intact. A daily CI job runs it against live data so an
  upstream shape change surfaces the day it happens.
- Still to do here: alerting on a stale `ingest_runs.promoted_at`, and the icon pipeline.
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
- **Count-only solve mode — done**, but it buys memory (~176x, 1,583 MB to 9 MB), not speed.
  Enumeration still walks every solution. It makes large counts *possible*; it does not make them
  fast.
- **Must-have pruning — done.** `visitTalentFiltered` pruned only on must-not-have; must-have was
  tested once per completed path, so "I want these three talents" paid for a full enumeration and
  discarded almost all of it. Two bitmask tests fix it (a passed-over required talent is
  unreachable; each owed talent costs a point). Filtered search is now output-sensitive: 33.5x on
  a realistic filter at 30 points, 1.1 s instead of 37.9 s.
- **Counting should not go through the enumerator at all.** A frontier DP counts the same sets in
  time polynomial in tree size: 3.7 s for all 78 presets at every budget, versus 42 s for the
  engine to count one budget of one tree, with counts verified identical on 11 trees. Prototyped
  in [`../../tools/frontier-dp/`](../../tools/frontier-dp/README.md). Productionising it means
  counting is answered inline in the API in milliseconds, precomputable at ingest, with no queue
  or worker involved. Extend it for hero talents and `tiered` level-gated ranks first.
- Add what the worker contract needs: NDJSON streaming output (incrementally flushed), a progress
  file driven by the existing `runningCount`, meaningful exit codes, and the **wall-clock
  `--time-budget` that does not exist today**.
- Lower the default safety guard. At 500,000,000 it did not trip on a solve that produced 305
  million combinations and a 9.5 GB file, so it protected nothing. Derive it from the output
  budget we are willing to serve, per job.
- Emit resolved, nodeId-keyed rows. The current format is raw decimal `SIND` integers whose
  meaning depends on a separate header line, so no row is self-describing.
- `solve_jobs` queue in Postgres with `FOR UPDATE SKIP LOCKED`, lease-based crash recovery, and
  `request_hash` dedup/caching.
- Worker container with hard CPU/memory limits.
- API pre-validates tree size (the 64-slot ceiling) before spawning, so an oversized tree is a
  clean 4xx and never a terminated worker.

Exit criteria: a solve submitted over HTTP returns streamed, paginated results; an identical
second request is served from cache without recomputation; a deliberately oversized or
pathological job is capped cleanly rather than taking down a container.

### Still to do in this phase

- **Allocation in the hot path.** `possibleTalents` is passed by value, so every recursion node
  copies a vector. This is a constant-factor win on its own and the precondition for any useful
  threading — malloc contention is the likely reason earlier parallel attempts showed no gain.
- **Then parallelism**, if still wanted: sequential descent to a shallow depth cut produces
  thousands of independent subproblems for a work-stealing pool. Partitioning by the
  lowest-index selected talent gives a provably disjoint cover, so threads never overlap and
  output needs no merge. Note the functions named `countConfigurationsParallel` are *not*
  parallel, and the PPL usage parallelised across trees, not within one — which is very likely
  why earlier attempts never sped up a single-tree solve.
- **Confirm the filter language against choice-node sides** (open question Q11).

### API — count endpoint done

[`../../services/api/README.md`](../../services/api/README.md). `/health` (with data age),
`/trees`, `/trees/{key}`, `/trees/{key}/counts`, and `POST /counts` — the pre-flight gate,
answering filtered counts in 10-38 ms on a tree with 872 million builds, with `listable`
telling the UI whether enumerating is worth offering. 16 tests.

Still missing from Phase 2: the worker that claims a `solve_jobs` row and execs
`ttm-solver`, NDJSON streaming, progress reporting, and the wall-clock `--time-budget`.

## Phase 3 — Core product (Loadout Editor + Solver)

The first genuinely user-facing phase.

- Tree canvas (SVG), pan/zoom, per-class theming, talent tooltips with descriptions.
- Point spending: left-click adds, right-click removes (the convention players expect), choice-node
  toggling, gating and prerequisite validation, level cap, point-budget display.
- Hero sub-tree selection.
- Loadout management: multiple named builds per tree.
- Solver UI built around the count-first flow: constraint painting (must-have / must-not-have /
  at-least-one / exactly-one) with a **live exact count** updating as constraints are painted,
  per-talent marginals ("requiring this drops you to 1,204"), a pre-flight gate refusing filters
  too broad to sim, then job submission with a real progress bar, results, and transfer into a
  loadout.
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
- Sim Analysis rebuilt on SimC's JSON report (open question Q9): export the filtered build set as
  profilesets, import results back, rank builds, and show per-talent statistics over that set.
  Feasible precisely because the filter bounds the set to something simmable.
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
