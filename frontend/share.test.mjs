/**
 * The share codec: every tree, both halves of each, and links from before the redesign.
 *
 *   node share.test.mjs
 */
import { createServer } from "vite";

const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const S = await server.ssrLoadModule("/src/lib/share.ts");
const W = await server.ssrLoadModule("/src/lib/workspace.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);

const classWork = {
  ...W.emptyWork("fixed"),
  points: { 76061: 1, 76062: 2, 96167: 1 },
  picks: { 76062: 1 },
  // A search is kept even while the tree is fixed, which is the point of the redesign.
  search: { ...W.EMPTY_SEARCH, required: [76070] },
};
const specWork = {
  ...W.emptyWork("open"),
  search: {
    budget: 27,
    required: [96200, 96201],
    excluded: [96210],
    sides: { 96220: "b", 96221: "a" },
    atLeastOne: [96230, 96231],
    exactlyOne: [96240, 96241, 96242],
    ranks: { 96200: { min: 2, max: null }, 96250: { min: 0, max: 1 } },
  },
};
const heroWork = { ...W.emptyWork("open"), search: { ...W.EMPTY_SEARCH, budget: 8 } };

const state = {
  spec: "retail/6/250/spec",
  hero: "retail/6/250/hero/31",
  limit: 5000,
  work: { class: classWork, spec: specWork, hero: heroWork },
};

const text = S.encode(state);

const bothState = {
  ...state,
  hero2: "retail/6/250/hero/33",
  both: true,
  work: { ...state.work, hero2: { ...W.emptyWork("open"), search: { ...W.EMPTY_SEARCH, budget: 9, required: [95999] } } },
};
const both = S.decode(S.encode(bothState));
check("both hero trees: the flag survives", both.both === true);
check("and the other hero tree with its own search", both.hero2 === "retail/6/250/hero/33" &&
  both.work.hero2?.search.budget === 9 && same(both.work.hero2.search.required, [95999]));
check("a link without it is one hero tree", S.decode(text).both === false && S.decode(text).hero2 === null);
const back = S.decode(text);

check("the spec and hero trees survive", back.spec === state.spec && back.hero === state.hero);
check("the sim limit survives", back.limit === 5000);
check("a fixed tree stays fixed", back.work.class.mode === "fixed");
check("with its points", same(back.work.class.points, classWork.points), JSON.stringify(back.work.class.points));
check("and its choice sides, including side 0", same(back.work.class.picks, classWork.picks));
check("and the search it is not using", same(back.work.class.search.required, [76070]));
check("an open tree stays open", back.work.spec.mode === "open");
check("its budget survives", back.work.spec.search.budget === 27);
check("every constraint kind survives", same(back.work.spec.search, specWork.search), JSON.stringify(back.work.spec.search));
check("a budget-only tree survives", back.work.hero.search.budget === 8 && back.work.hero.mode === "open");
console.log(`     (${text.length} characters for three trees)`);

// An open tree with nothing painted and the default budget still says it is open: in a game
// whose trees start fixed, silence would read back as fixed.
const bare = S.decode(S.encode({ ...state, work: { hero: W.emptyWork("open") } }));
check("an open tree with nothing on it still reads back open", bare.work.hero?.mode === "open");

const empty = S.decode("");
check("an empty link is empty", !empty.spec && Object.keys(empty.work).length === 0 && empty.limit === null);

/* --- links from before the redesign --------------------------------------- */

const oldLoadout = S.decode("?t=retail%2F6%2F250%2Fspec&bc=1mrs1-1mrt2&bs=1n001&bh=1no01&h=retail%2F6%2F250%2Fhero%2F31&m=b");
check("an old loadout link opens with all three trees fixed",
  ["class", "spec", "hero"].every((r) => oldLoadout.work[r]?.mode === "fixed"));
check("and their points", oldLoadout.work.class.points[String(parseInt("1mrs", 36))] === 1);

const oldSearch = S.decode("?t=retail%2F6%2F250%2Fhero%2F31&p=8&r=1n001&s=1n0aa&m=e");
check("an old search link maps its tree to a spec key", oldSearch.spec === "retail/6/250/spec", oldSearch.spec);
check("and opens that tree with the search", oldSearch.work.hero?.mode === "open" && oldSearch.work.hero.search.budget === 8);
check("pinned sides included", oldSearch.work.hero.search.sides[String(parseInt("1n0a", 36))] === "a");

/* --- the workspace model -------------------------------------------------- */

const node = (id, kind = "single", entries = 1) => ({
  nodeId: id, kind, name: `n${id}`, entries: Array.from({ length: entries }, (_, i) => ({ name: `e${i}` })),
});

let w = W.emptyWork("open");
w = W.paint(w, node(1), "toggle", false);
check("a click requires", same(w.search.required, [1]));
w = W.paint(w, node(1), "toggle", false);
check("a second bars", same(w.search.required, []) && same(w.search.excluded, [1]));
w = W.paint(w, node(1), "toggle", false);
check("a third clears", w.search.excluded.length === 0);
w = W.paint(w, node(1), "toggle", true);
check("right-click goes backwards, straight to barred", same(w.search.excluded, [1]));

const choice = node(5, "choice", 2);
let c = W.emptyWork("open");
const cycle = [];
for (let i = 0; i < 4; i++) {
  c = W.paint(c, choice, "toggle", false);
  cycle.push(c.search.excluded.includes(5) ? "barred" : (c.search.sides["5"] ?? "free"));
}
check("a choice node cycles left, right, barred, free", same(cycle, ["a", "b", "barred", "free"]), cycle.join(" "));

let g = W.emptyWork("open");
g = W.paint(g, node(7), "toggle", false);
g = W.paint(g, node(7), "atLeastOne", false);
check("adding to a group clears the talent's own constraint", same(g.search.atLeastOne, [7]) && g.search.required.length === 0);
check("a group of one is pending, not sent", W.pendingOf(g).length === 1 && !W.payloadOf(g, 30).atLeastOneOf);
g = W.paint(g, node(8), "atLeastOne", false);
check("a group of two is sent", same(W.payloadOf(g, 30).atLeastOneOf, [[7, 8]]));

/* --- rank limits on multi-rank talents ----------------------------------- */

const three = { ...node(9), maxPoints: 3 };
const stepThrough = [];
let r = W.emptyWork("open");
for (let i = 0; i < 5; i++) {
  r = W.paint(r, three, "toggle", false);
  stepThrough.push(W.rangeOf(r.search, three).join("-"));
}
check("a 3-rank talent steps its minimum: 1+, 2+, maxed, barred, free",
  same(stepThrough, ["1-3", "2-3", "3-3", "0-0", "0-3"]), stepThrough.join(" "));
check("and 1+ is plain 'required', with no range written",
  (() => { const one = W.paint(W.emptyWork("open"), three, "toggle", false); return same(one.search.required, [9]) && !Object.keys(one.search.ranks).length; })());

let cap = W.paint(W.emptyWork("open"), three, "toggle", false);
cap = W.paint(cap, three, "toggle", false); // at least 2
cap = W.paint(cap, three, "atMost", false); // at most 2 -> exactly 2
check("At most on a 2+ talent makes exactly 2", W.rangeOf(cap.search, three).join("-") === "2-2");
check("sent as a minimum and a cap", same(W.payloadOf(cap, 30).rankMin, { 9: 2 }) && same(W.payloadOf(cap, 30).rankMax, { 9: 2 })
  && same(W.payloadOf(cap, 30).mustHave, [9]));
cap = W.paint(cap, three, "atMost", false); // at most 1: the minimum comes down with it
check("a lower cap pulls the minimum down with it", W.rangeOf(cap.search, three).join("-") === "1-1");
cap = W.paint(cap, three, "atMost", false); // back to no cap
check("and the cap cycles back to none", W.rangeOf(cap.search, three).join("-") === "1-3" && !W.payloadOf(cap, 30).rankMax);

let dip = W.paint(W.emptyWork("open"), three, "atMost", false);
dip = W.paint(dip, three, "atMost", false);
check("a free talent capped at 1: a one-point dip or nothing", W.rangeOf(dip.search, three).join("-") === "0-1"
  && same(W.payloadOf(dip, 30).rankMax, { 9: 1 }) && !W.payloadOf(dip, 30).mustHave);
check("At most on a one-rank talent bars it", same(W.paint(W.emptyWork("open"), node(4), "atMost", false).search.excluded, [4]));
check("a group tool clears a range", !Object.keys(W.paint(cap, three, "atLeastOne", false).search.ranks).length);

const ranked = { ...W.emptyWork("open"), search: { ...cap.search, ranks: { 9: { min: 2, max: 2 }, 77: { min: 0, max: 1 } }, budget: 0 } };
const rt = S.decode(S.encode({ ...S.NOTHING, spec: "retail/6/250/spec", work: { spec: ranked } }));
check("rank limits survive a link", same(rt.work.spec.search.ranks, { 9: { min: 2, max: 2 }, 77: { min: 0, max: 1 } }),
  JSON.stringify(rt.work.spec.search.ranks));
check("and so does a budget of 0 -- a pooled tab held empty", rt.work.spec.search.budget === 0);

const budget = W.payloadOf({ ...W.emptyWork(), search: { ...W.EMPTY_SEARCH, budget: 40 } }, 34);
check("a budget above the cap is clamped to it", budget.points === 34);

await server.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall share and workspace tests passed");
process.exit(failures.length ? 1 : 0);
