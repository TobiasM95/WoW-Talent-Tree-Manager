import type { TreeDetail } from "./api";

/**
 * A custom project as the tree editor holds it: one to three trees, each a grid of talents.
 *
 * The shape is exactly what `POST /custom-trees` accepts, so saving is sending this object,
 * and the server's validation is the authority. The checks here exist to stop a person making
 * an edit that could never be saved -- a loop, a second talent in an occupied cell -- at the
 * moment they make it, instead of in an error after the fact.
 *
 * Every edit is a pure function returning a new design, which is what makes undo a stack of
 * old values and nothing more.
 */

export interface DesignEntry {
  name: string;
  icon: string | null;
  kind: "passive" | "active";
  ranks: string[];
}

export interface DesignNode {
  nodeId: number;
  name: string;
  kind: "single" | "choice";
  maxPoints: number;
  row: number;
  col: number;
  /** Null takes the tree's points-per-row gate for this row. */
  pointsRequired: number | null;
  parents: number[];
  entries: DesignEntry[];
}

export interface DesignTree {
  name: string;
  /** Null: every rank in the tree can be bought. */
  pointCap: number | null;
  /** Null: gates are set per talent. */
  pointsPerRow: number | null;
  nodes: DesignNode[];
}

export interface Design {
  name: string;
  /** Null: each tree has its own budget. A number: all trees draw on one pool. */
  sharedPointCap: number | null;
  trees: DesignTree[];
}

export const LIMITS = { trees: 3, nodes: 150, ranks: 9, rows: 30, cols: 20, engineSlots: 64 } as const;

export const emptyDesign = (name = "New project"): Design => ({
  name,
  sharedPointCap: null,
  trees: [{ name: "Tree 1", pointCap: null, pointsPerRow: 5, nodes: [] }],
});

/** The gate a talent actually has, whether set on it or inherited from its row. */
export const gateOf = (tree: DesignTree, node: DesignNode) =>
  node.pointsRequired ?? (tree.pointsPerRow ?? 0) * node.row;

const nextId = (design: Design) =>
  1 + Math.max(0, ...design.trees.flatMap((t) => t.nodes.map((n) => n.nodeId)));

const withTree = (design: Design, at: number, change: (t: DesignTree) => DesignTree): Design => ({
  ...design,
  trees: design.trees.map((t, i) => (i === at ? change(t) : t)),
});

export function addNode(design: Design, at: number, row: number, col: number): { design: Design; nodeId: number } {
  const nodeId = nextId(design);
  const node: DesignNode = {
    nodeId,
    name: "New talent",
    kind: "single",
    maxPoints: 1,
    row,
    col,
    pointsRequired: null,
    parents: [],
    entries: [{ name: "New talent", icon: null, kind: "passive", ranks: [] }],
  };
  return { design: withTree(design, at, (t) => ({ ...t, nodes: [...t.nodes, node] })), nodeId };
}

export function updateNode(design: Design, at: number, nodeId: number, change: (n: DesignNode) => DesignNode): Design {
  return withTree(design, at, (t) => ({ ...t, nodes: t.nodes.map((n) => (n.nodeId === nodeId ? change(n) : n)) }));
}

/** Rename a talent, keeping a single talent's one entry in step with it. */
export const renameNode = (n: DesignNode, name: string): DesignNode =>
  n.kind === "single" ? { ...n, name, entries: [{ ...n.entries[0]!, name }] } : { ...n, name };

export function setKind(n: DesignNode, kind: "passive" | "active" | "choice"): DesignNode {
  if (kind === "choice") {
    const first = n.entries[0] ?? { name: n.name, icon: null, kind: "passive" as const, ranks: [] };
    return {
      ...n,
      kind: "choice",
      maxPoints: 1,
      entries: [
        { ...first, ranks: first.ranks.slice(0, 1) },
        n.entries[1] ?? { name: "Alternative", icon: null, kind: "passive", ranks: [] },
      ],
    };
  }
  const first = n.entries[0] ?? { name: n.name, icon: null, kind, ranks: [] };
  return { ...n, kind: "single", entries: [{ ...first, name: n.name, kind }] };
}

export function moveNode(design: Design, at: number, nodeId: number, row: number, col: number): Design | null {
  const tree = design.trees[at]!;
  if (tree.nodes.some((n) => n.nodeId !== nodeId && n.row === row && n.col === col)) return null;
  return updateNode(design, at, nodeId, (n) => ({ ...n, row, col }));
}

export function deleteNode(design: Design, at: number, nodeId: number): Design {
  return withTree(design, at, (t) => ({
    ...t,
    nodes: t.nodes
      .filter((n) => n.nodeId !== nodeId)
      .map((n) => ({ ...n, parents: n.parents.filter((p) => p !== nodeId) })),
  }));
}

/** Would `child` requiring `parent` make a talent require itself? */
export function wouldLoop(tree: DesignTree, parent: number, child: number): boolean {
  if (parent === child) return true;
  const byId = new Map(tree.nodes.map((n) => [n.nodeId, n]));
  // A loop exists if `parent` already requires `child`, however indirectly.
  const stack = [parent];
  const seen = new Set<number>();
  while (stack.length) {
    const id = stack.pop()!;
    if (id === child) return true;
    if (seen.has(id)) continue;
    seen.add(id);
    stack.push(...(byId.get(id)?.parents ?? []));
  }
  return false;
}

/** Add the edge if absent, remove it if present. Null when adding it would make a loop. */
export function toggleEdge(design: Design, at: number, parent: number, child: number): Design | null {
  const tree = design.trees[at]!;
  const node = tree.nodes.find((n) => n.nodeId === child);
  if (!node) return design;
  if (node.parents.includes(parent)) {
    return updateNode(design, at, child, (n) => ({ ...n, parents: n.parents.filter((p) => p !== parent) }));
  }
  if (wouldLoop(tree, parent, child)) return null;
  return updateNode(design, at, child, (n) => ({ ...n, parents: [...n.parents, parent] }));
}

/**
 * What would stop this design saving or being useful, found before the server says so.
 *
 * Errors block saving; notes are true but survivable -- a tree too large to list builds for
 * can still be counted and planned.
 */
export function problems(design: Design): { errors: string[]; notes: string[] } {
  const errors: string[] = [];
  const notes: string[] = [];
  if (!design.name.trim()) errors.push("The project needs a name.");
  for (const t of design.trees) {
    const label = t.name.trim() || "A tree";
    if (!t.name.trim()) errors.push("Every tree needs a name.");
    if (!t.nodes.length) errors.push(`${label} has no talents yet: click an empty cell to add one.`);
    if (t.nodes.length > LIMITS.nodes) errors.push(`${label} has more than ${LIMITS.nodes} talents.`);
    for (const n of t.nodes) {
      if (!n.name.trim()) errors.push(`${label}: a talent needs a name.`);
      if (n.kind === "choice" && n.entries.some((e) => !e.name.trim())) {
        errors.push(`${label}: ${n.name || "a choice"} needs both alternatives named.`);
      }
    }
    const slots = t.nodes.reduce((a, n) => a + n.maxPoints, 0);
    if (slots > LIMITS.engineSlots) {
      notes.push(`${label} has ${slots} ranks: it can be counted and planned, but listing its builds needs ${LIMITS.engineSlots} or fewer.`);
    }
    const budget = design.sharedPointCap ?? t.pointCap ?? slots;
    const out = t.nodes.filter((n) => gateOf(t, n) >= budget && budget > 0);
    if (out.length) notes.push(`${label}: ${out.map((n) => n.name).join(", ")} can never be reached within ${budget} points.`);
  }
  return { errors, notes };
}

/**
 * An editable copy of existing trees -- the start of every "what if Blizzard changed this".
 *
 * Retail and Forever trees come in with their grid, gates, arrows, icons and rank texts. Their
 * node ids are kept, which is safe: a copy is a new project and ids only need to be unique
 * within it. Granted talents come across as ordinary ones, since a design has no notion of
 * free points; hero-tree selectors, which are not talents, are left out.
 */
export function fromTrees(name: string, trees: TreeDetail[], sharedPointCap: number | null): Design {
  return {
    name,
    sharedPointCap,
    trees: trees.slice(0, LIMITS.trees).map((t) => {
      const ids = new Set(t.nodes.filter((n) => n.kind !== "subtree").map((n) => n.nodeId));
      return {
        name: t.name.slice(0, 60),
        pointCap: t.pointCap ?? null,
        pointsPerRow: t.pointsPerRow ?? null,
        nodes: t.nodes
          .filter((n) => n.kind !== "subtree")
          .map((n) => {
            const choice = n.kind === "choice" && n.entries.length >= 2;
            const entries = (choice ? n.entries.slice(0, 2) : n.entries.slice(0, 1)).map((e) => ({
              name: (e.name || n.name).slice(0, 80),
              icon: e.icon ?? null,
              kind: (e.kind === "active" ? "active" : "passive") as "active" | "passive",
              ranks: (e.ranks ?? []).slice(0, n.maxPoints).map((r) => r.slice(0, 600)),
            }));
            return {
              nodeId: n.nodeId,
              name: n.name.slice(0, 80),
              kind: choice ? ("choice" as const) : ("single" as const),
              maxPoints: choice ? 1 : Math.min(LIMITS.ranks, Math.max(1, n.maxPoints)),
              row: Math.min(LIMITS.rows, n.row ?? 0),
              col: Math.min(LIMITS.cols, n.col ?? 0),
              pointsRequired: t.pointsPerRow ? null : n.pointsRequired,
              parents: n.parents.filter((p) => ids.has(p)),
              entries: entries.length ? entries : [{ name: n.name, icon: null, kind: "passive" as const, ranks: [] }],
            };
          }),
      };
    }),
  };
}
