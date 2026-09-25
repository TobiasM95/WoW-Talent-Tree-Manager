import { useEffect, useMemo, useState } from "react";
import { ApiError, getTree, listTrees, type TreeSummary } from "../lib/api";
import { blankTree, fromTrees, LIMITS, type Design, type DesignTree, type TreeStyle } from "../lib/design";

/**
 * Where a design starts: a blank tree of either style, or a copy of a real one.
 *
 * Most custom trees are "what if Blizzard changed this", so copying is as near as starting
 * blank: pick a game, a class, and which of its trees to bring. Retail gates come across as
 * barriers and Forever's as points per row, so the copy edits the way the original works.
 *
 * Used twice: for a new project (up to three trees) and for adding a tree to one (as many as
 * the project still has room for).
 */

export interface TemplatePickerProps {
  /** How many trees can still be taken. */
  room: number;
  /** What is being made, for the heading. */
  purpose: "project" | "tree";
  onPick: (design: Design) => void;
  onCancel: (() => void) | null;
}

type Source = "retail" | "forever";

const kindOrder: Record<string, number> = { class: 0, spec: 1, hero: 2, tab: 0 };

const labelOf = (t: TreeSummary) =>
  t.kind === "class" ? "Class tree" : t.kind === "spec" ? `${t.specName} spec tree` : t.kind === "hero" ? `${t.name} (hero)` : t.name;

export function TemplatePicker({ room, purpose, onPick, onCancel }: TemplatePickerProps) {
  const [source, setSource] = useState<Source>("retail");
  const [list, setList] = useState<TreeSummary[]>([]);
  const [className, setClassName] = useState("");
  // Retail's class and hero trees differ by spec, so a spec is chosen too.
  const [specName, setSpecName] = useState("");
  const [chosen, setChosen] = useState<string[]>([]);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

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
  const specs = useMemo(
    () => [...new Set(list.filter((t) => t.className === className && t.specName).map((t) => t.specName!))].sort(),
    [list, className],
  );
  useEffect(() => setSpecName(specs[0] ?? ""), [specs]);
  const ofClass = useMemo(
    () =>
      list
        .filter((t) => t.className === className && (source === "forever" || t.specName === specName))
        .sort((a, b) => kindOrder[a.kind]! - kindOrder[b.kind]! || (a.order ?? 0) - (b.order ?? 0) || labelOf(a).localeCompare(labelOf(b))),
    [list, className, specName, source],
  );

  // A sensible first choice: a class's three Forever tabs, or retail's class tree and first spec.
  useEffect(() => {
    const first =
      source === "forever"
        ? ofClass.map((t) => t.key)
        : [ofClass.find((t) => t.kind === "class")?.key, ofClass.find((t) => t.kind === "spec")?.key].filter(
            (k): k is string => Boolean(k),
          );
    setChosen(first.slice(0, room));
  }, [ofClass, source, room]);

  const toggle = (key: string) =>
    setChosen((c) => (c.includes(key) ? c.filter((k) => k !== key) : c.length < room ? [...c, key] : c));

  const blank = (style: TreeStyle) =>
    onPick({ name: "New project", sharedPointCap: null, trees: [blankTree("Tree 1", style)] });

  const copy = async () => {
    setBusy(true);
    setError(null);
    try {
      const details = await Promise.all(chosen.map((k) => getTree(k)));
      const pool = source === "forever" ? (details[0]?.sharedPointCap ?? null) : null;
      const name = source === "forever" ? className : `${specName} ${className}`;
      onPick(fromTrees(`${name} (copy)`, details, pool));
    } catch (exc) {
      setError(exc instanceof ApiError ? exc.detail : String(exc));
    } finally {
      setBusy(false);
    }
  };

  return (
    <section className="panel mx-auto my-4 w-full max-w-2xl p-4" aria-label="Start from">
      <div className="flex items-baseline justify-between gap-2">
        <h2 className="display text-[20px] leading-tight">
          {purpose === "project" ? "Start a new project" : "Add a tree"}
        </h2>
        {onCancel && (
          <button type="button" className="text-[12px] text-ink-faint underline" onClick={onCancel}>
            cancel
          </button>
        )}
      </div>

      <span className="label mt-3 block">Blank</span>
      <div className="mt-1.5 grid gap-2 sm:grid-cols-2">
        <button type="button" className="btn !h-auto flex-col !items-start !py-2 text-left" onClick={() => blank("retail")}>
          <span className="text-ink">Retail style</span>
          <span className="text-[11px] font-normal text-ink-faint">
            Wide grid, free connections, barriers between rows (8 and 20 to start).
          </span>
        </button>
        <button type="button" className="btn !h-auto flex-col !items-start !py-2 text-left" onClick={() => blank("classic")}>
          <span className="text-ink">Classic style</span>
          <span className="text-[11px] font-normal text-ink-faint">
            Four columns, each row opening 5 points after the last — like WoW Forever.
          </span>
        </button>
      </div>

      <span className="label mt-4 block">Copy an existing tree</span>
      <div className="mt-1.5 flex flex-wrap gap-2">
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
        {source === "retail" && (
          <select
            className="field min-w-32"
            value={specName}
            onChange={(event) => setSpecName(event.target.value)}
            aria-label="Template spec"
          >
            {specs.map((s) => (
              <option key={s} value={s}>
                {s}
              </option>
            ))}
          </select>
        )}
      </div>

      <ul className="mt-2 grid gap-x-3 gap-y-1 sm:grid-cols-2" aria-label="Template trees">
        {ofClass.map((t) => (
          <li key={t.key}>
            <label className="flex items-center gap-1.5 text-[12px] text-ink-soft">
              <input
                type="checkbox"
                checked={chosen.includes(t.key)}
                disabled={!chosen.includes(t.key) && chosen.length >= room}
                onChange={() => toggle(t.key)}
              />
              {labelOf(t)}
              <span className="num text-[10.5px] text-ink-faint">{t.nodeCount}</span>
            </label>
          </li>
        ))}
      </ul>
      <p className="mt-1.5 text-[11px] text-ink-faint">
        Up to {room} tree{room === 1 ? "" : "s"}. Talents, icons, arrows, gates and budgets all come across.
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
        {busy ? "Copying…" : `Copy ${chosen.length} tree${chosen.length === 1 ? "" : "s"} →`}
      </button>
    </section>
  );
}

/** Add a picked design's trees to a project, renumbering talents so ids stay unique within it. */
export function appendTrees(design: Design, extra: DesignTree[]): Design {
  let next = 1 + Math.max(0, ...design.trees.flatMap((t) => t.nodes.map((n) => n.nodeId)));
  const trees = extra.slice(0, LIMITS.trees - design.trees.length).map((t) => {
    const ids = new Map(t.nodes.map((n) => [n.nodeId, next++]));
    return {
      ...t,
      nodes: t.nodes.map((n) => ({ ...n, nodeId: ids.get(n.nodeId)!, parents: n.parents.map((p) => ids.get(p)!) })),
    };
  });
  return { ...design, trees: [...design.trees, ...trees] };
}
