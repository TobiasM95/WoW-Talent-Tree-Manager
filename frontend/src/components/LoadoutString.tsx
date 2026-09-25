import { useCallback, useEffect, useMemo, useState } from "react";
import type { TreeDetail } from "../lib/api";
import type { Points } from "../lib/loadout";
import { LoadoutStringError, decode, encode } from "../lib/loadoutString";

/**
 * Import and export the string the game itself uses.
 *
 * This is what makes the tool part of a workflow rather than a destination: paste the build
 * you are playing, change it here, paste it back. The same string is what SimulationCraft
 * takes, so it is also the export the sim analysis will need.
 *
 * Importing deliberately says no rather than guessing. A string for another specialisation,
 * a truncated one, or one this tool cannot read produces a message naming the problem --
 * because the alternative is a build that looks right and is not, which a person has no way
 * to notice.
 */

export interface LoadoutStringProps {
  spec: TreeDetail | null;
  trees: TreeDetail[];
  points: Points;
  choices: Record<string, number>;
  /** Which hero sub-tree the current loadout uses; the string has to name it. */
  heroSubTreeId: number | null;
  /**
   * Whether there is one build to write out. With any tree open there is a *set* of builds,
   * and a string of just the fixed trees read as a real loadout with empty trees in it.
   */
  exportable?: boolean;
  onImport: (
    points: Points,
    choices: Record<string, number>,
    heroSubTreeId: number | null,
  ) => void;
}

export function LoadoutString({
  spec,
  trees,
  points,
  choices,
  heroSubTreeId,
  onImport,
  exportable = true,
}: LoadoutStringProps) {
  const [draft, setDraft] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [copied, setCopied] = useState(false);

  const exported = useMemo(() => {
    if (!spec) return "";
    try {
      return encode({ spec, trees, points, choices, heroSubTreeId });
    } catch {
      // A spec whose node order never made it through the ingest cannot be exported. The
      // panel says so below rather than showing an empty box that looks broken.
      return "";
    }
  }, [spec, trees, points, choices, heroSubTreeId]);

  useEffect(() => {
    if (!copied) return;
    const timer = setTimeout(() => setCopied(false), 2000);
    return () => clearTimeout(timer);
  }, [copied]);

  const copy = useCallback(async () => {
    try {
      await navigator.clipboard.writeText(exported);
      setCopied(true);
    } catch {
      setError("Could not reach the clipboard — select the text and copy it.");
    }
  }, [exported]);

  const load = useCallback(() => {
    if (!spec) return;
    try {
      const result = decode(draft, spec, trees);
      onImport(result.points, result.choices, result.heroSubTreeId);
      setDraft("");
      setError(null);
    } catch (exc) {
      setError(
        exc instanceof LoadoutStringError ? exc.message : "That string could not be read.",
      );
    }
  }, [draft, spec, trees, onImport]);

  const spendable = Object.values(points).reduce((sum, n) => sum + n, 0);

  return (
    <section className="panel p-3.5">
      <span className="label">Talent string</span>

      {!spec?.fullNodeOrder ? (
        <p className="mt-2 text-[11.5px] leading-snug text-ink-soft">
          This specialisation has no node order recorded, so its talent string cannot be
          built. Re-run the ingest.
        </p>
      ) : (
        <>
          <p className="mt-1 text-[11.5px] leading-snug text-ink-soft">
            {exportable
              ? "The string the game and SimulationCraft use, covering all three trees."
              : "Paste the string you play to fix all three trees to it. A string to copy appears once every tree is fixed."}
          </p>

          <textarea
            className="mt-2 w-full resize-none rounded-[2px] px-2 py-1.5 text-[11px]"
            style={{
              background: "var(--panel-sunken)",
              border: "1px solid color-mix(in srgb, var(--brass) 30%, transparent)",
              color: "var(--ink-soft)",
              fontFamily: "var(--font-mono)",
              minHeight: "3.4rem",
              wordBreak: "break-all",
            }}
            value={draft || (exportable ? exported : "")}
            onChange={(event) => {
              setDraft(event.target.value);
              setError(null);
            }}
            spellCheck={false}
            aria-label="Talent loadout string"
            placeholder="Paste a talent string here"
          />

          <div className="mt-2 flex gap-1.5">
            <button
              type="button"
              className="btn flex-1"
              onClick={() => void copy()}
              disabled={!exportable || !exported || spendable === 0}
              title={
                !exportable
                  ? "Some trees are open: there is a set of builds, not one"
                  : spendable === 0
                    ? "Spend some points first"
                    : "Copy this build's string"
              }
            >
              {copied ? "Copied" : "Copy"}
            </button>
            <button
              type="button"
              className="btn flex-1"
              onClick={load}
              disabled={!draft.trim()}
              title="Replace the loadout with the pasted string"
            >
              Import
            </button>
          </div>

          {error && (
            <p className="mt-2 text-[11.5px] leading-snug" style={{ color: "var(--barred)" }}>
              {error}
            </p>
          )}

          <p className="mt-2 text-[10.5px] leading-snug text-ink-faint">
            Verified against a string exported from the game — it decodes and re-encodes
            byte for byte.
          </p>
        </>
      )}
    </section>
  );
}
