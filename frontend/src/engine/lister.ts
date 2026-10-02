import { groupsMet, movesOf, rulesOf, walk, type Graph, type Search, type State } from "./counter";

/**
 * Listing the builds a search matches, from the counter's own states.
 *
 * The counter's walk already knows, at every step, which states lead anywhere: a state that
 * cannot end at the asked points with every group met has no build behind it. So the walk is
 * run forward once, keeping each step's states. Then, going backward, each (state, points) is
 * marked if it can still finish. Finally the builds are read out forward along marked branches
 * only, so no branch is a dead end and listing costs what the builds themselves cost.
 *
 * Counting and listing share one set of rules (`movesOf`), so what is listed is exactly what is
 * counted. The C++ engine lists the same builds independently, and the release CI checks the
 * two agree as sets.
 *
 * Builds are sets in the engine's sense: a talent and its ranks, with a choice node's side left
 * open unless the search pins it. Each is keyed by talent id, granted talents left out, as the
 * engine's results always were.
 */

export interface Listing {
  builds: Record<string, number>[];
  /** Every matching set, listed or not. */
  total: bigint;
  /** True when `limit` stopped the listing early. */
  capped: boolean;
}

export function list(graph: Graph, search: Search, points: number, limit: number): Listing {
  const rules = rulesOf(graph, search, false);
  const groups = (search.atLeastOneOf ?? []).filter((g) => g.length).length;
  const layers: Map<string, State>[] = [];
  const final = walk(graph, search, points, false, (_, states) => layers.push(states));
  // layers[0] is the start, layers[i + 1] the states after step i.

  let total = 0n;
  for (const st of final.values()) if (groupsMet(st, groups)) total += st.v[points] ?? 0n;
  if (total === 0n || limit <= 0) return { builds: [], total, capped: total > 0n };

  // Backward: which (state, points spent) can still end at `points` with every group met.
  const n = graph.steps.length;
  const can: Map<string, Uint8Array>[] = new Array(n + 1);
  can[n] = new Map();
  for (const [key, st] of final) {
    if (!groupsMet(st, groups) || !st.v[points]) continue;
    const mark = new Uint8Array(points + 1);
    mark[points] = 1;
    can[n]!.set(key, mark);
  }
  for (let i = n - 1; i >= 0; i--) {
    const here = new Map<string, Uint8Array>();
    for (const [key, st] of layers[i]!) {
      const mark = new Uint8Array(points + 1);
      let any = false;
      for (const m of movesOf(graph, rules, i, st)) {
        const next = can[i + 1]!.get(m.key);
        if (!next) continue;
        for (let p = 0; p <= points; p++) {
          if (!st.v[p]) continue;
          const to = m.take ? p + 1 : p;
          if (m.take && (p < m.from || p >= points)) continue;
          if (next[to]) {
            mark[p] = 1;
            any = true;
          }
        }
      }
      if (any) here.set(key, mark);
    }
    can[i] = here;
  }

  // Forward: read the builds out along marked branches only.
  const builds: Record<string, number>[] = [];
  const taken: number[] = [];
  const read = (i: number, st: State, p: number): boolean => {
    if (builds.length >= limit) return false;
    if (i === n) {
      const build: Record<string, number> = {};
      for (const s of taken) {
        const id = String(graph.steps[s]!.orig);
        build[id] = (build[id] ?? 0) + 1;
      }
      builds.push(build);
      return true;
    }
    for (const m of movesOf(graph, rules, i, st)) {
      const to = m.take ? p + 1 : p;
      if (m.take && (p < m.from || p >= points)) continue;
      if (!can[i + 1]!.get(m.key)?.[to]) continue;
      const next = layers[i + 1]!.get(m.key)!;
      if (m.take) taken.push(i);
      read(i + 1, next, to);
      if (m.take) taken.pop();
      if (builds.length >= limit) return false;
    }
    return true;
  };
  const [startKey, start] = [...layers[0]!][0]!;
  if (can[0]!.get(startKey)?.[0]) read(0, start, 0);
  return { builds, total, capped: BigInt(builds.length) < total };
}
