# API

FastAPI over `current_trees` and `tree_counts`. Its job is the **pre-flight count** the
product model hangs on: *how many builds match these constraints?* — answered inline, in
milliseconds, before any solve job exists.

```bash
docker compose up -d api
curl localhost:8000/health
python services/api/test_api.py        # 16 tests, stdlib only
open http://localhost:8000/docs        # generated OpenAPI
```

## Endpoints

| | |
|---|---|
| `GET /health` | Liveness, the promoted revision, and **how old the data is** |
| `GET /trees` | Every tree in the promoted revision; filter by `kind`, `classId`, `specId` |
| `GET /trees/{key}` | Full definition: nodes, edges, gating, entries, per-rank descriptions |
| `GET /trees/{key}/counts` | Precomputed unfiltered counts for every budget |
| `POST /counts` | **The gate.** Counts under constraints |

`/health` reporting data age is deliberate: the legacy pipeline's defining failure was that
nothing ever asked how old the data was, so it served stale trees for months after breaking.

## The gate

```jsonc
POST /counts
{
  "treeKey": "retail/11/102/spec",
  "points": 30,
  "levelCap": 90,
  "mustHave":    [88203],
  "mustNotHave": [88210],
  "choiceSides": {"88209": "a"}      // "a" | "b" | "none"
}
```

```jsonc
{
  "sets": 7483888,            // selections, choice-node sides unresolved
  "builds": 214290032,        // sides resolved: sum over sets of 2^(choice nodes)
  "filtered": true,
  "source": "computed",       // or "precomputed"
  "elapsedMs": 15.2,
  "listable": false,          // is enumerating this worth offering?
  "listingLimit": 2000000
}
```

**Two numbers, because they answer different questions.** `sets` is how many rows a listing
job would produce; `builds` is what a person means by "how many builds". Conflating them
understates the answer by up to ~50x.

**Two paths, both fast.** Unfiltered counts are a primary-key read against rows precomputed
at load time. Filtered counts run the frontier DP on demand — and because every supported
filter only *removes* transitions, a constrained count is cheaper than an unconstrained one.
Measured on Balance Druid's spec tree at 30 points (872 million builds): 10 ms precomputed,
15–38 ms filtered. Fast enough to run on every keystroke while a user paints constraints.

**`listable` is the point.** The count is free, so the API can say up front whether
enumerating the matches is worth offering, rather than letting a worker discover it. A job
only reaches the queue with a known, bounded result size.

## Correctness

The DP behind these counts is cross-checked against the C++ engine on all 160 trees, and now
on filtered counts too — require/exclude combinations agree exactly at every budget tested.
So the number the gate reports is the number the enumerator will produce.

Three invariants the tests assert over HTTP, each the analogue of a property the lower layers
check:

- `mustHave(X) + mustNotHave(X) == unfiltered` — every build either takes X or does not.
- `side_a + side_b + excluded == unfiltered` — for any choice node.
- `builds >= sets` always, since a set with k choice nodes expands to 2^k builds.

One thing worth knowing about the data: requiring a talent does not always reduce the count.
The top rows of a spec tree are in *every* build at a realistic budget — excluding Eclipse at
20 points yields zero — so requiring them narrows nothing. That is not a bug; it is exactly
the "requiring this changes nothing" signal the marginals are meant to surface.

## Not yet here

- **Solve jobs.** The schema and the gate exist; the worker that claims a job and execs
  `ttm-solver` does not.
- **`pointCap` is null.** The upstream payload does not carry the game's real per-tree cap,
  and guessing would be fabrication. `maxPointsInTree` (the sum of max ranks) is exposed
  instead, which is enough to bound a budget but not to validate one authentically.
- **Auth.** Nothing here needs an account; identity is additive and comes later.
- **or-group and one-of filters.** Expressible by composing the existing ones — "at least one
  of G" is `count(all) - count(none of G)`, and "exactly one of G" is the sum over members —
  but not yet exposed as request fields.
