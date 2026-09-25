import type { TreeDetail } from "./api";
import type { Points } from "./loadout";
import type { Search } from "./workspace";

/**
 * The space of whole characters: one build from each tree, every combination.
 *
 * Three facts make this a product rather than anything cleverer:
 *
 *   - class, spec and hero trees have their own point budgets, so what one takes never
 *     limits another;
 *   - a fixed tree contributes exactly one build;
 *   - an open tree contributes every build its search matches;
 *   - a slot can hold two trees' builds side by side -- both hero trees at once -- which
 *     makes that factor a sum: class x spec x (hero A + hero B).
 *
 * **Choice sides are expanded, not guessed.** The enumerator works in *selections*: it says
 * a choice node is taken, not which alternative. A selection with k unpinned choice nodes is
 * really 2^k builds, and the API's `builds` count already counts them that way (verified: a
 * hero tree reports 67 selections and 580 builds; pinning one choice node's side halves the
 * builds that take it). The first export simmed only the 67 and quietly never simmed the
 * other side of any choice node -- the thing a choice node exists to decide.
 */

export interface Variant {
  /** nodeId -> points within this tree. */
  points: Points;
  /** nodeId -> which alternative, 0 or 1, for every choice node taken. */
  choices: Record<string, number>;
  /**
   * The tree this build belongs to, when a slot holds builds from more than one tree -- the
   * hero slot, when both hero trees are searched at once. Absent means the slot's own key.
   */
  tree?: string;
  /** The hero sub-tree a hero build names, which the talent string has to carry. */
  hero?: number | null;
}

export interface Character {
  /** 1-based, and the profileset number: `ttm_0042` is character 42. */
  line: number;
  /** The variant each tree contributed, by tree key. */
  parts: Record<string, Variant>;
  /** Everything merged, as the talent-string encoder wants it. */
  points: Points;
  choices: Record<string, number>;
  /** Which hero sub-tree this character uses, when its hero build says. */
  heroSubTreeId?: number | null;
}

const choiceNodes = (tree: TreeDetail) =>
  new Set(
    tree.nodes
      .filter((n) => n.kind === "choice" && n.entries.length >= 2 && !n.preFilled)
      .map((n) => String(n.nodeId)),
  );

/**
 * Every build one enumerated selection stands for.
 *
 * A pinned side contributes itself; a free one contributes both.
 */
export function expand(tree: TreeDetail, selection: Points, sides: Search["sides"]): Variant[] {
  const choice = choiceNodes(tree);
  const taken = Object.keys(selection).filter((id) => choice.has(id) && selection[id]! > 0);

  let out: Variant[] = [{ points: selection, choices: {} }];
  for (const id of taken) {
    const pinned = sides[id];
    const options = pinned === "a" ? [0] : pinned === "b" ? [1] : [0, 1];
    out = out.flatMap((v) =>
      options.map((side) => ({ points: v.points, choices: { ...v.choices, [id]: side } })),
    );
  }
  return out;
}

/** A fixed tree's single build, with a side for every choice node it takes. */
export function fixedVariant(tree: TreeDetail, points: Points, picks: Record<string, number>): Variant {
  const choice = choiceNodes(tree);
  const choices: Record<string, number> = {};
  for (const id of Object.keys(points)) if (choice.has(id)) choices[id] = picks[id] ?? 0;
  return { points, choices };
}

/**
 * Product of a count per tree, for display.
 *
 * In floating point on purpose. An unconstrained class, spec and hero tree multiply to well
 * past 2^53, and the only honest thing to do with a number that size is to say it is far too
 * large -- exact digits would be both wrong and irrelevant.
 */
export const productOf = (counts: number[]) => counts.reduce((a, b) => a * b, 1);

/**
 * Every character, in a stable order.
 *
 * The order is the trees' order, then each tree's variant order, so the same inputs always
 * number the same characters the same way -- which is what lets a SimulationCraft report,
 * naming `ttm_0042`, be read back against a list rebuilt later.
 */
export function characters(
  trees: { key: string; variants: Variant[] }[],
  limit: number,
): Character[] {
  const out: Character[] = [];
  const walk = (at: number, parts: Record<string, Variant>) => {
    if (out.length >= limit) return;
    if (at === trees.length) {
      const points: Points = {};
      const choices: Record<string, number> = {};
      let heroSubTreeId: number | null | undefined;
      for (const v of Object.values(parts)) {
        Object.assign(points, v.points);
        Object.assign(choices, v.choices);
        if (v.hero !== undefined) heroSubTreeId = v.hero;
      }
      out.push({ line: out.length + 1, parts: { ...parts }, points, choices, heroSubTreeId });
      return;
    }
    const tree = trees[at]!;
    for (const variant of tree.variants) {
      walk(at + 1, { ...parts, [variant.tree ?? tree.key]: variant });
      if (out.length >= limit) return;
    }
  };
  walk(0, {});
  return out;
}

/**
 * The identities a character carries, for asking what each one was worth.
 *
 * Every talent taken is keyed by its node. A choice node is *also* keyed by the side taken,
 * because its two alternatives are two different talents: the node key answers "is spending
 * a point here worth it", the side keys answer "and on which of the two".
 */
export function traits(character: Character): string[] {
  const out: string[] = [];
  for (const [id, points] of Object.entries(character.points)) {
    if (!points) continue;
    out.push(id);
    const side = character.choices[id];
    if (side !== undefined) out.push(`${id}:${side}`);
  }
  return out;
}

/** Short, readable, and a number format a profileset name can carry. */
export const lineName = (line: number, width: number) =>
  `ttm_${String(line).padStart(width, "0")}`;

/** Big counts in words a person can compare: 9,216 / 1.4 million / 3.2e21. */
export function formatCount(n: number): string {
  if (!Number.isFinite(n)) return "∞";
  if (n < 1e6) return Math.round(n).toLocaleString("en-US");
  if (n < 1e9) return `${(n / 1e6).toFixed(n < 1e7 ? 1 : 0)} million`;
  if (n < 1e12) return `${(n / 1e9).toFixed(n < 1e10 ? 1 : 0)} billion`;
  if (n < 1e15) return `${(n / 1e12).toFixed(n < 1e13 ? 1 : 0)} trillion`;
  const exp = Math.floor(Math.log10(n));
  return `${(n / 10 ** exp).toFixed(1)}e${exp}`;
}
