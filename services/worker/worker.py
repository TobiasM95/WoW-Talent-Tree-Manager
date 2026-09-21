#!/usr/bin/env python3
"""Solve worker: claim a job, run the C++ engine, store the builds.

    python services/worker/worker.py [--once] [--solver PATH]

Claims rows from `solve_jobs` with FOR UPDATE SKIP LOCKED, renders the tree into the
engine's format, execs `ttm-solver`, decodes its output into nodeId-keyed builds, and
writes them back. Also requeues jobs whose lease has expired, so a killed worker does not
strand work.

Every job arrives with a known result size, because the API's pre-flight count gated it.
That is why this can store results normally instead of streaming to object storage: the
unbounded case was refused before a row was ever inserted.

Exit is clean on SIGTERM so `docker compose down` does not orphan a running solve.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for path in (os.path.join(REPO, "services", "ingest"), os.path.join("services", "ingest")):
    sys.path.insert(0, path)

from ttm_ingest import ttm_format  # noqa: E402

POLL_SECONDS = 2.0
LEASE_SECONDS = 600
MAX_ATTEMPTS = 3

# Defaults; a job's request may lower them but never raise them past these.
DEFAULT_TIME_BUDGET_MS = 120_000
DEFAULT_MAX_RESULTS = 2_000_000

COUNT_RE = re.compile(r"Tree 0: (\d+) combinations")
_stopping = False


def _stop(_signum, _frame):
    global _stopping
    _stopping = True
    print("worker: stop requested, finishing current job", flush=True)


class SolveFailed(RuntimeError):
    """The solve could not be completed. The message reaches the user."""


# ---------------------------------------------------------------------------
# decoding the engine's output
# ---------------------------------------------------------------------------

def decode_results(output_path: str, tree: dict, limit: int) -> list[dict[str, int]]:
    """Turn the engine's output into nodeId-keyed point maps.

    The engine emits a header listing, per bit, the positional talent index that bit
    belongs to, then one line per set: the raw SIND, followed by the indices of any choice
    nodes present.

    Multi-rank talents occupy several bits that all map to the same talent, so a node's
    point total is how many of its bits are set -- which is why this counts rather than
    flags. Output rows are positional; what we store is keyed by Blizzard nodeId, because
    positional references are exactly what made shared builds unsafe before.
    """
    node_ids = [n["nodeId"] for n in tree["nodes"]]

    with open(output_path, encoding="utf-8") as handle:
        header = handle.readline().strip()
        if not header:
            return []
        try:
            bit_to_index = [int(x) for x in header.split("/") if x != ""]
        except ValueError as exc:
            raise SolveFailed(f"unreadable result header: {header[:80]!r}") from exc

        builds: list[dict[str, int]] = []
        for line in handle:
            line = line.strip()
            if not line:
                continue
            raw = line.split(",", 1)[0]
            try:
                mask = int(raw)
            except ValueError as exc:
                raise SolveFailed(f"unreadable result row: {line[:80]!r}") from exc

            points: dict[str, int] = {}
            for bit, positional in enumerate(bit_to_index):
                if mask & (1 << bit):
                    if positional >= len(node_ids):
                        raise SolveFailed(
                            f"result references talent index {positional}, but the tree "
                            f"has {len(node_ids)}"
                        )
                    key = str(node_ids[positional])
                    points[key] = points.get(key, 0) + 1
            builds.append(points)
            if len(builds) >= limit:
                break
    return builds


def build_filter_string(tree: dict, must_have: list[int], must_not_have: list[int],
                        at_least_one_of: list[list[int]] | None = None,
                        exactly_one_of: list[list[int]] | None = None) -> str:
    """The engine's filter is positional over the tree's node order.

    Sentinel values, per Engine/src/TreeSolver.cpp:
        >0  this talent must have that many points
        -1  must have none
        -2  member of the "at least one of these" group
        -3  member of the "exactly one of these" group

    Because a talent holds a single value, the engine supports one group of each kind --
    which is why the API refuses more than one before a job is ever created.
    """
    order = [n["nodeId"] for n in tree["nodes"]]
    values = ["0"] * len(order)
    index_of = {nid: i for i, nid in enumerate(order)}
    for nid in must_have:
        values[index_of[nid]] = "1"
    for nid in must_not_have:
        values[index_of[nid]] = "-1"
    for group in (at_least_one_of or []):
        for nid in group:
            values[index_of[nid]] = "-2"
    for group in (exactly_one_of or []):
        for nid in group:
            values[index_of[nid]] = "-3"
    return ":".join(values)


# ---------------------------------------------------------------------------
# running a job
# ---------------------------------------------------------------------------

def run_solve(solver: str, tree: dict, request: dict, workdir: str) -> tuple[list, dict]:
    points = int(request["points"])
    level_cap = int(request.get("levelCap", 90))
    must_have = [int(x) for x in request.get("mustHave", [])]
    must_not_have = [int(x) for x in request.get("mustNotHave", [])]
    at_least_one_of = [[int(x) for x in g] for g in request.get("atLeastOneOf", [])]
    exactly_one_of = [[int(x) for x in g] for g in request.get("exactlyOneOf", [])]
    at_least_one_of = [[int(x) for x in g] for g in request.get("atLeastOneOf", [])]
    exactly_one_of = [[int(x) for x in g] for g in request.get("exactlyOneOf", [])]
    time_budget = min(int(request.get("timeBudgetMs", DEFAULT_TIME_BUDGET_MS)),
                      DEFAULT_TIME_BUDGET_MS)
    max_results = min(int(request.get("maxResults", DEFAULT_MAX_RESULTS)),
                      DEFAULT_MAX_RESULTS)

    structure = os.path.join(workdir, "tree.txt")
    output = os.path.join(workdir, "results.txt")
    with open(structure, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(ttm_format.tree_to_structure_line(tree, level_cap=level_cap) + "\n")

    args = [
        solver,
        "--structure-file-path", structure,
        "--structure-indices", "0",
        "--target-talent-count", str(points),
        "--output-file-path", output,
        "--max-results", str(max_results),
        "--time-budget-ms", str(time_budget),
    ]
    if must_have or must_not_have or at_least_one_of or exactly_one_of:
        args += ["--filter", build_filter_string(tree, must_have, must_not_have,
                                                 at_least_one_of, exactly_one_of)]

    started = time.monotonic()
    proc = subprocess.run(args, capture_output=True, text=True,
                          timeout=(time_budget / 1000) + 60)
    elapsed = time.monotonic() - started

    if proc.returncode != 0:
        raise SolveFailed(
            f"solver exited {proc.returncode}: {(proc.stderr or proc.stdout)[:300]}"
        )
    if "No valid trees found" in proc.stdout:
        raise SolveFailed("the engine rejected the generated tree")

    match = COUNT_RE.search(proc.stdout)
    if not match:
        raise SolveFailed(f"no count in solver output: {proc.stdout[-200:]!r}")

    meta = {
        "reported": int(match.group(1)),
        "timedOut": "time budget exceeded" in proc.stdout,
        "capped": "safety guard triggered" in proc.stdout,
        "elapsed": elapsed,
    }
    builds = decode_results(output, tree, max_results) if os.path.exists(output) else []
    return builds, meta


# ---------------------------------------------------------------------------
# queue
# ---------------------------------------------------------------------------

def claim(conn, worker_id: str):
    """Take the oldest queued job. SKIP LOCKED lets workers scale without coordination."""
    from psycopg.rows import dict_row
    with conn.cursor(row_factory=dict_row) as cur:
        cur.execute(
            """
            UPDATE solve_jobs SET state = 'running', locked_by = %s, locked_at = now(),
                                  attempts = attempts + 1
            WHERE id = (
                SELECT id FROM solve_jobs WHERE state = 'queued'
                ORDER BY priority, created_at
                FOR UPDATE SKIP LOCKED LIMIT 1
            )
            RETURNING id, tree_id, tree_revision, request, attempts, expected_count
            """,
            (worker_id,),
        )
        job = cur.fetchone()
        conn.commit()
        return job


def load_tree(conn, tree_id, revision) -> dict:
    from psycopg.rows import dict_row
    with conn.cursor(row_factory=dict_row) as cur:
        cur.execute(
            "SELECT definition FROM trees WHERE id = %s AND revision = %s",
            (tree_id, revision),
        )
        row = cur.fetchone()
    if not row:
        raise SolveFailed(f"tree {tree_id} revision {revision} is missing")
    return row["definition"]


def finish(conn, job_id, state, *, result_count=None, error=None, builds=None):
    with conn.cursor() as cur:
        if builds:
            cur.executemany(
                "INSERT INTO solve_results (job_id, ordinal, points) VALUES (%s, %s, %s)",
                [(job_id, i, json.dumps(b)) for i, b in enumerate(builds)],
            )
        cur.execute(
            """
            UPDATE solve_jobs
            SET state = %s, result_count = %s, error = %s, finished_at = now(),
                progress = 1, locked_by = NULL, locked_at = NULL
            WHERE id = %s
            """,
            (state, result_count, error, job_id),
        )
    conn.commit()


def requeue_expired(conn) -> int:
    """Return jobs whose worker died to the queue; fail them once they run out of tries.

    Without this a killed worker strands a job in `running` forever, which is
    indistinguishable to a user from one that is simply slow.
    """
    with conn.cursor() as cur:
        cur.execute(
            """
            UPDATE solve_jobs SET state = 'queued', locked_by = NULL, locked_at = NULL
            WHERE state = 'running' AND locked_at < now() - make_interval(secs => %s)
              AND attempts < %s
            """,
            (LEASE_SECONDS, MAX_ATTEMPTS),
        )
        requeued = cur.rowcount
        cur.execute(
            """
            UPDATE solve_jobs
            SET state = 'failed', finished_at = now(), locked_by = NULL, locked_at = NULL,
                error = 'abandoned after ' || attempts || ' attempts'
            WHERE state = 'running' AND locked_at < now() - make_interval(secs => %s)
              AND attempts >= %s
            """,
            (LEASE_SECONDS, MAX_ATTEMPTS),
        )
        failed = cur.rowcount
    conn.commit()
    return requeued + failed


def process_one(conn, solver: str, worker_id: str) -> bool:
    job = claim(conn, worker_id)
    if not job:
        return False

    job_id = job["id"]
    print(f"worker: claimed {job_id} (attempt {job['attempts']})", flush=True)
    workdir = tempfile.mkdtemp(prefix="ttm-solve-")
    try:
        tree = load_tree(conn, job["tree_id"], job["tree_revision"])
        builds, meta = run_solve(solver, tree, job["request"], workdir)

        if meta["timedOut"] or meta["capped"]:
            # A truncated result is a distinct outcome, not a failure and not a success.
            reason = "time budget exceeded" if meta["timedOut"] else "result cap reached"
            finish(conn, job_id, "capped", result_count=len(builds), builds=builds,
                   error=f"partial result: {reason}")
            print(f"worker: {job_id} capped ({reason}), {len(builds)} builds", flush=True)
            return True

        expected = job.get("expected_count")
        if expected is not None and int(expected) != meta["reported"]:
            # The gate and the engine must agree; if they do not, the user should not be
            # handed a result that silently contradicts the count they were shown.
            raise SolveFailed(
                f"engine produced {meta['reported']} sets but the pre-flight count "
                f"predicted {int(expected)}"
            )

        finish(conn, job_id, "done", result_count=len(builds), builds=builds)
        print(f"worker: {job_id} done, {len(builds)} builds in {meta['elapsed']:.2f}s",
              flush=True)
    except SolveFailed as exc:
        finish(conn, job_id, "failed", error=str(exc)[:500])
        print(f"worker: {job_id} failed: {exc}", flush=True)
    except subprocess.TimeoutExpired:
        finish(conn, job_id, "failed", error="solver did not exit within its budget")
        print(f"worker: {job_id} failed: solver hung", flush=True)
    except Exception as exc:  # noqa: BLE001 - a worker must not die on one bad job
        finish(conn, job_id, "failed", error=f"{type(exc).__name__}: {exc}"[:500])
        print(f"worker: {job_id} failed unexpectedly: {exc}", flush=True)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="TTM solve worker.")
    parser.add_argument("--database-url", default=os.environ.get("DATABASE_URL"))
    parser.add_argument("--solver", default=os.environ.get("TTM_SOLVER", "/usr/local/bin/ttm-solver"))
    parser.add_argument("--once", action="store_true", help="process one job and exit")
    args = parser.parse_args()

    if not args.database_url:
        print("FATAL: no database URL", file=sys.stderr)
        return 2
    solver = os.path.normpath(args.solver)
    if not os.path.exists(solver) and os.path.exists(solver + ".exe"):
        solver += ".exe"
    if not os.path.exists(solver):
        print(f"FATAL: solver not found at {solver}", file=sys.stderr)
        return 2

    signal.signal(signal.SIGTERM, _stop)
    signal.signal(signal.SIGINT, _stop)

    import psycopg

    worker_id = f"{socket.gethostname()}:{os.getpid()}"
    print(f"worker: {worker_id} using {solver}", flush=True)

    with psycopg.connect(args.database_url) as conn:
        last_sweep = 0.0
        while not _stopping:
            if time.monotonic() - last_sweep > 30:
                recovered = requeue_expired(conn)
                if recovered:
                    print(f"worker: recovered {recovered} expired job(s)", flush=True)
                last_sweep = time.monotonic()

            worked = process_one(conn, solver, worker_id)
            if args.once:
                return 0 if worked else 1
            if not worked:
                time.sleep(POLL_SECONDS)
    print("worker: stopped", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
