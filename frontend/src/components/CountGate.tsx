import type { CountResult } from "../lib/api";

/**
 * The pre-flight count: the app's distinctive feature, so it gets the starlight accent and the
 * biggest number on screen.
 *
 * Two numbers, because they answer different questions. `builds` is what a person means by
 * "how many builds"; `sets` is how many rows an enumeration would produce. Conflating them
 * understates the answer by up to ~50x, so both are shown and labelled rather than one being
 * quietly chosen.
 *
 * `listable` is the point of the whole thing: the count is free, so the UI can say up front
 * whether enumerating is worth offering instead of letting a worker discover it.
 */

export interface CountGateProps {
  result: CountResult | null;
  error: string | null;
  stale: boolean;
  pending: string[];
  onSolve: () => void;
  solveDisabled: boolean;
}

const fmt = (n: number) => n.toLocaleString("en-US");

export function CountGate({
  result,
  error,
  stale,
  pending,
  onSolve,
  solveDisabled,
}: CountGateProps) {
  return (
    <section className="panel p-3.5" aria-live="polite">
      <header className="flex items-baseline justify-between gap-3">
        <h2 className="label">Possibility space</h2>
        {result && (
          <span className="text-[11px] num text-ink-faint">
            {result.source === "precomputed" ? "precomputed" : `${result.elapsedMs} ms`}
          </span>
        )}
      </header>

      {error ? (
        <p className="mt-3 text-[13px]" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      ) : (
        <>
          <div
            className="mt-1.5 num-display leading-none"
            style={{
              // The count is the biggest thing on the page and the instrument face carries
              // it: a measurement, read off a dial, not a headline.
              fontSize: "clamp(1.9rem, 5vw, 2.5rem)",
              color: "var(--star)",
              textShadow: "var(--star-glow)",
              // While a count is in flight the number is the previous answer, so it says so
              // rather than pretending to be current.
              opacity: stale ? 0.45 : 1,
              transition: "opacity 120ms",
            }}
          >
            {result ? fmt(result.builds) : "—"}
          </div>
          <div className="mt-1 text-[12px] text-ink-soft">
            builds
            {result && (
              <>
                {" · "}
                <span className="num">{fmt(result.sets)}</span> selections
              </>
            )}
          </div>

          {result && (
            <div className="mt-3">
              <div className="rule" />
              <p className="mt-3 text-[13px] text-ink-soft">
                {result.sets === 0 ? (
                  <>Nothing matches. Loosen a constraint.</>
                ) : result.listable ? (
                  <>
                    Small enough to enumerate. Every matching build can be listed and
                    simulated.
                  </>
                ) : (
                  <>
                    Too many to enumerate — the limit is{" "}
                    <span className="num">{fmt(result.listingLimit)}</span> selections.
                    Add constraints to narrow it.
                  </>
                )}
              </p>
            </div>
          )}
        </>
      )}

      {pending.length > 0 && (
        <ul className="mt-3 space-y-1 text-[12px]" style={{ color: "var(--brass)" }}>
          {pending.map((note) => (
            <li key={note}>{note}</li>
          ))}
        </ul>
      )}

      <button
        type="button"
        className="btn btn-primary mt-3.5"
        onClick={onSolve}
        disabled={solveDisabled}
      >
        Enumerate matching builds
      </button>
    </section>
  );
}
