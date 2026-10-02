/**
 * Parity: the browser counter and lister against the C++ engine.
 *
 *   python tools/parity/engine_reference.py --solver <ttm-solver> --out .parity.json <tree dirs>
 *   node parity.test.mjs .parity.json
 *
 * The engine's answers come from the native solver (see tools/parity/engine_reference.py).
 * For every tree and search:
 *   - the browser's count of sets at each budget equals the engine's, and where the engine
 *     declined a budget as more than the tree holds, the browser agrees it is out of range;
 *   - where the engine listed every build, the browser lists exactly the same builds, as sets.
 *
 * The release CI runs this. A feature in one implementation and not the other fails it.
 */
import { createServer } from "vite";
import { readFile } from "node:fs/promises";

const path = process.argv[2];
if (!path) throw new Error("usage: node parity.test.mjs <engine reference json>");

const server = await createServer({ server: { middlewareMode: true, hmr: false }, appType: "custom", logLevel: "error" });
const C = await server.ssrLoadModule("/src/engine/counter.ts");
const L = await server.ssrLoadModule("/src/engine/lister.ts");

const ref = JSON.parse(await readFile(path, "utf8"));
const graphs = new Map(Object.entries(ref.trees).map(([key, tree]) => [key, C.buildGraph(tree, ref.levelCap)]));
const canon = (b) => JSON.stringify(Object.entries(b).filter(([, v]) => v > 0).sort(([a], [b]) => Number(a) - Number(b)));

let counts = 0;
let listings = 0;
let builds = 0;
const failures = [];
const kinds = new Set();
for (const c of ref.cases) {
  const graph = graphs.get(c.key);
  kinds.add(c.label);
  const top = Math.max(...Object.keys(c.counts).map(Number));
  const spread = C.countSpread(graph, c.search, Math.min(top, graph.slots), false);
  for (const [b, engine] of Object.entries(c.counts)) {
    const points = Number(b);
    if (engine === null) {
      if (points <= graph.slots) failures.push(`${c.key} ${c.label} @${b}: the engine gave no count, but the tree holds ${graph.slots} points`);
      continue;
    }
    counts++;
    const mine = points <= graph.slots ? spread[points] : 0n;
    if (mine !== BigInt(engine)) failures.push(`${c.key} ${c.label} @${b}: browser ${mine}, engine ${engine}`);
  }
  if (c.listing) {
    listings++;
    const mine = L.list(graph, c.search, c.listing.points, 1e7);
    const a = new Set(mine.builds.map(canon));
    const e = new Set(c.listing.builds.map(canon));
    builds += e.size;
    const missing = [...e].find((x) => !a.has(x));
    const extra = [...a].find((x) => !e.has(x));
    if (a.size !== e.size || missing || extra) {
      failures.push(`${c.key} ${c.label} @${c.listing.points}: browser lists ${a.size}, engine ${e.size}` +
        (missing ? `; engine has ${missing.slice(0, 80)}` : "") + (extra ? `; browser has ${extra.slice(0, 80)}` : ""));
    }
  }
}
await server.close();

console.log(`${Object.keys(ref.trees).length} trees, ${ref.cases.length} searches (${[...kinds].sort().join(", ")})`);
console.log(`${counts - failures.filter((f) => f.includes("engine ") && !f.includes("lists")).length} of ${counts} counts agree with the engine`);
console.log(`${listings} listings compared, ${builds} builds`);
for (const f of failures.slice(0, 15)) console.log(`FAIL ${f}`);
console.log(failures.length ? `\n${failures.length} FAILED` : "\nthe browser and the C++ engine agree");
process.exit(failures.length ? 1 : 0);
