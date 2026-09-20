-- TTM schema, initial.
--
-- Design notes, each responding to something the legacy web app got wrong
-- (docs/01-current-state/legacy-web-app.md): it had no foreign keys or constraints
-- anywhere, and a single global ContentID space shared across six tables that had to be
-- probed to discover what an id even referred to. So: real keys, real constraints, one id
-- space per entity.
--
-- Trees are immutable revisions. A build pins the revision it was made against, which is
-- what stops a preset update from silently reinterpreting someone's saved points -- the
-- failure mode that positional skillset strings had.

BEGIN;

CREATE EXTENSION IF NOT EXISTS pgcrypto;   -- gen_random_uuid()

-- ---------------------------------------------------------------------------
-- ingest provenance
-- ---------------------------------------------------------------------------

-- One row per ingest attempt, successful or not. `promoted_at` is what makes a revision
-- live. The legacy pipeline's fatal property was that a failed run was indistinguishable
-- from a quiet day; here a run that never promotes is visible, and staleness is queryable.
CREATE TABLE ingest_runs (
    revision        integer PRIMARY KEY,
    source_provider text        NOT NULL,
    source_origin   text        NOT NULL,
    source_digest   text        NOT NULL,
    fetched_at      timestamptz NOT NULL,
    started_at      timestamptz NOT NULL DEFAULT now(),
    promoted_at     timestamptz,
    tree_count      integer     NOT NULL DEFAULT 0,
    node_count      integer     NOT NULL DEFAULT 0,
    anomalies       jsonb       NOT NULL DEFAULT '{}'::jsonb,
    description_coverage real,
    warnings        jsonb       NOT NULL DEFAULT '[]'::jsonb,
    CONSTRAINT ingest_runs_coverage_range
        CHECK (description_coverage IS NULL
               OR (description_coverage >= 0 AND description_coverage <= 1))
);

COMMENT ON COLUMN ingest_runs.promoted_at IS
    'NULL means the run never passed validation; such a revision must not be served.';

-- ---------------------------------------------------------------------------
-- trees
-- ---------------------------------------------------------------------------

CREATE TYPE tree_kind AS ENUM ('class', 'spec', 'hero', 'custom');

-- Retail gates on points spent in this tree; classic-era trees gate on a mandatory
-- prerequisite chain per tab. Explicit so one schema can span both (open question Q3).
CREATE TYPE gating_kind AS ENUM ('reqPoints', 'prereqChain');

CREATE TABLE trees (
    id                 uuid        NOT NULL,
    revision           integer     NOT NULL REFERENCES ingest_runs(revision) ON DELETE CASCADE,
    key                text        NOT NULL,
    kind               tree_kind   NOT NULL,
    game               text        NOT NULL DEFAULT 'retail',
    gating             gating_kind NOT NULL DEFAULT 'reqPoints',
    class_id           integer,
    spec_id            integer,
    class_name         text,
    spec_name          text,
    sub_tree_id        integer,
    name               text        NOT NULL,
    -- The node graph. Read whole and rarely queried field-wise, so jsonb beats a
    -- row-per-node table; add a GIN index only if node-level querying materialises.
    definition         jsonb       NOT NULL,
    -- Not in the upstream payload, so NULL rather than a guess.
    point_cap          integer,
    -- Derived fact: the sum of every node's max ranks.
    max_points_in_tree integer     NOT NULL,
    node_count         integer     NOT NULL,
    created_at         timestamptz NOT NULL DEFAULT now(),

    PRIMARY KEY (id, revision),
    CONSTRAINT trees_node_count_positive CHECK (node_count > 0),
    CONSTRAINT trees_max_points_positive  CHECK (max_points_in_tree > 0),
    CONSTRAINT trees_hero_has_sub_tree
        CHECK (kind <> 'hero' OR sub_tree_id IS NOT NULL),
    CONSTRAINT trees_game_known CHECK (game IN ('retail', 'classic'))
);

CREATE UNIQUE INDEX trees_key_revision_idx ON trees (key, revision);
CREATE INDEX trees_spec_idx ON trees (game, class_id, spec_id, kind);
CREATE INDEX trees_revision_idx ON trees (revision);

-- The trees that should actually be served: highest promoted revision only.
CREATE VIEW current_trees AS
SELECT t.*
FROM trees t
JOIN ingest_runs r ON r.revision = t.revision
WHERE r.promoted_at IS NOT NULL
  AND r.revision = (SELECT max(revision) FROM ingest_runs WHERE promoted_at IS NOT NULL);

-- ---------------------------------------------------------------------------
-- precomputed counts
-- ---------------------------------------------------------------------------

-- The frontier DP answers "how many valid builds at N points" in milliseconds and the
-- answer never changes for a given tree revision, so it is computed once at ingest.
-- This is what makes the count a free pre-flight gate rather than a job.
CREATE TABLE tree_counts (
    tree_id       uuid    NOT NULL,
    tree_revision integer NOT NULL,
    points        integer NOT NULL,
    -- Counts exceed 2^31 on nine class trees (shaman_class_elemental reaches
    -- 37,296,642,700), which is what overflowed the engine's 32-bit counter. numeric
    -- rather than bigint because a combined multi-tree count could exceed 2^63 too.
    build_count   numeric NOT NULL,
    level_cap     integer NOT NULL,

    PRIMARY KEY (tree_id, tree_revision, points, level_cap),
    FOREIGN KEY (tree_id, tree_revision) REFERENCES trees(id, revision) ON DELETE CASCADE,
    CONSTRAINT tree_counts_points_positive CHECK (points > 0),
    CONSTRAINT tree_counts_non_negative    CHECK (build_count >= 0)
);

COMMENT ON COLUMN tree_counts.level_cap IS
    'Tiered nodes maximum ranks depend on character level, so a count is only valid for
     the cap it was computed at (open question Q10).';

-- ---------------------------------------------------------------------------
-- identity (deliberately minimal: the core loop works anonymously)
-- ---------------------------------------------------------------------------

CREATE TABLE users (
    id            uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    display_name  text        NOT NULL,
    email         text        UNIQUE,
    battle_net_id text        UNIQUE,
    created_at    timestamptz NOT NULL DEFAULT now(),
    CONSTRAINT users_has_an_identity
        CHECK (email IS NOT NULL OR battle_net_id IS NOT NULL)
);

-- Server-side sessions rather than JWT: simpler, and revocable. The legacy app used
-- httpOnly JWT cookies with CORS set to origins:"*" alongside credentials, which is
-- precisely the combination that does not work.
CREATE TABLE sessions (
    token_hash bytea       PRIMARY KEY,
    user_id    uuid        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at timestamptz NOT NULL DEFAULT now(),
    expires_at timestamptz NOT NULL,
    CONSTRAINT sessions_expire_after_creation CHECK (expires_at > created_at)
);

CREATE INDEX sessions_user_idx ON sessions (user_id);
CREATE INDEX sessions_expiry_idx ON sessions (expires_at);

-- ---------------------------------------------------------------------------
-- loadouts and builds
-- ---------------------------------------------------------------------------

CREATE TABLE loadouts (
    id            uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id       uuid        REFERENCES users(id) ON DELETE CASCADE,
    tree_id       uuid        NOT NULL,
    tree_revision integer     NOT NULL,
    name          text        NOT NULL,
    description   text        NOT NULL DEFAULT '',
    is_public     boolean     NOT NULL DEFAULT false,
    created_at    timestamptz NOT NULL DEFAULT now(),
    updated_at    timestamptz NOT NULL DEFAULT now(),
    FOREIGN KEY (tree_id, tree_revision) REFERENCES trees(id, revision)
);

CREATE INDEX loadouts_user_idx ON loadouts (user_id);
CREATE INDEX loadouts_tree_idx ON loadouts (tree_id, tree_revision);

CREATE TABLE builds (
    id            uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    loadout_id    uuid        REFERENCES loadouts(id) ON DELETE CASCADE,
    tree_id       uuid        NOT NULL,
    -- Pinning the revision is what makes a shared build safe. Positional skillset
    -- strings had no such anchor, so a preset regeneration silently reassigned points.
    tree_revision integer     NOT NULL,
    name          text        NOT NULL DEFAULT '',
    level_cap     integer,
    -- Keyed by Blizzard nodeId as text, never by position: {"82043": 2, "82050": 1}.
    points        jsonb       NOT NULL,
    -- Which entry of each choice node: {"82061": "entry:124"}.
    choices       jsonb       NOT NULL DEFAULT '{}'::jsonb,
    -- Which hero sub-tree is selected, where applicable.
    sub_tree_id   integer,
    position      integer     NOT NULL DEFAULT 0,
    created_at    timestamptz NOT NULL DEFAULT now(),
    FOREIGN KEY (tree_id, tree_revision) REFERENCES trees(id, revision),
    CONSTRAINT builds_points_is_object CHECK (jsonb_typeof(points) = 'object'),
    CONSTRAINT builds_choices_is_object CHECK (jsonb_typeof(choices) = 'object')
);

CREATE INDEX builds_loadout_idx ON builds (loadout_id, position);
CREATE INDEX builds_tree_idx ON builds (tree_id, tree_revision);

-- ---------------------------------------------------------------------------
-- solve jobs
-- ---------------------------------------------------------------------------

CREATE TYPE job_state AS ENUM ('queued', 'running', 'done', 'failed', 'cancelled', 'capped');

-- Filtered enumeration, dispatched only after the DP count has confirmed the result set
-- is small enough to be worth producing. That pre-flight gate is why this queue never has
-- to defend against unbounded output.
CREATE TABLE solve_jobs (
    id             uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id        uuid        REFERENCES users(id) ON DELETE SET NULL,
    tree_id        uuid        NOT NULL,
    tree_revision  integer     NOT NULL,
    -- Point budget, filters, level cap.
    request        jsonb       NOT NULL,
    -- sha256 over the tree snapshot plus the normalised request. Dedup and cache are the
    -- same mechanism: an identical in-flight or finished solve is reused, not recomputed.
    request_hash   bytea       NOT NULL,
    state          job_state   NOT NULL DEFAULT 'queued',
    priority       smallint    NOT NULL DEFAULT 100,
    attempts       smallint    NOT NULL DEFAULT 0,
    progress       real        NOT NULL DEFAULT 0,
    -- What the DP predicted before dispatch; the worker's output must match it.
    expected_count numeric,
    result_count   numeric,
    error          text,
    locked_by      text,
    locked_at      timestamptz,
    created_at     timestamptz NOT NULL DEFAULT now(),
    finished_at    timestamptz,
    FOREIGN KEY (tree_id, tree_revision) REFERENCES trees(id, revision),
    CONSTRAINT solve_jobs_progress_range CHECK (progress >= 0 AND progress <= 1),
    CONSTRAINT solve_jobs_attempts_sane  CHECK (attempts >= 0 AND attempts < 100),
    CONSTRAINT solve_jobs_running_is_locked
        CHECK (state <> 'running' OR (locked_by IS NOT NULL AND locked_at IS NOT NULL))
);

-- The claim path: WHERE state='queued' ORDER BY priority, created_at
--                 FOR UPDATE SKIP LOCKED LIMIT 1
CREATE INDEX solve_jobs_queue_idx ON solve_jobs (priority, created_at)
    WHERE state = 'queued';

-- Reuse an equivalent solve rather than repeating it.
CREATE UNIQUE INDEX solve_jobs_dedup_idx ON solve_jobs (request_hash)
    WHERE state IN ('queued', 'running', 'done');

-- Requeue sweeper: rows stuck in running past their lease.
CREATE INDEX solve_jobs_lease_idx ON solve_jobs (locked_at) WHERE state = 'running';

CREATE TABLE solve_results (
    job_id     uuid        NOT NULL REFERENCES solve_jobs(id) ON DELETE CASCADE,
    ordinal    integer     NOT NULL,
    -- One matching build, nodeId-keyed like builds.points.
    points     jsonb       NOT NULL,
    PRIMARY KEY (job_id, ordinal),
    CONSTRAINT solve_results_points_is_object CHECK (jsonb_typeof(points) = 'object')
);

COMMENT ON TABLE solve_results IS
    'Bounded by the pre-flight count, so results can be stored normally. Before the count
     gate existed an unconstrained solve produced 305 million rows and 9.5 GB.';

COMMIT;
