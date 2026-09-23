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

export type NodeState = "neutral" | "required" | "excluded" | "anyOf" | "oneOf";

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
  /**
   * What the sim said this talent is worth, from -1 to 1, scaled to the widest swing in the
   * set. Signed, so it needs a diverging treatment rather than the one-ended heat `share`
   * uses: "this talent costs you damage" is a different statement from "few builds take it".
   */
  impact?: number;
  /**
   * Build mode: `spent` is what this node holds, and `reachable` says whether another point
   * could go in right now. Out-of-reach talents recede rather than disappear -- a player is
   * choosing *where to go next*, and a tree with the unreachable parts hidden cannot answer
   * that.
   */
  reachable?: boolean;
  /**
   * True while points are being spent by hand.
   *
   * `spent` means two different things depending on this: in build mode it is what the
   * player has put into the node, and in explore mode it is what the enumerated build being
   * inspected contains. The two need opposite treatments -- an untaken talent you could
   * still take must stay inviting, while one absent from a finished build should recede --
   * so they are distinguished here rather than sharing one attribute and one set of rules.
   */
  editing?: boolean;
  onActivate: (node: NodeData, alternate: boolean) => void;
  onHover: (node: NodeData | null, element: HTMLElement | null) => void;
}

const SIZE = 44;

/** A constraint state read aloud. "anyOf" is a variable name, not a sentence. */
const SPOKEN: Record<NodeState, string> = {
  neutral: "",
  required: "required",
  excluded: "barred",
  anyOf: "in an at-least-one group",
  oneOf: "in an exactly-one group",
};

/**
 * What silhouette a talent gets, which is a question about the talent rather than about
 * the tree structure.
 *
 * The game draws passives as circles and abilities as squares, and a player reading a tree
 * uses that before reading a single word: "what does this build actually *do*" is answered
 * by counting squares. We had circles for everything with one point, boxes for everything
 * with two, which encoded the data model instead of the thing.
 *
 * A choice node is a hexagon whatever it contains -- its shape has to say "a decision" more
 * loudly than it says "an ability", because that is what distinguishes it. The hero-tree
 * selector is the octagon, and there is exactly one per tree.
 */
function shapeOf(node: NodeData): "passive" | "active" | "choice" | "subtree" {
  if (node.kind === "subtree") return "subtree";
  if (node.kind === "choice" && node.entries.length >= 2) return "choice";
  return node.entries.some((entry) => entry.kind === "active") ? "active" : "passive";
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
  impact,
  reachable,
  editing,
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
      data-shape={shapeOf(node)}
      data-spent={spent === undefined ? undefined : spent > 0 ? "yes" : "no"}
      data-share={
        share === undefined ? undefined : share >= 0.999 ? "all" : share > 0 ? "some" : "none"
      }
      data-impact={impact === undefined ? undefined : impact >= 0 ? "good" : "bad"}
      data-reachable={reachable === undefined ? undefined : reachable ? "yes" : "no"}
      data-editing={editing ? "" : undefined}
      data-side={side ?? undefined}
      data-stale={stale ? "" : undefined}
      style={{
        // The share rides along as a custom property so the stylesheet can interpolate the
        // heat, rather than this component deciding what a frequency looks like.
        ...(share === undefined ? {} : { "--share": String(share) }),
        ...(impact === undefined ? {} : { "--impact": String(Math.abs(impact)) }),
        left: x,
        top: y,
        width: SIZE,
        height: SIZE,
        marginLeft: -SIZE / 2,
        marginTop: -SIZE / 2,
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
        state === "neutral" ? "" : `, ${SPOKEN[state]}`
      }. ${node.maxPoints} point${node.maxPoints === 1 ? "" : "s"}.`}
      aria-pressed={state !== "neutral"}
    >
      {/* Three layers, one silhouette: keyline, state colour, artwork. The stylesheet
          explains why a ring cannot be a box-shadow here. */}
      <span className="ttm-node-ring" aria-hidden="true">
        <span className="ttm-node-band">
          <span className="ttm-node-face">
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
        </span>
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
