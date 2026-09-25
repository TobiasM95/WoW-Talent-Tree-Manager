import { formatCount, productOf } from "../lib/space";

/**
 * How many whole characters the three trees describe, and whether that is simmable.
 *
 * The number that decides what happens next, so it is shown as the product it is: class
 * builds × spec builds × hero builds. Seeing the factors is what tells a player *where* to
 * narrow -- a spec tree contributing 2 million and a hero tree contributing 4 are not the same
 * problem, and one total alone would not say which tree to work on.
 *
 * The limit defaults to 10,000 because SimulationCraft sims every line in full: ten thousand
 * profilesets is an evening at sensible iterations, and much more is not something anyone
 * waits for. It can be raised or lowered, and choice sides are counted, since each side is a
 * separate sim.
 */

export interface SpaceRow {
  label: string;
  builds: number | null;
  fixed: boolean;
  stale: boolean;
  error: string | null;
}

export interface SpaceCardProps {
  rows: SpaceRow[];
  limit: number;
  onLimit: (limit: number) => void;
  pending: string[];
  onSimulate: () => void;
}

export const LIMITS = [1000, 5000, 10000, 25000, 50000] as const;

export function SpaceCard({ rows, limit, onLimit, pending, onSimulate }: SpaceCardProps) {
  const known = rows.every((r) => r.builds !== null && !r.error);
  const total = known ? productOf(rows.map((r) => r.builds!)) : null;
  const stale = rows.some((r) => r.stale);
  const errored = rows.find((r) => r.error);

  const verdict = (() => {
    if (errored) return { ok: false, text: `${errored.label}: ${errored.error}` };
    if (pending.length) return { ok: false, text: `Finish the ${pending[0]}.` };
    if (total === null) return { ok: false, text: "Counting…" };
    if (total === 0) {
      const empty = rows.find((r) => r.builds === 0);
      return {
        ok: false,
        text: `Nothing matches${empty ? ` in the ${empty.label.toLowerCase()} tree` : ""}. Loosen a constraint there.`,
      };
    }
    if (total > limit) {
      // Name the tree doing the most damage, since that is where narrowing pays off most.
      const worst = rows.reduce((a, b) => ((b.builds ?? 0) > (a.builds ?? 0) ? b : a));
      return {
        ok: false,
        text: `${formatCount(total / limit)}× too many to sim. The ${worst.label.toLowerCase()} tree contributes the most — narrow it, or fix it.`,
      };
    }
    return { ok: true, text: "Small enough to sim." };
  })();

  return (
    <section className="panel p-3.5" aria-live="polite">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Possibility space</span>
        <label className="flex items-center gap-1 text-[10.5px] text-ink-faint">
          sim limit
          <select
            className="num rounded-[2px] border border-[color-mix(in_srgb,var(--brass)_30%,transparent)] bg-transparent px-1 text-[11px] text-ink-soft"
            value={limit}
            onChange={(event) => onLimit(Number(event.target.value))}
            aria-label="Most builds to sim"
          >
            {LIMITS.map((l) => (
              <option key={l} value={l}>
                {l.toLocaleString("en-US")}
              </option>
            ))}
          </select>
        </label>
      </div>

      <div
        className="mt-1.5 num text-[30px] leading-none"
        style={{
          color: verdict.ok ? "var(--star)" : "var(--ink)",
          opacity: stale ? 0.55 : 1,
        }}
        data-total={total ?? ""}
      >
        {total === null ? "…" : formatCount(total)}
      </div>
      <div className="mt-0.5 text-[11px] text-ink-faint">builds, choice sides included</div>

      {/* The factors, so the total explains itself. */}
      <ul className="mt-2.5 space-y-0.5 text-[11.5px]">
        {rows.map((r, i) => (
          <li key={r.label} className="flex items-baseline gap-1.5">
            <span className="w-3 text-ink-faint">{i === 0 ? "" : "×"}</span>
            <span className="text-ink-soft">{r.label}</span>
            <span className="chip ml-1 !text-[9.5px]">{r.fixed ? "fixed" : "open"}</span>
            <span className="num ml-auto" style={{ color: r.error ? "var(--barred)" : "var(--ink)" }}>
              {r.error ? "error" : r.builds === null ? "…" : formatCount(r.builds)}
            </span>
          </li>
        ))}
      </ul>

      <p
        className="mt-2.5 text-[11.5px] leading-snug"
        style={{ color: verdict.ok ? "var(--ink-soft)" : "var(--any-of)" }}
      >
        {verdict.text}
      </p>

      <button
        type="button"
        className="btn btn-primary mt-2.5"
        onClick={onSimulate}
        disabled={!verdict.ok || stale}
      >
        {verdict.ok && total !== null
          ? `Simulate ${formatCount(total)} build${total === 1 ? "" : "s"} →`
          : "Simulate →"}
      </button>
    </section>
  );
}
