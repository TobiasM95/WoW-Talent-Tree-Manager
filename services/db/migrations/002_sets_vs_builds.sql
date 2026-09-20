-- Separate set counts from build counts.
--
-- 001 stored a single `build_count`, populated from the DP's default mode -- which counts
-- SETS, where a choice node occupies one slot and its side is left unresolved. That is
-- what the engine's filtered path counts, and it is not what a user means by "how many
-- builds": taking a choice node with two alternatives yields two distinct builds.
--
-- builds = sum over sets of 2^(choice nodes in the set)
--
-- Both numbers are wanted. The set count is what the enumerator will produce rows for;
-- the build count is what to show a person and what to gate on.
--
-- Existing rows were computed under the old meaning and would be silently wrong under the
-- new column name, so they are deleted rather than reinterpreted. They are derived data
-- and cost milliseconds to recompute.

BEGIN;

DELETE FROM tree_counts;

ALTER TABLE tree_counts
    ADD COLUMN set_count numeric NOT NULL,
    ADD CONSTRAINT tree_counts_set_count_non_negative CHECK (set_count >= 0),
    -- A set with k choice nodes expands to 2^k builds, so builds can never be fewer.
    ADD CONSTRAINT tree_counts_builds_at_least_sets CHECK (build_count >= set_count);

COMMENT ON COLUMN tree_counts.set_count IS
    'Distinct talent selections, with choice-node sides unresolved. One row of enumerator
     output per set.';

COMMENT ON COLUMN tree_counts.build_count IS
    'Distinct builds, with choice-node sides resolved: sum over sets of 2^(choice nodes).
     This is the user-facing number.';

COMMIT;
