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
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import threading
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for path in (os.path.join(REPO, "services", "ingest"), os.path.join("services", "ingest")):
    sys.path.insert(0, path)

from psycopg.types.json import Jsonb  # noqa: E402

from ttm_ingest import ttm_format  # noqa: E402

POLL_SECONDS = 2.0
LEASE_SECONDS = 600
MAX_ATTEMPTS = 3

# Defaults; a job's request may lower them but never raise them past these.
DEFAULT_TIME_BUDGET_MS = 120_000
DEFAULT_MAX_RESULTS = 2_000_000

COUNT_RE = re.compile(r"Tree 0: (\d+) combinations")
PROGRESS_RE = re.compile(r"^PROGRESS (\d+)$")

# How often the engine emits a progress line, and the floor between database writes.
# The engine can report far more often than this, but a job's progress is read by a
# person watching a bar move; a write per second is enough for that and keeps a
# hundred concurrent jobs from turning into a hundred writes per second.
PROGRESS_INTERVAL_MS = 1000
PROGRESS_MIN_WRITE_SECONDS = 1.0
PROGRESS_ROW_BATCH = 20_000
_stopping = False


def _stop(_signum, _frame):
    global _stopping
    _stopping = True
    print("worker: stop requested, finishing current job", flush=True)


class SolveFailed(RuntimeError):
    """The solve could not be completed. The message reaches the user."""


class Cancelled(RuntimeError):
    """Someone asked for this job to stop. Not a failure, and not the job's fault."""


# ---------------------------------------------------------------------------
# decoding the engine's output
# ---------------------------------------------------------------------------

def iter_results(output_path: str, tree: dict, limit: int):
    """Yield the engine's output as nodeId-keyed point maps, one row at a time.

    The engine emits a header listing, per bit, the positional talent index that bit
    belongs to, then one line per set: the raw SIND, followed by the indices of any choice
    nodes present.

    Multi-rank talents occupy several bits that all map to the same talent, so a node's
    point total is how many of its bits are set -- which is why this counts rather than
    flags. Output rows are positional; what we store is keyed by Blizzard nodeId, because
    positional references are exactly what made shared builds unsafe before.

    A generator rather than a list: a job at the listing limit is two million rows, and
    holding two million dicts to hand them to an INSERT costs gigabytes for no reason.
    The file is read in step with the COPY that consumes it.
    """
    node_ids = [n["nodeId"] for n in tree["nodes"]]

    with open(output_path, encoding="utf-8") as handle:
        header = handle.readline().strip()
        if not header:
            return
        try:
            bit_to_index = [int(x) for x in header.split("/") if x != ""]
        except ValueError as exc:
            raise SolveFailed(f"unreadable result header: {header[:80]!r}") from exc
        for positional in bit_to_index:
            if positional >= len(node_ids):
                raise SolveFailed(
                    f"result references talent index {positional}, but the tree "
                    f"has {len(node_ids)}"
                )
        # Hoisted out of the row loop: the mapping is the same for every row, and at two
        # million rows anything done per row per bit is the whole cost of the phase.
        bit_keys = [(1 << bit, str(node_ids[positional]))
                    for bit, positional in enumerate(bit_to_index)]

        emitted = 0
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
            for bit_value, key in bit_keys:
                if mask & bit_value:
                    points[key] = points.get(key, 0) + 1
            yield points
            emitted += 1
            if emitted >= limit:
                return


def decode_results(output_path: str, tree: dict, limit: int) -> list[dict[str, int]]:
    """List form of iter_results, for callers small enough not to care."""
    return list(iter_results(output_path, tree, limit))


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

def _run_streaming(args: list[str], stdout_path: str, hard_timeout: float,
                   on_progress) -> tuple[str, str, int, bool, bool]:
    """
    Run the solver, consuming its stderr line by line so progress is visible while the
    job is still running.

    stdout goes to a file rather than a second pipe on purpose: reading only one of two
    pipes deadlocks as soon as the unread one fills its buffer, and the result summary
    on stdout is not needed until the process has exited anyway.

    The hard timeout is a watchdog thread rather than a read timeout, because a hung
    solver produces no lines at all -- there would be nothing for a read deadline to
    interrupt. It is a backstop; the engine's own --time-budget-ms is what normally
    stops a long solve, cleanly and with partial results intact.

    `on_progress` returning False means stop: the solver is killed rather than asked
    nicely, because a cancelled enumeration has no partial value to preserve and the
    engine has no input channel to ask on.

    Returns (stdout, stderr tail, exit code, timed out, stopped on request).
    """
    killed = threading.Event()
    stopped = False
    stderr_tail: list[str] = []

    with open(stdout_path, "w", encoding="utf-8") as sink:
        proc = subprocess.Popen(args, stdout=sink, stderr=subprocess.PIPE,
                                text=True, bufsize=1)

        def _kill():
            killed.set()
            proc.kill()

        watchdog = threading.Timer(hard_timeout, _kill)
        watchdog.start()
        try:
            for line in proc.stderr:
                line = line.strip()
                match = PROGRESS_RE.match(line)
                if match:
                    if on_progress is not None and not on_progress(int(match.group(1))):
                        stopped = True
                        proc.kill()
                        break
                elif line:
                    # Anything else on stderr is diagnostic; keep the tail for the
                    # error message rather than the whole stream.
                    stderr_tail.append(line)
                    del stderr_tail[:-20]
            proc.wait()  # also reaps the process after an early break
        finally:
            watchdog.cancel()
            if proc.stderr:
                proc.stderr.close()

    with open(stdout_path, encoding="utf-8", errors="replace") as handle:
        stdout_text = handle.read()
    return (stdout_text, "\n".join(stderr_tail), proc.returncode,
            killed.is_set(), stopped)


def run_solve(solver: str, tree: dict, request: dict, workdir: str,
              on_progress=None) -> tuple[str | None, dict]:
    points = int(request["points"])
    level_cap = int(request.get("levelCap", 90))
    must_have = [int(x) for x in request.get("mustHave", [])]
    must_not_have = [int(x) for x in request.get("mustNotHave", [])]
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
    if on_progress is not None:
        args += ["--progress", "--progress-interval-ms", str(PROGRESS_INTERVAL_MS)]
    if must_have or must_not_have or at_least_one_of or exactly_one_of:
        args += ["--filter", build_filter_string(tree, must_have, must_not_have,
                                                 at_least_one_of, exactly_one_of)]

    hard_timeout = (time_budget / 1000) + 60
    started = time.monotonic()
    stdout_text, stderr_tail, returncode, killed, stopped = _run_streaming(
        args, os.path.join(workdir, "solver.out"), hard_timeout, on_progress)
    elapsed = time.monotonic() - started

    if stopped:
        raise Cancelled("cancelled while solving")
    if killed:
        raise subprocess.TimeoutExpired(args, hard_timeout)
    if returncode != 0:
        raise SolveFailed(
            f"solver exited {returncode}: {(stderr_tail or stdout_text)[:300]}"
        )
    if "No valid trees found" in stdout_text:
        raise SolveFailed("the engine rejected the generated tree")

    match = COUNT_RE.search(stdout_text)
    if not match:
        raise SolveFailed(f"no count in solver output: {stdout_text[-200:]!r}")

    meta = {
        "reported": int(match.group(1)),
        "timedOut": "time budget exceeded" in stdout_text,
        "capped": "safety guard triggered" in stdout_text,
        "elapsed": elapsed,
    }
    meta["maxResults"] = max_results
    return (output if os.path.exists(output) else None), meta


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


class ProgressReporter:
    """
    Turns the engine's progress lines into `solve_jobs.progress`.

    Two jobs, not one. The obvious one is the fraction a client polls for. The second
    is the lease: `requeue_expired` treats a job whose `locked_at` is older than
    LEASE_SECONDS as abandoned, and a solve that legitimately runs longer than the
    lease would be requeued underneath a worker that is still working on it. A job
    that is visibly making progress is not abandoned, so the same write says so.

    Writes are throttled because the denominator is a person watching a bar, not a
    scheduler. `expected` is the pre-flight count the gate already computed, which is
    what makes this a real fraction rather than a spinner.

    It runs on its own connection, and has to. The storing phase reports from inside a
    COPY, and a connection in COPY mode accepts no other statement -- an UPDATE sent
    down the same connection waits for a COPY that is itself waiting on the row that
    would trigger the next report. That deadlocks, silently, with the job stuck at 0%.

    It is also how the worker hears about cancellation. The flag rides back on the same
    statement's RETURNING clause, so noticing costs nothing: there was already a write
    per second, and a cancel is now observed within that second without a second query,
    a second connection, or LISTEN/NOTIFY.
    """

    def __init__(self, conn, job_id, expected):
        self.conn = conn
        self.job_id = job_id
        self.expected = int(expected) if expected else 0
        self.phase = None
        self.last_write = 0.0
        self.broken = False
        self.cancelled = False

    def enter(self, phase: str, expected=None) -> None:
        """Start a new phase, resetting the fraction and forcing an immediate write.

        Also the cancellation checkpoint between phases: there is no point starting to
        store two million rows that a pending cancel is about to throw away.
        """
        self.phase = phase
        if expected is not None:
            self.expected = int(expected)
        self.last_write = 0.0
        self(0)
        if self.cancelled:
            raise Cancelled("cancelled before " + phase)

    def __call__(self, count: int, force: bool = False) -> bool:
        """Report progress; returns False once a cancellation has been seen.

        A return value rather than an exception, because the caller is sometimes inside a
        COPY or holding a live subprocess and has to unwind it deliberately.
        """
        if self.cancelled:
            return False
        if self.broken:
            return True
        now = time.monotonic()
        if not force and now - self.last_write < PROGRESS_MIN_WRITE_SECONDS:
            return True
        self.last_write = now
        # Clamp: the column is CHECKed to [0, 1], and a count can exceed the
        # prediction if the request was edited between the gate and the solve.
        fraction = min(count / self.expected, 1.0) if self.expected else 0.0
        try:
            with self.conn.cursor() as cur:
                cur.execute(
                    "UPDATE solve_jobs SET progress = %s, phase = %s, locked_at = now() "
                    "WHERE id = %s AND state = 'running' "
                    "RETURNING cancel_requested",
                    (fraction, self.phase, self.job_id),
                )
                row = cur.fetchone()
            self.conn.commit()
            # No row means the job is no longer running under us -- cancelled outright,
            # or requeued by a sweeper that thought we were dead. Either way, stop.
            if row is None or row[0]:
                self.cancelled = True
                return False
        except Exception:  # noqa: BLE001
            # Losing progress reporting must not lose the job that is producing it.
            self.broken = True
            try:
                self.conn.rollback()
            except Exception:  # noqa: BLE001
                pass
        return True


def store_results(conn, job_id, rows, reporter=None) -> int:
    """Write the decoded builds with COPY, reporting progress as they go.

    COPY rather than executemany: this phase, not the engine, is where a listing job
    spends its time. An unfiltered 25-point Balance Druid solve enumerates 1,906,208
    sets in 0.11 s and then spent over ten minutes on a per-row INSERT. COPY turns that
    back into something proportionate to the data.

    The denominator for the fraction lives on the reporter, which the caller has already
    pointed at the engine's own count rather than the pre-flight one -- a capped run
    produced fewer rows than the gate predicted.
    """
    stored = 0
    with conn.cursor() as cur:
        with cur.copy(
            "COPY solve_results (job_id, ordinal, points) FROM STDIN (FORMAT BINARY)"
        ) as copy:
            copy.set_types(["uuid", "int4", "jsonb"])
            for points in rows:
                copy.write_row((job_id, stored, Jsonb(points)))
                stored += 1
                # The check is batched because it is on the hot path; the reporter
                # throttles the writes themselves on top of this.
                if (reporter is not None and stored % PROGRESS_ROW_BATCH == 0
                        and not reporter(stored)):
                    # Unwinding out of the COPY aborts the transaction, which is exactly
                    # right: a cancelled job leaves no rows behind to explain.
                    raise Cancelled("cancelled while storing")
            if reporter is not None:
                # Marked here, inside the block, not after it: leaving a COPY is not free.
                # Closing it flushes the stream and waits while the server builds
                # solve_results' primary key over everything just written -- for two
                # million rows that is several seconds with nothing left to count, and
                # setting the phase afterwards would mark a wait that had already ended.
                reporter.enter("finalizing")

        # Talent frequencies, in the same transaction as the rows they describe: either a
        # job has results and statistics or it has neither. Done here rather than on demand
        # because the aggregate walks every result row -- about a second for 165,000 builds
        # -- which is nothing inside a job that is already asynchronous, and far too slow
        # for a request someone is waiting on.
        if stored:
            cur.execute(
                """
                INSERT INTO solve_stats (job_id, node_id, builds, points)
                SELECT %s, (kv.key)::bigint, count(*), sum((kv.value)::bigint)
                FROM solve_results r, jsonb_each(r.points) kv
                WHERE r.job_id = %s
                GROUP BY kv.key
                """,
                (job_id, job_id),
            )
    conn.commit()
    return stored


def finish(conn, job_id, state, *, result_count=None, error=None):
    """Record a terminal state and release the lease.

    `progress` is only forced to 1 for a job that ran to the end. A cancelled or failed
    job keeps the fraction it actually reached: claiming 100% for work that was abandoned
    at 40% would misreport what happened, and that number is what a client displays.
    """
    completed = state in ("done", "capped")
    with conn.cursor() as cur:
        cur.execute(
            """
            UPDATE solve_jobs
            SET state = %s, result_count = %s, error = %s, finished_at = now(),
                progress = CASE WHEN %s THEN 1 ELSE progress END,
                phase = NULL, locked_by = NULL, locked_at = NULL
            WHERE id = %s
            """,
            (state, result_count, error, completed, job_id),
        )
    conn.commit()


def requeue_expired(conn) -> int:
    """Return jobs whose worker died to the queue; fail them once they run out of tries.

    Without this a killed worker strands a job in `running` forever, which is
    indistinguishable to a user from one that is simply slow.
    """
    with conn.cursor() as cur:
        # A job whose cancellation is pending must not be handed back to the queue: the
        # worker that was told to stop is the one that died, and requeueing would start
        # the work again with the instruction to stop still sitting unread. This is the
        # only place a row returns to 'queued', so it is the only place that can catch it.
        cur.execute(
            """
            UPDATE solve_jobs
            SET state = 'cancelled', finished_at = now(),
                locked_by = NULL, locked_at = NULL, phase = NULL
            WHERE state = 'running' AND cancel_requested
              AND locked_at < now() - make_interval(secs => %s)
            """,
            (LEASE_SECONDS,),
        )
        cancelled = cur.rowcount
        cur.execute(
            """
            UPDATE solve_jobs SET state = 'queued', locked_by = NULL, locked_at = NULL,
                                  phase = NULL
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
    return requeued + failed + cancelled


def _rollback(conn) -> None:
    """A failure part-way through the COPY leaves the transaction aborted, and every
    statement after that -- including the one recording the failure -- would be rejected.
    Discarding the partial rows is also what we want: a job is stored whole or not at all."""
    try:
        conn.rollback()
    except Exception:  # noqa: BLE001
        pass


def process_one(conn, progress_conn, solver: str, worker_id: str) -> bool:
    job = claim(conn, worker_id)
    if not job:
        return False

    job_id = job["id"]
    print(f"worker: claimed {job_id} (attempt {job['attempts']})", flush=True)
    workdir = tempfile.mkdtemp(prefix="ttm-solve-")
    started = time.monotonic()
    try:
        tree = load_tree(conn, job["tree_id"], job["tree_revision"])
        expected = job.get("expected_count")
        reporter = ProgressReporter(progress_conn, job_id, expected)
        reporter.enter("solving")
        output, meta = run_solve(solver, tree, job["request"], workdir,
                                 on_progress=reporter)

        truncated = meta["timedOut"] or meta["capped"]
        if not truncated and expected is not None and int(expected) != meta["reported"]:
            # The gate and the engine must agree; if they do not, the user should not be
            # handed a result that silently contradicts the count they were shown.
            raise SolveFailed(
                f"engine produced {meta['reported']} sets but the pre-flight count "
                f"predicted {int(expected)}"
            )

        # The engine's own count is the denominator for storing, not the pre-flight one:
        # a capped run produced fewer rows than the gate predicted, and a bar measured
        # against the prediction would stop short of the end for a job that did finish.
        reporter.enter("storing", expected=min(meta["reported"], meta["maxResults"]))
        rows = iter_results(output, tree, meta["maxResults"]) if output else iter(())
        stored = store_results(conn, job_id, rows, reporter)

        if truncated:
            # A truncated result is a distinct outcome, not a failure and not a success.
            reason = "time budget exceeded" if meta["timedOut"] else "result cap reached"
            finish(conn, job_id, "capped", result_count=stored,
                   error=f"partial result: {reason}")
            print(f"worker: {job_id} capped ({reason}), {stored} builds", flush=True)
            return True

        finish(conn, job_id, "done", result_count=stored)
        print(f"worker: {job_id} done, {stored} builds in {meta['elapsed']:.2f}s "
              f"solving, {time.monotonic() - started:.2f}s total", flush=True)
    except Cancelled as exc:
        _rollback(conn)
        finish(conn, job_id, "cancelled", error=None)
        print(f"worker: {job_id} cancelled ({exc})", flush=True)
    except SolveFailed as exc:
        _rollback(conn)
        finish(conn, job_id, "failed", error=str(exc)[:500])
        print(f"worker: {job_id} failed: {exc}", flush=True)
    except subprocess.TimeoutExpired:
        _rollback(conn)
        finish(conn, job_id, "failed", error="solver did not exit within its budget")
        print(f"worker: {job_id} failed: solver hung", flush=True)
    except Exception as exc:  # noqa: BLE001 - a worker must not die on one bad job
        _rollback(conn)
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

    # Two connections: one runs the job, one reports its progress. See ProgressReporter --
    # progress is written from inside a COPY, which a single connection cannot do.
    with (psycopg.connect(args.database_url) as conn,
          psycopg.connect(args.database_url) as progress_conn):
        last_sweep = 0.0
        while not _stopping:
            if time.monotonic() - last_sweep > 30:
                recovered = requeue_expired(conn)
                if recovered:
                    print(f"worker: recovered {recovered} expired job(s)", flush=True)
                last_sweep = time.monotonic()

            worked = process_one(conn, progress_conn, solver, worker_id)
            if args.once:
                return 0 if worked else 1
            if not worked:
                time.sleep(POLL_SECONDS)
    print("worker: stopped", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
