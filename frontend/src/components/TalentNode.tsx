import { memo } from "react";
import type { TalentNode as NodeData } from "../lib/api";
import { iconUrl } from "../lib/api";

/**
 * One talent on the canvas.
 *
 * Shape carries meaning, the way the game's own UI does it: the silhouette tells you what
 * kind of node it is before you read anything. That replaces a legend, and it is why the
 * constraint state is drawn with light and weight rather than with yet another colour of
 * border -- the border is already doing a job.
 *
 *   circle          a single talent
 *   split diamond   a choice node: two alternatives, one point
 *   hexagon         a tiered talent, whose max ranks depend on character level
 *   square          a hero sub-tree selector
 */

export type NodeState = "neutral" | "required" | "excluded" | "grouped";

export interface TalentNodeProps {
  node: NodeData;
  /** Canvas pixel position. Kept out of `node` so the API shape stays the API shape. */
  x: number;
  y: number;
  state: NodeState;
  /** Which side of a choice node is pinned, if any. */
  side?: "a" | "b" | "none";
  /** Set while a count is in flight, so the canvas can read as provisional. */
  stale?: boolean;
  /**
   * Points this talent has in the build being inspected, or undefined when no build is
   * selected. A selected build takes over the node's appearance from the constraint state:
   * the question has moved from "what did I ask for" to "what does this build do".
   */
  spent?: number;
  /**
   * How often this talent appears across a job's whole result set, 0..1, or undefined when
   * statistics are not being shown. Drawn as heat, because "which of these is actually a
   * decision" is a question about the shape of the tree, and a person reading a talent tree
   * reads a tree rather than a table.
   */
  share?: number;
  onActivate: (node: NodeData, alternate: boolean) => void;
  onHover: (node: NodeData | null, element: HTMLElement | null) => void;
}

const CLIP: Record<string, string | undefined> = {
  // A hexagon reads as "this one changes with level" without needing a badge.
  tiered: "polygon(25% 2%, 75% 2%, 100% 50%, 75% 98%, 25% 98%, 0% 50%)",
  subtree: undefined, // square: left as a plain box, the most "structural" silhouette
};

const SIZE = 44;

function shapeClass(kind: NodeData["kind"]): string {
  if (kind === "single") return "rounded-full";
  if (kind === "choice") return "rounded-[3px]";
  if (kind === "subtree") return "rounded-[2px]";
  return "rounded-[3px]";
}

function EntryIcon({
  icon,
  name,
  half,
}: {
  icon: string | null;
  name: string;
  half?: "left" | "right";
}) {
  const url = iconUrl(icon, 56);
  return (
    <div
      className="absolute inset-0 bg-cover bg-center"
      style={{
        backgroundImage: url ? `url(${url})` : undefined,
        // A choice node shows both alternatives, split down the middle: the node is one
        // point spent on one of two things, and the silhouette should say so.
        clipPath:
          half === "left"
            ? "polygon(0 0, 100% 0, 0 100%)"
            : half === "right"
              ? "polygon(100% 0, 100% 100%, 0 100%)"
              : undefined,
      }}
      // An icon is decoration here; the name is already in the tooltip and the aria-label.
      role="presentation"
      aria-hidden="true"
      data-icon={url ? undefined : "missing"}
      title={url ? undefined : name}
    />
  );
}

export const TalentNode = memo(function TalentNode({
  node,
  x,
  y,
  state,
  side,
  stale,
  spent,
  share,
  onActivate,
  onHover,
}: TalentNodeProps) {
  const entries = node.entries;
  const isChoice = node.kind === "choice" && entries.length >= 2;

  return (
    <button
      type="button"
      className="ttm-node absolute"
      data-state={state}
      data-kind={node.kind}
      data-spent={spent === undefined ? undefined : spent > 0 ? "yes" : "no"}
      data-share={
        share === undefined ? undefined : share >= 0.999 ? "all" : share > 0 ? "some" : "none"
      }
      data-side={side ?? undefined}
      data-stale={stale ? "" : undefined}
      style={{
        // The share rides along as a custom property so the stylesheet can interpolate the
        // heat, rather than this component deciding what a frequency looks like.
        ...(share === undefined ? {} : { "--share": String(share) }),
        left: x,
        top: y,
        width: SIZE,
        height: SIZE,
        marginLeft: -SIZE / 2,
        marginTop: -SIZE / 2,
        clipPath: CLIP[node.kind],
      }}
      // Left click cycles toward requiring; right click (or shift) cycles the other way.
      // Two directions on one control, because painting constraints is the main gesture in
      // this app and reaching for a mode switch between every node would dominate it.
      onClick={(event) => onActivate(node, event.shiftKey)}
      onContextMenu={(event) => {
        event.preventDefault();
        onActivate(node, true);
      }}
      onMouseEnter={(event) => onHover(node, event.currentTarget)}
      onFocus={(event) => onHover(node, event.currentTarget)}
      onMouseLeave={() => onHover(null, null)}
      onBlur={() => onHover(null, null)}
      aria-label={`${node.name}${
        state === "neutral" ? "" : `, ${state}`
      }. ${node.maxPoints} point${node.maxPoints === 1 ? "" : "s"}.`}
      aria-pressed={state !== "neutral"}
    >
      <span className={`ttm-node-face ${shapeClass(node.kind)}`}>
        {isChoice ? (
          <>
            <EntryIcon icon={entries[0]!.icon} name={entries[0]!.name} half="left" />
            <EntryIcon icon={entries[1]!.icon} name={entries[1]!.name} half="right" />
            <span className="ttm-node-split" aria-hidden="true" />
          </>
        ) : (
          <EntryIcon icon={entries[0]?.icon ?? null} name={entries[0]?.name ?? node.name} />
        )}
      </span>

      {/* Rank pip. While a build is being inspected it shows what that build spends here;
          otherwise it shows the talent's maximum, and only where that says something -- a
          one-point talent does not need "1/1". */}
      {/* A talent no matching build takes is already invisible on the canvas; labelling it
          "0%" adds a row of noise across the unreachable bottom of the tree. */}
      {share !== undefined ? (
        share > 0 ? (
          <span className="ttm-node-ranks" aria-hidden="true">
            {share >= 0.999 ? "all" : `${Math.round(share * 100)}%`}
          </span>
        ) : null
      ) : (spent !== undefined ? spent > 0 : node.maxPoints > 1) ? (
        <span className="ttm-node-ranks" aria-hidden="true">
          {spent !== undefined ? `${spent}/${node.maxPoints}` : node.maxPoints}
        </span>
      ) : null}
    </button>
  );
});
