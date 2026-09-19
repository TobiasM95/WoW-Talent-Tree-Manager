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
| 10 | 0.04 s | 223 | 2.4 KB |
| 15 | 0.05 s | 17,541 | 279 KB |
| 20 | 0.12 s | 71,030 | 1.35 MB |
| 25 | **30.6 s** | 25,299,311 | **689 MB** |
| 30 | **452.9 s** (7.5 min) | **305,286,987** | **9.5 GB** |

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

A **count-only mode** (increment, do not store) makes the headline question answerable for any
tree at any point budget in bounded memory. The recursive `visitTalent*` functions already thread
a `runningCount` through; suppressing the `combinations.push_back` is a small, contained change.

**Implemented and measured** (`--count-only`). Like-for-like, same tree, same budget, neither
writing an output file:

| druid_restoration, 25 points | Full enumeration | `--count-only` |
|---|---:|---:|
| Count | 25,299,311 | **25,299,311** (identical) |
| Peak RSS | 1,583 MB | **9 MB** |
| Solve time | 2.09 s | 1.96 s |

**The win is memory, roughly 176×, not speed.** Enumeration cost is unchanged — the recursion
still walks every solution — so counting is only ~6% faster.

Two earlier numbers in this document were wrong and are corrected here:

- A first draft predicted "roughly the same CPU cost". That was **right**. A second draft
  "corrected" it to 10.8× faster, which was **wrong**: that comparison was count-only *without*
  an output file against full enumeration *with* a 9.5 GB file write. The speedup was the absent
  disk I/O, not the absent storage.
- The 452.9 s / 42.0 s pair measures "writes 9.5 GB" vs "writes nothing". It says nothing about
  count-only, because the flag was not reaching the solver at the time (a CLI dispatch bug: only
  the `--parallel` branch propagated it, and the measurements used the default branch).

What count-only actually buys is the ability to answer the question **at all**. Full enumeration
of a 30-point spec tree needs ~2.4 GB of results plus copies; `shaman_class_elemental` at 24
points needs ~26 GB and was OOM-killed twice before the dispatch bug was fixed. In constant
memory those become routine.

For *speed*, enumeration is the wrong tool entirely — see below.

### 3b. Counting should not use the enumerator at all

Count-only removes the storage cost, but the *work* is still proportional to the number of
solutions. That is inherent to enumeration and cannot be optimised away.

It is also unnecessary. A **frontier dynamic program** counts the same sets in time polynomial in
the size of the tree, independent of how many builds exist. Prototyped and verified in
[`../../tools/frontier-dp/`](../../tools/frontier-dp/README.md):

| | Engine (`--count-only`) | Frontier DP |
|---|---:|---:|
| `druid_restoration`, 30 points | 42.0 s | — |
| `druid_restoration`, all budgets 1–30 | 30 separate runs | **0.10 s** |
| all 78 presets, all budgets | — | **3.7 s** |

Counts are identical to the engine on every tree tested (11 trees, 6 classes, class and spec
trees, plus 223 / 17,541 / 71,030 / 305,286,987 on `druid_restoration`). Peak DP state count
across every tree in the game: 8,859.

The decisive example: `shaman_class_elemental` has **37,296,642,700** valid builds at its best
budget, computed in 0.2 s. The enumerator would need ~86 minutes merely to count that, and
hundreds of gigabytes to store it. This is a complexity-class difference, not a constant factor.

**Architectural consequence.** Counting and enumerating become separate paths:

- *Counting* is a DP, answered inline in the API in milliseconds — no queue, no worker, no cache,
  and precomputable at ingest into one table.
- *Enumerating* stays the C++ engine behind the job queue, for listing builds matching filters,
  which is what it is genuinely good at.
- Per-talent marginals ("appears in 34% of valid builds") fall out of a forward-backward pass over
  the same DP, without listing a single build.

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
