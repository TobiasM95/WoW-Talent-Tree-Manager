/**
 * Round-trip tests for the share codec.
 *
 *   node share.test.mjs
 *
 * No browser: this is pure string handling, and it is the part where a mistake is silent.
 * A link that loses a constraint still opens, shows a plausible tree, and gives the wrong
 * answer -- there is nothing for a person to notice.
 *
 * Run through Vite so the TypeScript source is the thing under test rather than a copy.
 */
import { createServer } from "vite";

const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const { encode, decode } = await server.ssrLoadModule("/src/lib/share.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};

const state = {
  tree: "retail/11/102/spec",
  points: 27,
  required: [88203, 88210, 91045],
  excluded: [88219],
  sides: new Map([
    [88209, "a"],
    [88221, "b"],
    [88236, "none"],
  ]),
  atLeastOne: [88204, 88215, 88219],
  exactlyOne: [88221, 88236],
  spent: {
    class: { 76169: 1, 76170: 2 },
    spec: { 88203: 1, 88225: 1 },
    hero: { 91045: 1 },
  },
  heroKey: "retail/11/102/hero/23",
  mode: "explore",
};

const round = decode(encode(state));

check("the tree key survives, slashes and all", round.tree === state.tree, round.tree);
check("the point budget survives", round.points === state.points, String(round.points));
check(
  "required and excluded survive",
  String(round.required) === String(state.required) &&
    String(round.excluded) === String(state.excluded),
  `${round.required} / ${round.excluded}`,
);
check(
  "every choice side survives, including 'none'",
  [...state.sides].every(([id, side]) => round.sides.get(id) === side),
  JSON.stringify([...round.sides]),
);
check(
  "both group kinds survive",
  String(round.atLeastOne) === String(state.atLeastOne) &&
    String(round.exactlyOne) === String(state.exactlyOne),
);
/*
  A link carries one of two things, chosen by the mode: a *loadout* or a *search*. Carrying
  both is what produced links that arrived with a leftover budget and constraints their
  sender never meant to send.
*/
check("a search link carries no loadout",
      round.spent.class === null && round.spent.spec === null && round.spent.hero === null,
      JSON.stringify(round.spent));

const asBuild = decode(encode({ ...state, mode: "build" }));
check("a loadout link carries the loadout for all three trees",
      JSON.stringify(asBuild.spent) === JSON.stringify(state.spent),
      JSON.stringify(asBuild.spent));
check("and carries no search state",
      asBuild.points === null && asBuild.required.length === 0 &&
        asBuild.excluded.length === 0 && asBuild.sides.size === 0,
      `${asBuild.points} / ${asBuild.required.length} required`);

// Multi-rank talents are the case an encoding is most likely to flatten, since a naive
// "list of taken ids" loses how many points each one has.
const ranked = decode(encode({
  ...state, mode: "build",
  spent: { class: { 100: 1, 200: 2, 300: 3 }, spec: null, hero: null },
}));
check("point counts are not flattened to 1",
      ranked.spent.class?.["200"] === 2 && ranked.spent.class?.["300"] === 3,
      JSON.stringify(ranked.spent.class));

// A talent with zero points is not in the build; carrying it would inflate every link.
const sparse = decode(encode({
  ...state, mode: "build",
  spent: { class: { 100: 0, 200: 1 }, spec: null, hero: null },
}));
check("zero-point talents are omitted", !("100" in (sparse.spent.class ?? {})),
      JSON.stringify(sparse.spent.class));
check("the hero tree is named, since a spec has two", round.heroKey === state.heroKey,
      String(round.heroKey));

// A link can hold both a loadout and an enumerated build; guessing the mode from which
// fields are present reopened one showing neither.
check("the mode is carried, not guessed", round.mode === state.mode, String(round.mode));

const blank = { tree: null, points: null, required: [], excluded: [],
                sides: new Map(), atLeastOne: [], exactlyOne: [],
                spent: { class: null, spec: null, hero: null }, heroKey: null,
                mode: null };
const empty = decode(encode(blank));
check(
  "an empty state encodes to nothing and decodes back to empty",
  encode(blank) === "" && empty.tree === null && empty.spent.class === null,
);

// Called with hand-assembled state in a few places, so a missing field must drop a
// parameter rather than throw and lose the whole link.
const partial = { ...blank };
delete partial.spent;
check("a partial state encodes without throwing", encode(partial) === "");

// Links get truncated, hand-edited and pasted out of chat clients. Garbage must produce an
// empty view, never a wrong one.
for (const junk of ["?t=&p=abc&r=zzz-", "?r=-,-&x=!!!", "?bc=-&s=x", "?p=-5"]) {
  const parsed = decode(junk);
  const ok =
    parsed.required.every((id) => Number.isFinite(id) && id > 0) &&
    parsed.excluded.every((id) => Number.isFinite(id) && id > 0) &&
    (parsed.points === null || parsed.points > 0);
  check(`malformed input is discarded, not misread: ${junk}`, ok, JSON.stringify(parsed));
}

// Links live in chat clients that break long ones across lines.
const size = encode(state).length;
check("a full link stays short enough to paste", size < 400, `${size} characters`);
console.log(`     (${size} characters for a 27-point build with 9 constraints)`);

await server.close();

console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all share codec tests passed");
