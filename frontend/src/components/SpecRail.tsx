import { useMemo } from "react";
import type { TreeSummary } from "../lib/api";
import { classColour } from "../lib/classes";

/**
 * Choosing what you are looking at: thirteen classes, then that class's specialisations.
 *
 * Two rows of names rather than two dropdowns. A dropdown hides the whole set behind a
 * click and tells you nothing about what else exists; there are only thirteen classes and
 * three or four specs, which is a number you can simply show. It also lets class colour do
 * its job -- the palette is already known to the player, and seeing it in the rail means
 * the rail is doing the same work as a label.
 */

export interface SpecRailProps {
  trees: TreeSummary[];
  className: string | null;
  specName: string | null;
  onSelect: (className: string, specName: string | null) => void;
}

export function SpecRail({ trees, className, specName, onSelect }: SpecRailProps) {
  const classes = useMemo(() => {
    const seen = new Map<string, string>();
    for (const tree of trees) if (!seen.has(tree.className)) seen.set(tree.className, tree.className);
    return [...seen.keys()].sort((a, b) => a.localeCompare(b));
  }, [trees]);

  const specs = useMemo(() => {
    const seen: string[] = [];
    for (const tree of trees) {
      if (tree.className !== className || tree.kind !== "spec" || !tree.specName) continue;
      if (!seen.includes(tree.specName)) seen.push(tree.specName);
    }
    return seen;
  }, [trees, className]);

  return (
    <div className="flex min-w-0 flex-col gap-0.5">
      <div
        className="flex flex-wrap items-center gap-x-0.5 gap-y-0.5"
        role="group"
        aria-label="Class"
      >
        {classes.map((name) => {
          const colour = classColour(name);
          return (
            <button
              key={name}
              type="button"
              className="rail-item"
              aria-pressed={name === className}
              onClick={() => onSelect(name, null)}
              // The tint is set per item so an unselected class still shows its colour on
              // hover, which is how someone finds the one they want without reading.
              style={
                {
                  "--class-glow": colour.glow,
                  "--class-ink": colour.ink,
                } as React.CSSProperties
              }
            >
              {name}
            </button>
          );
        })}
      </div>

      {specs.length > 0 && (
        <div
          className="flex flex-wrap items-center gap-x-0.5"
          role="group"
          aria-label="Specialisation"
        >
          {specs.map((name) => (
            <button
              key={name}
              type="button"
              className="rail-item !text-[13px]"
              aria-pressed={name === specName}
              onClick={() => onSelect(className!, name)}
            >
              {name}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
