/**
 * What the colours and the shapes on the canvas mean.
 *
 * Both halves exist because the canvas says things silently. A ring colour is a constraint
 * kind and a silhouette is a talent kind, and neither was written down anywhere: "at least
 * one" and "exactly one" in particular were the same ring, so the one control that
 * distinguished them was a button the player had already clicked and forgotten.
 *
 * The swatches are the real thing rather than a drawing of it -- same keyline sandwich, same
 * clip paths, driven by the same custom properties -- so a legend cannot drift from the
 * canvas it describes.
 */

import type { NodeState } from "./TalentNode";

type Kind = "passive" | "active" | "choice" | "subtree";

export function Swatch({ state, shape }: { state?: NodeState; shape?: Kind }) {
  return (
    <span className="ttm-swatch" data-state={state} data-shape={shape ?? "passive"}>
      <span className="ttm-swatch-band" />
    </span>
  );
}

const CONSTRAINTS: { state: NodeState; label: string; meaning: string }[] = [
  { state: "required", label: "Required", meaning: "in every build" },
  { state: "excluded", label: "Barred", meaning: "in none" },
  { state: "anyOf", label: "At least one", meaning: "one or more of the group" },
  { state: "oneOf", label: "Exactly one", meaning: "one of the group only" },
];

export interface ConstraintLegendProps {
  counts: Record<string, number>;
}

export function ConstraintLegend({ counts }: ConstraintLegendProps) {
  return (
    <ul className="mt-2.5 space-y-1">
      {CONSTRAINTS.map(({ state, label, meaning }) => (
        <li key={state} className="flex items-baseline gap-1.5 text-[11.5px] leading-snug">
          <Swatch state={state} />
          <span className="whitespace-nowrap text-ink">{label}</span>
          <span className="truncate text-ink-faint" title={meaning}>
            — {meaning}
          </span>
          {counts[state] ? (
            <span className="num ml-auto text-[10.5px] text-ink-faint">{counts[state]}</span>
          ) : null}
        </li>
      ))}
    </ul>
  );
}

const SHAPES: { shape: Kind; label: string }[] = [
  { shape: "passive", label: "Passive" },
  { shape: "active", label: "Ability" },
  { shape: "choice", label: "Choice of two" },
  { shape: "subtree", label: "Hero tree" },
];

export function ShapeKey() {
  return (
    <div className="flex flex-wrap items-center gap-x-3 gap-y-1">
      {SHAPES.map(({ shape, label }) => (
        <span key={shape} className="flex items-center gap-1 text-[10.5px] text-ink-faint">
          <Swatch shape={shape} />
          {label}
        </span>
      ))}
    </div>
  );
}
