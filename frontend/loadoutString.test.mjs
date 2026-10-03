/**
 * The Blizzard loadout string codec.
 *
 *   node loadoutString.test.mjs [site-url]
 *
 * A round trip proves almost nothing on its own: encode and decode share a reading of the
 * layout, so a wrong reading round-trips perfectly. Two things are therefore load-bearing
 * here, and neither is the round trip.
 *
 * **A golden string exported from the game.** It caught three faults a round trip could not:
 * a missing `purchased` bit, an unhandled hero-tree selector, and the fact that class trees
 * differ per specialisation. Decoding it and re-encoding it byte for byte is the only check
 * that speaks to whether the layout is right at all.
 *
 * **The refusal path**, because the alternative to rejecting a string this tool cannot read
 * is a build that looks right and is not -- which a person has no way to detect.
 */
import { createServer } from "vite";
import { siteGet } from "./testlib.mjs";

// The site, whose data files stand in for the old API (testlib.mjs).
const site = (process.argv[2] ?? "http://localhost:8081").replace(/\/$/, "");
const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const S = await server.ssrLoadModule("/src/lib/loadoutString.ts");
const L = await server.ssrLoadModule("/src/lib/loadout.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};
const get = (p) => siteGet(site, p);
const rejects = (fn) => {
  try {
    fn();
    return null;
  } catch (error) {
    return error instanceof S.LoadoutStringError ? error.message : `wrong type: ${error}`;
  }
};

/*
  --- the golden string ------------------------------------------------------

  Exported from the game: a Feral Druid with a full spec tree, a half-spent class tree and a
  partly-spent hero tree, chosen to cover granted talents, partial ranks, choice nodes and
  the hero-tree selector at once.
*/
const GOLDEN = {
  text: "CcGAAAAAAAAAAAAAAAAAAAAAAAAAAAAwYmxiZMzMzMjlZWGmxMzMzAAAAwSYGDegZmZqZmBYGAAAAAAAAGDAAAIjxMmBEYBwMAADAAAzGeAA",
  className: "Druid",
  specName: "Feral",
  specId: 103,
  heroSubTreeId: 21,
  perTree: { class: 22, spec: 34, hero: 8 },
  granted: 5,
};

const summaries = await get("/trees");

// A class tree belongs to a specialisation, not just a class: which talents are granted
// differs between them, so matching on class alone picks the wrong tree and the string stops
// matching. That is one of the faults the golden string caught.
const treesFor = async (className, specName) => {
  const mine = summaries.filter((t) => t.className === className && t.specName === specName);
  const out = [];
  for (const summary of mine) out.push(await get(`/trees/${summary.key}`));
  return out;
};

{
  const goldenTrees = await treesFor(GOLDEN.className, GOLDEN.specName);
  const goldenSpec = goldenTrees.find((t) => t.kind === "spec");
  const decoded = S.decode(GOLDEN.text, goldenSpec, goldenTrees);

  check("the golden string names its specialisation",
        decoded.specId === GOLDEN.specId, String(decoded.specId));
  check("it names its hero tree",
        decoded.heroSubTreeId === GOLDEN.heroSubTreeId, String(decoded.heroSubTreeId));
  check("it carries granted talents, which cost no points",
        decoded.granted.length === GOLDEN.granted, `${decoded.granted.length}`);
  check("nothing in it is unknown to these trees",
        decoded.unknown.length === 0, JSON.stringify(decoded.unknown));

  const per = {};
  for (const tree of goldenTrees) {
    const spentHere = tree.nodes.reduce(
      (sum, node) => sum + (decoded.points[String(node.nodeId)] ?? 0), 0);
    if (spentHere) per[tree.kind] = spentHere;
  }
  // Compared key by key: the two objects are built in different orders, and JSON.stringify
  // is sensitive to that in a way nobody means it to be.
  const sameTotals =
    Object.keys(GOLDEN.perTree).length === Object.keys(per).length &&
    Object.entries(GOLDEN.perTree).every(([kind, n]) => per[kind] === n);
  check("the points land where the build put them", sameTotals,
        `${JSON.stringify(per)} vs ${JSON.stringify(GOLDEN.perTree)}`);

  // The check that actually speaks to correctness: byte for byte, not merely self-consistent.
  const again = S.encode({
    spec: goldenSpec, trees: goldenTrees, points: decoded.points,
    choices: decoded.choices, heroSubTreeId: decoded.heroSubTreeId,
  });
  check("re-encoding reproduces the game's string exactly", again === GOLDEN.text,
        again.slice(0, 48));
}

const specSummary = summaries.find((t) => t.kind === "spec");
const trees = await treesFor(specSummary.className, specSummary.specName);
const spec = trees.find((t) => t.kind === "spec");
const classTree = trees.find((t) => t.kind === "class");
const heroTree = trees.find((t) => t.kind === "hero");

check("the spec tree carries a node order", (spec.fullNodeOrder?.length ?? 0) > 0,
      String(spec.fullNodeOrder?.length));
check("the order is wider than this spec's own trees",
      spec.fullNodeOrder.length > trees.reduce((n, t) => n + t.nodes.length, 0) / 2,
      `${spec.fullNodeOrder.length} ids vs ${trees.reduce((n, t) => n + t.nodes.length, 0)} nodes`);

// --- a real loadout, built under the solver's rules -------------------------
const build = (tree, cap, seed) => {
  let state = seed >>> 0;
  const rnd = () => ((state = (state * 1664525 + 1013904223) >>> 0) / 4294967296);
  let points = {};
  for (let i = 0; i < cap; i++) {
    const open = [...L.available(tree, points, cap)];
    if (!open.length) break;
    // Drawn once, into a variable: calling rnd() inside the find predicate advances it on
    // every node tested, so each comparison is against a different element and the search
    // almost always comes back empty.
    const nodeId = open[Math.floor(rnd() * open.length)];
    const node = tree.nodes.find((n) => n.nodeId === nodeId);
    const result = L.add(tree, points, cap, node);
    if (result.reason) break;
    points = result.points;
  }
  return points;
};

const capOf = (t) => t.pointCap ?? t.maxPointsInTree;
const points = {
  ...build(classTree, capOf(classTree), 5),
  ...build(spec, capOf(spec), 9),
  ...build(heroTree, capOf(heroTree), 3),
};
const choices = {};
for (const tree of trees) {
  for (const node of tree.nodes) {
    if (node.kind === "choice" && points[String(node.nodeId)]) choices[String(node.nodeId)] = 1;
  }
}

const text = S.encode({ spec, trees, points, choices });
check("a loadout encodes to a base64-looking string",
      /^[A-Za-z0-9+/]+$/.test(text), text.slice(0, 40));
check("the string is a sane length for a full loadout",
      text.length > 20 && text.length < 400, `${text.length} characters`);

const back = S.decode(text, spec, trees);
check("every talent survives the round trip",
      JSON.stringify(back.points) === JSON.stringify(points),
      `${Object.keys(back.points).length} of ${Object.keys(points).length}`);
check("multi-rank talents keep their rank",
      Object.entries(points).every(([id, n]) => back.points[id] === n));
check("choice-node sides survive",
      Object.entries(choices).every(([id, side]) => back.choices[id] === side),
      JSON.stringify(back.choices));
check("no talent decodes that this spec does not have", back.unknown.length === 0,
      JSON.stringify(back.unknown.slice(0, 5)));
check("the header names this spec", back.specId === spec.specId, String(back.specId));

// An empty loadout is still a valid string: it is what "I have spent nothing" looks like.
const emptyText = S.encode({ spec, trees, points: {} });
check("an empty loadout round-trips to empty",
      Object.keys(S.decode(emptyText, spec, trees).points).length === 0);

// --- refusing what it cannot read -------------------------------------------
/*
  The half that matters. Every case below is a string this tool must *reject*, because the
  alternative is a build that looks right and is not -- and a user has no way to tell.
*/
check("junk characters are refused",
      Boolean(rejects(() => S.decode("not a loadout!!", spec, trees))),
      "accepted");
check("an empty paste is refused", Boolean(rejects(() => S.decode("   ", spec, trees))));

const otherSpec = (await treesFor(
  spec.className,
  summaries.find((t) => t.kind === "spec" && t.className === spec.className
    && t.specName !== spec.specName).specName,
)).find((t) => t.kind === "spec");
const foreign = S.encode({ spec: otherSpec, trees, points });
const wrongSpec = rejects(() => S.decode(foreign, spec, trees));
check("a string for a sibling specialisation is refused", Boolean(wrongSpec), "accepted");
check("and it says which specialisation it was for",
      /specialisation \d+/.test(wrongSpec ?? ""), String(wrongSpec));

const truncated = text.slice(0, Math.floor(text.length / 2));
check("a truncated string is refused or decodes to fewer talents", (() => {
  const message = rejects(() => S.decode(truncated, spec, trees));
  if (message) return true;
  const partial = S.decode(truncated, spec, trees);
  return Object.keys(partial.points).length < Object.keys(points).length;
})());

// A different class entirely: a longer node order read against this spec must not pass.
const otherClass = summaries.find((t) => t.kind === "spec" && t.className !== spec.className);
const otherClassSpec = await get(`/trees/${otherClass.key}`);
const otherTrees = [otherClassSpec];
const alien = S.encode({
  spec: otherClassSpec,
  trees: otherTrees,
  points: build(otherClassSpec, capOf(otherClassSpec), 11),
});
check("a string from another class is refused",
      Boolean(rejects(() => S.decode(alien, spec, trees))), "accepted");

// Version byte bumped: the layout may have changed under us, so stop rather than guess.
const bumped = (() => {
  const A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const bits = [];
  for (const ch of text) for (let b = 0; b < 6; b++) bits.push((A.indexOf(ch) >> b) & 1);
  bits[0] = 1; // version 2 -> 3
  bits[1] = 1;
  let out = "";
  for (let i = 0; i < bits.length; i += 6) {
    let chunk = 0;
    for (let b = 0; b < 6; b++) chunk |= (bits[i + b] ?? 0) << b;
    out += A[chunk];
  }
  return out;
})();
const versionError = rejects(() => S.decode(bumped, spec, trees));
check("an unknown format version is refused", Boolean(versionError), "accepted");
check("and it says so plainly", /version/i.test(versionError ?? ""), String(versionError));

// --- what comes back must be buildable --------------------------------------
// A string that decodes to something the spending rules reject would put the tool in a
// state it says is impossible.
for (const tree of trees) {
  const mine = Object.fromEntries(
    Object.entries(back.points).filter(([id]) => tree.nodes.some((n) => String(n.nodeId) === id)),
  );
  const replaced = L.place(tree, mine, capOf(tree));
  check(`${tree.kind}: the decoded loadout is one the rules allow`,
        L.total(replaced.points) === L.total(mine) && Object.keys(replaced.dropped).length === 0,
        `${L.total(replaced.points)} of ${L.total(mine)} placed`);
}

await server.close();
console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all loadout-string tests passed");
