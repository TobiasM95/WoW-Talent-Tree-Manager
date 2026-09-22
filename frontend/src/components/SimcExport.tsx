import { useCallback, useState } from "react";
import { getResults, type Job, type TreeDetail } from "../lib/api";
import type { Points } from "../lib/loadout";
import { total } from "../lib/loadout";
import { buildStrings, profilesets } from "../lib/simc";

/**
 * Export an enumerated result set as SimulationCraft profilesets.
 *
 * This is the end of the product's arc: count the possibility space, narrow it to something
 * simmable, then hand those builds to the thing that can rank them. The export changes
 * nothing but the talents, so the gear, rotation and fight length stay the player's own --
 * a talent comparison against a profile this tool invented would not mean anything.
 *
 * Each line is a whole character. The solver varies one tree at a time, so every build is the
 * enumerated tree's points combined with whatever the other two hold, which is why a base
 * loadout has to exist first.
 */

export interface SimcExportProps {
  job: Job;
  spec: TreeDetail | null;
  trees: TreeDetail[];
  /** The tree the job enumerated. */
  varying: TreeDetail | null;
  /** The hand-built loadout the other trees keep. */
  base: Points;
  choices: Record<string, number>;
  heroSubTreeId: number | null;
}

/*
  How many builds to offer.

  SimulationCraft runs each profileset as a full simulation, so a few hundred is minutes and a
  few thousand is an evening. The counting gate already refused anything unbounded; this is the
  second, human limit -- what a person will actually wait for.
*/
const SIZES = [50, 200, 1000] as const;
const PAGE = 500;

export function SimcExport({
  job,
  spec,
  trees,
  varying,
  base,
  choices,
  heroSubTreeId,
}: SimcExportProps) {
  const [count, setCount] = useState<number>(SIZES[1]);
  const [withProfile, setWithProfile] = useState(false);
  const [busy, setBusy] = useState(false);
  const [result, setResult] = useState<{ text: string; lines: number; collapsed: number } | null>(
    null,
  );
  const [error, setError] = useState<string | null>(null);

  const available = Math.min(count, job.resultCount ?? 0);
  const baseSpend = total(base);

  const generate = useCallback(async () => {
    if (!spec || !varying) return;
    setBusy(true);
    setError(null);
    try {
      // Paged, because a job can hold two million rows and the export wants the first N of
      // them rather than all of them.
      const builds: Points[] = [];
      for (let offset = 0; offset < available; offset += PAGE) {
        const page = await getResults(job.id, offset, Math.min(PAGE, available - offset));
        builds.push(...page.builds);
        if (page.builds.length === 0) break;
      }
      const { strings, collapsed } = buildStrings({
        spec,
        trees,
        varying,
        base,
        choices,
        heroSubTreeId,
        builds,
      });
      const text = profilesets(strings, {
        className: spec.className,
        specName: spec.specName ?? "",
        withProfile,
        note: `${varying.kind} tree at ${job.points} points, ${
          (job.resultCount ?? 0).toLocaleString("en-US")
        } matching selections`,
      });
      setResult({ text, lines: strings.length, collapsed });
    } catch (exc) {
      setError(exc instanceof Error ? exc.message : String(exc));
    } finally {
      setBusy(false);
    }
  }, [job, spec, trees, varying, base, choices, heroSubTreeId, available, withProfile]);

  const download = useCallback(() => {
    if (!result) return;
    const blob = new Blob([result.text], { type: "text/plain" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `ttm-${spec?.specName?.toLowerCase().replace(/\s+/g, "-") ?? "builds"}.simc`;
    link.click();
    URL.revokeObjectURL(url);
  }, [result, spec]);

  return (
    <section className="panel p-3.5">
      <span className="label">Simulate</span>

      {baseSpend === 0 ? (
        /*
          Stated rather than silently exporting a spec tree with no class talents. Every line
          is a whole character, so without a base loadout the export is a character with two
          empty trees -- which sims, and means nothing.
        */
        <p className="mt-1 text-[11.5px] leading-snug text-ink-soft">
          Spend points in Build first. Each exported line is a whole character, so the class
          and hero trees have to hold something.
        </p>
      ) : (
        <>
          <p className="mt-1 text-[11.5px] leading-snug text-ink-soft">
            SimulationCraft profilesets — one per build, changing nothing but the talents, so
            your gear and rotation stay yours.
          </p>

          <div className="mt-2.5 flex items-center gap-1.5">
            {SIZES.map((size) => (
              <button
                key={size}
                type="button"
                className="btn !px-2 !py-0.5 !text-[11px]"
                aria-pressed={count === size}
                onClick={() => {
                  setCount(size);
                  setResult(null);
                }}
                disabled={(job.resultCount ?? 0) === 0}
              >
                {size.toLocaleString("en-US")}
              </button>
            ))}
            <label className="ml-auto flex items-center gap-1 text-[11px] text-ink-faint">
              <input
                type="checkbox"
                checked={withProfile}
                onChange={(event) => {
                  setWithProfile(event.target.checked);
                  setResult(null);
                }}
                style={{ accentColor: "var(--star)" }}
              />
              runnable
            </label>
          </div>

          <button
            type="button"
            className="btn btn-primary mt-2.5"
            onClick={() => void generate()}
            disabled={busy || available === 0 || !spec || !varying}
          >
            {busy ? "Building…" : `Export ${available.toLocaleString("en-US")} builds`}
          </button>

          {result && (
            <>
              <p className="mt-2 text-[11.5px] leading-snug text-ink-soft">
                <span className="num">{result.lines.toLocaleString("en-US")}</span> profilesets
                {result.collapsed > 0 && (
                  <>
                    {" "}
                    ·{" "}
                    <span className="num">{result.collapsed.toLocaleString("en-US")}</span>{" "}
                    collapsed as duplicates
                  </>
                )}
              </p>
              <textarea
                className="mt-2 w-full resize-none rounded-[2px] px-2 py-1.5 text-[10.5px]"
                style={{
                  background: "var(--panel-sunken)",
                  border: "1px solid color-mix(in srgb, var(--brass) 30%, transparent)",
                  color: "var(--ink-soft)",
                  fontFamily: "var(--font-mono)",
                  minHeight: "5rem",
                  whiteSpace: "pre",
                }}
                readOnly
                value={result.text}
                spellCheck={false}
                aria-label="SimulationCraft profilesets"
                onFocus={(event) => event.currentTarget.select()}
              />
              <button type="button" className="btn mt-1.5 w-full" onClick={download}>
                Download .simc
              </button>
            </>
          )}

          {error && (
            <p className="mt-2 text-[11.5px] leading-snug" style={{ color: "var(--barred)" }}>
              {error}
            </p>
          )}
        </>
      )}
    </section>
  );
}
