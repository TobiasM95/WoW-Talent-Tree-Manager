import type { TalentNode as NodeData, TreeDetail } from "../lib/api";
import type { TreeMode } from "../lib/workspace";
import { formatCount } from "../lib/space";
import type { NodeState } from "./TalentNode";
import { TreeCanvas } from "./TreeCanvas";

/**
 * One tree, with everything about it in its own header.
 *
 * Each tree is independently **fixed** (you spend its points, it contributes one build) or
 * **open** (you paint what you want, it contributes every build that matches). The switch,
 * the budget and the count live on the tree they describe rather than in a sidebar, because
 * "which tree am I changing" was the question the old layout made hardest to answer.
 */

export interface TreePaneProps {
  tree: TreeDetail | null;
  title: string;
  subtitle?: string | null;
  /** Absent on read-only panes, such as the analysis page. */
  mode?: TreeMode;
  onMode?: (mode: TreeMode) => void;
  /**
   * Points: spent of cap when fixed, budget of cap when open. `shared` drops the cap from a
   * fixed tree's readout: when trees draw on one pool, a tree's "cap" is only what the others
   * happen to leave, and "47/47 spent" says nothing a player can use.
   */
  points?: { value: number; cap: number; shared?: boolean };
  onBudget?: (points: number) => void;
  /** Builds this tree contributes; null while counting. */
  count?: { builds: number | null; stale: boolean; error: string | null };
  states?: Map<number, NodeState>;
  sides?: Map<number, "a" | "b" | "none">;
  build?: Record<string, number> | null;
  impacts?: Map<number, number> | null;
  reachable?: Set<number> | null;
  editing?: boolean;
  onNode?: (node: NodeData, alternate: boolean) => void;
  className?: string;
  /** Rendered at the end of the heading; the hero pane puts its tree picker here. */
  children?: React.ReactNode;
}

const EMPTY_STATES = new Map<number, NodeState>();
const EMPTY_SIDES = new Map<number, "a" | "b" | "none">();
const noop = () => {};

export function TreePane({
  tree,
  title,
  subtitle,
  mode,
  onMode,
  points,
  onBudget,
  count,
  states,
  sides,
  build,
  impacts,
  reachable,
  editing,
  onNode,
  className,
  children,
}: TreePaneProps) {
  return (
    <section
      className={`ttm-tree panel relative flex min-h-0 flex-col ${className ?? ""}`}
      data-mode={mode}
      aria-label={title}
    >
      <header className="flex shrink-0 flex-col gap-1.5 px-3 pt-2 pb-1.5">
        <div className="flex min-w-0 items-baseline gap-2">
          <span className="label !text-[10px]">{title}</span>
          {subtitle && (
            <span className="display truncate text-[15px] leading-none text-ink">{subtitle}</span>
          )}
          {children}
        </div>

        {mode && (
          <div className="flex items-center gap-2">
            {/* Fixed or open, as a segmented switch on the tree itself. Words rather than
                icons, because the distinction is the whole model and has to be read. */}
            <div className="seg" role="group" aria-label={`${title} tree mode`}>
              {(["fixed", "open"] as const).map((m) => (
                <button
                  key={m}
                  type="button"
                  aria-pressed={mode === m}
                  onClick={() => onMode?.(m)}
                  title={
                    m === "fixed"
                      ? "Spend points by hand: this tree contributes one build"
                      : "Paint what you want: this tree contributes every build that matches"
                  }
                >
                  {m === "fixed" ? "Fixed" : "Open"}
                </button>
              ))}
            </div>

            {points && mode === "open" && onBudget && (
              <label className="flex items-center gap-1 text-[11px] text-ink-soft" title="Points to spend in this tree">
                <button
                  type="button"
                  className="step"
                  onClick={() => onBudget(Math.max(1, points.value - 1))}
                  disabled={points.value <= 1}
                  aria-label={`Spend one point fewer in ${title}`}
                >
                  −
                </button>
                <span className="num min-w-[3.2rem] text-center text-ink">
                  {points.value}
                  <span className="text-ink-faint">/{points.cap}</span>
                </span>
                <button
                  type="button"
                  className="step"
                  onClick={() => onBudget(Math.min(points.cap, points.value + 1))}
                  disabled={points.value >= points.cap}
                  aria-label={`Spend one point more in ${title}`}
                >
                  +
                </button>
              </label>
            )}

            {points && mode === "fixed" && (
              <span
                className="num text-[11.5px]"
                style={{
                  color: !points.shared && points.value < points.cap ? "var(--any-of)" : "var(--ink-soft)",
                }}
                title={points.shared ? undefined : points.value < points.cap ? "Points left unspent" : "Every point spent"}
              >
                {points.value}
                <span className="text-ink-faint">{points.shared ? " spent" : `/${points.cap} spent`}</span>
              </span>
            )}

            {count && (
              <span
                className="num ml-auto shrink-0 text-[12px]"
                data-count-for={title}
                style={{
                  color: count.error ? "var(--barred)" : "var(--ink)",
                  opacity: count.stale ? 0.55 : 1,
                }}
                title={count.error ?? "Builds this tree contributes, choice sides included"}
              >
                {count.error
                  ? "error"
                  : count.builds === null
                    ? "…"
                    : `${formatCount(count.builds)} build${count.builds === 1 ? "" : "s"}`}
              </span>
            )}
          </div>
        )}
      </header>

      <div className="relative min-h-0 flex-1">
        {tree ? (
          <TreeCanvas
            tree={tree}
            states={states ?? EMPTY_STATES}
            sides={sides ?? EMPTY_SIDES}
            stale={count?.stale}
            build={build}
            impacts={impacts}
            reachable={reachable}
            editing={editing}
            onActivate={onNode ?? noop}
          />
        ) : (
          <div className="absolute inset-0 flex items-center justify-center text-[12px] text-ink-faint">
            —
          </div>
        )}
      </div>
    </section>
  );
}
