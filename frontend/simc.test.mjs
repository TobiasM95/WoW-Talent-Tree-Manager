/**
 * The character space and its SimulationCraft export, against the real API.
 *
 *   node simc.test.mjs [site-url]
 *
 * Two claims carry the redesigned workflow, and both are checked here against the solver
 * rather than against my reading of it:
 *
 *   1. Expanding enumerated selections into both sides of every free choice node gives
 *      *exactly* the API's `builds` count -- so the number a player narrows against is the
 *      number of lines they get.
 *   2. Every exported line is a whole, legal character: each tree's part is a build its
 *      search allows, and the string decodes back to precisely that character.
 */
import { createServer } from "vite";
import { count as countOf, listBuilds, siteGet } from "./testlib.mjs";

// The site, whose data files stand in for the old API (testlib.mjs).
const site = (process.argv[2] ?? "http://localhost:8081").replace(/\/$/, "");
const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const SPACE = await server.ssrLoadModule("/src/lib/space.ts");
const SIMC = await server.ssrLoadModule("/src/lib/simc.ts");
const STR = await server.ssrLoadModule("/src/lib/loadoutString.ts");
const L = await server.ssrLoadModule("/src/lib/loadout.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};
const get = (p) => siteGet(site, p);

/** An open tree's builds, listed as the page lists them. */
const enumerate = (treeKey, body) => listBuilds(site, treeKey, body);
const post = async (_path, body) => {
  const r = await countOf(site, body);
  if (r.status) throw new Error(r.detail);
  return r;
};

const trees = {};
for (const key of ["retail/6/250/class", "retail/6/250/spec", "retail/6/250/hero/31", "retail/6/250/hero/33"]) {
  trees[key] = await get(`/trees/${key}`).catch(() => null);
}
const heroKey = trees["retail/6/250/hero/31"] ? "retail/6/250/hero/31" : null;
const hero = trees[heroKey];
const choiceIds = hero.nodes.filter((n) => n.kind === "choice" && n.entries.length >= 2).map((n) => String(n.nodeId));

/* --- 1. the expansion is the API's count ----------------------------------- */

const cases = [
  ["hero, 8 points, every side free", { points: 8 }],
  ["hero, 8 points, one side pinned", { points: 8, choiceSides: { [choiceIds[0]]: "a" } }],
  ["hero, 8 points, two sides pinned", { points: 8, choiceSides: { [choiceIds[0]]: "b", [choiceIds[1]]: "a" } }],
  ["hero, 10 points", { points: 10 }],
];
for (const [label, body] of cases) {
  const count = await post("/counts", { treeKey: heroKey, ...body });
  const selections = await enumerate(heroKey, body);
  const expanded = selections.flatMap((s) => SPACE.expand(hero, s, body.choiceSides ?? {}));
  check(
    `${label}: expansion equals the API's builds`,
    expanded.length === count.builds && selections.length === count.sets,
    `${selections.length} selections -> ${expanded.length} expanded, API says ${count.sets} / ${count.builds}`,
  );
  const unique = new Set(expanded.map((v) => JSON.stringify([v.points, v.choices])));
  check(`${label}: and every expanded build is distinct`, unique.size === expanded.length);
}

/* --- 2. whole characters, exported ---------------------------------------- */

const spec = trees["retail/6/250/spec"];
const cls = trees["retail/6/250/class"];
const all = Object.values(trees).filter(Boolean);
const capOf = (t) => t.pointCap ?? t.maxPointsInTree;

// Fixed class and spec trees: the first legal full build of each.
const fill = (tree) => {
  let points = {};
  for (let i = 0; i < 80; i++) {
    const next = [...L.available(tree, points, capOf(tree))][0];
    if (next === undefined) break;
    const node = tree.nodes.find((n) => n.nodeId === next);
    points = L.add(tree, points, capOf(tree), node).points;
  }
  return points;
};
const classPoints = fill(cls);
const specPoints = fill(spec);
const heroSelections = await enumerate(heroKey, { points: 8 });
const heroVariants = heroSelections.flatMap((s) => SPACE.expand(hero, s, {}));

const characters = SPACE.characters(
  [
    { key: cls.key, variants: [SPACE.fixedVariant(cls, classPoints, {})] },
    { key: spec.key, variants: [SPACE.fixedVariant(spec, specPoints, {})] },
    { key: hero.key, variants: heroVariants },
  ],
  10000,
);
check("the product is class × spec × hero", characters.length === heroVariants.length, `${characters.length}`);
check("characters are numbered from 1, in order", characters.every((c, i) => c.line === i + 1));

const capped = SPACE.characters([{ key: "x", variants: heroVariants }], 100);
check("the limit caps the list", capped.length === 100);

const strings = SIMC.talentStrings({ spec, trees: all, heroSubTreeId: hero.subTreeId, characters });
let exact = 0;
let legal = 0;
for (let i = 0; i < characters.length; i++) {
  const c = characters[i];
  const decoded = STR.decode(strings[i], spec, all);
  const samePoints = JSON.stringify(Object.entries(decoded.points).sort()) ===
    JSON.stringify(Object.entries(c.points).filter(([, v]) => v > 0).sort());
  const sameChoices = Object.entries(c.choices).every(([id, side]) => decoded.choices[id] === side);
  if (samePoints && sameChoices && decoded.heroSubTreeId === hero.subTreeId) exact++;
  else if (process.env.DEBUG_DECODE && !globalThis.__shown) {
    globalThis.__shown = true;
    const want = Object.fromEntries(Object.entries(c.points).filter(([, v]) => v > 0));
    const diffP = [...new Set([...Object.keys(want), ...Object.keys(decoded.points)])]
      .filter((k) => want[k] !== decoded.points[k])
      .map((k) => `${k}: want ${want[k]} got ${decoded.points[k]} (${all.flatMap((t) => t.nodes).find((n) => String(n.nodeId) === k)?.kind})`);
    const diffC = Object.entries(c.choices).filter(([k, v]) => decoded.choices[k] !== v)
      .map(([k, v]) => `${k}: want side ${v} got ${decoded.choices[k]}`);
    console.log("   first mismatch, line", c.line, { samePoints, sameChoices, hero: decoded.heroSubTreeId, diffP, diffC });
  }
  const heroPart = c.parts[hero.key].points;
  if (L.place(hero, heroPart, 8).points && L.total(heroPart) === 8) legal++;
}
check("every string decodes back to exactly its character", exact === characters.length, `${exact} of ${characters.length}`);
check("every hero part is a legal 8-point build", legal === characters.length, `${legal} of ${characters.length}`);
check("every line is a whole character, not one tree",
  characters.every((c) => Object.keys(c.parts).length === 3));
check("strings are unique", new Set(strings).size === strings.length);

// Both sides of a free choice node are simmed.
const id = choiceIds[0];
const sides = new Set(characters.filter((c) => c.choices[id] !== undefined).map((c) => c.choices[id]));
check("both sides of a free choice node are in the export", sides.size === 2, [...sides].join(","));

/* --- both hero trees in one search ----------------------------------------- */

const other = trees["retail/6/250/hero/33"];
const otherSelections = await enumerate(other.key, { points: 8 });
const tag = (tree, list) => list.map((v) => ({ ...v, tree: tree.key, hero: tree.subTreeId }));
const pooled = SPACE.characters(
  [
    { key: cls.key, variants: [SPACE.fixedVariant(cls, classPoints, {})] },
    { key: spec.key, variants: [SPACE.fixedVariant(spec, specPoints, {})] },
    {
      key: "hero",
      variants: [
        ...tag(hero, heroSelections.flatMap((x) => SPACE.expand(hero, x, {}))),
        ...tag(other, otherSelections.flatMap((x) => SPACE.expand(other, x, {}))),
      ],
    },
  ],
  10000,
);
const otherCount = (await post("/counts", { treeKey: other.key, points: 8 })).builds;
check("both hero trees make the hero factor a sum", pooled.length === heroVariants.length + otherCount,
  `${pooled.length} = ${heroVariants.length} + ${otherCount}`);
const pooledStrings = SIMC.talentStrings({ spec, trees: all, heroSubTreeId: hero.subTreeId, characters: pooled });
let named = 0;
for (let i = 0; i < pooled.length; i++) {
  const c = pooled[i];
  const partKey = Object.keys(c.parts).find((k) => k.includes("/hero/"));
  const d = STR.decode(pooledStrings[i], spec, all);
  const wantHero = trees[partKey].subTreeId;
  const samePoints = JSON.stringify(Object.entries(d.points).sort()) ===
    JSON.stringify(Object.entries(c.points).filter(([, v]) => v > 0).sort());
  if (d.heroSubTreeId === wantHero && samePoints) named++;
}
check("each line names its own hero tree, and decodes to its character", named === pooled.length,
  `${named} of ${pooled.length}`);

/* --- the text -------------------------------------------------------------- */

const text = SIMC.profilesets(strings, { className: "Death Knight", specName: "Blood", note: "test" });
const lines = text.trim().split("\n");
const sets = lines.filter((l) => l.startsWith("profileset."));
check("one profileset line per character", sets.length === characters.length);
check("each overrides only talents", sets.every((l) => /^profileset\."ttm_\d+"\+=talents=[A-Za-z0-9+/]+$/.test(l)), sets[0]);
check("names are zero-padded to sort in order", /ttm_0*1"/.test(sets[0]) && sets[0].includes(`ttm_${"1".padStart(String(sets.length).length, "0")}"`));
const settings = text.split("profileset.")[0].split("\n").filter((l) => l.trim() && !l.startsWith("#"));
check("the file carries no profile of its own", settings.length === 0, settings[0] ?? "");
check("it says how to run it and bring the report back", text.includes("json2=report.json") && text.includes("_Death_Knight_Blood.simc"));

await server.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall space and export tests passed");
process.exit(failures.length ? 1 : 0);
