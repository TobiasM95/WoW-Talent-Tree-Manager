-- Custom trees: player-designed, content-addressed, and served like any other game's.
--
-- A custom tree lives in `trees` like every other, so the counting DP, the solver, the worker
-- and every endpoint take it without a special case. It belongs to one fixed, always-promoted
-- revision of a game called 'custom' -- `current_trees` serves each game from its latest
-- promoted revision, so every custom tree is current the moment it is written.
--
-- Trees are immutable and keyed by the hash of their content (`custom/<hash>/<n>`): saving
-- an edit writes a new version rather than changing the old one. That is what lets a link
-- to a custom tree keep meaning the tree it was made from, and what makes caching by key --
-- the DP graph cache, solve dedup -- safe without invalidation. Nothing is tied to a person;
-- the browser remembers which projects are yours.

ALTER TABLE trees DROP CONSTRAINT trees_game_known;
ALTER TABLE trees ADD CONSTRAINT trees_game_known
    CHECK (game IN ('retail', 'classic', 'forever', 'custom'));

-- The one revision every custom tree belongs to. Revision 1 sits below every ingested one.
INSERT INTO ingest_runs (revision, source_provider, source_origin, source_digest, fetched_at,
                         promoted_at, game)
VALUES (1, 'custom', 'tree editor', '-', now(), now(), 'custom')
ON CONFLICT (revision) DO NOTHING;

-- A project: one to three trees designed together, with an optional shared point pool.
CREATE TABLE custom_projects (
    id          text        PRIMARY KEY,           -- content hash, 16 hex digits
    name        text        NOT NULL,
    tree_count  integer     NOT NULL CHECK (tree_count BETWEEN 1 AND 3),
    shared_pool integer,                           -- NULL: each tree has its own budget
    created_at  timestamptz NOT NULL DEFAULT now(),
    CONSTRAINT custom_projects_id_is_hash CHECK (id ~ '^[0-9a-f]{16}$')
);
