/**
 * SimulationCraft export.
 *
 *   node simc.test.mjs [api-url]
 *
 * The thing worth checking is not the text format -- that is a few lines of string joining --
 * but that every exported line is a *whole, valid character*. The solver varies one tree at a
 * time, so each line has to be the enumerated tree's points combined with whatever the other
 * two hold; drop that and the export is a spec tree with no class talents, which sims but is
 * not what anyone asked for.
 *
 * So each generated string is decoded back and checked: the fixed trees must be untouched,
 * the varying tree must match the build it came from, and the whole thing must still be a
 * loadout the spending rules allow.
 */
import { createServer } from "vite";

const api = (process.argv[2] ?? "http://localhost:8001").replace(/\/$/, "");
const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const SIMC = await server.ssrLoadModule("/src/lib/simc.ts");
const STR = await server.ssrLoadModule("/src/lib/loadoutString.ts");
const L = await server.ssrLoadModule("/src/lib/loadout.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};
const get = async (p) => {
  const r = await fetch(api + p);
  if (!r.ok) throw new Error(`${p}: ${r.status}`);
  return r.json();
};

const summaries = await get("/trees");
const specSummary = summaries.find((t) => t.kind === "spec");
const mine = summaries.filter(
  (t) => t.className === specSummary.className && t.specName === specSummary.specName,
);
const trees = [];
for (const s of mine) trees.push(await get(`/trees/${s.key}`));
const spec = trees.find((t) => t.kind === "spec");
const classTree = trees.find((t) => t.kind === "class");
const heroTree = trees.find((t) => t.kind === "hero");
const capOf = (t) => t.pointCap ?? t.maxPointsInTree;

check("class and spec tokens are SimC-shaped",
      SIMC.token("Death Knight") === "death_knight" && SIMC.token("Beast Mastery") === "beast_mastery",
      `${SIMC.token("Death Knight")} / ${SIMC.token("Beast Mastery")}`);

// --- a base loadout across all three trees ---------------------------------
const build = (tree, cap, seed) => {
  let state = seed >>> 0;
  const rnd = () => ((state = (state * 1664525 + 1013904223) >>> 0) / 4294967296);
  let points = {};
  for (let i = 0; i < cap; i++) {
    const open = [...L.available(tree, points, cap)];
    if (!open.length) break;
    const id = open[Math.floor(rnd() * open.length)];
    const node = tree.nodes.find((n) => n.nodeId === id);
    const result = L.add(tree, points, cap, node);
    if (result.reason) break;
    points = result.points;
  }
  return points;
};

const base = {
  ...build(classTree, capOf(classTree), 5),
  ...build(spec, capOf(spec), 9),
  ...build(heroTree, capOf(heroTree), 3),
};

// --- enumerated variations of one tree -------------------------------------
// Real results, from a real job, so the export is exercised on what it will actually get.
const budget = 10;
const submitted = await fetch(api + "/solve", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ treeKey: spec.key, points: budget }),
}).then((r) => r.json());

let job = submitted;
for (let i = 0; i < 200 && !["done", "capped", "failed", "cancelled"].includes(job.state); i++) {
  await new Promise((r) => setTimeout(r, 200));
  job = await get(`/solve/${submitted.id}`);
}
check("a job to export from finished", job.state === "done", `${job.state}: ${job.error}`);

const page = await get(`/solve/${job.id}/results?limit=40`);
const builds = page.builds;
check("it produced builds to export", builds.length > 1, `${builds.length}`);

const { strings, collapsed } = SIMC.buildStrings({
  spec, trees, varying: spec, base, choices: {}, heroSubTreeId: heroTree?.subTreeId ?? null,
  builds,
});
check("every build becomes a talent string", strings.length > 0,
      `${strings.length} of ${builds.length}, ${collapsed} collapsed`);

// --- each line is a whole character ----------------------------------------
const varyingIds = new Set(spec.nodes.map((n) => n.nodeId));
const fixedExpected = Object.fromEntries(
  Object.entries(base).filter(([id]) => !varyingIds.has(Number(id))),
);

let intact = 0;
let matched = 0;
let legal = 0;
for (let i = 0; i < strings.length; i++) {
  const decoded = STR.decode(strings[i], spec, trees);

  // The trees that were held fixed must come back exactly as they went in.
  const fixedBack = Object.fromEntries(
    Object.entries(decoded.points).filter(([id]) => !varyingIds.has(Number(id))),
  );
  if (JSON.stringify(fixedBack) === JSON.stringify(fixedExpected)) intact += 1;

  // And the varying tree must be one of the enumerated results.
  const varyBack = Object.fromEntries(
    Object.entries(decoded.points).filter(([id]) => varyingIds.has(Number(id))),
  );
  if (builds.some((b) => JSON.stringify(b) === JSON.stringify(varyBack))) matched += 1;

  // The whole thing must still be buildable, tree by tree.
  const ok = trees.every((tree) => {
    const owned = Object.fromEntries(
      Object.entries(decoded.points).filter(([id]) =>
        tree.nodes.some((n) => String(n.nodeId) === id)),
    );
    const placed = L.place(tree, owned, capOf(tree));
    return L.total(placed.points) === L.total(owned);
  });
  if (ok) legal += 1;
}
check("the fixed trees survive untouched in every line", intact === strings.length,
      `${intact} of ${strings.length}`);
check("each line's varying tree is one of the enumerated builds",
      matched === strings.length, `${matched} of ${strings.length}`);
check("every exported build is one the rules allow", legal === strings.length,
      `${legal} of ${strings.length}`);
check("every line spends more than the varying tree alone",
      Object.keys(STR.decode(strings[0], spec, trees).points).length >
        Object.keys(builds[0]).length);

// --- the text ---------------------------------------------------------------
const text = SIMC.profilesets(strings, {
  className: spec.className, specName: spec.specName, note: `${budget} points`,
});
const lines = text.trim().split("\n");
const setLines = lines.filter((l) => l.startsWith("profileset."));
check("one profileset line per build", setLines.length === strings.length,
      `${setLines.length} of ${strings.length}`);
check("profileset lines override only talents",
      setLines.every((l) => /^profileset\."[^"]+"\+=talents=[A-Za-z0-9+/]+$/.test(l)),
      setLines[0]);
check("names are zero-padded so they sort in enumeration order",
      setLines.length < 10 || /_01\b|_001\b/.test(setLines[0]), setLines[0]);
check("the header explains what to do with it",
      lines[0].startsWith("#") && text.includes("your own profile"), lines[0]);

const runnable = SIMC.profilesets(strings, {
  className: spec.className, specName: spec.specName, withProfile: true,
});
check("a runnable profile names the class and spec",
      runnable.includes(`${SIMC.token(spec.className)}="TTM_`) &&
        runnable.includes(`spec=${SIMC.token(spec.specName)}`),
      runnable.split("\n").find((l) => l.startsWith("spec=")));
check("and it sets a talent string", /^talents=[A-Za-z0-9+/]+$/m.test(runnable));

// Duplicates are collapsed: the same character twice is wasted sim time.
const dupes = SIMC.buildStrings({
  spec, trees, varying: spec, base, choices: {}, heroSubTreeId: heroTree?.subTreeId ?? null,
  builds: [builds[0], builds[0], builds[1]],
});
check("identical builds are collapsed", dupes.strings.length === 2 && dupes.collapsed === 1,
      `${dupes.strings.length} kept, ${dupes.collapsed} collapsed`);

await server.close();
console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all simc export tests passed");
