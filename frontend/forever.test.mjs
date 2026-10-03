/**
 * WoW Forever in the browser: three tabs, one pool of 51.
 *
 *   node forever.test.mjs [url]
 *
 * What makes Forever different from retail is the shared pool, so that is what this presses on:
 * points spent in one tab come out of the others, the pool stops at 51 however it is split,
 * the level follows the points, and a link reproduces the split. Plus the things that must not
 * leak across from retail -- Simulate, the talent string, choice and hero-tree shapes.
 */
import { chromium } from "playwright";
import { count } from "./testlib.mjs";

// Defaults to the built container, which is what every verification here runs against.
const url = process.argv[2] ?? "http://localhost:8081";

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
await waitForServer(`${url}/data/forever/index.json`);
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("pageerror", (e) => check("no page error", false, String(e)));

await page.goto(url, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");
await page.locator('button:text-is("WoW Forever")').click();
await page.locator('button:text-is("Warrior")').waitFor();
await page.locator('button:text-is("Warrior")').click();
await page.waitForSelector('section[aria-label="Arms"] .ttm-node');
await page.waitForTimeout(800);

const panes = await page.locator("section.ttm-tree").evaluateAll((els) => els.map((e) => e.getAttribute("aria-label")));
check("a class's three tabs are the three panes", panes.join() === "Arms,Fury,Protection", panes.join());
check("the class starts fixed and empty, like a calculator",
  (await page.locator('[aria-label="Class mode"] button[aria-pressed="true"]').innerText()) === "Fixed build");
check("with one switch for the class, none per tab", (await page.locator('section.ttm-tree [aria-label$="tree mode"]').count()) === 0);
check("with no points spent", (await page.locator("[data-split]").getAttribute("data-split")) === "0/0/0");

const shapes = await page.$$eval(".ttm-node", (els) => [...new Set(els.map((e) => e.dataset.shape))].sort().join());
check("talents draw as passives and abilities only", shapes === "active,passive", shapes);

// Spend: fill Arms as far as it goes, then Fury.
const fill = async (name) => {
  const pane = page.locator(`section[aria-label="${name}"]`);
  for (let i = 0; i < 70; i++) {
    const next = pane.locator('.ttm-node[data-reachable="yes"]').first();
    if (!(await next.count())) break;
    await next.click();
    await page.waitForTimeout(25);
  }
};
await fill("Arms");
const arms = Number((await page.locator("[data-split]").getAttribute("data-split")).split("/")[0]);
await fill("Fury");
await fill("Protection");
const split = (await page.locator("[data-split]").getAttribute("data-split")).split("/").map(Number);
const total = split.reduce((a, b) => a + b, 0);
check("the pool stops at 51 however it is split", total === 51, split.join("/"));
check("points in one tab come out of the others", split[0] === arms && split[1] + split[2] === 51 - arms, split.join("/"));
check("and 51 points takes level 60", /level 60/.test(await page.locator('section[aria-label="Talent points"]').innerText()));

// Nothing more can be spent anywhere.
const reachable = await page.locator('.ttm-node[data-reachable="yes"]').count();
check("with the pool spent, no talent anywhere takes another point", reachable === 0, `${reachable} still reachable`);

// A refund frees a point for any tab.
const lastArms = page.locator('section[aria-label="Arms"] .ttm-node[data-spent="yes"]').last();
await lastArms.click({ button: "right" });
await page.waitForTimeout(200);
check("a refund in one tab frees a point in the others",
  (await page.locator('section[aria-label="Protection"] .ttm-node[data-reachable="yes"]').count()) > 0);

// Retail's sim half does not leak in.
check("Simulate is not offered", await page.locator('nav[aria-label="Workflow"] button:has-text("Simulate")').isDisabled());
check("and says why", /does not simulate WoW Forever/.test(await page.locator("aside").innerText()));
check("no Blizzard talent string", (await page.locator("text=Talent string").count()) === 0);
check("the data is credited, as its licence asks", /talentsforever\.com/.test(await page.locator("aside").innerText()));

// Open: one search over all three tabs, counted over every split of the pool. Checked against
// a sum worked out here from each tab's counts one total at a time (the counter asked directly,
// testlib.mjs) -- not from the spreads and the pooling the page used.
const tabs = ["arms", "fury", "protection"];
const perPoints = async (tab, filters = {}) => {
  // Zero points is the empty tab: one build, unless something is required in it.
  const out = [filters.mustHave?.length ? 0 : 1];
  for (let k = 1; k <= 64; k++) {
    const r = await count(url, { treeKey: `forever/warrior/${tab}`, points: k, ...filters });
    if (r.status) break;
    out.push(r.builds);
  }
  return out;
};
const pooledBy = (vectors, budget, exact = []) => {
  let sum = 0n;
  const [a, b, c] = vectors;
  for (let x = 0; x < a.length && x <= budget; x++) {
    if (exact[0] != null && x !== exact[0]) continue;
    for (let y = 0; y < b.length && x + y <= budget; y++) {
      if (exact[1] != null && y !== exact[1]) continue;
      const z = budget - x - y;
      if (z >= c.length || (exact[2] != null && z !== exact[2])) continue;
      sum += BigInt(a[x]) * BigInt(b[y]) * BigInt(c[z]);
    }
  }
  return Number(sum);
};
const vectors = await Promise.all(tabs.map((t) => perPoints(t)));
const shownTotal = async () => {
  await page.waitForTimeout(900);
  return Number(await page.locator("[data-total]").getAttribute("data-total"));
};

await page.locator('[aria-label="Class mode"] button:text-is("Open search")').click();
const open51 = await shownTotal();
const expect51 = pooledBy(vectors, 51);
check("an open class counts every split of the whole pool", Math.abs(open51 - expect51) / expect51 < 1e-12,
  `${open51} vs ${expect51}`);
check("and the tabs no longer count alone", (await page.locator("section.ttm-tree [data-count-for]").count()) === 0);

await page.locator('button[aria-label="One point fewer"]').click();
await page.locator('button[aria-label="One point fewer"]').click();
const open49 = await shownTotal();
check("fewer points to spend, fewer builds, still every split", open49 === pooledBy(vectors, 49), `${open49} vs ${pooledBy(vectors, 49)}`);
check("and 49 points takes level 58", /level 58/.test(await page.locator('section[aria-label="Talent points"]').innerText()));

await page.locator('select[aria-label="Points in Arms"]').selectOption("31");
const arms31 = await shownTotal();
check("a tab held to exactly 31 counts only those splits", arms31 === pooledBy(vectors, 49, [31]), `${arms31} vs ${pooledBy(vectors, 49, [31])}`);
check("and the splits say so", /^31 \/ /.test(await page.locator('[aria-label="Splits"] li').first().innerText()));

// Painting still narrows, per tab: require a Fury talent.
const furyTalent = page.locator('section[aria-label="Fury"] .ttm-node').first();
const furyId = Number(await furyTalent.getAttribute("data-node-id"));
await furyTalent.click();
const required = await shownTotal();
const furyRequired = await perPoints("fury", { mustHave: [furyId] });
const expectRequired = pooledBy([vectors[0], furyRequired, vectors[2]], 49, [31]);
check("painting a tab narrows the whole class, exactly", required === expectRequired && required < arms31,
  `${required} vs ${expectRequired}`);
await page.screenshot({ path: "shots/forever-open.png" });

// A link reproduces all of it.
const link = page.url();
check("the link names a Forever tree", /t=forever/.test(link), link);
const fresh = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector('section[aria-label="Arms"] .ttm-node');
await fresh.waitForTimeout(1200);
await fresh.waitForTimeout(800);
const reopened = Number(await fresh.locator("[data-total]").getAttribute("data-total"));
check("and reopens on the same search: 49 points, 31 in Arms, the Fury talent", reopened === required, `${reopened} vs ${required}`);
check("in WoW Forever", (await fresh.locator('button[aria-pressed="true"]:text-is("WoW Forever")').count()) === 1);
await page.screenshot({ path: "shots/forever.png" });

// And retail is still there.
await page.locator('button:text-is("Retail")').click();
await page.waitForTimeout(1500);
const retailPanes = await page.locator("section.ttm-tree").evaluateAll((els) => els.map((e) => e.getAttribute("aria-label")));
check("switching back to retail brings its trees back", retailPanes.join() === "Class,Spec,Hero", retailPanes.join());

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall Forever checks passed");
process.exit(failures.length ? 1 : 0);
