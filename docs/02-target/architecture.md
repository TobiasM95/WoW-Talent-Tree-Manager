# Target architecture

Status: proposal, pre-implementation. Assumes the constraints stated for the revival: Docker
Compose, a simple and performant backend, a React + Tailwind frontend, the C++ engine invoked as
a command-line worker, a real queue, Postgres — with few components, doing as much as possible
in Postgres.

## 1. Component topology

Five containers. Deliberately no Redis, no RabbitMQ, no Celery, no separate cache — Postgres
covers queueing and caching at this scale, and every component removed is one fewer thing to
operate.

```
                    ┌─────────────┐
   browser ────────►│   caddy     │  TLS, static assets, reverse proxy
                    └──┬───────┬──┘
                       │       │
            /api/*     │       │  /*
                       ▼       ▼
                ┌──────────┐  ┌──────────────┐
                │   api    │  │  frontend    │  React+Tailwind (static build)
                │ FastAPI  │  └──────────────┘
                └────┬─────┘
                     │ SQL + LISTEN/NOTIFY
                     ▼
                ┌──────────┐        ┌──────────────────────────┐
                │ postgres │◄───────│  worker (1..N)           │
                │          │        │  python supervisor       │
                └──────────┘        │   └─ execs ttm-solver    │
                     ▲              └──────────────────────────┘
                     │
                ┌────┴───────┐
                │  ingest    │  scheduled job: talent data → trees
                └────────────┘
```

| Container | Role | Notes |
|---|---|---|
| `caddy` | TLS termination, static file serving, reverse proxy | Automatic HTTPS; one less config file than nginx |
| `frontend` | build-time only | Vite build output served by caddy; no Node at runtime |
| `api` | HTTP API | FastAPI + uvicorn; stateless; horizontally scalable |
| `worker` | claims solve jobs, execs the C++ binary | Thin Python supervisor around `ttm-solver` |
| `postgres` | data + queue + cache | Postgres 17 |
| `ingest` | talent data ingestion | Scheduled; see [`talent-data-sources.md`](talent-data-sources.md) |

### Why FastAPI

The honest tradeoff: Go or Rust would be measurably faster per request, but essentially all
request latency here is Postgres round-trips and the solver is a separate process anyway, so the
web layer is not the bottleneck. Python keeps one language across the API, the worker supervisor,
and the ingest pipeline — and the ingest logic already exists in Python
(`tree_presets_generator.py`). FastAPI with async endpoints and asyncpg is more than sufficient.
Revisit only if profiling shows the web tier is actually the constraint.

## 1b. The product model: count, then filter, then sim

This is the shape of the whole service, and it decides most of what follows.

The native client enumerated every build into RAM, then let the user filter that set down
to something simmable. A web service cannot hold 305 million builds per user, but it does not
need to — **the filter is the selection mechanism, not a sampling step.** The user's
constraints are the point, and they want every build satisfying them.

So the flow inverts into three stages:

1. **Count** — the frontier DP answers "how many builds match these constraints?" in
   milliseconds, inline in the API. No queue, no worker.
2. **Filtered enumeration** — the C++ engine produces the matching builds, with the result
   size *already known* from stage 1.
3. **Sim** — export as SimC profilesets, run, import results, rank, show per-talent statistics.

What each stage buys:

- Stage 1 is a **pre-flight gate**. A filter matching 40 million builds is rejected before a
  worker is spawned ("too many to sim, tighten it"). Every job that reaches the queue has a
  known, bounded size — which removes most of the defensive machinery the earlier design
  needed.
- Because the count is known in advance, the job gets a **real progress bar** with a true
  denominator and an honest ETA, rather than an open-ended spinner.
- Stage 2 is now genuinely cheap for realistic filters. See below.

Explicitly **not** the model: sampling random builds. A user who constrains to 5,000 builds
wants those 5,000, not a sample of a larger space.

### Filtered search is now output-sensitive

Must-have pruning was added to `visitTalentFiltered` (it previously pruned only on
must-not-have, testing must-have once per completed path — so the most common query,
"I want these talents", got no speedup at all and paid for a full enumeration).

Measured on `druid_restoration`, engine-reported solve time:

| Query at 30 points | Builds | Solve | vs unfiltered |
|---|---:|---:|---:|
| unfiltered | 305,286,987 | 37.9 s | — |
| must-have ×3 | 56,877,903 | 7.1 s | 5.3× |
| must-have ×3 + must-not ×2 | 9,464,517 | 1.1 s | 33.5× |

Cost now tracks the size of the answer rather than the size of the search space, which is
what makes stage 2 interactive. Two bitmask tests do it: a required talent whose position has
been passed can never be taken (paths visit strictly increasing positions), and each still-owed
required talent costs at least one point.

### Storage

Results are bounded by stage 1, so they are small enough to store normally — no 9.5 GB files,
no object storage, no result streaming protocol. The queue remains worthwhile because a
filtered solve can still take seconds, but it no longer has to defend against unbounded output.

## 2. The queue: Postgres, not a broker

`SELECT ... FOR UPDATE SKIP LOCKED` is the whole mechanism. It has been the correct answer for
single-digit-thousands-of-jobs-per-day workloads since Postgres 9.5, and it gives transactional
consistency between job state and result data for free — which a separate broker cannot.

```sql
CREATE TYPE job_state AS ENUM ('queued','running','done','failed','cancelled','capped');

CREATE TABLE solve_jobs (
  id             uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id        uuid REFERENCES users(id) ON DELETE SET NULL,
  tree_id        uuid NOT NULL,
  tree_revision  int  NOT NULL,
  request        jsonb NOT NULL,          -- point budget, filters
  request_hash   bytea NOT NULL,          -- sha256(tree snapshot + request) for dedup/cache
  state          job_state NOT NULL DEFAULT 'queued',
  priority       smallint NOT NULL DEFAULT 100,
  attempts       smallint NOT NULL DEFAULT 0,
  progress       real NOT NULL DEFAULT 0,
  result_count   bigint,
  error          text,
  locked_by      text,
  locked_at      timestamptz,
  created_at     timestamptz NOT NULL DEFAULT now(),
  finished_at    timestamptz,
  FOREIGN KEY (tree_id, tree_revision) REFERENCES trees(id, revision),
  CHECK (progress >= 0 AND progress <= 1)
);

CREATE INDEX ON solve_jobs (state, priority, created_at) WHERE state = 'queued';
CREATE UNIQUE INDEX ON solve_jobs (request_hash) WHERE state IN ('queued','running','done');
```

Claim loop:

```sql
UPDATE solve_jobs SET state='running', locked_by=$1, locked_at=now(), attempts=attempts+1
WHERE id = (
  SELECT id FROM solve_jobs WHERE state='queued'
  ORDER BY priority, created_at
  FOR UPDATE SKIP LOCKED LIMIT 1
)
RETURNING *;
```

Three details that matter:

- **Wake-ups via `LISTEN`/`NOTIFY`**, with a periodic poll as a fallback. Notify alone is not
  reliable enough on its own (a notification delivered while no worker is listening is lost), so
  poll every few seconds as a safety net rather than relying on either mechanism alone.
- **Dedup and caching are the same mechanism.** `request_hash` = sha256 over the tree snapshot
  plus the normalised request. A partial unique index means an identical in-flight or completed
  solve is reused instead of recomputed. Given the enumeration cost, this is the single highest-
  value optimisation available and it costs one index.
- **Crash recovery**: a sweeper requeues rows stuck in `running` past a lease timeout
  (`locked_at < now() - interval '10 min'`), up to a max `attempts`. Workers extend the lease as
  they report progress.

### Results

Solve results are potentially enormous — the engine's own safety guard defaults to 500 million
combinations. Never materialise that into the database or into a JSON response.

- The worker writes the engine's NDJSON output to object storage (or a mounted volume for a
  single-host deployment), and stores only a pointer plus `result_count` in Postgres.
- Postgres stores a **bounded top-N page** of results for immediate display, not the full set.
- The API paginates over the result file by byte offset, or re-reads it on demand.
- Apply a retention policy: results are derived data, and can be recomputed from the cached
  `request_hash`. Expire them.

## 3. The engine as a worker

The engine stays C++ and stays authoritative. It is the reason this project is worth reviving,
and a rewrite would be strictly worse. See
[`../01-current-state/engine-and-solver.md`](../01-current-state/engine-and-solver.md) for the
algorithm and portability analysis.

### Packaging

A dedicated multi-stage image. The existing MSVC `.vcxproj` files cannot build in Linux, so a
CMake build is required:

```dockerfile
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y build-essential cmake
COPY Engine CLI CMakeLists.txt ./
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j

FROM debian:bookworm-slim AS runtime
COPY --from=build /build/ttm-solver /usr/local/bin/ttm-solver
```

Porting work is genuinely small — there are no SIMD or MSVC intrinsics, all bit operations are
plain portable `uint64_t` shifts, and `std::filesystem` is standard C++17. What must change:

- `<Windows.h>` in three files. The only real dependency is `GlobalMemoryStatusEx` in
  `setSafetyGuard` (`Engine/src/TreeSolver.cpp:27`) — replace with an explicit `--mem-budget`
  argument, which is the correct behaviour in a container anyway, since host RAM is the wrong
  number to read.
- `SHGetKnownFolderPath` for `%APPDATA%` (`Engine/src/TTMEnginePresets.cpp:23`) — remove
  entirely; a worker takes explicit input paths and has no user profile.
- MSVC-only PPL `Concurrency::parallel_for` in `CLI/CLI.cpp:271` — replace with `std::thread`.
  Note this is the *only* actual parallelism in the codebase; the functions named
  `countConfigurationsParallel` are single-threaded (that name refers to computing all point
  totals in one pass).
- New CMake build alongside the retained `.vcxproj` files, so the native app still builds.

### Worker protocol

Args plus files, not stdin — it keeps the contract inspectable and lets a failed job be replayed
by hand, which matters a lot when debugging an enumeration bug.

```
ttm-solver solve \
  --tree       /work/<job>/tree.json      \
  --request    /work/<job>/request.json   \
  --out        /work/<job>/results.ndjson \
  --progress   /work/<job>/progress.json  \
  --max-results  5000000 \
  --mem-budget   2147483648 \
  --time-budget  60000
```

- **stdout** stays clean for a single final JSON summary; **stderr** carries logs.
- **Results stream** as NDJSON, one decoded build per line, flushed incrementally. The current
  CLI buffers everything until completion and emits raw decimal integers, which is unusable for
  a responsive UI.
- **Progress** is written to a small separate file from the existing `runningCount` counter — a
  cheap change against a counter the hot loop already maintains.
- **Exit codes** must distinguish: success, cap-exceeded-but-partial-results, invalid input,
  internal error. "Capped" is a legitimate, expectable outcome and needs to be a first-class
  state, not an error.
- **`--time-budget` does not exist today** and must be added: the engine has a combination-count
  guard and a memory guard but *no wall-clock guard at all*, which is unacceptable for a shared
  server.
- Every job runs in a container with hard `cpus`/`memory` limits, so a pathological tree cannot
  take down the host.

### Validate before spawning, and solve one tree at a time

The API must reject oversized trees before a worker is spawned, because the engine throws an
uncaught `std::logic_error` and terminates when a tree exceeds the 64-slot `SIND` bitset. A
crashed worker is a far worse error surface than a clean 4xx. The native app did exactly this
pre-check (`GUI/src/LoadoutSolverWindow.cpp:515-525`, "Talent tree too large"), and the rewrite
must keep it.

The ceiling is verified as comfortable **provided each tree is solved independently**:

- The `> 64` check counts total expanded single-point slots for the whole tree
  (`Engine/src/TreeSolver.cpp:54,204,439`); pre-filled talents consume zero bits because
  `expandTreeTalents` removes them and promotes their children to roots.
- Across all 79 shipped presets the worst case is **60 bits** (`druid_class_*`);
  `druid_restoration` is 43 nodes / 48 bits. Most DF talents are 1-point.
- A TWW hero tree (~10-11 nodes, ~15-20 points) solved as its own third independent tree stays
  far under the ceiling.

**Therefore: keep solves per-tree.** A single *combined* class+spec+hero solve would need roughly
75-80 bits and would require widening `SIND`. Blizzard's point economy does not require a
combined solve, and neither the engine nor the native GUI ever attempted one. If widening is
ever wanted, it is contained rather than an algorithm redesign — `SIND` is one typedef
(`Engine/src/TTMEnginePresets.h:13`) with ~94 references, but only ~5 sites actually assume
64-bit width: three `1ULL <<` literals (`TreeSolver.cpp:844,1044`, `CLI.cpp:361`), one hardcoded
`< 64` loop bound (`TreeSolver.cpp:852`), and decimal serialisation in `CLI.cpp:385` and
`LoadoutSolverWindow.cpp:1024,1118`.

## 4. Frontend

React + Tailwind, Vite (not CRA — the legacy app's `react-scripts` is effectively unmaintained).

The tree canvas is the heart of the product and the main technical risk. Three options:

| Approach | Verdict |
|---|---|
| SVG + absolutely-positioned nodes | **Recommended.** ~100 nodes with edges is trivial for SVG; gives free hit-testing, CSS styling, accessibility, and text rendering. |
| reactflow | What the legacy app used. Powerful, but built for user-authored graph editing and heavier than needed for a mostly-static layout. Reasonable fallback for the *tree editor*. |
| Canvas/WebGL | Unnecessary at this node count; loses text and accessibility. |

Interaction notes carried over from the native UI (see
[`../01-current-state/gui-feature-inventory.md`](../01-current-state/gui-feature-inventory.md)):
the desktop app overloads left/right/middle/Ctrl/Shift-click on the canvas to mean different
things per view. That does not survive contact with a browser or a touch device. Re-design the
gesture vocabulary explicitly rather than porting it — and keep left-click-adds /
right-click-removes, which is the one convention WoW players already expect.

Solving is server-side, so the UI submits a job and subscribes to progress (SSE is sufficient;
WebSockets are not needed for one-directional progress). The UI must be designed around solves
being *asynchronous and occasionally capped* — that is a product surface, not just an
implementation detail.

## 5. Data ingestion

A scheduled container, not a GitHub Action committing binaries to git. The legacy pipeline's
failure mode was precisely that it failed *silently* — a crash simply skipped the commit step
with no alert, which is how the TWW break went unnoticed for months.

Requirements, each one a direct response to a documented legacy failure:

1. **Validate the upstream payload against a schema** before transforming. Raw `KeyError`s
   inside a 3-minute CI timeout produced no diagnostics.
2. **Fail loudly.** A failed ingest must alert. Track `last_successful_ingest_at` and surface
   staleness in the UI.
3. **Never overwrite the live snapshot in place.** Write a new tree `revision`; promote it only
   after validation passes. Builds pin revisions, so old builds keep working.
4. **Detect the unknown.** Unrecognised node types, new sub-tree kinds, or a changed class/spec
   set should warn rather than silently mis-classify — the legacy code silently mis-classified
   raidbots' `tiered` node type as something else.
5. **Decouple data-schema version from app release version.** The legacy `resource_versions.txt`
   conflated them, and had already drifted (`1.3.8` hardcoded in the generator vs `1.4.2`
   elsewhere).
6. **Icons are individual files behind a CDN/cache**, not a 16 MB packed atlas. The atlas existed
   only to save bandwidth for the native updater. Committing it 76 times is why `.git` is 1.4 GB
   against a 90 MB working tree.

## 6. Auth

Keep it minimal and correct. The legacy implementation had `origins:"*"` together with
credentialed CORS, CSRF wired for exactly one route, secrets read via bare `os.environ[...]`,
and Gmail SMTP for transactional mail.

- Session cookies (httpOnly, `Secure`, `SameSite=Lax`) with server-side sessions in Postgres.
  Simpler and more revocable than JWT, and there is no cross-domain requirement to justify JWT.
- Explicit CORS allow-list, or same-origin only via the reverse proxy — which is the simpler
  option and is what the topology above gives for free.
- OAuth (Battle.net is the natural choice, and would additionally allow importing a player's
  actual characters) plus optional email/password.
- Anonymous use must work for the core loop. Requiring an account to view or build a tree is the
  fastest way to lose the audience this tool had.

## 7. What is deliberately excluded

- **No Redis / Celery / RabbitMQ** — Postgres `SKIP LOCKED` is sufficient and is one less
  service.
- **No Kubernetes** — Compose on a single host is appropriate for this scale.
- **No microservices** — three processes (api, worker, ingest) around one database.
- **No server-side rendering** — this is an interactive tool, not a content site. Static SPA plus
  an API is correct. (Revisit only if SEO on shared-build pages becomes a goal; a prerendered
  OG-image endpoint for share links would cover most of that benefit far more cheaply.)
