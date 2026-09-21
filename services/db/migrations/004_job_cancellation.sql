-- The `cancelled` job state has existed since 001 with nothing able to set it.
--
-- Cancelling a queued job is a single UPDATE. Cancelling a *running* one is a request,
-- not a fact: a worker is holding the row, a solver process is running, and neither can
-- be stopped by writing to a table. So intent and outcome are separate columns. Writing
-- state = 'cancelled' directly would claim the job had stopped while its solver was
-- still burning a core, and the worker's own finish() would then overwrite the claim.
--
-- The worker learns about it for free: it already updates progress once a second, so the
-- flag rides back on that statement's RETURNING clause with no extra round trip.
ALTER TABLE solve_jobs ADD COLUMN cancel_requested boolean NOT NULL DEFAULT false;

COMMENT ON COLUMN solve_jobs.cancel_requested IS
    'Someone asked for this job to stop. The worker notices on its next progress write and
     finishes the job as cancelled, discarding partial results. Set, never cleared: a
     cancellation is not something a job recovers from.';

-- Requeueing a job whose cancellation is already pending would hand a worker work that is
-- meant to stop. The sweeper is the only thing that moves a row back to 'queued', so this
-- is where that has to be caught -- see requeue_expired in services/worker/worker.py.
CREATE INDEX solve_jobs_cancelling_idx ON solve_jobs (id)
    WHERE cancel_requested AND state = 'running';
