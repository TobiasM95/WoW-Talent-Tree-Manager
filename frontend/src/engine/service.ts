import { buildGraph, countSpread, type CounterTree, type Graph, type Search } from "./counter";
import { list } from "./lister";

/**
 * What the API's /counts, /counts/spread and /solve did, in the page: validate a search
 * against a tree, then count it, spread it over every point total, or list its builds.
 *
 * The checks and their sentences are the API's, because the page shows them as they are:
 * which talent, which budget, why. A search the counter would silently misread is refused
 * instead -- a constraint on a granted talent, for instance, used to be quietly ignored.
 */

export const LEVEL_CAP = 90;

export class SearchError extends Error {
  constructor(
    readonly status: number,
    message: string,
  ) {
    super(message);
  }
}

const graphs = new WeakMap<CounterTree, Graph>();
export function graphOf(tree: CounterTree): Graph {
  let g = graphs.get(tree);
  if (!g) {
    g = buildGraph(tree, LEVEL_CAP);
    graphs.set(tree, g);
  }
  return g;
}

const ids = (list: Iterable<number>) => `[${[...list].sort((a, b) => a - b).slice(0, 5).join(", ")}]`;

export function validate(key: string, tree: CounterTree, search: Search, points: number): Graph {
  const graph = graphOf(tree);
  const inTree = new Set(tree.nodes.map((n) => n.nodeId));
  const groups = [...(search.atLeastOneOf ?? []), ...(search.exactlyOneOf ?? [])];
  const named = new Set<number>([
    ...(search.mustHave ?? []),
    ...(search.mustNotHave ?? []),
    ...Object.keys(search.choiceSides ?? {}).map(Number),
    ...groups.flat(),
    ...Object.keys(search.rankMin ?? {}).map(Number),
    ...Object.keys(search.rankMax ?? {}).map(Number),
  ]);
  for (const g of groups) {
    if (g.length < 2) throw new SearchError(400, "a group constraint needs at least two nodes; use mustHave for one");
  }
  const unknown = [...named].filter((id) => !inTree.has(id));
  if (unknown.length) throw new SearchError(400, `node id(s) ${ids(unknown)} are not in '${key}'`);
  // Granted talents are no part of any build's point spend; a constraint on one has no meaning.
  const granted = [...named].filter((id) => !graph.modelled.has(id));
  if (granted.length) {
    throw new SearchError(400, `node id(s) ${ids(granted)} are granted automatically, so they cannot be required, excluded or grouped`);
  }
  const excluded = new Set(search.mustNotHave ?? []);
  const both = (search.mustHave ?? []).filter((id) => excluded.has(id));
  if (both.length) throw new SearchError(400, `node id(s) ${ids(both)} are both required and excluded`);
  for (const [k, low] of Object.entries(search.rankMin ?? {})) {
    const ranks = graph.ranks.get(Number(k)) ?? 1;
    if (low < 1 || low > ranks) throw new SearchError(400, `node ${k} has ${ranks} rank(s); at least ${low} cannot be asked`);
    const high = search.rankMax?.[k];
    if (high !== undefined && high < low) throw new SearchError(400, `node ${k}: at least ${low} and at most ${high} ranks contradict`);
    if (excluded.has(Number(k))) throw new SearchError(400, `node ${k} is both excluded and given a minimum rank`);
  }
  for (const [k, high] of Object.entries(search.rankMax ?? {})) {
    const ranks = graph.ranks.get(Number(k)) ?? 1;
    if (high < 0 || high >= ranks) throw new SearchError(400, `node ${k} has ${ranks} rank(s); a cap must be below that, from 0`);
  }
  if (points < 1) throw new SearchError(422, "points must be at least 1");
  if (points > graph.slots) {
    throw new SearchError(400, `'${key}' has only ${graph.slots} point slots at level cap ${LEVEL_CAP}; ${points} points cannot be spent in it`);
  }
  return graph;
}

/** Sets and builds at exactly `points`, as plain numbers for display. */
export function countAt(key: string, tree: CounterTree, search: Search, points: number) {
  const graph = validate(key, tree, search, points);
  return {
    sets: Number(countSpread(graph, search, points, false)[points]),
    builds: Number(countSpread(graph, search, points, true)[points]),
  };
}

/** Counts at every point total up to the tree's slots, for trees sharing a pool. */
export function spreadOf(key: string, tree: CounterTree, search: Search) {
  const graph = validate(key, tree, search, 1);
  return {
    sets: countSpread(graph, search, graph.slots, false).map(Number),
    builds: countSpread(graph, search, graph.slots, true).map(Number),
  };
}

/** Every build a search matches, up to `limit`. More than that is refused, as the API did. */
export function listAt(key: string, tree: CounterTree, search: Search, points: number, limit: number) {
  const graph = validate(key, tree, search, points);
  const result = list(graph, search, points, limit);
  if (result.total === 0n) throw new SearchError(400, "no builds match these constraints");
  if (result.capped) {
    throw new SearchError(413, `${result.total.toLocaleString("en-US")} matching selections exceeds the limit of ${limit.toLocaleString("en-US")}. Add constraints or lower the point budget.`);
  }
  return result.builds;
}
