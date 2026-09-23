import { useEffect, useMemo, useState } from "react";
import type { TreeDetail } from "../lib/api";
import { impact, type Ranking, type TalentImpact } from "../lib/simcReport";
import type { Exported } from "./SimcExport";

/**
 * What each talent was worth, across the set that was simmed.
 *
 * The thing this whole tool was built to be able to say. A sim ranks whole characters, so the
 * value of a single talent is not something SimulationCraft reports — it only appears once a
 * *set* of builds that differ in a controlled way has been simmed, and producing exactly that
 * set is what counting, filtering and enumerating were all for.
 *
 * The number is the mean of the builds that take a talent against the mean of those that do
 * not. Two honest limits come with it, and both are stated in the panel rather than buried
 * here: it is observational, so talents that are only ever taken together cannot be told
 * apart, and the differences are only meaningful next to the sim's own error bars.
 */

export interface TalentImpactPanelProps {
  ranking: Ranking;
  exported: Exported;
  tree: TreeDetail;
  /** Publishes the per-node impact so the canvas can paint it. */
  onHighlight: (heat: Map<number, number> | null) => void;
}

const pct = (n: number) => `${n >= 0 ? "+" : "−"}${Math.abs(n * 100).toFixed(1)}%`;

export function TalentImpactPanel({
  ranking,
  exported,
  tree,
  onHighlight,
}: TalentImpactPanelProps) {
  const [showing, setShowing] = useState(true);

  const impacts = useMemo(
    () =>
      impact(ranking, (line) => {
        const points = exported.points[line - 1];
        return points ? Object.keys(points).map(Number) : undefined;
      }),
    [ranking, exported],
  );

  const names = useMemo(
    () => new Map(tree.nodes.map((node) => [node.nodeId, node.name])),
    [tree],
  );

  // Only talents this tree owns: the export holds the varying tree's points, but a stale
  // ranking against another tree would otherwise list node ids with no names.
  const mine = useMemo(
    () => impacts.filter((row) => names.has(row.nodeId)),
    [impacts, names],
  );

  /*
    The scale is set by the largest swing in the set, not by a fixed range.

    Talent differences are small -- a few percent across a whole set is normal -- so a scale
    fixed at ±10% would render every real result as the same flat colour.
  */
  const widest = useMemo(
    () => Math.max(0.0001, ...mine.map((row) => Math.abs(row.delta))),
    [mine],
  );

  useEffect(() => {
    if (!showing) {
      onHighlight(null);
      return;
    }
    onHighlight(new Map(mine.map((row) => [row.nodeId, row.delta / widest])));
    return () => onHighlight(null);
  }, [mine, widest, showing, onHighlight]);

  if (!mine.length) {
    return (
      <section className="panel p-3.5">
        <span className="label">Talent value</span>
        <p className="mt-1 text-[11.5px] leading-snug text-ink-soft">
          Every simmed build takes the same talents in this tree, so there is nothing to
          compare. Loosen a constraint and sim again.
        </p>
      </section>
    );
  }

  const best = mine.slice(0, 5);
  const worst = mine.slice(-5).reverse();
  const errorPct = (ranking.best.error / ranking.best.mean) * 100;

  return (
    <section className="panel p-3.5">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Talent value</span>
        <button
          type="button"
          className="btn !px-2 !py-0.5 !text-[11px]"
          onClick={() => setShowing(!showing)}
          aria-pressed={showing}
        >
          {showing ? "On the tree" : "Show on tree"}
        </button>
      </div>

      <p className="mt-1.5 text-[11.5px] leading-snug text-ink-soft">
        Builds taking each talent, against builds that do not.
      </p>

      <ul className="mt-2 space-y-0.5">
        {best.map((row) => (
          <Row key={row.nodeId} row={row} name={names.get(row.nodeId)} total={mine.length} />
        ))}
      </ul>

      {worst.length > 0 && best[best.length - 1] !== worst[worst.length - 1] && (
        <ul className="mt-2 space-y-0.5 border-t border-[color-mix(in_srgb,var(--brass)_20%,transparent)] pt-2">
          {worst.map((row) => (
            <Row key={row.nodeId} row={row} name={names.get(row.nodeId)} total={mine.length} />
          ))}
        </ul>
      )}

      {/*
        Two caveats, stated where the number is read rather than in a footnote.

        The first is statistical: a difference smaller than the sim's own error bar is not a
        difference. The second is structural and easier to miss -- this is an observational
        comparison over builds the enumerator produced, so two talents that the tree forces
        to be taken together will show the same value, and neither of them earned it alone.
      */}
      <p className="mt-2 text-[11px] leading-snug text-ink-faint">
        Each build carries ±{errorPct.toFixed(1)}%; smaller differences are noise. Talents the
        tree never separates share a score.
      </p>
    </section>
  );
}

function Row({
  row,
  name,
  total,
}: {
  row: TalentImpact;
  name: string | undefined;
  total: number;
}) {
  const good = row.delta >= 0;
  return (
    <li className="flex items-baseline gap-2 text-[11.5px] leading-snug">
      <span className="truncate text-ink" title={name}>
        {name ?? `Node ${row.nodeId}`}
      </span>
      <span
        className="num ml-auto shrink-0"
        style={{ color: good ? "var(--must)" : "var(--barred)" }}
        title={`${Math.round(row.withIt).toLocaleString("en-US")} with, ${Math.round(
          row.without,
        ).toLocaleString("en-US")} without`}
      >
        {pct(row.delta)}
      </span>
      <span className="num shrink-0 text-[10px] text-ink-faint" title={`${row.taken} of the ranked builds take it`}>
        {row.taken}
      </span>
      <span className="sr-only">of {total} talents compared</span>
    </li>
  );
}
