/**
 * The build counter, in the browser: how many builds a tree has under a search, at every point
 * total, without listing them.
 *
 * A port of the frontier DP (tools/frontier-dp/frontier_dp.py), which replicates the C++
 * engine's rules, and kept at parity with that engine by the release CI (see
 * docs/03-plan/browser-only.md):
 *
 *   - a granted root is removed and its children become roots;
 *   - a multi-rank talent is a chain of single ranks, and its children hang off the last rank,
 *     so they open only once it is maxed;
 *   - talents are visited in a topological order, ready ones sorted by points required;
 *   - a talent can be taken once enough points are spent before it, and once it is a root or
 *     one of its parents is taken.
 *
 * The DP walks that order keeping, per state, the taken talents that an unvisited child still
 * needs and the points spent, so it counts every build without enumerating any.
 *
 * Groups ("at least one of", "exactly one of") are part of the state here: a flag per group,
 * or the number of its members taken so far. The Python counts them by combining several runs
 * (inclusion-exclusion). That gives the same numbers, but tracking them in the state counts in
 * one run, and the same states can be walked to list the builds.
 *
 * Counts are BigInt. A single tree can pass 2^53, and a rounded count is a wrong count.
 */

export interface CounterNode {
  nodeId: number;
  kind: string;
  maxPoints: number;
  pointsRequired: number;
  preFilled: boolean;
  parents: number[];
  children: number[];
  rankLevels?: { level: number; maxRanks: number }[] | null;
}

export interface CounterTree {
  nodes: CounterNode[];
}

export type Side = "a" | "b" | "none";

export interface Search {
  mustHave?: number[];
  mustNotHave?: number[];
  choiceSides?: Record<string, Side>;
  atLeastOneOf?: number[][];
  exactlyOneOf?: number[][];
  rankMin?: Record<string, number>;
  rankMax?: Record<string, number>;
}

/** One rank of one talent: a step of the walk. */
export interface Step {
  orig: number;
  rank: number;
  req: number;
  choice: boolean;
  /** Positions in the walk of the steps this one hangs off. */
  parents: number[];
  /** The last position that still needs to know whether this step was taken. */
  lastNeeded: number;
}

export interface Graph {
  steps: Step[];
  /** Point slots: the most points the tree can hold. */
  slots: number;
  /** Talents the walk models; granted roots are not among them. */
  modelled: Set<number>;
  /** Ranks per talent, at the level cap. */
  ranks: Map<number, number>;
}

/** Tiered talents' ranks depend on character level. Mirrors ttm_format._resolve_max_points. */
export function resolveMaxPoints(node: CounterNode, levelCap: number | null): number {
  const declared = node.maxPoints || 1;
  const steps = node.rankLevels;
  if (!steps || !steps.length || levelCap === null) return declared;
  let allowed = 0;
  for (const s of steps) if (levelCap >= s.level) allowed = Math.max(allowed, s.maxRanks);
  return allowed ? Math.max(1, Math.min(declared, allowed)) : 1;
}

/** Tuple order on (talent id, rank), as Python sorts its node keys. */
const byKey = (a: [number, number], b: [number, number]) => a[0] - b[0] || a[1] - b[1];

/** The walk: granted roots removed, ranks expanded, in the engine's order. */
export function buildGraph(tree: CounterTree, levelCap: number | null = 90): Graph {
  const ids = new Set(tree.nodes.map((n) => n.nodeId));
  const nodes = new Map(
    tree.nodes.map((n) => [
      n.nodeId,
      {
        id: n.nodeId,
        choice: n.kind === "choice",
        maxPoints: resolveMaxPoints(n, levelCap),
        req: n.pointsRequired,
        preFilled: Boolean(n.preFilled),
        par: new Set(n.parents.filter((p) => ids.has(p))),
        chi: new Set(n.children.filter((c) => ids.has(c))),
      },
    ]),
  );

  // Granted roots go, and their children become roots, to a fixed point: a promoted child
  // may itself be granted. A promoted child loses every parent link, as in the engine.
  const alive = new Set(nodes.keys());
  for (let changed = true; changed; ) {
    changed = false;
    for (const id of [...alive]) {
      const n = nodes.get(id)!;
      if (!alive.has(id) || n.par.size || !n.preFilled) continue;
      for (const c of n.chi) {
        const child = nodes.get(c)!;
        for (const p of child.par) nodes.get(p)?.chi.delete(c);
        child.par.clear();
      }
      alive.delete(id);
      n.chi = new Set();
      changed = true;
    }
  }

  // Ranks expand into chains; a talent's children hang off its last rank. Edges come from the
  // children lists, as in the Python and the engine.
  type Key = [number, number];
  const keyOf = (k: Key) => `${k[0]}:${k[1]}`;
  const par = new Map<string, Key[]>();
  const chi = new Map<string, Key[]>();
  const meta = new Map<string, { key: Key; req: number; choice: boolean }>();
  const link = (from: Key, to: Key) => {
    chi.get(keyOf(from))!.push(to);
    par.get(keyOf(to))!.push(from);
  };
  const live = [...alive].sort((a, b) => a - b);
  for (const id of live) {
    const n = nodes.get(id)!;
    for (let r = 0; r < n.maxPoints; r++) {
      const key: Key = [id, r];
      meta.set(keyOf(key), { key, req: n.req, choice: n.choice });
      par.set(keyOf(key), []);
      chi.set(keyOf(key), []);
    }
  }
  for (const id of live) {
    const n = nodes.get(id)!;
    for (let r = 1; r < n.maxPoints; r++) link([id, r - 1], [id, r]);
  }
  for (const id of live) {
    const n = nodes.get(id)!;
    for (const c of n.chi) if (alive.has(c)) link([id, n.maxPoints - 1], [c, 0]);
  }

  // Kahn's algorithm, the ready queue kept sorted by (points required, id, rank).
  const indeg = new Map([...meta.keys()].map((k) => [k, par.get(k)!.length]));
  const readyKey = (k: Key) => [meta.get(keyOf(k))!.req, k[0], k[1]] as const;
  const sortReady = (list: Key[]) =>
    list.sort((a, b) => {
      const x = readyKey(a);
      const y = readyKey(b);
      return x[0] - y[0] || x[1] - y[1] || x[2] - y[2];
    });
  let ready = sortReady([...meta.values()].filter((m) => indeg.get(keyOf(m.key)) === 0).map((m) => m.key));
  const order: Key[] = [];
  while (ready.length) {
    const n = ready.shift()!;
    order.push(n);
    for (const m of [...chi.get(keyOf(n))!].sort(byKey)) {
      const k = keyOf(m);
      indeg.set(k, indeg.get(k)! - 1);
      if (indeg.get(k) === 0) ready.push(m);
    }
    ready = sortReady(ready);
  }
  if (order.length !== meta.size) throw new Error(`the tree has a cycle: ${order.length} of ${meta.size} ranks ordered`);

  const pos = new Map(order.map((k, i) => [keyOf(k), i]));
  const steps: Step[] = order.map((k) => {
    const m = meta.get(keyOf(k))!;
    const children = chi.get(keyOf(k))!.map((c) => pos.get(keyOf(c))!);
    return {
      orig: k[0],
      rank: k[1],
      req: m.req,
      choice: m.choice,
      parents: par.get(keyOf(k))!.map((p) => pos.get(keyOf(p))!),
      lastNeeded: children.length ? Math.max(...children) : -1,
    };
  });
  return {
    steps,
    slots: steps.length,
    modelled: alive,
    ranks: new Map(live.map((id) => [id, nodes.get(id)!.maxPoints])),
  };
}

interface Rules {
  mustTake: boolean[];
  mustSkip: boolean[];
  multiplier: number[];
  /** For a step that is a talent's first rank: the groups it belongs to. */
  atLeast: number[][];
  exactly: number[][];
  atLeastCount: number;
  exactlyCount: number;
}

function rulesOf(graph: Graph, search: Search, weightChoices: boolean): Rules {
  const require = new Set(search.mustHave ?? []);
  const exclude = new Set(search.mustNotHave ?? []);
  const sides = search.choiceSides ?? {};
  const rankMin = search.rankMin ?? {};
  const rankMax = search.rankMax ?? {};
  const atLeastGroups = (search.atLeastOneOf ?? []).filter((g) => g.length);
  const exactlyGroups = (search.exactlyOneOf ?? []).filter((g) => g.length);
  const memberOf = (groups: number[][], id: number) =>
    groups.flatMap((g, i) => (g.includes(id) ? [i] : []));

  const rules: Rules = {
    mustTake: [],
    mustSkip: [],
    multiplier: [],
    atLeast: [],
    exactly: [],
    atLeastCount: atLeastGroups.length,
    exactlyCount: exactlyGroups.length,
  };
  for (const s of graph.steps) {
    const side = s.choice ? (sides[String(s.orig)] ?? "either") : "either";
    rules.multiplier.push(weightChoices && s.choice && side === "either" ? 2 : 1);
    // A required talent: its first rank carries it, later ranks hang off that one. A minimum
    // rank forces the first ranks; a cap forbids the rank after it.
    const min = rankMin[String(s.orig)] ?? 0;
    const max = rankMax[String(s.orig)];
    const take = (require.has(s.orig) && s.rank === 0) || s.rank < min || side === "a" || side === "b";
    const skip = exclude.has(s.orig) || (max !== undefined && s.rank >= max) || side === "none";
    rules.mustTake.push(take);
    rules.mustSkip.push(skip);
    // A talent is "taken" for a group when its first rank is.
    rules.atLeast.push(s.rank === 0 ? memberOf(atLeastGroups, s.orig) : []);
    rules.exactly.push(s.rank === 0 ? memberOf(exactlyGroups, s.orig) : []);
  }
  return rules;
}

/** One DP state: taken steps still needed, group progress, and counts by points spent. */
interface State {
  live: number[];
  /** At-least-one groups as a bitmask, then exactly-one counts (0 or 1) per group. */
  atLeast: number;
  exactly: number[];
  v: bigint[];
}

const stateKey = (live: number[], atLeast: number, exactly: number[]) =>
  `${live.join(",")}|${atLeast}|${exactly.join("")}`;

/**
 * Walk the DP, calling `layer` after each step with the states it ends in. The counter only
 * needs the last layer; the lister keeps them all.
 */
export function walk(
  graph: Graph,
  search: Search,
  maxPoints: number,
  weightChoices: boolean,
  layer?: (index: number, states: Map<string, State>) => void,
): Map<string, State> {
  const rules = rulesOf(graph, search, weightChoices);
  const width = maxPoints + 1;
  let states = new Map<string, State>();
  const empty = new Array(rules.exactlyCount).fill(0);
  const start: bigint[] = new Array(width).fill(0n);
  start[0] = 1n;
  states.set(stateKey([], 0, empty), { live: [], atLeast: 0, exactly: empty, v: start });

  const add = (into: Map<string, State>, live: number[], atLeast: number, exactly: number[], pts: number, count: bigint) => {
    const key = stateKey(live, atLeast, exactly);
    let st = into.get(key);
    if (!st) {
      st = { live, atLeast, exactly, v: new Array(width).fill(0n) };
      into.set(key, st);
    }
    st.v[pts]! += count;
  };

  graph.steps.forEach((step, i) => {
    const next = new Map<string, State>();
    const keep = (live: number[]) => live.filter((x) => graph.steps[x]!.lastNeeded > i);
    for (const st of states.values()) {
      // Skip this step, unless the search insists on it.
      if (!rules.mustTake[i]) {
        const live = keep(st.live);
        for (let p = 0; p < width; p++) if (st.v[p]) add(next, live, st.atLeast, st.exactly, p, st.v[p]!);
      }
      // Take it: allowed by the search, reachable from a parent, behind its gate.
      if (rules.mustSkip[i]) continue;
      if (step.parents.length && !step.parents.some((p) => st.live.includes(p))) continue;
      let exactly = st.exactly;
      if (rules.exactly[i]!.length) {
        if (rules.exactly[i]!.some((g) => st.exactly[g])) continue; // a second member: never exactly one
        exactly = [...st.exactly];
        for (const g of rules.exactly[i]!) exactly[g] = 1;
      }
      let atLeast = st.atLeast;
      for (const g of rules.atLeast[i]!) atLeast |= 1 << g;
      const live = keep([...st.live, i].sort((a, b) => a - b));
      const mult = BigInt(rules.multiplier[i]!);
      for (let p = Math.max(0, step.req); p < maxPoints; p++) {
        if (st.v[p]) add(next, live, atLeast, exactly, p + 1, st.v[p]! * mult);
      }
    }
    states = next;
    layer?.(i, states);
  });
  return states;
}

/** Whether a final state satisfies every group. */
export const groupsMet = (st: { atLeast: number; exactly: number[] }, atLeastCount: number) =>
  st.atLeast === (1 << atLeastCount) - 1 && st.exactly.every((x) => x === 1);

/** Counts at every point total 0..maxPoints. Index k: builds (or sets) of exactly k points. */
export function countSpread(graph: Graph, search: Search, maxPoints: number, weightChoices: boolean): bigint[] {
  const final = walk(graph, search, maxPoints, weightChoices);
  const groups = (search.atLeastOneOf ?? []).filter((g) => g.length).length;
  const totals: bigint[] = new Array(maxPoints + 1).fill(0n);
  for (const st of final.values()) {
    if (!groupsMet(st, groups)) continue;
    for (let p = 0; p <= maxPoints; p++) totals[p]! += st.v[p]!;
  }
  return totals;
}

/** Sets and builds at exactly `points`. */
export function count(graph: Graph, search: Search, points: number): { sets: bigint; builds: bigint } {
  return {
    sets: countSpread(graph, search, points, false)[points]!,
    builds: countSpread(graph, search, points, true)[points]!,
  };
}
