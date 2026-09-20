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
import os
import sys
import time
from typing import Annotated, Any, Literal

from fastapi import Depends, FastAPI, HTTPException, Query
from pydantic import BaseModel, Field, field_validator

# The frontier DP lives in tools/ as the verified reference implementation; the API uses it
# rather than reimplementing counting.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))), "tools", "frontier-dp"))
sys.path.insert(0, os.path.join("tools", "frontier-dp"))

DEFAULT_LEVEL_CAP = 90

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

    @field_validator("mustHave", "mustNotHave")
    @classmethod
    def _cap_filter_size(cls, v: list[int]) -> list[int]:
        if len(v) > 64:
            raise ValueError("at most 64 node constraints")
        return v


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


# ---------------------------------------------------------------------------
# endpoints
# ---------------------------------------------------------------------------

@app.get("/health")
def health() -> dict[str, Any]:
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
            WHERE promoted_at IS NOT NULL
            ORDER BY revision DESC LIMIT 1
            """
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
        "dataAgeSeconds": run["age_seconds"],
    }


@app.get("/trees", response_model=list[TreeSummary])
def list_trees(
    game: str = "retail",
    kind: str | None = Query(None, description="class, spec or hero"),
    classId: int | None = None,
    specId: int | None = None,
) -> list[TreeSummary]:
    """Every tree in the promoted revision, optionally narrowed."""
    sql = ["SELECT key, kind::text, name, class_name, spec_name, sub_tree_id,",
           "       node_count, max_points_in_tree, point_cap",
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
    sql.append("ORDER BY class_name, spec_name, kind, sub_tree_id")

    return [
        TreeSummary(
            key=r["key"], kind=r["kind"], name=r["name"],
            className=r["class_name"], specName=r["spec_name"],
            subTreeId=r["sub_tree_id"], nodeCount=r["node_count"],
            maxPointsInTree=r["max_points_in_tree"], pointCap=r["point_cap"],
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


@app.post("/counts", response_model=CountResponse)
def count_builds(req: CountRequest) -> CountResponse:
    """The pre-flight gate: how many builds match these constraints?

    This is what makes the product model work. It is free enough to answer on every
    keystroke while a user paints constraints, and it tells the UI whether listing the
    matches is worth offering before any worker is involved.
    """
    started = time.perf_counter()
    graph = _dp_graph(req.treeKey, req.levelCap)

    unknown = (set(req.mustHave) | set(req.mustNotHave)) - graph["node_ids"]
    unknown |= {int(k) for k in req.choiceSides} - graph["node_ids"]
    if unknown:
        raise HTTPException(
            400,
            f"node id(s) {sorted(unknown)[:5]} are not in {req.treeKey!r}",
        )
    contradictory = set(req.mustHave) & set(req.mustNotHave)
    if contradictory:
        raise HTTPException(
            400, f"node id(s) {sorted(contradictory)} are both required and excluded"
        )

    if req.points > graph["slots"]:
        raise HTTPException(
            400,
            f"{req.treeKey!r} has only {graph['slots']} point slots at level cap "
            f"{req.levelCap}; {req.points} points cannot be spent in it",
        )

    filtered = bool(req.mustHave or req.mustNotHave or req.choiceSides)

    if not filtered:
        # Precomputed: a primary-key read rather than a computation.
        rows = query(
            """
            SELECT c.set_count, c.build_count
            FROM tree_counts c
            JOIN current_trees t ON t.id = c.tree_id AND t.revision = c.tree_revision
            WHERE t.key = %s AND c.points = %s AND c.level_cap = %s
            """,
            (req.treeKey, req.points, req.levelCap),
        )
        sets = int(rows[0]["set_count"]) if rows else 0
        builds = int(rows[0]["build_count"]) if rows else 0
        source: Literal["precomputed", "computed"] = "precomputed"
    else:
        from frontier_dp import count_frontier_dp
        sides = {int(k): v for k, v in req.choiceSides.items()}
        common = dict(require=set(req.mustHave), exclude=set(req.mustNotHave),
                      choice_sides=sides)
        set_totals, _ = count_frontier_dp(
            graph["meta"], graph["par"], graph["chi"], graph["order"], req.points, **common
        )
        build_totals, _ = count_frontier_dp(
            graph["meta"], graph["par"], graph["chi"], graph["order"], req.points,
            weight_choices=True, **common
        )
        sets = set_totals.get(req.points, 0)
        builds = build_totals.get(req.points, 0)
        source = "computed"

    return CountResponse(
        treeKey=req.treeKey, points=req.points, levelCap=req.levelCap,
        sets=sets, builds=builds, filtered=filtered, source=source,
        elapsedMs=round((time.perf_counter() - started) * 1000, 2),
        listable=0 < sets <= LISTING_LIMIT,
        listingLimit=LISTING_LIMIT,
    )
