/**
 * Produce a real SimulationCraft export from a real job, so the importer can be built
 * against a report the actual tool wrote rather than against a reading of its source.
 *
 *   node export-sample.mjs [url] [outFile]
 */
import { chromium } from "playwright";
import { writeFile } from "node:fs/promises";

const url = process.argv[2] ?? "http://localhost:8081";
const out = process.argv[3] ?? "sample.simc";

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("pageerror", (e) => console.log("PAGE ERROR", String(e)));

// A hero tree: small enough to enumerate whole and to sim in a minute.
await page.goto(`${url}/?t=retail/6/250/hero/31&m=b`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node", { timeout: 20000 });
await page.waitForTimeout(1200);

// A base loadout has to exist before an export means anything: every line is a whole
// character, so the class and spec trees must hold something.
for (let i = 0; i < 40; i++) {
  const next = page.locator('[data-active="yes"] .ttm-node[data-reachable="yes"]').first();
  if (!(await next.count())) break;
  await next.click();
  await page.waitForTimeout(60);
}
for (const pane of [0, 1]) {
  await page.locator("[data-active]").nth(pane).click();
  await page.waitForTimeout(500);
  for (let i = 0; i < 40; i++) {
    const next = page.locator('[data-active="yes"] .ttm-node[data-reachable="yes"]').first();
    if (!(await next.count())) break;
    await next.click();
    await page.waitForTimeout(60);
  }
}

// Point the solver back at the hero tree and enumerate it.
await page.locator("[data-active]").nth(2).click();
await page.waitForTimeout(400);
await page.locator('button:has-text("Explore")').first().click();
await page.waitForTimeout(1500);

// A hero tree at its full 13 points has exactly one build -- every talent is taken. Drop
// the budget and the tree becomes a real comparison set.
await page.locator("#points").fill("8");
await page.locator("#points").dispatchEvent("change");
await page.waitForTimeout(1500);

const solve = page.locator('button:has-text("Enumerate")');
await solve.waitFor({ timeout: 15000 });
await solve.click();
await page.waitForSelector('button:has-text("Export")', { timeout: 120000 });

// "runnable" adds a minimal profile above the lines, which is what makes this file something
// SimulationCraft can be handed on its own.
// Scoped to the Simulate panel: the statistics panel has a checkbox of its own and it
// comes first in the sidebar.
await page.locator('label:has-text("runnable") input[type="checkbox"]').check();
await page.waitForTimeout(300);
await page.locator('button:has-text("Export")').first().click();
await page.waitForSelector('textarea[aria-label="SimulationCraft profilesets"]', { timeout: 60000 });

const text = await page.locator('textarea[aria-label="SimulationCraft profilesets"]').inputValue();
await writeFile(out, text, "utf8");
console.log(`${text.split("\n").filter((l) => l.startsWith("profileset.")).length} profilesets -> ${out}`);
console.log(text.split("\n").slice(0, 14).join("\n"));

await browser.close();
