# Talent data ingest

Turns the upstream talent payload into TTM tree JSON — one file per tree, plus a
manifest recording provenance and every anomaly observed.

```bash
python services/ingest/ingest.py --dry-run                    # validate live data, write nothing
python services/ingest/ingest.py --out data/generated         # fetch and write
python services/ingest/ingest.py --source talents.json --out data/generated
python services/ingest/tests/test_ingest.py talents.json      # tests (payload arg optional)
```

Only `requests` is needed, and only for fetching — a local payload needs no dependencies.

## What it produces

From 40 spec entries it emits **160 trees**:

| Kind | Count | |
|---|---:|---|
| `class` | 40 | one per spec, because Blizzard's class-tree tooltips vary by spec |
| `spec` | 40 | |
| `hero` | 80 | exactly two per spec |

4,567 nodes total. Each tree is a standalone JSON document keyed
`retail/<classId>/<specId>/<kind>[/<subTreeId>]`, with a `uuid5` `id` derived from that
key — so re-ingesting identical data produces identical ids, and revisions diff cleanly.

**The N-trees-per-spec shape is the point.** The native client hardcoded exactly two
trees per spec (class + spec, joined by `complementaryTreeIndex`), which is why hero
talents had nowhere to go and the tool stalled at the TWW pre-patch. See
[`../../docs/02-target/data-model.md`](../../docs/02-target/data-model.md).

## Failing loudly

The legacy pipeline's defining failure was silence: a crash skipped the commit step, CI
went green, and stale data shipped for months. Nothing here degrades gracefully.

**Fatal** — an unknown node or entry type, a missing required field, a truncated or
non-JSON payload, a node with no entries or `maxPoints < 1`, a parent or child outside
the tree, a child/parent pair that disagree, a node unreachable from any root, duplicate
tree keys, or hero nodes referencing a sub-tree no selector offers.

**Warned** — a change in the class or spec count. Never hardcode that set: Demon Hunter
gained a third spec (Devourer), so the legacy assumption of 39 specs is already wrong.
The count is derived and drift is reported.

Output is staged in a temporary directory and swapped in only after validation passes,
so a bad run leaves the previous revision untouched (the prior one is kept alongside as
`<out>.previous`).

## Anomalies are expected, and recorded

These are real properties of live data, not defects. The manifest records each with
counts and detail; see
[`../../docs/02-target/raidbots-live-schema.md`](../../docs/02-target/raidbots-live-schema.md).

| Anomaly | Live count | What it is |
|---|---:|---|
| `degenerateNodes` | 6 | Nodes with an empty entry and no name. Dropped. A naive `entry["type"]` read raises `KeyError` here — the legacy failure mode exactly. |
| `siblingSubtreeNodes` | 14 | A sub-tree selector's `nodes[]` is the **union across both specs** sharing that hero tree, so it names nodes absent from this spec. Membership comes from each hero node's `subTreeId`, never from `nodes[]`. |
| `unresolvedPrerequisites` | 96 | Hero nodes whose `requiresNode` points into a class or spec tree — often the *sibling* spec's, since hero trees are shared between two specs. Normal, and the reason prerequisite resolution cannot assume same-tree. |
| `crossTreeEdges` | 0 | `prev`/`next` never cross tree boundaries today. Recorded so it is noticed if that changes. |

`requiresNode: 0` is upstream's sentinel for "no prerequisite" (17 nodes) and is
normalised to `null`. Treating `0` as a node id invents an edge.

## Known gaps

- **Descriptions are empty.** The payload carries `spellId`, `icon` and names but no
  tooltip text; the legacy pipeline scraped Wowhead HTML for it. `ranks: []` on every
  entry is a placeholder, not an oversight — open question Q4.
- **`pointCap` is `null`.** The game's real per-tree point cap is not in the payload, and
  guessing would be fabrication. `maxPointsInTree` (the sum of max ranks) is emitted
  instead as a derived fact.
- **`rankLevels` is carried but not resolved.** Tiered nodes' max ranks depend on
  character level, which the solver must resolve against a level cap before expanding a
  tree — open question Q10.
- **No icons yet.** Individual files behind a cache, not the 16 MB packed atlas that grew
  `.git` to 1.4 GB.

## Adding a source

Everything downstream works off `Payload.specs`, so a second source means one more loader
returning that shape. The intended fallback is wago.tools raw DB2 CSVs (`TraitNode`,
`TraitEdge`, `TraitCond`, `TraitSubTree`). Do **not** use simc as the layout source — it
never parses `TraitEdge` and so discards the prerequisite graph entirely. See
[`../../docs/02-target/talent-data-sources.md`](../../docs/02-target/talent-data-sources.md).
