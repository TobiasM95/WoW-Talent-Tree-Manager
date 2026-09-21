-- A solve job has two phases that cost wildly different amounts of time, and a single
-- `progress` number cannot say which one a job is in.
--
-- Measured on an unfiltered 25-point Balance Druid spec solve (1,906,208 sets): the
-- engine finished in 0.11 s and storing the results took over ten minutes. A progress
-- bar driven only by the engine would have sat at 100% for the entire wait, and one
-- driven by the total would have jumped to 100% in the first tick and then stalled.
--
-- So `progress` is now explicitly per-phase, and `phase` says which. There is a third,
-- 'finalizing': COPY streams two million rows in about 13 seconds and then spends another
-- 15 building the primary key and committing, with nothing left to count. A bar parked at
-- 99% for half the wait is worse than a phase that admits it has no number.
ALTER TABLE solve_jobs ADD COLUMN phase text;

COMMENT ON COLUMN solve_jobs.phase IS
    'Which part of the job progress refers to: solving (the engine enumerating), storing
     (decoding and writing rows), or finalizing (index build and commit, which has no
     meaningful fraction). NULL before a worker picks the job up and after it finishes.';

ALTER TABLE solve_jobs ADD CONSTRAINT solve_jobs_phase_known
    CHECK (phase IS NULL OR phase IN ('solving', 'storing', 'finalizing'));
