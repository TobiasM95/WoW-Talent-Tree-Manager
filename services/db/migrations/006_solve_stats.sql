-- Per-talent statistics over one job's result set.
--
-- This is the analytical payoff of enumerating rather than sampling. "Of the 34,619 builds
-- matching your constraints, every one takes Eclipse and 42% take Shooting Stars" is a
-- statement only an exhaustive set can make, and it answers the question a user actually
-- has: which of these choices is already made for me, and which ones are real?
--
-- Computed by the worker immediately after storing, not on demand. The aggregate is a
-- jsonb_each over every result row -- 950 ms for 165,000 builds, and proportionally worse
-- toward the listing limit -- which is fine inside a job that is already asynchronous and
-- has the rows hot, and far too slow for a request a person is waiting on.
CREATE TABLE solve_stats (
    job_id  uuid   NOT NULL REFERENCES solve_jobs(id) ON DELETE CASCADE,
    node_id bigint NOT NULL,
    -- How many of the job's results include this talent at all.
    builds  bigint NOT NULL,
    -- Total points spent on it across those results; divided by `builds` it gives the mean
    -- rank, which is what distinguishes "always taken at 1 of 2" from "always maxed".
    points  bigint NOT NULL,
    PRIMARY KEY (job_id, node_id),
    CONSTRAINT solve_stats_counts_sane CHECK (builds > 0 AND points >= builds)
);

COMMENT ON TABLE solve_stats IS
    'Talent frequency across one job''s results. Derived data: dropping it costs a re-solve,
     not information. Cascades with the job.';

-- The read is always "every node for one job, most common first".
CREATE INDEX solve_stats_job_idx ON solve_stats (job_id, builds DESC);
