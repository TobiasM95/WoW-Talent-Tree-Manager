-- More than one game, each served from its own latest revision.
--
-- `current_trees` served the single highest promoted revision across everything. That was
-- right while there was one game, and wrong the moment there is a second: loading WoW
-- Forever's 27 trees as a new revision would have made it "the" current revision and
-- silently unserved all 160 retail trees. A revision now belongs to a game, and each game is
-- served from its own latest promoted revision.

ALTER TABLE ingest_runs ADD COLUMN game text NOT NULL DEFAULT 'retail';
CREATE INDEX ingest_runs_game_promoted_idx ON ingest_runs (game, revision) WHERE promoted_at IS NOT NULL;

-- WoW Forever: vanilla-shaped talent trees -- three per class, one shared point pool.
ALTER TABLE trees DROP CONSTRAINT trees_game_known;
ALTER TABLE trees ADD CONSTRAINT trees_game_known CHECK (game IN ('retail', 'classic', 'forever'));

-- A classic talent tab: one of a class's three trees, not a class, spec or hero tree.
ALTER TYPE tree_kind ADD VALUE IF NOT EXISTS 'tab';

CREATE OR REPLACE VIEW current_trees AS
SELECT t.*
FROM trees t
JOIN ingest_runs r ON r.revision = t.revision
WHERE r.promoted_at IS NOT NULL
  AND r.revision = (
      SELECT max(r2.revision) FROM ingest_runs r2
      WHERE r2.promoted_at IS NOT NULL AND r2.game = r.game
  );
