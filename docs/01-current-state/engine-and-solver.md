# Engine & Solver — Technical Reference

Scope: `Engine/src/TreeSolver.{h,cpp}`, `Engine/src/TTMEnginePresets.{h,cpp}`, the algorithmic
routines in `Engine/src/TalentTrees.{h,cpp}` (cycle checking, ordering, auto-positioning,
expand/contract, skillset validation, level requirements), and `CLI/CLI.{h,cpp}`. String
parsing/serialization in `TalentTrees.cpp` is intentionally not covered here (another doc
covers it). All line numbers refer to the current `master` (commit `b82a9f9`).

---

## 1. Algorithm — how counting works end to end

### 1.1 Preprocessing: expand multi-rank talents into single-point nodes

Before solving, `expandTreeTalents(TalentTree&)` (`TalentTrees.cpp:1714-1756`) rewrites the
tree so **every node is worth exactly 1 point**:

- A talent with `maxPoints > 1` is chopped into `maxPoints` chained single-point talents
  (`expandTalentAndAdvance`, `TalentTrees.cpp:1761-1856`). Each new part gets a synthetic
  index `(talent->index + 1) * maxTalentPoints + i` (`TalentTrees.cpp:1817`) — this exact
  formula is duplicated in three other places and called out in code comments as
  fragile/load-bearing: `TreeSolver.cpp:222-229` (filter setup), `TreeSolver.cpp:894-901`
  (`filterSolvedSkillsets`), `TreeSolver.cpp:1027-1035` (`skillsetIndexToSkillset`), and
  `CLI.cpp:305-313`. A comment literally says "if this changes, also change ..." at every
  site — this indexing scheme is a real landmine for anyone refactoring the engine.
- Pre-filled talents (already fixed/free points) are turned into pseudo-roots: their
  children become new tree roots and the pre-filled talent itself is deleted from
  `orderedTalents` (`TalentTrees.cpp:1719-1743`). This is how "free" talents (already
  granted) are made to not consume search-tree structure.
- Switch talents (`TalentType::SWITCH`, a 1-point either/or choice between two abilities in
  one slot) are *not* expanded into two nodes — they stay one bit, and the *choice*
  (which of the two) is tracked out-of-band (see §1.4 / §5).

`countConfigurations*` always calls `expandTreeTalents` internally on a **copy** of the
processed tree (`TreeSolver.cpp:42,46`); callers pass in the "compact"/authored tree.

### 1.2 `createSortedMinimalDAG` — topological sort into an array-of-children DAG

`createSortedMinimalDAG(TalentTree tree)` (`TreeSolver.cpp:654-745`) runs a textbook Kahn's
algorithm (the pseudocode is transcribed verbatim in a comment, `TreeSolver.cpp:664-681`) to
turn the (already-expanded, tree-of-shared_ptr) `TalentTree` into two parallel flat arrays:

- `sortedTalents: TalentVec` — talents in topological order.
- `minimalTreeDAG: vec2d<int>` — for talent at position `i`, `minimalTreeDAG[i][0]` is a
  **weight**: `2` if the talent is a `SWITCH`, else `1` (`TreeSolver.cpp:733`); `[1..]` are
  the **integer positions** (not object pointers) of its children in `sortedTalents`.

Critically, ties in Kahn's queue are broken by sorting the ready-set by `pointsRequired`
ascending before each pop (`TreeSolver.cpp:682-684, 715-717`). This guarantees: (a) talents
are laid out left-to-right, top-to-bottom, and (b) **every talent's positional index is a
valid proxy for "how deep/gated it is"** — a talent with a higher required point count never
gets a lower array index than one with a lower requirement. This monotonic property is what
lets the recursive visitor use plain integer-index comparisons instead of re-checking parent
pointers.

Root talents are recorded as `rootIndices` (positions in `sortedTalents`), and
`TalentTree tree` is destructively consumed — parent lists inside talents are emptied by the
process (explicitly warned about, `TreeSolver.cpp:658`).

A near-identical, no-longer-used-by-the-fast-path duplicate exists as
`createSortedMinimalDAGLegacy` (`TreeSolver.cpp:747-838`) for the legacy pipeline.

### 1.3 Bitset representation: `SIND` = `uint64_t`

`using SIND = std::uint64_t;` (`TTMEnginePresets.h:13`). A skillset is one 64-bit integer:
bit `i` set means "the talent at `sortedTalents[i]` (after expansion) is selected"
(`setTalent`, `TreeSolver.cpp:843-845`: `talent |= 1ULL << index`). This is the whole
memoization trick: **a legal, complete talent selection of N points is a single 64-bit
integer**, trivially hashable/comparable/storable, and one enumeration path = one integer
push into a `std::vector<SIND>`.

The 64-bit width is a hard ceiling: both `countConfigurationsSingle`, `-Filtered` and
`-Parallel` throw `std::logic_error("Number of talents exceeds 64, need different indexing
type instead of uint64")` if `sortedTreeDAG.sortedTalents.size() > 64`
(`TreeSolver.cpp:54-55, 204-205, 439-440`). Because expansion turns every multi-rank talent
into `maxPoints` separate 1-point nodes, this is a limit on **total point-slots in the
(expanded) tree**, not on the number of authored talent nodes — e.g. a tree with 40 talents
where several have 2-3 ranks can easily exceed 64 expanded slots. Real Dragonflight class/
spec trees (~30-65 authored nodes, many multi-rank) sit close to or over this ceiling; this
is the single most important portability/scaling constraint of the whole engine.

### 1.4 The recursive enumerator: `visitTalentSingle` / `visitTalentFiltered` / `visitTalentParallel`

All three share one shape (e.g. `visitTalentSingle`, `TreeSolver.cpp:114-181`):

1. **Mark** the current talent's bit in `visitedTalents` (`setTalent`), increment
   `talentPointsSpent`, decrement `talentPointsLeft`, and multiply `currentMultiplier` by the
   node's weight (`1` normal, `2` switch) — `currentMultiplier` is the number of *distinct
   real skillsets* one bit-pattern represents once switch-talent side is chosen (used only by
   the Parallel/Legacy variants' `allCombinations` running totals, `TreeSolver.cpp:518,628`;
   the Single/Filtered fast paths don't need it because they emit exact bit patterns per
   switch choice... actually they don't branch per switch choice at all, see below).
2. **Terminate** if `talentPointsLeft == 0`: push `visitedTalents` into `combinations` and
   return (`TreeSolver.cpp:143-147`).
3. **Prune** (the key speed trick): if
   `sortedTalents.size() - talentIndexReqPair.first - 1 < talentPointsLeft`, the remaining
   suffix of the topologically sorted array is too short to ever spend the leftover points —
   dead branch, return immediately (`TreeSolver.cpp:151-154`). This is a cheap O(1) integer
   check enabled entirely by the topological sort from §1.2.
4. **Expand frontier**: append the current talent's children (mapped through
   `minimalTreeDAG`) into `possibleTalents`, keeping the vector sorted via
   `insertIntoVector`'s `std::lower_bound` insert (`TreeSolver.cpp:871-875`) — this both
   de-duplicates (a talent with two parents is only inserted once) and preserves an order
   that supports the next step.
5. **Recurse** only over `possibleTalents[i]` where the candidate's array index is greater
   than the current node's index *and* `talentPointsSpent >= pointsRequired` of the
   candidate (`TreeSolver.cpp:162-165`). The strictly-increasing-index constraint is what
   turns "which subset of talents is reachable" into "which strictly increasing sequence of
   array indices is reachable" — this is the classic trick for enumerating combinations
   without a visited-set/hash-dedup: **because the DAG is topologically sorted, walking only
   forward in index order can never revisit a node or produce a permutation of the same set
   twice.** This — combined with the O(1) suffix-length prune in step 3 — is *why the search
   is fast*: no hashing, no dedup pass, no permutation explosion; the recursion tree is
   already exactly the set of realizable skillsets, pruned as early as topologically
   possible.

`pointsRequired` (row-gating, e.g. "must have spent 8 points before unlocking this row") is
enforced purely through the `talentPointsSpent >= pointsRequired` check in step 5 — no
separate "row" concept exists in the solver; `pointsRequired` is just an integer compared
against a running counter.

**Switch talents are not branched on during enumeration at all.** A switch talent occupies
one bit like a normal talent; which of its two abilities is "active" is not part of the
combinatorial search space — it's resolved afterward as a fixed per-switch-talent global
choice (`switchTalentChoices`, defaulted to `1` for every switch talent — see
`TreeSolver.cpp:104-108,328-333,474-479`; also mirrored by the CLI's own switch-bit
extraction at `CLI.cpp:350-369`). This means the engine counts *skillsets* (which nodes get a
point) not *specs* (which nodes + which switch choice) — enumerating actual switch
choice-space is left to the caller (CLI does it post-hoc for output only, not for counting).

### 1.5 Root selection quirk

Enumeration only starts from roots whose `pointsRequired == 0`
(`TreeSolver.cpp:74,298,462`) — a comment explains this "prevents starting at root nodes that
might come later in the tree (e.g. druid wild charge)", i.e. some trees have a root-level
node (no parent) that is nonetheless gated by point requirement and must be reached only via
the point-count check, not treated as a free starting point.

---

## 2. Variants: Single vs Parallel vs Filtered vs Legacy

| Function | File:line | Purpose | Threading | Output shape |
|---|---|---|---|---|
| `countConfigurationsSingle` | `TreeSolver.cpp:34-112` | Count configurations for **exactly** `talentPointsLimit` points, no filter | single-threaded | `allCombinations`: a `vec2d<SIND>` padded with empty vectors for points `1..N-1`, real data only in the last slot (`TreeSolver.cpp:94-99`) |
| `countConfigurationsFiltered` | `TreeSolver.cpp:183-337` | Same as Single but filter is applied **during** enumeration (early-exit) | single-threaded | `allCombinations` has exactly one vector (the filtered results for `talentPointsLimit`) |
| `countConfigurationsParallel` | `TreeSolver.cpp:419-482` | Enumerate **all** combinations for every point count `1..talentPointsLimit` in one DFS pass (every intermediate node, not just leaves, is recorded) | **misnomer** — despite the name, this function is entirely single-threaded; it just returns "parallel" results, i.e. all-N-at-once, not multi-threaded execution | `allCombinations[k]` = every valid combination using exactly `k+1` points |
| Legacy: `countConfigurationsParallelLegacy` / `visitTalentParallelLegacy` | `TreeSolver.cpp:544-647` | Same as Parallel but stores `std::pair<SIND,int>` (skillset, multiplier) instead of bare `SIND`, and uses a hardcoded `500000000` safety cap instead of the dynamic memory-based one | single-threaded | `vec2d<std::pair<SIND,int>>` |

**Important finding: none of these C++ functions spawn threads.** "Parallel" in the naming
refers to computing results for all point-totals in parallel *conceptually* (one pass
produces every intermediate answer) rather than literal multithreading. The *only* actual
multi-threading in the whole stack is in the CLI, one level up:
`CLI::startThreadedCombinationCount` (`CLI.cpp:269-297`) uses Microsoft's **PPL**
(`Concurrency::parallel_for`, `#include <ppl.h>`, `CLI.cpp:26,271`) to run
`countConfigurationsFiltered` for **multiple different trees** concurrently (one thread per
tree/"structure" in the input file), gated behind the `--parallel`/`--concurrent` CLI flag.
There is no intra-tree parallelism (e.g. splitting one tree's search space across cores) —
each individual tree solve is single-threaded. PPL is MSVC/Windows-only (see §7).

`RESERVED_MEMORY_LIMIT` and `safetyGuard` sizing (see §4) only exist on the `TreeDAGInfo`
struct used by Single/Filtered/Parallel; the Legacy struct hardcodes `500000000`
(`TreeSolver.cpp:611`) with no memory awareness at all.

---

## 3. Filtering

### 3.1 Filter encoding (`TalentSkillset::assignedSkillPoints`, values are sentinel-coded)

A filter is just a `TalentSkillset` (`TalentTrees.h:79-85`) whose `assignedSkillPoints` map
uses out-of-band negative sentinels to mean "constraint" rather than "points assigned"
(built in `countConfigurationsFiltered`, `TreeSolver.cpp:236-280`, and duplicated verbatim in
`filterSolvedSkillsets`, `TreeSolver.cpp:909-953`):

- value `> 0` → **include**: this talent must have exactly this many points (bits for ranks
  `0..value-1` are OR'd into `includeFilter`).
- value `== -1` → **exclude**: this talent must have zero points (`excludeFilter`).
- value `== -2` → **or-group**: at least one talent among all `-2`-tagged entries must be
  selected (all such talents' bits OR'd into one `orFilter`; satisfied if
  `(skillset & orFilter) > 0`).
- value `== -3` → **one-of-group ("exactly one maxed, rest excluded")**: talents sharing this
  tag are partitioned into `(inc, exc)` bit-pairs — one pair per involved talent, where `inc`
  = that talent's own bits and `exc` = every other same-group talent's bits — pushed onto
  `oneFilter: vector<pair<SIND,SIND>>` (`TreeSolver.cpp:236-260`). A skillset passes if
  **exactly one** pair in `oneFilter` matches (`matches == 1`, see `checkSkillsetFilter`).

### 3.2 Where filtering happens: during enumeration OR as a post-pass, depending on entry point

- **During enumeration**: `countConfigurationsFiltered` → `visitTalentFiltered`
  (`TreeSolver.cpp:343-413`) checks the filter twice: (a) on the exclude bits, as an
  **early-exit prune** identical in spirit to the length-prune
  (`(visitedTalents & excludeFilter) != 0` short-circuits the whole subtree,
  `TreeSolver.cpp:379-383`), and (b) the full `checkSkillsetFilter` only at a **complete**
  path (`talentPointsLeft == 0`, `TreeSolver.cpp:372`). So exclude constraints prune the
  search tree; include/or/one constraints are checked only at completion (they can't safely
  prune mid-path because points not yet spent could still satisfy them).
- **As a post-pass**: `filterSolvedSkillsets(tree, treeDAG, filter)` (`TreeSolver.cpp:880-986`)
  takes an already-computed `treeDAG->allCombinations` (typically from
  `countConfigurationsParallel`, which has no filter argument) and produces
  `treeDAG->filteredCombinations` by scanning every stored combination with the same bitmask
  logic (`TreeSolver.cpp:960-983`). This is the path used when you want to explore many
  different filters against one precomputed full enumeration without re-solving.
- `checkSkillsetFilter` (`TreeSolver.cpp:988-1014`) is the single shared predicate used both
  inline (during `visitTalentFiltered`) and in the post-pass loop.

Both paths use the exact same bit-arithmetic filter representation, so a filter built for
one is directly reusable for the other.

---

## 4. Limits & resource use

- `RESERVED_MEMORY_LIMIT = 4294967296` (4 GiB) — `TreeSolver.h:9`, a fixed amount of system
  RAM assumed reserved for OS/other processes and *not* available to the solver.
- `setSafetyGuard(TreeDAGInfo&)` (`TreeSolver.cpp:1064-1069`) calls Win32
  `GlobalMemoryStatusEx` to read total physical RAM, then sets
  `safetyGuard = (totalPhysicalBytes - RESERVED_MEMORY_LIMIT) * 0.5 * 0.125`. In words: take
  installed RAM, subtract the reserved 4 GiB, use half of what's left, and budget roughly
  1/8th-byte-per-unit of that (the `0.125` factor) as the maximum number of *complete
  combinations counted* (`runningCount`) before aborting. E.g. on a 16 GiB machine:
  `(16e9 - 4.29e9) * 0.5 * 0.125 ≈ 7.3e8` combinations allowed. The struct's default
  fallback (before `setSafetyGuard` runs, and always for the Legacy path) is
  `size_t safetyGuard = 500000000;` (`TreeSolver.h:31`, also hardcoded as a literal in
  `visitTalentParallelLegacy`, `TreeSolver.cpp:611`).
- The guard is checked at the top of every recursive visit
  (`runningCount >= sortedTreeDAG.safetyGuard || safetyGuardTriggered`,
  `TreeSolver.cpp:133,501`) and, once tripped, short-circuits all further recursion and sets
  `safetyGuardTriggered = true` on the returned `TreeDAGInfo`
  (`TreeSolver.cpp:89-91,317-319,465-467`) — callers must check this flag; there is no
  partial-results contract stated anywhere beyond "whatever had been pushed before the
  trip stays in `combinations`".
- **SIND width limit** (repeated from §1.3): hard cap of **64 expanded talent point-slots**
  per tree, enforced by a thrown `std::logic_error` (`TreeSolver.cpp:54-55, 204-205,
  439-440`). No graceful degradation — a too-large tree crashes the solve call.
- **No CPU-time limit at all** — only combination-count and memory are guarded; a
  pathological tree with abundant valid combinations well under the memory guard but few
  points spent per node (i.e. very deep before it starts pruning) can still run for a long
  wall-clock time without protection.
- **Realistic sizing** (no benchmark numbers exist in the codebase — none of the files
  contain measured timings, so the following is inferred from the algorithm's own
  complexity, not quoted from code): counting combinations is the number of ways to walk a
  DAG spending exactly K of its ~50-64 point-slots respecting order/prerequisites — this is
  large but heavily pruned by the point-requirement gates (typical Dragonflight trees gate
  additional rows at 8/20 points spent, see `autoPointRequirements`, `TalentTrees.cpp:1572-
  1591`, mirroring Blizzard's real point-gating). For a full 30/31-point single spec tree
  this is known (from community tooling built on the same idea) to run into the tens-to-
  hundreds of millions of raw combinations before filtering — consistent with why the
  safety guard defaults to ~5×10^8 and scales with RAM. `elapsedTime` is measured internally
  with `std::chrono::high_resolution_clock` (`TreeSolver.cpp:69,92,293,320,456,468,574,587`)
  and stored in seconds on the result struct, but no example values are hardcoded anywhere.

---

## 5. Outputs

- **Single/Filtered/Parallel** all populate `TreeDAGInfo` (`TreeSolver.h:20-32`):
  `allCombinations` (`vec2d<SIND>`), `filteredCombinations` (only meaningfully populated by
  `filterSolvedSkillsets`, `TreeSolver.cpp:955-985`), `elapsedTime`, `safetyGuardTriggered`,
  plus the DAG scaffolding (`minimalTreeDAG`, `sortedTalents`, `rootIndices`,
  `switchTalentChoices`, `processedTree`).
- **`skillsetIndexToSkillset(tree, treeDAG, skillsetIndex)`**
  (`TreeSolver.cpp:1019-1062`) turns one `SIND` bit-pattern back into a full
  `TalentSkillset` (a real, named, `assignedSkillPoints`-keyed-by-*original compact
  index* result): it builds `expandedToCompactIndexMap` using the exact same
  `(index+1)*maxTalentPoints+(rank-1)` formula from expansion (§1.1) to fold multi-rank
  expanded bits back onto one compact talent index, incrementing
  `assignedSkillPoints[compactIndex]` per set bit, and — for switch talents — looks up the
  chosen side from `treeDAG->switchTalentChoices` (defaulting to `1`,
  `TreeSolver.cpp:1048-1054`).
- **`fillOutTreeWithBinaryIndexToString(comb, tree, treeDAG)`** (`TreeSolver.cpp:850-869`) is
  the alternate reconstruction path: it walks all 64 bits of `comb`, sets `points = 1` on the
  corresponding *expanded* talent objects directly in a copy of `treeDAG.sortedTalents`,
  then calls `contractTreeTalents(tree)` (folds expanded single-point chains back into
  multi-rank talents, `TalentTrees.cpp:1861-1918`) and finally `getTalentString(tree)` to
  serialize to TTM's own string format. This function also contains a hardcoded/debug
  side-effect: if bit 190000 happens to be set it calls `visualizeTree(...)` — dead debug
  code (`i == 190000` can never be true since the loop only goes to 64,
  `TreeSolver.cpp:852,860-861,865-866`) that should simply be deleted when porting.
- Both reconstruction paths are O(64) or O(#talents) — cheap; the expensive part is always
  the enumeration, not the decode.
- **CLI-specific output**: see §6 — the CLI writes its own compact text format, it does not
  call `skillsetIndexToSkillset`/`fillOutTreeWithBinaryIndexToString` at all; it works
  directly with raw `SIND` integers and a hand-rolled bit→talent-index remapping.

---

## 6. The CLI — exact interface as it exists today

Entry point: `CLI/CLI.cpp:28-38` (`main`). No usage/help text is printed for invalid flags
beyond the one guard below.

### 6.1 Arguments (`processCommandLine`, `CLI.cpp:41-78`)

| Flag | Required | Meaning |
|---|---|---|
| `--structure-file-path <path>` | Yes (argc≤1 aborts with `missingStructureFilePath`) | Path to a text file, one tree per line, in TTM's tree-string format |
| `--structure-indices <csv ints>` | No | Comma-separated line indices (0-based) to select from the structure file; empty = all |
| `--filter-file-path <path>` | No | Path to a file of skillset-format filter lines, one per selected tree, in file order |
| `--filter <raw string>` | No | Inline filters, `;`-separated (one per tree), each filter itself `:`-separated either positionally (row/col order) or `talentIndex,value:...` keyed |
| `--output-file-path <path>` | No (no `--output-file-path` ⇒ no file is written at all, results only printed to stdout via `printSettings`) | Where to write results |
| `--target-talent-count <int>` | No, default `1` | Points to solve for (single N only — the CLI always calls `countConfigurationsFiltered`, never `-Single`/`-Parallel`) |
| `--parallel` / `--concurrent` | No | Solve multiple selected trees concurrently via PPL `parallel_for` (one thread per tree, not per search) |

Argument parsing is a linear scan with no validation of malformed values (`std::stoi` will
throw uncaught on garbage input, killing the process) and no `--help`.

### 6.2 What it does (`runCombinationCount`, `CLI.cpp:80-89`)

1. `generateRunDetails` (`CLI.cpp:111-267`): parse the structure file line-by-line (skipping
   non-selected indices), rewrite each line's preset marker to force `"custom"`
   (`CLI.cpp:132-134` — this hardcodes CLI-solved trees away from named presets), validate/
   repair with `Engine::validateAndRepairTreeStringFormat`, then `Engine::parseTree`. Then
   parse filters (inline `--filter` or `--filter-file-path`), supporting either positional
   (sorted row/col order) or explicit-index skillset syntax.
2. `startThreadedCombinationCount` (`CLI.cpp:269-371`): calls
   `Engine::clearTree` + `Engine::countConfigurationsFiltered` per tree (parallel or serial
   per `--parallel`), then builds a **bit-position → positional-talent-index** table
   (`bitToIndexVec`) and, per result combination, a list of which switch talents were
   selected (`assignedSwitchIndices`) — this is the CLI's own hand-written analogue of
   `skillsetIndexToSkillset`, done independently rather than by calling that function.
3. `outputCombinations` (`CLI.cpp:373-393`): if `--output-file-path` was given, writes, per
   tree: one header line of `bitToIndexVec` values joined by `/`, then one line per resulting
   combination: the raw decimal `SIND` integer, followed by `,`-joined switch-bit positional
   indices, then a blank line separating trees.

### 6.3 I/O format summary

- **Input**: plain text files using TTM's own `;`/`:`/`,`-delimited tree string format
  (documented in `TTMEnginePresets.h:179-208`) and a comparable skillset-string format for
  filters. No JSON, no XML — a bespoke, whitespace-fragile custom format.
- **Output**: plain text, one raw decimal `uint64` per matching skillset plus CSV-ish switch
  metadata; **not** a re-usable skillset string, **not** JSON — the caller must independently
  know the `bitToIndexVec` mapping to interpret bits back into named talents.
- **What it cannot do today**: no progress reporting, no partial/streaming output (writes
  everything at the very end after the whole solve finishes), no distinction in output
  between "solved cleanly" vs "safety guard triggered" (that flag is computed
  per-`RunDetails` but never written to `outFile` or printed), no per-run time reporting to
  the user (elapsed time is computed inside `TreeDAGInfo` but never surfaced by the CLI), no
  way to run the Single/Parallel (all-N) variants — only the Filtered single-N path is wired
  up, and an empty/no-op filter is a legal way to get "no filter" behavior. No exit code
  differentiation beyond `1` for missing structure path vs `0` for everything else (including
  a run that hit the safety guard).

---

## 7. Portability assessment — Linux/Docker build

### 7.1 Toolchain facts (from the `.vcxproj` files)

- `PlatformToolset v143` (Visual Studio 2022 MSVC), `LanguageStandard: stdcpp17` (C++17) for
  every configuration of both `Engine.vcxproj` and `CLI.vcxproj`.
- `Engine` builds as a **StaticLibrary**; `CLI` is an **Application** referencing it via
  `<ProjectReference>` (`CLI.vcxproj:166-169`) and adds
  `$(SolutionDir)Engine\src` as an include dir (`CLI.vcxproj:96`, etc.) — i.e. CLI compiles
  against Engine's headers/sources directly, no public API boundary or install step exists.
- `Engine.vcxproj` lists libcurl headers (`src\libs\libcurl\...`) as `ClInclude` only (not
  compiled into the ClCompile list, and no `curl_*` symbol appears in `TalentTrees.cpp`,
  `TreeSolver.cpp`, or `TTMEnginePresets.cpp` — confirmed by grep). **Curl is not a real
  dependency of the Engine/CLI code paths we need**; it's presumably used elsewhere (GUI/
  AppUpdater) and just sits in this project file unused for our purposes.
- No `#pragma once`-incompatible or MSVC-`__declspec` usage found; no vendored/prebuilt
  Windows `.lib`/`.dll` binaries are linked into Engine/CLI (only curl headers, unused).

### 7.2 Concrete blockers to a Linux/gcc-or-clang/Docker build

- **`#include <Windows.h>` / `<windows.h>`** in three files that must compile on Linux:
  - `Engine/src/TreeSolver.cpp:27` — used solely for `GlobalMemoryStatusEx` in
    `setSafetyGuard` (`TreeSolver.cpp:1064-1069`). Replace with `sysconf(_SC_PHYS_PAGES) *
    sysconf(_SC_PAGE_SIZE)` (POSIX) or `/proc/meminfo` parsing, or (recommended for a
    container worker) an explicit memory-budget config/env-var instead of querying host RAM
    at all, since container cgroup limits ≠ host `GlobalMemoryStatusEx` results anyway.
  - `Engine/src/TalentTrees.cpp:30` — `#include "Windows.h"`; a scan of the file did not
    turn up other obviously Windows-only API calls in the algorithmic sections read for this
    doc, but the full file (parsing/serialization portions, out of scope here) should be
    checked by whoever owns that half before assuming this include is dead weight.
  - `Engine/src/TTMEnginePresets.cpp:23-24` — `<windows.h>` + `<shlobj.h>`, used by
    `getAppPath()` (`TTMEnginePresets.cpp:298-331`) which calls `SHGetKnownFolderPath(
    FOLDERID_RoamingAppData, ...)` to locate `%APPDATA%\WoWTalentTreeManager`. This is pure
    Windows and has no POSIX equivalent — needs a real redesign, not a shim (see 7.3).
- **`#include <ppl.h>` + `Concurrency::parallel_for`** in `CLI/CLI.cpp:26,271` — Microsoft's
  Parallel Patterns Library, MSVC-only. Must be replaced with `std::thread`/
  `std::async`/a thread pool, or (simplest) Intel TBB / OpenMP `#pragma omp parallel for`,
  or just `std::for_each(std::execution::par, ...)` (C++17 parallel algorithms, needs
  libstdc++'s parallel STL / oneTBB on Linux — check availability in the chosen base image).
- **Windows filesystem/resource loading**: `getAppPath()` (`%APPDATA%\WoWTalentTreeManager`)
  is the path every preset/resource file (`presets.txt`, `node_id_orders.txt`) is read from
  (`TTMEnginePresets.cpp:333-362, 410-446`, etc.) — this must become an explicit,
  configurable path (env var or CLI flag) for a headless Linux worker; there is no reason a
  container worker should resolve "the user's roaming profile directory."
- **No `_BitScanForward`/`__popcnt`/other MSVC intrinsics found** anywhere in
  `TreeSolver.cpp`, `TalentTrees.cpp`, or `TTMEnginePresets.cpp` (grepped explicitly) — the
  bit manipulation in the solver (`setTalent`, filter masking) is done with plain portable
  `uint64_t` shifts/ANDs/ORs (`TreeSolver.cpp:843-845`, `988-1014`), which is fully portable
  as-is. No SIMD/intrinsic porting work needed here.
- **`std::filesystem`** is used throughout (`CLI.cpp:121,228`, `TTMEnginePresets.cpp`) —
  fully standard C++17, portable to gcc/clang without changes, just needs `-lstdc++fs` on
  some older toolchains (not needed on modern GCC ≥ 9 / Clang ≥ 9).
- **Everything else** (STL containers, `std::chrono`, `std::regex`, recursion, shared_ptr
  graphs) is standard C++17 and should compile unmodified with GCC/Clang given a
  CMake/Makefile replacement for the `.vcxproj` build definition (which itself is entirely
  MSVC-specific and must be replaced, not translated).

### 7.3 Net assessment

The **hot path — the actual solver algorithm (`TreeSolver.cpp` minus 6 lines of
`GlobalMemoryStatusEx`, and the algorithmic parts of `TalentTrees.cpp` read for this doc) —
is portable C++17** with no real platform dependency once `setSafetyGuard` is rewritten. The
two real blockers are (1) `getAppPath()`'s Windows-only resource-directory resolution, which
needs to become an explicit path/argument for a Dockerized worker anyway (a container has no
meaningful "roaming profile"), and (2) the CLI's PPL-based `parallel_for`, which is a small,
mechanical swap to `std::thread`/`std::async`/a hand-rolled thread pool since the workload
(independent trees) is embarrassingly parallel and doesn't need PPL's specific scheduler
semantics. Neither blocker touches the actual combinatorics/DAG code, so the "crown jewel"
algorithm itself carries over with effectively no behavioral risk. A CMake build (gcc/clang,
`-std=c++17`) replacing the two `.vcxproj` files is the natural next step.

---

## 8. Worker interface proposal

The existing CLI's custom text I/O (§6.3) is unsuitable for a web backend: it has no
progress channel, no error/status signaling beyond a process exit code, no chunking, and
requires the caller to independently reconstruct the `bitToIndexVec` mapping to make sense of
output. Recommendation, concrete:

### 8.1 Invocation shape: **args + files, not stdin/stdout JSON for the payload**

- Talent trees and filters are naturally *large, structured, reusable* inputs (presets are
  already stored as files today) — pass them as **file paths** via argv (as today), not
  inline JSON blobs on the command line (argv length limits, shell-escaping headaches) and
  not piped through stdin (stdin is better reserved for a lightweight control/progress
  channel, see below, or left unused).
- Convert the *format* of those files from TTM's bespoke `;:,`-delimited string to **JSON**
  (or NDJSON for the per-line structure file) so the web backend can generate/validate them
  with a normal JSON schema instead of hand-rolling the delimiter grammar. This is a surface
  change to the parsing layer (out of this doc's scope) but doesn't touch the solver.
- New argv contract, additive to what exists conceptually:
  `ttm-worker solve --tree <tree.json> --filter <filter.json optional> --points <N>
  --mode {single|filtered|parallel} --out <result-file-or-prefix> --progress-fd <fd or
  path> --max-results <N> --time-budget-ms <N> --mem-budget-bytes <N>`.
- Keep one process invocation = one tree solve (matches today's per-tree granularity); let
  the *web backend* fan out multiple worker processes for multiple trees (simple process
  pool / job queue) rather than reusing the CLI's in-process PPL fan-out — this is more
  robust for a backend (per-job resource limits, crash isolation, easy horizontal scaling)
  than one fat multi-tree process.

### 8.2 Output: **NDJSON to a result file (or stdout when small), explicitly chunked**

- Do **not** hold the entire result set in memory and dump it as one JSON array at the end
  (today's behavior, just in a worse format) — for large trees this is exactly the
  hundreds-of-millions-of-combinations regime the safety guard is designed around (§4), so
  buffering all of it defeats the point of guarding memory in the first place.
- Emit **newline-delimited JSON**, one line per resulting skillset (or per chunk of, say,
  10k skillsets, if per-line overhead matters), each line already resolved to the
  human-usable form (`skillsetIndexToSkillset` output: talent-index → points map) rather
  than a raw `SIND` integer — push the bit→talent decoding into the worker (it already has
  `treeDAGInfo`) instead of pushing it onto every caller like the current CLI does.
- Write results incrementally as they're produced (flush every N results) so a caller
  tailing the output file (or reading a streamed stdout) gets partial results even if the
  job is later killed for exceeding a budget — turns "safety guard tripped" from "silent
  truncation" (today's behavior, §4) into "graceful early stop with everything found so far
  still delivered."
- Final line (or a sibling `*.meta.json` file) reports a summary object: `{ "complete":
  bool, "safetyGuardTriggered": bool, "resultCount": N, "elapsedMs": N,
  "reason": "ok"|"safety_guard"|"time_budget"|"mem_budget"|"error" }` — today none of
  `safetyGuardTriggered` or `elapsedTime` reach the CLI's output at all (§6.3); this is a
  strict improvement with minimal code change since both values already exist on
  `TreeDAGInfo`.

### 8.3 Progress reporting

- The recursive solver has no natural "percent done" (DFS over an irregular tree), but it
  does have `runningCount` (§4) as a monotonically increasing integer already threaded
  through every recursive call. Cheapest useful signal: periodically (every K increments of
  `runningCount`, e.g. every 1,000,000) write a progress line to a **separate** channel — a
  second file descriptor/file (`--progress-fd`/`--progress-file`) rather than mixing it into
  the NDJSON result stream — containing `{ "resultsSoFar": N, "elapsedMs": N }`. This needs a
  small, localized change inside `visitTalentSingle`/`visitTalentFiltered` (an optional
  callback or atomic counter checked every N calls), not an architecture change.
- For jobs expected to run long, prefer polling a progress file over parsing live stdout —
  simpler for the backend, survives worker restarts/log rotation, and avoids the need for
  the worker to flush stdout precisely.

### 8.4 Capping work per job

- Enforce **all three** limits the engine already has hooks for, explicitly and
  container-aware, instead of inferring from host RAM (`GlobalMemoryStatusEx`/`sysconf` is
  meaningless inside a cgroup-limited container — §7.2):
  - `--max-results N` → pass through as the existing `safetyGuard` field (already a
    plain `size_t` on `TreeDAGInfo`, `TreeSolver.h:31`) instead of computing it from
    physical memory; the web backend decides the cap per plan/tier.
  - `--mem-budget-bytes N` → same field, computed backend-side from the *container's*
    memory limit if a dynamic cap is still wanted, never from host `GlobalMemoryStatusEx`.
  - `--time-budget-ms N` → **new**, not present in the engine today (§4 notes there is
    currently *no* wall-clock guard at all). Add a cheap `std::chrono` deadline check
    alongside the existing `runningCount >= safetyGuard` check at the top of each
    `visitTalent*` call (`TreeSolver.cpp:133,501` are the two call sites to touch) — same
    mechanism, second condition.
- Reject (fast, before spawning a worker) any tree whose expanded point-slot count exceeds
  64 (§1.3/§4) at the backend layer, with a clear user-facing error, rather than letting the
  worker process crash on the `std::logic_error` the engine throws today
  (`TreeSolver.cpp:54-55,204-205,439-440`) — that exception currently has no `try/catch`
  around the call sites we read, so today it would `std::terminate` the whole CLI process.

### 8.5 Summary recommendation

Args-and-files for the request (JSON tree/filter files, not stdin), NDJSON streamed to a
result file for the response (not a single blob), a sibling progress file/fd for status
polling, and three explicit, container-aware caps (`max-results`, `mem-budget-bytes`, a new
`time-budget-ms`) enforced inside the existing recursive visitor with minimal code changes.
One process per tree-solve job; let the backend's job scheduler provide the "many trees in
parallel" behavior the CLI currently does in-process via PPL, since that removes the last
Windows-only dependency from the hot path entirely.

---

## Solver size ceiling: verified analysis

Follow-up investigation triggered by a coordinator question about whether the 64-bit `SIND`
ceiling (§1.3/§4) is actually compatible with real Dragonflight retail trees, given that DF
spec trees have ~40-70 nodes and several multi-rank talents.

### 1. What exactly does the `> 64` check count?

The throw sites are identical in all three live variants:
`if (sortedTreeDAG.sortedTalents.size() > 64) throw std::logic_error(...)`
(`TreeSolver.cpp:54-55` in `countConfigurationsSingle`, `TreeSolver.cpp:204-205` in
`countConfigurationsFiltered`, `TreeSolver.cpp:439-440` in `countConfigurationsParallel`).
`sortedTreeDAG` comes from `createSortedMinimalDAG(*processedTree)`
(`TreeSolver.cpp:51,201,436`), and `processedTree` is the result of
`expandTreeTalents(*processedTree)` (`TreeSolver.cpp:46,196,431`) run on a full copy of the
input tree — **not** narrowed by `talentPointsLimit` in any way. `expandTreeTalents`
(`TalentTrees.cpp:1714`) unconditionally walks every root and expands every multi-rank
talent into `maxPoints` chained single-point nodes regardless of how many points the caller
intends to spend. So the quantity compared to 64 is: **the total number of expanded
single-point node slots in the entire tree structure** (i.e. Σ `maxPoints` over every
non-excluded talent, see #2) — not a budget-limited reachable subset, and not "number of
authored nodes" (switch talents keep `maxPoints == 1` in the source data, confirmed in
`Engine/resources/presets.txt`, e.g. druid_restoration talent 5 "Nature's Splendor / Passing
Seasons" has field `maxPoints = 1` — a switch talent always costs exactly one bit no matter
how many sides it has).

### 2. Does expansion prune anything, and do pre-filled talents consume bits?

**No talents are dropped for being unreachable within `talentPointsLimit`** — expansion is
budget-agnostic (see #1). **Pre-filled talents are dropped from the bit-consuming set
entirely.** In `expandTreeTalents`, when a root talent has `preFilled == true`
(`TalentTrees.cpp:1720-1743`): its children are cut loose from it (`child->parents.clear()`,
`TalentTrees.cpp:1736`) and pushed onto `tree.talentRoots` directly, and the pre-filled
talent itself is erased from `tree.orderedTalents` (`TalentTrees.cpp:1740`,
`tree.orderedTalents.erase(talentIndex)`). After the loop, `tree.talentRoots` is rebuilt
solely from what remains in `orderedTalents` (`TalentTrees.cpp:1747-1752`), so the erased
pre-filled talent can never reappear as a root or child. `createSortedMinimalDAG` only ever
walks from `tree.talentRoots`/`children` (`TreeSolver.cpp:661-719`), so a pre-filled talent
that was excised this way **never enters `sortedTalents` and consumes zero bits**. (A
non-root talent is only allowed to be pre-filled if it has a pre-filled parent —
`expandTalentAndAdvance` throws `"Non-root nodes cannot be pre filled!"` otherwise,
`TalentTrees.cpp:1793-1804` — so in practice pre-filled points only ever occur at/near the
top of a tree, exactly the WoW UI convention of a few "free" starter nodes.)

### 3. Real preset numbers, from `Engine/resources/presets.txt`

Parsed every one of the 79 preset records in `Engine/resources/presets.txt` directly
(talent record fields per `TTMEnginePresets.h:191-202`: field `[6]` = `maxPoints`, field
`[8]` = pre-filled flag, field `[3]` = type where `2` = switch).

`druid_restoration` (line 3, header declares `43` talents): **43 nodes**, **Σ maxPoints =
48**, 0 pre-filled points, 10 switch talents (each contributing 1 bit as expected) → the
expanded DAG needs **48 bits**, comfortably under 64, with 16 bits of headroom.

Across **all 79 presets** (every class and spec tree shipped at the time this repo was
current), the worst case is:

| preset | nodes | Σ maxPoints (bits needed) | pre-filled pts | switch talents |
|---|---|---|---|---|
| `druid_class_restoration/feral/guardian/balance` (all 4 identical structure) | 49 | **60** | 2 | 4 |
| `priest_class_shadow/holy/discipline` | 49 | 57 | 2 | 10 |
| `shaman_class_enhancement/restoration/elemental` | 48 | 57 | 2 | 11 |

**No preset in the shipped resource file exceeds 64** — the maximum is 60 (druid class
trees), 4 bits under the ceiling. So the original worry ("well over 64") does not hold for
any real, shipped Dragonflight preset: the reason is that the vast majority of DF talent
nodes are 1-point, only a handful per tree are 2-point, and — critically — **class trees and
spec trees are always separate `TalentTree` objects solved independently**, never merged
into one combined structure, which is what keeps each individual solve's node/point count in
the 40s-60s rather than the 90-130 range a combined class+spec tree would need.

### 4. GUI-side guard that prevented the shipped app from ever hitting the throw

`GUI/src/LoadoutSolverWindow.cpp` pre-empts the engine's exception with its own check,
executed *before* any solver call:

- `TalentTreeManagerDefinitions.h:270`: `const int loadoutSolverMaxTalentPoints = 64;`
- `LoadoutSolverWindow.cpp:515-525`: on clicking "Process tree", the code first clamps the
  requested points-to-solve-for to `tree.maxTalentPoints - tree.preFilledTalentPoints`, then
  checks `if (uiData.loadoutSolverTalentPointLimit > uiData.loadoutSolverMaxTalentPoints ||
  talentTreeCollection.activeTree().maxTalentPoints > uiData.loadoutSolverMaxTalentPoints)` —
  if true, it opens a `"Talent tree too large"` popup (wired to the modal at
  `LoadoutSolverWindow.cpp:183-186`: *"Talent tree has too many possible talent points!
  Maximum number of possible talent points spent is 64."*) and `return`s **without ever
  calling into `Engine::countConfigurations*`**. A comment at the guard
  (`LoadoutSolverWindow.cpp:519-521`) states explicitly: *"This check prevents trees with
  more than 64 spendable talent points from being handled. In a world where indexing would
  work with infinite spendable talent points (simple switch from uint64 to bitsets) this
  condition can be removed and everything should work fine."* — i.e. the original author
  already anticipated exactly this widening as the fix, not a redesign of the algorithm.
- The in-window help text also documents both limits to the user directly
  (`LoadoutSolverWindow.cpp:471`): *"Maximum value is 64 for now (should be enough for all
  retail talent trees) ... maximum limit of generated combinations is 500 million, this
  requires a system with at least 16 GB of RAM."*
- A separate, adjacent warning (`LoadoutSolverWindow.cpp:479-484`) confirms the
  class/spec-tree-only, no-combined-tree scope: *"The solver does not yet support classic
  talent trees (3 subtrees in one big class tree with independent point requirements)!"* —
  the shipped app was already scoped to single-DAG solves, one class **or** one spec tree at
  a time, never a merged multi-tree structure.

So the shipped app "solved real spec trees" not because the 64-bit ceiling was secretly
irrelevant, but because (a) real DF trees top out at 60 bits, and (b) the GUI actively
refuses to attempt anything over 64 before ever reaching the engine, converting the raw
`std::logic_error` crash into a friendly popup.

### Practical conclusion for the web rewrite

Given real shipped Dragonflight data, the practical ceiling is comfortable, not marginal: the
worst observed tree needs 60 of 64 bits, and the engine's own architecture (one `TalentTree`
= one independent DAG solve, class and spec never merged) is exactly what keeps every
individual solve well inside the limit — this is a load-bearing design decision, not
incidental. For TWW, hero talent trees are a *third*, small, independently-chosen pool
(typically ~10-11 nodes, mostly 1-point, roughly 15-20 total points) layered alongside the
class and spec trees in Blizzard's own UI, not merged into either; solving it as its own
third `TalentTree` (matching the existing per-tree-independence model) keeps every individual
solve far under 64 bits and **requires no widening of `SIND` at all**. Widening would only
become necessary if the web rewrite chooses a different product shape than today's — e.g. a
single combined solve across class+spec+hero as one DAG (not something Blizzard's own point
economy requires, and not something the current engine or GUI ever attempted) — in which case
totals could plausibly exceed 64 (60 + ~15-20 hero bits ≈ 75-80). If that combined-solve
feature is ever wanted, widening is a contained, mechanical change, not a rewrite of the
algorithm: `SIND` is a single typedef (`TTMEnginePresets.h:13`) referenced ~94 times across 7
files (`TreeSolver.h`/`.cpp`, `CLI.h`/`.cpp`, `TTMEnginePresets.h`,
`LoadoutSolverWindow.cpp`, `TalentTreeManagerDefinitions.h`), but only a handful of call
sites actually assume 64-bit width and would need real edits: the three raw shift literals
`1ULL << ...` (`TreeSolver.cpp:844,1044`; `CLI.cpp:361`), one hardcoded loop bound
`for (int i = 0; i < 64; i++)` in `fillOutTreeWithBinaryIndexToString`
(`TreeSolver.cpp:852`), and the handful of places that serialize a `SIND` as plain decimal —
`outFile << comb` in the CLI's result file (`CLI.cpp:385`) and `std::to_string(skillsetIndex)`
in the GUI (`LoadoutSolverWindow.cpp:1024,1118`) — none of which have a built-in
`operator<<`/`std::to_string` for `unsigned __int128` (GCC/Clang-only anyway, not portable to
MSVC) or `std::bitset` (no native decimal formatting). The pragmatic approach if/when this is
needed: introduce a small custom fixed-width (128-bit) integer wrapper type with the handful
of operators actually used (`&`, `|`, `~`, `==`, shift-by-runtime-`int`, and a decimal
`toString`), swap the `SIND` typedef to it, and fix the ~5 call sites above — a half-day,
low-risk change, not a redesign, and not something the current data justifies doing yet.
