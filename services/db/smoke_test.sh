#!/usr/bin/env bash
# End-to-end smoke test of the data stack. Brings up Postgres, migrates, loads the
# ingested trees, and asserts the results are queryable and the constraints bite.
#
#   bash services/db/smoke_test.sh
#
# Requires docker compose and a completed ingest in data/generated.
set -euo pipefail

export MSYS_NO_PATHCONV=1
psql() { docker compose exec -T postgres psql -U "${POSTGRES_USER:-ttm}" -d "${POSTGRES_DB:-ttm}" -qtAX "$@"; }
fail() { echo "FAIL: $1" >&2; exit 1; }

echo "== bringing up postgres"
docker compose up -d postgres >/dev/null
for _ in $(seq 1 30); do
  [ "$(docker inspect --format='{{.State.Health.Status}}' ttm-postgres-1 2>/dev/null)" = healthy ] && break
  sleep 2
done

echo "== migrating"
docker compose run --rm migrate >/dev/null

echo "== loading trees"
docker compose run --rm loader >/dev/null

echo "== asserting"
trees=$(psql -c "SELECT count(*) FROM current_trees")
[ "$trees" -eq 160 ] || fail "expected 160 current trees, got $trees"

hero=$(psql -c "SELECT count(*) FROM current_trees WHERE kind = 'hero'")
[ "$hero" -eq 80 ] || fail "expected 80 hero trees, got $hero"

# Every promoted tree must have counts, or the pre-flight gate has nothing to read.
uncounted=$(psql -c "
  SELECT count(*) FROM current_trees t
  WHERE NOT EXISTS (SELECT 1 FROM tree_counts c
                    WHERE c.tree_id = t.id AND c.tree_revision = t.revision)")
[ "$uncounted" -eq 0 ] || fail "$uncounted tree(s) have no precomputed counts"

# Counts above 2^31 must survive: this is what overflowed the engine's 32-bit counter.
big=$(psql -c "SELECT count(*) FROM tree_counts WHERE build_count > 2147483647")
[ "$big" -gt 0 ] || fail "no counts above 2^31 -- numeric storage may be truncating"

promoted=$(psql -c "SELECT count(*) FROM ingest_runs WHERE promoted_at IS NOT NULL")
[ "$promoted" -ge 1 ] || fail "no promoted ingest run"

# A build must not be able to reference a tree revision that does not exist.
if psql -c "INSERT INTO builds (tree_id, tree_revision, points)
            VALUES ('00000000-0000-0000-0000-000000000000', 999999, '{}')" 2>/dev/null; then
  fail "foreign key on builds did not reject a dangling tree revision"
fi

# Points must be a nodeId-keyed object, never a positional array.
if psql -c "INSERT INTO builds (tree_id, tree_revision, points)
            SELECT id, revision, '[1,2,3]' FROM current_trees LIMIT 1" 2>/dev/null; then
  fail "builds accepted a positional array for points"
fi

echo "== ok: $trees trees ($hero hero), counts present, constraints enforced"
