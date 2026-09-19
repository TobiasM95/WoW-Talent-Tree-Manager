# Target data model

Status: proposal, pre-implementation. Synthesises the findings in
[`../01-current-state/data-formats.md`](../01-current-state/data-formats.md) and
[`../01-current-state/data-pipeline-and-scraper.md`](../01-current-state/data-pipeline-and-scraper.md).

## 1. The one rule: every stored reference is keyed by a stable identifier

The most consequential defect in the legacy data model is that **stored and shared data is
positional**. A skillset is serialised as a bare list of point values, matched to talents by
iterating `tree.orderedTalents` in the same order on both the writing and the reading side
(`Engine/src/TalentTrees.cpp`, `createSkillsetStringRepresentation` / `importSkillsets`).
Validation checks only that the *count* matches.

Meanwhile `tree_presets_generator.py` re-sorts raidbots nodes by `(row, column)` into a fresh
`0..N-1` index on **every regeneration**. So when Blizzard moves a node, or adds one, or the
generator's sort changes, every previously-shared build silently points at the wrong talents.
No error, no warning — just wrong data that still looks plausible.

Three notions of identity are conflated today:

| Identity | Stable? | Used for |
|---|---|---|
| Blizzard `nodeID` | **Yes** — the only durable id | Blizzard hash import/export; written by the preset generator only |
| TTM internal `index` | No — reassigned on regen/reindex | parent/child edges, skillset map keys |
| positional array order | No — implicit, unverifiable | the skillset share string (no key at all) |

Worse, `nodeID` is written by the offline preset generator but **never** by the live app's own
`createTreeStringRepresentation` — so the stable id is discarded the moment a user edits and
re-saves a tree.

**Rule for the rewrite:** persisted and shared references use `nodeId` (Blizzard's) for
game-derived nodes and a generated UUID (`localId`) for user-authored custom nodes. Positional
order may exist only as a transient, in-process detail of the solver — never in storage, never
on the wire, never in a share code.

## 2. Canonical entities

Four concepts, deliberately separated. The legacy model fused tree definition, point
assignment, solver state, and sim results into one mutable `TalentTree` struct
(`Engine/src/TalentTrees.h`), which is why nothing could be cached, shared, or versioned
independently.

- **Tree** — an immutable, versioned *definition*: node graph, layout, ranks, gating.
  Game-derived trees are snapshots of an ingest run; custom trees are user-authored.
  `gating` is an explicit, pluggable field (`"reqPoints"` for retail, `"prereqChain"` for classic)
  rather than an implicit assumption — see open question Q3. In retail the edges are largely
  cosmetic and `pointsRequired` is the real gate; in classic-era trees the prerequisite chain is
  mandatory and gating is per-tab. Hardcoding either assumption blocks the other era.
- **Build** (legacy name: "skillset") — a point assignment against one specific tree revision.
- **Loadout** — a named collection of builds for one tree, plus description.
- **SolveJob** — a request to enumerate valid builds under constraints, plus its results.

### Tree

```jsonc
{
  "schemaVersion": 1,
  "id": "uuid",
  "revision": 7,                    // bumped per ingest; builds pin this
  "kind": "class" | "spec" | "hero" | "custom",
  "game": "retail" | "classic",
  "gameVersion": "11.2.0",          // patch this snapshot came from
  "classId": 2, "specId": 105,      // Blizzard ids; null for custom
  "name": "Restoration Druid",
  "description": "",
  "source": { "provider": "raidbots", "fetchedAt": "2026-09-19T06:20:08Z" },
  "pointCap": 30,                   // points spendable in THIS tree
  "nodes": [ /* Node */ ]
}
```

### Node

```jsonc
{
  "nodeId": 82043,                  // Blizzard nodeID — primary key for game trees
  "localId": null,                  // uuid for custom nodes; exactly one of the two is set
  "kind": "single" | "choice" | "tiered" | "subtree",   // upstream node type
  "maxPoints": 3,
  "rankLevels": null,               // tiered nodes: [{level, maxRanks}] — see below
  "pointsRequired": 8,              // gate: points spent in this tree before unlocking
  "preFilled": false,               // upstream freeNode
  "freeLevel": null,                // level at which a free node is granted
  "row": 4, "column": 9,            // derived from upstream posX/posY
  "entries": [                      // 1 entry normally; 2+ for choice/tiered nodes
    { "entryId": 123, "spellId": 774, "name": "Rejuvenation", "kind": "passive",
      "icon": "spell_nature_rejuvenation", "ranks": ["desc r1", "desc r2"] }
  ],
  "parents":  [82041, 82042],       // nodeId/localId references — never indices
  "children": [82050],
  "requiresNode": null,             // cross-tree prerequisite; null, never 0
  "subTreeId": null                 // which hero sub-tree this node belongs to
}
```

Node `kind` mirrors the upstream vocabulary (`single`, `choice`, `tiered`, `subtree`) rather than
being inferred. The legacy generator inferred type from `choice` or `maxRanks > 1` alone and so
silently mis-handled both `tiered` and `subtree`. Entry `kind` is `passive` / `active` /
`tierrank` / `subtree`. **Unknown values must fail the ingest loudly, not fall through.**

### Tiered nodes: maxPoints is a function of level

Verified in live data ([`raidbots-live-schema.md`](raidbots-live-schema.md)): there are exactly 40
`tiered` nodes — one per spec — and each carries `rankLevels` such as
`[{level:81,maxRanks:1},{level:84,maxRanks:3},{level:90,maxRanks:4}]`. A node's maximum rank
therefore depends on **character level**, not only on points spent.

The engine cannot express this: `Talent::maxPoints` is a single static integer that
`expandTreeTalents` bakes into a fixed number of bitset slots. So the effective `maxPoints` must
be resolved **against the build's level cap before the tree is handed to the solver**, which means
the expanded slot count — and thus the 64-bit budget — varies with level cap. Treat level cap as a
solver input, not a display-only field.

Also note `requiresNode: 0` is upstream's sentinel for "no prerequisite" (17 nodes use it).
Normalise it to `null` on ingest; code that treats `0` as a node id will invent a bogus edge.

### Build

```jsonc
{
  "schemaVersion": 1,
  "id": "uuid",
  "treeId": "uuid",
  "treeRevision": 7,                // pins the exact snapshot
  "name": "Raid single-target",
  "levelCap": 80, "useLevelCap": true,
  "selectedSubTreeId": 12345,       // which hero sub-tree, null pre-TWW/classic
  "points":  { "82043": 2, "82050": 1 },      // keyed by nodeId, ALWAYS tagged
  "choices": { "82061": "entry:124" }         // which entry of a choice node
}
```

`treeRevision` is what makes shared builds safe: a build always knows which tree snapshot it was
valid against, so the app can detect staleness and offer a migration rather than silently
misreading the points.

## 3. Hero talents: the structural gap

This is the actual reason the project died, and it is **not** a scraper bug. Two facts establish
that:

1. `presets.txt` holds 78 game trees = 39 specs × (class tree + spec tree), joined only by
   `complementaryTreeIndex` / `complementarySkillsetIndex` on `TalentTree`
   (`Engine/src/TalentTrees.h`). There is no slot for a third sub-tree.
2. Raidbots' `talents.json` **already publishes** `heroNodes` / `subTreeNodes`, with subtree
   choice and cross-tree `requiresNode` prerequisites — verified live, see
   [`raidbots-live-schema.md`](raidbots-live-schema.md). The generator only ever reads
   `classNodes` / `specNodes` (`Engine/resources/cicd_presets/tree_presets_generator.py`).

The upstream source did not break. The consumer's two-tree data model did.

A hero sub-tree is **shared between two specs of a class**, and `traitTreeId` is per class rather
than per spec. So a hero node's `requiresNode` can point into the *sibling* spec's spec tree — for
example Druid Balance's "Boundless Moonlight" (node 94608) requires node 92587, which lives in
Druid **Guardian**'s spec nodes. Unresolvable prerequisites are therefore normal and expected data,
not corruption.

Modelling requirements this imposes:

- A spec has **N trees**, not 2: class + spec + one-of-several hero sub-trees. Model the
  spec→trees relation as a list, never a pair.
- A build must record **which hero sub-tree is selected**, and hero nodes must be gated on that
  selection.
- `requiresNode` crosses tree boundaries, so prerequisite resolution cannot assume the
  prerequisite lives in the same tree. The legacy engine's per-tree DAG assumes it does — this is
  the one place where hero talents genuinely break the *engine*, not just the data model.
- Hero trees must be solved as their own independent tree. A hero tree is small (~10-11 nodes),
  so it fits the solver's 64-slot bitset easily; a *combined* class+spec+hero solve would not.
  See [`../01-current-state/engine-and-solver.md`](../01-current-state/engine-and-solver.md) and
  [`architecture.md`](architecture.md).

## 4. Interop formats: keep, but quarantine

These are external contracts. Keep them, generate them on demand, and confine them to an
explicit import/export boundary — never as internal storage.

| Format | Decision | Note |
|---|---|---|
| Blizzard loadout hash | **Keep** (interop) | Bit widths are an external contract copied from Blizzard's Lua. Version byte pinned to `1`; the 128-bit `tree_hash` field is **always zero-filled on export and discarded on import**, so exported strings are technically non-conformant and stale-tree mismatches go undetected. Fix the hash field; keep the codec. |
| SimC `talents=` strings | **Keep** (interop) | Generate on demand. `simcTokenizeName` is lossy with no collision detection. |
| TTM tree string | **Replace** with JSON | Keep a one-time importer so existing users' `%APPDATA%` files aren't orphaned. |
| TTM skillset string | **Replace** with JSON | Positional and unsafe; importable only together with the exact tree it came from. |
| `presets.txt` / `node_id_orders.txt` | **Replace** with JSON | Keep the *concept* of a spec→`fullNodeOrder` table; it is required to build Blizzard hashes. |
| SimC sim-result text scraping | **Replace** | Use SimC's structured JSON report instead of literal-prefix string slicing. |

## 5. Share codes

Do not hand-roll bit packing for general data. Use a self-describing envelope:

```
ttm1.<base64url(gzip(canonical JSON))>
```

The `ttm1.` prefix lets the importer auto-detect TTM codes vs. a raw Blizzard hash vs. a SimC
string, and `schemaVersion` inside the payload allows real migrations. Server-side short links
should store *this same blob* rather than invent a second format. Reserve true bit-packing for
the Blizzard hash alone, where the widths are an external requirement.

## 6. Postgres mapping

Consistent with the "do a lot in Postgres" goal, and correcting the legacy schema's problems
(no FKs, no constraints anywhere, and a global `ContentID` space shared across six tables that
required multi-table probing just to resolve an entity's type):

- `trees` — one row per tree *revision*; `(id, revision)` primary key; `definition jsonb`
  holding the node graph. The graph is read whole and rarely queried field-wise, so `jsonb`
  beats a row-per-node table. Add a GIN index only if node-level querying actually materialises.
- `builds` — `points jsonb` keyed by nodeId, plus a real FK `(tree_id, tree_revision)` →
  `trees`. Normalised per-assignment rows are the alternative; `jsonb` is acceptable here
  **because** assignments are keyed rather than positional — the legacy objection to a blob was
  really an objection to a *positional* blob.
- `loadouts`, `loadout_builds` — ordinary relational rows.
- `users`, `sessions` — see [`architecture.md`](architecture.md).
- `solve_jobs`, `solve_results` — queue and results; see [`architecture.md`](architecture.md).
- Use real foreign keys, `NOT NULL`, and `CHECK` constraints throughout. One id space per
  entity, never a shared one. Validate `schemaVersion` on every write.
