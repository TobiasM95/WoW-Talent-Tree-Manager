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
  build: { 88203: 1, 88210: 2, 91045: 1 },
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
check(
  "a build survives with its point counts",
  JSON.stringify(round.build) === JSON.stringify(state.build),
  JSON.stringify(round.build),
);

// Multi-rank talents are the case an encoding is most likely to flatten, since a naive
// "list of taken ids" loses how many points each one has.
const ranked = decode(encode({ ...state, build: { 100: 1, 200: 2, 300: 3 } }));
check(
  "point counts are not flattened to 1",
  ranked.build?.["200"] === 2 && ranked.build?.["300"] === 3,
  JSON.stringify(ranked.build),
);

// A talent with zero points is not in the build; carrying it would inflate every link.
const sparse = decode(encode({ ...state, build: { 100: 0, 200: 1 } }));
check("zero-point talents are omitted", !("100" in (sparse.build ?? {})),
      JSON.stringify(sparse.build));

const empty = decode(encode({
  tree: null, points: null, required: [], excluded: [],
  sides: new Map(), atLeastOne: [], exactlyOne: [], build: null,
}));
check(
  "an empty state encodes to nothing and decodes back to empty",
  encode({
    tree: null, points: null, required: [], excluded: [],
    sides: new Map(), atLeastOne: [], exactlyOne: [], build: null,
  }) === "" && empty.tree === null && empty.build === null,
);

// Links get truncated, hand-edited and pasted out of chat clients. Garbage must produce an
// empty view, never a wrong one.
for (const junk of ["?t=&p=abc&r=zzz-", "?r=-,-&x=!!!", "?b=-&s=x", "?p=-5"]) {
  const parsed = decode(junk);
  const ok =
    parsed.required.every((id) => Number.isFinite(id) && id > 0) &&
    parsed.excluded.every((id) => Number.isFinite(id) && id > 0) &&
    (parsed.points === null || parsed.points > 0) &&
    (parsed.build === null || Object.values(parsed.build).every((p) => p > 0));
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
