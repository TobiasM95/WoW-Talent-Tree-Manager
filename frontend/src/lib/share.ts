import type { ChoiceSide } from "./api";

/**
 * The whole view, encoded into the URL.
 *
 * The point of rebuilding this as a web app was that the tool should be a URL. That only
 * means something if the URL carries what you are looking at -- the tree, the constraints
 * you painted, and the build you are inspecting -- so a link reproduces the screen rather
 * than the front page.
 *
 * **Node ids, never positions.** This is the same rule the storage schema enforces, and for
 * the same reason: the legacy format stored point assignments positionally and the preset
 * generator re-indexed nodes on every regeneration, so a data update silently reassigned
 * someone's points to different talents. A shared link is a stored build that happens to
 * live in someone's chat history, and it outlives more revisions than a database row does.
 *
 * Ids are base36 to keep links short -- 88203 becomes "1txf" -- which is a transport
 * encoding of the id, not a substitute for it.
 */

export interface ShareState {
  tree: string | null;
  points: number | null;
  required: number[];
  excluded: number[];
  sides: Map<number, ChoiceSide>;
  atLeastOne: number[];
  exactlyOne: number[];
  /** A specific enumerated build being inspected on the active tree: nodeId -> points. */
  build: Record<string, number> | null;
  /**
   * Points spent by hand, per tree.
   *
   * Three separate fields rather than one keyed by tree name, because a loadout spans class,
   * specialisation and hero and a link has to carry all three -- a class build without its
   * spec is not a build anyone meant to share. `hero` names which hero tree, since a spec
   * has two and the points alone would not say which.
   */
  spent: { class: Record<string, number> | null;
           spec: Record<string, number> | null;
           hero: Record<string, number> | null };
  heroKey: string | null;
  /**
   * Which mode the link opens in.
   *
   * Carried explicitly rather than inferred from which fields are present. A link can hold
   * both a hand-built loadout and an enumerated build, and guessing from that produced a
   * link that reopened in build mode showing none of the result it was sharing. The mode is
   * part of what the person was looking at, so it is part of the state.
   */
  mode: "build" | "explore" | null;
}

export const EMPTY: ShareState = {
  tree: null,
  points: null,
  required: [],
  excluded: [],
  sides: new Map(),
  atLeastOne: [],
  exactlyOne: [],
  build: null,
  spent: { class: null, spec: null, hero: null },
  heroKey: null,
  mode: null,
};

const b36 = (id: number) => id.toString(36);
const unb36 = (text: string) => Number.parseInt(text, 36);

const ids = (list: number[]) => list.map(b36).join("-");
const parseIds = (text: string | null): number[] =>
  (text ?? "")
    .split("-")
    .map(unb36)
    .filter((id) => Number.isFinite(id) && id > 0);

const SIDE_CODE: Record<ChoiceSide, string> = { a: "a", b: "b", none: "n" };
const CODE_SIDE: Record<string, ChoiceSide> = { a: "a", b: "b", n: "none" };

/**
 * A build is `<base36 id><points>` per talent, joined by `-`. Points are a single digit
 * because no talent in the game has ten ranks, and keeping them fused to the id avoids a
 * second separator -- which matters when a link carries thirty of them.
 */
const encodeBuild = (build: Record<string, number>) =>
  Object.entries(build)
    .filter(([, points]) => points > 0)
    .map(([id, points]) => `${b36(Number(id))}${Math.min(9, points)}`)
    .join("-");

const decodeBuild = (text: string | null): Record<string, number> | null => {
  if (!text) return null;
  const build: Record<string, number> = {};
  for (const token of text.split("-")) {
    if (token.length < 2) continue;
    const id = unb36(token.slice(0, -1));
    const points = Number(token.slice(-1));
    if (Number.isFinite(id) && id > 0 && points > 0) build[String(id)] = points;
  }
  return Object.keys(build).length ? build : null;
};

export function encode(state: ShareState): string {
  const params = new URLSearchParams();
  if (state.tree) params.set("t", state.tree);
  if (state.points) params.set("p", String(state.points));
  if (state.required.length) params.set("r", ids(state.required));
  if (state.excluded.length) params.set("x", ids(state.excluded));
  if (state.sides.size) {
    params.set(
      "s",
      [...state.sides].map(([id, side]) => `${b36(id)}${SIDE_CODE[side]}`).join("-"),
    );
  }
  if (state.atLeastOne.length) params.set("o", ids(state.atLeastOne));
  if (state.exactlyOne.length) params.set("e", ids(state.exactlyOne));
  if (state.build) params.set("b", encodeBuild(state.build));
  // Defaulted rather than assumed: this is called with hand-assembled state in places, and
  // a missing field should drop a parameter, not throw and lose the whole link.
  const spent = state.spent ?? EMPTY.spent;
  if (spent.class) params.set("bc", encodeBuild(spent.class));
  if (spent.spec) params.set("bs", encodeBuild(spent.spec));
  if (spent.hero) params.set("bh", encodeBuild(spent.hero));
  if (state.heroKey) params.set("h", state.heroKey);
  if (state.mode) params.set("m", state.mode === "build" ? "b" : "e");
  return params.toString();
}

export function decode(search: string): ShareState {
  const params = new URLSearchParams(search);
  const points = Number(params.get("p"));

  const sides = new Map<number, ChoiceSide>();
  for (const token of (params.get("s") ?? "").split("-")) {
    if (token.length < 2) continue;
    const id = unb36(token.slice(0, -1));
    const side = CODE_SIDE[token.slice(-1)];
    if (Number.isFinite(id) && id > 0 && side) sides.set(id, side);
  }

  return {
    // A tree key contains slashes ("retail/11/102/spec"); URLSearchParams handles the
    // escaping in both directions, so it is stored as-is rather than mangled into a
    // custom format that would then need its own parser.
    tree: params.get("t"),
    points: Number.isFinite(points) && points > 0 ? points : null,
    required: parseIds(params.get("r")),
    excluded: parseIds(params.get("x")),
    sides,
    atLeastOne: parseIds(params.get("o")),
    exactlyOne: parseIds(params.get("e")),
    build: decodeBuild(params.get("b")),
    spent: {
      class: decodeBuild(params.get("bc")),
      spec: decodeBuild(params.get("bs")),
      hero: decodeBuild(params.get("bh")),
    },
    heroKey: params.get("h"),
    mode: params.get("m") === "b" ? "build" : params.get("m") === "e" ? "explore" : null,
  };
}

/**
 * Replace the address bar without adding a history entry.
 *
 * Painting constraints is a continuous gesture -- a dozen clicks while narrowing a search --
 * and pushing each one would turn the back button into an undo stepper for something the
 * user does not think of as navigation. The URL stays current so it can be copied at any
 * moment; it just does not accumulate.
 */
export function syncUrl(state: ShareState): void {
  const query = encode(state);
  const next = `${window.location.pathname}${query ? `?${query}` : ""}`;
  if (next !== window.location.pathname + window.location.search) {
    window.history.replaceState(null, "", next);
  }
}
