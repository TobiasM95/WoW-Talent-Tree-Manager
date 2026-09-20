# Solve worker

Claims jobs from `solve_jobs`, runs the C++ engine, and writes back the matching builds.

```bash
docker compose up -d worker
docker compose up -d --scale worker=4 worker    # SKIP LOCKED needs no coordination
docker compose logs -f worker
```

## The loop

1. **Claim** a job with `FOR UPDATE SKIP LOCKED`, taking the lease.
2. **Render** the tree into the engine's format (`ttm_format`), with tiered ranks resolved
   against the job's level cap.
3. **Exec** `ttm-solver` with `--max-results` and `--time-budget-ms`.
4. **Decode** its output into nodeId-keyed builds.
5. **Store** them and mark the job `done`, `capped` or `failed`.

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

## Not yet here

- **Progress reporting.** Jobs jump from 0 to 1. The engine maintains a running count in its
  hot loop; surfacing it needs a progress file and a lease-renewing read.
- **Cancellation.** The `cancelled` state exists in the schema but nothing sets it.
- **or-group and one-of filters.** The engine supports them; the worker only passes
  must-have and must-not-have through.
