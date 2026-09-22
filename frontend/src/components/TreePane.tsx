import type { TalentNode as NodeData, TreeDetail } from "../lib/api";
import type { NodeState } from "./TalentNode";
import { TreeCanvas } from "./TreeCanvas";

/**
 * One tree in the multi-tree view, with its own heading and active state.
 *
 * A spec is three trees -- class, specialisation, hero -- and a player thinks about all of
 * them at once, so all of them are on screen at once. The solver still works on one tree at
 * a time, which is a property of the engine rather than of the product, so the one it is
 * pointed at is marked and the others stay visible as context. Context that is hidden is
 * not context.
 */

export interface TreePaneProps {
  tree: TreeDetail | null;
  title: string;
  subtitle?: string | null;
  active: boolean;
  onActivate: () => void;
  /** Only the active pane carries constraints; the rest render plain. */
  states?: Map<number, NodeState>;
  sides?: Map<number, "a" | "b" | "none">;
  stale?: boolean;
  build?: Record<string, number> | null;
  shares?: Map<number, number> | null;
  reachable?: Set<number> | null;
  editing?: boolean;
  /** Points spent by hand in this tree, shown beside the heading in build mode. */
  budget?: { spent: number; cap: number } | null;
  onNode?: (node: NodeData, alternate: boolean) => void;
  className?: string;
  /** Rendered under the heading; the hero pane puts its sub-tree picker here. */
  children?: React.ReactNode;
}

const EMPTY_STATES = new Map<number, NodeState>();
const EMPTY_SIDES = new Map<number, "a" | "b" | "none">();
const noop = () => {};

export function TreePane({
  tree,
  title,
  subtitle,
  active,
  onActivate,
  states,
  sides,
  stale,
  build,
  shares,
  reachable,
  editing,
  budget,
  onNode,
  className,
  children,
}: TreePaneProps) {
  return (
    <section
      className={`ttm-tree panel relative flex min-h-0 flex-col ${
        active ? "bracket" : ""
      } ${className ?? ""}`}
      data-active={active ? "yes" : "no"}
      aria-label={title}
    >
      <header className="flex shrink-0 items-baseline gap-2 px-3 pt-2 pb-1.5">
        {/* Clicking the heading points the solver here. The heading is the pane's name and
            its control at once, which keeps a second row of buttons off the screen. */}
        <button
          type="button"
          className="rail-item !px-0 !text-[11px]"
          aria-pressed={active}
          onClick={onActivate}
          title={active ? "The solver is pointed here" : `Point the solver at ${title}`}
        >
          <span className="label !text-[10px]" style={active ? { color: "inherit" } : undefined}>
            {title}
          </span>
        </button>
        {subtitle && (
          <span className="display truncate text-[15px] leading-none text-ink">
            {subtitle}
          </span>
        )}
        {budget && (
          <span className="num shrink-0 text-[12px] text-ink-soft">
            {budget.spent}
            <span className="text-ink-faint">/{budget.cap}</span>
          </span>
        )}
        {children}
      </header>

      <div className="relative min-h-0 flex-1">
        {tree ? (
          <TreeCanvas
            tree={tree}
            states={active ? (states ?? EMPTY_STATES) : EMPTY_STATES}
            sides={active ? (sides ?? EMPTY_SIDES) : EMPTY_SIDES}
            stale={active ? stale : false}
            build={build}
            shares={active ? shares : null}
            reachable={reachable}
            editing={editing}
            // In build mode every tree is editable, because a loadout spans all three.
            // In explore mode only the tree the solver is pointed at takes constraints, so a
            // click anywhere else re-points it instead.
            onActivate={onNode ?? (active ? noop : () => onActivate())}
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
