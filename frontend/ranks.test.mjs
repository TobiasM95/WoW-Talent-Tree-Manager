/**
 * Rank limits on multi-rank talents, in the browser: paint "at least 2", cap it to "exactly 2",
 * make a one-point dip -- each count checked against the API asked directly -- then a link
 * reproduces them. And the planner: a partly spent talent shows its ranks as arcs, and the
 * path through the tree lights up.
 *
 *   node ranks.test.mjs [url]
 */
import { chromium } from "playwright";
import { count } from "./testlib.mjs";

// Defaults to the built container, which is what every verification here runs against.
const url = process.argv[2] ?? "http://localhost:8081";
const SPEC = "retail/6/250/spec";

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};

async function waitForServer(target, timeoutMs = 30000) {
  const deadline = Date.now() + timeoutMs;
  for (;;) {
    try {
      const r = await fetch(target, { redirect: "manual" });
      if (r.status < 500) return;
    } catch {
      /* not listening */
    }
    if (Date.now() > deadline) throw new Error(`${target} silent after ${timeoutMs}ms`);
    await new Promise((r) => setTimeout(r, 250));
  }
}
await waitForServer(url);
await waitForServer(`${url}/data/retail/index.json`);

const api = async (body) => (await count(url, body)).builds;

const spec = await (await fetch(`${url}/data/trees/${SPEC}.json`)).json();
const target = spec.nodes
  .filter((n) => n.maxPoints >= 3 && !n.preFilled && n.kind !== "choice")
  .sort((a, b) => a.pointsRequired - b.pointsRequired || a.row - b.row)[0];
const id = String(target.nodeId);
const cap = spec.pointCap ?? spec.maxPointsInTree;
console.log(`   limiting ${target.name} (${target.maxPoints} ranks) on Blood's spec tree`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("pageerror", (e) => check("no page error", false, String(e)));

await page.goto(`${url}/?t=${encodeURIComponent(SPEC)}`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");
// Only the spec tree varies: fixed trees contribute one build each, so the total is its count.
for (const pane of ["Class", "Hero"]) await page.locator(`section[aria-label="${pane}"] .seg button:text-is("Fixed")`).click();
await page.locator(`section[aria-label="Spec"] .seg button:text-is("Open")`).click();

const talent = page.locator(`section[aria-label="Spec"] .ttm-node[data-node-id="${id}"]`);
const total = async () => {
  await page.waitForTimeout(700);
  await page.waitForFunction(() => !document.querySelector('[data-stale]'), null, { timeout: 15000 }).catch(() => {});
  return Number(await page.locator("[data-total]").getAttribute("data-total"));
};
const badge = async () => (await talent.locator("[data-limit]").count()) ? talent.locator("[data-limit]").innerText() : "";
const arcs = async (kind) => talent.locator(`.ttm-rank-arc[data-kind="${kind}"]`).count();

const whole = await total();
check("the open spec tree alone is the whole count", whole === (await api({ treeKey: SPEC, points: cap })), String(whole));

await talent.click();
await talent.click();
check("two clicks: at least 2 ranks", (await talent.getAttribute("data-range")) === `2-${target.maxPoints}` && (await badge()) === "2+",
  `${await talent.getAttribute("data-range")} ${await badge()}`);
check("drawn as two green arcs", (await arcs("must")) === 2);
const atLeast2 = await total();
check("and counted exactly", atLeast2 === (await api({ treeKey: SPEC, points: cap, mustHave: [target.nodeId], rankMin: { [id]: 2 } })),
  String(atLeast2));

await page.locator('[aria-label="Paint tool"] button:text-is("At most")').click();
for (let i = 0; i < target.maxPoints - 2; i++) await talent.click();
check("At most brings the cap down to exactly 2", (await badge()) === "=2" && (await arcs("barred")) === target.maxPoints - 2, await badge());
const exactly2 = await total();
const expect2 = await api({ treeKey: SPEC, points: cap, mustHave: [target.nodeId], rankMin: { [id]: 2 }, rankMax: { [id]: 2 } });
check("exactly 2, counted exactly", exactly2 === expect2, `${exactly2} vs ${expect2}`);
await page.screenshot({ path: "shots/ranks.png" });

// A link carries the limit and lands on the same count.
const link = page.url();
check("the link carries the rank limit", /sn=/.test(link), link);
const fresh = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector(".ttm-node");
await fresh.waitForTimeout(1500);
check("and reopens on it", (await fresh.locator(`section[aria-label="Spec"] .ttm-node[data-node-id="${id}"]`).getAttribute("data-range")) === "2-2");
check("with the same count", Number(await fresh.locator("[data-total]").getAttribute("data-total")) === exactly2);
await fresh.close();

// A one-point dip: clear, then cap a free talent at 1.
await page.locator('[aria-label="Paint tool"] button:text-is("Require / bar")').click();
for (let i = 0; i < 20 && (await talent.getAttribute("data-range")); i++) await talent.click({ button: "right" });
await page.locator('[aria-label="Paint tool"] button:text-is("At most")').click();
for (let i = 0; i < target.maxPoints - 1; i++) await talent.click();
check("a free talent capped at 1 reads as a dip", (await badge()) === "≤1", await badge());
const dip = await total();
check("and counts as none-or-one rank", dip === (await api({ treeKey: SPEC, points: cap, rankMax: { [id]: 1 } })), String(dip));

// The planner: arcs for a partly spent talent, and the path lit up.
await page.goto(`${url}/?t=forever%2Fwarrior%2Farms`, { waitUntil: "networkidle" }).catch(() => {});
await page.locator('button:text-is("WoW Forever")').click();
await page.locator('button:text-is("Warrior")').click();
await page.waitForSelector('section[aria-label="Arms"] .ttm-node');
await page.waitForTimeout(600);
const arms = page.locator('section[aria-label="Arms"]');
const first = arms.locator('.ttm-node[data-reachable="yes"]').first();
const ranksOf = Number((await first.getAttribute("aria-label")).match(/(\d+) points?\./)[1]);
await first.click();
await first.click();
check("a partly spent talent shows its ranks as arcs: 2 gold of " + ranksOf,
  (await first.locator('.ttm-rank-arc[data-kind="spent"]').count()) === 2 &&
  (await first.locator(".ttm-rank-arc").count()) === ranksOf);
// Spend on down the tree: a build that runs through arrows must light them.
for (let i = 0; i < 20; i++) {
  const next = arms.locator('.ttm-node[data-reachable="yes"]').last();
  if (!(await next.count())) break;
  await next.click();
  await page.waitForTimeout(25);
}
const taken = await arms.locator('.ttm-edge[data-flow="taken"]').count();
const open = await arms.locator('.ttm-edge[data-flow="open"]').count();
check("the path a build takes is gold", taken > 0, String(taken));
check("and where it could go next is green", open > 0, String(open));
await page.screenshot({ path: "shots/ranks-planner.png" });

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall rank checks passed");
process.exit(failures.length ? 1 : 0);
