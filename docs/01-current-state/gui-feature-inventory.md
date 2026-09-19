# WoW Talent Tree Manager — Dear ImGui GUI Feature Inventory

Source analyzed: `GUI/src/*.cpp/.h` (excluding vendored libs under `GUI/src/libs/`). This is the exhaustive baseline of what the native C++/ImGui/DirectX11 desktop app does today, to guide feature parity for the web rewrite. Every non-obvious interaction below is cited `file:line` so the rewrite can verify behavior against the original rather than against this summary alone.

## App shell & navigation

Win32 + DirectX11 + Dear ImGui (docking branch) app. Entry point `Main.cpp:59` (`WinMain`). Window class "ImGui Example", title "Talent Tree Manager", initial size 1600x900 (`Main.cpp:85,88`).

**Startup sequence** (`Main.cpp:59-198`):
- Deletes a leftover `AppUpdaterTemp.exe` if present (`Main.cpp:69-78`), `curl_global_init` (`:81`).
- Loads saved `WINDOWPLACEMENT` and applies it to the window **before** the D3D device/ImGui context exist (`:90-91`).
- `ImGuiConfigFlags_DockingEnable` turned on, keyboard/gamepad nav left disabled (commented out) (`:109-111`).
- Fonts: embedded compressed Roboto Medium TTF baked at 5 base sizes × 4 offsets = 20 texture-atlas variants (`:134-140`), selected later via `Presets::PUSH_FONT` per the user's chosen logical size.
- Renders a one-off splash frame with the app banner and the text "Initialize workspace (can take a while on first open) and load resources..." (`:161`) while `TTM::initWorkspace()` copies bundled resources into `%APPDATA%`, `TTM::refreshIconMap`/`loadWorkspace`/`loadActiveIcons` run (`:190-198`).
- `io.IniFilename`/`io.LogFilename` are redirected to `%APPDATA%/imgui.ini` and `imgui_log.txt` (`:185-188`).

**Main loop** (`Main.cpp:200-259`):
- Pumps Win32 messages; on the custom `WM_SAVEBEFOREDESTROY` message it saves the workspace, calls `stopAllSolvers`, then **blocks the message loop** polling `updateSolverStatus` every 200ms until every background solver thread (`uiData.currentSolvers`) has actually exited, before finally posting the real `WM_CLOSE` (`:210-218`) — a native-only graceful-shutdown pattern; a web rewrite has no equivalent "block navigation until background job threads exit" step since solving will live server-side.
- Autosave check every frame: if `now - lastSaveTime > autoSaveInterval` (default 300s), calls `saveWorkspace` (`:236-239`).
- Renders either the full-screen `RenderUpdateWindow` overlay or the normal `RenderMainWindow`, gated on `uiData.updateStatus` (`:241-246`).

**`WndProc`** (`Main.cpp:336-363`): `WM_SIZE` recreates the D3D render target on resize (`:343-349`); `WM_SYSCOMMAND` swallows `SC_KEYMENU` to disable the Alt-activated native menu (`:351-354`); `WM_CLOSE` does **not** close the window directly — it posts the custom `WM_SAVEBEFOREDESTROY` message instead, which is what triggers the save-then-block-then-close sequence above (`:358-360`).

**Menu bar** (`RenderMenuBar`, `TalentTreeManager.cpp:201-471`):
- **File**: MenuItem "Save" (Ctrl+S label) (`:204-206`) and MenuItem "Close" (saves then quits) (`:207-210`); a global Ctrl+S key check runs every frame independent of the menu being open (`:213-215`).
- **Styles**: MenuItems "Company Grey" (`:217-220`), "Path of Talent Tree" (`:221-224`), "Light Mode" (`:225-228`) each call `Presets::SET_GUI_STYLE`; separator; checkbox-style MenuItem "Show icon glow" bound to `uiData.enableGlow` (`:230`); separator; five manually-implemented "radio" MenuItems — "Font mini"/"Font small"/"Font default"/"Font large"/"Font huge" (`:232-256`), each comparing/writing `uiData.fontsize`.
- **Help**: MenuItem "Controls & Tips" → `showHelpPopup=true` (`:260-262`); "About TTM" → `showAboutPopup=true` (`:263-265`); "Visit Github" → `ShellExecute` opens `https://github.com/TobiasM95/WoW-Talent-Tree-Manager` in the OS default browser (`:266-268`); "Check Updates" → resets `updateStatus`/`renderedOnce` to force a re-check (`:269-272`); "Show Changelog" → `showChangelogPopup=true` (`:273-275`); separator; "Reset TTM" → `showResetPopup=true` (`:277-279`); "Reset resources" → `updateStatus=RESETRESOURCES` (`:280-282`).
- If `menuBarUpdateLabel` is set and contains the literal substring "New TTM update found", the label itself becomes a clickable MenuItem "> Press here to update <" that runs `ShellExecuteW(..., L"runas", L".\AppUpdater.exe", ...)` — an **elevated, separate updater executable**, invoked directly from the menu bar (`:288-297`); otherwise the label is just static colored text (`:300`).

**Popups triggered from the menu bar** (all `BeginPopupModal`, `AlwaysAutoResize`, OK-button-closes unless noted):
- **"About##Popup"** (`:322-335`): TTM version string, feedback contact "BuffMePls#2973 (Discord)", GitHub link, and credits for Dear ImGui / libcurl / stb / miniz.
- **"Controls & Tips##Popup"** (`:336-423`): the app's only in-app documentation — one long scrollable text with per-view sections (Tree editor, Loadout editor, Loadout solver, Sim analysis) and a Tips section (SimC workflow, custom icons instructions); auto-scrolls to top on first open.
- **"Changelog##Popup"** (`:424-452`): reads `changelog.txt` next to the executable via `loadChangeLogData` (`:1284-1316`, note: **working directory**, not `%APPDATA%`); blank lines render as a spacing+separator; if the file is missing/empty/unreadable it shows "Could not load changelog data. See github releases page for changelog information." instead of erroring.
- **"Reset TTM##Popup"** (`:453-470`): "Do you want to reset TTM? This will close the app, delete the workspace (all trees and loadouts) and settings!"; Button "Reset" calls `resetWorkspaceAndTrees()` (a full recursive delete of the `%APPDATA%` folder, `:1252-1255`, with **no further confirmation at that layer**) and sets `done=true`; Button "Cancel" closes the popup.
- **Update overlay** (`RenderUpdateWindow`, `:43-191`, replaces the entire app while active): states are NOTCHECKED ("Looking for updates..."), UPDATEERROR (sets a persistent menu-bar warning label instead of a popup), OUTDATED (boxed prompt "New update was detected. Do you want to update? (This includes presets, images, etc.)" plus the live `updateMessage` text, Checkbox "Update current workspace##updateWindowWorkspace" with a warning that updating "could invalidate and remove some of your loadouts", Button "Update" / Button "Ignore"), RESETRESOURCES (same layout, prompt "Do you want to reset your resources?...", Button "Update" / Button "Cancel"), UPDATEINITIATED (shows "Updating..." for one frame), UPDATEINPROGRESS (calls `updateResources`, reloads icons, and if a skillset was active, reactivates it so the UI doesn't lose selection, `:180-187`).

**Tree tab bar** (`RenderTalentTreeTabs`, `:498-847`) — multi-document interface, one tab per open tree:
- Flags: `AutoSelectNewTabs | Reorderable | FittingPolicyScroll | TabListPopupButton` (`:499-502`) — tabs are drag-reorderable and get an overflow "list" popup button when too many are open.
- Trailing `TabItemButton` **"+"** opens popup "Create new tree" (`:506-508`); trailing `TabItemButton` **"X"** opens popup "Close all confirmation" (`:509-511`).
- **"Create new tree" modal** (`:517-717`), five mutually exclusive creation paths stacked with "or" dividers:
  1. Button **"New custom tree"** (`:519-533`) — blank tree via `Engine::loadTreePreset("custom")`.
  2. Combo class (`:540`, resets spec combo to 0 on class change `:541-543`) + Combo spec (`:544-553`, clamped to the class's spec count) + Button **"Load preset"** (`:555-569`).
  3. Combo of previously-saved custom tree files (lazily refreshed via `updateCustomTreeFileList`, `:571-597`) + Button **"Load custom tree"** (`:599-623`) — on failure closes this popup and opens **"Tree load error"** (`:736-743`, "This custom tree was unable to be loaded. It might be corrupted or out of date.").
  4. Label "TTM tree import string:" + `InputText` (auto-select-all) + Button **"Import tree##popup"** (`:627-649`) — on invalid format, the error is written **into the input field itself** ("Invalid import string!") rather than a separate popup/toast.
  5. Label "In-game import string:" + `InputText` + Checkbox **"Import class tree"** + Checkbox **"Import spec tree"** + label "Tree name:" + `InputText` (letters-only filter) + Button **"Import tree##blizzpopup"** (`:650-716`) — requires at least one checkbox checked; internally creates **both** a class-tree and spec-tree copy temporarily (named "`<name>` class"/"`<name>` spec"), decodes the Blizzard hash into the spec tree using the class tree as a complementary reference (`Engine::importBlizzardHash(activeTree, compTreePtr, hash, true)`), then deletes whichever of the two the user didn't check; on failure both temporary trees are erased, the previous active tree index is restored, and the field text is overwritten with "Invalid import string!".
- **"Close all confirmation" modal** (`:719-735`): "Close all tabs?" → Button "Yes" clears every open tree; Button "Cancel".
- Each tab is colored per WoW class via `Presets::SET_TAB_ITEM_COLOR`; the active tab's label is prefixed "> "; a custom `BeginTabItemNoClose` is used (no built-in ImGui close-X) because closing needs a confirmation step.
- Hovering + left-click on a **non-active** tab switches trees, clearing textboxes, resetting complementary-tree indices, deselecting the current talent, and reloading icons (`:768-779`).
- Clicking a tab's own close indicator sets `deleteTreeIndex`; **holding Shift** deletes immediately with no confirmation (`:783-794`); otherwise falls through to the **"Delete tree confirmation" modal** (`:817-844`, text literally says "(Hold Shift to skip this warning)") with Button "Yes"/"Cancel".
- A `treeSwitchCD` cooldown flag guards against ImGui's own tab-selection change and the app's click-handler both trying to switch the active tree in the same frame (`:802-816`).

**Sub-view tab bar** (`RenderTreeViewTabs`, `:849-939`) — second-level tabs per open tree, in this fixed order: **Talent Loadout Editor**, **Talent Loadout Solver**, **Sim Analysis**, **Talent Tree Editor**. Active tab label prefixed "> ". A `uiData.editorViewTarget` field lets other code (e.g. clicking a Sim Analysis ranking row) programmatically force-select a tab, not just direct clicks. Selecting the Loadout Editor tab re-validates the loadout and reactivates the (clamped) active skillset, or sets `activeSkillsetIndex=-1` if the loadout is empty (`:869-883`); selecting any of the other three tabs just flags `isLoadoutInitValidated=false` so re-entering Loadout Editor later re-validates again.

**Docking layout** (`SubmitDockSpace`, `:941-969`): a fixed programmatic dock layout is built exactly once via `ImGui::DockBuilder*` — splits the workspace Right by `uiData.dockWindowRatio` (default 0.40, clamped ≤1.0) into `dock_id_right`/`dock_id_left`, then splits the left pane Down by a fixed 0.045 into a thin bottom strip. Docks window **"TreeWindow"** into the left pane, **"SettingsWindow"** into the right pane, **"SearchWindow"** into the bottom-left strip. All four sub-views render into these same three window IDs, and the split ratio is user-draggable at runtime and persisted (see Settings section).

**Status bar** (`RenderStatusBar`, `:981-1011`): colored per active tree's class; text = "Currently active tree: `<name or 'none'>`" + " - node count: N" + " - maximum skill points: N", and — only while the Loadout Editor tab is active and a skillset is selected — " - required level: N", with an additional " - REQUIRED LEVEL IS GREATER THAN CURRENT MAX LEVEL (70)" warning appended if the computed required level exceeds 70 (`:987-1003`).

**Persisted cross-view state (App shell level)**: `uiData.talentSearchString`/`searchedTalents` (the "SearchWindow" filter) is a single shared field rendered identically in all four sub-views, so a search typed in one view stays active and highlighted when switching to another. `uiData.editorViewTarget` allows one view to force-navigate to another (used by Sim Analysis ranking rows to jump to Loadout Editor). Background solver threads (`uiData.currentSolvers`/`solvedTrees`) keep running regardless of which sub-view or tree tab is currently visible. `uiData.treeEditorSelectedTalent` is cleared to `nullptr` on every tree-tab switch, tree creation, and tree import path.

## View: Talent Tree Editor (`TalentTreeEditorWindow.cpp/.h`)

**Purpose**: create/edit the talent tree graph itself (nodes, connections, metadata) — the "authoring" tool for trees, as opposed to picking points in one.

**Sub-pages** (buttons in the right "SettingsWindow" panel, `TreeEditPage` enum): **Tree Information**, **Tree Editor**, **Save/Load Trees**. Switching to Save/Load clears the import/export string buffers (`:193-195`).

### Tree Information page (`:200-237`)
- `InputText` tree name, letters-only filter (`:203-204`).
- Read-only preset-name display (`:206-208`).
- Combo **tree type**: "Class tree"/"Spec tree" (`:210-224`) — changing it resets `presetName` to "custom", re-runs node-count/order/requirement-separator recompute, re-validates the loadout, and clears any in-progress solver/sim-analysis state for this tree.
- Read-only node count / max talent points (`:226-232`).
- Multiline tree description textbox (`:234-236`).

### Tree Editor page → "Create Talent" header (`:243-415`)
- Combo **talent type**: Active/Passive/Switch (`:247-248`).
- `InputText` **Name** (`:251-252`); second **Name (switch)** field, disabled unless type = Switch (`:255-259`).
- Icon-name combo with a live text-filter box + **"Clear"** button (`:261-330`); a second, independent icon combo+filter+preview for the switch variant's icon.
- Slider **Max points** 1-9 (disabled/forced to 1 for Active/Switch types) (`:332-338`).
- Slider **Points required** 0-50 (`:340-341`).
- Checkbox **"Is talent pre-filled"** (`:343-344`).
- Per-rank multiline description boxes, dynamically resized to `maxPoints` (+1 extra box for the switch variant) (`:346-353`).
- Slider **Row** (1..`maxRowLimit`), Slider **Column** (1..`maxColumnLimit`) (`:355-359`).
- Slider **Parent count** / **Children count** (0..`nodeCount`), each driving a dynamically-sized array of Combo pickers listing every existing talent as "`index`: `name`" (`:361-408`).
- Button **"Create Talent"** (`:410-414`, validated in `validateAndInsertTalent`, `:1391-1482`):
  - Rejects if the (row, column) cell is already occupied → popup **"Talent spot occupied##creation"**.
  - Rejects if the same talent index appears in both the parent and child pick lists → popup **"Cycle detected"**.
  - Enforces "a pre-filled talent must be a root or have a pre-filled parent" → popup **"Pre filled talent error"**.
  - Runs a full graph cycle check (`checkIfTalentInsertsCycle`) → popup **"Cycle detected"**.
  - On success: wires parent/child links both directions, updates the root list, assigns `index = tree.maxID`, forces `presetName = "custom"`, recomputes node count/ordered list/requirement separators, re-validates the loadout, clears solver/sim-analysis caches, and resets the creation form fields.

### "Edit/Delete Talent" header (`:419-675`)
Same field set as Create, bound to `treeEditorSelectedTalent`; shows "Select talent to start editing..." if nothing is selected.
- Button **"Reset talent"** (`:588-610`) — discards in-progress edits, reloading the talent's currently-saved values.
- Button **"Update talent"** (`:612-616`, validated in `validateTalentUpdate`, `:1484-1620`): re-checks position occupancy (excluding the talent's own current cell) → **"Talent spot occupied##update"**; cycle-in-combo-selection check → **"Cycle detected"**; pre-filled validity **both directions** — becoming pre-filled requires a pre-filled parent, and **leaving** pre-filled state is blocked if any dependent child is still pre-filled (since that child would become unreachable) → **"Pre filled talent error"**; a full cycle check via a temporary relink-and-restore against `checkIfTreeHasCycle` → **"Cycle detected"**. On success, commits the relink, replaces the entry in `orderedTalents`, and rebuilds `talentRoots`.
- Button **"Delete talent"** (`:618-666`) — also bound to **Ctrl+Delete** while a talent is selected. Removes the node, repairs every child's parent list and every parent's child list (orphaned children are promoted to roots), deletes it from `orderedTalents`, and recalculates all derived tree metadata.
- Button **"Clear selection"** (`:668-670`) — deselects without any data changes.

### "Misc." header (`:683-980`) — bulk tree-authoring tools
- Sliders "Shift rows by" / "Shift columns by" (bounded to ±`maxRowLimit-1`/`maxColumnLimit-1`) (`:687-690`) + Button **"Shift talents"** (`:691-732`) — computes the tightest legal offset across every talent and, if the requested shift exceeds it, opens popup **"Shift value exceeds bounds"** showing the computed min/max; otherwise applies the offset to every node and updates `maxCol`.
- Three sliders (Active/Passive/Switch counts, 0-40) (`:736-741`) + Button **"Insert talents"** (`:742-821`) — bulk-creates blank talents. Guards the total against `maxRowLimit*maxColumnLimit/4` (≈400 nodes, per the popup text at `:1038`) → popup **"Too many talents inserted"**; otherwise auto-places blanks into free grid cells via a snake/zig-zag algorithm starting at (1,1), incrementing column by 2 then wrapping the row by 2, splitting the grid into left/right halves; every new blank becomes a root.
- Button **"Cleanup tree (deletes loadout!)"** (`:823-837`) — runs `reindexTree` + `autoShiftTreeToCorner` + `autoPointRequirements` as one combined action; the button label itself is the only warning that it destroys existing loadouts.
- (Disabled/commented-out) "Auto position talents in tree" — a full automatic no-crossing-connections layout algorithm exists in source (`:839-854`) but is inactive, documented in-app as "beta ... unstable" in the Controls & Tips help text.
- Button **"Double talent positions"** (`:855-900`) — sorts talents bottom-right-first, doubles every row/column (clamped to the grid limits), skipping any move that would land on an already-occupied cell.
- Button **"Sort talent indices (deletes loadout!)"** (`:901-916`) — `reindexTree` only; again destroys the loadout, warned only in the label text.
- Button **"Auto set point requirements"** (`:917-929`) — `autoPointRequirements`.
- Button **"Auto shift tree to corner"** (`:930-943`) — `autoShiftTreeToCorner`.
- Button **"Auto insert icon names"** (`:944-962`) — `autoInsertIconNames`, name-matches every talent against all loaded icon names.
- Button **"Remove all connections"** (`:963-979`) — clears every talent's parent/child lists and makes every talent a root.
- Every Misc action marks the tree `presetName = "custom"`, refreshes node-count/ordered-list/requirement-separator caches, re-validates the loadout, and clears solver/sim-analysis state for this tree.

### "Save/Load Trees" page (`:1045-1371`)
- **"Load preset" header**: Class combo (`:1049-1054`, resets spec on class change) + Specialization combo (`:1055-1065`) + Checkbox **"Try to keep skillsets"** (`:1066`) + Button **"Load"** (`:1067-1069`) → opens **"Load preset confirmation" modal** (`:1223-1246`) which on confirm either `restorePreset` (keeps the existing loadout) or `loadTreePreset` (discards it), then re-validates and reloads icons.
- **"Custom trees" header**: list box of saved custom tree files (`:1071-1090`, lazily refreshed) + Buttons **"Save tree"** (`:1092-1111`), **"Load tree"** (`:1113-1128`), **"Delete tree"** (`:1130-1145`), **"Refresh list"** (`:1147-1149`). Each of Save/Load/Delete first re-diffs the on-disk file list and aborts with popup **"File list changed"** if it's gone stale since last read; Save on a name collision shows **"Overwrite custom tree?"** confirm; Load/Delete each have their own confirmation modal; any I/O failure shows **"Tree save/load/delete error"**.
- **"Import/Export" header** (`:1151-1219`): TTM string **Import** (`InputText` + Button "Import") — validates via `Engine::validateAndRepairTreeStringFormat`/`parseTree`, popup **"Invalid import tree string"** on failure or **"Tree import successful"** on success; TTM string **Export** (read-only, auto-select-all field + Button "Export") via `createTreeStringRepresentation`; **Pastebin export** (read-only field + Button "Export") with a client-side 1-minute cooldown (`PASTEBIN_EXPORT_COOLDOWN`) and a live countdown shown in place of the button while active; **Readable export** (read-only field + Button) via `createReadableTreeString`, a human-readable text dump; **"To clipboard"** button captures the TreeWindow canvas region to the Windows clipboard as a bitmap via raw GDI calls (`createScreenshotToClipboard`, `:2144-2227`) — Windows-only, no browser equivalent (a web port needs Canvas `toBlob`/download or the async Clipboard API, both of which require an explicit user gesture rather than the native one-click flow).
- Note: Blizzard talent-hash import/export for a **tree** is not exposed here — only the field-clearing reference (`treeEditorImportBlizzHashString = ""`, `:195`) lives in this file; the actual Blizzard hash UI lives in the Loadout Editor and the tree-creation popup in `TalentTreeManager.cpp`.

### Canvas interaction (`TreeWindow`, `:127-180`, `:1771-2027`)
- **Pan**: left-click-drag on empty canvas space sets scroll via `SetScrollX/Y` from `MouseDelta` (`:136-139`); disabled while Ctrl is held, since Ctrl is reserved for zoom (`:128-132`).
- **Zoom**: Ctrl+mouse-wheel while hovered, `treeEditorZoomFactor += 0.2 * wheel`, clamped **[0.5, 3.0]** (`:141-152`); re-centers on the cursor position via a scroll-remap using relative world-mouse-position math (`:2006-2026`).
- **Left-click** a node → `selectTalent` (`:1890`, `:2029-2055`): copies it into `treeEditorSelectedTalent`, switches the settings panel to the Tree Editor page, opens the Edit/Delete header, rebuilds the parent/child combo placeholders.
- **Alt+click** another node while one is already selected → `:1850-1887`: makes the clicked node the **child** of the selected node; if the link already exists it is removed instead (toggle); guarded by `checkIfTreeHasCycle`, reverting + popup **"Cycle detected"** on failure.
- **Shift+Alt+click** → same as above but makes the clicked node the **parent** instead.
- **Left-click-drag a node** → `repositionTalent` (`:2057-2141`): movement snaps to the grid based on `treeEditorBaseTalentHalfSpacing`/zoom, clamped to the row/column limits; dropping onto an occupied cell **displaces the occupant**, which is buffered in `treeEditorTempReplacedTalents` and restored to its original cell once that cell becomes free again (multi-hop swap chains are possible).
- **Hover** (no Alt held) → tooltip (`AttachTalentEditTooltip`, `:33-122`): icon(s), id/position, type, pre-filled flag, max points/points required, per-rank descriptions.
- **Shift held** (no Alt/Ctrl) while hovering the canvas → renders a name-label box over **every** talent simultaneously (`:1977-1995`).
- **Ctrl+Delete** with a node selected → deletes it (same as the "Delete talent" button).
- Right-click on a node is **not used** in this view.
- Selected/searched nodes get a glow overlay: red = selected, blue = search match (`:1944-1961`); connections are drawn as arrows per parent→child edge (`:1793-1807`); horizontal point-requirement separator lines with an "`N` points" label are drawn at each distinct `pointsRequired` breakpoint (`:1808-1820`), mimicking the in-game tree look. The tree auto-centers horizontally if it's narrower than the viewport (`:1781-1783`).

### Validation & error behavior
Nearly every mutating action funnels through the same recompute pipeline: `updateNodeCountAndMaxTalentPointsAndMaxID` → `updateOrderedTalentList` → `updateRequirementSeparatorInfo` → `validateLoadout(tree, true)` → `clearSolvingProcess`/`clearSimAnalysisProcess`, and forces `presetName = "custom"`. Row/column sliders' max bound is dynamic per tree (`maxRowLimit`/`maxColumnLimit`), not a global constant. Cycle detection is checked **twice** on create/update — first a cheap combo-index-overlap shortcut, then a full graph traversal — before the change is committed.

### Persisted/shared state
`treeEditorSelectedTalent`, the creation-form staging fields, and the icon filter strings are local to this view and reset on tree-tab switch. The tree data itself (`orderedTalents`, `talentRoots`, metadata) is the single shared source of truth read/written by all four sub-views for the active tree. The custom-tree file-list cache (`treeEditorCustomTreeFileList`) is lazily invalidated, not re-scanned every frame.

### Performance note
Rendering is fully immediate-mode: every frame iterates all `orderedTalents` twice (arrows, then icons/buttons), plus a third full pass when Shift is held for name-label overlays (`:1793-1995`) — O(nodes+edges) per frame, fine at native frame rates for realistic tree sizes but a naive DOM/canvas port doing per-node reflow could bottleneck on very large trees or many simultaneously open tabs; recommend a single canvas/WebGL draw pass or virtualization.

## View: Talent Loadout Editor (`LoadoutEditorWindow.cpp/.h`)

**Purpose**: pick actual talent-point allocations ("skillsets") on a tree, like spending points in-game; manage multiple named skillsets per tree.

Right-panel top toggle buttons **"Loadout Information"** / **"Import/Export Skillsets"** switch `uiData.loadoutEditPage` (`:201-207`).

### Loadout Information page (`:217-309`)
- `InputText` skillset name (`:217`).
- Slider **"Set level cap"** 11-70 (`:220`) + Checkbox **"Activate level cap"** (`:228`) — automatically forced off if the skillset's computed required level already exceeds the chosen cap (`:221-231`).
- Read-only text: points spent, required level, with an over-70 warning (`:232-236`).
- Button **"Reset skillset"** (`:238`) — zeroes all points, `assignedSkillPoints`, and `talentSwitch` choices for the active skillset (no confirmation popup).
- `ListBox` **"Skillsets:"** (`:257`) — one `Selectable` row per skillset; clicking switches the active skillset via `Engine::activateSkillset` (`:262-265`); merely **hovering** a row sets `hoveredEditorSkillset` (a copy) so the canvas live-previews it without committing the switch (`:271-273`).
- Button **"Add skillset"** (`:277`) — creates + activates a new blank skillset.
- Button **"Copy skillset"** (`:282`) — duplicates the active skillset.
- Button **"Delete skillset"** (`:288`) — no confirmation popup for a single delete.
- Button **"Delete all skillsets"** (`:303`) — clears the whole loadout, `activeSkillsetIndex = -1`.
- `InputTextMultiline` **"Loadout description"** (`:309`, 500px tall) — bound to `tree.loadoutDescription`, a field distinct from the tree's own description (set in the Tree Editor).

### Import/Export Skillsets page — three collapsible headers (`:313-490`)
- **"Talent Tree Manager imports/exports"** (`:313-345`): `InputText` + Button **"Import"** for a TTM skillset string → `Engine::importSkillsets`, result reported in modal **"Import skillsets result"** (`:315-323`, `:494-506`), which reports counts imported/discarded and auto-switches back to the Loadout Information page if anything succeeded; read-only auto-select fields + Button **"Export"** for the active skillset and for all skillsets (`:324-339`); Button **"To clipboard"** screenshots the TreeWindow canvas (`:342-344`, same GDI mechanism as the Tree Editor).
- **"In-game Skillsets imports/exports"** (`:347-457`): Combo **"Complementary tree/skillset"** (`:353-398`) listing other open trees of the same class/opposite tree-type, with per-skillset `Selectable` rows; hovering an entry sets `hoveredBlizzHashCombo` for a canvas preview (`:385-390`); Checkbox whose label depends on tree type — **"Import spec tree into selected tree too"** or **"Import class tree into selected tree too"** (`:401-406`); `InputText` + Button **"Import"** for a real in-game Blizzard talent-loadout hash (`:407-435`, validated via `verifyTreeIDWithBlizzHash`, applied via `Engine::importBlizzardHash`, result in modal **"Import ingame skillset result"**, `:507-523`, whose message text varies depending on the checkbox state); read-only field + Button **"Export"** for a Blizzard hash of the active skillset (`:438-457`, `Engine::exportBlizzardHash`).
- **"SimulationCraft exports"** (`:460-490`): Checkbox **"Create profileset"** (`:463`) + Button **"Export"** the active skillset to SimC syntax (`:462-472`); Button **"Export"** all skillsets to SimC (`:473-480`); Button to create a "single talent comparison" profileset export (`:486-490`).

### Search window (`:524-539`)
`InputText` "Search:" filters `talentSearchString` → `searchedTalents` via `Engine::filterTalentSearch` (shared with all other views); also accepts the keywords "active"/"passive"/"switch" (`:535`).

### Canvas interaction (`placeLoadoutEditorTreeElements`, `:540-915`)
- Empty loadout state: a centered Button **"Create first skillset"** (`:545`).
- **Pan**: left-click-drag on empty canvas (`:166-169`). **Zoom**: Ctrl+scroll, clamped **0.5–3.0×** (`:171-183`).
- **Left-click** a talent → **spend 1 point**, only if: its parent is filled, its point-requirement is met, it isn't already maxed, and the level cap (if active) allows it (`:705-723`).
- **Ctrl+click** a switch-type talent → toggles which of its two variants is chosen (1↔2) **without** spending a point unless it already has points assigned (`:697-704`).
- **Right-click** (press+release on the same talent) → **remove 1 point**; see validation behavior below (`:823-851`).
- **Middle-click** (press+release) → toggles the switch-talent variant, functionally identical to Ctrl+click even at 0 points (`:826, 852-859`).
- **Shift held** (no Ctrl) while hovering → reveals name-label overlays over every talent (`:865-883`).
- Hover → `AttachLoadoutEditTooltip` (`:26-153`, suppressed while Alt is held): name/icon/id/position/type/pre-filled flag/points/description; switch talents additionally show both variant names with the hint "(switch, ctrl+click: `<other name>`)".
- Visual states: gold glow = maxed, green = partially filled, blue = search match (`:772-804`); unassigned switch talents render as a split icon showing both options side-by-side (`:744-757`).
- **Pre-filled/pre-selected talents** auto-fill on render (`:649-657`) and are **always** locked/non-interactive (grayed out, cannot be clicked at all) — they represent baseline points the player gets for free and cannot be removed through this UI.
- Talents whose prerequisites aren't met (points-required unmet, parent not maxed, or the level cap makes them unreachable) render disabled/grayed (`:634-648`).

### Validation & error behavior — the point-removal auto-revert
Right-click removal is implemented as **speculative apply then whole-tree recheck**, not a pre-computed legality check: the point is removed first, then `Engine::checkTalentValidity` re-validates the entire tree's assigned points; if the removal broke validity (e.g. a still-point-having descendant now has an unmet parent/points-required condition), the removal is **silently reverted** — the point is put back with **no popup, toast, or explanation shown to the user** (`:823-851`). This is the closest thing to "undo" anywhere in the loadout editor: there is no explicit undo/redo stack. A rewrite should almost certainly surface *why* a removal was blocked, since the native app gives no feedback beyond "nothing visibly happened."

The level cap only prevents **assigning new** points once the computed required level would exceed the cap — it does not retroactively strip already-assigned points if the cap is lowered afterward.

### Persisted/shared state
`talentTreeCollection.activeTree().loadout` (the skillset list) and `activeSkillsetIndex` are read/written by this view and by the Loadout Solver (which appends generated skillsets into the same list) and by Sim Analysis (which activates a skillset from a ranking row via `Engine::activateSkillset` and force-navigates here). `uiData.hoveredEditorSkillset`/`hoveredBlizzHashCombo` are transient, view-local hover-preview state that does not persist across a view switch.

## View: Talent Loadout Solver (`LoadoutSolverWindow.cpp/.h`)

**Purpose**: exhaustively enumerate every valid talent-point combination for a tree and let the user filter/browse them by constraints, then promote selected results into the loadout. *(Forward note: the web rewrite keeps this exact C++ enumeration engine and runs it server-side as a CLI job invoked from a backend queue — see the parity checklist. The behavior documented below is the native, in-process implementation that the server-side worker will reuse as-is.)*

Runs per tree (`talentTreeCollection.activeTreeData()`), only "active" (showing the filter UI) once `isTreeSolveProcessed` (`:196`); otherwise the canvas shows the setup/progress screen.

### Setup phase (canvas, `:407-557`, shown before a solve starts)
- Table "loadoutSolverStatusTable" listing every in-progress/solved solver across **all** open trees, with per-row Button **"cancel"**/**"reset"** (`:426-464`; duplicated in the settings-panel status tab at `:337-376` and `:571-609`).
- Red warning text that class/custom-type trees are very RAM-intensive, and that classic 3-subtree trees are unsupported (`:479-484`).
- Slider **"Talent points limit:"**, 1..`loadoutSolverMaxTalentPoints`, drag-only (`ImGuiSliderFlags_NoInput`) (`:491`).
- Checkbox **"Solve only for max points"** (`onlyLimitSolve`) (`:494`).
- Button **"Process tree (max 3 solvers)"** (`:510`), disabled once 3 solves are already running app-wide (`maxConcurrentSolvers`). On click, resets solve state, clamps the point limit to the tree's actual max spendable points, and runs the tree-size guard: if the tree's spendable points **or** the requested limit exceeds 64, or the resulting combination count would exceed the safety threshold, it opens popup **"Talent tree too large"** and refuses to start (`LoadoutSolverWindow.cpp:515-525`, threshold driven by `loadoutSolverMaxTalentPoints`) — **this exact pre-flight check must be re-implemented as a server-side validation gate before a job is dispatched**, since a browser-side check alone cannot be trusted. Otherwise spawns `Engine::countConfigurationsSingle` or `...Parallel` on a **detached `std::thread`**, fully asynchronous to the render loop (`:533-550`).
- While solving (`:558-624`): live status table + centered "Processing..." text, Button **"Cancel"** (this tree) and Button **"Cancel all solves##loadoutSolverCancelAllButton"** — both are **cooperative** cancellation (a flag the worker thread polls, `safetyGuardTriggered`), not a hard thread kill.

### Settings panel once solved (`isTreeSolveProcessed`, `:196-404`)
Two toggle buttons: **"Solution Filter"** / **"Tree Solve Status"** (`:198-204`).

**Solution Filter tab** (`:207-331`):
- Red warning if `safetyGuardTriggered` (hit the safety cap or was cancelled).
- Summary text: total combination count, points range, and "Processing took %.3f seconds" (`:220`).
- Button **"Reset solutions"** (`:221`) → `clearSolvingProcess`, no confirmation.
- "(?)" tooltip explaining the color code: green/yellow = minimum points spent, red = excluded, blue = "at least one of group", purple = "exactly one maxed" (`:224-227`).
- Checkbox **"Auto apply filter"** (`:229`) — when on, the manual Filter button is disabled and every canvas click re-filters immediately.
- Button **"Filter"** (`:241`, disabled when auto-apply is on) → `Engine::filterSolvedSkillsets` against the current constraint set, resets pagination.
- Button **"Clear filter"** (`:252`) — zeroes every constraint, then re-filters.
- Once filtered: Checkbox to restrict to an exact point total (`:266`) + Combo 1..limit (`:273-286`, hidden when `onlyLimitSolve` is set) + `ListBox` of point-totals showing "`N` (`count`)" combination counts per total (`:291-326`) — selecting one sets `loadoutSolverTalentPointSelection` and resets the page.
- **Filtered-skillset browser** (`displayFilteredSkillsetSelector`, `:910-1130`), shown once a point-total is selected:
  - `ListBox` "Filtered skillsets: (hover to preview)" showing "Id: `<SIND>`" per row (`:919-940`); hovering renders the full skillset preview on the canvas.
  - Pagination: Buttons **"<<" / "<" / "N/M" (page indicator) / ">" / ">>"** (`:944-977`); pages are materialized **lazily** — only the current page's IDs are decoded into `loadoutSolverPageResults`, avoiding ever holding the full result list expanded (`:1132-1152`).
  - Per-switch-talent Slider(1,2, drag-only) **"Switch talent choices:"** (`:982-990`) — globally selects which variant is used when materializing skillsets from raw indices (a corresponding checkbox exists in source but is commented out, `:987`).
  - `InputText` **"Prefix:"** for generated skillset names (`:995`).
  - Button **"Add selected to loadout"** (`:996`) — decodes the selected index via `Engine::skillsetIndexToSkillset`, names it `prefix + id (+switch suffix)`, applies pre-selected talents, pushes it into `tree.loadout`, shows modal **"Add to loadout successfull"** [sic] (`:395-402`).
  - Button **"Add all in page to loadout"** (`:1012`) — same for every ID currently buffered on the page.
  - Slider (count) + Button **"Add N random skillsets to loadout"** (`:1031-1104`) — samples N unique random indices (`std::sample` for small sets, a custom collision-avoiding loop for huge sets, `:1046-1089`).
  - Button **"Add all to loadout"** (`:1105`) — adds every filtered result for that point total, hard-capped at `uiData.loadoutSolverAddAllLimit`, with the cap shown inline as "(Limited to `N`)" (`:1122-1129`).

**Tree Solve Status tab** (`:332-376`): a table of in-progress solvers (Button **"cancel"**) and already-solved trees (Button **"reset"**), with colored "solved"/"canceled" status text.

### Canvas interaction (`placeLoadoutSolverTreeElements`, `:407-908`) — the constraint-painting model
Same pan (left-drag) / zoom (Ctrl+scroll, 0.5–3.0×) / Shift-reveal-names as the editor (`:154-173`, `:858-876`). But clicking a node here does **not** spend a point — it cycles a **constraint value**:
- **Left-click** cycles the constraint **forward**: 0 → 1 → 2 ... → `maxPoints` → wraps to **-3** ("exactly one of this switch-group must be maxed", purple) once past `maxPoints` (`:715-718`).
- **Right-click-release** cycles the constraint **backward**, wrapping from below -3 back up to `maxPoints` (`:840-844`).
- No Ctrl/middle-click handling exists on this canvas (unlike the Loadout Editor) — switch-variant choice for materialized results is handled separately, via the post-filter "Switch talent choices" slider, not by clicking the tree.
- Constraint semantics (from the tooltip text, `:64-71`): value ≥0 = "at least N points", **-1** = exclude entirely (red), **-2** = "at least one of this switch group" (blue), **-3** = "exactly one of this group, maxed" (purple).
- Pre-filled talents are disabled for constraint-painting, rendered at reduced alpha (0.35/0.6) (`:692-695, 853-856`).
- If **"Auto apply filter"** is on, **every single click immediately re-filters and resets pagination** (`:719-725, 845-851`) — see performance note below.
- Color glow overlays encode the current constraint value per node: gold = maxed/exactly, green = partial/"at least", red = exclude, blue = group, purple = exactly-one-maxed (`:762-811`).

### Validation & error behavior
The only hard validation is the pre-flight tree-size/combination-count guard (`:515-525`, "Talent tree too large" popup) before a solve is allowed to start; there is no equivalent "did this filter produce zero results" messaging beyond an empty point-total list.

### Solver internals & performance (native implementation, to be reused server-side as-is)
State lives in `talentTreeCollection.activeTreeData()`: `treeDAGInfo` (the solve result — per-point-total vectors of compact `SIND` combination indices, `allCombinationsSum`, `filteredCombinations`, `switchTalentChoices`, `elapsedTime`, `safetyGuardTriggered`/`safetyGuard` cap) and `skillsetFilter` (a reused `TalentSkillset` acting as the constraint vector). `uiData.currentSolvers`/`solvedTrees` track background threads by tree name across the whole app, capped at 3 concurrent solves. Enumeration uses a **64-bit bitmask index scheme** over expanded single-point talent slots (pre-filled talents consume **zero** bits, since they're not user-choices); this has been independently verified sufficient — the worst case across all 79 shipped presets is **60 bits** (the `druid_class_*` presets), so the existing bitset design does not need to change for any currently-shipped tree. Explicit caps: **64 spendable talent points** (architectural limit of the 64-bit index) and a documented **500-million-combination safety guard** (requiring ≥16GB RAM to hold in memory, `:471, :210`). Filtering (`filterSolvedSkillsets`) is a **synchronous, main-thread** scan of the full combination set — a potential UI hitch on large result sets, especially with "Auto apply filter" re-running it on every single click. Results are paginated lazily and full `TalentSkillset` objects are only decoded from a compact index on demand (hover preview, add-to-loadout) — the bulk data structure stays as integer indices until needed. This lazy-materialization design is exactly what a server-side job + paginated-results-API architecture should preserve: the client should only ever request the page it's displaying, never the full result set.

### Persisted/shared state
Solve results and in-progress solver threads persist per-tree regardless of which sub-view tab is active — a solve started here keeps running and can be checked later from any tab, and results feed directly into `tree.loadout`, the same list the Loadout Editor reads/writes.

## View: Sim Analysis (`SimAnalysisWindow.cpp/.h`)

**Purpose**: import SimulationCraft/Raidbots sim output and rank individual talents/ranks by simulated performance, overlaying results as a heatmap on the tree.

Top nav buttons: **"Sim Analysis Settings"**, **"Skillset Rankings"**, **"Talent Breakdown"** (`:363-373`) switch `uiData.simAnalysisPage`.

### Settings page (`:377-533`)
- "(?)" tooltip with the full import instructions (exact expected SimC sim setup) (`:379-380`).
- `InputText` free-text field for either a local file/folder path or a Raidbots URL (`:381`).
- Button **"Add result"** (`:383-389`) → `FetchSimData` → `Engine::ImportSimData` → `AnalyzeRawResults` → `CalculateAnalysisRankings` → `UpdateColorGlowTextures`.
- `ListBox` of imported results, each row labeled "`<skillsetCount>`: `<import name>`" (`:391-405`).
- Button **"Remove result"** (`:406-416`) — erases the selected raw result and re-runs the same pipeline. **No confirmation popup** for this destructive action.
- Read-only stat block: analyzed skillset count, lowest/median/highest/average DPS skillset (name, source import, DPS) (`:419-435`).
- **"Skillset distribution" histogram** over all imported skillsets (`:436-444`) — custom `PlotHistogramRedGreen` widget (not a standard chart library), bounds `[0.95×lowest, 1.05×highest]`.
- Two slider-toggle switches (custom `OptionSwitch` widget, `:21-87`), each with a "(?)" tooltip: **"Talent Icon ↔ Rating"** (`simAnalysisIconRatingSwitch`, `:451`, purely cosmetic — swaps the tree node display between icon and a rank-percentile label, does **not** trigger recompute); **"Relative dps ↔ Ranking"** (`relativeDpsRankingSwitch`, `:456`, **does** trigger recomputation of rankings and colors); **"Show lowest ↔ Show highest"** (`showLowestHighestSwitch`, `:462`, recomputes for multi-rank/switch talents).
- Six `RadioButton`s in one row for the ranking metric: **"Top 1"**, **"Top 3"**, **"Top 5"**, **"Median"**, **"Average"**, **"Top 1 + Median"** (`:472-482`) — changing this recalculates rankings (`:483-486`).
- "Misc" section: Slider "Create top N skillsets simc profilesets" (`:492-502`, N clamped to `[1, min(loadoutSolverAddAllLimit, skillsetCount)]`) + read-only output field + Button **"Generate"** (`:503-525`) builds a SimC profileset export for the top-N ranked skillsets.
- Read-only field + Button **"Generate"** (`:528-532`) — "single talent SimC export" via `Engine::createSingleTalentsSimcString`.

### Skillset Ranking page (`:534-685`) — table, 4 columns, sorted descending by DPS
- Col "Rank": disabled-look Button showing the rank number (`:592`).
- Col "Skillset name": disabled-look Button (`:595`).
- Col "Performance": disabled-look Button "DPS (pct%)", colored yellow if below the reference-DPS ratio, else green (`:598-611`).
- Col "Press to view": a colored horizontal bar sized proportionally to `DPS/referenceDPS` (green→red gradient) overlaid with a full-row-width `InvisibleButton` (`:613-649`). **Left-click** the row → activates that skillset (`Engine::activateSkillset`) and force-navigates to the Loadout Editor tab (`:632-648`). **Hover** any row → live-swaps the TreeWindow canvas to a preview of that skillset (`:652-661`, cleared on mouse-leave, `:675-678`). **Right-click** a row (on release) → silently re-pins that row's DPS as the new `referenceDPS` baseline used for every bar/percentage in the table (`:679-683`) — **no visible label or confirmation** for this "set baseline" gesture; easy to lose in a rewrite if not explicitly documented as its own feature.

### Talent Breakdown page (`:686-897`)
- Empty state: "Select talent to display breakdown. (ctrl+click)" if nothing is selected (`:690-692`). **There is no in-page picker** — the only way to select a talent for this page is **Ctrl+click on its node in the tree canvas** (see canvas interaction below).
- Normal talent: per-rank (1..maxPoints) block — rank header, "Number of skillsets", then (if >0) Lowest/Median/Highest/Average DPS-with-talent and DPS-without-talent, each shown with a colored delta ratio, plus Absolute ranking % and Relative performance %, plus a per-rank "Skillset distribution" histogram (`:703-765`).
- Switch-type talent: the same full breakdown rendered **twice**, once for `talent.name` and once for `talent.nameSwitch` (`:766-894`).

### Search window (`:901-914`)
Identical shared search box as the other three views (`talentSearchString` → `searchedTalents`, blue-glow highlight, `active`/`passive`/`switch` keywords).

### Canvas interaction (`placeSimAnalysisTreeElements`, `:917-1141`)
- **Pan**: **Ctrl+left-click-drag** scrolls the canvas (`:326-335`) — **note: this differs from the other three views**, which pan on a plain left-drag with no modifier.
- **Zoom**: Ctrl+mouse-wheel, clamped **1.0–3.0×** (`:337-349`, `:1120-1140`) — **note: this differs from the other three views' 0.5–3.0× range**, i.e. Sim Analysis cannot zoom out below 100%.
- **Shift held** (no Ctrl) while hovering → reveals name-label overlays over every node (`:1091-1109`).
- Per-talent tooltip (`AttachSimAnalysisTooltip`, hover-triggered, `:135-319`): **left-click** (no Ctrl) cycles the displayed rank forward (wraps at `maxPoints`) or toggles a switch talent's two options; **right-click** cycles the rank backward; **Ctrl+left-click** navigates to the Talent Breakdown page for that talent (`analysisBreakdownTalentIndex = talent->index`). Tooltip body shows the talent name (or an "empty data" message if `skillsetCount==0`), id/position, the hints "(click: switch rank)"/"(ctrl+click: select talent)", the same Lowest/Median/Highest/Average with/without stats, ranking percentages, and a histogram.
- Talent nodes double as a heatmap: colored red→green by the chosen ranking metric, optionally showing a rank/percentile text label instead of the icon (per the Icon↔Rating switch).

### Import path (critical, and currently semi-broken natively)
A **single free-text field**, disambiguated by regex (`:1143-1175`):
- A Windows absolute-path regex → treated as a local file or folder; if a folder, every `*.txt` inside is read (`FetchSimData`, `:1150-1166`; `ReadSimFile`, `:1177-1186`, plain line-by-line `ifstream` read).
- A `https://www.raidbots.com/simbot/report/...` URL regex → **the actual fetch code path (`ReadRaidbots`, which would `libcurl`-GET `raidbots.com/reports/<hash>/output.txt`, `:1188-1223`) is commented out/disabled in shipped code** (`:1168`); instead the input field is overwritten with "Fetching data from Raidbots not yet supported. You can still download the outputs.txt from Raidbots and provide an absolute path." Any other string → the field is overwritten with "Invalid URL or absolute path!" (`:1172`).
- **In practice, the only working import path today is a local SimC/Raidbots `output.txt`-style plain-text file or folder — there is no JSON parsing anywhere in this feature**, and no browser build can replicate the local-path branch directly (needs a file picker/drag-drop + `FileReader`, and a server-side proxy for any future live Raidbots fetch since raidbots.com does not send permissive CORS headers).

### Validation & error behavior
The text parser (`Engine::ImportSimResult`) looks for specific literal marker lines (`"Player:"`, `"Profilesets (median Damage per Second):"`, `"Baseline Performance:"`) and colon-split `dps:name` rows; extracted `(name, dps)` pairs are matched against `tree.loadout` by **exact skillset-name string match** — unmatched sim rows are **silently discarded with no error surfaced to the user** (consistent with the in-app help text's warning about duplicate/mismatched names). There is **no schema validation and no try/catch** around the numeric parse (`std::stod`) — malformed numeric text in the input file can throw an **uncaught exception and crash the app**, a real risk the rewrite should not repeat (add real validation/error UI instead of porting this parser verbatim).

### Persisted/shared state
Imported raw results (`tree.simAnalysisRawResults`), the selected result index, and the full `AnalysisResult` cache persist per-tree and are unaffected by switching sub-view tabs; `analysisBreakdownTalentIndex` and the ranking "reference DPS" baseline are the only pieces of state that reset implicitly (not explicitly saved to the workspace file — sim analysis results are **not** part of `saveWorkspace`'s serialized tree string, so they do not survive an app restart).

### Performance note
All statistics are computed synchronously on the main thread on every import, removal, or ranking-setting change, rebuilding an O(skillsets × talent-columns) selection matrix from scratch each time; acceptable for typical sim sizes (dozens–low hundreds of profilesets) but the Ranking table renders **all** rows every frame with no virtualization — a web port using a plain HTML table at thousands of rows would need virtualization that the native immediate-mode renderer doesn't need to bother with.

## Settings, theming, data model & infrastructure

**Settings/preferences** (`TalentTreeManagerDefinitions.h`, save/load logic actually in `TalentTreeManager.cpp:1042-1324`): theme (`style`), icon-glow toggle, font size (5 presets), autosave interval (default 300s), and the docking split ratio are persisted in a hand-rolled `KEY=VALUE` text file `settings.txt` (with a single-generation backup copy `settings_backup.txt` written before each overwrite); explicitly documented as fragile ("settings can't have ':' in them"). A legacy fallback re-parses these same keys out of `workspace.txt` if `settings.txt` is missing.

**Data model** (Engine layer, referenced throughout GUI): `Talent` (name(+switch name), type Active/Passive/Switch, row/column, points/maxPoints/pointsRequired, pre-filled flag, description text per rank, parent/child graph edges, icon name(s)); `TalentSkillset` (name + `assignedSkillPoints` map + level cap); `TalentTree` (preset name or "custom", Class/Spec type, ordered talent map + roots, one or more `loadout` skillsets + active index, cached solver/sim-analysis results); GUI-level `TalentTreeData` wraps a tree with solve/filter state, and `TalentTreeCollection` holds all open trees plus the active index and the loaded presets map. Trees serialize to/from a single custom delimited string (`Engine::createTreeStringRepresentation`/`parseTree`) — the same format used for file storage, clipboard export, and Pastebin export.

**Window layout persistence**: native `WINDOWPLACEMENT` (position, maximize state) stored as a CSV inside `settings.txt` and applied before the D3D device even exists; ImGui's own docking/window layout persists via the standard `imgui.ini` mechanism pointed at the app-data folder; the app-specific left/right dock split ratio is separately computed and stored since `imgui.ini` doesn't capture that semantic.

**Themes**: three built-in styles — Company Grey (dark), Path of Talent Tree (near-black, square corners, high contrast), Light Mode — each a full ImGui color/style table override; additionally every WoW class gets a hardcoded accent color applied to tab items and the status bar (12 classes, matched by preset-name substring). Class/spec talent-tree presets themselves are not compiled in — they're loaded at runtime from a bundled/updatable plain-text `resources/presets.txt` (plus `node_id_orders.txt` for per-class node ID ordering), copied into `%APPDATA%` on first run and kept current by the updater.

**Icon handling**: icons ship pre-packed server-side into a single tall PNG (`icons_packed.png`) + a metadata text file; at startup the app unpacks it and uploads **each icon as its own separate GPU texture** (parallelized), plus a precomputed grayscale/"disabled" variant per icon. Default icon, node-shape alpha masks (Active/Passive/Switch), and 5 colored glow-ring textures are embedded as raw pixel byte arrays directly in a ~770KB header file. Switch-type talents get a composited "half-and-half" icon splicing two source icons with a divider bar. Custom user icons are supported by dropping 40x40 PNGs into `%APPDATA%/resources/icons/custom/` (same-name files override built-ins). For the web rewrite, this whole "one giant packed texture, unpack into individual GPU textures at boot" approach should be replaced with real individual asset files or a CSS sprite sheet; the alpha-mask node-shape trick maps naturally to CSS `clip-path`/`mask-image`, and glow rings map to CSS `box-shadow`/filters rather than pre-baked bitmaps per color.

**Offline/local storage**: everything lives under `%APPDATA%/<app>/` — `settings.txt`(+backup), `workspace.txt`(+backup, one serialized tree per line plus active-tree/active-skillset markers), `resources/presets.txt`, `resources/node_id_orders.txt`, `resources/resource_versions.txt`, `resources/icons/` (packed PNG + custom folder), plus ImGui's own `imgui.ini`/log. Autosave fires on a timer (default every 5 minutes) and additionally after most explicit edit actions throughout the app. "Reset TTM" performs a full recursive delete of the app-data directory.

**App updater & network calls**: all network I/O uses libcurl, synchronously, on the UI thread (blocking — a browser rewrite trivially improves on this via async fetch). The updater GETs a plaintext version manifest from a public GitHub raw-content URL, compares four tracked resource versions (app version, presets, icons, node-id orders), and re-downloads any stale ones from the same GitHub location; there is **no in-app binary self-update** — a new app version just shows a menu-bar prompt linking to GitHub (a separate `AppUpdater.exe` companion project is referenced but its logic isn't in this GUI source). Update integrity checking is a crude string search for "404:" with no real HTTP status/signature validation (flagged in-code as risky). **No telemetry or analytics of any kind exists anywhere in the app.** A **Pastebin.com** export path exists (`exportToPastebin`) that POSTs a tree's serialized string to Pastebin's API as an unlisted paste with a 1-month expiry and a 60-second client-side export cooldown, returning a shareable URL — this is the app's only built-in "share a tree with someone else" mechanism.

## Cross-cutting UX patterns worth preserving

- **Canvas gesture language** shared by all four tree-based views: pan, zoom-to-cursor, hover tooltip, Shift-hold to reveal all node names, a bottom-left search bar filtering by name or `active`/`passive`/`switch` keyword with blue-glow highlighting. Each view then overloads plain-click/right-click/Ctrl-click/middle-click differently — see the Gesture vocabulary table below for the full breakdown.
- **"Hold Shift to skip confirmation"** pattern used for tab-close and tree-delete.
- **Read-only + auto-select-all InputText** is the app-wide "copy to clipboard" idiom for every export string (tree string, skillset string, Blizzard hash, SimC profilesets, Pastebin URL) — relies on the user manually hitting Ctrl+C after the field auto-selects; a web port should add explicit "Copy" buttons since clipboard writes require an explicit user gesture in browsers anyway.
- **In-app documentation is minimal**: a single long "Controls & Tips" modal is the only onboarding; there's no contextual empty-state guidance beyond a couple of "Select a talent to..." placeholder texts and `(?)` hover tooltips in Sim Analysis.
- No undo/redo system anywhere except the Loadout Editor's single-step self-healing point removal (validity recheck-and-revert, with no user-visible explanation when it triggers). No formal validation errors surfaced for Sim Analysis's fragile text parser (can crash on malformed input).

## Gesture vocabulary (as built)

The same physical gesture means something different in almost every view — this table makes the overloading visible in one place so the rewrite can deliberately redesign it rather than accidentally reproduce it (especially for touch, where middle-click doesn't exist and Ctrl-modifiers are awkward).

| Gesture | View | Effect |
|---|---|---|
| Left-click + drag on empty canvas | Tree Editor, Loadout Editor, Loadout Solver | Pan the view |
| Ctrl + left-click + drag on empty canvas | Sim Analysis | Pan the view — **inconsistent**: this is the only view that requires Ctrl to pan |
| Ctrl + mouse wheel (canvas hovered) | Tree Editor, Loadout Editor, Loadout Solver | Zoom, clamped **0.5–3.0×**, centered on cursor |
| Ctrl + mouse wheel (canvas hovered) | Sim Analysis | Zoom, clamped **1.0–3.0×** — **inconsistent**: cannot zoom below 100% in this view only |
| Left-click a talent node | Tree Editor | Select the node (opens Edit/Delete panel, red glow) |
| Left-click a talent node | Loadout Editor | Spend 1 point (if legal) |
| Left-click a talent node | Loadout Solver | Cycle the node's filter constraint forward (0 → N → exclude → group → exactly-one-maxed) |
| Left-click a talent node | Sim Analysis | Cycle the displayed rank/switch-option forward |
| Right-click a talent node | Tree Editor | Not used |
| Right-click a talent node | Loadout Editor | Remove 1 point (auto-reverted, silently, if it breaks tree validity) |
| Right-click a talent node | Loadout Solver | Cycle the node's filter constraint backward |
| Right-click a talent node | Sim Analysis | Cycle the displayed rank backward |
| Ctrl + left-click a talent node | Tree Editor | Not used (Ctrl reserved for zoom) |
| Ctrl + left-click a talent node | Loadout Editor | Toggle switch-talent variant (no point spent unless already >0) |
| Ctrl + left-click a talent node | Loadout Solver | Not used |
| Ctrl + left-click a talent node | Sim Analysis | Navigate to the Talent Breakdown page for that talent |
| Middle-click (release) a talent node | Loadout Editor | Toggle switch-talent variant (same effect as Ctrl+click) |
| Middle-click (release) a talent node | Tree Editor, Loadout Solver, Sim Analysis | Not used |
| Alt + click a talent node (another node already selected) | Tree Editor | Make the clicked node the **child** of the selected node (toggles the link off if it exists) |
| Alt + click a talent node | Loadout Editor, Loadout Solver, Sim Analysis | Not used |
| Shift + Alt + click a talent node | Tree Editor | Make the clicked node the **parent** of the selected node |
| Shift + Alt + click a talent node | Loadout Editor, Loadout Solver, Sim Analysis | Not used |
| Left-click + drag a talent node | Tree Editor | Reposition it (grid-snapped, displaces/swaps any occupant of the target cell) |
| Left-click + drag a talent node | Loadout Editor, Loadout Solver, Sim Analysis | Not used (node positions are fixed) |
| Shift held (no Ctrl), hovering canvas | All four views | Reveal a name-label overlay over every node at once |
| Ctrl+Delete (node selected) | Tree Editor | Delete the selected node |
| Ctrl+Delete | Loadout Editor, Loadout Solver, Sim Analysis | Not used |
| Typing in the "Search:" box | All four views | Shared `talentSearchString` filters/highlights matching nodes (blue glow); accepts keywords `active`/`passive`/`switch` |
| Hold Shift while closing a tab / confirming a delete | App shell (tree tabs) | Skips the confirmation popup |
| Right-click a row in the Sim Analysis Skillset Ranking table (not a canvas gesture) | Sim Analysis | Re-pins that skillset's DPS as the new reference baseline for the bar chart / percentage column — no visible affordance |

## Web app parity checklist

| Feature | Priority | Justification |
|---|---|---|
| Talent tree canvas: pan/zoom/select, node rendering, connections, point-requirement separators | MUST-HAVE | Core visual product; every other view depends on rendering a tree correctly. |
| Loadout Editor: click-to-spend points, switch-talent toggle, multiple named skillsets, level cap | MUST-HAVE | This is the app's primary end-user value (building/sharing a build), used far more than tree authoring. |
| Talent Tree Editor: create/edit/delete nodes, connections, tree metadata | MUST-HAVE | Needed to support custom trees and to keep bundled class/spec presets current as WoW patches change talents; without it the app can't outlive Blizzard's next talent revamp. |
| TTM tree/skillset string import & export | MUST-HAVE | The app's native interchange format; needed for save/share continuity and to not orphan existing users' saved data. |
| Blizzard in-game talent hash import/export | MUST-HAVE | The single most valuable "bridge to the real game" feature — lets users pull their actual in-game build in and push edited builds back out. |
| Class/spec bundled presets, kept up to date | MUST-HAVE | Without current presets the tool is useless each expansion/patch; but delivery mechanism can be simplified (see updater row). |
| Loadout Solver (combination enumeration/filtering) | MUST-HAVE | The one differentiator no competing web talent-tree tool offers. Plan: keep the existing C++ engine as-is and run it **server-side** as a CLI worker invoked from a backend job queue, streaming results back to the client; the browser's job is only to submit a constrained job, show progress, and paginate results — not to compute combinations itself. |
| Solver pre-flight size validation ("Talent tree too large" check, `LoadoutSolverWindow.cpp:515-525`, `loadoutSolverMaxTalentPoints` threshold) | MUST-HAVE | Must be re-implemented as a **server-side** guard evaluated before a job is dispatched, not merely a client-side UI check, since the browser cannot be trusted to enforce it. |
| Local persistence of trees/loadouts (workspace) | MUST-HAVE | Users expect their work to survive a reload. The core loop must work for **anonymous users with no signup required** — browser-local storage plus URL share codes as the primary mechanism — with accounts layered on additively for cross-device sync, not as a gate to using the tool. |
| Search/filter talents by name/type | MUST-HAVE | Cheap to build, materially helps navigate larger trees. |
| Themes (dark/light at minimum) | SHOULD-HAVE | Nice-to-have polish; only 1-2 themes needed, not all 3 native ones verbatim, and per-class accent colors are a small, easy win. |
| Sim Analysis (import + rank talents) | SHOULD-HAVE | Valuable for theorycrafters but currently half-broken even natively (Raidbots URL disabled, fragile plain-text parser); worth rebuilding cleanly with real JSON/CSV support and a server-side Raidbots proxy rather than porting the existing parser as-is. |
| SimC profileset export | SHOULD-HAVE | Useful bridge to the sim ecosystem; cheap to implement as it's pure string generation once the data model exists. |
| Custom icon overrides | SHOULD-HAVE | Nice for power users/theming but not core; easy to defer since icons can just be a normal asset pipeline. |
| Pastebin export/import for sharing | SHOULD-HAVE | Sharing is valuable, but a rewrite should use its own URL/short-link sharing (e.g. shareable app links with encoded state) instead of depending on a third-party paste service and a hardcoded API key. |
| Window placement / docking layout persistence | DROP | Purely a native-window concept; a responsive web layout replaces it entirely. |
| Native screenshot-to-clipboard (GDI) | DROP | Replace with a simple "Export as PNG" (Canvas `toBlob`) or "Copy image" using the async Clipboard API; no need to replicate the Win32 mechanism. |
| Self-hosted app auto-updater / AppUpdater.exe | DROP | Meaningless for a web app — deployments replace this entirely. |
| Font-size menu (5 presets) / manual UI scaling | DROP | Browsers already provide zoom/text-size controls; better to invest in responsive design and standard accessibility text sizing instead. |
| Reset TTM (full local wipe) / workspace backup files | DROP | Native single-generation text-file backups are a poor pattern; a web app should rely on proper account data model or export/import instead. |

## Top implementation risks for the rewrite

1. **Loadout Solver's async job lifecycle** — the existing C++ solver engine is kept and invoked **server-side** as a CLI worker from a backend job queue, not reimplemented in-browser, so the real risk has shifted from "can we compute this in JS" to the web layer around it: job submission/queuing, streaming progress back to a browser tab that may be closed and reopened, cancellation semantics for a job already running on a server, and paginating potentially hundreds of millions of results without ever shipping the full result set to the client. The 64-bit bitmask indexing itself is already validated as sufficient (worst case 60 bits across all 79 shipped presets, pre-filled talents cost zero bits) and does not need to change.
2. **Canvas gesture overloading** — plain-click/right-click/Ctrl-click/middle-click/Shift-hold all mean different things per view and per hovered element (see the Gesture vocabulary table), and two views even disagree on the pan/zoom modifier itself; this many-to-one mouse-modifier scheme must be redesigned for mouse+touch parity (no middle-click, ambiguous Ctrl on mobile) without losing the interactions power users rely on (point spend/remove, switch-toggle, connect/disconnect, solver constraint cycling).
3. **Sim Analysis import pipeline is fragile and partly non-functional today** (Raidbots URL fetch is disabled in shipped code; the plain-text SimC log parser can throw on malformed numbers and silently drops unmatched skillset names) — this needs to be redesigned properly (server-side Raidbots proxy for CORS, robust parsing/validation) rather than ported as-is.
4. **Icon pipeline** (one packed PNG unpacked into hundreds of individual GPU textures at boot, plus raw-byte-embedded masks/glow textures) needs a full redesign as a web asset pipeline (individual files or sprite sheet + CSS masks/box-shadow) — not a mechanical port.
5. **Data model & string-format migration** — trees/skillsets/settings are all hand-rolled delimited text formats with documented fragility (e.g. can't contain `:`); the rewrite should define a real JSON schema up front and provide a one-time importer for existing users' `workspace.txt`/`settings.txt` files and TTM export strings, rather than trying to keep the legacy formats as the source of truth.
