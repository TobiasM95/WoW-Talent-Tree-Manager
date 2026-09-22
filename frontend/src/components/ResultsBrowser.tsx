import { useCallback, useEffect, useRef, useState } from "react";
import { getResults, type Job, type ResultPage } from "../lib/api";

/**
 * Stepping through the builds an enumeration produced.
 *
 * The list is not the interesting artefact -- a build is twenty talents, and twenty rows of
 * numbers tell nobody anything. The tree is. So this is a cursor: pick a build and it is
 * drawn on the canvas, with the talents it takes lit and their point counts shown. Moving
 * through builds animates the differences between them, which is the actual question a
 * person has after asking for every build matching their constraints.
 *
 * Paged rather than fetched whole, because "every matching build" reaches two million.
 */

export interface ResultsBrowserProps {
  job: Job;
  index: number;
  onSelect: (index: number, build: Record<string, number> | null) => void;
}

const PAGE = 100;
const fmt = (n: number) => n.toLocaleString("en-US");

export function ResultsBrowser({ job, index, onSelect }: ResultsBrowserProps) {
  const [page, setPage] = useState<ResultPage | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  // Which page the currently selected build lives on. Selection is by absolute index so
  // that paging is an implementation detail the rest of the app never sees.
  const pageStart = Math.floor(index / PAGE) * PAGE;
  const loaded = useRef<number | null>(null);

  useEffect(() => {
    if (loaded.current === pageStart) return;
    loaded.current = pageStart;
    setLoading(true);
    void getResults(job.id, pageStart, PAGE)
      .then((result) => {
        setPage(result);
        setError(null);
      })
      .catch((exc: unknown) => setError(exc instanceof Error ? exc.message : String(exc)))
      .finally(() => setLoading(false));
  }, [job.id, pageStart]);

  const total = page?.total ?? job.resultCount ?? 0;
  const build = page?.builds[index - pageStart] ?? null;

  // Report the selected build upward whenever it resolves, including after a page load.
  useEffect(() => {
    onSelect(index, build);
  }, [index, build, onSelect]);

  const step = useCallback(
    (delta: number) => {
      const next = Math.min(Math.max(0, index + delta), Math.max(0, total - 1));
      onSelect(next, null); // the build follows once its page is in hand
    },
    [index, total, onSelect],
  );

  // Arrow keys step through builds. This is the one control a person uses repeatedly here,
  // and reaching for a button every time would make comparing builds tedious.
  const onKeyDown = useCallback(
    (event: React.KeyboardEvent) => {
      if (event.key === "ArrowLeft" || event.key === "ArrowUp") {
        event.preventDefault();
        step(-1);
      } else if (event.key === "ArrowRight" || event.key === "ArrowDown") {
        event.preventDefault();
        step(1);
      }
    },
    [step],
  );

  return (
    <section className="panel p-3.5">
      <header className="flex items-baseline justify-between gap-3">
        <h2 className="label">Builds</h2>
        <span className="text-[11px] num text-ink-faint">
          {loading ? "loading…" : `${fmt(total)} found`}
        </span>
      </header>

      {error ? (
        <p className="mt-3 text-[13px]" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      ) : total === 0 ? (
        <p className="mt-3 text-[13px] text-ink-soft">This job produced no builds.</p>
      ) : (
        <>
          <div
            className="mt-2.5 flex items-center gap-2"
            tabIndex={0}
            onKeyDown={onKeyDown}
            role="group"
            aria-label="Step through builds"
          >
            <button
              type="button"
              className="btn px-3 py-1 text-[13px]"
              onClick={() => step(-1)}
              disabled={index <= 0}
              aria-label="Previous build"
            >
              ‹
            </button>
            <div className="flex-1 text-center">
              <div className="num text-[15px]">
                {fmt(index + 1)}
                <span className="text-ink-faint"> of {fmt(total)}</span>
              </div>
              <div className="text-[11px] text-ink-faint">arrow keys step</div>
            </div>
            <button
              type="button"
              className="btn px-3 py-1 text-[13px]"
              onClick={() => step(1)}
              disabled={index >= total - 1}
              aria-label="Next build"
            >
              ›
            </button>
          </div>

          <input
            type="range"
            min={0}
            max={Math.max(0, total - 1)}
            value={index}
            onChange={(event) => step(Number(event.target.value) - index)}
            className="mt-3 w-full"
            style={{ accentColor: "var(--taken)" }}
            aria-label="Build number"
          />

          <p className="mt-2 text-[12px] text-ink-soft">
            {build
              ? `${Object.keys(build).length} talents, ${Object.values(build).reduce(
                  (a, b) => a + b,
                  0,
                )} points — shown on the tree.`
              : "Loading this build…"}
          </p>
        </>
      )}
    </section>
  );
}
