# Verified: live talent data schema (raidbots)

**Everything in this document was verified directly against live data on 2026-09-19**, by
downloading and analysing `https://www.raidbots.com/static/data/live/talents.json`
(HTTP 200, 3,229,562 bytes, `application/json`). This is ground truth, not inference — unlike
most of the target documents, which are proposals.

It matters because it settles the central strategic question: **the data source did not break.
The consumer did.**

## 1. The endpoint is alive and still carries what we need

The legacy pipeline's primary source (`tree_presets_generator.py`) still works:

```
GET https://www.raidbots.com/static/data/live/talents.json  →  200, 3.2 MB JSON
```

Top level is an **array of 40 spec entries**, each shaped:

```jsonc
{
  "traitTreeId": 793,
  "className": "Druid", "classId": 11,
  "specName": "Balance", "specId": 102,
  "classNodes":   [ /* 53 */ ],
  "specNodes":    [ /* 38 */ ],
  "heroNodes":    [ /* 28 */ ],   // ← legacy generator never read this
  "subTreeNodes": [ /*  1 */ ],   // ← nor this
  "fullNodeOrder":[ /* 282 */ ]
}
```

`heroNodes` and `subTreeNodes` are **present and populated**. The legacy generator read only
`classNodes` and `specNodes`, which is precisely the gap described in
[`../01-current-state/data-pipeline-and-scraper.md`](../01-current-state/data-pipeline-and-scraper.md).

Corpus totals: **4,613 nodes** across **40 specs / 13 classes**.

## 2. Two findings that invalidate hardcoded assumptions

### 2.1 Demon Hunter now has a third spec

| Class | Specs |
|---|---|
| Druid | Balance, Feral, Guardian, Restoration |
| **Demon Hunter** | Havoc, Vengeance, **Devourer** |
| all others | 3 each |

That's 40 specs, where the legacy data had 39 (`node_id_orders.txt` is exactly 39 lines).
`Engine/src/TTMEnginePresets.h` hardcodes `DEMONHUNTER_SPEC_IDS` with two entries, so **the
engine's spec enums are already wrong**. Any hardcoded class/spec enumeration will keep breaking;
the ingest must derive the class/spec set from the data and warn when it changes.

### 2.2 Level caps and a new level-gated mechanic

Levels referenced in the data: **10, 71, 81, 84, 90**. The engine defaults
`TalentSkillset::levelCap = 70` (`Engine/src/TalentTrees.h`) — DF-era and stale by two
expansions.

More importantly, there are **40 `tiered` nodes — exactly one per spec** — and all 40 carry a
`rankLevels` array:

```jsonc
{
  "id": 110421, "type": "tiered", "name": "Ascendant Eclipses / ... / ...",
  "posX": 12300, "posY": 7650, "reqPoints": 20, "maxRanks": 4,
  "rankLevels": [ {"level":81,"maxRanks":1}, {"level":84,"maxRanks":3}, {"level":90,"maxRanks":4} ],
  "entries": [ {"type":"tierrank","maxRanks":1,"index":0,   ...},
               {"type":"tierrank","maxRanks":2,"index":100, ...},
               {"type":"tierrank","maxRanks":1,"index":200, ...} ]
}
```

**This is a mechanic the engine cannot represent at all.** A node's maximum ranks depends on
*character level*, not only on points spent. The engine's `Talent::maxPoints` is a single static
integer, and `expandTreeTalents` bakes it into a fixed number of bitset slots. Supporting tiered
nodes means `maxPoints` becomes a function of level cap — which changes the solver's input, since
the expanded slot count (and therefore the 64-bit budget) now varies with the level cap.

This is new since the native app was last maintained and is **not** covered by the other
analyses. It belongs in the data model as a first-class concept, and it is a second reason
(alongside hero talents) that this is a data-model problem rather than a scraper problem.

## 3. Node schema, by observed frequency

Across all 4,613 nodes:

| Node key | Count | Coverage | Meaning |
|---|---|---|---|
| `id`, `name`, `type`, `posX`, `posY`, `next`, `prev`, `entries` | 4613 | 100% | always present |
| `maxRanks` | 4567 | 99% | absent on `subtree` nodes |
| `reqPoints` | 2406 | 52% | points-spent gate; **only values 8, 20, 23** |
| `subTreeId` | 1120 | 24% | which hero sub-tree the node belongs to |
| `entryNode` | 324 | 7% | root of its (sub)tree |
| `freeNode` | 146 | 3% | granted free — the "prefilled" concept |
| `requiresNode` | 113 | 2% | prerequisite node id (see §4) |
| `freeLevel` | 83 | 1% | level at which a free node is granted |
| `rankLevels` | 40 | <1% | level-gated ranks (§2.2) |

Node `type` vocabulary: `single` (3874), `choice` (659), `tiered` (40), `subtree` (40).
Entry `type` vocabulary: `passive` (4294), `active` (893), `tierrank` (120), `subtree` (80).

The legacy generator inferred type from `choice` or `maxRanks > 1` only, so it silently
mis-handled `tiered` and `subtree`. **Ingest must reject unknown type values loudly** rather than
fall through to a default.

Entry keys: `id`, `type`, `name` (always); `definitionId`, `maxRanks`, `spellId`, `icon`, `index`
(5307 of 5387); `visibleSpellId` (31); and on subtree entries only, `traitSubTreeId`,
`traitTreeId`, `atlasMemberName`, `nodes`.

### Data-quality traps

- **6 degenerate nodes** exist with `name: ""` and `entries: [{}]` (e.g. Evoker Devastation node
  `93196`, at `posX: 16640, posY: -320` — off-grid). These must be filtered, not rendered. A
  naive `entry["type"]` access raises `KeyError`, which is exactly the failure mode that killed
  the legacy CI job silently.
- **`requiresNode: 0` is a sentinel for "none"**, used by 17 nodes, while 96 use a real id. Code
  that treats `0` as a valid node id will construct a bogus prerequisite.
- `posX`/`posY` are large integers on a coarse grid (the legacy code divided by 300 and applied
  hardcoded x-offsets of 1200/9000). Derive rows/columns from the data's own spacing; do not
  hardcode offsets.

## 4. Cross-tree prerequisites are real — Q5 answered: **yes**

This resolves the highest-risk open question. Hero nodes reference prerequisites that live in a
**different spec's** tree. For Druid Balance:

| Hero node | Sub-tree | `requiresNode` | Target lives in |
|---|---|---|---|
| 94608 Boundless Moonlight | 24 | 92587 | **Druid / Guardian / specNodes** ("Lunar Beam") |
| 94598 Moon Guardian | 24 | 82145 | Druid / Guardian / specNodes |
| 94607 Atmospheric Exposure | 24 | 92587 | Druid / Guardian / specNodes |
| 94587 The Eternal Moon | 24 | 92587 | Druid / Guardian / specNodes |
| 94600, 94599, 94591, 94606 | 23 | `0` | none (sentinel) |

The reason: a hero sub-tree is **shared between two specs of a class** (sub-tree 24, "Elune's
Chosen", is shared by Balance and Guardian), and `traitTreeId` is per *class*, not per spec. So a
hero node's prerequisite may point into the spec tree of the *sibling* spec, where it is
meaningful; when solving Balance, that prerequisite is not satisfiable and the node is gated by
other means.

Consequences:

- Prerequisite resolution **cannot assume the target is in the same tree**, nor even that it is
  present in the current spec's node set at all. Unresolvable `requiresNode` targets are normal
  and must be handled, not treated as corrupt data.
- The engine's DAG is strictly per-tree, so hero-tree solving needs a notion of
  externally-satisfied or externally-irrelevant prerequisites. Model this as a pre-satisfied
  input mask supplied per job, so the engine stays per-tree and no algorithm change is needed.
- `fullNodeOrder` for Druid Balance has **282 entries** while the spec's own four arrays hold only
  120 ids — it covers the whole *class* trait tree across all specs (162 extra ids). That is the
  correct ordering basis for Blizzard hash construction, and it confirms `node_id_orders.txt`'s
  purpose.

## 5. What this means for the plan

1. **Raidbots stays the primary source** for the first release. It is live, it is the only
   verified-working source, and it already carries hero talents, tiered nodes, layout, and gating.
   The SimC/Blizzard-API comparison in
   [`talent-data-sources.md`](talent-data-sources.md) should be read as *fallback and
   verification* strategy, not as a prerequisite for starting.
2. **Descriptions remain the weak point.** This file carries `spellId`, `icon`, and names, but
   *not* tooltip text — which is why the legacy pipeline scraped Wowhead HTML. A real answer for
   descriptions is still needed (open question Q4).
3. **Spike S2 is largely de-risked** by this document; the remaining work is transformation, not
   discovery.
4. **Add tiered/`rankLevels` support to the data model** and treat level cap as an input that
   affects available ranks.
5. **Ingest must validate and fail loudly**: unknown node/entry types, degenerate nodes, sentinel
   `requiresNode: 0`, unresolvable prerequisites, and a changed class/spec set are all conditions
   this data demonstrably produces.

## Reproducing

```bash
curl -s -o talents.json https://www.raidbots.com/static/data/live/talents.json
python - <<'EOF'
import json; d=json.load(open('talents.json'))
print(len(d), 'specs'); print(sorted(d[0].keys()))
EOF
```
