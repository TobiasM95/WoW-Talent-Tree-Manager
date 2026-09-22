/**
 * The one path that crosses between the two modes.
 *
 * Explore produces builds that live only as long as their job: no URL, gone on the next
 * search. Build owns loadouts, which are shareable and exportable. So there has to be a door
 * between them, and this walks through it -- enumerate, pick a result, take it into the
 * loadout, and check that what arrives is the build that was on screen and that it survives
 * being shared.
 */
import { chromium } from "playwright";

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
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("pageerror", (e) => check("no page error", false, String(e)));

// A hero tree is small enough to enumerate in full, which keeps this test about the bridge
// rather than about how long a 30-million-build count takes.
await page.goto(`${url}/?t=retail/6/250/hero/31&m=e`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node", { timeout: 20000 });
await page.waitForTimeout(1200);

const solve = page.locator('button:has-text("Enumerate")');
await solve.waitFor({ timeout: 15000 });
check("the count gate allows a hero tree", await solve.isEnabled());
await solve.click();

await page.waitForSelector('button:has-text("Use as my")', { timeout: 120000 });
const shown = await page.$$eval('[data-active="yes"] .ttm-node[data-spent="yes"]', (els) =>
  els.map((el) => el.getAttribute("aria-label")).sort(),
);
check("a result is drawn on the tree", shown.length > 0, `${shown.length} talents`);

await page.locator('button:has-text("Use as my")').click();
await page.waitForTimeout(900);

const sidebar = await page.locator("aside").innerText();
check("it lands in Build", /Talent string/i.test(sidebar));
check("and the results panel is gone with it", !/Builds/i.test(sidebar));

const taken = await page.$$eval('[data-active="yes"] .ttm-node[data-spent="yes"]', (els) =>
  els.map((el) => el.getAttribute("aria-label")).sort(),
);
check(
  "the loadout holds the build that was on screen",
  taken.length === shown.length && taken.every((t, i) => t === shown[i]),
  `${shown.length} enumerated vs ${taken.length} taken; missing ${JSON.stringify(
    shown.filter((t) => !taken.includes(t)),
  )}; extra ${JSON.stringify(taken.filter((t) => !shown.includes(t)))}`,
);

const link = await page.evaluate(() => location.href);
check("and it is shareable as a loadout", /[?&]m=b/.test(link) && /[?&]bh=/.test(link), link);

const fresh = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector(".ttm-node", { timeout: 20000 });
await fresh.waitForTimeout(1500);
// Scoped to the hero pane: the class and spec trees have granted talents of their own,
// which are drawn as taken because the character has them, and counting those here
// compares a hero tree against a whole character.
const reopened = await fresh.$$eval('[data-active="yes"] .ttm-node[data-spent="yes"]', (els) =>
  els.map((el) => el.getAttribute("aria-label")).sort(),
);
check(
  "the link reopens the same build",
  reopened.length === taken.length,
  `${taken.length} shared vs ${reopened.length} reopened`,
);

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nthe bridge holds");
process.exit(failures.length ? 1 : 0);
