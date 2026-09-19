# WoW Talent Tree Manager — Persisted / Shareable Data Formats

This document reverse-engineers every on-disk and shareable string format used by the
C++/Dear ImGui version of WoW Talent Tree Manager (TTM), as a basis for deciding what
to keep, keep-for-interop, or replace with JSON in the web rewrite.

Scope: only *stored/shared* formats (files, clipboard/pastebin strings, hashes). In-memory
structures (`TalentTree`, `Talent`, etc. in `Engine/src/TalentTrees.h`) are referenced only
to explain field meaning.

---

## 1. TTM tree string

This is the master format: it round-trips through `createTreeStringRepresentation()` /
`parseTree()` / `parseCustomTree()`, and is what gets written to per-tree files, into
`workspace.txt`, and into the pastebin/manual-paste export box.

### 1.1 Top-level grammar

```
TreeString      ::= MetaInfo ";" TalentRecord* SkillsetRecord*
MetaInfo        ::= Version ":" PresetOrCustom ":" TreeType ":" Name ":" TreeDesc ":" LoadoutDesc ":" NumTalents ":" NumSkillsets
TalentRecord     (only present when PresetOrCustom == "custom")
SkillsetRecord   (0..NumSkillsets of them, always present regardless of preset/custom)
```

Records are `;`-terminated; fields inside a record are `:`-delimited; multi-value fields use
`,` as a sub-delimiter. Free-text fields are passed through `cleanString()` /
`restoreString()` (`Engine/src/TalentTrees.cpp:349-366`), which escapes the four structural
characters as tokens: `:` → `__cl__`, `\n` → `__n__`, `,` → `__cm__`, `;` → `__sc__`. This is
a hand-rolled escaping scheme with no escaping of the escape sequences themselves (i.e. the
literal substring `__cl__` typed by a user in a tree description would be silently
"unescaped" back into `:` on load — a real, if obscure, data-corruption bug).

Source: `createTreeStringRepresentation` (`Engine/src/TalentTrees.cpp:371-415`),
`parseCustomTree` (`:845-1002`), `parseTreeFromPreset` (`:796-843`).

### 1.2 MetaInfo fields (8 colon-separated fields, `TalentTrees.cpp:373`)

| # | Field | Type | Notes |
|---|-------|------|-------|
| 0 | Version | string | e.g. `1.4.2`, from `Presets::TTM_VERSION` (`TTMEnginePresets.h:17`). Drives the repair/migration chain. |
| 1 | PresetName | string | `"custom"` for a user-authored tree, or a preset key like `druid_class_restoration` / `druid_restoration` that looks the record up in `presets.txt` (`Presets::LOAD_PRESETS()`). |
| 2 | TreeType | `"0"` \| `"1"` | 0 = CLASS tree, 1 = SPEC tree (`enum class TreeType`, `TalentTrees.h:24`). |
| 3 | Name | string | Validated against a fixed allow-list of chars (letters/digits/space/`():'-`) in `validateAndRepairTreeStringFormat` (`:565-569`). |
| 4 | TreeDescription | escaped string | Free text, `cleanString`-escaped. |
| 5 | LoadoutDescription | escaped string | Free text, `cleanString`-escaped. |
| 6 | NumTalents | int | Talent record count that follows (0 for non-custom trees, since talent data is looked up from the preset instead). |
| 7 | NumSkillsets | int | Skillset record count that follows. |

If `PresetName != "custom"`, no `TalentRecord`s are emitted/expected — the parser instead
loads the full talent tree from `presets.txt` by preset name and only overlays the
skillsets from this string (`parseTreeFromPreset`, `:796-843`). This means a saved "preset
tree" file barely contains any tree data at all — just metadata + the user's skillsets — and
loading it silently uses *whatever the current `presets.txt` says*, so a preset update after
the fact changes what a previously "saved" tree resolves to.

### 1.3 TalentRecord (12 or 13 colon-separated fields, custom trees only)

```
TalentRecord ::= Index ":" Name["," SwitchName] ":" Desc["," Desc]* ":" Type ":" Row ":" Col ":"
                 MaxPoints ":" PointsRequired ":" PreFilled ":" Parents ":" Children ":"
                 IconName "," IconNameSwitch [ ":" NodeID ]
```

| # | Field | Type | Notes |
|---|-------|------|-------|
| 0 | Index | int | **Internal, positional talent ID** — not a Blizzard identifier. Assigned by `tree.maxID` counter on creation (`createTalent`, `:496-507`) or, for presets, by the generator script's row/column sort order (see §3). Used as the join key for `parents`/`children`/skillset ordering *within this string only*. |
| 1 | Name[, SwitchName] | string(s) | `SwitchName` only present for `TalentType::SWITCH` (choice) nodes. |
| 2 | Descriptions | comma list | One entry per rank (or 2 entries for a SWITCH). |
| 3 | Type | `0`\|`1`\|`2` | ACTIVE / PASSIVE / SWITCH (`TalentType`, `TalentTrees.h:18-22`). |
| 4,5 | Row, Col | int | Grid layout position — presentation data smuggled into the persisted format. |
| 6 | MaxPoints | 1 digit | Validated to be a single char (`:754`), i.e. **hard cap of 9 ranks**. |
| 7 | PointsRequired | int | Points-in-tree gate for unlocking the node. |
| 8 | PreFilled | `0`\|`1` | "Free" node auto-granted (Blizzard's `freeNode`). |
| 9 | Parents | comma list of Index | References field 0 of other records **in this same string**. |
| 10 | Children | comma list of Index | Same. |
| 11 | IconName,IconNameSwitch | string pair | PNG filenames resolved against the local icon pack (see §7 / icons_packed). |
| 12 | NodeID | int, *optional* | The real Blizzard node ID. **Only present in `presets.txt`-authored records** — `createTreeStringRepresentation` (the function the live app uses to save/export a tree) never writes this field (`TalentTrees.cpp:371-415` has no 13th field). Consequence: as soon as a user loads a preset into a custom tree and re-saves/exports it, `nodeID` reverts to its struct default (`-1`, `TalentTrees.h:31`) and is lost. |

Talent identity is therefore **doubly positional**: node cross-references inside a tree
string use the small resortable `Index`, and the *only* stable Blizzard-facing ID (`NodeID`)
is not persisted at all by the C++ app once a tree round-trips through it.

### 1.4 SkillsetRecord (embedded form, inside a tree string)

```
SkillsetRecord ::= Name "," LevelCap "," UseLevelCap (":" Points)*
```

`Points` is repeated once per talent, **in the iteration order of `tree.orderedTalents`**
(a `std::map<int, Talent_s>`, i.e. ascending numeric `Index` order) — not tagged with
which talent it belongs to (see §2 for why this is the central fragility of the whole
format). Source: `createTreeStringRepresentation:404-413`, parsed at
`parseCustomTree:962-999` / `parseTreeFromPreset:811-840`.

### 1.5 Versioning / repair chain

`Presets::TTM_VERSION` is currently `"1.4.2"` (`TTMEnginePresets.h:17`), but the *string
format* itself has only been bumped twice, tracked by `repairTreeStringFormat`
(`TalentTrees.cpp:613-644`):

- `repairToV120` (`:679-730`): fixes the pre-1.2.0 format, which had **no version field at
  all** (7 meta fields instead of 8) and no icon-name field on talents (11 fields instead
  of 12). Detected purely by field *count* — there's no magic byte, so this is a heuristic
  (`if (metaInfo.size() == 7)`), and it can silently accept/mis-migrate a corrupt string
  that happens to have 7 top-level fields.
- `repairToV121` (`:648-672`): pure version-string bump; the 1.2.0 format itself didn't
  change.
- The chain is a hand-written cascade of `if (repairVersionStart || metaInfo[0] == "X")`
  checks; a comment at `:633-641` shows the intended pattern for adding `repairToV130`, but
  no further migrations exist even though the live version is 1.4.2 — i.e. **all format
  changes since 1.2.1 were additive/non-breaking by convention**, not enforced by the
  migration code. NodeID (added later, §1.3) is one such silent additive change: old and
  new strings are both accepted because the field is optional (`talentInfo.size() > 12`
  check, `:937`).

`validateAndRepairTreeStringFormat` (`:543-611`) does full structural + character-class
validation (field counts, numeric-only fields, name charset) before a string is trusted;
`repairTreeStringFormat` is applied first, then per-talent (`validateTalentStringFormat`,
`:732-770`) and per-skillset (`validateSkillsetStringFormat`, `:1152-1176`) format checks.

### 1.6 Annotated real example

From `Engine/resources/presets.txt:2` (Druid Restoration class tree), trimmed to the
meta line + first talent record:

```
1.3.8:druid_class_restoration:0:Druid class (Restoration):This is the preset for the Druid class tree as Restoration.__n__You can start editing the tree/loadout now.::49:0;
└ver─┘└──presetName───────────┘ │  └───────────name─────────────────┘ └───────────treeDescription (escaped \n)──────────────┘│└49 talents┘└0 skillsets┘
                                 └TreeType=0 (CLASS)                                                                          └loadoutDescription (empty)

0:Rake:Balance,  Guardian,  Restoration\nRake the target...:0:1:3:1:0:0::4,9:rakeDRes.png,default.png:82199;
└i┘ └name┘└─────────────descriptions (comma+escaped list)───────────┘│ │ │ │ │ │ │  └children┘└───icon names────┘  └NodeID┘
                                                                      │ │ │ │ │ │ └parents (none, it's a root)
                                                                      │ │ │ │ │ └preFilled=0
                                                                      │ │ │ │ └pointsRequired=0
                                                                      │ │ │ └maxPoints=1
                                                                      │ │ └col=3
                                                                      │ └row=1
                                                                      └type=0 (ACTIVE)
```

(NodeID `82199` here — this is a **preset** record, generated by the Python pipeline, §3;
it would not survive a save/export round-trip through the live C++ app, per §1.3.)

---

## 2. TTM skillset string (standalone export)

`createSkillsetStringRepresentation` (`TalentTrees.cpp:2041-2048`) / `importSkillsets`
(`:1992-2039`) — used by the Loadout Editor's "export/import skillset(s)" text boxes,
independent of a full tree export.

```
SkillsetString  ::= (Skillset ";")*
Skillset        ::= Name "," LevelCap "," UseLevelCap (":" Points)*
```

Example: `Raid Single Target,70,1:0:3:1:0:2:0:...` — one `Points` entry per talent.

**Ordering is positional, not tagged.** `Points` values are written by iterating
`skillset->assignedSkillPoints` (a `std::map<int,int>` keyed by the internal talent
`Index`, so ascending-Index order) and are read back the same way, by zipping the `i`-th
number in the string to the `i`-th entry of `tree.orderedTalents` **for whatever tree is
currently loaded** (`importSkillsets:2016-2024`, `parseCustomTree:977-994`). There is:

- **No talent name, no Index, and no NodeID in the string at all** — points are
  positional-only.
- Only a coarse safety net: `validateSkillsetStringFormat` requires the field count to
  equal `tree.orderedTalents.size()` (`:1152-1176`) — i.e. it can only detect a *count*
  mismatch, not a *semantic* one.

**Fragility this implies:** a skillset string exported from one version/variant of a tree
(e.g. before a talent was added/removed/reordered, or against a slightly different preset
revision) and imported into another tree with the *same talent count* will silently
misassign points to the wrong talents — no error, no warning, just a wrong build. This is
the single most dangerous format in the app and the strongest argument for switching to a
tagged (nodeID- or name-keyed) representation in the rewrite.

The Simc export variant (`createSkillsetSimcStringRepresentation`, `:2050-2069`, see §6)
is by contrast tagged by *name*, which is more robust but still not immune to renames.

---

## 3. `Engine/resources/presets.txt`

79 lines, one record per line, each using the exact `TreeString` grammar from §1 (meta
info + talent records, always `NumSkillsets = 0`). Generated by
`Engine/resources/cicd_presets/tree_presets_generator.py`, copied verbatim into the shipped
resource and into `GUI/resources/updatertarget/presets.txt` for the in-app updater
(`preset_processor.py`).

- Line 1 is always the built-in `custom` template tree (welcome text), keyed by preset
  name `custom` (`tree_presets_generator.py:558`, `combine_tree_strings`).
- Every other line is keyed by preset name `"{class}_{spec}"` (spec tree) or
  `"{class}_class_{spec}"` (class tree) — see `preambel()` (`tree_presets_generator.py:539-550`).
  **Class-tree presets are duplicated once per spec** (e.g. `warrior_class_arms`,
  `warrior_class_fury`, `warrior_class_protection` are three separate, nearly-identical
  copies of the Warrior class tree) because Blizzard's shared class-tree tooltips vary
  per-spec (e.g. "Rake: Balance / Guardian / Restoration" text block). This is why the file
  has 79 lines: 1 custom + 39 spec presets + 39 class-tree presets (13 classes × ~3 specs
  each, matching current retail spec count).
- `LOAD_PRESETS()` (`Engine/src/TTMEnginePresets.cpp:333-362`) parses the file at runtime
  by reading each line and extracting the substring between the 1st and 2nd `:` as the map
  key — i.e. **the file itself has no header/index; it's a flat list keyed by convention**,
  and a duplicate preset name silently overwrites the earlier one in the map.
- Talent `Index` values inside a preset are assigned by the generator's
  `clean_tree()` step (`tree_presets_generator.py:345-371`): all of a spec's raidbots
  talent nodes are **sorted by `(row, column)`** and re-numbered `0..N-1`; this is a
  *different* number from Blizzard's `nodeID`, and is **not guaranteed stable across
  regenerations** if row/column values ever tie or change between game patches (sort is
  stable, so ties resolve by original raidbots array order, which is itself not a documented
  contract).
- The real Blizzard `nodeID` is preserved as the trailing 13th field per talent
  (`tree_presets_generator.py:401`, `str(talent.node_id)`) — this is the *only* place in the
  whole persisted-format stack where the authoritative Blizzard identifier survives.

---

## 4. `Engine/resources/node_id_orders.txt`

39 lines (one per spec — not per class), grammar:

```
Line ::= PresetKey ":" ClassID ":" SpecID ":" NodeID ("," NodeID)*
```

Example (`Engine/resources/node_id_orders.txt:1`):
```
druid_restoration:11:105:82043,82045,82046,...
```
`PresetKey` = `"{class}_{spec}"` (note: **no** `_class` infix, even for the class-tree half
of the data — see below). `ClassID`/`SpecID` are Blizzard's numeric class/spec IDs.
The comma list is `spec_dict["fullNodeOrder"]` straight from raidbots' `talents.json`
(`tree_presets_generator.py:149-153`) — i.e. **Blizzard's own canonical node ordering for
that spec's talent UI**, covering *both* the class tree and the spec tree nodes for that
spec in one combined sequence.

Purpose: this is the missing link between TTM's internal, resortable `Index` and
Blizzard's `nodeID`-and-bit-position scheme used by the official in-game export string
(§5). `LOAD_RAW_NODE_ID_ORDER()` (`TTMEnginePresets.cpp:411-446`) strips any `_class`
infix from the requested preset name before lookup, because one `node_id_orders.txt` line
serves both the class-tree and spec-tree preset variants of a given spec
(`exportBlizzardHash:2196-2202`, `importBlizzardHash:2402-2408`) — the bit sequence is
walked once, and TTM matches nodes to *either* the class-tree or spec-tree `TalentTree` via
a shared `nodeID → Talent` map built from both trees (`exportBlizzardHash:2222-2237`).

---

## 5. Blizzard talent loadout hash (import/export string)

This is TTM's interop format with the actual game client / Raidbots / other community
tools, implemented in `exportBlizzardHash` (`TalentTrees.cpp:2185-2305`) and
`importBlizzardHash` (`:2370-2523`), explicitly ported from
[SimC's `player.cpp`](https://github.com/simulationcraft/simc), which itself mirrors
Blizzard's `Blizzard_ClassTalentImportExport.lua`.

### 5.1 Alphabet and bit-packing primitives

- Alphabet: `base64_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"`
  (`:2173`) — standard base64 alphabet, but **packed 6 bits per character with no padding
  `=` and a custom bit-order** (not standard base64 encoding of a byte buffer).
- `byte_size = 6` (`:2182`) — each output character carries exactly 6 bits.
- Bits are packed **LSB-first within each 6-bit group**, and groups are emitted in
  ascending field order (`put_bit` lambda, `:2244-2256`; mirrored by `get_bit`,
  `:2307-2320`).

### 5.2 Header (fixed, in order)

| Field | Bits | Meaning |
|---|---|---|
| `version` | 8 | `LOADOUT_SERIALIZATION_VERSION = 1` (`:2175`), hardcoded from Blizzard's Lua. Import rejects anything else (`:2396-2399`) — **this is the version-pin that will break the moment Blizzard ships format v2.** |
| `spec_id` | 16 | Blizzard specialization ID (matched against `node_id_orders.txt`'s `SpecID` column). |
| `tree_hash` | 128 | `C_Traits.GetTreeHash()` — a content hash of the *current* talent tree definition on Blizzard's side, meant to reject import strings from a stale tree version. TTM **always writes zero bits here on export** (`put_bit(tree_bits, 0)`, `:2260`, comment: "0-filled to bypass validation, as GetTreeHash() is unavailable externally") and simply **reads-and-discards** it on import (`:2418`, comment: "we can ignore it, as invalid/outdated strings can error in later checks"). This means TTM-exported strings are technically malformed per Blizzard's own spec (an in-game import of a TTM-exported string may be rejected client-side), and TTM cannot itself detect a stale-tree-version mismatch on strings from elsewhere. |

Total header = 152 bits before any per-node data.

### 5.3 Per-node encoding (repeated for every `nodeID` in that spec's `node_id_orders.txt` order)

```
selected        : 1 bit   — 0 = node not selected, stop (no further bits for this node)
  partial?      : 1 bit   — 1 = node partially ranked, read `rank` next; 0 = fully ranked (use maxPoints)
    rank        : 6 bits  — present only if partial
  choice?       : 1 bit   — 1 = this is a choice(SWITCH) node, read `choice` next
    choice_idx  : 2 bits  — 0-based choice index, present only if choice
```

- If a node is absent from TTM's `nodeIDTalentMap` (i.e. TTM doesn't know this nodeID —
  stale/unsupported preset), export writes a single `0` bit (treated as "not selected",
  `:2263-2266`) — **silently dropping** any talent TTM doesn't recognize rather than
  erroring.
- On import, a SWITCH/choice node's final stored "points" value is `rank + choice`
  (`:2459-2464`) where `rank` defaults to the node's own `maxPoints` if not partially
  ranked — this reconstructs TTM's own `points ∈ {0,1,2}` scheme for a 2-way choice
  (mirrors the `talentSwitch` encoding used in `addTalentAndChildrenToMap`, §1.4/`Talent`
  struct field `talentSwitch`).
- Import pads the input hash with 12 filler `A` characters up front
  (`paddedHash = hash_string + "AAAAAAAAAAAA"`, `:2378`) as a defensive measure against
  short/truncated input — a workaround rather than a length-based validation.
- After the header, `spec_id` is cross-checked against `node_id_orders.txt`
  (`:2410-2413`) and the hash is rejected if it doesn't match the *currently loaded* tree's
  spec — so a hash can only be imported into the matching spec's tree, never
  auto-detected-and-loaded generically (`getClassSpecPresetsFromBlizzHash`,
  `:2322-2340`, is the one place that *does* resolve spec-id → preset name generically,
  used by higher-level UI code to pick the right tree before calling `importBlizzardHash`).
- `verifyTreeIDWithBlizzHash` (`:2342-2368`) only checks the decoded `spec_id` maps back to
  the *same preset name family* as the currently-open tree — again, no actual tree-hash
  verification, since `tree_hash` is discarded.

### 5.4 Version pinning / patch-break risks (explicit call-out)

- `LOADOUT_SERIALIZATION_VERSION`, `rank_bits` (6), `choice_bits` (2), `tree_bits` (128),
  `spec_bits` (16) are all compile-time constants copied from one point-in-time snapshot of
  Blizzard's Lua source (comment at `:2169-2170`). If Blizzard changes any bit width (e.g.
  to support >63 ranks, or >4-way choice nodes, or bumps the serialization version), this
  code silently produces/reads garbage until manually updated and recompiled.
- The per-spec node order (`node_id_orders.txt`) is scraped from Raidbots at preset-build
  time (§3/§4) and is **not** re-derived at runtime from the hash itself — if Blizzard
  reorders/renumbers nodes for a spec, TTM's node order goes stale until the CI preset
  pipeline is manually re-run and shipped.

---

## 6. SimC talent strings / profilesets

Several related exports, all built on `createSkillsetSimcStringRepresentation`
(`TalentTrees.cpp:2050-2069`):

```
SimcTalentString ::= ("class_talents=" | "spec_talents=") Entry ("/" Entry)*
Entry            ::= TokenName ":" Rank
```

- One `class_talents=`/`spec_talents=` line per skillset (chosen by `tree.type`).
- Only non-zero talents are emitted (`:2053-2055`); PASSIVE/ACTIVE use their rank as
  `Rank`; SWITCH nodes always emit `Rank=1`, using the base name if points==1 or the
  **switch name** if points==2 (`:2059-2066`) — i.e. the *name itself* encodes which side
  of a choice was taken, unlike the TTM string format's numeric encoding.
- `createSingleTalentsSimcString` / `createSingleTalentComparisonSimcString`
  (`:2089-2140`) generate full **SimC `profileset."<label>"+="..."` blocks** for
  "vary one talent at a time" sim batches — a text-generation feature, not really a
  "stored" format but worth noting as an emission target if the web rewrite still wants to
  hand off to SimulationCraft.

### `simcTokenizeName` slug rules (`:2756-2802`)

Given a talent/rank display name, produces a SimC-safe token:
1. Strip any leading run of `_`/`+`.
2. Drop any byte ≥ `0x80` (non-ASCII) entirely (not transliterated — just removed).
3. Lowercase ASCII letters.
4. Replace space with `_`.
5. Drop any character that isn't a letter, digit, `_`, `+`, `.`, or `%`.

This is a lossy, ASCII-only slugifier with no collision handling — two differently-named
talents that reduce to the same slug (e.g. differing only by punctuation or accents) would
collide in SimC output with no detection.

---

## 7. Local user file storage

All paths root at `%APPDATA%\WoWTalentTreeManager\` (`Presets::getAppPath()`,
`Engine/src/TTMEnginePresets.cpp:298-331`, via `SHGetKnownFolderPath(FOLDERID_RoamingAppData)`
— Windows-only, no XDG/macOS equivalent, a portability issue for a rewrite target anyway).

| Path | Format | Written by |
|---|---|---|
| `resources/presets.txt`, `resources/node_id_orders.txt`, `resources/resource_versions.txt`, `resources/icons/*` | Copies of the shipped resources; overwritten by the in-app **updater** (`GUI/src/Updater.cpp`) when a newer version is published. | `initWorkspace()` (`TalentTreeManager.cpp:1024-1040`), `Updater.cpp:210-322`. |
| `CustomTrees\<sanitized-name>.txt` | **One `TreeString` (§1) per file, single line.** Filename = tree name with all non-alphanumeric characters stripped (`treeNameToFileName`, `TalentTreeEditorWindow.cpp:1742-1745`) + `.txt`. No collision handling beyond overwrite-by-same-sanitized-name; two differently-named trees that sanitize to the same string clobber each other. | `saveTreeToFile` / `loadTreeFromFile` / `deleteTreeFromFile` (`TalentTreeEditorWindow.cpp:1677-1740`). |
| `workspace.txt` | Newline-separated: one `TreeString` per currently-open tree in the workspace, followed by `ACTIVETREE=<index>` and `ACTIVESKILLSET=<index>` marker lines. This is separate from the `CustomTrees\` folder — it's session/workspace state, not the "saved tree library". | `saveWorkspace` / `loadWorkspace` (`TalentTreeManager.cpp:1042-1250`). |
| `settings.txt` | Ad-hoc `KEY=value` lines (`STYLE=`, `GLOW=0/1`, `FONTSIZE=`, `WINDOWPLACEMENT=l,t,r,b,showCmd`, `DIVIDERRATIO=<float>`). Comment at `:1063` notes settings values must avoid `:` because the workspace-file fallback parser (`:1183-1220`, kept for backward compatibility with old saves that stored settings inside `workspace.txt`) is a substring `.find()`-based parser with no real tokenization. | `saveWorkspace`/`loadWorkspace` (`TalentTreeManager.cpp:1042-1250`). |
| `settings_backup.txt`, `workscape_backup.txt` (sic) | Byte-for-byte backup of the previous `settings.txt`/`workspace.txt`, made before each save (`:1044-1058`). Simple poor-man's undo/corruption recovery, not versioned history. | `saveWorkspace`. |

"Reset" (`resetWorkspaceAndTrees`, `TalentTreeManager.cpp:1252-1255`) is `remove_all()` on
the entire AppData folder — destructive and total, no export/confirm safety net beyond the
UI prompt (not shown here).

### Clipboard / sharing mechanism

There is **no OS clipboard integration for text** import/export (aside from a
"screenshot to clipboard" bitmap feature using raw Win32 `OpenClipboard`/`SetClipboardData`,
`TalentTreeEditorWindow.cpp:2144-2216`). Tree/skillset "export" and "import" are plain
read-only/editable ImGui `InputText` boxes the user manually selects and copies
(`ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll`,
`TalentTreeEditorWindow.cpp:1174`); the user's own OS clipboard shortcuts (Ctrl+C/V) do the
rest. The one built-in "share" integration is **Pastebin**: `exportToPastebin`
(`GUI/src/Updater.cpp:389-…`) POSTs the raw `TreeString` to
`https://pastebin.com/api/api_post.php` using a bundled `PASTEBIN_API_DEV_KEY`, with a
client-side cooldown (`Presets::PASTEBIN_EXPORT_COOLDOWN`,
`TalentTreeEditorWindow.cpp:1180-1204`) and gets back a plain pastebin URL — the pasted
content is the exact same `TreeString` as the manual-copy path, just hosted. There is no
corresponding "import from URL" — a user must open the pastebin link and paste the text
back manually.

---

## 8. Sim result import (Raidbots / SimC output)

`ImportSimResult` (`TalentTrees.cpp:2553-2614`) parses **SimulationCraft's plain-text
human-readable report** (the `output.txt`/console-style text report, not the JSON report),
either from a local file/folder (`ReadSimFile`, `SimAnalysisWindow.cpp:1177-1186`, one
`SimResult` per `.txt` file in a chosen directory) or — nominally — from a Raidbots report
URL (`ReadRaidbots`, `SimAnalysisWindow.cpp:1188-…`, fetches
`https://www.raidbots.com/reports/<hash>/output.txt` via libcurl), though the live UI path
for that is currently disabled with a "not yet supported" placeholder message
(`SimAnalysisWindow.cpp:1167-1170`) — only the file/folder path is actually reachable.

Parsing is pure **line-prefix / substring scraping** of SimC's text report, with no
structure beyond that:

- A baseline run is detected by a line starting with literal `"Player:"` (`:2562`); the DPS
  value is read from the *next* line by locating the substrings `"DPS="` and `"DPS-Error"`
  and slicing between them (`:2580-2582`).
- Profileset results are detected by the literal header line
  `"Profilesets (median Damage per Second):"` (`:2586`), after which each subsequent line
  is split on `:` into `dps : name` pairs until a line starting with `"Baseline
  Performance:"` or a blank line is hit (`:2591-2600`).
- Extracted `(name, dps)` pairs are then matched back to the tree's `loadout` **by exact
  skillset name string** (`:2605-2611`) — so a sim run's profileset name must exactly equal
  a skillset's `name` field, tying two independently-edited strings together only by
  incidental equality.

This is inherently brittle: it depends on SimC's exact human-readable text layout
(column spacing/wording is not a stable contract the way SimC's JSON output is), and the
whole matching pipeline hinges on profileset names not being renamed/duplicated between
export and import.

---

## 9. Recommendations for the web rewrite

### 9.1 Keep / replace matrix

| Format | Recommendation | Why |
|---|---|---|
| TTM tree string (§1) | **Replace with JSON** | Positional talent `Index`, hand-rolled escaping, ad-hoc versioning by field-count sniffing. No reason to keep once nothing needs to read old save files natively — but see migration note below. |
| TTM skillset string (§2) | **Replace with JSON** | Purely positional, untagged — the single most dangerous format in the app (§2). Must not survive into a system where builds get shared across even slightly different tree revisions. |
| `presets.txt` (§3) | **Replace with JSON**, keep the *pipeline* | The CI scraper (`tree_presets_generator.py`) pulling from Raidbots is genuinely useful and should be kept/ported; its *output* format should become JSON keyed by Blizzard `nodeID`, not resorted positional index. |
| `node_id_orders.txt` (§4) | **Replace with JSON, but keep the concept** | Still needed as the class/spec → `fullNodeOrder` lookup table for Blizzard-hash import/export; just express it as JSON (`{presetKey, classId, specId, nodeOrder: [...]}`). |
| Blizzard talent loadout hash (§5) | **Keep as-is, interop only** | This is the one format defined by an external party (Blizzard/SimC). Port the bit-packing logic faithfully (ideally unit-tested against known-good in-game strings), keep it version-pinned exactly like today, but *isolate* it behind an explicit "import/export to game string" boundary rather than using it as internal storage. Internal storage should always be nodeID-keyed JSON that can be losslessly projected into this hash on demand. |
| SimC talent strings / profilesets (§6) | **Keep for interop only** | Needed if the rewrite still wants to hand data to SimulationCraft. Regenerate on demand from JSON; don't store it. Consider using SimC's JSON report/import instead of text scraping (§9.4). |
| Local file storage (§7) | **Replace with JSON files** (or a small embedded DB/IndexedDB for a web app) | The `%APPDATA%` flat-file scheme is Windows-only and not applicable to a web app anyway; this is a forced rewrite, use it to fix the format too. |
| Sim result import (§8) | **Replace with SimC JSON report parsing** | SimC can emit a structured `json2=<file>` report; parsing that instead of the human-readable text report removes the entire brittle substring-scraping layer (§9.4). |

### 9.2 The identifier-stability problem (central issue)

Three different notions of "which talent is this" exist today and are conflated:

1. **Blizzard `nodeID`** — the only identifier that's actually stable across patches and
   meaningful outside TTM (used by the game client, SimC, Raidbots, wowhead). Currently
   *not* persisted by the live app once a tree is edited/saved (§1.3) — a serious gap.
2. **TTM internal `Index`** — a resortable `0..N-1` position assigned by row/column sort
   order at preset-generation time, or by an incrementing counter for user-authored nodes.
   Used as the join key for parent/child edges *and* as the implicit key for skillset point
   arrays. It has no meaning outside a single tree-string instance and is not guaranteed
   stable even across two generations of the *same* preset if raidbots' underlying node
   order changes.
3. **Positional array order** in the skillset string (§2) — not even a written index, just
   "the i-th number belongs to the i-th talent when iterated in Index order right now." This
   is strictly more fragile than (2) because it isn't even self-describing within its own
   string.

Order-dependent/positional indexing is dangerous for anything *stored or shared* because
correctness silently depends on an out-of-band invariant (both ends agree on exactly the
same tree definition and iteration order) that the format itself cannot verify — a
count-only check (as today's `validateSkillsetStringFormat` does) catches only gross
mismatches, never a same-length reordering, addition+removal pair, or a talent-swap. Any
tree edit, preset regeneration, or version skew silently corrupts every previously-exported
skillset that happens to still match on count. **The fix is to always key stored talent
references by a stable ID** — ideally Blizzard `nodeID` when available (spec/class
presets), falling back to a durable, never-reused internal UUID/ID for custom, non-Blizzard
trees (still never reusing a numeric position).

### 9.3 Proposed JSON schemas

**(a) Talent tree definition**

```jsonc
{
  "schemaVersion": 1,
  "id": "druid_restoration",            // stable slug; nodeID-bearing presets keyed by class_spec
  "presetOrigin": { "source": "raidbots", "fetchedAt": "2026-09-01", "classId": 11, "specId": 105 },
  "kind": "class" | "spec",
  "name": "Restoration Druid",
  "description": "...",
  "talents": [
    {
      "nodeId": 82199,                  // Blizzard node id — PRIMARY key, always present when known
      "localId": "t-0f3a...",           // stable internal id (uuid) — used when nodeId is absent (custom trees)
      "name": "Rake",
      "switchName": null,               // set only for choice nodes
      "type": "active" | "passive" | "choice",
      "maxPoints": 1,
      "pointsRequired": 0,
      "preFilled": false,
      "row": 1,
      "column": 3,
      "descriptions": ["..."],          // one per rank, or [textA, textB] for choice
      "icon": { "default": "rakeDRes.png", "switch": null },
      "parents": [82190, 82191],        // nodeId references (or localId for custom nodes)
      "children": [82205]
    }
  ]
}
```

**(b) Skillset / build** — always tagged, never positional:

```jsonc
{
  "schemaVersion": 1,
  "name": "Raid Single Target",
  "treeId": "druid_restoration",
  "levelCap": 70,
  "useLevelCap": true,
  "points": {
    "82199": 1,        // keyed by nodeId (string key; JSON object keys are always strings)
    "82217": 2,
    "82220": 0
  },
  "choices": {
    "82220": "switch"  // for choice nodes: "base" | "switch" (or 0/1), explicit rather than encoded in the point count
  }
}
```

**(c) Loadout** (a tree + its collection of builds — what today's single `TreeString`
conflates):

```jsonc
{
  "schemaVersion": 1,
  "treeId": "druid_restoration",
  "treeRevision": "2026-09-01T00:00:00Z",   // pins which tree-definition snapshot this loadout was built against
  "description": "...",
  "skillsets": ["<skillset id or inline object, per (b)>"],
  "activeSkillsetId": "raid-single-target"
}
```

Keeping an explicit `treeRevision`/version pin on the loadout (rather than resolving the
"current" preset at load time as today's `parseTreeFromPreset` does, §1.2) fixes the
"preset changed underneath a saved tree" problem for free.

### 9.4 Shareable URL code

For a short, pastable/URL-friendly code (replacing the raw `TreeString`/skillset-string
paste boxes and the Pastebin integration):

- Don't reinvent bit-packing for the general tree/loadout case — that's what made the TTM
  string fragile. Use **JSON → deflate/gzip → base64url**, which is simple, self-describing
  (still versioned by an explicit `schemaVersion` field before compression), and trivially
  future-proof (add fields without breaking old decoders, same principle .zip-shareable
  configs use elsewhere).
- Reserve genuine bit-packing (à la §5) *only* for the Blizzard-interop hash itself, since
  that format's bit widths are dictated by an external, versioned contract you must match
  exactly — not something to design further formats around.
- A practical shape: `ttm1.<base64url(gzip(JSON loadout))>` — the `ttm1` prefix disambiguates
  from a raw Blizzard export string (which always starts with base64 alphabet chars and has
  no natural prefix), lets the importer dispatch instantly, and gives you a format version
  to bump later without breaking detection of old links. Keep accepting bare Blizzard hash
  strings *as an alternate paste target* (auto-detect by trying the fixed-width hash decode
  first, since it has almost no false-positive surface against `ttm1...`), so "paste your
  in-game export string" keeps working unchanged.
- Keep server-side link shortening (Pastebin equivalent) purely as *storage* of that same
  encoded payload, not a second competing format — resolve `/share/<slug>` to the stored
  blob and feed it through the identical decoder used for pasted codes.
