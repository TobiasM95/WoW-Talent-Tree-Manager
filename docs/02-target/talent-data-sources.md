# WoW Talent Tree Data Sources — Research Report

Date of research: 2026-09-19. WoW's current retail expansion is **Midnight** (launched 2026-03-02), the second chapter of the Worldsoul Saga, following The War Within (TWW, 2024). This matters because SimulationCraft's `midnight` branch is now the default/live branch, and `thewarwithin` is the previous-expansion branch — both were inspected below.

Verification legend: **VERIFIED** = confirmed by fetching/curling the primary artifact myself (URL given). **CORROBORATED** = confirmed via a secondary/community source (e.g. a maintained API wrapper's source code) that reflects real behavior but isn't Blizzard's own docs. **UNVERIFIED/INFERRED** = could not directly confirm; stated as a reasoned inference, flagged explicitly.

---

## 1. SimulationCraft as a data source

### 1.1 Repo layout (VERIFIED via GitHub API/raw content, 2026-09-19)

- Default branch is **`midnight`** (`api.github.com/repos/simulationcraft/simc` → `"default_branch": "midnight"`). `thewarwithin` still exists as a separate branch for the prior expansion.
- Generated DBC/DB2-derived data lives in **`engine/dbc/generated/*.inc`** (C++ initializer-list source, compiled directly into the sim binary — not a runtime-loaded format). Confirmed present on both `midnight` and `thewarwithin`.
- Talent/trait-relevant generated files, confirmed present:
  - **`trait_data.inc`** / **`trait_data_ptr.inc`** — the modern (Dragonflight-onward) talent/trait node table. Present on both `midnight` and `thewarwithin`.
  - **`sc_talent_data.inc`** / **`sc_talent_data_ptr.inc`** — the **legacy pre-Dragonflight** `Talent.db2`-style talent table (tab/tier/column). Present on `thewarwithin` but **absent on `midnight`** — simc dropped it as dead weight once legacy support was no longer needed for the current game version. This is a concrete, dated signpost of how the schema is expected to keep shifting under a live tool.
  - **`class_spells.inc`** (new on `midnight`, replacing/supplementing `active_spells.inc`+`specialization_spells.inc` naming from `thewarwithin`) — active-ability list per class/spec, related but distinct from the talent graph itself.
- The header comment in `trait_data.inc` (`midnight`) reads `// Player trait definitions, wow build 12.1.0.69875`, confirming the file is regenerated per game build and stamped with the exact build number.

### 1.2 `trait_data_t` record layout (VERIFIED — fetched `engine/dbc/trait_data.hpp` from the `midnight` branch)

```cpp
struct trait_data_t {
  unsigned    tree_index;
  unsigned    id_class;
  unsigned    id_trait_node_entry;
  unsigned    id_node;
  unsigned    max_ranks;
  unsigned    req_points;          // points-in-tree gate
  unsigned    id_trait_definition;
  unsigned    id_spell;
  unsigned    id_replace_spell;
  unsigned    id_override_spell;
  short       row;                 // grid row, NOT raw pixel Y
  short       col;                 // grid col, NOT raw pixel X
  short       selection_index;     // for choice nodes
  const char* name;
  std::array<unsigned, 4> id_spec;
  std::array<unsigned, 4> id_spec_starter;
  unsigned    id_sub_tree;         // hero talent tree id
  unsigned    node_type;           // 0 normal, 1 tiered, 2 choice, 3 sub-tree-selection
};
```

A second struct, `trait_definition_effect_entry_t` (`id_trait_definition`, `effect_index`, `operation`, `id_curve`), carries scaling-curve links for talent effect magnitudes.

Sample row from `trait_data.inc` (midnight, build 12.1.0.69875):
```
{ 1, 1, 112179, 90322, 2, 23, 117184, 382896, 0, 0, 9, 4, 0, "Two-Handed Weapon Specialization", {71,0,0,0}, {0,0,0,0}, 0, 0 },
```

### 1.3 How `row`/`col`/`req_points` are actually computed (VERIFIED — read `dbc_extract3/dbc/filter.py` and `dbc_extract3/dbc/generator.py` on `midnight`)

- `filter.py`'s `TraitSet._filter()`:
  - `pos_x = round(entry['node'].pos_x, -2)` and an analogous `pos_y` — i.e., **`row`/`col` are derived from the real `TraitNode.PosX`/`PosY` fields**, bucketed into a grid index (`entry['row'] = ... index ... + 1`, `entry['col'] = ...`). So simc's grid is a legitimate, if lossy (rounded to hundreds, then rank-indexed), reduction of the true layout coordinates — not something invented independently.
  - `req_points` is populated from **`TraitCond.req_points`**, taking the max across matching node/group conditions (`max([...cond.req_points for cond in (node['cond'] | group['cond'])])`). So the points-required gate is read from the real `TraitCond` table.
  - `TraitGenerator` (in `generator.py`, class starting at line ~4903) builds each entry from `TraitNode`, `TraitNodeEntry`, `TraitDefinition`, `TraitDefinitionEffectPoints`, `TraitSubTree`, and `TraitTreeLoadout` — but I grepped both `filter.py` and `generator.py` (and `trait_data.cpp`/`trait_data.hpp`/`dbc.hpp`/`client_data.hpp`/`sc_data.cpp`) for the literal string **`TraitEdge`** and got zero matches anywhere in the codebase (`grep -n "TraitEdge" filter.py generator.py` → exit code 1, no hits).

### 1.4 Decisive answer: does simc carry full tree layout/graph data? — **No, not the edges.**

This is the load-bearing finding of this report, and it is **VERIFIED by reading simc's own extraction source, not inferred**:

- simc's raw upstream tooling (`dbc_extract3`) **does** know about the full modern trait schema — its per-build format manifests (e.g. `dbc_extract3/formats/12.1.5.69594.json`) explicitly define `TraitTree`, `TraitNode`, `TraitNodeEntry`, `TraitEdge`, `TraitCond`, `TraitNodeGroup`, `TraitSubTree`, `TraitCurrency`, `TraitTreeLoadout`, etc. — so the **capability to parse `TraitEdge`** (the real prerequisite-arrow table) exists in the toolchain.
- But the actual **consumer/generator code that produces `trait_data.inc`** only reads `TraitNode` (for position), `TraitNodeEntry`/`TraitDefinition` (for the talent/spell), and `TraitCond` (for the point-gate). It **never touches `TraitEdge`**. The resulting `trait_data_t` array therefore has: a grid position (row/col), a "points spent in tree" gate (req_points), rank/choice/subtree metadata — but **no explicit prerequisite-node-A-unlocks-node-B edge list**.
- Practical read: simc's data is enough to (a) validate that a talent-string encodes a legal allocation by points-spent thresholds and (b) know exactly which spell/effect a chosen node/rank grants. It is **not** enough, by itself, to draw the connector lines between nodes the way the in-game UI or Raidbots' talent calculator does — you would be reconstructing an approximation from row/col adjacency, which will be wrong for any tree where a real edge does not correspond to physical adjacency (this does happen, notably at hero-tree entry points and multi-parent nodes).
- Row/col/req_points is likely *sufficient* in practice for the great majority of Dragonflight-onward trees, because Blizzard's live talent trees mostly gate through points-spent rather than a dense arbitrary DAG — but "mostly" is not "always," and a tool that markets itself as a talent tree *manager* (built for point-and-click tree editing/planning) should not rely on this approximation when the real edge table is one CSV fetch away (see §2.2).

### 1.5 Tooling / update cadence (VERIFIED)

- `dbc_extract3/` (Python, `dbc_extract.py`, `casc_extract.py`, `generate.sh`/`generate.bat`) is the pipeline: `casc_extract.py` pulls the client build from Blizzard's CDN, `dbc_extract.py`/`dbc_extract3` parses the DB2/CASC files (`dbc/wdc1.py`…`wdc5.py` — multiple DB2 format-version parsers are present, meaning the tool has been updated across many client format revisions), and `generator.py` writes the `.inc` C++ source files.
- Regeneration is **per game build**, automated by GitHub Actions and bot-authored PRs. I found live evidence of this cadence directly on GitHub: PRs titled `[live] Game data update (Build 69814)`, `[live] Game data update (Build 69875)`, etc., described in their own text as *"created by the Raidbots hotfix watcher"* — i.e., a Raidbots-run bot watches for new WoW builds/hotfixes and opens simc data-update PRs automatically, essentially every time Blizzard ships a build or a hotfix DBCache change. (`.github/workflows/generate_files.yml` itself, which I fetched in full, only handles APL/profile regeneration on push — the actual spell/trait data-update automation lives in the bot-driven PR flow referenced above, not in a workflow file with an obvious name in this repo.)
- Consequence for a downstream tool: **there is no fixed cadence contract** ("every Tuesday", "every major patch") — updates land whenever Blizzard ships something and the bot notices, which can be same-day for major patches and irregular for hotfixes.

### 1.6 Practical consumption paths for `trait_data.inc`

1. **Parse the generated `.inc` C++ initializer list directly** with a small regex/parser (it's a very regular `{ a, b, c, ..., "Name", {..}, {..}, x, y },` grid) — no C++ toolchain required, just text parsing. This is the lowest-effort way to reuse simc's already-curated node list, but inherits the row/col/no-edges limitation from §1.4.
2. **Run `dbc_extract3` yourself** against a CASC/DBCache dump to get the full `TraitEdge`/`TraitCond`/`TraitNodeGroup` tables in JSON/CSV — this recovers the graph simc throws away, at the cost of running Python tooling against real game client data (which itself requires either owning a WoW install or hitting a CDN mirror).
3. **Skip simc's extraction step entirely** and pull the same underlying DB2 tables from a community mirror (wago.tools) that already publishes `TraitEdge` as CSV (see §2.2) — this is strictly less work than option 2 for a web app that only needs data, not a C++ build.
4. I did not find a Python package that exposes `dbc_extract3`'s per-build JSON format definitions (`dbc_extract3/formats/*.json`) as an installable library; `simc-support` (PyPI `simc-support`, GitHub `bloodmallet/simc_support`) is a related but separate community project focused on simc profile/APL metadata, not talent-tree topology — I did not deep-dive it since it wasn't central to the ask.

### 1.7 simc's talent string vs. Blizzard's loadout export string

- **CORROBORATED (via search of simc wiki/issue text + general knowledge; the SimC "Characters" wiki page could not be fully re-verified because GitHub wiki pages return errors to automated fetches)**: SimC's `talents=` field is simply **Blizzard's own in-game "Export" hash** — the same base64-ish string produced by the in-game talent UI's Export button or by the SimC WoW addon, unmodified. SimC does not invent its own encoding; it consumes Blizzard's `Blizzard_ClassTalentImportExport.lua` format directly and can also decode/re-encode it.
- Structurally (per community documentation of Blizzard's own export format, referenced from search results — **UNVERIFIED at the bit level** since I did not fetch Blizzard's Lua source itself): the string is a base64-encoded binary blob containing a class/spec identifier, a tree "checksum"/version marker (so old strings can be rejected if the tree has changed shape between patches), and a sequence of per-node purchased-rank/choice values in a fixed node-iteration order.
- SimC additionally lets you **override or hand-author** talents in a `.simc` profile using `class_talents=`, `spec_talents=`, `hero_talents=`-style `/`-delimited `name:rank` or `id:rank` pairs, layered on top of (or instead of) a full hash. This is documented on the simc wiki "Characters" page (title/nav confirmed reachable; page body was not fully retrievable via automated fetch, so treat the exact syntax as **CORROBORATED, not VERIFIED verbatim**).
- Practical implication: a web tool that wants to **import a player's real build** should implement Blizzard's export-string codec (well precedented — many third-party sites, e.g. Wowhead's talent calculator and various comparebuilds-style tools, already do this), not a simc-specific format; simc format compatibility falls out "for free" once you can decode Blizzard's hash, because that hash *is* what simc consumes.

---

## 2. Alternative / complementary sources

### 2.1 Official Blizzard Game Data API

The official docs site (`community.developer.battle.net/documentation/world-of-warcraft/game-data-apis`, reached via a confirmed 301 redirect from `develop.battle.net/...`) is **JS-rendered** and repeatedly returned only navigation chrome to automated fetching — I could not pull the endpoint table from it directly, despite several attempts. I instead cross-checked exact endpoint paths and response shapes against an actively-maintained third-party Go client (`Thenecromance/Go_Blizzard_API`) whose generated package docs mirror the real response structs, and against a maintained Python wrapper (`trevorphillipscoding/python-blizzardapi`, which has a live test suite hitting real namespaces). This is **CORROBORATED**, not VERIFIED against Blizzard's own prose, and is flagged as such.

Endpoints (namespace `static-{region}`, e.g. `static-us`):
| Endpoint | Path | Notes |
|---|---|---|
| Talent Tree Index | `/data/wow/talent-tree/index` | Lists `class_talent_trees[]`, `spec_talent_trees[]`, and **`hero_talent_trees[]`** |
| Talent Tree (nodes for one spec) | `/data/wow/talent-tree/{talentTreeId}/playable-specialization/{specId}` | Full node graph, see below |
| Talent Tree (class-only) | `/data/wow/talent-tree/{talentTreeId}` | `talent_nodes[]` + `spec_talent_trees[]` refs |
| Talent Index | `/data/wow/talent/index` | Flat list of all talents |
| Talent | `/data/wow/talent/{talentId}` | Single talent, spell ref, rank descriptions |
| PvP Talent Index / PvP Talent | `/data/wow/pvp-talent/index`, `/data/wow/pvp-talent/{id}` | |
| Playable Specialization | `/data/wow/playable-specialization/{specId}` | Includes **`hero_talent_trees[]`** and `spec_talent_tree` |
| Playable Specialization Index | `/data/wow/playable-specialization/index` | |

The Talent Tree node payload (per the Go client's `TalentTreeModel`, corroborated) includes, per node: `id`, `nodeType`, `displayRow`, `displayCol`, **`rawPositionX`/`rawPositionY`**, **`lockedBy`**, **`unlocks`**, `ranks[]` (each rank with `tooltip.spellTooltip`, `tooltip.talent`, `defaultPoints`), and `choiceOfTooltips` for choice nodes — separated into `class_talent_nodes[]`, `spec_talent_nodes[]`, and `hero_talent_trees[]` (each hero tree being its own sub-graph). **This is important: `lockedBy`/`unlocks` means the official API — unlike simc's reduced dataset — does expose the real prerequisite-edge relationships**, alongside genuine pixel-space coordinates (`rawPositionX/Y`), not just a rounded grid bucket.

- **Auth**: OAuth2 client-credentials flow (Battle.net Developer Portal app, client ID + secret) — CORROBORATED via multiple wrapper libraries' setup instructions (`get-wow-data` explicitly documents `wow_api_id`/`wow_api_secret` env vars). No user login needed for Game Data endpoints, only for Profile endpoints.
- **Rate limits**: widely reported by third parties as roughly 36,000 requests/hour and 100 requests/second per client credential — I could **not verify this figure against Blizzard's own docs** (blocked by the JS-rendering issue above), so treat the specific numbers as **UNVERIFIED** (commonly cited, not independently confirmed here).
- **Classic namespaces**: confirmed distinct namespaces exist — `static-classic-{region}` (Progression/Cataclysm-era realms) and `classic1x-{region}` (Classic Era/Hardcore realms use `classic1x-` per a 2024 Blizzard forum change: Era realms moved to `classic1x-`, Progression realms kept `classic-`). **However**, I could find **zero evidence of a talent-related method in the classic branch of a well-maintained Python wrapper that otherwise implements ~25 other classic Game Data endpoints** (creature, item, guild-crest, playable-class/race, power-type — all present; talent — absent, in both the implementation file and its test file). This is a strong (if indirect) signal that **Blizzard's Classic Game Data API does not expose talent-tree data at all**, but it is an **absence-of-evidence inference**, not a documented statement from Blizzard, so I flag it as **UNVERIFIED/INFERRED**.

### 2.2 Community raw-DBC mirrors

- **wago.tools** — **VERIFIED live and working today.** `https://wago.tools/db2/{TableName}/csv` returns a full CSV dump of any DB2 table, no auth, no API key. I pulled and confirmed real column headers for:
  - `TraitNode`: `ID,TraitTreeID,PosX,PosY,Type,Flags,TraitSubTreeID`
  - `TraitEdge`: `ID,VisualStyle,LeftTraitNodeID,RightTraitNodeID,Type` — **this is the actual prerequisite-arrow table simc discards**; `Type` distinguishes plain-visual vs. gating edges per the Warcraft Wiki description in §2.4.
  - `TraitNodeEntry`: `ID,TraitDefinitionID,MaxRanks,NodeEntryType,TraitSubTreeID`
  - `TraitCond`: `ID,CondType,TraitTreeID,GrantedRanks,QuestID,AchievementID,SpecSetID,TraitNodeGroupID,TraitNodeID,TraitNodeEntryID,TraitCurrencyID,SpentAmountRequired,Flags,RequiredLevel,...`
  - wago.tools is the acknowledged community successor to the now-largely-deprecated wow.tools (Marlamin's own site), per multiple corroborating sources; wow.tools stopped archiving new builds in early 2023 and a locally-hostable clone (`Marlamin/wow.tools.local`) exists for those who still need the old UI.
  - No formal published API contract/SLA was found (it's a community hobby project); treat schema stability and uptime as best-effort, not guaranteed.
- **WoWDBDefs** (`github.com/wowdev/WoWDBDefs`) — **VERIFIED**: this is the canonical, actively-maintained (most recent commit in the search results was a same-generation build `12.1.0.69299` merge) community repository of DB2 **schema definitions** (`.dbd` text files, one per table, versioned by build-range headers). It is the schema WoW-tooling projects (wago.tools, WoWDBDefs-consuming parsers, this project's own future extractor) should build against for both retail and Classic tables. I fetched `Talent.dbd` directly and it has entries covering builds from `0.7.0` (original alpha) through `1.13.7.x` (current Classic Era), confirming full historical + Classic Era coverage of the *old* talent schema alongside the new `Trait*.dbd` family for retail.
- **Raidbots** — **VERIFIED, and the standout practical finding of this research.** `https://www.raidbots.com/static/data/live/talents.json` is a public, unauthenticated, CORS-enabled (`access-control-allow-origin: *`) static JSON file, ~3.1 MB, served via Cloudflare/GCS, `Cache-Control: public, max-age=2678400` (31 days), `last-modified: Tue, 25 Aug 2026` at fetch time — i.e. it had been refreshed within the current patch cycle. A PTR variant exists at `.../static/data/ptr/talents.json` (also live, HTTP 200). I found **no** classic equivalent at the obvious guessed paths (`.../classic/talents.json`, `.../cata/talents.json` both 404) — absence noted but not exhaustively searched.
  - **This file already reconstructs the full graph** that simc discards: each of the 40 class/spec entries has `classNodes[]`, `specNodes[]`, `heroNodes[]`, and `subTreeNodes[]`, and **every node carries `posX`/`posY` (real coordinates, e.g. `2100,1500`), `next[]`/`prev[]` (explicit edges), `reqPoints`, `maxRanks`, `entryNode`, `freeNode`/`freeLevel`, `requiresNode`, and `entries[]`** (the choice-node options, each with `spellId`, `icon`, `type: active|passive`). This is essentially a ready-to-render talent-tree data model, sourced (per Raidbots' own developer messaging referenced in search results) from the same CASC/DBC family of tools as simc, but evidently reconstructed by Raidbots' own pipeline rather than reused verbatim from simc's `trait_data.inc` (since it has the edges simc's `.inc` files lack).
  - No documented SLA/versioning guarantee was found for this endpoint; it is not officially part of a published API contract, just a static asset Raidbots happens to expose. Treat it as **free, extremely convenient, but unofficial and revocable at any time**.
- **Wowhead** — scraping-only, as expected; no public data API/export was found or claimed. Not evaluated further given ToS risk and the far better options above.
- **Keystone.guru / Archon-style sites** — not directly investigated in depth (out of scope relative to the stronger sources found); these are generally consumers of the same DB2/Raidbots/Blizzard-API data rather than independent sources, so they add little beyond what's listed above.

### 2.3 Relevant DB2 tables (VERIFIED column-level via wago.tools CSV headers where noted; others via Warcraft Wiki's Dragonflight Talent System page, which is itself sourced from datamining and is treated as CORROBORATED)

| Table | Carries | Verification |
|---|---|---|
| `TraitTree` | One row per class talent-tree "system"; currency/cost config | CORROBORATED (wiki) |
| `TraitNode` | `PosX`, `PosY`, `Type`, `TraitSubTreeID` — the UI "button" | **VERIFIED** (wago.tools CSV header) |
| `TraitNodeEntry` | `TraitDefinitionID`, `MaxRanks`, `NodeEntryType`, `TraitSubTreeID` — one/two per node (choice nodes have 2) | **VERIFIED** (wago.tools CSV header) |
| `TraitEdge` | `LeftTraitNodeID`, `RightTraitNodeID`, `Type`, `VisualStyle` — **the prerequisite arrows** | **VERIFIED** (wago.tools CSV header) |
| `TraitCond` | Point-spend gates, level gates, quest/achievement gates, per-node or per-group | **VERIFIED** (wago.tools CSV header) |
| `TraitNodeGroup` / `TraitNodeGroupXTraitNode` / `...XTraitCond` / `...XTraitCost` | Applies shared gating/cost to a bundle of nodes at once | CORROBORATED (wiki) |
| `TraitSubTree` | Hero-talent tree metadata: name, icon atlas, parent `TraitTreeID`, `centerX`/`topY` for coordinate offsetting | CORROBORATED (wiki) |
| `TraitDefinition` / `TraitDefinitionEffectPoints` | Talent → spell mapping, effect scaling via curves | CORROBORATED (wiki) + VERIFIED struct usage in simc |
| `SpecSetMember` | Maps a `SpecSetID` (used by `TraitCond`) to concrete spec IDs | CORROBORATED (wiki) |
| `TraitTreeLoadout` / `TraitTreeLoadoutEntry` | Blizzard-provided preset loadouts (e.g. "Recommended" builds) | **VERIFIED** referenced directly in simc's `generator.py` (`TraitLoadoutGenerator`) |

Layout/graph fields, concretely: **position** = `TraitNode.PosX/PosY`; **prerequisites/edges** = `TraitEdge.LeftTraitNodeID/RightTraitNodeID/Type`; **point gating** = `TraitCond.SpentAmountRequired` (+ `RequiredLevel`, `QuestID`, etc. for non-point gates); **choice nodes** = a `TraitNode` with 2 `TraitNodeEntry` rows; **max ranks** = `TraitNodeEntry.MaxRanks`; **hero trees** = `TraitSubTree` + `TraitNode.TraitSubTreeID`.

---

## 3. Hero talents (TWW+): structure and what a tool must model

**CORROBORATED** (Warcraft Wiki's Dragonflight Talent System page, cross-checked against Raidbots' live JSON structure in §2.2, which matches it exactly in practice):

- Each spec's `TraitTree` has **one hidden selection node per spec** of `TraitNodeType.SubTreeSelection` (node_type `3` in simc's own enum, confirmed in `trait_data.hpp`), with exactly as many `TraitNodeEntry` options as the spec has hero trees (almost always 2). Choosing one calls `C_Traits.SetSelection()` in-game.
- Each hero tree is a **`TraitSubTree`** row with its own name/icon/`centerX`/`topY`, and its own **separate node graph** (a set of `TraitNode` rows carrying that `TraitSubTreeID`) with its own currency (`TraitCurrency`, one per subtree, shared cost pool across the classes that can pick it where relevant) — i.e., hero trees are not a cosmetic filter over the normal spec tree, they are **structurally their own mini-tree** grafted onto the spec tree at the selection node, confirmed directly in the Raidbots JSON (`heroNodes[]` is a distinct array per spec entry, and `subTreeNodes[0].entries[]` lists, per hero-tree option, exactly which node IDs belong to it).
- Node coordinates for a hero subtree are given **relative to the subtree's own origin** and need re-projecting into the shared canvas using the formula documented on the wiki: `(nodePosX / 10) − subTreePosX`, `(−nodePosY / 10) + subTreePosY`, using the `TraitSubTree.centerX/topY` offsets — i.e., **a renderer cannot treat all of a spec's nodes as one flat coordinate space**; hero-tree nodes require this offset transform.
- **What a tool must model**, concretely: (1) a normal class/spec talent graph, (2) a per-spec list of 2 (usually) selectable hero trees, each a full independent sub-graph with its own edges/points-gates, (3) the hidden selection node and the mutual exclusivity it enforces, (4) the coordinate-offset transform to place hero-tree nodes on the shared canvas, and (5) validation that a build only "spends" hero points in the currently-selected subtree.

This is almost certainly **exactly the reason the old HTML-scraper pipeline broke**: hero talents (introduced with TWW, i.e. after this project's last working pipeline) do not exist as a simple extension of the old 3-tab/DOM-scrapeable talent-calculator page structure — they're a materially different graph-of-graphs data model requiring dedicated schema support, not a scraper tweak. I was not able to inspect the project's actual old scraper code as part of this task (out of scope — this was a research task, not a codebase audit), so treat "this is what broke it" as a **strong, well-supported inference**, not a verified root-cause diagnosis of the specific old code.

---

## 4. Classic support

- **SimulationCraft itself has no maintained Classic-client support.** The branches named `mop`, `wod`, `tbc`, `legion-dev`, `bfa-dev` in the simc repo are **VERIFIED, by inspecting their file trees directly**, to be old **retail development branches from when those expansions were the live game** (e.g. the `mop` branch still uses a pre-CMake build system — `.xcodeproj`, `CMakeLists.txt.old`, `win32_release_msvc11.bat` — clearly a ~2012-2014-era snapshot, not a "MoP Classic" adapter). They are frozen historical artifacts, not classic-flavor ports.
- A dedicated **`EPIC: Classic Support`** issue (simc#5821, opened March 2021) explicitly requests Classic/TBC support as community-contributed work, lists "talent tree restructuring beyond the current simple ranking system" as an open requirement, and — per its content — remains **unowned/unresolved** for most classes. This is **VERIFIED by reading the issue**, and it directly confirms simc is not a viable Classic data source today (2026).
- The actual community tool for Classic-era simulation is **WowSims** (`wowsims.com`, `github.com/wowsims/cata` etc.) — a **separate Go-based codebase**, not a simc fork, with its own data pipeline. I did not deep-dive its extraction internals (out of the core scope, which was simc-focused), but its existence as the de facto standard is well corroborated by search results (in-game exporter addons, active Cataclysm-Classic-specific repo).
- **Classic's talent data model is fundamentally simpler and structurally different from retail**, confirmed directly from **`Talent.dbd`** in WoWDBDefs (fetched and VERIFIED): the classic-era `Talent` table has `TierID` (row), `ColumnIndex` (0–2, three-tab-of-columns layout), `TabID` (which of the 3 tabs), `PrereqTalent[3]`/`PrereqRank[3]` (**prerequisites embedded as direct foreign keys on the talent row itself**, not a separate edge table), `SpellRank[9]` (up to 9 ranks per talent, each a distinct spell ID), and `ClassID`. Points-required-per-tier gating (5/10/15/... points spent in the tab to unlock the next tier) is a client-side/UI constant, not a per-row stored value in this table (i.e., simpler but also less self-describing than modern `TraitCond`).
- **Blizzard's Classic Game Data API almost certainly does not expose talent data.** Confirmed namespaces exist (`static-classic-{region}` for Progression/Cata-Classic realms, `classic1x-{region}` for Classic Era/Hardcore realms post a 2024 Blizzard-forum-announced split) — but a maintained Python wrapper library that implements ~25 other Classic Game Data endpoints (creature, item, item-set, guild-crest, playable-class/race, power-type, both implementation and test files checked) has **zero talent-related methods or tests**. This is an **UNVERIFIED/INFERRED** conclusion (absence of evidence in a third-party wrapper is not a Blizzard statement), but it is a reasonably strong signal.
- **Can one schema cover both retail and classic?** Only at a fairly abstract level. A shared conceptual model (`Node{id, position, maxRanks, spellId(s), edges[], gate}` / `Tree{nodes[], edges[]}`) can represent both, but the **gating semantics differ in kind**: modern retail is "points-spent-in-tree" threshold gating (`TraitCond.SpentAmountRequired`) layered with an edge graph that's mostly cosmetic/entry-point-defining; classic is "row/tier × points-spent-in-tab" gating **plus mandatory direct single/multi-parent prerequisites** (`PrereqTalent`/`PrereqRank`) that are load-bearing, not decorative. A data layer that models "edges" as optional visual hints (fine for retail) will under-model classic, where an edge is often the *only* gate. Recommend an explicit `gate` type enum per tree generation (`points_threshold` vs. `prereq_chain`) rather than pretending one gating primitive covers both eras.

---

## 5. Recommendation

### Ranked source strategy

1. **Primary (retail): Raidbots' `static/data/live/talents.json`.** It is free, unauthenticated, CORS-open, already contains the full graph (positions + edges + hero-tree sub-graphs + choice-node entries) that simc's own generated files lack, and is demonstrably kept in sync with live patches (last-modified within the current patch window as of this research). It requires **zero data-pipeline work** on this project's part beyond a scheduled fetch + schema-mapping layer. Ship the PTR variant (`.../ptr/talents.json`) behind a "preview/PTR" toggle for free, using the same code path.
2. **Fallback / cross-check / long-term independence: raw DB2 via wago.tools CSV exports of `TraitTree`, `TraitNode`, `TraitNodeEntry`, `TraitEdge`, `TraitCond`, `TraitSubTree`, `TraitDefinition`.** This is the "own the pipeline, don't depend on one hobby project's goodwill" option — it's the actual source-of-truth data (not someone else's reconstruction of it), it's schema-documented via WoWDBDefs `.dbd` files (so you can detect and adapt to schema changes build-over-build rather than being surprised), and it doesn't require running simc or CASC tooling yourself. Treat this as the thing to build toward once the Raidbots-based MVP is working, and as the thing to fall back to if Raidbots ever changes/removes the endpoint.
3. **Do not build on SimulationCraft's generated files as the primary tree-layout source.** Use simc data (or the official Blizzard API) only for what it's actually good for: spell/effect IDs, tooltip text via the Blizzard API, and cross-validation of "does this talent-string decode to a legal build." It is the wrong primary source for *rendering a tree*, because it deliberately discards `TraitEdge`.
4. **Official Blizzard Game Data API** as a third-tier fallback/validator, not primary: it has the most complete and most "official" per-node data (real edges via `lockedBy`/`unlocks`, real pixel coordinates, hero trees) — CORROBORATED but not directly confirmed against Blizzard's own docs due to their JS-rendered site blocking automated verification — but it requires OAuth app registration/secret management, has rate limits, and is one more moving part than a static JSON fetch. Good for verifying a user's real character build, less good as the thing that drives your tree-editor UI's canonical data.
5. **Classic**: no good talent-graph API or feed exists. Plan to hand-author/import Classic trees once from WoWDBDefs' `Talent.dbd`-shaped data (one-time-ish, since classic content is largely static/slow-moving relative to retail), rather than building an automated per-patch pipeline for it. Treat Classic as a fundamentally different (simpler) data model with its own gate semantics (see §4), not a variant of the retail schema.

### Sketch of an automated ingestion job

- **Trigger**: scheduled (e.g. every 6–12 hours) fetch of `https://www.raidbots.com/static/data/live/talents.json` (and the `ptr` variant on a separate schedule/branch), keyed off the file's `ETag`/`Last-Modified` response headers (both confirmed present) — only reprocess on change, don't re-ingest identical bytes.
- **Patch detection**: compare a content hash (or the `etag` header) against the last-ingested value; a change is your "new patch/hotfix landed" signal. Cross-check against `traitTreeId`/node-id churn per spec to distinguish "cosmetic tooltip text changed" from "tree topology changed" if you want finer-grained changelposting.
- **Normalization output**: map Raidbots' per-spec JSON (`classNodes`/`specNodes`/`heroNodes`/`subTreeNodes`) into this project's own internal tree schema — one pass per class/spec, with hero-tree nodes re-projected using the coordinate-offset math from §3 if you want a single flat canvas, or kept as separate sub-graphs if your UI already supports a "selectable sub-panel" concept.
- **Fail loudly, don't silently degrade**: assert expected top-level shape on every ingest (40 spec entries present for retail as of this research; each entry has non-empty `classNodes`/`specNodes`; every referenced `next`/`prev`/`requiresNode` id resolves to a real node in the same or a linked sub-graph; every `entries[].spellId` is non-null). If any of these invariants fail, **abort the ingest and keep serving the last-known-good dataset**, paired with an alert — never publish a half-parsed tree silently, since a subtly-broken tree (missing edges, wrong hero-tree offsets) is worse for users than a stale-but-correct one. Specifically instrument for the two known-fragile points: (a) Raidbots changing or removing the static-data path/shape without notice (it's an unofficial convenience endpoint, not a contract), and (b) Blizzard adding a new node type or gating mechanism your normalizer doesn't recognize yet (log and hard-fail on any `nodeType`/`type` value your mapper doesn't already have a case for, rather than defaulting silently).
- **What to prototype first, to de-risk this whole approach before investing in the full ingestion job**: write a throwaway script that fetches `talents.json` once, picks one spec with hero talents (e.g. Balance Druid, confirmed above to have `heroNodes`/`subTreeNodes`), and renders it — including hero-tree node re-projection via the `centerX/topY` offset formula — as a static SVG or simple canvas drawing with edges as lines. If that one spec renders correctly (positions look right, edges connect the right nodes, the two hero-tree options are visually distinguishable/switchable), the hard, decisive risk (whether externally-sourced JSON is actually sufficient to reconstruct what the old scraper used to get from the DOM) is retired, and the rest is normalization/scaling work across 39 more specs, not new unknowns.

---

## Sources consulted

- https://github.com/simulationcraft/simc (repo root, branches via `api.github.com/repos/simulationcraft/simc/branches`)
- https://github.com/simulationcraft/simc/tree/midnight/engine/dbc/generated and .../thewarwithin/engine/dbc/generated (directory listings via GitHub Contents API)
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/engine/dbc/generated/trait_data.inc
- https://raw.githubusercontent.com/simulationcraft/simc/thewarwithin/engine/dbc/generated/sc_talent_data.inc
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/engine/dbc/generated/class_spells.inc
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/engine/dbc/trait_data.hpp
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/dbc_extract3/README.md
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/dbc_extract3/dbc/filter.py
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/dbc_extract3/dbc/generator.py
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/dbc_extract3/formats/12.1.5.69594.json
- https://raw.githubusercontent.com/simulationcraft/simc/midnight/.github/workflows/generate_files.yml
- https://github.com/simulationcraft/simc/wiki/GameClientData and .../Using-CASC-Extract-and-DBC-Extract (partially fetchable; wiki page bodies intermittently failed to render for automated fetch)
- https://github.com/simulationcraft/simc/issues/5821 (Classic Support epic)
- https://github.com/simulationcraft/simc/pull/11880, /11877, /11884 etc. (`[live] Game data update (Build ...)` bot PRs, found via search, confirming Raidbots-bot-driven update cadence)
- https://warcraft.wiki.gg/wiki/Dragonflight_Talent_System (Trait* table field descriptions, hero-tree selection mechanics, coordinate-offset formula)
- https://wago.tools/db2/TraitNode/csv, /TraitEdge/csv, /TraitTree/csv, /TraitNodeEntry/csv, /TraitCond/csv (live-fetched CSV headers and sample rows)
- https://github.com/wowdev/WoWDBDefs (repo), raw `definitions/Talent.dbd` (fetched in full for the classic-era layout history)
- https://www.raidbots.com/static/data/live/talents.json and .../ptr/talents.json (live-fetched, parsed with Node.js, structure and header inspection)
- https://community.developer.battle.net/documentation/world-of-warcraft/game-data-apis (redirect target confirmed from https://develop.battle.net/documentation/world-of-warcraft/game-data-apis; page body is JS-rendered and could not be retrieved by automated fetch despite repeated attempts)
- https://pkg.go.dev/github.com/Thenecromance/Go_Blizzard_API/api/wow/DataService/Talent and .../PlayableSpecialization (community Go client docs, used to corroborate endpoint paths/response fields)
- https://github.com/trevorphillipscoding/python-blizzardapi, files `blizzardapi/wow/wow_game_data_api.py`, `wow_classic_game_data_api.py`, `tests/test_wow_classic_game_data_api.py` (used to corroborate endpoint set and the absence of Classic talent endpoints)
- https://github.com/JackBorah/get-wow-data (OAuth client-credentials setup corroboration)
- World of Warcraft: Midnight release-date confirmation: https://news.blizzard.com/en-us/article/24243639/world-of-warcraft-midnighttm-goes-live-march-2 and corroborating press coverage (GameSpot, Wikipedia, Warcraft Wiki, wccftech)
