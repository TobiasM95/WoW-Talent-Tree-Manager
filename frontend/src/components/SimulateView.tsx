import { useCallback, useEffect, useRef, useState } from "react";
import type { TreeDetail } from "../lib/api";
import { enumerate, type Progress, type TreeInput } from "../lib/enumerate";
import { profilesets, talentStrings } from "../lib/simc";
import { parseReport, rank, SimcReportError, type Ranking } from "../lib/simcReport";
import { formatCount, type Character } from "../lib/space";
import { BuildViewer } from "./BuildViewer";

/**
 * Step two: hand the builds to SimulationCraft, and take its answer back.
 *
 * One page for both directions, because they are one errand: download a file, run the sim,
 * drop the report. Before, the export and the import were two sidebar panels under five
 * others, the export refused to run without a "baseline" built somewhere else, and nothing
 * said what to do with the file once you had it.
 *
 * The builds are enumerated as soon as the page opens -- there is no second "enumerate"
 * button, because arriving here *is* the decision to enumerate.
 */

export interface Sim {
  /** Identifies the searches that produced these characters. */
  signature: string;
  characters: Character[];
  strings: string[];
  text: string;
}

export interface SimulateViewProps {
  signature: string;
  inputs: TreeInput[];
  labels: Record<string, string>;
  limit: number;
  spec: TreeDetail;
  trees: TreeDetail[];
  heroSubTreeId: number | null;
  className: string;
  specName: string;
  sim: Sim | null;
  onSim: (sim: Sim) => void;
  onRanking: (ranking: Ranking) => void;
  onBack: () => void;
}

export function SimulateView({
  signature,
  inputs,
  labels,
  limit,
  spec,
  trees,
  heroSubTreeId,
  className,
  specName,
  sim,
  onSim,
  onRanking,
  onBack,
}: SimulateViewProps) {
  const [progress, setProgress] = useState<Progress[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [reportError, setReportError] = useState<string | null>(null);
  const [over, setOver] = useState(false);
  const [copied, setCopied] = useState(false);
  const input = useRef<HTMLInputElement>(null);

  const current = sim && sim.signature === signature ? sim : null;

  // Enumerate on arrival, and again only if the searches changed since the last time.
  useEffect(() => {
    if (current) return;
    const controller = new AbortController();
    setError(null);
    void enumerate(inputs, limit, setProgress, controller.signal)
      .then((characters) => {
        const strings = talentStrings({ spec, trees, heroSubTreeId, characters });
        const note = inputs
          .map((i) =>
            i.work.mode === "fixed"
              ? `${labels[i.key]} fixed`
              : `${labels[i.key]} open at ${Math.min(i.cap, i.work.search.budget ?? i.cap)} points`,
          )
          .join(", ");
        onSim({
          signature,
          characters,
          strings,
          text: profilesets(strings, { className, specName, note }),
        });
      })
      .catch((exc: unknown) => {
        if ((exc as Error)?.name === "AbortError") return;
        setError(exc instanceof Error ? exc.message : String(exc));
      });
    return () => controller.abort();
    // The signature stands for every input; re-running on identity changes would restart
    // the enumeration on every render.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [signature, current === null]);

  const fileName = `ttm-${className}-${specName}`.toLowerCase().replace(/\s+/g, "-") + ".simc";

  const download = useCallback(() => {
    if (!current) return;
    const url = URL.createObjectURL(new Blob([current.text], { type: "text/plain" }));
    const link = document.createElement("a");
    link.href = url;
    link.download = fileName;
    link.click();
    URL.revokeObjectURL(url);
  }, [current, fileName]);

  const copy = useCallback(() => {
    if (!current) return;
    void navigator.clipboard?.writeText(current.text).then(() => {
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    });
  }, [current]);

  const read = useCallback(
    (text: string) => {
      if (!current) return;
      try {
        const ranking = rank(parseReport(text), current.characters.length);
        setReportError(null);
        onRanking(ranking);
      } catch (exc) {
        setReportError(
          exc instanceof SimcReportError
            ? exc.message
            : `Could not read that file: ${exc instanceof Error ? exc.message : String(exc)}`,
        );
      }
    },
    [current, onRanking],
  );

  const onFile = (file: File | null | undefined) => {
    if (file) void file.text().then(read);
  };

  return (
    <div className="flex min-h-0 w-full flex-1 flex-col gap-3 p-2 md:flex-row md:p-2.5">
    <div className="flex w-full shrink-0 flex-col gap-3 md:w-[26rem] md:overflow-y-auto md:pr-1">
      <div className="flex items-baseline justify-between gap-3">
        <h2 className="display text-[26px] leading-tight">
          {current
            ? `Simulate ${formatCount(current.characters.length)} builds`
            : "Listing the builds…"}
        </h2>
        <button type="button" className="btn" onClick={onBack}>
          ← Back to narrowing
        </button>
      </div>

      {/* 1. what is being listed */}
      <section className="panel p-4">
        <span className="label">1 · The builds</span>
        <ul className="mt-2 space-y-1 text-[12.5px]">
          {inputs.map((i) => {
            const p = progress.find((x) => x.key === i.key);
            const done = i.work.mode === "fixed" || p?.state === "done";
            return (
              <li key={i.key} className="flex items-baseline gap-2">
                <span className="w-28 text-ink-soft">{labels[i.key]}</span>
                <span className="chip">{i.work.mode}</span>
                <span className="num ml-auto" style={{ color: p?.state === "failed" ? "var(--barred)" : "var(--ink)" }}>
                  {i.work.mode === "fixed"
                    ? "1 build"
                    : p?.state === "failed"
                      ? p.error
                      : done && p?.builds !== null && p?.builds !== undefined
                        ? `${formatCount(p.builds)} builds`
                        : p?.state === "fetching"
                          ? "reading…"
                          : "solving…"}
                </span>
              </li>
            );
          })}
        </ul>
        {error && (
          <p className="mt-2 text-[12px]" style={{ color: "var(--barred)" }}>
            {error}
          </p>
        )}
        {current && (
          <p className="mt-2 text-[12px] text-ink-soft">
            Every combination of those, with both sides of each free choice node:{" "}
            <span className="num text-ink">{formatCount(current.characters.length)}</span> whole
            characters, one profileset each.
          </p>
        )}
      </section>

      {/* 2. the file */}
      <section className="panel p-4" aria-disabled={!current}>
        <span className="label">2 · Run them in SimulationCraft</span>
        <div className="mt-2.5 flex flex-wrap gap-2">
          <button type="button" className="btn btn-primary !w-auto" onClick={download} disabled={!current}>
            Download {fileName}
          </button>
          <button type="button" className="btn" onClick={copy} disabled={!current}>
            {copied ? "Copied" : "Copy text"}
          </button>
        </div>
        <p className="mt-3 text-[12px] leading-snug text-ink-soft">
          The file changes nothing but talents, so it goes under a character — your own profile,
          or a sample profile SimulationCraft ships for {specName} {className}:
        </p>
        <pre
          className="mt-2 overflow-x-auto rounded-[2px] px-3 py-2 text-[11.5px] leading-relaxed"
          style={{ background: "var(--panel-sunken)", fontFamily: "var(--font-mono)", color: "var(--ink)" }}
        >
{`simc your-character.simc ${fileName} json2=report.json`}
        </pre>
        <p className="mt-2 text-[11.5px] text-ink-faint">
          SimulationCraft sims every line in full; at 10,000 lines expect minutes to hours,
          depending on iterations and cores.
        </p>
      </section>

      {/* 3. the answer */}
      <section className="panel p-4">
        <span className="label">3 · Bring the report back</span>
        <div
          className="dropzone mt-2.5 flex flex-col items-center justify-center gap-2 px-4 py-6 text-center"
          data-over={over ? "" : undefined}
          onDragOver={(event) => {
            event.preventDefault();
            setOver(true);
          }}
          onDragLeave={() => setOver(false)}
          onDrop={(event) => {
            event.preventDefault();
            setOver(false);
            onFile(event.dataTransfer.files[0]);
          }}
        >
          <p className="text-[13px] text-ink">
            Drop <span className="num">report.json</span> here
          </p>
          <button
            type="button"
            className="btn"
            onClick={() => input.current?.click()}
            disabled={!current}
          >
            or choose the file
          </button>
          <input
            ref={input}
            type="file"
            accept=".json,application/json"
            className="hidden"
            aria-label="SimulationCraft report"
            onChange={(event) => onFile(event.target.files?.[0])}
          />
          <p className="text-[11px] text-ink-faint">
            Read in your browser — never uploaded.
          </p>
        </div>
        {reportError && (
          <p className="mt-2 text-[12px] leading-snug" style={{ color: "var(--barred)" }}>
            {reportError}
          </p>
        )}
      </section>
    </div>

    {/* The builds themselves, to flick through while the sim runs. */}
    {current ? (
      <BuildViewer
        characters={current.characters}
        strings={current.strings}
        trees={inputs.map((i) => ({
          key: i.key,
          label: i.tree.kind === "hero" ? "Hero" : (labels[i.key] ?? i.key),
          tree: i.tree,
        }))}
      />
    ) : (
      <div className="panel flex flex-1 items-center justify-center text-[12px] text-ink-faint">
        The builds appear here once they are listed.
      </div>
    )}
    </div>
  );
}
