# Legacy Web App Post-Mortem (`Web/`)

Status: abandoned, half-built. This document describes what exists under `Web/` as of the last commits, for the purpose of deciding what to salvage before scrapping it and starting a fresh React + Tailwind frontend (and presumably a fresh backend).

Scope note: `Web/frontend/public/*` binary assets were not opened, only enumerated. `node_modules` and `package-lock.json` were not read.

---

## 1. Backend

**Framework**: Flask 2.3.3 (`Web/server/requirements.txt:15`), synchronous WSGI app, no ASGI. Dev server only — `Web/server/README.md:3` says to run it with `flask --app server --debug run`. There is no Gunicorn/uWSGI config, no Dockerfile, no process manager anywhere in `Web/server/`. This is not production-serving code, just the Flask dev server.

**App wiring**: `Web/server/components/create_app.py` builds a module-level `Flask` app, a SQLAlchemy `Engine` pointed at a local SQLite file at `Web/server/database/data.sqlite` (`components/create_app.py:13-19`), a `DBHandler` (from `database_handler.py`), CORS (`flask_cors`, wide open: `origins: "*"` with `supports_credentials=True`, `components/create_app.py:30-34` — those two together are actually invalid/unsafe under browser CORS rules), and `flask_jwt_extended.JWTManager`. `Web/server/server.py` is the actual Flask entrypoint; it just imports `create_app` (which constructs `app`, `db_handler`, `jwt` as module singletons) and then imports the four view modules purely for their route-registration side effects (`Web/server/server.py:1-5`) — a common but fragile Flask pattern (import order matters, no blueprints/`create_app()` factory function despite the file being named `create_app.py`).

**Auth mechanism**: JWT stored in an httpOnly cookie (`JWT_TOKEN_LOCATION=["cookies"]`, `components/create_app.py:24`), 1-day expiry, refreshed via an `after_request` hook that reissues the cookie if it's within 30 minutes of expiring (`components/views/user.py:79-93`). Two login methods:
- Username/email + password: PBKDF-style hashing via `hashlib.scrypt` (n=2**14, r=8, p=1) with a random 16-byte salt per user (`components/views/user.py:64-65`).
- Google SSO: verifies a Google ID token server-side via `google.oauth2.id_token.verify_oauth2_token` (`components/views/user.py:32-48`), then looks up the login by email — i.e., SSO accounts must already exist with a matching email row; there's no auto-provisioning path visible.

Email/password accounts require **email activation**: `create_account` creates a `Logins` row with `IsActivated=False` plus an `Activations` row with a random activation UUID, and emails a link via `EmailHandler.send_verification_email` (`components/views/user.py:128-167`, `Web/server/email_handler.py`). `EmailHandler` sends plain SMTP over Gmail (`smtp.gmail.com:465`) using an app password from env vars — no templating, no HTML email, and the credentials/domain are all `os.environ[...]` reads that raise at import time if unset (`Web/server/email_handler.py:11-17`).

CSRF: the frontend manually reads a `csrf_access_token` cookie and sends it as `X-CSRF-TOKEN` on the one mutating call it makes (`copyImport`, `Web/frontend/src/api/contentAPI.jsx:10-14,64`), implying Flask-JWT-Extended's CSRF protection was intended but the config for it isn't visible in `create_app.py` (no `JWT_COOKIE_CSRF_PROTECT` set, so it's likely relying on the extension's default-True behavior without the app code fully wiring it up everywhere — most POST routes, e.g. `/login`, `/create_account`, don't send/require it at all).

**No queue, no background workers, no websockets.** Every route is a plain synchronous Flask view.

### Route table

| Method | Path | Auth | Params | Response shape (`msg` payload) |
|---|---|---|---|---|
| POST | `/login` | none | body: `auth_method` (`USERNAMEEMAIL`\|`SSO`), `user_name_email`/`password` or `sso_token` | `{success, msg, user_id?}`; sets JWT cookie |
| POST | `/logout` | none | — | `{success, msg}`; clears JWT cookie |
| POST | `/create_account` | none | body: `user_name`, `email`, `password` | `{success, msg}` |
| GET | `/activate_account/<activation_id>` | none | path: activation UUID | `{success, msg}` |
| GET | `/delete_account` | JWT (fresh) | — | `{success, msg}`; deletes `Logins` row, clears cookie |
| GET | `/check_if_logged_in` | JWT (optional) | — | `{success, msg, user_id?}` |
| GET/POST | `/tree/<content_id>` | JWT | path: content id | GET: `{success, msg: {name, description, isImported, classTalents, specTalents}}`. POST: **stub** — just echoes `content_id`, no body handling implemented (`components/views/tree.py:30-31`) |
| GET/POST | `/loadout/<content_id>` | JWT | path: content id | GET: `{success, msg: {name, isImported, description, treeName, builds}}`. POST: same stub as above |
| GET/POST | `/build/<content_id>` | JWT | path: content id | GET: `{success, msg: {name, isImported, levelCap, useLevelCap, assignedSkills, description, treeName, loadoutName}}`. POST: stub |
| GET | `/build/special/<class_name>/<spec_name>` | JWT | path params | `{success, msg: {buildColumns, buildData}}` — top/outlier WCL builds for a spec, per encounter |
| GET | `/tree/preset/<class_name>/<spec_name>` | JWT | path params | `{success, msg: content_id}` |
| GET | `/content/<content_id>` | JWT | path: any content id (tree/loadout/build) | `{success, msg: {contentType, tree, loadout|build, in_user_workspace}}` — the "combined" fetch used by the Viewer scene |
| POST | `/content/copyimport/<content_id>` | JWT | path: content id | `{success, msg: {contentID}}` — either deep-copies or creates an "import" alias row depending on whether the source is already in the user's workspace |
| GET | `/workspace` | JWT | — | `{success, msg: {treeColumns, treeData, loadoutColumns, loadoutData, buildColumns, buildData}}` — pre-shaped for MUI DataGrid |

Notably **absent**: no create/update/delete routes for trees, loadouts, or builds (the `POST` handlers on `/tree`, `/loadout`, `/build` are unimplemented stubs that just return the id), no route to list/search public/popular content by class+spec directly (the dashboard instead reuses `/build/special`), no comment/like endpoints despite `Comments`, `Likes`, and `Feedbacks` tables existing in the DB layer — those three tables have full CRUD in `database_handler.py` but **zero corresponding Flask routes**. So "liking"/"commenting"/feedback was modeled in the DB and never wired to the API or frontend at all.

---

## 2. Database

**Engine**: SQLite (`sqlite+pysqlite:///.../database/data.sqlite`, `components/create_app.py:19`), accessed through SQLAlchemy Core (`Engine`/`text()`), with `pypika` used as a query builder that generates SQL strings which are then re-wrapped in `sqlalchemy.text()` (`database_handler.py:5-6, 325` etc.) — an unusual two-layer indirection (pypika → SQL string → `text()` → SQLAlchemy). No ORM models, no Alembic/migrations; schema is created imperatively with `CREATE TABLE IF NOT EXISTS` in `initialize_databases()` (`Web/server/database_handler.py:273-315`), gated by a `SELECT count(*) FROM sqlite_master WHERE type='table'` check that only checks the table *count* equals 8 (i.e. it assumes no partial-init state and no schema drift — adding a 9th table breaks the "already initialized" check).

**No foreign keys, no indexes, no NOT NULL/UNIQUE constraints anywhere.** Every column in every `CREATE TABLE` is untyped-enough SQLite (`TEXT`/`INTEGER`/`BLOB`) with no constraints at all (`database_handler.py:299-312`). All relationships are enforced only in Python application code.

### Schema (as created in `initialize_databases`, `database_handler.py:299-312`)

**Logins**
| Column | Type | Notes |
|---|---|---|
| UserID | TEXT | app-generated UUID, "public" id |
| AltUserID | TEXT | second UUID, used as the JWT `sub`/identity so the "real" UserID is never in the token |
| AuthMethod | TEXT | `PW` or `SSO` |
| Email | TEXT | |
| PasswordHash | BLOB | scrypt hash, unused for SSO |
| Salt | BLOB | |
| UserName | TEXT | lower-cased at write time in app code |
| LastLoginTimestamp | TEXT | set once at creation; never observed being updated on subsequent logins |
| IsActivated | INTEGER | boolean-as-int |

**Activations** — AltUserID (TEXT), ActivationID (TEXT). One-time email-activation tokens; row is deleted after use (`db_handler.delete_activation`).

**Workspaces** — UserID, ContentType (`TREE`/`LOADOUT`/`BUILD`), ContentID, Public (INTEGER DEFAULT 0). This is the many-to-many join table between users and "content" (trees/loadouts/builds), and it's also where public/private visibility is tracked per (user, content) pair rather than on the content row itself.

**Trees** (user-owned) / **PresetTrees** (curated/official) — near-identical: ContentID, `ImportID` (Trees only — see "import" pattern below), Name, Description, ClassTalents (TEXT, JSON array of talent content-ids), SpecTalents (TEXT, JSON array of talent content-ids); PresetTrees additionally has ClassName/SpecName instead of ImportID.

**Loadouts** — ContentID, ImportID, TreeID (FK by convention only), Name, Description.

**Builds** (user-owned) / **TopBuilds**, **OutlierBuilds** (curated, per encounter) — ContentID, ImportID (Builds only)/EncounterID (Top/Outlier only), TreeID, LoadoutID (nullable), Name, ClassName+SpecName (Top/Outlier only), LevelCap, UseLevelCap, AssignedSkills (TEXT, JSON — see below), Description.

**Talents** (user tree talents) / **PresetTalents** (curated) — ContentID, OrderID (dense index within a tree, used as the stable id referenced by `AssignedSkills` and by `ClassTalents`/`SpecTalents` JSON arrays), NodeID (the *native desktop engine's* node id — a second id system layered under OrderID), TalentType (`ACTIVE`/`PASSIVE`/`SWITCH`), Name, NameSwitch, Description (TEXT — JSON-encoded array of strings for PASSIVE talents with multiple description "paragraphs" per rank, plain string otherwise — a type-dependent encoding baked into a single TEXT column), DescriptionSwitch, Row, Column (grid layout position), MaxPoints, RequiredPoints (the "points needed in tree so far" gating threshold used to draw divider lines), PreFilled (INTEGER — talents auto-granted with no points spent), ParentIDs/ChildIDs (TEXT, JSON arrays of OrderIDs — the DAG edges), IconName, IconNameSwitch.

**Likes** — UserID, ContentID, Timestamp. Full CRUD exists in `database_handler.py` (`create_like`/`delete_like`/`get_likes`), **no route ever calls it**.

**Comments** — CommentID, UserID, ContentID, Message, Timestamp, ReplyID (self-referential, for threaded replies), WasEdited. Same story: full CRUD, zero routes. Also has a real bug: `get_comments` appends `Like(*row)` instead of `Comment(*row)` (`database_handler.py:1255`), so calling it would silently misconstruct objects (fortunately unreachable dead code).

**Feedbacks** — FeedbackID, UserID, Message. CRUD has `create_feedback` only, no read/delete, no route.

### How trees/builds were stored: hybrid blob-and-row

This is the most important design fact: **talent trees are normalized (one row per talent node), but assignment of points into a tree ("what did the user actually spec") is a JSON blob.** Specifically:
- A tree's structure lives as *rows* in `Talents`/`PresetTalents`, one per node, with the DAG encoded as JSON arrays of neighbor ids inside a TEXT column (`ParentIDs`/`ChildIDs`) rather than an edge table.
- A tree *record* (`Trees`/`PresetTrees`) just stores two JSON arrays of talent-content-ids (`ClassTalents`, `SpecTalents`) — so "which talents belong to this tree" is also blob-encoded, not a join table.
- A `Build`'s `AssignedSkills` column is a JSON-serialized `[{orderID: points}, {orderID: points}]` pair (class dict, spec dict) — see `Build.__init__` (`database_handler.py:122-149`) and its use in `components/views/tree.py:323` (`json.loads(build.assigned_skills)`). This is conceptually the same "skillset string" idea as the native app's talent string format, just re-encoded as JSON instead of the desktop app's custom `:`/`;`-delimited text format — and `data_management/preset_updater.py` shows the desktop format is still the source of truth being *translated into* this JSON on ingest (see §4).

### "Import" pattern (copy vs. reference)

Trees/Loadouts/Builds support two ways to acquire someone else's content into your workspace: a real **copy** (new row, new ContentID, full data duplicated) or an **import** (a new row with `Name=NULL` and `ImportID` pointing at the source ContentID — effectively a lazy alias/symlink row). Reads then walk the `ImportID` chain until they hit a row with a non-null `Name` (`find_root_tree`/`find_root_loadout`/`find_root_build` in `components/views/tree.py:167-181, 235-250, 288-308`). This is a reasonable "cheap forking" idea (avoids duplicating whole trees when someone just wants a pointer into someone else's public tree) but it's implemented as an unbounded linked-list walk with no cycle detection and no depth limit — an import chain of imports would walk N deep on every single read, and a cycle (should one ever be created by a bug) would infinite-loop.

### Schema assessment

**Modeled reasonably well:**
- Separating "definition" (Talents/Trees) from "custom vs. preset" via parallel tables is simple and avoids a `source` enum + nullable-everything design.
- The `OrderID` vs `NodeID` split (a dense per-tree index used for wire/storage compactness, vs. the engine's own semantic node id used to correlate against external data like WarcraftLogs) is a real and useful distinction, and it's carried through consistently.
- User content vs. curated/official content as separate tables (rather than an `is_preset` flag) keeps the curated data pipeline (see §4) from ever being able to corrupt user data via a shared table, and keeps preset reads index-free/full-table-scannable since the preset tables are small and rarely written.

**Modeled poorly:**
- No foreign keys/constraints at all — every relationship (Workspaces.ContentID → Trees/Loadouts/Builds, Builds.TreeID → Trees, ParentIDs/ChildIDs → other Talents rows) is a bare TEXT/INTEGER column enforced only by application discipline. A stray delete or typo silently orphans data; nothing in the DB would ever complain.
- `ContentID` is a single global id space shared across Trees, Loadouts, Builds, PresetTrees, TopBuilds, and OutlierBuilds (`validate_content_access` in `database_handler.py:469-506` literally probes multiple tables in sequence to figure out what a given id even is). This makes every "resolve a content id" call an N-table lookup in the worst case and means the type of a piece of content isn't self-describing from its id.
- JSON-in-TEXT columns (ParentIDs, ChildIDs, ClassTalents, SpecTalents, AssignedSkills, and conditionally Description) mean the DB can't query/index/validate any of that structure — every consumer has to `json.loads` and trust the shape. Combined with no schema versioning, changing the JSON shape later has no migration story.
- Booleans stored as INTEGER with no CHECK constraint (`IsActivated`, `Public`, `PreFilled`, `UseLevelCap`, `WasEdited`) — fine for SQLite but a footgun if this ever moves to Postgres/MySQL where the app code's `== 1` / `== "1"` comparisons scattered through the codebase would need auditing.
- `Likes`/`Comments`/`Feedbacks` are fully-modeled dead weight — schema and full CRUD exist with no route ever calling them, i.e. "modeled ahead of need" that never got finished.
- The desktop engine's `NodeID` and the web app's `OrderID` are both present on every talent row, which is good for traceability, but nothing enforces `OrderID` is actually dense/contiguous per tree — it's just "whatever order the preset importer emitted them in."
- SQLite itself: fine for a single-process dev server, a real non-starter for anything with concurrent writers (Python's stdlib/SQLAlchemy default sqlite behavior here has no WAL mode configured, no busy-timeout handling visible) — every `db_handler` method opens its own connection and commits immediately, so under any real concurrency this would hit `database is locked` errors constantly.

---

## 3. Engine integration

**There is no runtime integration with the C++ solver/engine at all.** No subprocess, no FFI/pybind, no message queue, no RPC to a native binary anywhere in `Web/server/`. The only cross-reference to the native app's build artifacts is **file-based, offline, at ETL time**:
- `Web/server/update_data.py` computes a SHA-256 hash over two *files produced by the native desktop app's CI* — `Engine/resources/cicd_presets/presets.txt` and `GUI/resources/updatertarget/icons_packed_meta.txt` (`Web/server/update_data.py:26-57`) — and only re-runs the preset/icon ETL if that hash changed since last run (cached in a local `hash.txt`).
- `data_management/preset_updater.py` parses `presets.txt`, which is the *native app's own custom talent-string export format* (colon/semicolon-delimited, with `__cl__`/`__n__`/`__cm__`/`__sc__` escape sequences for literal `:`, newline, `,`, `;` — see `restore_string()` at `data_management/preset_updater.py:167-175`), and converts it into `PresetTree`/`Talent` rows for the web DB.
- `data_management/extract_icons_to_public.py` reads a packed icon spritesheet + metadata file also produced by the native GUI's build (`GUI/resources/updatertarget/icons_packed.png` / `icons_packed_meta.txt`) and slices it into individual PNGs dropped into `frontend/public/preset_icons/`.

So the actual "engine" (whatever solves/optimizes talent builds in the C++ app) was never called from the web backend. The web app only ever *displayed* talent trees/builds; it never computed anything solver-related. The one piece of "smart" server-side logic (`data_management/create_popular_builds.py`) doesn't touch the local engine either — it pulls real player data from the WarcraftLogs GraphQL API (`https://www.warcraftlogs.com/api/v2/client`, OAuth2 client-credentials flow) to derive "top" and "outlier" builds per encounter (see §4), which is a completely different code path from the desktop app's optimizer.

**Conclusion for the rewrite**: there is no existing pattern to imitate or reuse for wiring a solver into a request handler — that integration (worker process, job queue, subprocess sandboxing, whatever is chosen) has to be designed from scratch.

---

## 4. Data pipeline (`update_data.py`, `data_management/*`)

`Web/server/update_data.py:update_data()` is the intended daily cron entrypoint (per its own comment, `update_data.py:2`) and does, in order:
1. **Change detection**: hash `presets.txt` + `icons_packed_meta.txt`; skip steps 2–3 if unchanged (`update_data.py:26-72`).
2. **`preset_updater.update_presets()`** (only if changed): parse the native app's `presets.txt` export, split each line into either a "custom" tree (no class/spec — content authored directly in-app rather than scraped, tracked separately) or a class/spec combination pair, decode the escaped talent metadata (name/description/icon per rank, including SWITCH talents' comma-separated dual name/description/icon fields), and either bulk-recreate or (default) `UPDATE ... WHERE Name = ...` the `PresetTrees`/`PresetTalents` tables (`data_management/preset_updater.py:36-176`). Note the update path keys off human-readable `Name` (e.g. `"Fire Mage"`) rather than a stable id — a renamed spec would silently fail to match and update path (`update_preset_trees`, `database_handler.py:662-673`).
3. **`extract_icons_to_public.extract_icons()`** (only if changed): slice the packed icon spritesheet into individual PNGs, written both into the frontend's `public/preset_icons/` (served at dev time) and, if present, directly into a hardcoded production path `/var/www/ttm/build/preset_icons/` (`data_management/extract_icons_to_public.py:27-34,52-53`) — a deploy-target path baked into the ETL script itself.
4. **`create_popular_builds.create_popular_builds()`** (always runs): authenticates to the WarcraftLogs API via OAuth2 client-credentials, and for every (encounter × class × spec) combination fetches the top-100 Mythic character rankings, maps each player's chosen talent ids to the web app's internal `NodeID`→`OrderID` scheme via a talent-id table fetched live from `raidbots.com/static/data/live/talents.json`, and computes two derived builds per combination: the literal #1-ranked build ("top") and the build within the top 100 that's most different from the population mean by a simple weighted L2-ish outlier score (`np.argmin` over `points * column-mean`, `data_management/create_popular_builds.py:130-150`). Results are upserted into `TopBuilds`/`OutlierBuilds` keyed by matching `PresetTrees.Name` string again (`update_top_and_outlier_builds`, `create_popular_builds.py:287-317`) — same fragile name-matching pattern as step 2. This step fans out over `encounter × class × spec` combinations concurrently via a plain `ThreadPoolExecutor(max_workers=11)` (`create_popular_builds.py:54`) with no retry/backoff and best-effort per-combination exception swallowing (logs and continues, `create_popular_builds.py:85-99`).

Net effect: this is a real, somewhat sophisticated ETL pipeline (three external data sources — native app CI output, WarcraftLogs, Raidbots — reconciled into one SQLite DB) but it's brittle: name-based joins instead of stable ids in two different places, a hardcoded prod path inside a script that's supposed to be environment-agnostic, no tests, and `if __name__ == "__main__"` runners with no scheduling/orchestration wired up beyond "some cron presumably calls update_data.py daily" (never actually configured anywhere in this repo).

---

## 5. Frontend

**Stack**: React 18.2 via Create React App (`react-scripts` 5.0.1 — not Vite; `Web/frontend/package.json:22,26`), MUI v5 (`@mui/material`, `@mui/icons-material`, `@mui/x-data-grid`) as the component/UI library, `@emotion` for styling (MUI's default), `reactflow` 11.9 for the talent tree canvas, `@tanstack/react-query` v5 for server-state/data-fetching, `react-router-dom` v6 for routing, `formik` + `yup` for form state/validation, `react-pro-sidebar` for the nav sidebar, `@nivo/bar` + `@nivo/core` for charts (only used by the orphaned `Bar` demo scene), `local-storage` (a tiny localStorage wrapper) for persisting UI prefs (sidebar collapsed state), and `jwt-decode` (present in deps but not observed being imported by any file that was read). **New target explicitly drops MUI in favor of Tailwind**, so essentially none of the component styling is directly reusable — it would need to be rebuilt, though the *behavioral* logic (what state a component holds, what it renders conditionally) largely transfers.

This is visibly a fork of a popular MUI admin-dashboard tutorial template (the color token structure in `theme.js`, the `Bar`/`Team`/`FAQ`/`Form` scenes, and `mockData.js`'s Game-of-Thrones-named contact/team/invoice fixtures and world-bank geography data are 1:1 recognizable as that template's stock demo content, not WoW-specific work). Those scenes (`scenes/bar`, `scenes/form`, `scenes/faq`, `scenes/team`) and `data/mockData.js` are **orphaned**: `scenes/global/appdisplay.jsx:59-96` (the actual `<Routes>` table) never mounts any of `Bar`/`Form`/`FAQ`/`Team`, and `mockData.js`'s only consumers are `scenes/team` and `components/BarChart.jsx`, both themselves unrouted. Similarly `scenes/tree/index.jsx` and `scenes/build/index.jsx` are hardcoded-content-id smoke-test scenes for `TreeViewer`/`BuildViewer` (they call `treeAPI.get("8bf345aa-...")` / `buildAPI.get("25998983-...")` with literal hardcoded UUIDs) that are also not in the route table — early scratch pages superseded by the unified `scenes/viewer`.

**Entry/bootstrap**: `src/index.js` renders `<BrowserRouter><App/></BrowserRouter>` (React 18 `createRoot`, StrictMode explicitly commented out — `index.js:8-14`, likely disabled because double-invoked effects were breaking something, itself a small red flag). `App.js` nests providers: `QueryClientProvider` → `ColorModeContext.Provider` (dark/light MUI theme toggle, defaults to dark, `theme.js:222-236`) → `AuthProvider` → `DragProvider` → `ThemeProvider`/`CssBaseline` → `AppDisplay`.

**Routing** (`scenes/global/appdisplay.jsx:59-96`): `/` (Dashboard), `/configurator` (stub), `/workspace` (protected), `/viewer/:contentID?` (protected), `/editor` (stub), `/analysis` (stub), `/login` (public), `/activation/:activationID?` (public). `ProtectedRoute` (`components/ProtectedRoute.jsx`) redirects to `/` if `loginState===false` and renders nothing while `loginState===null` (auth-check in flight) — simple but workable gate, no role/permission levels, just logged-in/not.

**State management**: no Redux/Zustand — just React Context for two small pieces of global UI state (`AuthProvider` for `loginState`/`userID`, `DragProvider` for "is the user currently dragging the reactflow canvas" so tooltips can suppress themselves during a pan) plus React Query for all server data (each scene owns its own `useQuery`, manually disabled with `enabled:false` and refetched imperatively via `refetch()` in `useEffect`s keyed on route params or class/spec selections — see `scenes/viewer/index.jsx:99-146` for three separate manually-triggered queries in one component). This pattern (disable-by-default + manual refetch) works but fights react-query's idiomatic auto-refetch-on-key-change model; it was clearly done to sequence "don't fetch until a UUID is present" rather than adopting `queryKey`-driven enabling more cleanly.

**Talent tree rendering — the most salvageable piece**:
- `components/tree_components/TreeViewer.jsx` (read-only tree display) and `BuildViewer.jsx` (same tree, colored/filled according to a build's assigned points) both wrap `reactflow`. Talent rows from the API (a dict keyed by `order_id`) are mapped to reactflow nodes/edges: position is `column`/`row` × a fixed `gridSpace` (`TreeViewer.jsx:40-42`), node type is chosen from `talent_type` (`DIVIDER`/`PASSIVE`/`ACTIVE`/`SWITCH` → `dividerNode`/`passiveNode`/`activeNode`/`switchNode`), and edges are drawn from each node's `child_ids` with arrow color depending on whether both ends are "filled" (gold), one end is (green in BuildViewer), or neither is (grey/dim) — see `TreeObjectToFlowEdges` in both files.
- `components/tree_components/NodeTypes.jsx` implements the four visual node types as MUI `Box`/`Tooltip` compositions: circular passive nodes, square active nodes, octagonal dual-icon switch nodes (showing both options split-image when unpicked, single icon once a choice is made), and a small "divider" pseudo-node used purely to draw the horizontal "N points required" separator line across the tree (not a real talent).
- `components/tree_components/utils.js:insertDividerLines` is a nontrivial bit of layout logic: it scans all nodes for distinct `requiredPoints` thresholds, finds each threshold's row band, and synthesizes a pair of "divider" nodes + a connecting edge positioned at the midpoint between adjacent bands — i.e., it's reconstructing WoW's "you need N points in this tree before unlocking the next row" visual convention entirely from node data, with no author-placed divider objects in the DB.
- Interaction is deliberately minimal/read-only: `nodesDraggable={false}`, `zoomOnScroll={false}`, `panOnScroll={false}` — the only enabled interactions are click-drag panning (guarded by `DragProvider` to suppress tooltips mid-pan, since a stuck-open tooltip while panning was apparently a bug they hit) and hover tooltips showing name/description/icon/points. There is no editing capability implemented anywhere (the `Editor` scene is an empty stub) and the commented-out `onNodeDragStart`/`onNodeDragStop` handlers in `TreeViewer.jsx:148-169` show they *started* wiring drag-to-reposition for an editor but abandoned it mid-way.
- Icons are referenced as flat filenames (`/preset_icons/${iconName}`) served as static files, sliced at ETL time from the desktop app's packed spritesheet (§4) — i.e., the web app has its own icon set derived from, but decoupled from, the native app's `class_icons`/`spec_icons` assets that ship in `public/`.

**Wire format implied by `data/mockTree.js`/`mockData.js`**: `mockTree.js` is trivial — four hardcoded `passiveNode`s wired in a simple tree, clearly an early reactflow-getting-started scratch file, not a real fixture; it predates and doesn't match the real API shape (real nodes carry the full `data` object built in `TreeObjectToFlowNode`, not just `{size}`). `mockData.js` is entirely unrelated template boilerplate (Game of Thrones names, ISO country codes, invented invoice/transaction data) with no bearing on the TTM wire format at all — it's noise left over from the admin-dashboard template this frontend was bootstrapped from, not evidence of intended API shape. The *real* wire format is best read from the Flask route handlers directly (§1's route table) and from `TreeObjectToFlowNode`'s consumption of a talent record (§ above) — talents keyed by dense integer `order_id`, `child_ids`/`parent_ids` as integer arrays, `description` as either a plain string or (for multi-rank PASSIVE talents) a JSON string containing a string array.

**Auth approach** (frontend half — backend covered in §1): `AuthProvider` polls `/check_if_logged_in` once on mount and again on every route change (`appdisplay.jsx:31-33`), storing only a boolean `loginState` and a `userID` — no token/claims are held client-side (correct given httpOnly JWT cookies), login/logout/create-account are thin wrappers around `userAPI` calls that flip `loginState`. Google SSO uses Google's own hosted `accounts.id` JS SDK loaded via a global `window.google` (script tag not found in the files read — likely injected in `public/index.html`, which was excluded from this review) with the client id read from a `../../data/secrets` module that is **not present** in the files read (imported in `scenes/login/index.jsx:17` as `googleClientID` from `"../../data/secrets"` — this file must exist locally/gitignored but wasn't part of the reviewed set, and its absence from `Web/frontend/src/data/*` in the earlier file listing suggests it may not even be committed).

---

## 6. Maturity assessment

| Area | Status | Evidence |
|---|---|---|
| Auth (password + SSO + activation) | **Functional**, not production-hardened | Full login/create/activate/logout/delete flow works end-to-end; CORS config (`origins:"*"` + credentials) is actually broken/unsafe as configured; CSRF only wired for one route |
| Tree viewing (read-only) | **Finished** for the happy path | `TreeViewer`/`NodeTypes`/`utils.js` divider-line logic is complete and handles all three talent types + preset vs custom |
| Build viewing (read-only) | **Finished** for the happy path | `BuildViewer` mirrors `TreeViewer` with point-fill coloring; `Viewer` scene wires tabs between tree/loadout/build views |
| Tree/build/loadout **editing** | **Not started** | `Editor` scene is a one-line stub; `POST /tree`, `POST /loadout`, `POST /build` are unimplemented stubs that just echo the id; commented-out drag handlers in `TreeViewer.jsx` show an abandoned start |
| Copy/import content between users | **Functional** | `content.py`'s `copy_import` + the `ImportID`-chain read pattern is fully implemented and reasonably clever, if unbounded/uncapped |
| Workspace (my content list) | **Functional but rough** | Works, author's own comment calls the layout "complete garbage" (`scenes/workspace/index.jsx:73`); no create button is wired (visible `Button` has no `onClick`) |
| Popular/curated builds (WCL top/outlier) | **Functional pipeline, thin UI** | Full ETL in `create_popular_builds.py`; `Dashboard` scene displays it only after picking class+spec and only when logged in |
| Sim/analysis integration with the native engine | **Not started at all** | No code path anywhere calls the C++ engine; `Analysis` scene is a one-line stub |
| Comments/Likes/Feedback | **DB-only, unreachable** | Full CRUD in `database_handler.py`, zero Flask routes, zero frontend code |
| Preset/icon data pipeline | **Functional** | `preset_updater.py` + `extract_icons_to_public.py` work against real native-app CI artifacts, gated by a hash-based change check |
| Data pipeline scheduling | **Not started** | `update_data.py` is a manually-run script; nothing invokes it on a schedule |
| Deployment | **Not started** | No Dockerfile, no WSGI server config beyond Flask dev server, one hardcoded prod path (`/var/www/ttm/...`) inside an ETL script is the only deployment artifact in evidence |
| Tests | **None found** | No test files encountered anywhere under `Web/` |
| Frontend template cleanup | **Incomplete** | Four entire scenes + a chart component + a large mock-data file are unrouted leftovers from the MUI admin-dashboard tutorial this was bootstrapped from |

Overall: this is a solid **prototype of the read path** (view a tree, view a build, view popular community builds, browse your own workspace) with authentication that mostly works, sitting on top of a genuinely interesting but partially-dead-weight database design, and with **zero write/editing capability and zero engine integration** — the two things that would make it an actual product. The tree-rendering layer is the one piece that's both finished and conceptually sound.

---

## 7. Post-mortem / lessons

### What to salvage

1. **The talent-tree rendering approach** (`TreeViewer.jsx`, `NodeTypes.jsx`, `utils.js`): reactflow-as-canvas with typed custom node components per talent kind (passive/active/switch) and the derived-divider-line algorithm are worth reimplementing directly against Tailwind styling. The core idea — position from `(row, column)` on a fixed grid, color edges/borders by "points invested" state, synthesize divider lines from `requiredPoints` bands rather than storing them — is sound and class/framework-agnostic.
2. **The dense `OrderID` vs. engine `NodeID` split** for talent identity: a compact per-tree integer id for wire efficiency and JSON-key use, kept separate from whatever the "real" engine/import-source id is. Worth keeping as a modeling pattern even in a new schema.
3. **Separating curated/preset content from user content into parallel tables** rather than a shared table with an `is_preset` flag — keeps the ETL pipeline from ever risking user data.
4. **The copy-vs-import (deep copy vs. lazy alias-by-id) idea** for "add someone else's content to your workspace" — genuinely useful UX concept, just needs a bounded/cycle-safe implementation (e.g., always resolve to the ultimate root at copy/import time rather than chaining aliases, or cap chain depth).
5. **The offline, hash-gated ETL pipeline concept** for turning native-app build artifacts (preset talent exports, packed icon sheets) into web-servable data (`update_data.py`'s change-detection via file hashing) — cheap and effective, worth keeping the *shape* of (detect upstream change → convert format → regenerate icons) even if the implementation is rebuilt.
6. **The WarcraftLogs "top build" / "outlier build" per-encounter idea** (`create_popular_builds.py`) — pulling real ranked-player talent data and surfacing both the consensus build and the most-different-from-consensus build is a genuinely good product idea worth keeping, independent of any code reuse.
7. **Icon/asset sets already extracted**: `Web/frontend/public/class_icons/` and `Web/frontend/public/spec_icons/{deathknight,druid,evoker,hunter,mage,monk,paladin,priest,rogue,shaman,warlock,warrior}/` are ready-to-use class/spec icon sets (not opened for content, but present and organized) that a new frontend can likely reuse as-is or as a starting point.
8. **Email-activation flow shape** (create unactivated → emailed token → activate → flip flag) is a reasonable, standard pattern worth keeping conceptually, just needs a real transactional-email provider instead of raw Gmail SMTP with an app password.

### What to avoid / why scrapping is right

1. **Zero engine integration was ever attempted.** The single hardest problem for a real product — how does a request to "optimize/solve a build" reach the C++ engine and come back — has no precedent in this codebase to build on or avoid; a new design needs to think from scratch about sync-vs-async (a solver call in a synchronous Flask request handler would block a worker thread for the whole computation; this codebase never even got far enough to make that mistake, but the new one must actively design against it — a job queue + polling/websocket result, not an inline blocking call).
2. **No write path was ever built**, meaning the hardest state-management problems (conflict handling, partial-save, optimistic UI, validation of a talent-point allocation against the DAG's constraints) are completely unproven here. Don't assume the read-side design (e.g., the JSON-blob `AssignedSkills` field) will hold up once an editor needs to mutate it incrementally, node by node, potentially with undo/redo — a normalized `(build_id, order_id, points)` row-per-assignment table would serialize much more safely under concurrent partial edits than a single JSON blob written wholesale on every save.
3. **No foreign keys / constraints anywhere** — this was fine for a single-developer prototype but would need real constraints (and probably a real RDBMS, not SQLite) the moment there's concurrent writers or any external API consumers; don't carry the "everything is a bare TEXT column, enforce relationships in Python" pattern forward.
4. **Global shared `ContentID` space across six tables** requiring multi-table probing to resolve type is a self-inflicted complexity tax (`validate_content_access`'s sequential-table-probe design) — a new schema should make content type self-describing (either a discriminator column on a single content table, or type-prefixed ids).
5. **Mock-data-driven / template-driven development left real dead weight**: four unrouted scenes, a large irrelevant mock-data file, and a chart component that's never rendered are still sitting in the tree. Bootstrapping from a tutorial template is fine as a starting point, but this shows the risk of never doing the cleanup pass — a new project should budget time to actually delete scaffolding once superseded, not leave it as permanent clutter.
6. **Auth/security loose ends that must not be repeated as-is**: CORS configured with `origins:"*"` *and* `supports_credentials=True` together (browsers reject or silently strip this combination — it's not even internally consistent, let alone secure); CSRF protection wired for exactly one endpoint instead of applied uniformly; secrets read via bare `os.environ[...]` that throw uncaught `KeyError`s at import time if unset (bad failure mode for ops); Gmail SMTP + app password for transactional email instead of a real provider (deliverability and rate-limit risk).
7. **Name-keyed joins in the data pipeline** (`update_preset_trees` matching on `Name`, `create_popular_builds` matching on `f"{spec} {class}"` strings) are fragile — any content rename silently breaks the update path with no error surfaced. A new pipeline should key on stable ids throughout.
8. **SQLite with no WAL/busy-timeout tuning and a connection-per-call pattern** doesn't survive concurrent writers; don't carry this forward past prototype scale without at least enabling WAL mode, or moving to Postgres if multi-writer access (e.g. a job worker plus the API process) is expected — which it will be, the moment engine integration exists.
9. **Dependency staleness risk**: `requirements.txt` pins 2023-era versions of Flask/Flask-JWT-Extended/SQLAlchemy/google-auth, and the frontend is CRA (react-scripts 5, already in maintenance mode upstream in favor of Vite/other tooling) — a fresh start avoids inheriting nearly two years of unpatched dependency drift on day one.
10. **No tests anywhere.** Every piece of nontrivial logic found here (divider-line placement, the import-chain walk, the outlier-build statistic, the talent-string escape/unescape round-trip) is exactly the kind of logic that regresses silently without unit tests; the rewrite should not repeat "prototype now, test later" since "later" never arrived here.
