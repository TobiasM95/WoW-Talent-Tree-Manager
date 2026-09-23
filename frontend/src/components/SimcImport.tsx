import { useCallback, useRef, useState } from "react";
import type { Exported } from "./SimcExport";
import { parseReport, rank, SimcReportError, type Ranking } from "../lib/simcReport";

/**
 * Read a SimulationCraft report back and rank the builds it measured.
 *
 * The return leg, and the only step that can say which build is *good*. Everything before it
 * is about what is possible: the count says how many builds exist, the enumeration produces
 * them, the statistics say which talents they agree on. None of that knows what a talent is
 * worth, and nothing here can work it out either — only a sim can, and only over a set small
 * enough to sim, which is what the whole filtering arc exists to produce.
 *
 * The file stays on the machine. It is parsed in the browser and nothing is uploaded: a sim
 * report describes somebody's character, gear and rotation, and none of that is this tool's
 * business.
 */

export interface SimcImportProps {
  /** What the export produced, or null if nothing has been exported yet. */
  exported: Exported | null;
  onRanking: (ranking: Ranking | null) => void;
  ranking: Ranking | null;
}

const fmt = (n: number) => Math.round(n).toLocaleString("en-US");
const pct = (n: number) => `${n >= 0 ? "+" : ""}${(n * 100).toFixed(1)}%`;

export function SimcImport({ exported, onRanking, ranking }: SimcImportProps) {
  const [error, setError] = useState<string | null>(null);
  const [note, setNote] = useState<string | null>(null);
  const input = useRef<HTMLInputElement>(null);

  const read = useCallback(
    (text: string) => {
      if (!exported) return;
      try {
        const report = parseReport(text);
        const ranked = rank(report, exported.points.length, exported.label);
        onRanking(ranked);
        setError(null);
        const parts = [`${ranked.builds.length} builds ranked by ${report.metric.toLowerCase()}`];
        if (ranked.missing.length) parts.push(`${ranked.missing.length} not in the report`);
        if (ranked.unmatched.length) parts.push(`${ranked.unmatched.length} from elsewhere`);
        setNote(parts.join(" · "));
      } catch (exc) {
        onRanking(null);
        setNote(null);
        setError(
          exc instanceof SimcReportError
            ? exc.message
            : `Could not read that file: ${exc instanceof Error ? exc.message : String(exc)}`,
        );
      }
    },
    [exported, onRanking],
  );

  const onFile = useCallback(
    (file: File | null | undefined) => {
      if (!file) return;
      void file.text().then(read);
    },
    [read],
  );

  if (!exported) return null;

  return (
    <section
      className="panel p-3.5"
      onDragOver={(event) => event.preventDefault()}
      onDrop={(event) => {
        event.preventDefault();
        onFile(event.dataTransfer.files[0]);
      }}
    >
      <span className="label">Sim results</span>

      {!ranking && (
        <p className="mt-1 text-[11.5px] leading-snug text-ink-soft">
          Run the export, then drop SimulationCraft&apos;s <span className="num">report.json</span>{" "}
          here. Add <span className="num">json2=report.json</span> to the simc command to get one.
          The file is read in your browser and never uploaded.
        </p>
      )}

      <input
        ref={input}
        type="file"
        accept=".json,application/json"
        className="hidden"
        onChange={(event) => onFile(event.target.files?.[0])}
      />

      <div className="mt-2.5 flex gap-1.5">
        <button type="button" className="btn flex-1" onClick={() => input.current?.click()}>
          {ranking ? "Load another report" : "Choose report.json"}
        </button>
        {ranking && (
          <button
            type="button"
            className="btn"
            onClick={() => {
              onRanking(null);
              setNote(null);
            }}
          >
            Clear
          </button>
        )}
      </div>

      {error && (
        <p className="mt-2 text-[11.5px] leading-snug" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      )}

      {ranking && (
        <>
          <p className="mt-2 text-[11px] text-ink-faint">{note}</p>

          <dl className="mt-2.5 space-y-1 text-[11.5px]">
            <div className="flex items-baseline justify-between gap-2">
              <dt className="text-ink-soft">Best</dt>
              <dd className="num text-ink">
                build {ranking.best.line} · {fmt(ranking.best.mean)}
              </dd>
            </div>
            <div className="flex items-baseline justify-between gap-2">
              <dt className="text-ink-soft">Worst</dt>
              <dd className="num text-ink-soft">
                build {ranking.worst.line} · {pct(ranking.worst.behind)}
              </dd>
            </div>
            {ranking.baseline !== null && ranking.baseline > 0 && (
              <div className="flex items-baseline justify-between gap-2">
                <dt className="text-ink-soft">Your profile</dt>
                <dd className="num text-ink-soft">
                  {fmt(ranking.baseline)} · best is {pct(ranking.best.mean / ranking.baseline - 1)}
                </dd>
              </div>
            )}
          </dl>

          {/*
            The honest caveat, and the reason this panel does not just print a winner.

            A sim reports a mean with an error bar, and across a set of similar builds the
            intervals overlap: in the run this was built against, the whole 67-build set spans
            5.9% while each mean carries about ±0.8%. So the top build is genuinely ahead of
            the field, and builds four and five are not distinguishable from each other. A
            ranking that hides that is inventing precision the sim did not produce.
          */}
          <p className="mt-2 text-[11.5px] leading-snug text-ink-faint">
            Spread across the set is {pct(ranking.spread).replace("+", "")}, and each build
            carries ±{((ranking.best.error / ranking.best.mean) * 100).toFixed(1)}%. Builds
            closer together than that are ties, not an order.
          </p>
        </>
      )}
    </section>
  );
}
