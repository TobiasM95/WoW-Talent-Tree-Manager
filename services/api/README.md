# API

FastAPI over `current_trees` and `tree_counts`. Its job is the **pre-flight count** the
product model hangs on: *how many builds match these constraints?* — answered inline, in
milliseconds, before any solve job exists.

```bash
docker compose up -d api
curl localhost:8001/health
python services/api/test_api.py        # 28 tests, stdlib only
open http://localhost:8001/docs        # generated OpenAPI
```

## Endpoints

| | |
|---|---|
| `GET /health` | Liveness, the promoted revision, **how old the data is**, and description/icon coverage |
| `GET /trees` | Every tree in the promoted revision; filter by `kind`, `classId`, `specId` |
| `GET /trees/{key}` | Full definition: nodes, edges, gating, entries, per-rank descriptions |
| `GET /trees/{key}/counts` | Precomputed unfiltered counts for every budget |
| `POST /counts` | **The gate.** Counts under constraints |
| `POST /solve` | Queue a filtered enumeration — refused if the gate says it is too large |
| `GET /solve/{id}` | Job state, phase, progress, expected and actual counts |
| `GET /solve/{id}/results` | A page of matching builds, nodeId-keyed |
| `GET /solve/{id}/stats` | How often each talent appears across all of them |
| `POST /solve/{id}/cancel` | Stop a job, if it can still be stopped |
| `GET /icons/{name}` | One talent icon, cached for a year |

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

## Progress

`GET /solve/{id}` returns `progress` together with `phase` — `solving`, `storing` or
`finalizing`. One number cannot carry this: a 25-point Balance Druid solve enumerates
1,906,208 sets in 0.11 s and then spends the rest of its wall clock storing them, so a bar
driven by the solver alone would sit at 100% for almost the whole wait. `finalizing` has no
fraction to report and says so rather than inventing one. See
[`../worker/README.md`](../worker/README.md).

## Cancellation

`POST /solve/{id}/cancel` answers with the job, and the job says which of two things
happened:

- `state: "cancelled"` — it was still queued, and is now gone.
- `state: "running"`, `cancelRequested: true` — a worker holds it. It stops within about a
  second, and a UI shows "cancelling..." on that flag rather than pretending it is done.

Cancelling an already-cancelled job returns 200 with the same answer, because a client that
lost the response to its first attempt cannot tell whether it landed. Cancelling a `done`,
`capped` or `failed` job is a 409: those cannot be un-produced.

A cancelled job keeps no results and does not report 100% progress. See
[`../worker/README.md`](../worker/README.md) for how the worker notices.

## Talent statistics

`GET /solve/{id}/stats` reports, for every talent, how many of the job's builds take it,
that as a `share`, its `meanPoints` among the builds that take it, and whether it is
`mandatory` — present in *every* matching build.

This is what exhaustive enumeration buys that sampling cannot: a statement about the whole
matching set rather than about a draw from it. "Every one of these 34,619 builds takes
Eclipse" is a fact about the constraints; "42% take Shooting Stars" locates the decision
that is actually open.

**Computed by the worker, not on demand.** The aggregate walks every result row — about a
second for 165,000 builds, proportionally worse toward the listing limit — which is fine
inside a job that is already asynchronous and has the rows hot, and far too slow for a
request someone is waiting on. It is written in the same transaction as the rows it
describes, so a job has results and statistics or neither.

**Cross-checked against the DP.** The test suite asserts that every talent's `builds` equals
the count `POST /counts` gives for requiring that talent. The two sides share nothing — one
is a SQL aggregate over rows the C++ engine produced and the worker decoded, the other is
the frontier DP — so agreement on every talent exercises the whole pipeline at once.

## Icons

`GET /icons/{name}` serves one talent icon with `Cache-Control: immutable`, a one-year
max-age and an ETag, so a returning visitor makes no icon requests at all. `?size=` takes
18, 36 or 56. The `.jpg` a browser will append is optional.

**A 404 is expected.** Upstream has no art for every name the talent payload uses (19 of
2,094 today), so a client renders the talent without its icon rather than treating it as an
error. `iconCoverage` on `/health` says how complete the cache is; the application works
with it empty. See [`../../docs/02-target/icons.md`](../../docs/02-target/icons.md).

## Not yet here

- **Auth.** Nothing here needs an account; identity is additive and comes later.

## Custom trees

`POST /custom-trees` takes a project from the tree editor -- one to three trees, each a list of
talents with a grid cell, ranks, a gate and requirements -- validates everything the solver
relies on, and writes the trees into `trees` like any other game's, under game `custom`. From
there every other endpoint takes them unchanged: counts, solves, results, the tree detail.

Projects are **content-addressed**: the id is the hash of the canonical design, so saving the
same design twice is the same project and an edit is a new one. Keys therefore never change
meaning, which is what makes the DP-graph cache and solve dedup safe without invalidation. The
design itself is stored beside the derived trees and is the source of truth (migration 010): the
first version rebuilt it from the trees, which made the derivation part of the identity, so an
opened project saved back as a different one.

A project has one `style`. **Classic** (the default, and every project made before styles
existed): one to three tabs, `pointsPerRow` gates, optionally a `sharedPointCap`. **Retail**:
trees with a `role` -- one `class` first, then one to four `spec`, then up to six `hero`, each
hero naming the specs that take it (`specs`, by place) -- served as retail's own kinds with a
`specName` or `heroSpecs`, so the planner takes a custom project exactly as it takes a real spec.
Retail takes no pool and no per-row gates, classic no barriers; mixing is refused (migration 012
lets a project hold the eleven trees retail's shape allows).

A tree's gates come from, in order: a talent's own `pointsRequired`; else the highest
**barrier** (`barriers: [{row, points}]`, retail's lines, which must rise going down) at or
above its row; and `pointsPerRow` times its row, whichever is higher. A talent marked
`granted` is served as `preFilled` -- free, like retail's starting talents -- which is what lets
a copied retail tree count exactly like the original. Both fields are only written when used,
so projects saved before they existed keep their ids.

`GET /custom-trees/{project}` returns a project's trees and its design, for the editor to keep
editing. Custom trees are never listed wholesale -- `GET /trees?game=custom` is refused -- since
listing would publish every project anyone saved; they are reached by their link.
`GET /icons?search=` lists cached icon names for the editor's picker.

## Top players (WarcraftLogs)

`GET /popular/content` lists the current raid and Mythic+ season; `GET /popular/{spec key}?zone=&encounter=all|<id>&difficulty=`
reads the top-ranked players of a spec there and returns, in this tool's terms: the hero-tree
split, each talent's pick rate, choice-side shares, and every distinct build with how many
players run it. Answers are cached for six hours (migration 011): the public API allows 3,600
points an hour and a page of 100 players costs one or two.

Keys come from `.env` (`WCL_CLIENT_ID`, `WCL_CLIENT_SECRET`) through docker compose, and never
reach the browser. The v2 client-credentials flow needs no redirect URL, which is what lets a
local service use it. Without keys these endpoints answer 503 and nothing else changes.

WarcraftLogs names a talent by its **trait entry id**, so a choice node's side comes for free.
Two things had to be learned from real data: granted talents are listed and are dropped, and
a **tiered** node is listed once per rank -- Blood's 4-rank capstone arrives as four entries of
one point. Taking the last entry instead of the sum dropped three points from every Blood build;
a legality check cannot see that (spending less is legal), so the API suite now also asserts
every top build fills every tree to its cap. Fixing it moved SimC's verdict on the top builds
from 15% behind its sample profile to 1.2% ahead, and flipped which hero tree won.

Every real build is also checked against our copy of the trees. The live game allowed them, so
one our data refuses means the data is stale; the API reports those rather than hiding them, and
the suite asserts there are none -- 809 top Blood players, zero today.
