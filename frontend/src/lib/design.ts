import type { TreeDetail } from "./api";

/**
 * A custom project as the tree editor holds it: a set of trees, each a grid of talents.
 *
 * A project has one style, as the games do. **Classic**: one to three tabs gated by row, with
 * their own budgets or one shared pool -- WoW Forever's shape. **Retail**: one class tree, one
 * to four spec trees and up to six hero trees, each with its own budget and gated by barriers;
 * each hero tree names the specs that may take it, and planning one picks a spec, as retail does.
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
  /** Free: always taken, costing no point -- retail's starting talents. */
  granted?: boolean;
  parents: number[];
  entries: DesignEntry[];
}

/** Retail's gate: a line above `row`, crossed once `points` are spent in the tree. */
export interface Barrier {
  row: number;
  points: number;
}

export type TreeRole = "class" | "spec" | "hero";

export interface DesignTree {
  /** Retail style only: what the tree is. */
  role?: TreeRole;
  /** Retail hero trees: the specs that may take it, by place among the spec trees. */
  specs?: number[];
  name: string;
  /** Null: every rank in the tree can be bought. */
  pointCap: number | null;
  /** Classic: each row opens a fixed number of points after the last. Null in retail style. */
  pointsPerRow: number | null;
  /** Retail: lines between rows, crossed once enough is spent. */
  barriers?: Barrier[];
  nodes: DesignNode[];
}

export type TreeStyle = "retail" | "classic";

export interface Design {
  name: string;
  /** Missing on projects made before styles existed, which were all classic in shape. */
  style?: TreeStyle;
  /** Classic only. Null: each tree has its own budget. A number: all trees draw on one pool. */
  sharedPointCap: number | null;
  trees: DesignTree[];
}

export const styleOf = (d: Design): TreeStyle => d.style ?? "classic";

/**
 * A draft from before projects had one style, made one. For a few days a tree could use
 * barriers on its own; a draft with any is taken as retail -- its first tree the class tree,
 * the rest specs -- and anything else stays classic, as every earlier project was.
 */
export function normalise(d: Design): Design {
  if (d.style || !d.trees.some((t) => t.barriers?.length)) return d;
  return {
    ...d,
    style: "retail",
    sharedPointCap: null,
    trees: d.trees.map((t, i) => ({ ...t, role: i === 0 ? "class" : "spec", pointsPerRow: null })),
  };
}

export const LIMITS = {
  trees: 3,
  specs: 4,
  heroes: 6,
  nodes: 150,
  ranks: 9,
  rows: 30,
  cols: 20,
  engineSlots: 64,
} as const;

/** How many more trees of a role a project can take; classic tabs have no role. */
export function roomFor(design: Design, role?: TreeRole): number {
  if (styleOf(design) === "classic") return LIMITS.trees - design.trees.length;
  const have = design.trees.filter((t) => t.role === role).length;
  return role === "spec" ? LIMITS.specs - have : role === "hero" ? LIMITS.heroes - have : 0;
}

/** A blank tree: retail's are wide and gated by barriers, classic's are four columns by rows. */
export function blankTree(name: string, style: TreeStyle, role?: TreeRole): DesignTree {
  if (style === "classic") return { name, pointCap: null, pointsPerRow: 5, nodes: [] };
  if (role === "hero") return { role, specs: [], name, pointCap: 10, pointsPerRow: null, barriers: [], nodes: [] };
  return {
    role: role ?? "spec",
    name,
    pointCap: 30,
    pointsPerRow: null,
    barriers: [
      { row: 4, points: 8 },
      { row: 7, points: 20 },
    ],
    nodes: [],
  };
}

export const emptyDesign = (name = "New project", style: TreeStyle = "retail"): Design =>
  style === "retail"
    ? {
        name,
        style,
        sharedPointCap: null,
        trees: [blankTree("Class", "retail", "class"), blankTree("Spec 1", "retail", "spec")],
      }
    : { name, sharedPointCap: null, trees: [blankTree("Tree 1", "classic")] };

const roleRank: Record<TreeRole, number> = { class: 0, spec: 1, hero: 2 };

/**
 * Add trees to a project where they belong -- a retail spec before the hero trees, a hero tree
 * last -- renumbering their talents so ids stay unique within it. A new hero tree with no
 * specs is offered to all of them; a new spec is left for the designer to give hero trees to.
 */
export function insertTrees(design: Design, extra: DesignTree[]): { design: Design; at: number } {
  let next = 1 + Math.max(0, ...design.trees.flatMap((t) => t.nodes.map((n) => n.nodeId)));
  const specCount = design.trees.filter((t) => t.role === "spec").length;
  const trees = [...design.trees];
  let at = trees.length;
  for (const t of extra) {
    const ids = new Map(t.nodes.map((n) => [n.nodeId, next++]));
    const tree: DesignTree = {
      ...t,
      nodes: t.nodes.map((n) => ({ ...n, nodeId: ids.get(n.nodeId)!, parents: n.parents.map((p) => ids.get(p)!) })),
    };
    if (tree.role === "hero" && !tree.specs?.length) tree.specs = Array.from({ length: specCount }, (_, i) => i);
    const role = tree.role;
    at = role ? trees.filter((x) => roleRank[x.role ?? "spec"] <= roleRank[role]).length : trees.length;
    trees.splice(at, 0, tree);
  }
  return { design: { ...design, trees }, at };
}

/** Remove a tree. A retail spec's place is taken out of every hero tree's spec list. */
export function removeTree(design: Design, at: number): Design {
  const gone = design.trees[at];
  if (!gone) return design;
  const specIndex = gone.role === "spec" ? design.trees.slice(0, at).filter((t) => t.role === "spec").length : -1;
  return {
    ...design,
    trees: design.trees
      .filter((_, i) => i !== at)
      .map((t) =>
        specIndex >= 0 && t.role === "hero"
          ? { ...t, specs: (t.specs ?? []).filter((s) => s !== specIndex).map((s) => (s > specIndex ? s - 1 : s)) }
          : t,
      ),
  };
}

/** The gate a row gives the talents in it: its points-per-row, or the barriers above it. */
export const rowGate = (tree: DesignTree, row: number) =>
  Math.max(
    (tree.pointsPerRow ?? 0) * row,
    ...(tree.barriers ?? []).filter((b) => b.row <= row).map((b) => b.points),
    0,
  );

/** The gate a talent actually has, whether set on it or inherited from where it sits. */
export const gateOf = (tree: DesignTree, node: DesignNode) => node.pointsRequired ?? rowGate(tree, node.row);

/**
 * Set, move or clear the barrier above `row` (points 0 clears it). Barriers must rise going
 * down, as the server insists, so this refuses one that would not.
 */
export function setBarrier(tree: DesignTree, row: number, points: number): DesignTree | null {
  const rest = (tree.barriers ?? []).filter((b) => b.row !== row);
  const next = points > 0 ? [...rest, { row, points }].sort((a, b) => a.row - b.row) : rest;
  for (let i = 1; i < next.length; i++) if (next[i]!.points <= next[i - 1]!.points) return null;
  return { ...tree, barriers: next };
}

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
  if (styleOf(design) === "retail") {
    const specs = design.trees.filter((t) => t.role === "spec").map((t) => t.name.trim());
    if (new Set(specs).size !== specs.length) errors.push("Each spec tree needs its own name: the planner picks a spec by it.");
    for (const t of design.trees) {
      if (t.role === "hero" && !t.specs?.length) errors.push(`${t.name || "A hero tree"} is taken by no spec yet.`);
    }
  }
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
 * within it. Granted talents stay granted, so a copy counts exactly like its original;
 * hero-tree selectors, which are not talents, are left out.
 */
export function fromTrees(name: string, trees: TreeDetail[], sharedPointCap: number | null): Design {
  if (trees.some((t) => t.kind === "class" || t.kind === "spec" || t.kind === "hero")) return fromRetail(name, trees);
  return { name, sharedPointCap, trees: trees.slice(0, LIMITS.trees).map(copyTree) };
}

/**
 * Retail trees as a retail-style project: the class tree, each spec tree given, and every
 * hero tree once, taken by the specs it came with. Retail lists a hero tree under each spec
 * that can take it, which is how its spec list is rebuilt here.
 */
function fromRetail(name: string, trees: TreeDetail[]): Design {
  const specs = trees.filter((t) => t.kind === "spec").slice(0, LIMITS.specs);
  const specNames = specs.map((t) => t.specName);
  const heroes = new Map<number | string, { tree: TreeDetail; specs: Set<number> }>();
  for (const h of trees.filter((t) => t.kind === "hero")) {
    const id = h.subTreeId ?? h.key;
    const entry = heroes.get(id) ?? { tree: h, specs: new Set<number>() };
    const s = specNames.indexOf(h.specName);
    if (s >= 0) entry.specs.add(s);
    heroes.set(id, entry);
  }
  const classTree = trees.find((t) => t.kind === "class");
  return {
    name,
    style: "retail",
    sharedPointCap: null,
    trees: [
      classTree ? { ...copyTree(classTree), role: "class" as const, name: "Class" } : blankTree("Class", "retail", "class"),
      ...specs.map((t) => ({ ...copyTree(t), role: "spec" as const, name: (t.specName ?? t.name).slice(0, 60) })),
      ...[...heroes.values()].slice(0, LIMITS.heroes).map(({ tree, specs: s }) => ({
        ...copyTree(tree),
        role: "hero" as const,
        specs: [...s].sort((a, b) => a - b),
      })),
    ],
  };
}

/** One real tree as a design tree: grid, gates, arrows, icons and rank texts. */
export function copyTree(t: TreeDetail): DesignTree {
  const ids = new Set(t.nodes.filter((n) => n.kind !== "subtree").map((n) => n.nodeId));
  const barriers = t.pointsPerRow ? [] : barriersOf(t);
  const shell: DesignTree = { name: "", pointCap: null, pointsPerRow: t.pointsPerRow ?? null, barriers, nodes: [] };
  return {
    name: t.name.slice(0, 60),
    pointCap: t.pointCap ?? null,
    pointsPerRow: t.pointsPerRow ?? null,
    barriers,
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
          // Explicit only where the talent's gate differs from what its row now gives it.
          pointsRequired:
            t.pointsPerRow || n.pointsRequired === rowGate(shell, Math.min(LIMITS.rows, n.row ?? 0))
              ? null
              : n.pointsRequired,
          ...(n.preFilled ? { granted: true } : {}),
          parents: n.parents.filter((p) => ids.has(p)),
          entries: entries.length ? entries : [{ name: n.name, icon: null, kind: "passive" as const, ranks: [] }],
        };
      }),
  };
}

/**
 * A real tree's gates as barriers: each distinct gate becomes a line above the first row that
 * uses it. Retail's 8 and 20 come out as two lines; a gate that would not rise going down is
 * left out, and its talents keep it as their own.
 */
export function barriersOf(t: TreeDetail): Barrier[] {
  const first = new Map<number, number>();
  for (const n of t.nodes) {
    if (n.kind === "subtree" || !n.pointsRequired) continue;
    const row = Math.min(LIMITS.rows, n.row ?? 0);
    first.set(n.pointsRequired, Math.min(first.get(n.pointsRequired) ?? Infinity, row));
  }
  const out: Barrier[] = [];
  for (const [points, row] of [...first].sort((a, b) => a[0] - b[0])) {
    const last = out[out.length - 1];
    if (row >= 1 && (!last || row > last.row)) out.push({ row, points });
  }
  return out;
}
