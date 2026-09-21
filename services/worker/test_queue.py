#!/usr/bin/env python3
"""Tests for the worker's queue behaviour. Needs a database; no solver.

    DATABASE_URL=... python services/worker/test_queue.py
    docker compose run --rm --entrypoint python worker /app/services/worker/test_queue.py

What this covers is the lease sweeper, which is the only thing standing between a killed
worker and a job that is stranded forever. It is also the only code that moves a row back
to `queued`, which makes it the one place a pending cancellation can be lost.

These cases cannot live in test_worker.py: that suite is deliberately database-free, and
the whole point here is what the SQL does.
"""
from __future__ import annotations

import os
import sys
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import worker  # noqa: E402

_failures = []


def check(name, condition, detail=""):
    print(f"{'ok  ' if condition else 'FAIL'} {name}"
          f"{(' -- ' + detail) if detail and not condition else ''}")
    if not condition:
        _failures.append(name)


def seed(conn, tree, **overrides):
    """Insert one running job with a stale lease, plus whatever the case needs."""
    fields = {
        "state": "running",
        "cancel_requested": False,
        "attempts": 1,
        "progress": 0.42,
        "lease_age": "2 hours",
    }
    fields.update(overrides)
    with conn.cursor() as cur:
        cur.execute(
            """
            INSERT INTO solve_jobs (tree_id, tree_revision, request, request_hash,
                                    state, cancel_requested, attempts, progress,
                                    locked_by, locked_at)
            VALUES (%s, %s, '{"points": 10}', decode(md5(%s), 'hex'),
                    %s, %s, %s, %s, 'dead-worker', now() - %s::interval)
            RETURNING id
            """,
            (tree[0], tree[1], str(uuid.uuid4()), fields["state"],
             fields["cancel_requested"], fields["attempts"], fields["progress"],
             fields["lease_age"]))
        return cur.fetchone()[0]


def read(conn, job_id):
    with conn.cursor() as cur:
        cur.execute("SELECT state, progress, locked_by, phase FROM solve_jobs WHERE id = %s",
                    (job_id,))
        state, progress, locked_by, phase = cur.fetchone()
    return state, float(progress), locked_by, phase


def main():
    dsn = os.environ.get("DATABASE_URL")
    if not dsn:
        print("skip: no DATABASE_URL")
        return 0

    import psycopg

    with psycopg.connect(dsn) as conn:
        with conn.cursor() as cur:
            cur.execute("SELECT id, revision FROM current_trees LIMIT 1")
            tree = cur.fetchone()
        if not tree:
            print("skip: no trees loaded")
            return 0

        cases = {
            # A cancel was requested and then the worker died. Requeueing this would start
            # the work over with the instruction to stop still sitting unread.
            "cancel pending": seed(conn, tree, cancel_requested=True),
            # The ordinary abandoned job: give it back to the queue.
            "abandoned, tries left": seed(conn, tree, attempts=1),
            # Out of tries. Something about this job kills workers; stop feeding it to them.
            "abandoned, out of tries": seed(conn, tree, attempts=worker.MAX_ATTEMPTS),
            # Still within its lease. A slow job is not an abandoned one.
            "lease still valid": seed(conn, tree, lease_age="1 second"),
        }
        conn.commit()

        moved = worker.requeue_expired(conn)
        got = {name: read(conn, jid) for name, jid in cases.items()}

        check("a cancel-pending job is cancelled, not requeued",
              got["cancel pending"][0] == "cancelled", str(got["cancel pending"]))
        check("a cancelled job keeps the progress it actually reached",
              abs(got["cancel pending"][1] - 0.42) < 1e-6, str(got["cancel pending"]))
        check("an abandoned job with tries left is requeued",
              got["abandoned, tries left"][0] == "queued",
              str(got["abandoned, tries left"]))
        check("a requeued job releases its lease",
              got["abandoned, tries left"][2] is None
              and got["abandoned, tries left"][3] is None,
              str(got["abandoned, tries left"]))
        check("an abandoned job out of tries fails",
              got["abandoned, out of tries"][0] == "failed",
              str(got["abandoned, out of tries"]))
        check("a job inside its lease is left alone",
              got["lease still valid"][0] == "running", str(got["lease still valid"]))
        check("the sweep counts every row it moved", moved == 3, f"{moved}")

        with conn.cursor() as cur:
            cur.execute("DELETE FROM solve_jobs WHERE id = ANY(%s)",
                        (list(cases.values()),))
        conn.commit()

    print()
    if _failures:
        print(f"{len(_failures)} failed: {', '.join(_failures)}")
        return 1
    print("all queue tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
