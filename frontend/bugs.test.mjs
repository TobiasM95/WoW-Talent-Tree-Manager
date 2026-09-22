/**
 * The six reported bugs, checked against the running stack rather than against my reading
 * of the diff.
 */
import { chromium } from "playwright";
import { mkdir } from "node:fs/promises";

const url = process.argv[2] ?? "http://localhost:8081";
const outDir = process.argv[3] ?? "shots";
await mkdir(outDir, { recursive: true });

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

const nodes = () => page.locator(".ttm-node");
const settle = () => page.waitForTimeout(600);

await page.goto(url, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node", { timeout: 20000 });

/* --- shapes ------------------------------------------------------------- */
const shapes = await page.$$eval(".ttm-node", (els) => {
  const out = {};
  for (const el of els) {
    const s = el.dataset.shape;
    out[s] = (out[s] ?? 0) + 1;
  }
  return out;
});
console.log("   shapes:", JSON.stringify(shapes));
check("active abilities are square", (shapes.active ?? 0) > 0);
check("choice nodes are hexagons", (shapes.choice ?? 0) > 0);
check("passives are circles", (shapes.passive ?? 0) > 0);

const clip = await page.$eval('.ttm-node[data-shape="choice"] .ttm-node-ring', (el) =>
  getComputedStyle(el).clipPath,
);
check("the hexagon reaches the ring layer", clip.includes("polygon"), clip);

/* --- the constraint sandwich -------------------------------------------- */
const active = page.locator('[data-active="yes"]');
await active.locator(".ttm-node").first().click();
await settle();
const ring = await page.$eval('.ttm-node[data-state="required"]', (el) => {
  const r = el.querySelector(".ttm-node-ring");
  const b = el.querySelector(".ttm-node-band");
  const f = el.querySelector(".ttm-node-face");
  return {
    keyline: getComputedStyle(r).backgroundColor,
    band: getComputedStyle(b).backgroundColor,
    inner: getComputedStyle(f).boxShadow,
    glow: getComputedStyle(r).filter,
  };
});
console.log("   required ring:", JSON.stringify(ring));
check("a required node has a keyline under its band", ring.keyline !== ring.band);
check("and a keyline inside it too", ring.inner.includes("inset"));

/* --- the four constraint kinds are four colours -------------------------- */
const swatches = await page.$$eval(".ttm-swatch", (els) =>
  els
    .filter((el) => el.dataset.state)
    .map((el) => [el.dataset.state, getComputedStyle(el.querySelector(".ttm-swatch-band")).backgroundColor]),
);
console.log("   legend:", JSON.stringify(swatches));
check("the legend names all four constraint kinds", swatches.length === 4);
check("each has its own colour", new Set(swatches.map(([, c]) => c)).size === 4);

const legendText = await page.locator("aside").innerText();
check("at-least-one is described", /one or more of the group/i.test(legendText));
check("exactly-one is described", /one of the group only/i.test(legendText));

await page.screenshot({ path: `${outDir}/bugs-explore.png`, fullPage: false });

/* --- mode ownership ------------------------------------------------------ */
// innerText comes back through the stylesheet uppercase transform, so these compare
// case-insensitively -- matching case made every negative assertion pass for free.
const sidebarHas = async (text) =>
  new RegExp(text, "i").test(await page.locator("aside").innerText());
check("Explore owns the point budget", await sidebarHas("Point budget"));
check("Explore owns constraints", await sidebarHas("Constraints"));
check("Explore does not own the talent string", !(await sidebarHas("Talent string")));

await page.locator('button:has-text("Build")').first().click();
await settle();
check("Build does not show the point budget", !(await sidebarHas("Point budget")));
check("Build does not show constraints", !(await sidebarHas("Constraints")));
check("Build owns the talent string", await sidebarHas("Talent string"));

/* --- a shared loadout arrives clean -------------------------------------- */
await page.locator('[data-active="yes"] .ttm-node[data-reachable="yes"]').first().click();
await settle();
await page.locator('[data-active="yes"] .ttm-node[data-reachable="yes"]').first().click();
await settle();
const link = await page.evaluate(() => location.href);
console.log("   loadout link:", link.slice(0, 120));
check("a loadout link carries no constraints", !/[?&][rxeo]=/.test(link));
check("a loadout link carries no budget", !/[?&]p=/.test(link));
check("a loadout link carries the loadout", /[?&]b[cseh]=/.test(link));

const fresh = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector(".ttm-node", { timeout: 20000 });
await fresh.waitForTimeout(1200);
const freshText = await fresh.locator("aside").innerText();
check("a shared loadout opens in Build", !/Point budget/i.test(freshText));
check("with no results panel in sight", !/Builds|Simulate/i.test(freshText));
const carried = await fresh.locator('.ttm-node[data-spent="yes"]').count();
check("and the points arrive", carried > 0, `${carried} nodes spent`);

await fresh.locator('button:has-text("Explore")').first().click();
await fresh.waitForTimeout(800);
const budget = await fresh.locator("#points").getAttribute("value");
const cap = await fresh.locator("#points").getAttribute("max");
check("its budget starts at the cap, not 30", budget === cap, `${budget}/${cap}`);
await fresh.screenshot({ path: `${outDir}/bugs-shared.png` });

/* --- and the same in the dark --------------------------------------------- */
const dark = await browser.newPage({
  viewport: { width: 1600, height: 1000 },
  colorScheme: "dark",
});
await dark.goto(url, { waitUntil: "networkidle" });
await dark.waitForSelector(".ttm-node", { timeout: 20000 });
await dark.locator('[data-active="yes"] .ttm-node').first().click();
await dark.waitForTimeout(600);
const night = await dark.$eval('.ttm-node[data-state="required"]', (el) => ({
  keyline: getComputedStyle(el.querySelector(".ttm-node-ring")).backgroundColor,
  band: getComputedStyle(el.querySelector(".ttm-node-band")).backgroundColor,
  glow: getComputedStyle(el.querySelector(".ttm-node-ring")).filter,
}));
console.log("   dark required ring:", JSON.stringify(night));
check("the dark theme keeps its own palette", night.band !== ring.band, night.band);
check("and still lays a keyline under it", night.keyline !== night.band);
check("and glows, which the plate does not", !night.glow.includes("0px 0px 0px)"));
const darkLegend = await dark.$$eval(".ttm-swatch[data-state]", (els) =>
  els.map((el) => getComputedStyle(el.querySelector(".ttm-swatch-band")).backgroundColor),
);
check("four distinct colours in the dark too", new Set(darkLegend).size === 4);
await dark.screenshot({ path: `${outDir}/bugs-dark.png` });

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall bug checks passed");
process.exit(failures.length ? 1 : 0);
