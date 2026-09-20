#!/usr/bin/env python3
"""Apply SQL migrations in order, once each.

    python services/db/migrate.py [--database-url URL] [--dry-run]

A deliberately small runner rather than Alembic: migrations here are plain SQL, applied
forward only, and the whole point is that what runs against the database is exactly what
is in the file and reviewable as such.

Each migration runs inside a transaction together with the row recording it, so a failed
migration leaves no trace and can simply be fixed and re-run. Files must be idempotent
only in the sense that they are never applied twice -- the ledger guarantees that.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import sys

MIGRATIONS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "migrations")

LEDGER_DDL = """
CREATE TABLE IF NOT EXISTS schema_migrations (
    filename    text        PRIMARY KEY,
    checksum    text        NOT NULL,
    applied_at  timestamptz NOT NULL DEFAULT now()
)
"""


def discover() -> list[str]:
    if not os.path.isdir(MIGRATIONS_DIR):
        return []
    return sorted(f for f in os.listdir(MIGRATIONS_DIR) if f.endswith(".sql"))


def checksum(path: str) -> str:
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description="Apply TTM database migrations.")
    parser.add_argument("--database-url", default=os.environ.get("DATABASE_URL"))
    parser.add_argument("--dry-run", action="store_true",
                        help="report what would be applied, change nothing")
    args = parser.parse_args()

    if not args.database_url:
        print("FATAL: no database URL. Pass --database-url or set DATABASE_URL.",
              file=sys.stderr)
        return 2

    files = discover()
    if not files:
        print(f"no migrations found in {MIGRATIONS_DIR}", file=sys.stderr)
        return 2

    import psycopg

    with psycopg.connect(args.database_url, autocommit=True) as conn:
        conn.execute(LEDGER_DDL)
        applied = {
            row[0]: row[1]
            for row in conn.execute("SELECT filename, checksum FROM schema_migrations")
        }

        pending = []
        for name in files:
            path = os.path.join(MIGRATIONS_DIR, name)
            digest = checksum(path)
            if name in applied:
                if applied[name] != digest:
                    # An applied migration that has since been edited means the database
                    # and the repository disagree about history. Refuse rather than guess.
                    print(
                        f"FATAL: {name} was already applied but its contents have changed.\n"
                        f"  recorded {applied[name][:12]}, on disk {digest[:12]}\n"
                        "  Add a new migration instead of editing an applied one.",
                        file=sys.stderr,
                    )
                    return 1
                continue
            pending.append((name, path, digest))

        if not pending:
            print(f"up to date ({len(applied)} migration(s) applied)")
            return 0

        print(f"{len(pending)} migration(s) pending:")
        for name, _, _ in pending:
            print(f"  {name}")
        if args.dry_run:
            print("dry run: nothing applied")
            return 0

        for name, path, digest in pending:
            with open(path, encoding="utf-8") as handle:
                sql = handle.read()
            print(f"applying {name} ...", end=" ", flush=True)
            try:
                # The migration and its ledger row commit together: a failure rolls both
                # back, so there is never a half-applied migration to reason about.
                with psycopg.connect(args.database_url) as run_conn:
                    run_conn.execute(sql)
                    run_conn.execute(
                        "INSERT INTO schema_migrations (filename, checksum) VALUES (%s, %s)",
                        (name, digest),
                    )
            except Exception as exc:  # noqa: BLE001 - report and stop, do not continue
                print("FAILED")
                print(f"FATAL: {name}: {exc}", file=sys.stderr)
                return 1
            print("ok")

    print("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
