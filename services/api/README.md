# API

FastAPI over `current_trees` and `tree_counts`. Its job is the **pre-flight count** the
product model hangs on: *how many builds match these constraints?* — answered inline, in
milliseconds, before any solve job exists.

```bash
docker compose up -d api
curl localhost:8000/health
python services/api/test_api.py        # 28 tests, stdlib only
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
| `POST /solve` | Queue a filtered enumeration — refused if the gate says it is too large |
| `GET /solve/{id}` | Job state, expected and actual counts |
| `GET /solve/{id}/results` | A page of matching builds, nodeId-keyed |

`/health` reporting data age is deliberate: the legacy pipeline's defining failure was that
nothing ever asked how old the data was, so it served stale trees for months after breaking.

## The gate

```jsonc
POST /counts
{
  "treeKey": "retail/11/102/spec",
  "points": 30,
  "levelCap": 90,
  "mustHave":     [88203],
  "mustNotHave":  [88210],
  "choiceSides":  {"88209": "a"},   // "a" | "b" | "none"
  "atLeastOneOf": [[88204, 88215, 88219]],
  "exactlyOneOf": [[88221, 88236]]
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

## Group filters

`atLeastOneOf` and `exactlyOneOf` take groups of node ids. Neither needs new DP state —
both are compositions of counts the DP already produces:

```
at least one of G  =  count(unconstrained) - count(all of G excluded)
exactly one of G   =  sum over m in G of count(m required, the rest excluded)
```

The second is valid because the alternatives are mutually exclusive by construction, so
the terms cannot double-count — which is exactly what the engine's `-3` sentinel means.
Multiple at-least-one groups are handled by inclusion-exclusion.

Verified against the engine's own `-2` and `-3` filter semantics: at 14 points on Balance
Druid, at-least-one gives 6,135 and exactly-one 1,625 from both implementations.

**One asymmetry, stated rather than hidden.** Counting supports any number of groups; the
engine's filter holds a single value per talent, so *listing* supports one group of each
kind. `POST /solve` refuses more with a 400 that says to use `/counts` — rather than
silently dropping a constraint the count already honoured.

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

- **Progress.** Jobs jump from 0 to 1; there is no incremental reporting yet.
- **Auth.** Nothing here needs an account; identity is additive and comes later.
- **Cancellation.** The `cancelled` state exists in the schema but nothing sets it.
