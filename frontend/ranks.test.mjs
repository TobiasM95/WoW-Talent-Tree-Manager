/**
 * Rank limits on multi-rank talents, in the browser: paint "at least 2", cap it to "exactly 2",
 * make a one-point dip -- each count checked against the API asked directly -- then a link
 * reproduces them. Granted talents cannot be clicked at all. And the planner: in-game colours
 * (green while ranks remain, gold when full), one outline per talent, and the path lit up, and the
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
// A granted talent is not a choice: clicking it in a search changes nothing and says nothing.
const classTree = await (await fetch(`${url}/data/trees/retail/6/250/class.json`)).json();
const grantedNode = classTree.nodes.find((n) => n.preFilled && !n.parents.length);
const grantedEl = page.locator(`section[aria-label="Class"] .ttm-node[data-node-id="${grantedNode.nodeId}"]`);
await page.waitForTimeout(800);
const before = Number(await page.locator("[data-total]").getAttribute("data-total"));
// Forced: Playwright already refuses, reading aria-disabled; this proves a real click is inert too.
await grantedEl.click({ force: true });
await grantedEl.click({ button: "right", force: true });
await page.waitForTimeout(800);
check("a granted talent is marked and inert", (await grantedEl.getAttribute("aria-disabled")) === "true" &&
  (await grantedEl.getAttribute("data-granted")) !== null);
check("clicking it in a search paints nothing", (await grantedEl.getAttribute("data-state")) === "neutral");
check("and changes no count, with no error", Number(await page.locator("[data-total]").getAttribute("data-total")) === before &&
  !/granted automatically/.test(await page.locator("body").innerText()));

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
// The limit is drawn in the talent's own border (a conic gradient as --band): count its
// segments by colour.
const arcs = async (kind) => {
  const style = (await talent.getAttribute("style")) ?? "";
  const band = /--band:\s*(conic-gradient\([^;]*\))/.exec(style)?.[1] ?? "";
  return band.split(kind === "must" ? "var(--must)" : "var(--barred)").length - 1;
};

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

// The planner: in-game colours, one outline, and the path lit up.
await page.goto(`${url}/?t=forever%2Fwarrior%2Farms`, { waitUntil: "networkidle" }).catch(() => {});
await page.locator('button:text-is("WoW Forever")').click();
await page.locator('button:text-is("Warrior")').click();
await page.waitForSelector('section[aria-label="Arms"] .ttm-node');
await page.waitForTimeout(600);
const arms = page.locator('section[aria-label="Arms"]');
// Pinned by id: "the first reachable talent" is another one once this one is full.
const firstId = await arms.locator('.ttm-node[data-reachable="yes"]').first().getAttribute("data-node-id");
const first = arms.locator(`.ttm-node[data-node-id="${firstId}"]`);
const ranksOf = Number((await first.getAttribute("aria-label")).match(/(\d+) points?\./)[1]);
await first.click();
await first.click();
const rankBadge = async () => (await first.locator(".ttm-node-ranks").innerText()).trim();
check(`a partly spent talent is green, 2/${ranksOf}`, (await first.getAttribute("data-full")) === "no" && (await rankBadge()) === `2/${ranksOf}`,
  `${await first.getAttribute("data-full")} ${await rankBadge()}`);
for (let i = 2; i < ranksOf; i++) await first.click();
check(`and gold once full, ${ranksOf}/${ranksOf}`, (await first.getAttribute("data-full")) === "yes" && (await rankBadge()) === `${ranksOf}/${ranksOf}`);
check("one outline per talent: no second ring anywhere", (await page.locator(".ttm-rank-arc, .ttm-rank-arcs").count()) === 0);
// The path, deterministically: in a fresh Protection tab, a talent whose child opens once the
// talent is maxed. Maxed, the arrow to its child is green (it can go there next); the child
// taken, the arrow is gold (the build runs along it).
const prot = await (await fetch(`${url}/data/trees/forever/warrior/protection.json`)).json();
const byId = new Map(prot.nodes.map((n) => [n.nodeId, n]));
const parent = prot.nodes.find((n) => n.pointsRequired === 0 && n.children.some((c) => byId.has(c)));
const child = byId.get(parent.children.find((c) => byId.has(c)));
const protPane = page.locator('section[aria-label="Protection"]');
const childEl = protPane.locator(`.ttm-node[data-node-id="${child.nodeId}"]`);
for (let i = 0; i < parent.maxPoints; i++) await protPane.locator(`.ttm-node[data-node-id="${parent.nodeId}"]`).click();
// Its row gate: spend elsewhere in the tab until the child opens.
for (let i = 0; i < 30 && (await childEl.getAttribute("data-reachable")) !== "yes"; i++) {
  const other = protPane.locator(`.ttm-node[data-reachable="yes"]:not([data-node-id="${child.nodeId}"])`).first();
  if (!(await other.count())) break;
  await other.click();
}
await page.waitForTimeout(200);
const open = await protPane.locator('.ttm-edge[data-flow="open"]').count();
check(`maxing ${parent.name}, the way on to ${child.name} is green`, open > 0, String(open));
await protPane.locator(`.ttm-node[data-node-id="${child.nodeId}"]`).click();
await page.waitForTimeout(200);
const taken = await protPane.locator('.ttm-edge[data-flow="taken"]').count();
check("taking it, the path the build runs is gold", taken > 0, String(taken));
await page.screenshot({ path: "shots/ranks-planner.png" });

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall rank checks passed");
process.exit(failures.length ? 1 : 0);
