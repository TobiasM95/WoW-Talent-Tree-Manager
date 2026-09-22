/**
 * Loadout rules, checked against the thing that counts.
 *
 *   node loadout.test.mjs [api-url]
 *
 * The assertion that matters is not "the rules look right" but **the solver agrees**: a
 * loadout built by these rules must be one of the builds `POST /counts` counts. If the two
 * ever disagree, the tool is telling a player they have made something it also says does
 * not exist -- and that is exactly the kind of quiet contradiction nobody notices until it
 * has been shipped for months.
 *
 * So this builds real loadouts on real trees and asks the API to count them. It needs the
 * API running; it does not need a browser.
 */
import { createServer } from "vite";

const api = (process.argv[2] ?? "http://localhost:8001").replace(/\/$/, "");

const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const L = await server.ssrLoadModule("/src/lib/loadout.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};

const get = async (path) => {
  const response = await fetch(api + path);
  if (!response.ok) throw new Error(`${path}: ${response.status}`);
  return response.json();
};

const count = async (body) => {
  const response = await fetch(api + "/counts", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!response.ok) throw new Error(`${await response.text()}`);
  return response.json();
};

/** Spend `budget` points by repeatedly taking a random available talent. */
function buildRandom(tree, budget, seed) {
  // Deterministic, so a failure can be rerun. A plain LCG is plenty here.
  let state = seed >>> 0;
  const rnd = () => ((state = (state * 1664525 + 1013904223) >>> 0) / 4294967296);

  let points = {};
  for (let i = 0; i < budget; i++) {
    const open = [...L.available(tree, points, budget)];
    if (open.length === 0) break;
    const nodeId = open[Math.floor(rnd() * open.length)];
    const node = tree.nodes.find((n) => n.nodeId === nodeId);
    const result = L.add(tree, points, budget, node);
    if (result.reason) break;
    points = result.points;
  }
  return points;
}

const trees = await get("/trees");
const sample = [
  trees.find((t) => t.kind === "spec"),
  trees.find((t) => t.kind === "class"),
  trees.find((t) => t.kind === "hero"),
].filter(Boolean);

for (const summary of sample) {
  const tree = await get(`/trees/${summary.key}`);
  const cap = Math.min(summary.pointCap ?? summary.maxPointsInTree, 20);

  // --- the rules agree with the counter -----------------------------------
  let agreed = 0;
  let checked = 0;
  for (let seed = 1; seed <= 6; seed++) {
    const points = buildRandom(tree, cap, seed);
    const spent = L.total(points);
    if (spent === 0) continue;
    checked += 1;
    const result = await count({
      treeKey: summary.key,
      points: spent,
      mustHave: Object.keys(points).map(Number),
    });
    if (result.sets >= 1) agreed += 1;
    else {
      console.log(`     ${summary.key} seed ${seed}: ${spent} points, ` +
                  `${Object.keys(points).length} talents -> 0 matching builds`);
    }
  }
  check(`${summary.kind}: every hand-built loadout is one the counter counts`,
        checked > 0 && agreed === checked, `${agreed}/${checked}`);

  // --- the rules are not vacuous ------------------------------------------
  const full = buildRandom(tree, cap, 42);
  check(`${summary.kind}: a loadout actually spends its budget`,
        L.total(full) === cap, `${L.total(full)} of ${cap}`);

  // --- gates bite ----------------------------------------------------------
  const gated = tree.nodes.filter((n) => n.pointsRequired > 0);
  if (gated.length) {
    const deepest = gated.reduce((a, b) => (a.pointsRequired > b.pointsRequired ? a : b));
    const fresh = L.add(tree, {}, cap, deepest);
    check(`${summary.kind}: a gated talent is refused from an empty tree`,
          Boolean(fresh.reason) && /points spent/.test(fresh.reason), fresh.reason ?? "allowed");
  }

  // --- the rules held at every step ---------------------------------------
  /*
    Replay the order the points actually went in and assert all three rules at each step.
    Checked this way rather than by hunting for a conveniently shaped pair of nodes: no tree
    in the live data has a two-rank root whose child has no other parent, so any hand-picked
    example is either unavailable or reachable another way, and the test ends up asserting
    something other than the rule.

    The third rule is the one most easily got wrong: the DP attaches a node's children to its
    *last* rank, so a parent must be **fully ranked**, not merely taken.
  */
  const granted = L.grantedRoots(tree);
  let stepFailures = [];
  for (let seed = 1; seed <= 6; seed++) {
    const wanted = buildRandom(tree, cap, seed);
    const replay = L.place(tree, wanted, cap);
    const sofar = {};
    let spentSoFar = 0;
    for (const nodeId of replay.order) {
      const node = tree.nodes.find((n) => n.nodeId === nodeId);
      const before = sofar[String(nodeId)] ?? 0;
      if (spentSoFar >= cap) stepFailures.push(`${node.name}: placed past the cap`);
      if (spentSoFar < node.pointsRequired) {
        stepFailures.push(`${node.name}: gate ${node.pointsRequired} with ${spentSoFar} spent`);
      }
      if (before === 0 && node.parents.length > 0) {
        const wayIn = node.parents.some((p) => {
          if (granted.has(p)) return true;
          const parent = tree.nodes.find((n) => n.nodeId === p);
          return parent && (sofar[String(p)] ?? 0) >= parent.maxPoints;
        });
        if (!wayIn) stepFailures.push(`${node.name}: no fully-ranked parent`);
      }
      sofar[String(nodeId)] = before + 1;
      spentSoFar += 1;
    }
  }
  check(`${summary.kind}: every point was legal when it was placed`,
        stepFailures.length === 0, stepFailures.slice(0, 3).join("; "));

  // --- removal cascades rather than corrupting ----------------------------
  const built = buildRandom(tree, cap, 7);
  const roots = tree.nodes.filter((n) => n.parents.length === 0 && (built[String(n.nodeId)] ?? 0) > 0);
  if (roots.length) {
    const after = L.remove(tree, built, cap, roots[0]);
    const replaced = L.place(tree, after.points, cap);
    check(`${summary.kind}: what survives a removal is still constructible`,
          L.total(replaced.points) === L.total(after.points) &&
            Object.keys(replaced.dropped).length === 0,
          `${L.total(after.points)} kept, ${Object.keys(after.dropped).length} dropped`);
  }

  // --- the cap is never exceeded ------------------------------------------
  const over = L.place(
    tree,
    Object.fromEntries(tree.nodes.map((n) => [String(n.nodeId), n.maxPoints])),
    cap,
  );
  check(`${summary.kind}: asking for everything stops at the cap`,
        over.spent === cap, `${over.spent} of ${cap}`);
}

await server.close();

console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all loadout tests passed");
