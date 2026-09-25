import { useLayoutEffect, useRef, useState } from "react";
import { useNodeNames } from "../lib/nodeNames";
import type { TalentNode } from "../lib/api";

/**
 * Talent tooltip: real content, read constantly, so legibility comes first.
 *
 * Positioned in fixed coordinates from the anchor's own rect rather than inside the
 * transformed canvas world -- otherwise the zoom would scale the text, and a tooltip at 0.4x
 * zoom would be unreadable while the same tooltip at 2.5x would fill the screen.
 */

export interface TooltipProps {
  node: TalentNode;
  anchor: HTMLElement;
}

const GAP = 12;
const MARGIN = 8;

export function Tooltip({ node, anchor }: TooltipProps) {
  const names = useNodeNames();
  const box = useRef<HTMLDivElement>(null);
  const [pos, setPos] = useState<{ left: number; top: number } | null>(null);

  useLayoutEffect(() => {
    const target = anchor.getBoundingClientRect();
    const self = box.current?.getBoundingClientRect();
    if (!self) return;

    // Prefer the right of the node, flip left when that would overflow, and clamp
    // vertically. A tooltip that runs off the viewport is the same as no tooltip.
    let left = target.right + GAP;
    if (left + self.width > window.innerWidth - MARGIN) {
      left = Math.max(MARGIN, target.left - self.width - GAP);
    }
    const top = Math.min(
      Math.max(MARGIN, target.top + target.height / 2 - self.height / 2),
      window.innerHeight - self.height - MARGIN,
    );
    setPos({ left, top });
  }, [anchor, node]);

  const choice = node.kind === "choice" && node.entries.length >= 2;

  /*
    Group entries by name.

    A choice node has two entries with two different names, and both belong in the tooltip.
    A tiered node also has several entries -- but they are ranks of the *same* talent, with
    the same name, so rendering them as separate blocks repeated the title three times and
    made one talent look like three.
  */
  const groups: { name: string; texts: string[] }[] = [];
  for (const entry of node.entries) {
    const name = entry.name || node.name;
    const text = entry.ranks[0];
    const last = groups[groups.length - 1];
    if (last && last.name === name) {
      if (text) last.texts.push(text);
    } else {
      groups.push({ name, texts: text ? [text] : [] });
    }
  }

  return (
    <div
      ref={box}
      className="ttm-tooltip"
      role="tooltip"
      // Rendered off-screen for one frame so it can be measured before being placed;
      // otherwise the first paint shows it in the wrong spot and it visibly jumps.
      style={pos ?? { left: -9999, top: -9999 }}
    >
      {groups.map((group, index) => (
        <div key={group.name + index} className={index > 0 ? "mt-3" : undefined}>
          {index > 0 && <div className="rule mb-2" />}
          <h4>{group.name}</h4>
          {group.texts.length > 0 ? (
            group.texts.map((text, rank) => (
              <p key={rank}>
                {/* Ranks are numbered only when there is more than one; "Rank 1" on a
                    single-rank talent is noise. */}
                {group.texts.length > 1 && (
                  <span className="rank-label">Rank {rank + 1}. </span>
                )}
                {text}
              </p>
            ))
          ) : (
            <p className="italic opacity-70">No description available.</p>
          )}
        </div>
      ))}

      {/*
        What a hero talent modifies, when that ability is in this specialisation.

        Upstream calls it `requiresNode`, and it is not a requirement: decoding every sample
        profile SimulationCraft ships found hero talents taken without it 26 times in 49 --
        24 because the ability belongs to the sibling spec that shares the hero tree, and 2
        by choice. So it is shown as what it is, and only when it names something this
        character could take; the sibling spec's version would be noise.
      */}
      {node.requiresNode && names.get(node.requiresNode) && (
        <p className="!mt-1.5 text-[11.5px]">
          Modifies <span className="text-ink">{names.get(node.requiresNode)!.name}</span>
          <span className="text-ink-faint"> ({names.get(node.requiresNode)!.tree.toLowerCase()} tree)</span>
        </p>
      )}

      <div className="meta">
        {choice ? "Choice" : node.kind === "tiered" ? "Scales with level" : node.kind}
        {node.maxPoints > 1 && ` · ${node.maxPoints} ranks`}
        {node.pointsRequired > 0 && ` · needs ${node.pointsRequired} spent`}
      </div>
    </div>
  );
}
