import type { ChoiceSide, Constraints, TalentNode, TreeDetail } from "./api";
import * as loadout from "./loadout";
import type { NodeState } from "../components/TalentNode";

/**
 * What a player is doing to each of the three trees.
 *
 * Every tree is independently either **fixed** -- points spent by hand, one build -- or
 * **open** -- a budget and constraints, a set of builds. The whole character space is then
 * the product of the three, because a character is a class build, a spec build and a hero
 * build, and the trees' point budgets do not interact.
 *
 * That replaces two earlier designs, both of which were wrong in ways that made the tool
 * hard to use:
 *
 *   - One tree searched at a time, with the other two supplied by a hand-built "baseline".
 *     The solver works one tree at a time, and I let that engine detail become the product:
 *     exporting a set of spec builds required building a class and hero tree first, in a
 *     different mode.
 *   - Two global modes, Build and Explore, owning disjoint state. Switching mode made
 *     everything painted in the other one vanish from the screen.
 *
 * Here nothing is ever thrown away. A tree keeps its hand-spent points *and* its search
 * whichever it is currently using, so flipping a tree from fixed to open and back returns
 * exactly what was there.
 */

export type TreeMode = "fixed" | "open";

/** What a click on an open tree does. */
export type Tool = "toggle" | "atLeastOne" | "exactlyOne";

export interface Search {
  /** Points to spend in this tree. Null follows the tree's cap, which is almost always wanted. */
  budget: number | null;
  required: number[];
  excluded: number[];
  /** Choice nodes pinned to one alternative. A pinned side implies the node is taken. */
  sides: Record<string, Exclude<ChoiceSide, "none">>;
  atLeastOne: number[];
  exactlyOne: number[];
}

export interface TreeWork {
  mode: TreeMode;
  /** The hand-built build: nodeId -> points. */
  points: loadout.Points;
  /** Which alternative each taken choice node uses in the hand-built build: 0 or 1. */
  picks: Record<string, number>;
  search: Search;
}

export const EMPTY_SEARCH: Search = {
  budget: null,
  required: [],
  excluded: [],
  sides: {},
  atLeastOne: [],
  exactlyOne: [],
};

export const emptyWork = (mode: TreeMode = "open"): TreeWork => ({
  mode,
  points: {},
  picks: {},
  search: EMPTY_SEARCH,
});

export const capOf = (tree: { pointCap: number | null; maxPointsInTree: number } | null) =>
  tree ? (tree.pointCap ?? tree.maxPointsInTree) : 0;

export const budgetOf = (work: TreeWork, cap: number) =>
  Math.min(cap, work.search.budget ?? cap);

const without = (list: number[], id: number) => list.filter((x) => x !== id);
const toggled = (list: number[], id: number) =>
  list.includes(id) ? without(list, id) : [...list, id];

const isChoice = (node: TalentNode) => node.kind === "choice" && node.entries.length >= 2;

/**
 * Paint a constraint onto an open tree.
 *
 * Clicking cycles neutral -> required -> barred -> neutral; right-click or shift goes the other
 * way, so a mis-click is one more click rather than a trip round the cycle. A choice node
 * cycles its *side* instead -- left, right, either -- because a choice node's interesting
 * property is which alternative, not whether. A group tool adds or removes the node from
 * that group and clears any individual constraint on it, since the group already says what
 * should happen to it.
 */
export function paint(work: TreeWork, node: TalentNode, tool: Tool, alternate: boolean): TreeWork {
  const id = node.nodeId;
  const s = work.search;

  if (tool !== "toggle") {
    const key = tool === "atLeastOne" ? "atLeastOne" : "exactlyOne";
    const other = tool === "atLeastOne" ? "exactlyOne" : "atLeastOne";
    return {
      ...work,
      search: {
        ...s,
        [key]: toggled(s[key], id),
        [other]: without(s[other], id),
        required: without(s.required, id),
        excluded: without(s.excluded, id),
      },
    };
  }

  if (isChoice(node)) {
    // free -> left -> right -> barred -> free, or the reverse. Pinning a side means taking
    // the node, so one cycle covers every state a choice node can be in.
    const cycle = ["free", "a", "b", "barred"] as const;
    const current = s.excluded.includes(id) ? "barred" : (s.sides[String(id)] ?? "free");
    const next = cycle[(cycle.indexOf(current) + (alternate ? 3 : 1)) % 4]!;
    const sides = { ...s.sides };
    delete sides[String(id)];
    if (next === "a" || next === "b") sides[String(id)] = next;
    return {
      ...work,
      search: {
        ...s,
        sides,
        excluded: next === "barred" ? [...without(s.excluded, id), id] : without(s.excluded, id),
        required: without(s.required, id),
        atLeastOne: without(s.atLeastOne, id),
        exactlyOne: without(s.exactlyOne, id),
      },
    };
  }

  const state = s.required.includes(id) ? 1 : s.excluded.includes(id) ? 2 : 0;
  const landing = (((state + (alternate ? -1 : 1)) % 3) + 3) % 3;
  return {
    ...work,
    search: {
      ...s,
      required: landing === 1 ? [...without(s.required, id), id] : without(s.required, id),
      excluded: landing === 2 ? [...without(s.excluded, id), id] : without(s.excluded, id),
      atLeastOne: without(s.atLeastOne, id),
      exactlyOne: without(s.exactlyOne, id),
    },
  };
}

/**
 * Spend or refund a point on a fixed tree.
 *
 * Left-click adds, right-click refunds, as in the game. A second click on a choice node that
 * is already taken swaps which alternative it uses, since the point is spent either way.
 */
export function spend(
  work: TreeWork,
  tree: TreeDetail,
  cap: number,
  node: TalentNode,
  refund: boolean,
): { work: TreeWork; note: string | null } {
  const id = String(node.nodeId);
  if (refund) {
    const result = loadout.remove(tree, work.points, cap, node);
    const lost = Object.keys(result.dropped).length;
    const picks = { ...work.picks };
    for (const dropped of [id, ...Object.keys(result.dropped)]) {
      if (!result.points[dropped]) delete picks[dropped];
    }
    return {
      work: { ...work, points: result.points, picks },
      note: lost
        ? `Removing ${node.name} also refunded ${lost} talent${lost === 1 ? "" : "s"} below it.`
        : null,
    };
  }
  if (isChoice(node) && (work.points[id] ?? 0) > 0) {
    return {
      work: { ...work, picks: { ...work.picks, [id]: ((work.picks[id] ?? 0) + 1) % 2 } },
      note: null,
    };
  }
  const result = loadout.add(tree, work.points, cap, node);
  return { work: { ...work, points: result.points }, note: result.reason ?? null };
}

/** The constraints this tree sends to the API. */
export function payloadOf(work: TreeWork, cap: number): Constraints {
  const s = work.search;
  const body: Constraints = { points: budgetOf(work, cap) };
  if (s.required.length) body.mustHave = [...s.required];
  if (s.excluded.length) body.mustNotHave = [...s.excluded];
  if (Object.keys(s.sides).length) body.choiceSides = { ...s.sides };
  // A group of one is not a group -- the API refuses it, correctly, since it nearly always
  // means the second talent has not been clicked yet.
  if (s.atLeastOne.length > 1) body.atLeastOneOf = [[...s.atLeastOne]];
  if (s.exactlyOne.length > 1) body.exactlyOneOf = [[...s.exactlyOne]];
  return body;
}

/** A group with one member, which is a constraint still being written. */
export function pendingOf(work: TreeWork): string[] {
  const out: string[] = [];
  if (work.search.atLeastOne.length === 1) out.push("the at-least-one group needs a second talent");
  if (work.search.exactlyOne.length === 1) out.push("the exactly-one group needs a second talent");
  return out;
}

export const constraintCount = (work: TreeWork) =>
  work.search.required.length +
  work.search.excluded.length +
  Object.keys(work.search.sides).length +
  work.search.atLeastOne.length +
  work.search.exactlyOne.length;

/** How each node of an open tree is drawn. */
export function statesOf(work: TreeWork): Map<number, NodeState> {
  const map = new Map<number, NodeState>();
  if (work.mode !== "open") return map;
  for (const id of work.search.required) map.set(id, "required");
  for (const id of work.search.excluded) map.set(id, "excluded");
  for (const id of work.search.atLeastOne) map.set(id, "anyOf");
  for (const id of work.search.exactlyOne) map.set(id, "oneOf");
  // A pinned side takes the node, so it reads as required; the dimmed half says which side.
  for (const id of Object.keys(work.search.sides)) map.set(Number(id), "required");
  return map;
}

/** Which half of each choice node is dimmed. */
export function sidesOf(work: TreeWork): Map<number, "a" | "b" | "none"> {
  if (work.mode === "open") {
    return new Map(Object.entries(work.search.sides).map(([id, side]) => [Number(id), side]));
  }
  return new Map(
    Object.entries(work.picks).map(([id, side]) => [Number(id), side === 1 ? "b" : "a"]),
  );
}

/**
 * The fixed build as the canvas draws it: granted talents included.
 *
 * They cost no point and so are absent from the spend map, but the character has them, and
 * drawing a granted talent as an empty socket is simply wrong.
 */
export function drawnBuild(work: TreeWork, tree: TreeDetail | null): loadout.Points {
  if (!tree) return work.points;
  const out: loadout.Points = { ...work.points };
  for (const id of loadout.grantedRoots(tree)) {
    const node = tree.nodes.find((n) => n.nodeId === id);
    out[String(id)] = node?.maxPoints ?? 1;
  }
  return out;
}

/** Turn a build into a fixed tree -- used by imports and by "use this build". */
export function fixedFrom(
  work: TreeWork,
  tree: TreeDetail,
  cap: number,
  points: loadout.Points,
  picks: Record<string, number>,
): TreeWork {
  const placed = loadout.place(tree, points, cap).points;
  const ids = new Set(tree.nodes.map((n) => String(n.nodeId)));
  const mine: Record<string, number> = {};
  for (const [id, side] of Object.entries(picks)) if (ids.has(id) && placed[id]) mine[id] = side;
  return { ...work, mode: "fixed", points: placed, picks: mine };
}
