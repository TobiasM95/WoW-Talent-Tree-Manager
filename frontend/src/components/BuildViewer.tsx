import { useCallback, useState } from "react";
import type { TreeDetail } from "../lib/api";
import * as loadout from "../lib/loadout";
import { formatCount, lineName, type Character } from "../lib/space";
import { TreePane } from "./TreePane";

/**
 * Flick through the builds about to be simmed, drawn on the three trees.
 *
 * Nothing here changes what gets exported. It is for looking: a list of 9,000 talent strings
 * says nothing, while stepping through them on the trees shows what the search actually
 * produced, and whether it is the set that was meant.
 */

export interface BuildViewerProps {
  characters: Character[];
  strings: string[];
  trees: { key: string; label: string; tree: TreeDetail }[];
}

export function BuildViewer({ characters, strings, trees }: BuildViewerProps) {
  const [index, setIndex] = useState(0);
  const [copied, setCopied] = useState(false);
  const total = characters.length;
  const character = characters[Math.min(index, total - 1)];

  const go = useCallback(
    (next: number) => setIndex(Math.max(0, Math.min(total - 1, next))),
    [total],
  );

  // What changed from the build before, which is what flicking is for.
  const previous = index > 0 ? characters[index - 1] : null;
  const nameOf = (id: string) => {
    for (const t of trees) {
      const node = t.tree.nodes.find((n) => String(n.nodeId) === id);
      if (node) return node;
    }
    return null;
  };
  const changes = (() => {
    if (!previous || !character) return null;
    const out: string[] = [];
    for (const [id, p] of Object.entries(character.points)) {
      const before = previous.points[id] ?? 0;
      if (p > before) out.push(`+${nameOf(id)?.name ?? id}${before ? ` ${before}→${p}` : ""}`);
    }
    for (const [id, p] of Object.entries(previous.points)) {
      if (!character.points[id] && p) out.push(`−${nameOf(id)?.name ?? id}`);
    }
    for (const [id, side] of Object.entries(character.choices)) {
      const was = previous.choices[id];
      if (was !== undefined && was !== side && character.points[id] && previous.points[id]) {
        out.push(nameOf(id)?.entries[side]?.name ?? id);
      }
    }
    return out;
  })();

  if (!character) return null;

  const copy = () => {
    const text = strings[index];
    if (!text) return;
    void navigator.clipboard?.writeText(text).then(() => {
      setCopied(true);
      setTimeout(() => setCopied(false), 1500);
    });
  };

  return (
    <section
      className="flex min-h-0 flex-1 flex-col gap-2 outline-none"
      tabIndex={0}
      aria-label="Browse the builds"
      onKeyDown={(event) => {
        const step = event.shiftKey ? 10 : 1;
        if (event.key === "ArrowRight" || event.key === "ArrowDown") {
          event.preventDefault();
          go(index + step);
        } else if (event.key === "ArrowLeft" || event.key === "ArrowUp") {
          event.preventDefault();
          go(index - step);
        } else if (event.key === "Home") {
          go(0);
        } else if (event.key === "End") {
          go(total - 1);
        }
      }}
    >
      <div className="panel flex flex-wrap items-center gap-x-3 gap-y-2 px-3.5 py-2.5">
        <span className="label">Browse</span>
        <button type="button" className="btn !px-2.5" onClick={() => go(index - 1)} disabled={index === 0} aria-label="Previous build">
          ‹
        </button>
        <span className="num text-[14px] text-ink" data-browse-index={index + 1}>
          {(index + 1).toLocaleString("en-US")}
          <span className="text-ink-faint"> of {formatCount(total)}</span>
        </span>
        <button type="button" className="btn !px-2.5" onClick={() => go(index + 1)} disabled={index >= total - 1} aria-label="Next build">
          ›
        </button>
        <input
          type="range"
          min={1}
          max={total}
          value={index + 1}
          onChange={(event) => go(Number(event.target.value) - 1)}
          className="min-w-[8rem] flex-1"
          aria-label="Build number"
        />
        <span className="num text-[11px] text-ink-faint">
          {lineName(character.line, Math.max(2, String(total).length))}
        </span>
        <button type="button" className="btn !py-1 !text-[11px]" onClick={copy}>
          {copied ? "Copied" : "Copy talent string"}
        </button>
        <p className="w-full truncate text-[11.5px] text-ink-soft" title={changes?.join("  ")}>
          {changes === null
            ? "Arrow keys step, shift+arrow jumps ten."
            : changes.length
              ? <>vs the build before: <span className="text-ink">{changes.join("  ")}</span></>
              : "Same talents as the build before."}
        </p>
      </div>

      <div className="flex min-h-[26rem] flex-1 flex-col gap-2 md:min-h-0 md:flex-row">
        {/* With both hero trees simmed, a build uses one of them: draw that one. */}
        {trees.filter((t) => t.tree.kind !== "hero" || character.parts[t.key]).map((t) => {
          const part = character.parts[t.key];
          const drawn: loadout.Points = { ...(part?.points ?? {}) };
          for (const id of loadout.grantedRoots(t.tree)) {
            drawn[String(id)] = t.tree.nodes.find((n) => n.nodeId === id)?.maxPoints ?? 1;
          }
          const sides = new Map<number, "a" | "b" | "none">(
            Object.entries(part?.choices ?? {}).map(([id, s]) => [Number(id), s === 1 ? "b" : "a"]),
          );
          return (
            <TreePane
              key={t.key}
              tree={t.tree}
              title={t.label}
              build={drawn}
              sides={sides}
              className={t.label === "Hero" ? "min-h-[16rem] md:w-[19rem] md:shrink-0" : "min-h-[20rem] flex-1"}
            />
          );
        })}
      </div>
    </section>
  );
}
