/**
 * The shared point pool, for games whose trees draw on one.
 *
 * WoW Forever gives a class 51 points across its three tabs, and a build is described by its
 * split -- "31/20/0" -- more than by anything else. So the split is the headline, with the
 * level it takes to have that many points: vanilla grants the first at level 10 and one a
 * level after, so a build of N points needs level N + 9.
 */

export interface PoolCardProps {
  tabs: { name: string; points: number }[];
  pool: number;
  /** The first level that grants a talent point; null for a game with no levels to show. */
  firstLevel?: number | null;
}

export function PoolCard({ tabs, pool, firstLevel = 10 }: PoolCardProps) {
  const total = tabs.reduce((a, t) => a + t.points, 0);
  const level = firstLevel !== null && total > 0 ? total + firstLevel - 1 : null;
  return (
    <section className="panel p-3.5" aria-label="Talent points">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Talent points</span>
        <span className="num text-[11px] text-ink-faint">
          {level ? `level ${level}` : total === 0 ? "no points spent" : ""}
        </span>
      </div>
      <div className="mt-1.5 num text-[26px] leading-none text-ink" data-split={tabs.map((t) => t.points).join("/")}>
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
        aria-label={`${total} of ${pool} points spent`}
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
        <span className="num text-ink">{total}</span> of <span className="num">{pool}</span> spent
        {total < pool && <> · {pool - total} left</>}
      </p>
    </section>
  );
}
