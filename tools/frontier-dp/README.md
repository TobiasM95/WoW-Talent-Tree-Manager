# Frontier DP — counting builds without enumerating them

A verified prototype. It counts valid talent selections with dynamic programming instead of
enumerating them, and reproduces the engine's counts exactly.

This is a **spike**, not production code: Python, single-threaded, and it encodes
Dragonflight-era semantics. See "Not yet handled" below.

## Why

The engine answers "how many valid builds are there?" by generating every one of them. That cost
is proportional to the answer, which for a full spec tree is 305 million results, 42 s in
`--count-only` mode and 9.5 GB if stored (see
[`../../docs/02-target/solver-performance.md`](../../docs/02-target/solver-performance.md)).

Counting does not require visiting each solution. Cost here is polynomial in the size of the
tree and independent of how many builds exist.

## Results

Identical counts to `ttm-solver` on every tree tested — 11 trees across 6 classes, class trees
and spec trees, plus four budget checkpoints on `druid_restoration`
(223 / 17,541 / 71,030 / **305,286,987**).

| | Engine (`--count-only`) | Frontier DP |
|---|---:|---:|
| `druid_restoration`, 30 points | 42.0 s | — |
| `druid_restoration`, **all** budgets 1–30 | 30 separate runs | **0.10 s** |
| **all 78 presets, all budgets** | — | **3.7 s** |

Peak DP states across every tree in the game: **8,859**.

The clearest case: `shaman_class_elemental` has **37,296,642,700** valid builds at its best
budget, computed in 0.2 s. Enumerating that would take ~86 minutes just to count, and hundreds of
gigabytes to store. It is a different complexity class, not a constant-factor win.

## How it works

There is an interactive explainer at
[`../../docs/explainers/frontier-sweep.html`](../../docs/explainers/frontier-sweep.html) —
open it in a browser. It steps through the sweep one talent at a time on a small tree, shows
states merging as they happen, and computes both the DP and a brute-force enumeration live so
the two can be seen to agree. Start there if the description below doesn't land; the short
version is that counting in groups is the same trick as counting grid routes with Pascal's
triangle, and the page makes that correspondence explicit.

The mechanics: process nodes in the engine's topological order (Kahn's, ready queue sorted by
`pointsRequired`).
At any point, the future only needs to know:

1. **points spent so far** — for `pointsRequired` gates and the budget, and
2. **which already-processed nodes are selected**, but only those that still have unprocessed
   children (a node's status stops mattering once its last child is processed).

That second set is the **frontier**. State is `(selected subset of frontier, points spent)`,
mapped to a count. Two transitions per node — skip it, or take it if a parent is selected and the
gate passes. Sum the states at the end.

Many different partial selections collapse onto the same state, and that collapse is the whole
trick: millions of partial selections become a few thousand states.

## Semantics replicated from the engine

- Pre-filled roots are deleted and their children promoted to roots (`expandTreeTalents`).
- Multi-rank talents expand into a rank chain, with the original children attached to the **last**
  rank — so a talent's children open only when it is **fully maxed**
  (`expandTalentAndAdvance`, `TalentTrees.cpp:1760`).
- A node may be taken iff points already spent `>=` its `pointsRequired`, and it is a root or at
  least one parent is already taken (OR semantics, per the child-insertion loop in
  `visitTalentSingle`).
- Counts **sets**. Switch/choice multiplicity is a separate post-pass in the engine
  (`switchTalentChoices`), so it is excluded here too.

## Not yet handled

- **Hero talents** — sub-tree selection and cross-tree `requiresNode` prerequisites. The latter
  would enter as a pre-satisfied input mask.
- **`tiered` nodes** — max ranks depend on character level, so `maxPoints` must be resolved
  against the level cap before the DP runs. See
  [`../../docs/02-target/raidbots-live-schema.md`](../../docs/02-target/raidbots-live-schema.md).
- **Include / one-of filters.** Exclude filters are easy (forbid taking a node); include and
  one-of are expressible but need thought. If the user wants the actual *list* of builds,
  enumeration is the right tool and the existing engine is good at it.

## Usage

```bash
# counts for every budget of one preset (index 2 = druid_restoration)
python tools/frontier-dp/frontier_dp.py Engine/resources/presets.txt 2 30

# check against the compiled solver (needs ttm-solver built)
python tools/frontier-dp/validate.py tools/frontier-dp

# every preset, every budget
python tools/frontier-dp/bench_all.py tools/frontier-dp
```

## Where this is headed

Counting and enumerating should be separate paths. Counting becomes a DP answered inline in the
API in milliseconds — no queue, no worker, precomputable at ingest into a single table.
Enumeration stays the existing C++ engine behind the job queue, for listing builds that match
filters.

A forward-backward pass over the same DP also yields per-talent marginals ("this talent appears
in 34% of all valid builds") without listing a single build.
