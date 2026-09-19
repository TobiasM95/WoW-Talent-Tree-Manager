# Existing Talent-Data Acquisition Pipeline

This document is a full account of how WoW Talent Tree Manager (TTM) has historically obtained
talent-tree data, how that data flowed from a scraper into the shipped app and the web app, and
exactly why the pipeline broke with WoW: The War Within (TWW). It is a description of the
**status quo**, not a proposal.

All line numbers refer to the state of the repository at commit `b82a9f9` (branch `master`).

---

## 1. Data sources

| # | URL / host | What it provides | Kind | Cited at |
|---|---|---|---|---|
| 1 | `https://www.raidbots.com/static/data/live/talents.json` | The entire talent tree graph for every class/spec: node ids, node positions (`posX`/`posY`), node type, max ranks, required points, parent/child (`prev`/`next`) links, per-rank spell ids and definition ids, icon file names, `classId`/`specId`, and `fullNodeOrder`. This is Raidbots' own processed mirror of Blizzard's `TraitTree`/`TraitNode` game-data tables (they run a similar extraction pipeline against the game client / Blizzard's trait API). | Community API (unofficial, but stable and widely relied upon by the WoW tools ecosystem) | `Engine/resources/cicd_presets/tree_presets_generator.py:130-132` |
| 2 | `https://nether.wowhead.com/tooltip/spell/{spellId}?def={definitionId}&rank={rank}&dataEnv=1` | Rendered tooltip HTML (as JSON with a `tooltip` field) for a given spell id/rank, used to scrape the human-readable talent description text. | HTML/JSON scraping of an internal Wowhead tooltip-rendering endpoint (not a documented public API) | `Engine/resources/cicd_presets/tree_presets_generator.py:292`, `:302` |
| 3 | `https://wow.zamimg.com/images/wow/icons/large/{icon}.jpg` | Full-resolution talent icon images (keyed by the `icon` field from Raidbots' entry data, which is itself a Blizzard icon file name). | Static asset CDN (Wowhead's image mirror, owned by the same company as Wowhead/Zam) | `Engine/resources/cicd_presets/tree_presets_generator.py:218` |
| 4 | `https://www.wowhead.com/spell={spellId}` | Human-facing tooltip URL, stored per-talent as `tooltip_urls` but not otherwise fetched/parsed by this script (informational/clickable link only). | Wowhead website (link only) | `Engine/resources/cicd_presets/tree_presets_generator.py:208-211` |
| 5 | `https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/resource_versions.txt` | Version manifest the running desktop app polls to detect whether presets/icons/node-id-orders are stale. | GitHub raw content (self-hosted data, not talent data) | `GUI/src/Updater.cpp:68`, `:196`; `AppUpdater/src/Updater.cpp:242` |
| 6 | `https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/presets.txt` | The actual TTM preset tree strings, downloaded at runtime by the installed app. | GitHub raw content | `GUI/src/Updater.cpp:241` |
| 7 | `https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/node_id_orders.txt` | Node ordering / class-spec-id table, downloaded at runtime. | GitHub raw content | `GUI/src/Updater.cpp:269` |
| 8 | `https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/icons_packed_meta.txt` and `.../icons_packed.png` | The packed icon atlas + its index, downloaded at runtime. | GitHub raw content | `GUI/src/Updater.cpp:307`, `:337` |
| 9 | `https://github.com/TobiasM95/WoW-Talent-Tree-Manager/releases/download/v{version}/TalentTreeManager.zip` | Full application zip, downloaded by the separate `AppUpdater.exe` binary to self-update the whole program (not just data). | GitHub Releases | `AppUpdater/src/Updater.cpp:262-263` |
| 10 | `https://pastebin.com/api/api_post.php` | Used by the client to publish an exported tree/loadout string as a Pastebin paste (for sharing), using a hardcoded `api_dev_key` plus an optional user-supplied `PASTEBIN_API_DEV_KEY`. | Community API (Pastebin) — outbound export, not talent-data ingestion | `GUI/src/Updater.cpp:390-412` |
| 11 | `https://www.warcraftlogs.com/oauth/token` and `https://www.warcraftlogs.com/api/v2/client` | Used only by `Web/server/data_management/create_popular_builds.py` (not in scope per the task, listed here for completeness) to build "popular build" presets from real raid logs, combined with a second call to the same Raidbots `talents.json` endpoint (line 201 of that file). | Official Blizzard-endorsed community API (WarcraftLogs GraphQL v2) | `Web/server/data_management/create_popular_builds.py:24,173,201` |

No official Blizzard Game Data API (`us.api.blizzard.com`) call exists anywhere in the pipeline. Everything talent-shaped is sourced from Raidbots' pre-digested JSON plus Wowhead's tooltip/icon infrastructure — i.e. the entire pipeline depends on two unofficial third parties staying in sync with Blizzard's own (frequently changed) internal talent data format.

---

## 2. Pipeline stages (end to end)

```
                         ┌────────────────────────────────────────────┐
                         │   Daily GitHub Actions cron (06:00 UTC)     │
                         │   .github/workflows/update_presets.yml     │
                         └───────────────────┬──────────────────────-─┘
                                              │ pip install -r requirements.txt
                                              ▼
 ┌──────────────────────────────────────────────────────────────────────────┐
 │ tree_presets_generator.py  (Engine/resources/cicd_presets/)              │
 │                                                                          │
 │ 1. GET raidbots talents.json  → one dict per class+spec                 │
 │      (generate_tree_structures_from_raidbots, line 130)                 │
 │ 2. For each spec: extract_tree(classNodes) + extract_tree(specNodes)    │
 │      → Talent dataclass per node: id, names, type, row/col (from        │
 │        posX/posY via fixed grid), maxRanks, reqPoints, prev/next,       │
 │        spellIds, iconUrls, iconNames                     (line 160)    │
 │ 3. fill_in_descriptions(): async-fetch nether.wowhead.com tooltip JSON  │
 │      per (spellId, rank) pair, regex-strip HTML → plain description    │
 │                                                            (line 234)   │
 │ 4. clean_tree(): normalize row/col to start at 1, remap indices        │
 │                                                            (line 345)   │
 │ 5. create_tree_string(): serialize each tree into the TTM tree-string  │
 │      wire format (colon/comma/semicolon delimited)         (line 374)  │
 │ 6. combine_tree_strings() → write ./presets.txt                        │
 │    export_node_orders_to_file() → write ./node_id_orders.txt           │
 │                                                            (line 553+)  │
 │ 7. md5() old vs new presets.txt+node_id_orders.txt; only if changed:   │
 │      download all talent icons in parallel (ThreadPoolExecutor),       │
 │      resize to 40x40, pack into one array, write icons_packed.png +    │
 │      icons_packed_meta.txt                                (line 100+)  │
 └───────────────────────────────┬──────────────────────────────────────-─┘
                                  ▼
 ┌──────────────────────────────────────────────────────────────────────────┐
 │ preset_processor.py  (same folder)                                       │
 │  - md5-compares the 4 freshly generated files against the committed     │
 │    copies in ../ and ../../../GUI/resources/                            │
 │  - if different: copies presets.txt / node_id_orders.txt /              │
 │    icons_packed*.{png,txt} into GUI/resources/ AND                      │
 │    GUI/resources/updatertarget/ (the two locations the shipped client   │
 │    and the raw.githubusercontent.com update mechanism read from)        │
 │  - bumps resource_versions.txt (presets;icons;nodeidorders get a new    │
 │    "<same semver>;<UTC timestamp>" version stamp) and copies it to      │
 │    GUI/resources/ and GUI/resources/updatertarget/                      │
 │  - prints 1 (changed) or 0 (no-op) to stdout, captured by CI            │
 └───────────────────────────────┬──────────────────────────────────────-─┘
                                  ▼
      CI step "commit files" (if toupdate > 0): git add -A; git commit;
      push to master as the "GitHub Action" bot user
                                  │
                                  ▼
 ┌──────────────────────────────────────────────────────────────────────────┐
 │ Desktop client runtime (GUI/src/Updater.cpp, AppUpdater/src/Updater.cpp) │
 │  - On launch/menu action: GET .../updatertarget/resource_versions.txt   │
 │    and diff against local resources/resource_versions.txt               │
 │  - Per out-of-date resource type: GET the corresponding raw file from   │
 │    .../updatertarget/ and overwrite the local resources/ copy           │
 │  - Re-parses presets.txt (LOAD_PRESETS) and re-applies to open trees    │
 │  - AppUpdater.exe separately downloads a full TalentTreeManager.zip     │
 │    release and self-replaces the whole application via miniz           │
 └──────────────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────────────┐
 │ Web app ETL (Web/server/data_management/)                                │
 │  - preset_updater.py re-reads Engine/resources/cicd_presets/presets.txt │
 │    (the CI-generated file, NOT re-scraped) and converts the TTM tree-   │
 │    string format into SQL rows (PresetTree/Talent) via pypika           │
 │  - extract_icons_to_public.py unpacks GUI/resources/updatertarget/      │
 │    icons_packed.png (using icons_packed_meta.txt) into individual PNGs │
 │    under Web/frontend/public/preset_icons/ (and an optional             │
 │    /var/www/ttm/build/preset_icons server path)                         │
 │  → i.e. the web app is a second-order consumer that piggybacks          │
 │    entirely on the desktop pipeline's output files; it has no           │
 │    scraper of its own.                                                  │
 └──────────────────────────────────────────────────────────────────────────┘
```

---

## 3. The icon pipeline in detail

1. **Sourcing**: for every talent-entry rank, `extract_tree()` builds an icon URL
   `https://wow.zamimg.com/images/wow/icons/large/{icon}.jpg` from Raidbots' `entry["icon"]` field,
   and an icon *name* via `format_name(names) + ClassLetter + SpecLetters3` (`tree_presets_generator.py:213-220`,
   `format_name` at `:334`), e.g. `ironfurDRes` for Guardian Druid "Ironfur" (visible verbatim in
   `Engine/resources/cicd_presets/icons_packed_meta.txt:6`).
2. **Download & normalize**: `download_icon()` (`:456`) fetches each `.jpg`, retries by mutating the
   URL (stripping `_` segments and re-trying with `-`) if the exact icon slug 404s, resizes every
   icon to a fixed 40×40 with Lanczos resampling, and forces full alpha.
3. **Packing**: `pack_and_save_icons()` (`:475`) prepends a hardcoded local `default.png` from
   `GUI/resources/icons/default.png`, stacks every icon's raw RGBA pixels into a single
   `(N, 1600, 4)` numpy array, and writes it as one `icons_packed.png` — effectively a 1-pixel-tall,
   N-icons-wide "image strip" where each icon's 40×40 image has been flattened into 1600 contiguous
   pixels in a single row `i`.
4. **Indexing**: `icons_packed_meta.txt` is a flat text file: line 1 = tile width (40), line 2 = tile
   height (40), line 3 = total icon count, line 4 = flattened pixel count per tile (1600), then one
   filename per remaining line, in the same order as the rows of `icons_packed.png`
   (`Engine/resources/cicd_presets/icons_packed_meta.txt:1-5`, confirmed against
   `pack_and_save_icons`/`extract_icons_to_public.py:38-51` which does `im_arr[i].reshape((h, w, 4))`).
5. **Matching to talents (generation time)**: deterministic — the preset generator writes the exact
   icon filename it downloaded directly into the tree string (`tree_presets_generator.py:398-400`),
   so presets.txt and icons_packed_meta.txt are always self-consistent by construction.
6. **Matching to talents (edit time, fuzzy)**: separately, the C++ engine has an
   `autoInsertIconNames()` routine (`Engine/src/TalentTrees.cpp:1621-1706`) used when a user creates
   or renames a **custom** talent and the engine tries to auto-guess a matching icon from the
   currently loaded icon set. It lowercases/strips both the talent name and every candidate icon
   name to letters-only, then ranks candidates with `getSimilarityRanking()`
   (`Engine/src/TalentTrees.cpp:2671-2697`), a Sørensen–Dice bigram-overlap coefficient computed by
   `createWordLetterPairs()` (`:2699-2705`, adjacent-letter-pair extraction) and picks the
   highest-scoring icon name. This fuzzy step is **not** part of the scraper/CI pipeline — it only
   runs client-side against whatever icon set is already packed, and is a fallback for
   user-authored content, not for official preset generation.
7. **Shipping**: `preset_processor.py` copies `icons_packed.png`/`icons_packed_meta.txt` into
   `GUI/resources/` (bundled with the installer) and `GUI/resources/updatertarget/` (served raw from
   GitHub for the live-update path). The web app's `extract_icons_to_public.py` explodes the same
   atlas back into loose PNGs for the React frontend's `public/preset_icons/` directory — i.e. the
   web app un-packs an atlas format that only existed to save bandwidth in a native updater.

---

## 4. Version / update mechanism

- `resource_versions.txt` (`Engine/resources/cicd_presets/resource_versions.txt` and mirrored to
  `GUI/resources/` and `GUI/resources/updatertarget/`) has exactly 4 lines, order-significant and
  read positionally by index (`ResourceType` enum cast to `int`, `GUI/src/Updater.cpp:113,122-138`):
  ```
  ttm;1.4.2;2022-11-26
  presets;1.4.2;2024-07-05-06-20-08
  icons;1.4.2;2024-07-05-06-20-08
  nodeidorders;1.4.2;2024-07-05-06-20-08
  ```
  Each line is `name;semver;timestamp`. The semver segment is **not a data schema version** — it is
  always hardcoded to match the app's own release version at generation time
  (`preambel(..., version="1.3.8")` in `tree_presets_generator.py:539` and `combine_tree_strings(...,
  version="1.3.8")` at `:553` — note this literal is already stale/inconsistent with the `1.4.2` in
  `resource_versions.txt`, an existing drift bug). `preset_processor.py:update_resource_versions()`
  (`:68-90`) only refreshes the *timestamp*, copying whatever semver was already in the committed
  file — so the version number itself never organically advances; the timestamp is the only real
  "did anything change" signal.
- **Client-side check** (`GUI/src/Updater.cpp:checkForUpdate`, `:53-151`): downloads the remote
  `resource_versions.txt` via `raw.githubusercontent.com`, does a **line-by-line string
  compare by fixed positional index** against the local copy under `resources/resource_versions.txt`
  (`Presets::getAppPath() / "resources" / "resource_versions.txt"`), and flags any line index whose
  content differs as out-of-date. If the file is missing, unreadable, or has a different number of
  lines than expected, it conservatively flags **all** resources as out of date
  (`:87-90`, `:112-120`).
- It also compares `Presets::TTM_VERSION` (hardcoded `"1.4.2"` in `Engine/src/TTMEnginePresets.h:17`)
  against the remote `ttm` line via `compareVersions()` (`:153-166`), which parses versions as
  exactly 3 dot-separated integers (`for (int i = 0; i < 3; i++)`) — any non-numeric or
  differently-shaped version string throws (`std::stoi`) or silently misbehaves.
- **Update application** (`updateResources`, `:168-230`): re-downloads each out-of-date resource
  individually from the corresponding `updatertarget/*` raw GitHub URL, overwrites the local file,
  then re-downloads `resource_versions.txt` itself to persist the new baseline. There is no
  atomicity across the whole set — a crash/network failure mid-update can leave presets, icons and
  node-id-orders at different versions relative to each other, with no rollback.
- **Pastebin dependency**: unrelated to *ingesting* talent data — it is the *export* mechanism
  (`exportToPastebin`, `GUI/src/Updater.cpp:389-432`) used when a user shares a custom
  tree/loadout. It POSTs to `https://pastebin.com/api/api_post.php` using a **hardcoded dev key
  committed in source** (`std::string api_dev_key = "BCmGxHU-akWMjGYiXm5yL5An0aclmzOC";`, line 390)
  as well as an optional separate `Presets::PASTEBIN_API_DEV_KEY` sourced from a gitignored
  `TTMGUIPresetsInternal.h` header (`GUI/src/Updater.cpp:31-35`, documented in
  `README.md:53`). This is a live external dependency risk (rate limits/key revocation) but not part
  of the talent-data scraping path.
- The whole desktop app can also **self-update entirely** via `AppUpdater.exe`
  (`AppUpdater/src/Updater.cpp`), which re-reads the same `resource_versions.txt` purely to extract
  the semver segment (`versionString.substr(...)`, `:260-263`) and builds a GitHub Releases zip URL
  from it, then uses `libcurl` (dynamically loaded via `LoadLibrary`/`GetProcAddress`, not linked) to
  download and `miniz` to extract over the existing install.

---

## 5. Fragility analysis (itemized failure modes)

1. **No handling of Hero Talents / third tree per spec at all.** Raidbots' `talents.json` schema for
   TWW added two new top-level fields per spec, `heroNodes` (a flat node array, tagged with
   `subTreeId` and a `requiresNode` cross-tree prerequisite pointing into `specNodes`) and
   `subTreeNodes` (choice nodes representing "pick one of two hero subtrees", e.g. "Elune's Chosen /
   Keeper of the Grove" for Balance Druid). `generate_tree_structures_from_raidbots()` only reads
   `spec_dict["classNodes"]` and `spec_dict["specNodes"]`
   (`tree_presets_generator.py:143-148`) — hero data is never touched, so an entire in-game talent
   tier is silently missing from every generated preset regardless of whether the script crashes.
   This is not a quick patch: TTM's whole data model assumes **exactly two trees per spec** (a class
   tree and a spec tree), encoded directly as a binary flag —
   `is_spec_tree = 0 if "class_" in spec_name else 1` (`tree_presets_generator.py:540`) and
   `class_and_spec_trees[class_name][f"class_{spec_name}"] / [spec_name]`
   (`:143-146`) — with no third slot, and it has no concept of "the player must choose exactly one of
   two mutually-exclusive subtrees" or of a node in one tree requiring a specific node in a
   *different* tree (`requiresNode`). Representing hero talents correctly needs a third tree slot, a
   subtree-choice construct, and cross-tree prerequisite edges — none of which exist in
   `Talent`/`TalentTree` today.
2. **Fixed pixel-grid row/column derivation.** `talent.row = (posY - y_offset) // 300` and
   `talent.column = (posX - x_offset) // 300` with hardcoded `x_offset = 1200 if is_class_tree else
   9000`, `y_offset = 1200` (`tree_presets_generator.py:163-164, 185-186`) assumes Raidbots always
   lays out class trees and spec trees in the same two fixed horizontal bands 300 units apart. Any
   change to Raidbots' layout constants (which they control, not Blizzard) — e.g. to make room for a
   third (hero) tree region — silently misplaces or overlaps nodes with no validation.
3. **Positional/hardcoded array indexing for versions.** `resource_versions.txt` is parsed purely by
   line index cast from an enum (`GUI/src/Updater.cpp:83, 102, 113, 122-138`) with no field names —
   adding, removing, or reordering a resource type breaks every existing client silently or fully
   invalidates the cache (`flagAllResources`, triggered whenever line counts differ, `:112-120`).
4. **Version string assumed to always be exactly 3 numeric dot-separated components.**
   `compareVersions()` (`GUI/src/Updater.cpp:153-166`) does `std::stoi` on exactly 3
   `split(".")` segments with no bounds/format checking — a malformed or differently-shaped version
   string (e.g. missing a segment, or containing a suffix like `-beta`) throws an unhandled exception
   or silently misindexes.
5. **Version number is not a real schema version — it's the app's own release semver, hand-typed and
   already inconsistent.** `preambel(...)`/`combine_tree_strings(...)` hardcode `version="1.3.8"`
   (`tree_presets_generator.py:539, 553`) while `resource_versions.txt` says `1.4.2`
   (`Engine/resources/cicd_presets/resource_versions.txt:1-4`) and `TTMEnginePresets.h:17` also says
   `1.4.2`. Nothing enforces these three copies of "the version" staying in sync; a future data
   *format* change (e.g. adding a hero-tree field to the tree string) would have no dedicated version
   bump to hang a client-side migration off of.
6. **HTML/undocumented-endpoint scraping for descriptions.** `fill_in_descriptions()` depends on the
   internal `nether.wowhead.com/tooltip/spell/...` JSON having a `tooltip` field containing a
   `<div class="q">...</div>` block (`tree_presets_generator.py:242-277`); if Wowhead changes their
   tooltip HTML/CSS class names (a purely cosmetic front-end change on their end, unversioned), every
   description silently degrades to the literal string `"Description not available"`
   (`:254-256, 273-275`) with no error raised — a correctness failure that would not even fail CI.
7. **Brittle image-URL retry heuristic.** `download_icon()` "fixes" 404s by repeatedly replacing the
   last `_` in the icon URL with `-` (`tree_presets_generator.py:456-469`) — a heuristic tuned to a
   historical Wowhead/Zam icon-naming quirk; if the icon CDN changes its naming scheme differently,
   icons fail silently (`print(f"error getting icon...")`, no exception, no CI failure) and the
   talent ends up with no icon in the packed atlas at all (only the icons that succeeded are packed;
   nothing pads the gap explicitly other than downstream fallback to `default.png`).
8. **String-format wire format with ad-hoc escaping.** The TTM tree string uses `:`, `,`, and `;` as
   structural delimiters and hand-rolled placeholder substitution for those same characters inside
   free text (`clean_string`/`clean_ttm_name`, `tree_presets_generator.py:429-453`, mirrored by
   `restore_string` in `Web/server/data_management/preset_updater.py:167-175`). Any future
   Blizzard/Wowhead text containing an un-escaped edge case (e.g. a new special character, or nested
   delimiter sequences) can silently corrupt the parse on both the C++ and Python consumers, and any
   schema change (e.g. a new field) requires manually re-counting `split(":")`/`split(";")` indices
   everywhere the format is consumed (`preset_updater.py:82-121` indexes fields purely positionally,
   e.g. `talent_information[11]` for icon names, `[-1]` for node id).
9. **Silent success/no-op on unexpected data shapes rather than hard failure.** The generator has no
   schema validation against Raidbots' JSON at all — it just does direct dict/key access
   (`talent_dict["id"]`, `["maxRanks"]`, `entry["spellId"]`, etc., throughout `extract_tree`,
   `tree_presets_generator.py:160-231`). A missing key throws a raw `KeyError` inside a 3-minute CI
   job (`timeout-minutes: 3`, `.github/workflows/update_presets.yml:29`) with no useful diagnostic
   surfaced anywhere except raw Action logs, and no retry/alerting.
10. **CI silently no-ops instead of failing loud.** The whole automation is "run it, hash the output,
    commit only if different" (`update_presets.yml:27-44`); if the scraper throws partway through, the
    step fails and the job simply doesn't commit — there is no notification, issue creation, or
    Slack/Discord webhook, so a break can go unnoticed indefinitely (as happened with the TWW
    change). `git commit`/push only runs `if: steps.updatestep.outputs.toupdate > 0`
    (`:36-48`), which also depends on the earlier step's `output` variable having been set at all —
    if `tree_presets_generator.py` crashes before printing/writing anything, `preset_processor.py`
    never runs, `toupdate` is never set, and the "commit" step is simply skipped with a green-looking
    partial failure.
11. **Talent "type" inferred rather than read**, via `"choice"` string match else `maxRanks > 1`
    else active (`tree_presets_generator.py:178-183`). Raidbots' current schema documents a `"tiered"`
    node type (WebFetch of live `talents.json` structure, confirmed 2026-09-19) that this branch does
    not know about — such nodes fall through to the `maxRanks > 1` heuristic and get mis-classified
    as ordinary passives rather than tiered/scaling nodes, with no error.
12. **Hardcoded local file dependency.** `pack_and_save_icons()` reads a fixed relative path
    `"../../../GUI/resources/icons/default.png"` (`tree_presets_generator.py:479`) — moving/renaming
    that asset breaks the CI job with a bare `FileNotFoundError` and no context.
13. **Pinned/ancient dependency versions.** `requirements.txt` pins `aiohttp==3.8.1`,
    `requests==2.27.1`, `numpy==1.22.4`, `pillow==9.1.1` (`requirements.txt:1-4`) and the workflow
    pins Python `3.10.8` (`update_presets.yml:19`) on `ubuntu-20.04` (`:10`) — all now multiple major
    versions behind; a GitHub-hosted `ubuntu-20.04` runner image is itself deprecated/being retired,
    which independently threatens the CI job regardless of the scraping logic.
14. **Desktop update is not atomic across the 4 resource files** (see §4) — a mid-update failure can
    leave presets/icons/node-id-orders mutually inconsistent client-side with no rollback or
    integrity check (no checksums are transmitted or verified, only re-derived from a byte-identical
    re-fetch).
15. **Everything downstream (web app) trusts the desktop pipeline's output blindly.**
    `Web/server/data_management/preset_updater.py` and `extract_icons_to_public.py` do not talk to
    Raidbots/Wowhead at all; they parse whatever `presets.txt`/`icons_packed.*` the CI job last
    produced (`preset_updater.py:20-33`, `extract_icons_to_public.py:8-25`). If the CI job silently
    stalls (per items 6, 9, 10, 11 above) the web app has no independent way to detect staleness — it
    will keep re-ingesting the last-known-good, pre-TWW data forever.

---

## 6. What the pipeline produced that we still need (target data fields)

Regardless of the future data source, the web app needs to obtain the following per class/spec, at
minimum equivalent to what this pipeline used to produce:

- **Tree topology**: one node graph per "tree" the player fills in. Historically 2 per spec (class
  tree, spec tree); for TWW this must become at least 3 concepts: class tree, spec tree, and hero
  talent tree(s) with **subtree selection** (mutually exclusive choice of hero path) and **cross-tree
  prerequisites** (a hero node requiring a specific spec node).
- **Per node**: stable node id (Blizzard's `id`), grid position (`row`/`column` or equivalent),
  node type (active/passive/choice, and now tiered), max ranks, points-required-to-unlock,
  "free"/prefilled flag, and parent/child adjacency (`prev`/`next`) for rendering connectors and
  validating point-spend order.
- **Per rank/entry** (a node can have 1 entry, or 2+ for a choice node): display name(s), spell id,
  a stable "definition id" (used by Wowhead-style tooltip lookups), and a talent description string.
- **Icon per entry**: a resolvable icon asset (file name/slug) plus the actual image bytes, at a
  consistent square resolution, matched 1:1 to the talent entry that owns it — with a documented
  deterministic naming/matching rule (not a fuzzy fallback) so the pipeline stays reproducible.
- **Class/spec identity metadata**: class name, spec name, Blizzard `classId`/`specId`, and a stable
  "full node order" list (used by TTM for solver/serialization ordering) per spec.
- **A versioning signal** that reflects the *data schema/content*, decoupled from the application's
  own release version, so clients (desktop or web) can detect "the underlying game data changed" vs.
  "the app itself changed" independently, and so a future format change (e.g. adding hero-tree
  fields) can be migrated deliberately rather than silently.
- **Human-readable descriptions** — whatever the source, this pipeline shows description text is
  necessary product content, not optional (it's stored and rendered per talent, per rank).
- **A packaging/delivery mechanism** for icons that a browser can consume directly (individual PNG
  files, as the web app already unpacks them to `Web/frontend/public/preset_icons/`), since the
  atlas-packing format existed purely as a bandwidth optimization for the native updater and buys
  the web app nothing.
