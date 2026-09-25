import { EMPTY_SEARCH, emptyWork, type Search, type TreeWork } from "./workspace";

/**
 * The whole workspace, encoded into the URL.
 *
 * The point of rebuilding this as a web app was that the tool should be a URL, and that only
 * means something if a link reproduces what the sender was looking at: every tree, whether it
 * is fixed or open, the build spent in it and the search painted on it.
 *
 * **All three trees, both halves of each.** An earlier codec carried either a loadout or a
 * search depending on a global mode, which made a link lose whichever half its sender was not
 * looking at. Now each tree carries its fixed build and its search, and a flag saying which
 * one is in use, so a link loses nothing.
 *
 * **Node ids, never positions.** A shared link is a stored build that happens to live in
 * someone's chat history, and it outlives more data revisions than a database row does. The
 * legacy format stored assignments positionally and a regenerated preset reassigned people's
 * points to different talents; ids cannot do that. They are written in base36 to keep links
 * short -- 88203 becomes "1txf" -- which is a transport encoding, not a substitute.
 *
 * Parameters, with `c`, `s`, `h` or `g` standing for the class, spec, hero or other hero tree:
 *
 *   t   the spec tree's key, which also names the class      h   the hero tree's key
 *   h2  the other hero tree's key, when it carries state     both  sim both hero trees
 *   ?m  f when the tree is fixed (absent means open)          l   the sim limit, if changed
 *   ?b  the fixed build            ?k  its choice sides
 *   ?p  the search's point budget  ?r ?x  required, barred
 *   ?s  pinned choice sides        ?o ?e  at-least-one, exactly-one groups
 */

/** `hero2` is the hero tree not currently shown -- kept, since both can be simmed at once. */
export type Role = "class" | "spec" | "hero" | "hero2";
const PREFIX: Record<Role, string> = { class: "c", spec: "s", hero: "h", hero2: "g" };
export const ROLES: Role[] = ["class", "spec", "hero", "hero2"];

export interface Shared {
  /** The spec tree's key; the class and the class tree follow from it. */
  spec: string | null;
  hero: string | null;
  /** The other hero tree, named only when it has state worth carrying. */
  hero2: string | null;
  /** Sim both hero trees together. */
  both: boolean;
  work: Partial<Record<Role, TreeWork>>;
  limit: number | null;
}

export const NOTHING: Shared = { spec: null, hero: null, hero2: null, both: false, work: {}, limit: null };

const b36 = (id: number) => id.toString(36);
const unb36 = (text: string) => Number.parseInt(text, 36);

const ids = (list: number[]) => list.map(b36).join("-");
const parseIds = (text: string | null): number[] =>
  (text ?? "")
    .split("-")
    .filter(Boolean)
    .map(unb36)
    .filter((id) => Number.isFinite(id) && id > 0);

/**
 * `<base36 id><digit>`, joined by `-`: a build's points, or a choice's side.
 *
 * One digit because no talent has ten ranks, and fused to the id so a thirty-talent build
 * needs no second separator.
 */
const pairs = (map: Record<string, number>) =>
  Object.entries(map)
    .filter(([, v]) => Number.isFinite(v) && v >= 0)
    .map(([id, v]) => `${b36(Number(id))}${Math.min(9, v)}`)
    .join("-");

const parsePairs = (text: string | null, keepZero = false): Record<string, number> => {
  const out: Record<string, number> = {};
  for (const token of (text ?? "").split("-")) {
    if (token.length < 2) continue;
    const id = unb36(token.slice(0, -1));
    const value = Number(token.slice(-1));
    if (Number.isFinite(id) && id > 0 && Number.isFinite(value) && (keepZero || value > 0)) {
      out[String(id)] = value;
    }
  }
  return out;
};

const sidePairs = (sides: Search["sides"]) =>
  Object.entries(sides)
    .map(([id, side]) => `${b36(Number(id))}${side}`)
    .join("-");

const parseSides = (text: string | null): Search["sides"] => {
  const out: Search["sides"] = {};
  for (const token of (text ?? "").split("-")) {
    const side = token.slice(-1);
    const id = unb36(token.slice(0, -1));
    if ((side === "a" || side === "b") && Number.isFinite(id) && id > 0) out[String(id)] = side;
  }
  return out;
};

export function encode(state: Shared): string {
  const params = new URLSearchParams();
  if (state.spec) params.set("t", state.spec);
  if (state.hero) params.set("h", state.hero);
  if (state.hero2 && state.work.hero2) params.set("h2", state.hero2);
  if (state.both) params.set("both", "1");
  if (state.limit) params.set("l", String(state.limit));

  for (const role of ROLES) {
    const work = state.work[role];
    if (!work || (role === "hero2" && !state.hero2)) continue;
    const p = PREFIX[role];
    const s = work.search;
    if (work.mode === "fixed") params.set(`${p}m`, "f");
    if (Object.keys(work.points).length) params.set(`${p}b`, pairs(work.points));
    if (Object.keys(work.picks).length) params.set(`${p}k`, pairs(work.picks));
    if (s.budget) params.set(`${p}p`, String(s.budget));
    if (s.required.length) params.set(`${p}r`, ids(s.required));
    if (s.excluded.length) params.set(`${p}x`, ids(s.excluded));
    if (Object.keys(s.sides).length) params.set(`${p}s`, sidePairs(s.sides));
    if (s.atLeastOne.length) params.set(`${p}o`, ids(s.atLeastOne));
    if (s.exactlyOne.length) params.set(`${p}e`, ids(s.exactlyOne));
  }

  const text = params.toString();
  return text ? `?${text}` : "";
}

/** The role a tree key names: `.../class`, `.../spec`, `.../hero/31`. */
export function roleOfKey(key: string): Role | null {
  if (/\/class$/.test(key)) return "class";
  if (/\/spec$/.test(key)) return "spec";
  if (/\/hero\/\d+$/.test(key)) return "hero";
  return null;
}

/** The spec tree's key, from any tree of the same specialisation. */
export const specKeyOf = (key: string) =>
  key.replace(/\/(class|hero\/\d+)$/, "/spec");

export function decode(search: string): Shared {
  const params = new URLSearchParams(search);
  const t = params.get("t");
  const out: Shared = {
    spec: t ? specKeyOf(t) : null,
    hero: params.get("h"),
    hero2: params.get("h2"),
    both: params.get("both") === "1",
    work: {},
    limit: Number(params.get("l")) > 0 ? Number(params.get("l")) : null,
  };

  for (const role of ROLES) {
    const p = PREFIX[role];
    const has = [..."mbkprxsoe"].some((k) => params.has(`${p}${k}`));
    if (!has) continue;
    const budget = Number(params.get(`${p}p`));
    out.work[role] = {
      mode: params.get(`${p}m`) === "f" ? "fixed" : "open",
      points: parsePairs(params.get(`${p}b`)),
      picks: parsePairs(params.get(`${p}k`), true),
      search: {
        budget: Number.isFinite(budget) && budget > 0 ? budget : null,
        required: parseIds(params.get(`${p}r`)),
        excluded: parseIds(params.get(`${p}x`)),
        sides: parseSides(params.get(`${p}s`)),
        atLeastOne: parseIds(params.get(`${p}o`)),
        exactlyOne: parseIds(params.get(`${p}e`)),
      },
    };
  }

  legacy(params, t, out);
  return out;
}

/**
 * Links from before the per-tree workspace still open.
 *
 * They carried a loadout (`bc`/`bs`/`bh`) or one tree's search (`p`, `r`, `x`, `s`, `o`, `e`
 * on the tree named by `t`). Both map cleanly onto the new model -- a loadout is three fixed
 * trees, a search is one open one -- so an old link lands on the same screen it described.
 */
function legacy(params: URLSearchParams, t: string | null, out: Shared) {
  const builds: [Role, string][] = [
    ["class", "bc"],
    ["spec", "bs"],
    ["hero", "bh"],
  ];
  for (const [role, key] of builds) {
    if (out.work[role] || !params.has(key)) continue;
    out.work[role] = { ...emptyWork("fixed"), points: parsePairs(params.get(key)) };
  }

  const role = t ? roleOfKey(t) : null;
  const legacySearch = ["p", "r", "x", "s", "o", "e"].some((k) => params.has(k));
  if (!role || !legacySearch || out.work[role]?.mode === "open") return;

  const sides: Search["sides"] = {};
  for (const token of (params.get("s") ?? "").split("-")) {
    const side = token.slice(-1);
    const id = unb36(token.slice(0, -1));
    if ((side === "a" || side === "b") && id > 0) sides[String(id)] = side;
  }
  const budget = Number(params.get("p"));
  out.work[role] = {
    ...(out.work[role] ?? emptyWork()),
    mode: "open",
    search: {
      ...EMPTY_SEARCH,
      budget: budget > 0 ? budget : null,
      required: parseIds(params.get("r")),
      excluded: parseIds(params.get("x")),
      sides,
      atLeastOne: parseIds(params.get("o")),
      exactlyOne: parseIds(params.get("e")),
    },
  };
}

/** Keep the address bar current, replacing rather than pushing: painting is not navigation. */
export function syncUrl(state: Shared): void {
  const next = `${window.location.pathname}${encode(state)}`;
  if (next !== `${window.location.pathname}${window.location.search}`) {
    window.history.replaceState(null, "", next);
  }
}
