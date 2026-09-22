import type { TalentNode, TreeDetail } from "./api";

/**
 * Spending points by hand, under the same rules the solver counts under.
 *
 * This is the property that matters: a loadout built here must be one of the builds the
 * counter counts. If the two disagree, the tool tells a player they have made something the
 * tool itself says does not exist. So the rules are lifted from the counting DP rather than
 * from a reading of the game, and the test suite asserts the agreement by asking the API to
 * count a hand-built loadout.
 *
 * The rules, from `tools/frontier-dp/frontier_dp.py`:
 *
 *   1. total spent so far is below the tree's cap;
 *   2. total spent so far is at least the node's `pointsRequired` -- the row gate, measured
 *      *before* this point is placed;
 *   3. the first point in a node needs a way in: the node is a root, or some parent is
 *      **fully ranked**. Not merely taken -- the DP attaches a node's children to its *last*
 *      rank, so a two-rank talent gates its children until both points are in it.
 *      Later points in the same node need nothing further, since the ranks form a chain.
 *
 * A pre-filled root is granted rather than chosen: it costs no point and its children start
 * as roots. Modelled here by treating it as permanently satisfied.
 */

export type Points = Record<string, number>;

export interface Placement {
  /** The part of the request that could actually be built. */
  points: Points;
  /** Total points it costs. */
  spent: number;
  /** Nodes whose requested points could not be placed, and how many were lost. */
  dropped: Points;
  /**
   * The node ids in the order the points went in, one entry per point.
   *
   * Exposed so the rules can be checked rather than believed: replaying this order lets a
   * test assert that at the moment each point was placed the gate was met and the node had
   * a way in. Without it the only testable claim is "the result looks plausible".
   */
  order: number[];
}

interface Rules {
  byId: Map<number, TalentNode>;
  /** Pre-filled roots are granted, so they satisfy a child's "way in" for free. */
  granted: Set<number>;
}

function rulesFor(tree: TreeDetail): Rules {
  const byId = new Map(tree.nodes.map((n) => [n.nodeId, n]));
  const granted = new Set<number>();
  // A pre-filled root is removed from the graph and its children promoted to roots. Iterated
  // to a fixed point, because a promoted child can itself be pre-filled.
  let changed = true;
  while (changed) {
    changed = false;
    for (const node of tree.nodes) {
      if (granted.has(node.nodeId) || !node.preFilled) continue;
      const rooted = node.parents.length === 0 || node.parents.every((p) => granted.has(p));
      if (rooted) {
        granted.add(node.nodeId);
        changed = true;
      }
    }
  }
  return { byId, granted };
}

/**
 * Build as much of `wanted` as the rules allow, and report what was lost.
 *
 * Greedy, and exact because of it: placing a point never *removes* an option. The spend only
 * grows, so gates that were open stay open, and a node that was fully ranked stays fully
 * ranked. So if any order can place a point, placing whatever is available now cannot
 * prevent it.
 */
export function place(tree: TreeDetail, wanted: Points, cap: number): Placement {
  const { byId, granted } = rulesFor(tree);
  const points: Points = {};
  const order: number[] = [];
  let spent = 0;

  const have = (id: number) => points[String(id)] ?? 0;
  const maxed = (id: number) => {
    if (granted.has(id)) return true;
    const node = byId.get(id);
    return node ? have(id) >= node.maxPoints : false;
  };

  let progress = true;
  while (progress && spent < cap) {
    progress = false;
    for (const node of tree.nodes) {
      const key = String(node.nodeId);
      const asked = Math.min(wanted[key] ?? 0, node.maxPoints);
      const current = points[key] ?? 0;
      if (current >= asked || spent >= cap) continue;
      if (spent < node.pointsRequired) continue;
      const wayIn =
        current > 0 || node.parents.length === 0 || node.parents.some((p) => maxed(p));
      if (!wayIn) continue;
      points[key] = current + 1;
      order.push(node.nodeId);
      spent += 1;
      progress = true;
    }
  }

  const dropped: Points = {};
  for (const [key, asked] of Object.entries(wanted)) {
    const node = byId.get(Number(key));
    const capped = Math.min(asked, node?.maxPoints ?? 0);
    const lost = capped - (points[key] ?? 0);
    if (lost > 0) dropped[key] = lost;
  }
  return { points, spent, dropped, order };
}

/**
 * Pre-filled roots, which are granted rather than chosen. Exported so a test can replay a
 * placement order under the same notion of "already satisfied" the placer used.
 */
export const grantedRoots = (tree: TreeDetail): Set<number> => rulesFor(tree).granted;

/** Whether one more point can go into this node right now, given what is already spent. */
export function canAdd(tree: TreeDetail, points: Points, cap: number, node: TalentNode): boolean {
  const next = { ...points, [String(node.nodeId)]: (points[String(node.nodeId)] ?? 0) + 1 };
  const result = place(tree, next, cap);
  return (result.points[String(node.nodeId)] ?? 0) > (points[String(node.nodeId)] ?? 0);
}

/**
 * Add a point, or report why not.
 *
 * Returning the reason rather than silently doing nothing: a click that does nothing is the
 * most confusing possible response, and the three reasons are all things a player can act on.
 */
export function add(
  tree: TreeDetail,
  points: Points,
  cap: number,
  node: TalentNode,
): { points: Points; reason?: string } {
  const key = String(node.nodeId);
  const current = points[key] ?? 0;
  const spent = total(points);

  if (current >= node.maxPoints) return { points, reason: `${node.name} is already maxed.` };
  if (spent >= cap) return { points, reason: `No points left — the cap is ${cap}.` };
  if (spent < node.pointsRequired) {
    return {
      points,
      reason: `${node.name} needs ${node.pointsRequired} points spent in this tree; you have ${spent}.`,
    };
  }
  const next = { ...points, [key]: current + 1 };
  const result = place(tree, next, cap);
  if ((result.points[key] ?? 0) <= current) {
    return { points, reason: `${node.name} is not connected to anything you have taken.` };
  }
  return { points: result.points };
}

/**
 * Remove a point, dropping anything that then has no way in.
 *
 * Cascading rather than refusing. Taking a point out of the middle of a tree can strand
 * everything below it, and a tool that answers "no" leaves the player to work out which of
 * thirty talents is the obstacle. Re-placing what remains says instead: here is what your
 * build becomes.
 */
export function remove(
  tree: TreeDetail,
  points: Points,
  cap: number,
  node: TalentNode,
): { points: Points; dropped: Points } {
  const key = String(node.nodeId);
  const current = points[key] ?? 0;
  if (current <= 0) return { points, dropped: {} };

  const next = { ...points };
  if (current === 1) delete next[key];
  else next[key] = current - 1;

  const result = place(tree, next, cap);
  return { points: result.points, dropped: result.dropped };
}

export const total = (points: Points): number =>
  Object.values(points).reduce((sum, n) => sum + n, 0);

/** Every node a point could legally go into right now, for highlighting what is reachable. */
export function available(tree: TreeDetail, points: Points, cap: number): Set<number> {
  const { byId, granted } = rulesFor(tree);
  const spent = total(points);
  const open = new Set<number>();
  if (spent >= cap) return open;

  const have = (id: number) => points[String(id)] ?? 0;
  const maxed = (id: number) => {
    if (granted.has(id)) return true;
    const parent = byId.get(id);
    return parent ? have(id) >= parent.maxPoints : false;
  };

  for (const node of tree.nodes) {
    const current = have(node.nodeId);
    if (current >= node.maxPoints) continue;
    if (spent < node.pointsRequired) continue;
    if (current > 0 || node.parents.length === 0 || node.parents.some((p) => maxed(p))) {
      open.add(node.nodeId);
    }
  }
  return open;
}
