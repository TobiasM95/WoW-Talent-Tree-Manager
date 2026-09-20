# Database

Postgres carries the data, the job queue and the cache. There is no Redis, no broker and no
separate cache to operate — see
[`../../docs/02-target/architecture.md`](../../docs/02-target/architecture.md).

```bash
docker compose up -d postgres
docker compose run --rm migrate                    # apply migrations
docker compose run --rm ingest --descriptions      # fetch upstream data
docker compose run --rm loader                     # load trees + precompute counts
bash services/db/smoke_test.sh                     # end-to-end check
```

## Shape

| Table | Holds |
|---|---|
| `ingest_runs` | One row per ingest attempt. `promoted_at` is what makes a revision live. |
| `trees` | One row per tree **revision**; the node graph in `definition jsonb`. |
| `tree_counts` | Precomputed build counts per tree, budget and level cap. |
| `users`, `sessions` | Minimal identity. The core loop works anonymously. |
| `loadouts`, `builds` | Saved builds, keyed by Blizzard `nodeId`. |
| `solve_jobs`, `solve_results` | The filtered-enumeration queue and its output. |

`current_trees` is a view returning only the highest **promoted** revision — that is what
the API should read, never `trees` directly.

## Why it looks like this

**Trees are immutable revisions, and builds pin one.** This is the fix for the defect that
made shared builds unsafe: the legacy format stored point assignments *positionally*, and
the preset generator re-indexed nodes on every regeneration, so a data update could
silently reassign someone's points to different talents. A build now records
`(tree_id, tree_revision)` and its `points` are keyed by `nodeId`, with a `CHECK` that
rejects a positional array outright.

**A revision is promoted, not written-and-hoped.** The loader inserts everything with
`promoted_at` NULL and sets it only after every statement has succeeded, so a partial load
leaves `current_trees` serving the previous revision. Same staged-then-swapped discipline
the ingest uses on disk. A run that never promotes is visible in `ingest_runs`, which is
precisely what the legacy pipeline lacked — there, a failed run was indistinguishable from
a quiet day, and stale data shipped for months.

**Counts are precomputed.** The frontier DP resolves every tree at every budget in
milliseconds, and the answer is fixed for a revision, so it is computed at load time. That
is what turns "how many builds match this?" into a free pre-flight gate at request time
rather than a job — and the gate is what lets `solve_jobs` assume bounded output.

**`build_count` is `numeric`, not `bigint`.** Live data reaches **179,067,291,710** builds
(a Paladin class tree at 34 points). Nine class trees exceed 2³¹, which is exactly what
overflowed the engine's 32-bit counter. `numeric` also leaves room for a combined
multi-tree count to pass 2⁶³.

**Real keys and constraints throughout.** The legacy schema had no foreign keys or
constraints anywhere, and one global `ContentID` space shared across six tables that had to
be probed to discover what an id referred to. Here every entity has its own id space, and
the smoke test asserts that the constraints actually reject bad data rather than merely
being declared.

**Queue is `FOR UPDATE SKIP LOCKED`.** A partial index on `state = 'queued'` serves the
claim path; a partial unique index on `request_hash` makes dedup and caching the same
mechanism, so an identical in-flight or completed solve is reused. `solve_jobs_lease_idx`
supports the sweeper that requeues jobs stuck past their lease.

## Migrations

Plain SQL, forward only, applied once each by `migrate.py` — a small runner rather than
Alembic, so what runs against the database is exactly the reviewable file. Each migration
commits together with its ledger row, so a failure leaves no half-applied state.

Editing an already-applied migration is refused: the runner compares checksums and stops,
because a mismatch means the database and the repository disagree about history. Add a new
migration instead.

## Naming note

Parent tables use `revision`; child tables use `tree_revision` for the foreign key. Correct,
but easy to mistype in a join — `tree_counts c JOIN trees t ON t.id = c.tree_id AND
t.revision = c.tree_revision`.
