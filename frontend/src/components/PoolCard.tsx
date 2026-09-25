import type { TreeMode } from "../lib/workspace";
import type { PoolCount } from "../lib/pool";
import { formatCount } from "../lib/space";

/**
 * The shared point pool, for games whose trees draw on one -- and where the class is planned
 * as one build.
 *
 * WoW Forever gives a class 51 points across its three tabs, and a build is described by its
 * split -- "31/20/0" -- more than by anything else. So:
 *
 *   - **Fixed** is a talent calculator: the split is the headline, with the level it takes
 *     (vanilla grants the first point at level 10 and one a level after, so N points need
 *     level N + 9).
 *   - **Open** is one search across every tab: the points to spend, and the splits that hold
 *     the most matching builds. A tab can be held to an exact share on its own pane.
 */

export interface PoolCardProps {
  tabs: { name: string; points: number; exact?: number | null }[];
  pool: number;
  /** The first level that grants a talent point; null for a game with no levels to show. */
  firstLevel?: number | null;
  mode: TreeMode;
  onMode: (mode: TreeMode) => void;
  /** Open: points to spend across the tabs. */
  budget: number;
  onBudget: (points: number) => void;
  /** Open: the pooled count, null while counting. */
  count: PoolCount | null;
}

export function PoolCard({ tabs, pool, firstLevel = 10, mode, onMode, budget, onBudget, count }: PoolCardProps) {
  const spent = tabs.reduce((a, t) => a + t.points, 0);
  const shown = mode === "fixed" ? spent : budget;
  const level = firstLevel !== null && shown > 0 ? shown + firstLevel - 1 : null;
  return (
    <section className="panel p-3.5" aria-label="Talent points">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Talent points</span>
        <span className="num text-[11px] text-ink-faint">
          {level ? `level ${level}` : shown === 0 ? "no points spent" : ""}
        </span>
      </div>

      {/* One switch for the class: its tabs are one build, drawing on one pool. */}
      <div className="seg mt-2 w-full" role="group" aria-label="Class mode">
        {(["fixed", "open"] as const).map((m) => (
          <button
            key={m}
            type="button"
            className="flex-1"
            aria-pressed={mode === m}
            onClick={() => onMode(m)}
            title={
              m === "fixed"
                ? "Plan one build: spend the pool across the tabs by hand"
                : "Search: every build across all tabs that matches what you paint, over every split"
            }
          >
            {m === "fixed" ? "Fixed build" : "Open search"}
          </button>
        ))}
      </div>

      {mode === "fixed" ? (
        <>
          <div className="mt-2.5 num text-[26px] leading-none text-ink" data-split={tabs.map((t) => t.points).join("/")}>
            {tabs.map((t) => t.points).join(" / ")}
          </div>
          <div className="mt-1 flex flex-wrap gap-x-3 text-[11px] text-ink-faint">
            {tabs.map((t) => (
              <span key={t.name}>{t.name}</span>
            ))}
          </div>
          {/* Filled from the left, one segment per tab, so the split reads as proportions. */}
          <div
            className="mt-2.5 flex h-1.5 overflow-hidden rounded-[1px]"
            style={{ background: "color-mix(in srgb, var(--ink-faint) 16%, transparent)" }}
            role="img"
            aria-label={`${spent} of ${pool} points spent`}
          >
            {tabs.map((t, i) => (
              <span
                key={t.name}
                style={{
                  width: `${(t.points / pool) * 100}%`,
                  background: "var(--class-tint)",
                  opacity: [1, 0.7, 0.45][i] ?? 0.45,
                }}
              />
            ))}
          </div>
          <p className="mt-1.5 text-[11.5px] text-ink-soft">
            <span className="num text-ink">{spent}</span> of <span className="num">{pool}</span> spent
            {spent < pool && <> · {pool - spent} left</>}
          </p>
        </>
      ) : (
        <>
          <div className="mt-2.5 flex items-center justify-between gap-2 text-[12px] text-ink-soft">
            <span>Points to spend</span>
            <span className="flex items-center gap-1">
              <button
                type="button"
                className="step"
                onClick={() => onBudget(Math.max(1, budget - 1))}
                disabled={budget <= 1}
                aria-label="One point fewer"
              >
                −
              </button>
              <span className="num min-w-[3.2rem] text-center text-ink" data-pool-budget={budget}>
                {budget}
                <span className="text-ink-faint">/{pool}</span>
              </span>
              <button
                type="button"
                className="step"
                onClick={() => onBudget(Math.min(pool, budget + 1))}
                disabled={budget >= pool}
                aria-label="One point more"
              >
                +
              </button>
            </span>
          </div>
          <p className="mt-1 text-[11px] leading-snug text-ink-faint">
            Every split of these points across {tabs.map((t) => t.name).join(", ")}
            {tabs.some((t) => t.exact !== null && t.exact !== undefined)
              ? `, with ${tabs
                  .filter((t) => t.exact !== null && t.exact !== undefined)
                  .map((t) => `exactly ${t.exact} in ${t.name}`)
                  .join(" and ")}`
              : ""}
            .
          </p>
          {count && count.top.length > 0 && (
            <div className="mt-2.5">
              <span className="text-[11px] text-ink-soft">
                Splits with the most builds <span className="text-ink-faint">· {count.splits.toLocaleString("en-US")} possible</span>
              </span>
              <ul className="mt-1 space-y-0.5" aria-label="Splits">
                {count.top.map((s) => (
                  <li key={s.split.join("/")} className="flex items-baseline justify-between text-[11.5px]">
                    <span className="num text-ink">{s.split.join(" / ")}</span>
                    <span className="num text-ink-faint">{formatCount(s.builds)}</span>
                  </li>
                ))}
              </ul>
            </div>
          )}
        </>
      )}
    </section>
  );
}
