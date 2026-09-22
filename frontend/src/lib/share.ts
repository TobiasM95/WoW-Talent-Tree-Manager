import type { ChoiceSide } from "./api";

/**
 * The whole view, encoded into the URL.
 *
 * The point of rebuilding this as a web app was that the tool should be a URL. That only
 * means something if a link reproduces the screen rather than the front page.
 *
 * There are two things worth sharing, and they are not the same thing:
 *
 *   **a loadout** -- points spent by hand across all three trees, which is a build;
 *   **a search** -- a tree, a budget and the constraints painted on it, which is a question.
 *
 * A link carries one or the other, chosen by the mode. Carrying both produced links that made
 * no sense: a shared loadout arrived with a leftover point budget and a possibility space
 * narrowed by constraints its sender never meant to send.
 *
 * An *enumerated* build is deliberately not shareable on its own. It only exists relative to
 * a job that has since gone, and a link to one would be a loadout wearing someone else's
 * context. Taking one into the loadout is an explicit act, and then it shares as a loadout.
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
   * It decides which half of the state is written, so it cannot be inferred from which
   * fields are present -- that reasoning is circular. It is also part of what the person was
   * looking at, which makes it part of what a link should reproduce.
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

/**
 * Only the fields the mode actually means.
 *
 * A link used to carry a hand-built loadout, a set of constraints and an inspected build all
 * at once, because every write went through one function that wrote everything it had. What
 * came back was incoherent: a shared loadout arrived with a point budget left over from some
 * earlier search, and the possibility space it showed was narrowed by constraints the sender
 * had never meant to send.
 *
 * The two modes are two different things to share. Build shares a *loadout*; explore shares a
 * *search*. Writing only one of them is what makes a link mean something.
 */
export function encode(state: ShareState): string {
  const params = new URLSearchParams();
  const building = state.mode === "build";

  if (state.tree) params.set("t", state.tree);
  if (!building) {
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
  }
  // Defaulted rather than assumed: this is called with hand-assembled state in places, and
  // a missing field should drop a parameter, not throw and lose the whole link.
  const spent = building ? (state.spent ?? EMPTY.spent) : EMPTY.spent;
  if (spent.class) params.set("bc", encodeBuild(spent.class));
  if (spent.spec) params.set("bs", encodeBuild(spent.spec));
  if (spent.hero) params.set("bh", encodeBuild(spent.hero));
  // Which hero tree is showing is true in both modes, so it is written in both.
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
