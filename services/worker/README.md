# Solve worker

Claims jobs from `solve_jobs`, runs the C++ engine, and writes back the matching builds.

```bash
docker compose up -d worker
docker compose up -d --scale worker=4 worker    # SKIP LOCKED needs no coordination
docker compose logs -f worker

# Tests: decoding, filter strings, and progress parsing against the real solver.
# Engine/ is mounted because the solver-backed test needs the preset file.
docker compose run --rm -v "$PWD/Engine:/app/Engine" worker   python /app/services/worker/test_worker.py
```

## The loop

1. **Claim** a job with `FOR UPDATE SKIP LOCKED`, taking the lease.
2. **Render** the tree into the engine's format (`ttm_format`), with tiered ranks resolved
   against the job's level cap.
3. **Exec** `ttm-solver` with `--max-results`, `--time-budget-ms` and `--progress`,
   reading progress off its stderr while it runs.
4. **Decode** its output into nodeId-keyed builds, one at a time.
5. **Store** them with `COPY` and mark the job `done`, `capped` or `failed`.

Every job arrives with a known result size, because the API's pre-flight count gated it
before the row existed. That is why results can be stored normally rather than streamed to
object storage — the unbounded case is refused up front, not handled here.

## Decoding

The engine emits a header listing, per bit, the positional talent index that bit belongs
to, then one line per set: a raw `SIND` followed by the indices of any choice nodes.

A multi-rank talent occupies several bits that all map to the same talent, so a node's
point total is **how many of its bits are set** — the decoder counts rather than flags.
Verified against real data: builds come back with up to 2 points on a node (matching
`maxPoints`), and every build spends exactly the requested budget.

Output rows are positional; what gets stored is keyed by Blizzard `nodeId`. Positional
references are precisely what made shared builds unsafe in the legacy format, so they do
not survive past this boundary.

## The gate and the engine must agree

The worker compares the engine's reported count against `expected_count`, the number the
API's pre-flight count produced, and **fails the job if they differ**. A user is shown a
count before the work happens; handing them a result that silently contradicts it would be
worse than an error.

They agree because both come from implementations cross-checked against each other: the DP
behind the gate matches the engine on all 160 trees and on every filter shape tested.

## Outcomes

| State | Meaning |
|---|---|
| `done` | Complete, and the count matched the prediction. |
| `capped` | Truncated by the time budget or the result cap. Partial builds are kept, with the reason in `error`. A distinct outcome, not a failure. |
| `failed` | The solve could not be completed. The message is user-facing. |

`capped` exists because a truncated result is genuinely different from both success and
failure — the engine had no wall-clock guard at all until this phase, so "still running"
was previously the only alternative to finishing.

## Crash recovery

A worker that dies leaves its job in `running` with a stale lease. Every 30 seconds the
worker returns such jobs to the queue, and fails them once `attempts` reaches 3. Without
this, a killed worker strands a job forever — indistinguishable, to a user, from one that
is merely slow.

`SIGTERM` finishes the current job before exiting, so `docker compose down` does not orphan
a solve mid-flight.

## Limits

`--time-budget-ms` and `--max-results` are clamped to the worker's own ceilings, so a
request cannot raise them. The container also carries hard `cpus` and `mem_limit` settings:
a pathological job should not be able to take the host with it.

## Progress

`solve_jobs.progress` is a fraction of `solve_jobs.phase`, and the phase matters as much as
the number:

| Phase | What it measures | Typical share of a large job |
|---|---|---|
| `solving` | Sets found, against the pre-flight count | 0.11 s of 16 s |
| `storing` | Rows streamed into `solve_results` | ~11 s of 16 s |
| `finalizing` | Primary key build and commit — no fraction to report | ~4 s of 16 s |

Those numbers are an unfiltered 25-point Balance Druid spec solve, 1,906,208 sets. They are
the reason this is per-phase: **the engine is not the slow part.** A bar driven by the
solver alone would sit at 100% for the entire wait. It is also why `--progress` mattered
less than expected and `COPY` mattered more — see below.

**The engine reports on stderr**, one `PROGRESS <count>` line per interval, emitted from the
same sampled clock check that enforces `--time-budget-ms` (so it costs one comparison, about
1.6% on a 23-second solve). Not a progress file: a file is shared state between the container
writing it and the process reading it, and needs a cleanup path for every way a job can end.
The worker already holds the child's stderr pipe.

The worker's stdout goes to a file rather than a second pipe, because reading only one of two
pipes deadlocks as soon as the other fills its buffer.

Two details worth knowing before changing this code:

- **The reporter has its own database connection.** A connection in `COPY` mode accepts no
  other statement, so an `UPDATE` sent down the connection running the COPY waits for a COPY
  that is itself waiting on the row that would trigger the next report. That deadlocks
  silently, with the job parked at 0%.
- **`finalizing` is set from inside the COPY block**, not after it. Leaving a COPY is not
  free — closing it waits while Postgres builds `solve_results`' primary key over everything
  just written. Marking the phase afterwards would mark a wait that had already ended.

Each progress write also refreshes `locked_at`, so a job that legitimately outlives its lease
is not requeued underneath the worker still running it.

## Storing results

`COPY`, not `executemany`, and a generator, not a list.

Storing 1,906,208 rows one INSERT at a time had not finished after 25 minutes — for a solve
that took 0.11 s. With `COPY` the same job completes in about 16 seconds. The decoder is a
generator feeding the COPY directly, so neither the engine's output nor the decoded builds
are ever all in memory at once.

## Not yet here

- **Cancellation.** The `cancelled` state exists in the schema but nothing sets it.
