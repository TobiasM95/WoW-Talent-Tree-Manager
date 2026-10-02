/**
 * The browser counter against reference counts: every case's sets and builds at every point
 * total must match exactly.
 *
 *   node counter.test.mjs <cases.json>
 *
 * A case names a tree file (relative to the repo root), a search, and the expected spreads, as
 * strings since they can pass 2^53. They come from an independent implementation: the Python DP
 * during the port, the C++ engine in the parity suite.
 */
import { createServer } from "vite";
import { readFile } from "node:fs/promises";
import { join } from "node:path";

const casesPath = process.argv[2];
if (!casesPath) throw new Error("usage: node counter.test.mjs <cases.json>");
const root = join(process.cwd(), "..");

const server = await createServer({ server: { middlewareMode: true, hmr: false }, appType: "custom", logLevel: "error" });
const C = await server.ssrLoadModule("/src/engine/counter.ts");

const cases = JSON.parse(await readFile(casesPath, "utf8"));
const trees = new Map();
const graphs = new Map();
let passed = 0;
const failures = [];
const slowest = { ms: 0, label: "" };
const started = performance.now();

for (const c of cases) {
  if (!trees.has(c.tree)) trees.set(c.tree, JSON.parse(await readFile(join(root, c.tree), "utf8")));
  if (!graphs.has(c.tree)) graphs.set(c.tree, C.buildGraph(trees.get(c.tree), 90));
  const graph = graphs.get(c.tree);
  const t0 = performance.now();
  const sets = C.countSpread(graph, c.search, c.slots, false).map(String);
  const builds = C.countSpread(graph, c.search, c.slots, true).map(String);
  const ms = performance.now() - t0;
  if (ms > slowest.ms) Object.assign(slowest, { ms, label: `${c.tree} ${c.label}` });
  const same = graph.slots === c.slots && sets.join() === c.sets.join() && builds.join() === c.builds.join();
  if (same) passed++;
  else {
    const k = sets.findIndex((v, i) => v !== c.sets[i]);
    failures.push(`${c.tree} ${c.label}: slots ${graph.slots}/${c.slots}, first difference at ${k}: ${sets[k]} vs ${c.sets[k]}`);
  }
}

await server.close();
console.log(`${passed} of ${cases.length} cases match, over ${trees.size} trees`);
console.log(`   ${((performance.now() - started) / 1000).toFixed(1)}s in all; slowest ${slowest.ms.toFixed(0)} ms (${slowest.label})`);
for (const f of failures.slice(0, 12)) console.log(`FAIL ${f}`);
process.exit(failures.length ? 1 : 0);
