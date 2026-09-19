# Verified: solver cost curve, and what it means for the product

**Measured on 2026-09-19** against the real `druid_restoration` spec-tree preset, Release builds
(MSVC v143 x64 and gcc 12 in Debian bookworm). These are not estimates — the engine's own
`docs/01-current-state/engine-and-solver.md` notes that no benchmark numbers existed anywhere in
the codebase. Now they do.

## The numbers

`ttm-solver --structure-file-path presets.txt --structure-indices 2 --target-talent-count N`,
no filter (exhaustive enumeration):

| Talent points | Wall time | Combinations | Output file |
|---:|---:|---:|---:|
| 10 | 0.04 s | 224 | 2.4 KB |
| 15 | 0.05 s | 17,542 | 279 KB |
| 20 | 0.12 s | 71,031 | 1.35 MB |
| 25 | **30.6 s** | 25,299,312 | **689 MB** |
| 30 | **452.9 s** (7.5 min) | **305,286,988** | **9.5 GB** |

A realistic query — *"show me every valid build in my spec tree"*, i.e. the full 30-point cap —
takes **7.5 minutes and writes a 9.5 GB file**. Growth is roughly an order of magnitude per 5
points past 20.

## Five consequences

### 1. Unconstrained full-tree solving is not a viable web feature

At 9.5 GB and 7.5 minutes of a full CPU core per request, a handful of concurrent users would
saturate any reasonable host. This was acceptable for a desktop app writing to the user's own
disk; it is not something to expose on a shared server.

The product must therefore treat the solver as **constrained by default**:

- Require filters (must-have / must-not-have / one-of) for high point budgets. Exclude filters
  prune *during* enumeration, so they cut real work, not just output.
- Or cap the point budget for interactive solves. Under 20 points is effectively instant
  (≤120 ms), which covers a large share of genuine use.
- Treat an unconstrained 30-point solve as a deliberate, rate-limited, authenticated operation —
  if it is offered at all.

### 2. The default safety guard is far too permissive for a server

`TreeDAGInfo::safetyGuard` defaults to 500,000,000 combinations
(`Engine/src/TreeSolver.h:31`). The 30-point solve produced 305 million — **it never tripped the
guard**, and still wrote 9.5 GB. A guard that permits a 9.5 GB result is not protecting anything.

Server-side defaults should be set from the output budget we are willing to serve, not from host
memory. A few million combinations is a sane interactive ceiling; the worker's `--max-results`
should default there and the guard must be a first-class, per-job argument.

### 3. Counting and enumerating must be separated

The engine stores every combination it finds in order to count them. But the two questions have
very different costs:

- *"How many valid builds are there?"* — needs only a counter. Cheap, bounded memory, and by far
  the more common question.
- *"List them."* — needs storage proportional to the answer, which is the 9.5 GB problem.

A **count-only mode** (increment, do not store) would make the headline question answerable for
any tree at any point budget in bounded memory, at roughly the same CPU cost. The recursive
`visitTalent*` functions already thread a `runningCount` through; suppressing the
`combinations.push_back` is a small, contained change.

This is the single highest-value engine improvement available and it belongs in Phase 2, not in
"later improvements". It converts the flagship feature from unshippable to cheap for the common
case.

### 4. Result caching is worth far more than assumed

[`architecture.md`](architecture.md) proposes `request_hash` dedup as "the single highest-value
optimisation available". These numbers justify that strongly: a cache hit on a 30-point solve
saves 7.5 CPU-minutes. Popular spec/filter combinations will repeat constantly.

Corollary: because results are this large, cache the *count* and a bounded top-N page
aggressively and permanently, and treat the full enumeration as ephemeral, recomputable on
demand, and aggressively expired.

### 5. A wall-clock budget is mandatory, not a nicety

Already flagged (open question R7), but the measurement makes it concrete: the gap between a
0.12 s solve and a 452 s solve is five talent points. A user can trivially, innocently, request
something 4,000× more expensive than what they asked for a moment earlier. Without
`--time-budget` the worker has no way to decline.

## Also observed

- **Streaming is required.** The current CLI buffers all results and writes at the end, so a
  9.5 GB result means 9.5 GB of process memory before the first byte is written. NDJSON streaming
  with incremental flush (per the worker protocol in
  [`architecture.md`](architecture.md)) is not a nice-to-have.
- **The engine writes `error_log.txt` to a relative path in the CWD**, and attempts to load
  presets from its data directory on startup *even when solving from an explicit
  `--structure-file-path`*. Both need addressing for a containerised worker: logs to stderr, and
  no unnecessary data-directory dependency.
- **Output format is unusable as-is** for a web UI: raw decimal `SIND` integers plus switch
  indices, preceded by a single header line mapping bit position to talent index. It requires the
  header to interpret any row, which makes rows non-self-describing. The worker should emit
  resolved, nodeId-keyed NDJSON instead.

## Reproducing

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/ttm-solver --structure-file-path Engine/resources/presets.txt \
    --structure-indices 2 --target-talent-count 20 --output-file-path /tmp/out.txt
```

Index 2 is `druid_restoration`; index 1 is `druid_class_restoration`. Do not run 30 points
without ~10 GB free and patience.
