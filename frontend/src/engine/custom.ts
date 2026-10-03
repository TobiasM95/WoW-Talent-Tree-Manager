/**
 * Player-designed trees: validate a project from the editor and build it into tree records.
 *
 * A port of services/ingest/ttm_ingest/custom.py, shared by the page (to build a saved
 * project's trees from its design) and the site's function that stores projects (to refuse
 * a design that could never be served). The messages are written for a person: they are
 * shown in the editor as they are.
 *
 * A project has one style, as the games do. **Classic**: one to three tabs gated by row, each
 * with its own budget or all sharing one pool. **Retail**: one class tree, one to four spec
 * trees and up to six hero trees, each with its own budget and gated by barriers; a hero tree
 * names the specs that may take it.
 *
 * **Content-addressed.** A project's id is the hash of its canonical form, so the same design
 * always has the same id, an edit is a new id, and a link always means one exact design.
 */

export const LIMITS = {
  trees: 3,
  specs: 4,
  heroes: 6,
  nodes: 150,
  ranks: 9,
  row: 30,
  col: 20,
  gate: 300,
  pool: 300,
  name: 80,
  text: 600,
} as const;
const PITCH = 600;
const ICON = /^[a-z0-9_-]{1,100}$/;

export class CustomTreeError extends Error {}

type Raw = Record<string, unknown>;
const isObject = (v: unknown): v is Raw => typeof v === "object" && v !== null && !Array.isArray(v);
const fail = (message: string): never => {
  throw new CustomTreeError(message);
};

function text(value: unknown, where: string, limit: number = LIMITS.name, required = true): string {
  if (value === null || value === undefined || (typeof value === "string" && !value.trim())) {
    if (required) fail(`${where} needs a name`);
    return "";
  }
  if (typeof value !== "string") fail(`${where} must be text`);
  const v = (value as string).trim();
  if (v.length > limit) fail(`${where} is longer than ${limit} characters`);
  return v;
}

function int(value: unknown, where: string, low: number, high: number): number {
  if (typeof value !== "number" || !Number.isInteger(value) || value < low || value > high) {
    fail(`${where} must be a whole number from ${low} to ${high}`);
  }
  return value as number;
}

export interface Entry {
  name: string;
  icon: string | null;
  kind: "passive" | "active";
  ranks: string[];
}

function entry(raw: unknown, where: string, maxRank: number): Entry {
  if (!isObject(raw)) fail(`${where} is not a talent`);
  const r = raw as Raw;
  let icon: string | null = null;
  if (r.icon !== null && r.icon !== undefined) {
    icon = String(r.icon).toLowerCase().replace(/\.jpg$/, "");
    if (!ICON.test(icon)) fail(`${where}: '${String(r.icon)}' is not an icon name`);
  }
  const kind = r.kind ?? "passive";
  if (kind !== "passive" && kind !== "active") fail(`${where}: kind must be passive or active`);
  const ranks = (r.ranks ?? []) as unknown;
  if (!Array.isArray(ranks) || ranks.length > maxRank) fail(`${where}: at most one description per rank`);
  return {
    name: text(r.name, where),
    icon,
    kind: kind as "passive" | "active",
    ranks: (ranks as unknown[]).map((x, i) => text(x, `${where} rank ${i + 1}`, LIMITS.text, false)),
  };
}

export interface CanonNode {
  nodeId: number;
  name: string;
  kind: "single" | "choice";
  maxPoints: number;
  row: number;
  col: number;
  pointsRequired: number;
  parents: number[];
  entries: Entry[];
  granted?: true;
}

export interface CanonTree {
  name: string;
  pointCap: number;
  pointsPerRow: number | null;
  nodes: CanonNode[];
  barriers?: { row: number; points: number }[];
  role?: "class" | "spec" | "hero";
  specs?: number[];
}

export interface Canon {
  name: string;
  style?: "retail";
  sharedPointCap: number | null;
  trees: CanonTree[];
}

function barriersOf(raw: unknown, treeName: string): { row: number; points: number }[] {
  if (raw === null || raw === undefined) return [];
  if (!Array.isArray(raw) || raw.length > LIMITS.row) fail(`${treeName}: barriers must be a list of up to ${LIMITS.row}`);
  const out = new Map<number, number>();
  for (const b of raw as unknown[]) {
    if (!isObject(b)) fail(`${treeName}: a barrier is a row and a number of points`);
    const row = int((b as Raw).row, `${treeName}: a barrier's row`, 1, LIMITS.row);
    if (out.has(row)) fail(`${treeName}: two barriers above row ${row + 1}`);
    out.set(row, int((b as Raw).points, `${treeName}: the barrier above row ${row + 1}`, 1, LIMITS.gate));
  }
  const rows = [...out.keys()].sort((a, b) => a - b);
  for (let i = 1; i < rows.length; i++) {
    if (out.get(rows[i]!)! <= out.get(rows[i - 1]!)!) {
      fail(`${treeName}: the barrier above row ${rows[i]! + 1} must ask for more than the one above it`);
    }
  }
  return rows.map((row) => ({ row, points: out.get(row)! }));
}

const inherited = (row: number, perRow: number | null, barriers: { row: number; points: number }[]) =>
  Math.max(
    Math.max(0, ...barriers.filter((b) => b.row <= row).map((b) => b.points)),
    perRow !== null ? row * perRow : 0,
  );

function rolesOf(trees: unknown[]): ("class" | "spec" | "hero")[] {
  const roles = trees.map((t) => (isObject(t) ? t.role : undefined));
  if (roles.some((r) => r !== "class" && r !== "spec" && r !== "hero")) {
    fail("each tree of a retail-style project is a class, spec or hero tree");
  }
  const count = (r: string) => roles.filter((x) => x === r).length;
  if (count("class") !== 1 || roles[0] !== "class") fail("a retail-style project has exactly one class tree, first");
  const specs = count("spec");
  const heroes = count("hero");
  if (specs < 1 || specs > LIMITS.specs) fail(`a retail-style project has one to ${LIMITS.specs} spec trees`);
  if (heroes > LIMITS.heroes) fail(`a retail-style project has at most ${LIMITS.heroes} hero trees`);
  const expected = ["class", ...Array(specs).fill("spec"), ...Array(heroes).fill("hero")];
  if (roles.join() !== expected.join()) fail("a retail-style project lists its class tree, then its specs, then its hero trees");
  return roles as ("class" | "spec" | "hero")[];
}

function heroSpecs(raw: unknown, specCount: number, treeName: string): number[] {
  if (raw === null || raw === undefined) return Array.from({ length: specCount }, (_, i) => i);
  if (!Array.isArray(raw) || !raw.length) fail(`${treeName}: a hero tree belongs to at least one spec`);
  return [...new Set((raw as unknown[]).map((s) => int(s, `${treeName}: a spec`, 0, specCount - 1)))].sort((a, b) => a - b);
}

function acyclic(nodes: CanonNode[], treeName: string) {
  const byId = new Map(nodes.map((n) => [n.nodeId, n]));
  const state = new Map<number, number>(); // 1 visiting, 2 done
  const visit = (id: number, path: string[]) => {
    if (state.get(id) === 2) return;
    if (state.get(id) === 1) fail(`${treeName}: ${[...path, byId.get(id)!.name].join(" -> ")} loops back on itself`);
    state.set(id, 1);
    for (const p of byId.get(id)!.parents) visit(p, [...path, byId.get(id)!.name]);
    state.set(id, 2);
  };
  for (const n of nodes) visit(n.nodeId, []);
}

/** Validate a project and reduce it to exactly what defines it, in a fixed order. */
export function canonical(project: unknown): Canon {
  if (!isObject(project)) fail("a project is an object with a name and trees");
  const p = project as Raw;
  const name = text(p.name, "The project", 60);
  const style = p.style ?? "classic";
  if (style !== "classic" && style !== "retail") fail("a project's style is retail or classic");
  const retail = style === "retail";
  let pool: number | null = null;
  if (p.sharedPointCap !== null && p.sharedPointCap !== undefined) {
    if (retail) fail("a retail-style project gives each tree its own budget, not a shared pool");
    pool = int(p.sharedPointCap, "The shared point pool", 1, LIMITS.pool);
  }
  if (!Array.isArray(p.trees)) fail("a project's trees are a list");
  const treesIn = p.trees as unknown[];
  const roles = retail ? rolesOf(treesIn) : [];
  if (!retail && (treesIn.length < 1 || treesIn.length > LIMITS.trees)) fail(`a classic project has one to ${LIMITS.trees} trees`);
  const specCount = roles.filter((r) => r === "spec").length;

  const seen = new Set<number>();
  const trees: CanonTree[] = treesIn.map((rawTree, t) => {
    if (!isObject(rawTree)) fail(`tree ${t + 1} is not a tree`);
    const rt = rawTree as Raw;
    const treeName = text(rt.name, `Tree ${t + 1}`, 60);
    let perRow: number | null = null;
    if (rt.pointsPerRow !== null && rt.pointsPerRow !== undefined) {
      if (retail) fail(`${treeName}: a retail-style tree gates with barriers, not points per row`);
      perRow = int(rt.pointsPerRow, `${treeName}: points per row`, 0, 50);
    }
    const barriers = barriersOf(rt.barriers, treeName);
    if (barriers.length && !retail) fail(`${treeName}: a classic tree gates by row, not with barriers`);
    if (!Array.isArray(rt.nodes) || rt.nodes.length < 1 || rt.nodes.length > LIMITS.nodes) {
      fail(`${treeName} has 1 to ${LIMITS.nodes} talents`);
    }

    const cells = new Map<string, string>();
    const ids = new Set<number>();
    const nodes: CanonNode[] = (rt.nodes as unknown[]).map((raw) => {
      if (!isObject(raw)) fail(`${treeName}: a talent is not an object`);
      const r = raw as Raw;
      const id = int(r.nodeId, `${treeName}: a talent id`, 1, 2 ** 31 - 1);
      if (seen.has(id)) fail(`${treeName}: talent id ${id} is used twice`);
      seen.add(id);
      ids.add(id);
      const label = text(r.name, `${treeName}: talent ${id}`);
      const where = `${treeName}: ${label}`;
      const maxRank = int(r.maxPoints, `${where}: ranks`, 1, LIMITS.ranks);
      const row = int(r.row, `${where}: row`, 0, LIMITS.row);
      const col = int(r.col, `${where}: column`, 0, LIMITS.col);
      const cell = `${row},${col}`;
      if (cells.has(cell)) fail(`${where} shares a cell with ${cells.get(cell)}`);
      cells.set(cell, label);
      const gate = int(
        r.pointsRequired === null || r.pointsRequired === undefined ? inherited(row, perRow, barriers) : r.pointsRequired,
        `${where}: points required`,
        0,
        LIMITS.gate,
      );
      const kind = r.kind ?? "single";
      if (kind !== "single" && kind !== "choice") fail(`${where}: kind must be single or choice`);
      // Python's `or`: a missing or empty list means one entry named like the talent.
      const entries = Array.isArray(r.entries) && r.entries.length === 0 ? [{ name: label }] : ((r.entries as unknown) ?? [{ name: label }]);
      if (!Array.isArray(entries)) fail(`${where}: entries must be a list`);
      const list = entries as unknown[];
      if (kind === "choice" && list.length !== 2) fail(`${where}: a choice node has exactly two alternatives`);
      if (kind === "single" && list.length !== 1) fail(`${where}: a single talent has exactly one entry`);
      if (kind === "choice" && maxRank !== 1) fail(`${where}: a choice node has one rank`);
      const granted = r.granted ?? false;
      if (typeof granted !== "boolean") fail(`${where}: granted is yes or no`);
      const parents = (r.parents ?? []) as unknown;
      if (!Array.isArray(parents) || parents.length > 8) fail(`${where}: parents must be a list of up to 8 talents`);
      const node: CanonNode = {
        nodeId: id,
        name: label,
        kind: kind as "single" | "choice",
        maxPoints: maxRank,
        row,
        col,
        pointsRequired: gate,
        parents: [...new Set((parents as unknown[]).map((x) => int(x, `${where}: a parent`, 1, 2 ** 31 - 1)))].sort((a, b) => a - b),
        entries: list.map((e, i) => entry(e, `${where} alternative ${i + 1}`, maxRank)),
      };
      // Free, like retail's starting talents. Only when set, so older ids hold.
      if (granted) node.granted = true;
      return node;
    });

    for (const n of nodes) {
      for (const parent of n.parents) {
        if (parent === n.nodeId) fail(`${treeName}: ${n.name} requires itself`);
        if (!ids.has(parent)) fail(`${treeName}: ${n.name} requires a talent that is not in the tree`);
      }
    }
    acyclic(nodes, treeName);

    const slots = nodes.reduce((a, n) => a + n.maxPoints, 0);
    const cap = rt.pointCap === null || rt.pointCap === undefined ? slots : int(rt.pointCap, `${treeName}: point budget`, 1, slots);
    nodes.sort((a, b) => a.row - b.row || a.col - b.col || a.nodeId - b.nodeId);
    const tree: CanonTree = { name: treeName, pointCap: cap, pointsPerRow: perRow, nodes };
    // Only when present, so a project saved before barriers existed keeps its id.
    if (barriers.length) tree.barriers = barriers;
    if (retail) {
      tree.role = roles[t];
      if (roles[t] === "hero") tree.specs = heroSpecs(rt.specs, specCount, treeName);
    }
    return tree;
  });

  if (retail) {
    const names = trees.filter((t) => t.role === "spec").map((t) => t.name);
    if (new Set(names).size !== names.length) fail("each spec tree needs its own name: the planner picks a spec by it");
    // Classic stays unmarked, so every project saved before styles existed keeps its id.
    return { name, style: "retail", sharedPointCap: null, trees };
  }
  return { name, sharedPointCap: pool, trees };
}

/** JSON with keys sorted at every level and no spaces: Python's json.dumps(sort_keys=True). */
export function canonicalJson(value: unknown): string {
  if (Array.isArray(value)) return `[${value.map(canonicalJson).join(",")}]`;
  if (isObject(value)) {
    return `{${Object.keys(value)
      .sort()
      .filter((k) => value[k] !== undefined)
      .map((k) => `${JSON.stringify(k)}:${canonicalJson(value[k])}`)
      .join(",")}}`;
  }
  return JSON.stringify(value);
}

/** A project's id: the first 16 hex characters of the SHA-256 of its canonical JSON. */
export async function projectId(canon: Canon): Promise<string> {
  const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(canonicalJson(canon)));
  return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("").slice(0, 16);
}

/** The project's trees in the shared tree format, as the planner and the counter read them. */
export function build(pid: string, canon: Canon) {
  const specNames = canon.trees.filter((t) => t.role === "spec").map((t) => t.name);
  return canon.trees.map((t, order) => {
    const role = t.role;
    const children = new Map<number, number[]>(t.nodes.map((n) => [n.nodeId, []]));
    for (const n of t.nodes) for (const p of n.parents) children.get(p)!.push(n.nodeId);
    const slots = t.nodes.reduce((a, n) => a + n.maxPoints, 0);
    const key = `custom/${pid}/${order}`;
    return {
      schemaVersion: 1,
      id: key,
      key,
      kind: (role ?? "tab") as "class" | "spec" | "hero" | "tab",
      game: "custom",
      name: t.name,
      description: "",
      classId: null,
      className: canon.name,
      specId: null,
      specName: role === "spec" ? t.name : null,
      heroSpecs: role === "hero" ? (t.specs ?? []).map((i) => specNames[i]!) : null,
      traitTreeId: null,
      // Hero trees are told apart by sub-tree id throughout; a project's own will do.
      subTreeId: role === "hero" ? order + 1 : null,
      gating: "reqPoints",
      pointCap: canon.sharedPointCap === null ? t.pointCap : Math.min(t.pointCap, canon.sharedPointCap),
      maxPointsInTree: slots,
      nodeCount: t.nodes.length,
      sharedPointCap: canon.sharedPointCap,
      pointsPerRow: t.pointsPerRow,
      barriers: t.barriers ?? [],
      order,
      project: pid,
      fullNodeOrder: null,
      subTreeSelector: null,
      source: { provider: "custom" },
      nodes: t.nodes.map((n) => ({
        nodeId: n.nodeId,
        localId: null,
        kind: n.kind,
        // The name the designer gave it, for choice nodes too.
        name: n.name,
        maxPoints: n.maxPoints,
        rankLevels: null,
        pointsRequired: n.pointsRequired,
        preFilled: Boolean(n.granted),
        freeLevel: null,
        entryNode: !n.parents.length,
        row: n.row,
        col: n.col,
        pos: { x: n.col * PITCH, y: n.row * PITCH },
        parents: n.parents,
        children: [...children.get(n.nodeId)!].sort((a, b) => a - b),
        requiresNode: null,
        subTreeId: null,
        entries: n.entries.map((e, i) => ({
          entryId: n.kind === "choice" ? n.nodeId * 2 + i : n.nodeId,
          definitionId: null,
          spellId: null,
          visibleSpellId: null,
          name: e.name,
          kind: e.kind,
          icon: e.icon,
          index: i,
          maxRanks: n.maxPoints,
          ranks: e.ranks,
        })),
      })),
    };
  });
}
