import { useEffect, useState } from "react";
import { getStats, type Job, type JobStats, type TreeDetail } from "../lib/api";

/**
 * What every matching build has in common, and where the real choices are.
 *
 * This is the payoff of enumerating rather than sampling: a statement about the whole
 * matching set rather than about a draw from it. Three readings, in the order they are
 * useful:
 *
 *   settled   taken by every matching build -- your constraints already decided these
 *   open      somewhere in between -- this is where the decision actually lives
 *   rare      taken by few -- usually a talent your constraints have nearly excluded
 *
 * The list is secondary to the canvas, which shows the same numbers as heat. A person
 * reading a talent tree reads a tree.
 */

export interface StatsPanelProps {
  job: Job;
  tree: TreeDetail;
  showing: boolean;
  onToggle: (showing: boolean) => void;
  onStats: (stats: JobStats | null) => void;
}

export function StatsPanel({ job, tree, showing, onToggle, onStats }: StatsPanelProps) {
  const [stats, setStats] = useState<JobStats | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;
    void getStats(job.id)
      .then((result) => {
        if (cancelled) return;
        setStats(result);
        onStats(result);
      })
      .catch((exc: unknown) => {
        if (!cancelled) setError(exc instanceof Error ? exc.message : String(exc));
      });
    return () => {
      cancelled = true;
    };
  }, [job.id, onStats]);

  if (error) {
    return (
      <section className="panel framed grain p-4">
        <h2 className="text-[13px] tracking-[0.14em] uppercase text-ink-faint">
          What they share
        </h2>
        <p className="mt-2 text-[13px]" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      </section>
    );
  }

  const nameOf = (nodeId: number) =>
    tree.nodes.find((n) => n.nodeId === nodeId)?.name ?? String(nodeId);

  const settled = stats?.talents.filter((t) => t.mandatory) ?? [];
  // Sorted by distance from a coin flip: a talent at 50% is the most informative thing on
  // the list, and one at 3% or 97% is nearly decided already.
  const open = (stats?.talents.filter((t) => !t.mandatory) ?? [])
    .slice()
    .sort((a, b) => Math.abs(a.share - 0.5) - Math.abs(b.share - 0.5))
    .slice(0, 6);

  return (
    <section className="panel framed grain p-4">
      <header className="flex items-baseline justify-between gap-3">
        <h2 className="text-[13px] tracking-[0.14em] uppercase text-ink-faint">
          What they share
        </h2>
        <label className="flex items-center gap-1.5 text-[11px] text-ink-faint">
          <input
            type="checkbox"
            checked={showing}
            onChange={(event) => onToggle(event.target.checked)}
            style={{ accentColor: "var(--arcane)" }}
          />
          {/* Also the way back from inspecting a single build, which is otherwise a
              one-way door. */}
          on the tree
        </label>
      </header>

      {!stats ? (
        <p className="mt-2 text-[13px] text-ink-faint">Reading the results…</p>
      ) : (
        <>
          <p className="mt-2 text-[12px] text-ink-soft">
            {settled.length > 0 ? (
              <>
                <span className="tabular">{settled.length}</span> talent
                {settled.length === 1 ? " is" : "s are"} in <em>every</em> matching build.
                Those are already decided.
              </>
            ) : (
              <>No talent appears in every matching build.</>
            )}
          </p>

          {open.length > 0 && (
            <>
              <div className="rule my-3" />
              <h3 className="text-[11px] uppercase tracking-[0.1em] text-ink-faint">
                Where the choice actually is
              </h3>
              <ul className="mt-2 space-y-1.5">
                {open.map((talent) => (
                  <li key={talent.nodeId} className="text-[12px]">
                    <div className="flex items-baseline justify-between gap-2">
                      <span className="truncate">{nameOf(talent.nodeId)}</span>
                      <span className="tabular shrink-0 text-ink-faint">
                        {(talent.share * 100).toFixed(0)}%
                      </span>
                    </div>
                    <div
                      className="mt-0.5 h-[3px]"
                      style={{ background: "var(--panel-sunken)" }}
                      aria-hidden="true"
                    >
                      <div
                        className="h-full"
                        style={{
                          width: `${talent.share * 100}%`,
                          background: "var(--arcane)",
                        }}
                      />
                    </div>
                  </li>
                ))}
              </ul>
            </>
          )}
        </>
      )}
    </section>
  );
}
