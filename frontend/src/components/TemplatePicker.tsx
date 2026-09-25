import { useEffect, useMemo, useState } from "react";
import { ApiError, getTree, listTrees, type TreeSummary } from "../lib/api";
import {
  blankTree,
  copyTree,
  emptyDesign,
  fromTrees,
  LIMITS,
  type Design,
  type TreeRole,
  type TreeStyle,
} from "../lib/design";

/**
 * Where a design starts: blank, or a copy of real trees.
 *
 * Most custom trees are "what if Blizzard changed this", so copying is as near as starting
 * blank. The style follows the source, as the games do: retail's trees make a retail-style
 * project (a class tree, the specs chosen, and their hero trees, gated by barriers), Forever's
 * tabs a classic one.
 *
 * Used for a new project, and for adding one tree of a kind to a project -- where only trees
 * of the project's own style and that kind are offered, so a project never mixes the two.
 */

export type PickerTarget = { kind: "project" } | { kind: "tree"; style: TreeStyle; role?: TreeRole };

export interface TemplatePickerProps {
  target: PickerTarget;
  /** How many trees of the kind being added can still be taken. Ignored for a new project. */
  room: number;
  onPick: (design: Design) => void;
  onCancel: (() => void) | null;
}

type Source = "retail" | "forever";

const byOrder = (a: TreeSummary, b: TreeSummary) => (a.order ?? 0) - (b.order ?? 0) || a.name.localeCompare(b.name);

export function TemplatePicker({ target, room, onPick, onCancel }: TemplatePickerProps) {
  const project = target.kind === "project";
  const role = target.kind === "tree" ? target.role : undefined;
  // A tree added to a project comes from a game of the project's style.
  const fixed: Source | null = target.kind === "tree" ? (target.style === "retail" ? "retail" : "forever") : null;
  const [source, setSource] = useState<Source>(fixed ?? "retail");
  const [list, setList] = useState<TreeSummary[]>([]);
  const [className, setClassName] = useState("");
  const [chosen, setChosen] = useState<string[]>([]);
  const [withHeroes, setWithHeroes] = useState(true);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  // A new project takes a class's three tabs, or up to four of its specs.
  const cap = project ? (source === "forever" ? LIMITS.trees : LIMITS.specs) : room;

  useEffect(() => {
    setList([]);
    setError(null);
    void listTrees({ game: source })
      .then((trees) => {
        setList(trees);
        setClassName(trees[0]?.className ?? "");
      })
      .catch((exc: unknown) => setError(exc instanceof ApiError ? exc.detail : String(exc)));
  }, [source]);

  const classes = useMemo(() => [...new Set(list.map((t) => t.className))].sort(), [list]);

  /*
    What can be ticked. Forever: the class's tabs. Retail: its specs -- for a project, or a
    spec tree added to one -- or its hero trees, once each, since retail lists a hero tree
    under every spec that can take it.
  */
  const options = useMemo(() => {
    const mine = list.filter((t) => t.className === className);
    if (source === "forever") return mine.filter((t) => t.kind === "tab").sort(byOrder);
    if (role === "hero") {
      const seen = new Set<number | null>();
      return mine
        .filter((t) => t.kind === "hero" && !seen.has(t.subTreeId) && seen.add(t.subTreeId))
        .sort(byOrder);
    }
    return mine.filter((t) => t.kind === "spec").sort((a, b) => (a.specName ?? "").localeCompare(b.specName ?? ""));
  }, [list, className, source, role]);

  // A sensible first choice: a class's tabs, or its first spec.
  useEffect(() => {
    setChosen(options.slice(0, source === "forever" ? cap : 1).map((t) => t.key));
  }, [options, source, cap]);

  const toggle = (key: string) =>
    setChosen((c) => (c.includes(key) ? c.filter((k) => k !== key) : c.length < cap ? [...c, key] : c));

  const labelOf = (t: TreeSummary) => (t.kind === "spec" ? (t.specName ?? t.name) : t.name);

  const blank = (style: TreeStyle) =>
    onPick(
      project
        ? emptyDesign("New project", style)
        : { name: "", style, sharedPointCap: null, trees: [blankTree("", style, role)] },
    );

  const copy = async () => {
    setBusy(true);
    setError(null);
    try {
      const picked = options.filter((t) => chosen.includes(t.key));
      if (target.kind === "tree") {
        // Single trees into a project: their role is the one being added.
        const details = await Promise.all(picked.map((t) => getTree(t.key)));
        onPick({
          name: "",
          style: target.style,
          sharedPointCap: null,
          trees: details.map((d) => ({
            ...copyTree(d),
            ...(role ? { role } : {}),
            ...(role === "spec" ? { name: (d.specName ?? d.name).slice(0, 60) } : {}),
          })),
        });
        return;
      }
      if (source === "forever") {
        const details = await Promise.all(picked.map((t) => getTree(t.key)));
        onPick(fromTrees(`${className} (copy)`, details, details[0]?.sharedPointCap ?? null));
        return;
      }
      // Retail: the class tree of the first spec chosen, every spec chosen, and their heroes.
      const specs = picked.map((t) => t.specName);
      const keys = list
        .filter(
          (t) =>
            t.className === className &&
            specs.includes(t.specName) &&
            (t.kind === "spec" ||
              (t.kind === "class" && t.specName === specs[0]) ||
              (t.kind === "hero" && withHeroes)),
        )
        .map((t) => t.key);
      const details = await Promise.all(keys.map((k) => getTree(k)));
      details.sort((a, b) => specs.indexOf(a.specName) - specs.indexOf(b.specName));
      const name = specs.length === 1 ? `${specs[0]} ${className} (copy)` : `${className} (copy)`;
      onPick(fromTrees(name, details, null));
    } catch (exc) {
      setError(exc instanceof ApiError ? exc.detail : String(exc));
    } finally {
      setBusy(false);
    }
  };

  const what = role === "hero" ? "hero tree" : role === "spec" ? "spec tree" : project ? "project" : "tree";
  const unit = source === "forever" ? "tab" : role === "hero" ? "hero tree" : "spec";

  return (
    <section className="panel mx-auto my-4 w-full max-w-2xl p-4" aria-label="Start from">
      <div className="flex items-baseline justify-between gap-2">
        <h2 className="display text-[20px] leading-tight">{project ? "Start a new project" : `Add a ${what}`}</h2>
        {onCancel && (
          <button type="button" className="text-[12px] text-ink-faint underline" onClick={onCancel}>
            cancel
          </button>
        )}
      </div>

      <span className="label mt-3 block">Blank</span>
      {target.kind === "project" ? (
        <div className="mt-1.5 grid gap-2 sm:grid-cols-2">
          <button type="button" className="btn !h-auto flex-col !items-start !py-2 text-left" onClick={() => blank("retail")}>
            <span className="text-ink">Retail style</span>
            <span className="text-[11px] font-normal text-ink-faint">
              A class tree, up to {LIMITS.specs} specs and {LIMITS.heroes} hero trees, each with its own budget. Wide
              grids, free connections, barriers between rows.
            </span>
          </button>
          <button type="button" className="btn !h-auto flex-col !items-start !py-2 text-left" onClick={() => blank("classic")}>
            <span className="text-ink">Classic style</span>
            <span className="text-[11px] font-normal text-ink-faint">
              Up to {LIMITS.trees} tabs of four columns, each row opening 5 points after the last, optionally one shared
              pool — like WoW Forever.
            </span>
          </button>
        </div>
      ) : (
        <button type="button" className="btn mt-1.5" onClick={() => blank(target.style)}>
          Blank {what}
        </button>
      )}

      <span className="label mt-4 block">Copy {project ? "existing trees" : `an existing ${what}`}</span>
      <div className="mt-1.5 flex flex-wrap gap-2">
        {!fixed && (
          <div className="seg" role="group" aria-label="Template game">
            {([
              ["retail", "Retail"],
              ["forever", "WoW Forever"],
            ] as const).map(([id, label]) => (
              <button key={id} type="button" aria-pressed={source === id} onClick={() => setSource(id)}>
                {label}
              </button>
            ))}
          </div>
        )}
        <select
          className="field min-w-40"
          value={className}
          onChange={(event) => setClassName(event.target.value)}
          aria-label="Template class"
        >
          {classes.map((c) => (
            <option key={c} value={c}>
              {c}
            </option>
          ))}
        </select>
      </div>

      <ul className="mt-2 grid gap-x-3 gap-y-1 sm:grid-cols-2" aria-label="Template trees">
        {options.map((t) => (
          <li key={t.key}>
            <label className="flex items-center gap-1.5 text-[12px] text-ink-soft">
              <input
                type="checkbox"
                checked={chosen.includes(t.key)}
                disabled={!chosen.includes(t.key) && chosen.length >= cap}
                onChange={() => toggle(t.key)}
              />
              {labelOf(t)}
            </label>
          </li>
        ))}
      </ul>
      {project && source === "retail" && (
        <label className="mt-2 flex items-center gap-1.5 text-[12px] text-ink-soft">
          <input type="checkbox" checked={withHeroes} onChange={(event) => setWithHeroes(event.target.checked)} />
          with their hero trees
        </label>
      )}
      <p className="mt-1.5 text-[11px] text-ink-faint">
        Up to {cap} {unit}
        {cap === 1 ? "" : "s"}.{" "}
        {project && source === "retail"
          ? "The class tree comes with the first spec ticked; each hero tree is taken by the specs it had."
          : "Talents, icons, arrows, gates and budgets all come across."}
      </p>

      {error && (
        <p className="mt-2 text-[11.5px]" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      )}
      <button
        type="button"
        className="btn btn-primary mt-3"
        disabled={busy || chosen.length === 0}
        onClick={() => void copy()}
      >
        {busy ? "Copying…" : `Copy ${chosen.length} ${unit}${chosen.length === 1 ? "" : "s"} →`}
      </button>
    </section>
  );
}
