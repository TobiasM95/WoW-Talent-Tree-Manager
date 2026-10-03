import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { iconUrl, searchIcons } from "../lib/api";
import {
  addNode,
  deleteNode,
  gateOf,
  insertTrees,
  LIMITS,
  moveNode,
  problems,
  removeTree,
  renameNode,
  roomFor,
  rowGate,
  setBarrier,
  setKind,
  styleOf,
  toggleEdge,
  updateNode,
  type Design,
  type DesignEntry,
  type DesignNode,
  type DesignTree,
  type TreeRole,
} from "../lib/design";
import { TemplatePicker } from "./TemplatePicker";

const ROLE_LABEL: Record<TreeRole, string> = { class: "class", spec: "spec", hero: "hero" };

/**
 * The tree editor: design talent trees of your own, then plan and count them like any other.
 *
 * A grid you build on directly. Click an empty cell for a talent, drag it to move it, and join
 * two with the Connect tool (or shift-click) to make one require the other. What each talent
 * is -- passive, ability, a choice of two -- is its shape, exactly as on the planner, so a
 * design reads the way it will play.
 *
 * Edits that could never be saved are refused as they happen rather than reported later: a
 * requirement that would loop is not drawn, a talent cannot land on an occupied cell. The
 * server validates again on save; this is so a person is not surprised by it.
 */

export interface EditorViewProps {
  draft: Design;
  onChange: (next: Design) => void;
  onSave: () => Promise<{ ok: boolean; message: string; warnings?: string[] }>;
  onPlan: (() => void) | null;
  dirty: boolean;
  saved: boolean;
}

type Tool = "select" | "connect";

const CELL_W = 84;
const CELL_H = 92;
const TILE = 52;

const SHAPE: Record<string, string> = {
  passive: "circle(50%)",
  active: "inset(0 round 7px)",
  choice: "polygon(50% 0%, 93.3% 25%, 93.3% 75%, 50% 100%, 6.7% 75%, 6.7% 25%)",
};
const shapeOf = (n: DesignNode) => (n.kind === "choice" ? "choice" : n.entries[0]?.kind === "active" ? "active" : "passive");

export function EditorView({ draft, onChange, onSave, onPlan, dirty, saved }: EditorViewProps) {
  const [at, setAt] = useState(0);
  const [selected, setSelected] = useState<number | null>(null);
  const [tool, setTool] = useState<Tool>("select");
  const [source, setSource] = useState<number | null>(null);
  const [message, setMessage] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);
  // Choosing what an added tree starts from: a classic tab, or a retail spec or hero tree.
  const [adding, setAdding] = useState<TreeRole | "tab" | null>(null);
  const [result, setResult] = useState<{ ok: boolean; message: string; warnings?: string[] } | null>(null);
  const past = useRef<Design[]>([]);
  const future = useRef<Design[]>([]);

  const tree = draft.trees[Math.min(at, draft.trees.length - 1)]!;
  const node = tree.nodes.find((n) => n.nodeId === selected) ?? null;
  const found = useMemo(() => problems(draft), [draft]);

  // One place every edit goes through, so every edit can be undone.
  const commit = useCallback(
    (next: Design | null, refusal?: string) => {
      if (!next) {
        if (refusal) setMessage(refusal);
        return;
      }
      past.current = [...past.current.slice(-99), draft];
      future.current = [];
      setMessage(null);
      setResult(null);
      onChange(next);
    },
    [draft, onChange],
  );

  const undo = useCallback(() => {
    const previous = past.current.pop();
    if (!previous) return;
    future.current = [draft, ...future.current];
    onChange(previous);
  }, [draft, onChange]);

  const redo = useCallback(() => {
    const next = future.current.shift();
    if (!next) return;
    past.current = [...past.current, draft];
    onChange(next);
  }, [draft, onChange]);

  // Keys that act on the design, but never while a person is typing into a field.
  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      const typing = (event.target as HTMLElement)?.closest("input, textarea, select");
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "z") {
        if (typing) return;
        event.preventDefault();
        if (event.shiftKey) redo();
        else undo();
      } else if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "y") {
        if (typing) return;
        event.preventDefault();
        redo();
      } else if ((event.key === "Delete" || event.key === "Backspace") && selected !== null && !typing) {
        event.preventDefault();
        commit(deleteNode(draft, at, selected));
        setSelected(null);
      } else if (event.key === "Escape") {
        setSource(null);
        setSelected(null);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [undo, redo, commit, draft, at, selected]);

  // Retail trees are wide and gated by barriers; classic ones are four columns gated by row.
  const retail = styleOf(draft) === "retail";
  const specNames = draft.trees.filter((t) => t.role === "spec").map((t) => t.name);
  const rows = Math.min(LIMITS.rows + 1, Math.max(retail ? 10 : 8, ...tree.nodes.map((n) => n.row + 3)));
  const cols = Math.min(LIMITS.cols + 1, Math.max(retail ? 9 : 4, ...tree.nodes.map((n) => n.col + 2)));
  const cellOf = (row: number, col: number) => tree.nodes.find((n) => n.row === row && n.col === col);

  const connect = (parent: number, child: number) => {
    const next = toggleEdge(draft, at, parent, child);
    commit(next, "That would make a talent require itself, so it was not drawn.");
  };

  const onNodeClick = (n: DesignNode, shift: boolean) => {
    if (tool === "connect" || shift) {
      const from = tool === "connect" ? source : selected;
      if (from === null || from === n.nodeId) {
        setSource(n.nodeId);
        setSelected(n.nodeId);
        setMessage(tool === "connect" ? `Now click the talent that requires ${n.name}.` : null);
        return;
      }
      connect(from, n.nodeId);
      setSource(tool === "connect" ? null : source);
      setSelected(n.nodeId);
      return;
    }
    setSelected(n.nodeId);
  };

  const save = async () => {
    setSaving(true);
    try {
      setResult(await onSave());
    } finally {
      setSaving(false);
    }
  };

  const setTree = (change: (t: DesignTree) => DesignTree) =>
    commit({ ...draft, trees: draft.trees.map((t, i) => (i === at ? change(t) : t)) });

  const BARRIER_RULE = "Each barrier must ask for more points than the one above it.";
  const barrierAt = (row: number, points: number) => {
    const next = setBarrier(tree, row, points);
    commit(next ? { ...draft, trees: draft.trees.map((t, i) => (i === at ? next : t)) } : null, BARRIER_RULE);
  };
  // A new barrier asks a little more than what the row already needs, if the one below allows.
  const addBarrier = (row: number) => {
    const here = rowGate(tree, row);
    const below = (tree.barriers ?? []).find((b) => b.row > row)?.points ?? Infinity;
    const points = [here + 5, here + 1].find((v) => v < below);
    if (points === undefined) setMessage(BARRIER_RULE);
    else barrierAt(row, points);
  };
  const removable = retail
    ? tree.role === "hero" || (tree.role === "spec" && specNames.length > 1)
    : draft.trees.length > 1;
  const adders: [TreeRole | "tab", string][] = retail
    ? [
        ["spec", "+ spec"],
        ["hero", "+ hero"],
      ]
    : [["tab", "+ tree"]];

  return (
    <div className="flex min-h-0 flex-1 flex-col gap-2 p-2 md:p-2.5">
      {/* The project: name, pool, trees, and the two things to do with it. */}
      <section className="panel flex flex-wrap items-center gap-x-4 gap-y-2 px-3.5 py-2.5">
        <label className="flex items-center gap-2">
          <span className="label">Project</span>
          <input
            className="field w-56"
            value={draft.name}
            maxLength={60}
            onChange={(event) => onChange({ ...draft, name: event.target.value })}
            onBlur={() => commit({ ...draft })}
            aria-label="Project name"
          />
        </label>

        <span className="chip" data-style={retail ? "retail" : "classic"} title={retail
          ? "A class tree, one to four specs and their hero trees, each with its own budget, gated by barriers"
          : "One to three tabs gated by row, like WoW Forever"}>
          {retail ? "Retail style" : "Classic style"}
        </span>

        {!retail && (
        <label className="flex items-center gap-1.5 text-[12px] text-ink-soft" title="All trees draw on one pool, like WoW Forever's 51">
          <input
            type="checkbox"
            checked={draft.sharedPointCap !== null}
            onChange={(event) => commit({ ...draft, sharedPointCap: event.target.checked ? 51 : null })}
          />
          one shared point pool
          {draft.sharedPointCap !== null && (
            <input
              className="field num w-16"
              type="number"
              min={1}
              max={300}
              value={draft.sharedPointCap}
              onChange={(event) => commit({ ...draft, sharedPointCap: Math.max(1, Math.min(300, Number(event.target.value) || 1)) })}
              aria-label="Shared pool size"
            />
          )}
        </label>
        )}

        <div className="seg" role="tablist" aria-label="Trees">
          {draft.trees.map((t, i) => (
            <button
              key={i}
              type="button"
              role="tab"
              aria-pressed={i === at}
              aria-selected={i === at}
              onClick={() => {
                setAt(i);
                setSelected(null);
                setSource(null);
              }}
            >
              {t.role && t.role !== "class" && (
                <span className="mr-1 text-[9.5px] uppercase tracking-wider text-ink-faint">{ROLE_LABEL[t.role]}</span>
              )}
              {t.name || `Tree ${i + 1}`}
            </button>
          ))}
          {adders.map(([role, label]) =>
            roomFor(draft, role === "tab" ? undefined : role) > 0 ? (
              <button
                key={role}
                type="button"
                aria-pressed={adding === role}
                onClick={() => {
                  setAdding(role);
                  setSelected(null);
                }}
                title={role === "tab" ? "Add a tree to the project" : `Add a ${role} tree to the project`}
              >
                {label}
              </button>
            ) : null,
          )}
        </div>

        <div className="ml-auto flex items-center gap-1.5">
          <button type="button" className="btn !px-2" onClick={undo} disabled={!past.current.length} title="Undo (Ctrl+Z)">
            ↶
          </button>
          <button type="button" className="btn !px-2" onClick={redo} disabled={!future.current.length} title="Redo (Ctrl+Shift+Z)">
            ↷
          </button>
          <span className="text-[11px] text-ink-faint" data-save-state={dirty ? "dirty" : saved ? "saved" : "new"}>
            {dirty ? "unsaved changes" : saved ? "saved" : "not saved yet"}
          </span>
          <button
            type="button"
            className="btn"
            onClick={() => void save()}
            disabled={saving || found.errors.length > 0 || (!dirty && saved)}
            title={found.errors[0] ?? "Save this version; the planner uses the last saved one"}
          >
            {saving ? "Saving…" : "Save"}
          </button>
          <button type="button" className="btn btn-primary !w-auto" onClick={() => onPlan?.()} disabled={!onPlan}>
            Plan with it →
          </button>
        </div>
      </section>

      {adding ? (
        <div className="min-h-0 flex-1 overflow-auto">
          <TemplatePicker
            target={{ kind: "tree", style: retail ? "retail" : "classic", role: adding === "tab" ? undefined : adding }}
            room={roomFor(draft, adding === "tab" ? undefined : adding)}
            onPick={(picked) => {
              // A blank tree is named for its place: the next spec, hero tree or tab.
              const blank = picked.trees.length === 1 && picked.trees[0]!.nodes.length === 0;
              const same = draft.trees.filter((t) => (t.role ?? "tab") === adding).length;
              const named = adding === "spec" ? `Spec ${same + 1}` : adding === "hero" ? `Hero ${same + 1}` : `Tree ${same + 1}`;
              const placed = insertTrees(draft, blank ? [{ ...picked.trees[0]!, name: named }] : picked.trees);
              commit(placed.design);
              setAt(placed.at);
              setAdding(null);
            }}
            onCancel={() => setAdding(null)}
          />
        </div>
      ) : (
      <div className="flex min-h-0 flex-1 flex-col gap-2 md:flex-row md:gap-2.5">
        {/* The grid. */}
        <section className="panel relative min-h-[26rem] flex-1 overflow-auto" aria-label="Tree grid">
          <div className="sticky top-0 z-10 flex items-center gap-2 px-3 py-2" style={{ background: "var(--panel)" }}>
            <div className="seg" role="group" aria-label="Editing tool">
              {([
                ["select", "Select / move"],
                ["connect", "Connect"],
              ] as const).map(([id, label]) => (
                <button
                  key={id}
                  type="button"
                  aria-pressed={tool === id}
                  onClick={() => {
                    setTool(id);
                    setSource(null);
                    setMessage(id === "connect" ? "Click the talent that is required, then the one that requires it." : null);
                  }}
                >
                  {label}
                </button>
              ))}
            </div>
            <span className="truncate text-[11.5px]" style={{ color: message ? "var(--any-of)" : "var(--ink-faint)" }}>
              {message ?? "Click an empty cell to add a talent. Drag to move. Shift-click another talent to connect."}
            </span>
          </div>

          <div className="relative m-3" style={{ width: cols * CELL_W + 64, height: rows * CELL_H }}>
            {/* Row gates, when the tree gates by row. */}
            {!retail &&
              Array.from({ length: rows }, (_, r) => (
                <span
                  key={r}
                  className="num absolute left-0 w-8 text-right text-[10px] text-ink-faint"
                  style={{ top: r * CELL_H + TILE / 2 - 6 }}
                  title={`Row ${r + 1} opens at ${r * tree.pointsPerRow!} points in this tree`}
                >
                  {r * tree.pointsPerRow!}
                </span>
              ))}

            {/* Barriers, when the tree gates retail's way: a line between two rows, crossed once
                enough is spent. Set in the margin: + adds one, the number edits it, x removes it. */}
            {retail &&
              Array.from({ length: rows - 1 }, (_, i) => {
                const row = i + 1;
                const barrier = (tree.barriers ?? []).find((b) => b.row === row);
                const y = row * CELL_H - (CELL_H - TILE) / 2 + 4;
                return barrier ? (
                  <div key={row} data-barrier={row}>
                    <div
                      className="editor-barrier pointer-events-none absolute left-10"
                      style={{ top: y, width: cols * CELL_W }}
                    />
                    <input
                      className="editor-barrier-points num absolute left-0"
                      style={{ top: y - 10 }}
                      inputMode="numeric"
                      value={barrier.points}
                      onChange={(event) => {
                        const v = Number(event.target.value.replace(/\D/g, ""));
                        if (v > 0) barrierAt(row, Math.min(300, v));
                      }}
                      aria-label={`Points to pass the barrier above row ${row + 1}`}
                      title={`Row ${row + 1} and below open at ${barrier.points} points spent in this tree`}
                    />
                    <button
                      type="button"
                      className="editor-barrier-remove absolute"
                      style={{ top: y - 8, left: cols * CELL_W + 42 }}
                      onClick={() => barrierAt(row, 0)}
                      aria-label={`Remove the barrier above row ${row + 1}`}
                    >
                      ×
                    </button>
                  </div>
                ) : (
                  <button
                    key={row}
                    type="button"
                    className="editor-barrier-add absolute left-2"
                    style={{ top: y - 8 }}
                    onClick={() => addBarrier(row)}
                    aria-label={`Add a barrier above row ${row + 1}`}
                    title={`Add a barrier above row ${row + 1}`}
                  >
                    +
                  </button>
                );
              })}

            <svg className="pointer-events-none absolute left-10 top-0" width={cols * CELL_W} height={rows * CELL_H}>
              {tree.nodes.flatMap((child) =>
                child.parents.map((pid) => {
                  const parent = tree.nodes.find((n) => n.nodeId === pid);
                  if (!parent) return null;
                  const x1 = parent.col * CELL_W + CELL_W / 2;
                  const y1 = parent.row * CELL_H + TILE / 2 + 4;
                  const x2 = child.col * CELL_W + CELL_W / 2;
                  const y2 = child.row * CELL_H + TILE / 2 + 4;
                  return (
                    <line
                      key={`${pid}-${child.nodeId}`}
                      x1={x1}
                      y1={y1}
                      x2={x2}
                      y2={y2}
                      stroke="var(--brass)"
                      strokeWidth={2}
                      markerEnd="url(#arrow)"
                      opacity={0.8}
                    />
                  );
                }),
              )}
              <defs>
                <marker id="arrow" viewBox="0 0 10 10" refX="16" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
                  <path d="M 0 0 L 10 5 L 0 10 z" fill="var(--brass)" />
                </marker>
              </defs>
            </svg>

            <div className="absolute left-10 top-0" style={{ width: cols * CELL_W, height: rows * CELL_H }}>
              {Array.from({ length: rows }, (_, row) =>
                Array.from({ length: cols }, (_, col) => {
                  const here = cellOf(row, col);
                  const x = col * CELL_W;
                  const y = row * CELL_H;
                  if (!here) {
                    return (
                      <button
                        key={`${row}-${col}`}
                        type="button"
                        className="editor-cell absolute"
                        style={{ left: x + (CELL_W - TILE) / 2, top: y + 4, width: TILE, height: TILE }}
                        aria-label={`Add a talent at row ${row + 1}, column ${col + 1}`}
                        data-cell={`${row},${col}`}
                        onClick={() => {
                          if (tree.nodes.length >= LIMITS.nodes) {
                            setMessage(`A tree holds at most ${LIMITS.nodes} talents.`);
                            return;
                          }
                          const made = addNode(draft, at, row, col);
                          commit(made.design);
                          setSelected(made.nodeId);
                        }}
                        onDragOver={(event) => event.preventDefault()}
                        onDrop={(event) => {
                          event.preventDefault();
                          const id = Number(event.dataTransfer.getData("text/plain"));
                          if (id) commit(moveNode(draft, at, id, row, col), "That cell is taken.");
                        }}
                      />
                    );
                  }
                  const shape = shapeOf(here);
                  const icon = iconUrl(here.entries[0]?.icon ?? null, 56);
                  const isSource = here.nodeId === source;
                  return (
                    <div
                      key={here.nodeId}
                      className="absolute flex flex-col items-center"
                      style={{ left: x, top: y + 4, width: CELL_W }}
                    >
                      <button
                        type="button"
                        draggable
                        onDragStart={(event) => event.dataTransfer.setData("text/plain", String(here.nodeId))}
                        onClick={(event) => onNodeClick(here, event.shiftKey)}
                        className="editor-node relative"
                        data-node={here.nodeId}
                        data-selected={here.nodeId === selected ? "" : undefined}
                        data-source={isSource ? "" : undefined}
                        data-granted={here.granted ? "" : undefined}
                        aria-label={`${here.name}, ${shape}, ${here.maxPoints} rank${here.maxPoints === 1 ? "" : "s"}`}
                        style={{ width: TILE, height: TILE }}
                      >
                        <span className="editor-node-ring" style={{ clipPath: SHAPE[shape] }}>
                          <span
                            className="editor-node-face"
                            style={{
                              clipPath: SHAPE[shape],
                              backgroundImage: icon ? `url(${icon})` : undefined,
                            }}
                          >
                            {!icon && <span className="text-[16px] text-ink-faint">{here.name.slice(0, 1)}</span>}
                          </span>
                        </span>
                        {here.maxPoints > 1 && <span className="ttm-node-ranks">{here.maxPoints}</span>}
                      </button>
                      <span className="mt-0.5 max-w-full truncate px-1 text-[10.5px] text-ink-soft" title={here.name}>
                        {here.name}
                      </span>
                    </div>
                  );
                }),
              )}
            </div>
          </div>
        </section>

        {/* The inspector. */}
        <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[22rem] md:overflow-y-auto md:pr-1">
          {node ? (
            <NodeInspector
              key={node.nodeId}
              tree={tree}
              node={node}
              onChange={(change) => commit(updateNode(draft, at, node.nodeId, change))}
              onDelete={() => {
                commit(deleteNode(draft, at, node.nodeId));
                setSelected(null);
              }}
              onRemoveParent={(pid) => connect(pid, node.nodeId)}
            />
          ) : (
            <section className="panel p-3.5 text-[12px] text-ink-soft">
              Select a talent to edit it, or click an empty cell to add one.
            </section>
          )}

          <section className="panel p-3.5">
            <span className="label">This tree</span>
            <div className="mt-2 grid grid-cols-[7rem_1fr] items-center gap-x-2 gap-y-2 text-[12px]">
              <span className="text-ink-soft">Name</span>
              <input
                className="field"
                value={tree.name}
                maxLength={60}
                onChange={(event) => setTree((t) => ({ ...t, name: event.target.value }))}
                aria-label="Tree name"
              />
              <span className="text-ink-soft" title="Points that can be spent in this tree">Point budget</span>
              <input
                className="field num"
                type="number"
                min={1}
                placeholder={`all ranks (${tree.nodes.reduce((a, n) => a + n.maxPoints, 0)})`}
                value={tree.pointCap ?? ""}
                onChange={(event) =>
                  setTree((t) => ({ ...t, pointCap: event.target.value === "" ? null : Math.max(1, Number(event.target.value)) }))
                }
                aria-label="Point budget"
              />
              {tree.role === "hero" && (
                <>
                  <span className="text-ink-soft" title="The specs that may take this hero tree">Taken by</span>
                  <div className="flex flex-wrap gap-x-2.5 gap-y-1" role="group" aria-label="Specs that take this hero tree">
                    {specNames.map((name, i) => (
                      <label key={i} className="flex items-center gap-1 text-ink-soft">
                        <input
                          type="checkbox"
                          checked={(tree.specs ?? []).includes(i)}
                          onChange={(event) =>
                            setTree((t) => ({
                              ...t,
                              specs: event.target.checked
                                ? [...(t.specs ?? []), i].sort((a, b) => a - b)
                                : (t.specs ?? []).filter((s) => s !== i),
                            }))
                          }
                        />
                        {name || `Spec ${i + 1}`}
                      </label>
                    ))}
                  </div>
                </>
              )}
              {!retail && (
                <>
                  <span className="text-ink-soft" title="Points spent in the rows above that open each row">Points per row</span>
                  <input
                    className="field num"
                    type="number"
                    min={0}
                    max={50}
                    value={tree.pointsPerRow ?? 0}
                    onChange={(event) =>
                      setTree((t) => ({ ...t, pointsPerRow: Math.max(0, Math.min(50, Number(event.target.value) || 0)) }))
                    }
                    aria-label="Points per row"
                  />
                </>
              )}
            </div>
            {retail && (
              <p className="mt-2 text-[11px] leading-snug text-ink-faint">
                Barriers sit between rows, like retail&apos;s 8 and 20: + in the grid&apos;s margin adds one, its number
                sets the points, × removes it. A talent can still override its own gate.
              </p>
            )}
            {removable && (
              <button
                type="button"
                className="btn mt-3 w-full"
                onClick={() => {
                  commit(removeTree(draft, at));
                  setAt(0);
                  setSelected(null);
                }}
              >
                Remove this tree
              </button>
            )}
          </section>

          {(found.errors.length > 0 || found.notes.length > 0 || result) && (
            <section className="panel p-3.5 text-[11.5px] leading-snug" aria-live="polite">
              {result && (
                <p style={{ color: result.ok ? "var(--must)" : "var(--barred)" }}>{result.message}</p>
              )}
              {result?.warnings?.map((w) => (
                <p key={w} className="mt-1" style={{ color: "var(--any-of)" }}>
                  {w}
                </p>
              ))}
              {found.errors.map((e) => (
                <p key={e} className="mt-1" style={{ color: "var(--barred)" }}>
                  {e}
                </p>
              ))}
              {found.notes.map((n) => (
                <p key={n} className="mt-1 text-ink-faint">
                  {n}
                </p>
              ))}
            </section>
          )}
        </aside>
      </div>
      )}
    </div>
  );
}

function NodeInspector({
  tree,
  node,
  onChange,
  onDelete,
  onRemoveParent,
}: {
  tree: DesignTree;
  node: DesignNode;
  onChange: (change: (n: DesignNode) => DesignNode) => void;
  onDelete: () => void;
  onRemoveParent: (parentId: number) => void;
}) {
  const kind = node.kind === "choice" ? "choice" : node.entries[0]?.kind === "active" ? "active" : "passive";
  const setEntry = (i: number, change: (e: DesignEntry) => DesignEntry) =>
    onChange((n) => ({ ...n, entries: n.entries.map((e, j) => (j === i ? change(e) : e)) }));
  const parents = node.parents.map((pid) => tree.nodes.find((n) => n.nodeId === pid)).filter(Boolean) as DesignNode[];

  return (
    <section className="panel p-3.5" aria-label="Talent">
      <div className="flex items-baseline justify-between">
        <span className="label">Talent</span>
        <span className="num text-[10.5px] text-ink-faint">
          row {node.row + 1}, column {node.col + 1}
        </span>
      </div>

      <input
        className="field mt-2 w-full !text-[14px]"
        value={node.name}
        maxLength={80}
        onChange={(event) => onChange((n) => renameNode(n, event.target.value))}
        aria-label="Talent name"
      />

      <div className="seg mt-2 w-full" role="group" aria-label="Talent kind">
        {([
          ["passive", "Passive"],
          ["active", "Ability"],
          ["choice", "Choice of two"],
        ] as const).map(([id, label]) => (
          <button key={id} type="button" className="flex-1" aria-pressed={kind === id} onClick={() => onChange((n) => setKind(n, id))}>
            {label}
          </button>
        ))}
      </div>

      <div className="mt-2.5 grid grid-cols-[7rem_1fr] items-center gap-x-2 gap-y-2 text-[12px]">
        <span className="text-ink-soft">Ranks</span>
        <input
          className="field num"
          type="number"
          min={1}
          max={LIMITS.ranks}
          value={node.maxPoints}
          disabled={node.kind === "choice"}
          title={node.kind === "choice" ? "A choice of two takes one point" : undefined}
          onChange={(event) => {
            const ranks = Math.max(1, Math.min(LIMITS.ranks, Number(event.target.value) || 1));
            onChange((n) => ({ ...n, maxPoints: ranks, entries: n.entries.map((e) => ({ ...e, ranks: e.ranks.slice(0, ranks) })) }));
          }}
          aria-label="Ranks"
        />
        <span className="text-ink-soft">Points required</span>
        <input
          className="field num"
          type="number"
          min={0}
          placeholder={`from its row: ${gateOf(tree, { ...node, pointsRequired: null })}`}
          value={node.pointsRequired ?? ""}
          onChange={(event) =>
            onChange((n) => ({ ...n, pointsRequired: event.target.value === "" ? null : Math.max(0, Number(event.target.value)) }))
          }
          aria-label="Points required"
        />
        <span className="text-ink-soft" title="Always taken and costs no point, like retail's starting talents">Granted</span>
        <label className="flex items-center gap-1.5 text-ink-soft">
          <input
            type="checkbox"
            checked={Boolean(node.granted)}
            onChange={(event) => onChange((n) => ({ ...n, granted: event.target.checked || undefined }))}
            aria-label="Granted"
          />
          free, always taken
        </label>
      </div>

      {node.entries.map((entry, i) => (
        <div key={i} className="mt-3 border-t border-[color-mix(in_srgb,var(--brass)_15%,transparent)] pt-2.5">
          {node.kind === "choice" && (
            <input
              className="field mb-2 w-full"
              value={entry.name}
              maxLength={80}
              onChange={(event) => setEntry(i, (e) => ({ ...e, name: event.target.value }))}
              aria-label={`Alternative ${i + 1} name`}
              placeholder={`Alternative ${i + 1}`}
            />
          )}
          <IconPicker value={entry.icon} onPick={(icon) => setEntry(i, (e) => ({ ...e, icon }))} />
          {Array.from({ length: node.maxPoints }, (_, r) => (
            <textarea
              key={r}
              className="field mt-1.5 w-full resize-y !text-[11.5px]"
              rows={2}
              maxLength={600}
              placeholder={node.maxPoints > 1 ? `Rank ${r + 1} description (optional)` : "Description (optional)"}
              value={entry.ranks[r] ?? ""}
              onChange={(event) =>
                setEntry(i, (e) => {
                  const ranks = [...e.ranks];
                  while (ranks.length < r) ranks.push("");
                  ranks[r] = event.target.value;
                  return { ...e, ranks };
                })
              }
              aria-label={`Rank ${r + 1} description`}
            />
          ))}
        </div>
      ))}

      <div className="mt-3 border-t border-[color-mix(in_srgb,var(--brass)_15%,transparent)] pt-2.5 text-[12px]">
        <span className="text-ink-soft">Requires</span>
        {parents.length ? (
          <ul className="mt-1 space-y-0.5">
            {parents.map((p) => (
              <li key={p.nodeId} className="flex items-center gap-2">
                <span className="text-ink">{p.name}</span>
                <span className="text-[10.5px] text-ink-faint">at full rank</span>
                <button type="button" className="ml-auto text-ink-faint hover:text-ink" onClick={() => onRemoveParent(p.nodeId)} aria-label={`Stop requiring ${p.name}`}>
                  ×
                </button>
              </li>
            ))}
          </ul>
        ) : (
          <p className="mt-0.5 text-[11px] text-ink-faint">Nothing — opened by its row alone. Use Connect to add a requirement.</p>
        )}
      </div>

      <button type="button" className="btn mt-3 w-full" onClick={onDelete}>
        Delete talent
      </button>
    </section>
  );
}

function IconPicker({ value, onPick }: { value: string | null; onPick: (icon: string | null) => void }) {
  const [search, setSearch] = useState("");
  const [results, setResults] = useState<string[]>([]);

  useEffect(() => {
    const term = search.trim();
    if (term.length < 2) {
      setResults([]);
      return;
    }
    const timer = setTimeout(() => {
      void searchIcons(term, 48)
        .then(setResults)
        .catch(() => setResults([]));
    }, 200);
    return () => clearTimeout(timer);
  }, [search]);

  const current = iconUrl(value, 36);
  return (
    <div>
      <div className="flex items-center gap-2">
        <span
          className="h-9 w-9 shrink-0 rounded-[3px] bg-cover bg-center"
          style={{ backgroundImage: current ? `url(${current})` : undefined, backgroundColor: "var(--panel-sunken)" }}
          aria-hidden="true"
        />
        <input
          className="field min-w-0 flex-1"
          value={search}
          onChange={(event) => setSearch(event.target.value)}
          placeholder={value ?? "Search icons: fire, shield, arrow…"}
          aria-label="Search icons"
        />
        {value && (
          <button type="button" className="text-[11px] text-ink-faint hover:text-ink" onClick={() => onPick(null)}>
            clear
          </button>
        )}
      </div>
      {results.length > 0 && (
        <div className="mt-1.5 grid grid-cols-8 gap-1" role="listbox" aria-label="Icons">
          {results.map((name) => (
            <button
              key={name}
              type="button"
              role="option"
              aria-selected={name === value}
              className="h-8 w-8 rounded-[3px] bg-cover bg-center outline-offset-1"
              style={{ backgroundImage: `url(${iconUrl(name, 36)})`, outline: name === value ? "2px solid var(--star)" : undefined }}
              title={name}
              onClick={() => {
                onPick(name);
                setSearch("");
                setResults([]);
              }}
            />
          ))}
        </div>
      )}
    </div>
  );
}
