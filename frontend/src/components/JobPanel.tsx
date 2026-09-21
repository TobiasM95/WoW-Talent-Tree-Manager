import type { Job } from "../lib/api";

/**
 * A running or finished enumeration.
 *
 * Progress is shown per phase, because the phases are not comparable: an unfiltered
 * 25-point spec solve enumerates 1.9 million selections in 0.11 s and then spends the rest
 * of its wall clock storing them. A single bar driven by the solver would sit at 100% for
 * almost the whole wait, so the phase name is part of the reading.
 *
 * `finalizing` has no fraction at all -- the server is building an index over rows already
 * written -- and it says so rather than inventing a number.
 */

export interface JobPanelProps {
  job: Job;
  onCancel: () => void;
  onDismiss: () => void;
}

const PHASE_LABEL: Record<string, string> = {
  solving: "Searching the tree",
  storing: "Storing builds",
  finalizing: "Finishing up",
};

const STATE_LABEL: Record<Job["state"], string> = {
  queued: "Queued",
  running: "Running",
  done: "Complete",
  capped: "Partial",
  cancelled: "Cancelled",
  failed: "Failed",
};

const fmt = (n: number) => n.toLocaleString("en-US");

export function JobPanel({ job, onCancel, onDismiss }: JobPanelProps) {
  const running = job.state === "queued" || job.state === "running";
  const indeterminate = job.phase === "finalizing";
  const tone =
    job.state === "failed"
      ? "var(--barred)"
      : job.state === "done"
        ? "var(--taken)"
        : job.state === "cancelled"
          ? "var(--ink-faint)"
          : "var(--arcane)";

  return (
    <section className="panel framed grain p-4" aria-live="polite">
      <header className="flex items-baseline justify-between gap-3">
        <h2 className="text-[13px] tracking-[0.14em] uppercase text-ink-faint">
          Enumeration
        </h2>
        <span className="text-[11px] uppercase tracking-[0.1em]" style={{ color: tone }}>
          {STATE_LABEL[job.state]}
          {job.cancelRequested && running ? " · stopping" : ""}
        </span>
      </header>

      {running && (
        <>
          <p className="mt-2 text-[13px] text-ink-soft">
            {job.phase ? (PHASE_LABEL[job.phase] ?? job.phase) : "Waiting for a worker"}
            {!indeterminate && job.phase && (
              <span className="tabular"> · {Math.round(job.progress * 100)}%</span>
            )}
          </p>
          <div
            className="mt-2 h-[6px] overflow-hidden"
            style={{
              background: "var(--panel-sunken)",
              border: "1px solid var(--metal-dim)",
              borderRadius: "1px",
            }}
            role="progressbar"
            aria-valuenow={indeterminate ? undefined : Math.round(job.progress * 100)}
            aria-valuemin={0}
            aria-valuemax={100}
            aria-label={job.phase ?? "queued"}
          >
            <div
              className="h-full"
              style={{
                width: indeterminate ? "100%" : `${Math.max(2, job.progress * 100)}%`,
                background: `linear-gradient(90deg, var(--arcane-dim), var(--arcane-bright))`,
                boxShadow: "var(--arcane-glow)",
                opacity: indeterminate ? 0.45 : 1,
                transition: "width 320ms ease-out",
              }}
            />
          </div>
        </>
      )}

      {job.expectedCount !== null && (
        <p className="mt-3 text-[12px] text-ink-faint tabular">
          {job.resultCount !== null
            ? `${fmt(job.resultCount)} selections stored`
            : `${fmt(job.expectedCount)} selections expected`}
        </p>
      )}

      {job.error && (
        <p
          className="mt-2 text-[12px]"
          style={{ color: job.state === "capped" ? "var(--metal)" : "var(--barred)" }}
        >
          {job.error}
        </p>
      )}

      <div className="mt-4 flex gap-2">
        {running ? (
          <button
            type="button"
            className="btn flex-1"
            onClick={onCancel}
            disabled={job.cancelRequested}
          >
            {job.cancelRequested ? "Stopping…" : "Cancel"}
          </button>
        ) : (
          <button type="button" className="btn flex-1" onClick={onDismiss}>
            Dismiss
          </button>
        )}
      </div>
    </section>
  );
}
