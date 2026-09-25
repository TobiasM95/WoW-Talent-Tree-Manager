import type { Tool } from "../lib/workspace";
import { ConstraintLegend } from "./Legend";

/**
 * What a click on an open tree does, and what each ring colour means.
 *
 * The tool and the legend are one panel because they are one question: "if I click here,
 * what happens and what will it look like". Split across two panels, a player had to connect
 * a button they pressed with a colour described somewhere else.
 */

export interface PaintToolsProps {
  tool: Tool;
  onTool: (tool: Tool) => void;
  /** Whether any tree is open; with none, there is nothing to paint. */
  anyOpen: boolean;
  onClear: () => void;
  clearable: boolean;
  /** How many of each constraint are painted, across the open trees. */
  counts: Record<string, number>;
}

const TOOLS: { tool: Tool; label: string; how: string }[] = [
  {
    tool: "toggle",
    label: "Require / bar",
    how:
      "Click a talent to require it, again to bar it. A multi-rank talent steps up a rank at a time first: at least 1, at least 2, … maxed. Choice nodes cycle left, right, barred.",
  },
  {
    tool: "atMost",
    label: "At most",
    how: "Click a multi-rank talent to cap its ranks, one fewer each click. With a minimum that makes exactly N, or a one-point dip. A one-rank talent is barred.",
  },
  {
    tool: "atLeastOne",
    label: "At least one",
    how: "Click talents to form a group. Every build takes one or more of them.",
  },
  {
    tool: "exactlyOne",
    label: "Exactly one",
    how: "Click talents to form a group. Every build takes exactly one of them.",
  },
];

export function PaintTools({ tool, onTool, anyOpen, onClear, clearable, counts }: PaintToolsProps) {
  const current = TOOLS.find((t) => t.tool === tool)!;
  return (
    <section className="panel p-3.5">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Paint</span>
        <button
          type="button"
          className="btn !px-2 !py-0.5 !text-[11px]"
          onClick={onClear}
          disabled={!clearable}
          title="Clear every constraint on every open tree"
        >
          Clear
        </button>
      </div>

      {anyOpen ? (
        <>
          {/* Two rows of two: what a talent must have, then groups across talents. */}
          <div className="mt-2 flex flex-col gap-1" role="group" aria-label="Paint tool">
            {[TOOLS.slice(0, 2), TOOLS.slice(2)].map((row, i) => (
              <div key={i} className="seg w-full">
                {row.map((t) => (
                  <button
                    key={t.tool}
                    type="button"
                    className="flex-1 whitespace-nowrap !px-1.5"
                    aria-pressed={tool === t.tool}
                    onClick={() => onTool(t.tool)}
                    title={t.how}
                  >
                    {t.label}
                  </button>
                ))}
              </div>
            ))}
          </div>
          <p className="mt-2 text-[11.5px] leading-snug text-ink-soft">
            {current.how} Right-click goes backwards.
          </p>
        </>
      ) : (
        <p className="mt-1.5 text-[11.5px] leading-snug text-ink-soft">
          Every tree is fixed. Switch one to <span className="text-ink">Open</span> to paint what
          you want from it.
        </p>
      )}

      {/* Every meaning, always -- not just the selected tool's. "At least one" and "exactly
          one" were described nowhere a player would read before clicking. */}
      <ConstraintLegend counts={counts} />
    </section>
  );
}
