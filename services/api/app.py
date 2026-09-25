"""TTM API.

Reads `current_trees` (the highest promoted ingest revision) and `tree_counts`, and answers
the pre-flight count that the whole product model hangs on: *how many builds match these
constraints?* -- inline, in milliseconds, before any job is dispatched. See
docs/02-target/architecture.md.

Two count paths, both fast for different reasons:

- **Unfiltered** -- a primary-key lookup in `tree_counts`, precomputed at load time.
- **Filtered** -- the frontier DP run on demand. Filters only remove transitions, so a
  constrained count is *cheaper* than an unconstrained one; a few milliseconds either way.

Anonymous by design: nothing here needs an account. Identity is additive and comes later.
"""
from __future__ import annotations

import functools
import json
import os
import re
import sys
import time
from typing import Annotated, Any, Literal

from fastapi import Depends, FastAPI, HTTPException, Query, Request, Response
from pydantic import BaseModel, Field, field_validator

# The frontier DP lives in tools/ as the verified reference implementation; the API uses it
# rather than reimplementing counting.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))), "tools", "frontier-dp"))
sys.path.insert(0, os.path.join("tools", "frontier-dp"))

# The ingest package owns icon-name normalisation; the API must agree with it exactly, or a
# lookup would miss the very row the sync wrote.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))), "services", "ingest"))
sys.path.insert(0, os.path.join("services", "ingest"))

DEFAULT_LEVEL_CAP = 90

# Icons are content-addressed by name and never change under a name, so they can be cached
# for a year. This is the entire reason to serve them per file instead of as an atlas.
ICON_CACHE_SECONDS = 31_536_000

# Above this many matching builds, listing them is not something to offer. The count is
# free, so this is a product decision enforced at the gate rather than a worker failure.
LISTING_LIMIT = 2_000_000

app = FastAPI(
    title="WoW Talent Tree Manager API",
    version="2.0.0-dev",
    summary="Talent trees, build counts, and the solver gate.",
)


# ---------------------------------------------------------------------------
# database
# ---------------------------------------------------------------------------

_pool = None


def pool():
    global _pool
    if _pool is None:
        from psycopg_pool import ConnectionPool
        url = os.environ.get("DATABASE_URL")
        if not url:
            raise RuntimeError("DATABASE_URL is not set")
        _pool = ConnectionPool(url, min_size=1, max_size=8, open=True)
    return _pool


def query(sql: str, params: tuple = ()) -> list[dict[str, Any]]:
    from psycopg.rows import dict_row
    with pool().connection() as conn:
        with conn.cursor(row_factory=dict_row) as cur:
            cur.execute(sql, params)
            return cur.fetchall()


# ---------------------------------------------------------------------------
# tree loading, cached
# ---------------------------------------------------------------------------

# What a stored solve result means, as a version. Part of the cache key, so bumping it makes
# every earlier result unreachable instead of silently served.
#
#   2  result bits name talents by rank over the whole tree, granted roots included --
#      version 1 ranked only the solved DAG and mislabelled every talent after a granted
#      root (migration 007 drops those rows).
SOLVE_CONTRACT = 3


@functools.lru_cache(maxsize=256)
def _dp_graph(tree_key: str, level_cap: int):
    """Build and cache the DP graph for a tree.

    Cached because it is pure: a tree revision's graph never changes, and the graph build
    costs more than the count itself.
    """
    from frontier_dp import build_expanded_graph, topo_sort, _resolve_max_points

    rows = query(
        "SELECT id, revision, definition FROM current_trees WHERE key = %s", (tree_key,)
    )
    if not rows:
        raise HTTPException(404, f"no current tree with key {tree_key!r}")
    definition = rows[0]["definition"]

    ids = {n["nodeId"] for n in definition["nodes"]}
    nodes = {
        n["nodeId"]: {
            "index": n["nodeId"],
            "name": n.get("name") or "",
            "type": 2 if n.get("kind") == "choice" else 1,
            "row": n["row"], "col": n["col"],
            "maxPoints": _resolve_max_points(n, level_cap),
            "req": n["pointsRequired"],
            "preFilled": bool(n["preFilled"]),
            "parents": [p for p in n["parents"] if p in ids],
            "children": [c for c in n["children"] if c in ids],
        }
        for n in definition["nodes"]
    }
    meta, par, chi = build_expanded_graph(nodes)
    order = topo_sort(meta, par, chi)
    return {
        "tree_id": rows[0]["id"], "revision": rows[0]["revision"],
        "meta": meta, "par": par, "chi": chi, "order": order,
        "slots": len(meta), "node_ids": ids,
        "ranks": {i: n["maxPoints"] for i, n in nodes.items()},
        # Which nodes the DP actually models, which is not every node in the tree.
        #
        # A pre-filled root is *granted*: the expansion removes it and promotes its children,
        # so no build ever spends a point on it. Constraints naming one used to pass
        # validation and then be silently dropped -- requiring Rake and excluding Rake both
        # returned the unfiltered count, so a user who barred a talent got builds that all
        # had it, with nothing to notice.
        "modelled": {m["orig"] for m in meta.values()},
    }


# ---------------------------------------------------------------------------
# models
# ---------------------------------------------------------------------------

class TreeSummary(BaseModel):
    key: str
    kind: str
    name: str
    className: str | None
    specName: str | None
    subTreeId: int | None
    nodeCount: int
    maxPointsInTree: int
    # Null until a verified source exists. The upstream payload does not carry the game's
    # real per-tree cap, and guessing it would be fabrication.
    pointCap: int | None
    # A classic tab's place among its class's three; None for retail trees.
    order: int | None = None


class CountRow(BaseModel):
    points: int
    sets: int = Field(description="Selections with choice-node sides unresolved.")
    builds: int = Field(description="Sides resolved: sum over sets of 2^(choice nodes).")


class CountRequest(BaseModel):
    treeKey: str
    points: int = Field(ge=1, le=64)
    levelCap: int = DEFAULT_LEVEL_CAP
    mustHave: list[int] = Field(default_factory=list)
    mustNotHave: list[int] = Field(default_factory=list)
    # {"88209": "a"} -- pin a choice node to an alternative, or exclude it outright.
    choiceSides: dict[str, Literal["a", "b", "none"]] = Field(default_factory=dict)
    # Groups of node ids. "at least one of these" / "exactly one of these".
    atLeastOneOf: list[list[int]] = Field(default_factory=list)
    exactlyOneOf: list[list[int]] = Field(default_factory=list)
    # Per-rank refinement of a multi-rank talent: {"88209": 2} -- at least / at most so many
    # ranks. Both together say "exactly"; a cap of 1 is a one-point dip.
    rankMin: dict[str, int] = Field(default_factory=dict)
    rankMax: dict[str, int] = Field(default_factory=dict)

    @field_validator("mustHave", "mustNotHave")
    @classmethod
    def _cap_filter_size(cls, v: list[int]) -> list[int]:
        if len(v) > 64:
            raise ValueError("at most 64 node constraints")
        return v


class SolveRequest(CountRequest):
    maxResults: int = Field(default=LISTING_LIMIT, ge=1, le=LISTING_LIMIT)
    timeBudgetMs: int = Field(default=120_000, ge=1_000, le=600_000)


class JobResponse(BaseModel):
    id: str
    state: str
    treeKey: str
    points: int
    expectedCount: int | None
    resultCount: int | None
    progress: float
    # Which part of the job `progress` measures -- "solving", "storing" or "finalizing",
    # null when the job is not running. A single fraction cannot carry this, because the
    # phases are not comparable: a solve can enumerate two million sets in a tenth of a
    # second and then spend far longer writing them. "finalizing" has no fraction at all
    # and reports 0 rather than inventing one.
    phase: str | None
    # A cancellation was asked for but the job has not stopped yet. A UI shows
    # "cancelling..." on this rather than pretending the job is already gone.
    cancelRequested: bool = False
    error: str | None
    createdAt: str
    finishedAt: str | None


class CountResponse(BaseModel):
    treeKey: str
    points: int
    levelCap: int
    sets: int
    builds: int
    filtered: bool
    source: Literal["precomputed", "computed"]
    elapsedMs: float
    # The gate: whether listing these builds is something to offer.
    listable: bool
    listingLimit: int


class SpreadResponse(BaseModel):
    treeKey: str
    levelCap: int
    # Index k: selections spending exactly k points. Index 0 is the empty build.
    sets: list[int]
    builds: list[int]
    elapsedMs: float


# ---------------------------------------------------------------------------
# endpoints
# ---------------------------------------------------------------------------

@app.get("/health")
def health(game: str = "retail") -> dict[str, Any]:
    """Liveness plus whether the data we serve is actually fit to serve.

    Reports the promoted revision and its age. The legacy pipeline's failure was that
    nothing ever asked how old the data was.
    """
    try:
        rows = query(
            """
            SELECT revision, promoted_at, tree_count, node_count, description_coverage,
                   extract(epoch FROM (now() - promoted_at))::bigint AS age_seconds
            FROM ingest_runs
            WHERE promoted_at IS NOT NULL AND game = %s
            ORDER BY revision DESC LIMIT 1
            """,
            (game,),
        )
    except Exception as exc:  # noqa: BLE001 - health must report, not raise
        return {"status": "degraded", "database": f"unreachable: {type(exc).__name__}"}

    if not rows:
        return {"status": "degraded", "database": "ok", "data": "no promoted revision"}

    run = rows[0]
    return {
        "status": "ok",
        "database": "ok",
        "revision": run["revision"],
        "trees": run["tree_count"],
        "nodes": run["node_count"],
        "descriptionCoverage": run["description_coverage"],
        "iconCoverage": _icon_coverage(),
        "dataAgeSeconds": run["age_seconds"],
    }


def _icon_coverage() -> float | None:
    """Fraction of referenced icon names with a cached image.

    Reported next to descriptionCoverage for the same reason: the legacy pipeline's defining
    failure was that nothing asked whether the data it served was complete. Icons are
    optional -- a client renders talents by name without them -- so this is information,
    not a health failure, and None means the cache has never been filled.
    """
    try:
        rows = query(
            """
            WITH referenced AS (
                SELECT DISTINCT lower(e->>'icon') AS name
                FROM current_trees t,
                     jsonb_array_elements(t.definition->'nodes') n,
                     jsonb_array_elements(n->'entries') e
                WHERE e->>'icon' IS NOT NULL
            )
            SELECT count(*) AS want,
                   count(i.name) AS have
            FROM referenced r
            LEFT JOIN icons i ON i.name = r.name AND i.status = 200
            """
        )
    except Exception:  # noqa: BLE001 - health must report, not raise
        return None
    want = int(rows[0]["want"] or 0)
    if not want:
        return None
    return round(int(rows[0]["have"] or 0) / want, 4)


@app.get("/trees", response_model=list[TreeSummary])
def list_trees(
    game: str = "retail",
    kind: str | None = Query(None, description="class, spec or hero"),
    classId: int | None = None,
    specId: int | None = None,
) -> list[TreeSummary]:
    """Every tree in the promoted revision, optionally narrowed."""
    if game == "custom":
        # A custom project is seen by those who have its link, not by browsing: listing every
        # tree anyone has saved would publish them. GET /custom-trees/{project} lists one.
        raise HTTPException(400, "custom trees are listed per project: GET /custom-trees/{project}")
    sql = ["SELECT key, kind::text, name, class_name, spec_name, sub_tree_id,",
           "       node_count, max_points_in_tree, point_cap,",
           "       (definition->>'order')::int AS tab_order",
           "FROM current_trees WHERE game = %s"]
    params: list[Any] = [game]
    if kind:
        sql.append("AND kind = %s")
        params.append(kind)
    if classId is not None:
        sql.append("AND class_id = %s")
        params.append(classId)
    if specId is not None:
        sql.append("AND spec_id = %s")
        params.append(specId)
    sql.append("ORDER BY class_name, spec_name, kind, tab_order, sub_tree_id")

    return [
        TreeSummary(
            key=r["key"], kind=r["kind"], name=r["name"],
            className=r["class_name"], specName=r["spec_name"],
            subTreeId=r["sub_tree_id"], nodeCount=r["node_count"],
            maxPointsInTree=r["max_points_in_tree"], pointCap=r["point_cap"],
            order=r["tab_order"],
        )
        for r in query(" ".join(sql), tuple(params))
    ]


@app.get("/trees/{tree_key:path}/counts", response_model=list[CountRow])
def tree_counts(tree_key: str, levelCap: int = DEFAULT_LEVEL_CAP) -> list[CountRow]:
    """Precomputed unfiltered counts for every budget. A primary-key read."""
    rows = query(
        """
        SELECT c.points, c.set_count, c.build_count
        FROM tree_counts c
        JOIN current_trees t ON t.id = c.tree_id AND t.revision = c.tree_revision
        WHERE t.key = %s AND c.level_cap = %s
        ORDER BY c.points
        """,
        (tree_key, levelCap),
    )
    if not rows:
        raise HTTPException(404, f"no counts for {tree_key!r} at level cap {levelCap}")
    return [
        CountRow(points=r["points"], sets=int(r["set_count"]), builds=int(r["build_count"]))
        for r in rows
    ]


@app.get("/trees/{tree_key:path}")
def get_tree(tree_key: str) -> dict[str, Any]:
    """The full tree definition: nodes, edges, gating, entries and descriptions."""
    rows = query(
        "SELECT definition FROM current_trees WHERE key = %s", (tree_key,)
    )
    if not rows:
        raise HTTPException(404, f"no current tree with key {tree_key!r}")
    return rows[0]["definition"]


def _group_nodes(req: "CountRequest") -> set[int]:
    return {n for group in (req.atLeastOneOf + req.exactlyOneOf) for n in group}


def _validate(req: "CountRequest", graph: dict) -> None:
    unknown = (set(req.mustHave) | set(req.mustNotHave)) - graph["node_ids"]
    unknown |= {int(k) for k in req.choiceSides} - graph["node_ids"]
    unknown |= {int(k) for k in (*req.rankMin, *req.rankMax)} - graph["node_ids"]
    unknown |= _group_nodes(req) - graph["node_ids"]
    for group in req.atLeastOneOf + req.exactlyOneOf:
        if len(group) < 2:
            raise HTTPException(
                400, "a group constraint needs at least two nodes; use mustHave for one")
    if unknown:
        raise HTTPException(400, f"node id(s) {sorted(unknown)[:5]} are not in {req.treeKey!r}")
    # Granted talents are not part of any build's point spend, so a constraint on one has no
    # meaning. Refused rather than ignored: silently dropping it answers a question the user
    # did not ask.
    granted = (
        (set(req.mustHave) | set(req.mustNotHave) | _group_nodes(req)
         | {int(k) for k in req.choiceSides} | {int(k) for k in (*req.rankMin, *req.rankMax)})
        & graph["node_ids"]
    ) - graph["modelled"]
    if granted:
        raise HTTPException(
            400,
            f"node id(s) {sorted(granted)[:5]} are granted automatically, so they cannot be "
            "required, excluded or grouped")
    contradictory = set(req.mustHave) & set(req.mustNotHave)
    if contradictory:
        raise HTTPException(
            400, f"node id(s) {sorted(contradictory)} are both required and excluded")
    for key, low in req.rankMin.items():
        ranks = graph["ranks"].get(int(key), 1)
        if not 1 <= low <= ranks:
            raise HTTPException(400, f"node {key} has {ranks} rank(s); at least {low} cannot be asked")
        if key in req.rankMax and req.rankMax[key] < low:
            raise HTTPException(400, f"node {key}: at least {low} and at most {req.rankMax[key]} ranks contradict")
        if int(key) in set(req.mustNotHave):
            raise HTTPException(400, f"node {key} is both excluded and given a minimum rank")
    for key, high in req.rankMax.items():
        ranks = graph["ranks"].get(int(key), 1)
        if not 0 <= high < ranks:
            raise HTTPException(400, f"node {key} has {ranks} rank(s); a cap must be below that, from 0")
    if req.points > graph["slots"]:
        raise HTTPException(
            400,
            f"{req.treeKey!r} has only {graph['slots']} point slots at level cap "
            f"{req.levelCap}; {req.points} points cannot be spent in it")


def _count_for(req: "CountRequest", graph: dict):
    """Shared by the gate and by job submission, so the number a user is shown is exactly
    the number the job is created against."""
    filtered = bool(req.mustHave or req.mustNotHave or req.choiceSides
                    or req.atLeastOneOf or req.exactlyOneOf or req.rankMin or req.rankMax)
    if not filtered:
        rows = query(
            """
            SELECT c.set_count, c.build_count
            FROM tree_counts c
            JOIN current_trees t ON t.id = c.tree_id AND t.revision = c.tree_revision
            WHERE t.key = %s AND c.points = %s AND c.level_cap = %s
            """,
            (req.treeKey, req.points, req.levelCap),
        )
        if rows:
            return int(rows[0]["set_count"]), int(rows[0]["build_count"]), "precomputed"

    from frontier_dp import count_with_groups
    common = dict(
        require=set(req.mustHave), exclude=set(req.mustNotHave),
        choice_sides={int(k): v for k, v in req.choiceSides.items()},
        at_least_one_of=req.atLeastOneOf, exactly_one_of=req.exactlyOneOf,
        rank_min={int(k): v for k, v in req.rankMin.items()},
        rank_max={int(k): v for k, v in req.rankMax.items()},
    )
    args = (graph["meta"], graph["par"], graph["chi"], graph["order"], req.points)
    sets = count_with_groups(*args, **common)
    builds = count_with_groups(*args, weight_choices=True, **common)
    return sets, builds, "computed"


def _job_row(row: dict, tree_key: str) -> "JobResponse":
    return JobResponse(
        id=str(row["id"]), state=row["state"], treeKey=tree_key,
        points=int(row["request"]["points"]),
        expectedCount=int(row["expected_count"]) if row["expected_count"] is not None else None,
        resultCount=int(row["result_count"]) if row["result_count"] is not None else None,
        progress=float(row["progress"]), phase=row.get("phase"),
        cancelRequested=bool(row.get("cancel_requested")), error=row["error"],
        createdAt=row["created_at"].isoformat(),
        finishedAt=row["finished_at"].isoformat() if row["finished_at"] else None,
    )


@app.post("/solve", response_model=JobResponse, status_code=202)
def submit_solve(req: SolveRequest) -> "JobResponse":
    """Queue a filtered enumeration -- but only if the pre-flight count says it is worth it.

    The gate runs here, not in the worker: a job is never created for a result set too
    large to serve, so the queue never has to defend against unbounded output.
    """
    import hashlib

    graph = _dp_graph(req.treeKey, req.levelCap)
    _validate(req, graph)

    # The engine's filter carries one value per talent, so it can express a single
    # at-least-one group and a single exactly-one group. Counting has no such limit, so
    # this restriction applies to listing only -- say that plainly rather than silently
    # dropping constraints the count already honoured.
    if len(req.atLeastOneOf) > 1 or len(req.exactlyOneOf) > 1:
        raise HTTPException(
            400,
            "listing supports at most one atLeastOneOf and one exactlyOneOf group "
            "(the engine's filter holds one value per talent). Counting these is "
            "supported -- use /counts.")
    overlap = set().union(*req.atLeastOneOf, set()) & set().union(*req.exactlyOneOf, set())
    if overlap:
        raise HTTPException(
            400, f"node id(s) {sorted(overlap)} appear in both group constraints")

    sets, builds, _ = _count_for(req, graph)

    if sets == 0:
        raise HTTPException(400, "no builds match these constraints")
    if sets > req.maxResults:
        raise HTTPException(
            413,
            f"{sets:,} matching selections ({builds:,} builds) exceeds the limit of "
            f"{req.maxResults:,}. Add constraints or lower the point budget.")

    payload = {
        "points": req.points, "levelCap": req.levelCap,
        "mustHave": sorted(req.mustHave), "mustNotHave": sorted(req.mustNotHave),
        "choiceSides": {str(k): v for k, v in sorted(req.choiceSides.items())},
        "atLeastOneOf": [sorted(g) for g in req.atLeastOneOf],
        "exactlyOneOf": [sorted(g) for g in req.exactlyOneOf],
        "maxResults": req.maxResults, "timeBudgetMs": req.timeBudgetMs,
    }
    # Only when used, so a request without rank limits hashes as it did.
    if req.rankMin:
        payload["rankMin"] = {str(k): v for k, v in sorted(req.rankMin.items())}
    if req.rankMax:
        payload["rankMax"] = {str(k): v for k, v in sorted(req.rankMax.items())}
    # Dedup and cache are the same mechanism. The hash covers the exact tree revision, so
    # a new ingest revision correctly yields a different job rather than a stale hit -- and
    # the solver contract, so a change to what a stored result *means* does too.
    digest = hashlib.sha256(
        f"v{SOLVE_CONTRACT}:{graph['tree_id']}:{graph['revision']}:"
        f"{json.dumps(payload, sort_keys=True)}".encode()).digest()

    from psycopg.rows import dict_row
    with pool().connection() as conn:
        with conn.cursor(row_factory=dict_row) as cur:
            cur.execute(
                "SELECT * FROM solve_jobs WHERE request_hash = %s "
                "AND state IN ('queued','running','done')", (digest,))
            existing = cur.fetchone()
            if existing:
                return _job_row(existing, req.treeKey)
            cur.execute(
                """
                INSERT INTO solve_jobs (tree_id, tree_revision, request, request_hash,
                                        expected_count)
                VALUES (%s, %s, %s, %s, %s) RETURNING *
                """,
                (graph["tree_id"], graph["revision"], json.dumps(payload), digest, sets))
            row = cur.fetchone()
            conn.commit()
    return _job_row(row, req.treeKey)


@app.get("/solve/{job_id}", response_model=JobResponse)
def get_job(job_id: str) -> "JobResponse":
    from psycopg.rows import dict_row
    with pool().connection() as conn:
        with conn.cursor(row_factory=dict_row) as cur:
            cur.execute(
                """
                SELECT j.*, t.key AS tree_key
                FROM solve_jobs j JOIN trees t
                  ON t.id = j.tree_id AND t.revision = j.tree_revision
                WHERE j.id = %s
                """, (job_id,))
            row = cur.fetchone()
    if not row:
        raise HTTPException(404, f"no job {job_id}")
    return _job_row(row, row["tree_key"])


@app.post("/solve/{job_id}/cancel", response_model=JobResponse)
def cancel_job(job_id: str) -> "JobResponse":
    """Stop a job, if it can still be stopped.

    A queued job is cancelled outright. A running one is *asked* to stop: a worker holds
    it and a solver process is running, and neither can be stopped by writing to a table,
    so this records the request and the worker acts on it within about a second. The
    response says which happened -- `state` is already `cancelled`, or `cancelRequested`
    is set and `state` is still `running`.

    One statement, because the alternative has a race: read the state, decide, write. A
    worker can claim a queued job in that gap, and the write would then mark a running
    job cancelled without anything telling the worker to stop.
    """
    from psycopg.rows import dict_row
    with pool().connection() as conn:
        with conn.cursor(row_factory=dict_row) as cur:
            cur.execute(
                """
                UPDATE solve_jobs SET
                    cancel_requested = true,
                    state       = CASE WHEN state = 'queued' THEN 'cancelled'
                                       ELSE state END,
                    finished_at = CASE WHEN state = 'queued' THEN now()
                                       ELSE finished_at END
                WHERE id = %s AND state IN ('queued', 'running')
                RETURNING *, (SELECT key FROM trees
                              WHERE id = tree_id AND revision = tree_revision) AS tree_key
                """, (job_id,))
            row = cur.fetchone()
            conn.commit()
            if row:
                return _job_row(row, row["tree_key"])

            # Nothing was updated: either the job does not exist, or it already finished.
            # Cancelling a cancelled job is not an error -- a client that retries should
            # get the same answer, not a failure.
            cur.execute(
                """
                SELECT j.*, t.key AS tree_key FROM solve_jobs j JOIN trees t
                  ON t.id = j.tree_id AND t.revision = j.tree_revision
                WHERE j.id = %s
                """, (job_id,))
            row = cur.fetchone()
    if not row:
        raise HTTPException(404, f"no job {job_id}")
    if row["state"] == "cancelled":
        return _job_row(row, row["tree_key"])
    raise HTTPException(409, f"job is already {row['state']} and cannot be cancelled")


@app.get("/solve/{job_id}/results")
def get_job_results(job_id: str, offset: int = 0, limit: int = Query(100, ge=1, le=1000)):
    """A page of matching builds, each keyed by Blizzard nodeId."""
    rows = query("SELECT state, result_count FROM solve_jobs WHERE id = %s", (job_id,))
    if not rows:
        raise HTTPException(404, f"no job {job_id}")
    if rows[0]["state"] not in ("done", "capped"):
        raise HTTPException(409, f"job is {rows[0]['state']}, results are not ready")
    results = query(
        "SELECT ordinal, points FROM solve_results WHERE job_id = %s "
        "ORDER BY ordinal OFFSET %s LIMIT %s", (job_id, offset, limit))
    return {
        "jobId": job_id, "state": rows[0]["state"],
        "total": int(rows[0]["result_count"] or 0), "offset": offset,
        "builds": [r["points"] for r in results],
    }




# ---------------------------------------------------------------------------
# popular builds, from WarcraftLogs
# ---------------------------------------------------------------------------

POPULAR_TTL_SECONDS = 6 * 3600
CONTENT_TTL_SECONDS = 24 * 3600


def _cached(key: str, ttl: int, compute):
    """Serve a stored answer while it is fresh; compute and store it otherwise."""
    rows = query(
        "SELECT body FROM wcl_cache WHERE key = %s AND fetched_at > now() - make_interval(secs => %s)",
        (key, ttl),
    )
    if rows:
        return rows[0]["body"]
    body = compute()
    with pool().connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                "INSERT INTO wcl_cache (key, body, fetched_at) VALUES (%s, %s, now()) "
                "ON CONFLICT (key) DO UPDATE SET body = EXCLUDED.body, fetched_at = now()",
                (key, json.dumps(body)),
            )
    return body


def _wcl_call(fn):
    from ttm_ingest import wcl

    try:
        return fn()
    except wcl.NotConfigured as exc:
        raise HTTPException(503, str(exc)) from None
    except wcl.WclError as exc:
        raise HTTPException(502, f"WarcraftLogs: {exc}") from None


@app.get("/popular/content")
def popular_content() -> list[dict[str, Any]]:
    """The raids and Mythic+ season that top-player builds can be read from right now."""
    from ttm_ingest import wcl

    return _wcl_call(lambda: _cached("content", CONTENT_TTL_SECONDS, wcl.current_content))


@app.get("/popular/{spec_key:path}")
def popular_builds(
    spec_key: str,
    zone: int,
    encounter: str = "all",
    difficulty: int = 5,
    pages: int = Query(1, ge=1, le=3),
) -> dict[str, Any]:
    """What the top-ranked players of a specialisation run, in this tool's terms.

    Every ranked player's talents become a build on our trees -- points per tree, a side for
    each choice node, which hero tree -- and the set is summarised three ways: how often each
    talent is taken, how the hero trees split, and which exact builds recur. Each build is also
    checked against our copy of the trees; one the live game allowed and our data does not is
    a sign the data has drifted, and is reported rather than hidden.
    """
    from ttm_ingest import wcl

    rows = query("SELECT definition FROM current_trees WHERE key = %s AND kind = 'spec'", (spec_key,))
    if not rows:
        raise HTTPException(404, f"no current spec tree {spec_key!r}")
    spec = rows[0]["definition"]
    group = query(
        "SELECT definition FROM current_trees WHERE game = 'retail' AND class_name = %s "
        "AND spec_name = %s", (spec["className"], spec["specName"]),
    )
    trees = [r["definition"] for r in group]
    by_key = {t["key"]: t for t in trees}

    def compute() -> dict[str, Any]:
        content = _cached("content", CONTENT_TTL_SECONDS, wcl.current_content)
        zone_info = next((z for z in content if z["zoneId"] == zone), None)
        if zone_info is None:
            raise HTTPException(404, f"zone {zone} is not current content")
        encounters = (
            zone_info["encounters"] if encounter == "all"
            else [e for e in zone_info["encounters"] if str(e["id"]) == encounter]
        )
        if not encounters:
            raise HTTPException(404, f"no encounter {encounter} in {zone_info['name']}")

        index = wcl.entry_index(trees)
        players = []
        for enc in encounters:
            for row in wcl.rankings(spec["className"], spec["specName"], enc["id"], difficulty, pages):
                build = wcl.convert(row, index)
                build["encounter"] = enc["name"]
                players.append(build)
        return _summarise(players, by_key, zone_info, encounters, difficulty)

    key = f"popular:v3:{spec_key}:{zone}:{encounter}:{difficulty}:{pages}"
    return _wcl_call(lambda: _cached(key, POPULAR_TTL_SECONDS, compute))


def _summarise(players, by_key, zone_info, encounters, difficulty) -> dict[str, Any]:
    from collections import Counter, defaultdict
    from ttm_ingest import wcl

    n = len(players)
    heroes = Counter()
    taken = Counter()
    ranks = defaultdict(int)
    sides: dict[str, Counter] = defaultdict(Counter)
    builds: dict[str, dict] = {}
    illegal: list[str] = []
    for p in players:
        hero_keys = [k for k in p["points"] if "/hero/" in k]
        hero = hero_keys[0] if hero_keys else None
        heroes[hero] += 1
        for key, pts in p["points"].items():
            tree = by_key.get(key)
            problem = wcl.problems(tree, pts, tree.get("pointCap")) if tree else "unknown tree"
            if problem and len(illegal) < 12:
                illegal.append(f"{p['name']} ({key.rsplit('/', 1)[-1] if '/hero/' not in key else 'hero'}): {problem}")
            for node, rank in pts.items():
                taken[node] += 1
                ranks[node] += rank
        for node, side in p["choices"].items():
            sides[node][side] += 1
        signature = json.dumps([sorted((k, sorted(v.items())) for k, v in p["points"].items()),
                                sorted(p["choices"].items())])
        entry = builds.setdefault(signature, {
            "count": 0, "best": None, "amounts": [], "points": p["points"], "choices": p["choices"],
            "hero": hero, "example": p["name"],
        })
        entry["count"] += 1
        entry["amounts"].append(p["amount"] or 0)

    # Every distinct build, most common first -- not a top few. They are what the page offers
    # to sim as they are: real, game-accepted loadouts, with no combinatorics in between.
    common = sorted(builds.values(), key=lambda b: (-b["count"], -max(b["amounts"])))
    for b in common:
        amounts = sorted(b.pop("amounts"))
        b["best"] = amounts[-1]
        b["median"] = amounts[len(amounts) // 2]
    return {
        "zone": {"id": zone_info["zoneId"], "name": zone_info["name"]},
        "encounters": [e["name"] for e in encounters],
        "difficulty": difficulty,
        "players": n,
        "heroes": [{"key": k, "count": c} for k, c in heroes.most_common()],
        # nodeId -> share of players taking it, and the mean rank among those who do.
        "pickRates": {node: {"share": c / n, "meanRank": ranks[node] / c} for node, c in taken.items()} if n else {},
        # nodeId -> [share on side 0, share on side 1], among players taking the node.
        "choiceSides": {node: [s[0] / sum(s.values()), s[1] / sum(s.values())] for node, s in sides.items()},
        "builds": common,
        "distinctBuilds": len(builds),
        "illegal": illegal,
        "fetchedAt": time.time(),
    }

# ---------------------------------------------------------------------------
# custom trees
# ---------------------------------------------------------------------------

CUSTOM_REVISION = 1
CUSTOM_BODY_LIMIT = 400_000


def _summary(t: dict) -> dict[str, Any]:
    return {
        "key": t["key"], "kind": t["kind"], "name": t["name"], "className": t["className"],
        "specName": t.get("specName"), "subTreeId": t.get("subTreeId"), "nodeCount": t["nodeCount"],
        "maxPointsInTree": t["maxPointsInTree"], "pointCap": t["pointCap"], "order": t["order"],
        "heroSpecs": t.get("heroSpecs"),
    }


@app.post("/custom-trees", status_code=201)
async def save_custom_project(request: Request) -> dict[str, Any]:
    """Save a project from the tree editor, and return its id and trees.

    Content-addressed: the id is the hash of the design, so saving the same design twice is
    the same project, and an edit is a new one. Nothing is overwritten and nothing belongs to
    anyone -- whoever has the id can open it, and the browser remembers which ids are yours.
    """
    from ttm_ingest import custom

    body = await request.body()
    if len(body) > CUSTOM_BODY_LIMIT:
        raise HTTPException(413, f"a project is at most {CUSTOM_BODY_LIMIT // 1000} kB")
    try:
        canon = custom.canonical(json.loads(body))
        pid, trees, warnings = custom.build(canon)
    except json.JSONDecodeError:
        raise HTTPException(400, "the body is not JSON") from None
    except custom.CustomTreeError as exc:
        raise HTTPException(400, str(exc)) from None

    with pool().connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                "INSERT INTO custom_projects (id, name, tree_count, shared_pool, design) "
                "VALUES (%s, %s, %s, %s, %s) "
                "ON CONFLICT (id) DO UPDATE SET design = EXCLUDED.design",
                (pid, canon["name"], len(trees), canon["sharedPointCap"], json.dumps(canon)),
            )
            cur.executemany(
                """
                INSERT INTO trees (
                    id, revision, key, kind, game, gating, class_id, spec_id, class_name,
                    spec_name, sub_tree_id, name, definition, point_cap, max_points_in_tree,
                    node_count
                ) VALUES (%s, %s, %s, %s, 'custom', 'reqPoints', NULL, NULL, %s, %s, %s,
                          %s, %s, %s, %s, %s)
                ON CONFLICT (id, revision) DO UPDATE SET
                    kind = EXCLUDED.kind, spec_name = EXCLUDED.spec_name,
                    sub_tree_id = EXCLUDED.sub_tree_id,
                    definition = EXCLUDED.definition, point_cap = EXCLUDED.point_cap,
                    max_points_in_tree = EXCLUDED.max_points_in_tree,
                    node_count = EXCLUDED.node_count
                """,
                # Same id means same design, so refreshing the derived record is always safe
                # -- and it is how a change to the derivation reaches trees saved before it.
                [
                    (t["id"], CUSTOM_REVISION, t["key"], t["kind"], t["className"], t["specName"],
                     t["subTreeId"], t["name"], json.dumps(t), t["pointCap"], t["maxPointsInTree"],
                     t["nodeCount"])
                    for t in trees
                ],
            )
    return {
        "project": pid,
        "name": canon["name"],
        "sharedPointCap": canon["sharedPointCap"],
        "trees": [_summary(t) for t in trees],
        "warnings": warnings,
    }


@app.get("/custom-trees/{project}")
def get_custom_project(project: str) -> dict[str, Any]:
    """A saved project: its trees, and its design in the editor's shape to keep editing."""
    from ttm_ingest import custom

    if not re.fullmatch(r"[0-9a-f]{16}", project):
        raise HTTPException(400, "not a project id")
    head = query("SELECT name, shared_pool, design FROM custom_projects WHERE id = %s", (project,))
    if not head:
        raise HTTPException(404, f"no custom project {project!r}")
    rows = query(
        "SELECT definition FROM current_trees WHERE game = 'custom' AND key LIKE %s ORDER BY key",
        (f"custom/{project}/%",),
    )
    # By place, not by key: as text, tree 10 of a retail project sorts before tree 2.
    trees = sorted((r["definition"] for r in rows), key=lambda t: t["order"])
    return {
        "project": project,
        "name": head[0]["name"],
        "sharedPointCap": head[0]["shared_pool"],
        "trees": [_summary(t) for t in trees],
        # The stored design, which is the source of truth; derived only for projects that
        # predate keeping it.
        "design": head[0]["design"] or custom.editable(trees, head[0]["name"], head[0]["shared_pool"]),
    }


@app.get("/icons")
def list_icons(search: str = "", limit: int = Query(80, ge=1, le=300)) -> list[str]:
    """Names of cached icons, for the tree editor's picker. Only these can be drawn."""
    from ttm_ingest import icons as icon_source

    needle = "".join(ch for ch in search.lower() if ch.isalnum() or ch in "_-")
    rows = query(
        "SELECT DISTINCT name FROM icons WHERE status = 200 AND size = 56 AND name LIKE %s "
        "ORDER BY name LIMIT %s",
        (f"%{needle}%", limit),
    )
    del icon_source
    return [r["name"] for r in rows]


@app.get("/icons/{name}", responses={200: {"content": {"image/jpeg": {}}}})
def get_icon(name: str, request: Request,
             size: int = Query(56, description="18, 36 or 56")) -> Response:
    """One talent icon, cached hard.

    Served per file rather than as an atlas, which is the point of the whole icon layer: a
    browser caches each icon separately and drawing one tree does not mean downloading
    every icon in the game. An icon never changes under a name, so the response is
    `immutable` with a one-year lifetime and an ETag -- after the first visit a tree canvas
    makes no icon requests at all.

    A 404 here is expected and survivable. Upstream does not have art for every name the
    talent payload uses (19 of 2,094 today), so a client must render a talent without its
    icon rather than treat this as an error.

    The `.jpg` a browser will happily append is stripped by the same normaliser the ingest
    uses, so `/icons/foo` and `/icons/foo.jpg` are the same icon.
    """
    from ttm_ingest import icons as icon_source

    try:
        key = icon_source.normalise(name)
    except icon_source.IconNameError:
        raise HTTPException(400, f"not an icon name: {name!r}") from None
    if size not in icon_source.SIZES:
        raise HTTPException(400, f"size must be one of {list(icon_source.SIZES)}")

    rows = query(
        "SELECT content_type, bytes, etag FROM icons "
        "WHERE name = %s AND size = %s AND status = 200", (key, size))
    if not rows:
        raise HTTPException(404, f"no cached icon {key!r} at {size}px")

    row = rows[0]
    etag = f'"{row["etag"]}"'
    headers = {
        "Cache-Control": f"public, max-age={ICON_CACHE_SECONDS}, immutable",
        "ETag": etag,
    }
    # A revalidating client gets no body. Cheap to honour and it is what `immutable`
    # promises, so honour it rather than shipping the bytes again.
    if request.headers.get("if-none-match") == etag:
        return Response(status_code=304, headers=headers)
    return Response(content=bytes(row["bytes"]), media_type=row["content_type"],
                    headers=headers)


@app.get("/solve/{job_id}/stats")
def get_job_stats(job_id: str) -> dict[str, Any]:
    """How often each talent appears across every build the job produced.

    This is what exhaustive enumeration buys that sampling cannot: a statement about the
    whole matching set rather than about a draw from it. "Every one of these 34,619 builds
    takes Eclipse" is a fact; "42% take Shooting Stars" locates the decision that is
    actually open.

    `share` is the fraction of results containing the talent at all. `meanPoints` is its
    average rank among the builds that take it, which separates "always taken, at one of
    two ranks" from "always maxed". `mandatory` marks the talents your constraints and the
    tree's own structure have already decided for you -- the ones worth *not* thinking
    about.
    """
    rows = query(
        "SELECT state, result_count FROM solve_jobs WHERE id = %s", (job_id,))
    if not rows:
        raise HTTPException(404, f"no job {job_id}")
    state, total = rows[0]["state"], int(rows[0]["result_count"] or 0)
    if state not in ("done", "capped"):
        raise HTTPException(409, f"job is {state}, statistics are not ready")

    stats = query(
        "SELECT node_id, builds, points FROM solve_stats WHERE job_id = %s "
        "ORDER BY builds DESC, node_id", (job_id,))

    return {
        "jobId": job_id,
        "state": state,
        "total": total,
        "talents": [
            {
                "nodeId": int(row["node_id"]),
                "builds": int(row["builds"]),
                "share": round(int(row["builds"]) / total, 6) if total else 0.0,
                "meanPoints": round(int(row["points"]) / int(row["builds"]), 3),
                "mandatory": total > 0 and int(row["builds"]) == total,
            }
            for row in stats
        ],
    }


@app.post("/counts/spread", response_model=SpreadResponse)
def count_spread(req: CountRequest) -> SpreadResponse:
    """Counts for every point total up to `points` (or the tree's slots), under a search.

    For trees that share a point pool, as WoW Forever's three tabs share 51: a class's count
    is a sum over every split of the pool across its tabs, so each tab's count at *every*
    total is what the planner needs -- and the DP yields all of them in the one run it takes
    to yield one.
    """
    started = time.perf_counter()
    graph = _dp_graph(req.treeKey, req.levelCap)
    req = req.model_copy(update={"points": min(req.points, graph["slots"])})
    _validate(req, graph)
    from frontier_dp import count_spread as spread
    args = (graph["meta"], graph["par"], graph["chi"], graph["order"], req.points)
    common = dict(
        require=set(req.mustHave), exclude=set(req.mustNotHave),
        choice_sides={int(k): v for k, v in req.choiceSides.items()},
        at_least_one_of=req.atLeastOneOf, exactly_one_of=req.exactlyOneOf,
        rank_min={int(k): v for k, v in req.rankMin.items()},
        rank_max={int(k): v for k, v in req.rankMax.items()},
    )
    return SpreadResponse(
        treeKey=req.treeKey, levelCap=req.levelCap,
        sets=spread(*args, **common), builds=spread(*args, weight_choices=True, **common),
        elapsedMs=round((time.perf_counter() - started) * 1000, 2),
    )


@app.post("/counts", response_model=CountResponse)
def count_builds(req: CountRequest) -> CountResponse:
    """The pre-flight gate: how many builds match these constraints?

    This is what makes the product model work. It is free enough to answer on every
    keystroke while a user paints constraints, and it tells the UI whether listing the
    matches is worth offering before any worker is involved.
    """
    started = time.perf_counter()
    graph = _dp_graph(req.treeKey, req.levelCap)
    _validate(req, graph)
    filtered = bool(req.mustHave or req.mustNotHave or req.choiceSides
                    or req.atLeastOneOf or req.exactlyOneOf or req.rankMin or req.rankMax)
    sets, builds, source = _count_for(req, graph)

    return CountResponse(
        treeKey=req.treeKey, points=req.points, levelCap=req.levelCap,
        sets=sets, builds=builds, filtered=filtered, source=source,
        elapsedMs=round((time.perf_counter() - started) * 1000, 2),
        listable=0 < sets <= LISTING_LIMIT,
        listingLimit=LISTING_LIMIT,
    )
