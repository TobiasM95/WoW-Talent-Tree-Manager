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

### Worker — done

[`../../services/worker/README.md`](../../services/worker/README.md). Claims jobs with
`FOR UPDATE SKIP LOCKED`, execs `ttm-solver`, decodes output into nodeId-keyed builds, and
recovers jobs whose lease expired. `capped` is a first-class outcome for a truncated result.

The engine gained the **wall-clock `--time-budget-ms`** it never had: a 30-point solve that
runs 38 s stops at 2.00017 s under a 2 s budget and reports `INCOMPLETE`, while a fast solve
under a generous budget is unaffected.

**Phase 2 exit criteria met.** A solve submitted over HTTP returns paginated results; an
identical request is served from the existing job rather than recomputed; an oversized job is
refused with a 413 before it is ever queued.

**All three items that were open here are now done.**

- **Group filters** reach the engine. `atLeastOneOf` and `exactlyOneOf` are counted by
  inclusion-exclusion in the DP and passed to the engine as its `-2`/`-3` sentinels, which
  agree exactly (6,135 and 1,625 at 14 points on Balance Druid). Counting supports any number
  of groups; listing supports one of each kind, because the engine's filter holds a single
  value per talent, and `/solve` refuses more rather than silently dropping one.
- **Progress** is reported per phase over stderr, not a progress file. Building it found the
  more interesting problem: the engine is not the slow part. A 25-point spec solve enumerates
  1,906,208 selections in 0.11 s and then had not finished storing them after 25 minutes at
  one INSERT per row. With `COPY` the job takes about 16 s, and progress is reported as
  `solving` / `storing` / `finalizing` because a single bar driven by the solver would sit at
  100% for almost the whole wait.
- **Cancellation** is a request rather than a state: a worker holds the row and a solver
  process is running, and neither stops because a table changed. The worker learns about it
  on the progress write it already makes once a second.

## Phase 3 — Core product (Loadout Editor + Solver)

The first genuinely user-facing phase. **Mostly done**; see
[`../../frontend/README.md`](../../frontend/README.md).

Done:

- **Tree canvas** with pan, zoom, per-class tinting, node silhouette carrying node type, and
  tooltips with per-rank descriptions. Both themes, desktop and phone.
- **Constraint painting** — must-have, must-not-have, choice sides, at-least-one and
  exactly-one — with a **live exact count** that updates as constraints land, and a pre-flight
  gate that disables enumeration when the result would be too large to list.
- **Job submission** with per-phase progress, cancellation, and a results browser that paints
  each enumerated build onto the tree rather than listing rows of numbers.
- **Per-talent statistics** over the whole matching set: which talents the constraints have
  already decided, and where the choice actually is. Cross-checked against the DP.
- **Share by URL**, carrying the tree, the budget, every constraint and the build being
  inspected — 124 characters for a 27-point build with nine constraints. No account needed.

- **Point spending by hand**, across all three trees at once, under the same rules the solver
  counts under. The rules are lifted from the counting DP rather than from a reading of the
  game, and the test suite asserts the agreement the only way that means anything: it builds
  loadouts and asks the API to count them.
- **Hero sub-tree selection**, and with it the multi-tree view.

Still to do in this phase:

- **Loadout management**: multiple named builds, which needs somewhere to keep them — so it
  waits on Phase 4.
- **Import/export**: the Blizzard talent string is done, in both directions -- paste the
  build you are playing, change it, paste it back. **Verified against a string the game
  exported**, which decodes and re-encodes byte for byte. SimC export builds on the same
  string and is done too: an enumerated result set exports as profilesets, one line per
  build, each a whole character rather than one tree.
- **`ttm1.` share codes** are superseded by the URL, which already carries the whole view.

**Reworked around the workflow, not the engine.** The first version exposed the solver's
one-tree-at-a-time nature as the product: a "baseline" loadout had to be hand-built before
anything could be exported, and two global modes (Build, Explore) held disjoint state that
vanished when switching. Now each tree is independently fixed or open, the character space
is their product, and the app is three steps -- Narrow, Simulate, Analyse -- with analysis as
a page of its own. Choice sides are expanded into separate builds, matching the API's count
exactly. See [`../../frontend/README.md`](../../frontend/README.md).

Building that found a data bug under everything: the CLI reported result bits ranked over
the solved DAG, which excludes granted roots, so every stored build on a class or hero tree
named each talent by its neighbour's id. Counts were never affected, which is why no count
test saw it. Fixed in the CLI, stale results dropped by migration 007, and guarded by a
legality check on stored builds (see [`../../services/worker/README.md`](../../services/worker/README.md)).

**Phase 3 exit criteria met.** A player can land on the site, build a spec across all three
trees, solve it under constraints, and share a link — without signing in.

## Phase 4 — Accounts and persistence

- Session cookies, server-side sessions, Battle.net OAuth plus optional email/password.
- Saved trees/loadouts, workspace, public/private sharing.
- Stale-build detection: a build pins `treeRevision`, so surface "this build predates patch X" and
  offer migration.

## Phase 5 — Beyond parity

Only now, once the foundation holds:

- Tree Editor (custom/homebrew trees) — the native app's authoring surface.
- Sim Analysis: **done, both legs.** An enumerated result set becomes SimulationCraft
  profilesets, one line per build, each a whole character rather than one tree -- feasible
  precisely because the counting gate bounds the set to something simmable. SimC's JSON
  report then reads back in the browser, ranks the builds, and attributes a value to each
  talent: the mean of the builds taking it against the mean of those that do not. That last
  number is the one the whole arc exists to produce, and it answers Q9 -- a sim ranks whole
  characters, so a single talent's worth only appears across a controlled set of builds,
  which is exactly what an enumeration under constraints is.
  Built against a report SimulationCraft actually wrote rather than against a reading of its
  schema, and `test:round` re-runs the entire loop -- export, sim, import -- wherever the
  SimC container is available. The UI states both limits beside the numbers: differences
  smaller than the sim's own error bar are ties rather than an order, and talents the tree
  never separates share a score.
  **Per-talent frequency statistics** are a separate thing and did not need SimC at all --
  they are a property of the enumeration, not of the sim.
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
