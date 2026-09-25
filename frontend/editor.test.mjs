/**
 * The tree editor, driven like a person would: build a tree by clicking, connect, move, undo,
 * save, plan with it, share it. Then the fidelity check that matters most -- a copied tree is
 * the same tree.
 *
 *   node editor.test.mjs [url]
 */
import { chromium } from "playwright";

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

const api = async (path, body) => {
  const r = await fetch(`${url}/api${path}`, body ? {
    method: "POST", headers: { "content-type": "application/json" }, body: JSON.stringify(body),
  } : undefined);
  return r.json();
};

await waitForServer(url);
await waitForServer(`${url}/api/health`);
const browser = await chromium.launch();
const context = await browser.newContext({ viewport: { width: 1600, height: 1000 } });
const page = await context.newPage();
page.on("pageerror", (e) => check("no page error", false, String(e)));

await page.goto(url, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");
await page.locator('button:text-is("Custom")').click();
await page.waitForSelector('section[aria-label="Start from"]');
check("the custom game opens on the editor", (await page.locator('nav[aria-label="Workflow"] [aria-current="step"]').innerText()).includes("Design"));
check("first asking what to start from", /Start a new project/.test(await page.locator('section[aria-label="Start from"]').innerText()));
// Classic, because the count below is worked out by hand from 5 points a row.
await page.locator('button:has-text("Classic style")').click();
await page.waitForSelector('section[aria-label="Tree grid"]');
check("with a project ready to design", (await page.locator('input[aria-label="Project name"]').count()) === 1);
check("a classic tree is four columns", (await page.locator('[data-cell^="0,"]').count()) === 4);

// Where a talent can go is visible before anything is there, not only on hover.
const outline = await page.locator('[data-cell="3,2"]').evaluate((e) => getComputedStyle(e).borderTopColor);
check("empty cells are visible without hovering", !/rgba\(.*,\s*0\)$|transparent/.test(outline), outline);

await page.locator('input[aria-label="Project name"]').fill("Test brew");

const cell = (r, c) => page.locator(`[data-cell="${r},${c}"]`);
const talent = (name) => page.locator(`.editor-node[aria-label^="${name},"]`);
const nameField = page.locator('input[aria-label="Talent name"]');

// Build: Strength (3 ranks), Grit (2), Cleave (an ability, requires Strength), a choice.
await cell(0, 0).click();
await nameField.fill("Strength");
await page.locator('input[aria-label="Ranks"]').fill("3");
await cell(0, 1).click();
await nameField.fill("Grit");
await page.locator('input[aria-label="Ranks"]').fill("2");
await cell(1, 0).click();
await nameField.fill("Cleave");
await page.locator('section[aria-label="Talent"] button:text-is("Ability")').click();
await cell(1, 1).click();
await nameField.fill("Temper");
await page.locator('section[aria-label="Talent"] button:text-is("Choice of two")').click();
await page.locator('input[aria-label="Alternative 1 name"]').fill("Fury");
await page.locator('input[aria-label="Alternative 2 name"]').fill("Calm");

check("four talents on the grid", (await page.locator(".editor-node").count()) === 4);
check("an ability draws square, a choice as a hexagon",
  (await talent("Cleave").getAttribute("aria-label")).includes("active") &&
  (await talent("Temper").getAttribute("aria-label")).includes("choice"));

// Connect Strength -> Cleave, then try the loop back.
await page.locator('button:text-is("Connect")').click();
await talent("Strength").click();
await talent("Cleave").click();
check("a connection draws an arrow", (await page.locator('section[aria-label="Tree grid"] svg line').count()) === 1);
await talent("Cleave").click();
await talent("Strength").click();
const refusal = await page.locator('section[aria-label="Tree grid"]').innerText();
check("a loop is refused as it is drawn", /require itself/.test(refusal) &&
  (await page.locator('section[aria-label="Tree grid"] svg line').count()) === 1);
await page.locator('button:text-is("Select / move")').click();

// Move Grit, then undo the move.
await talent("Grit").dragTo(cell(0, 2));
check("dragging moves a talent", (await page.locator('[data-cell="0,1"]').count()) === 1);
await page.keyboard.press("Control+z");
check("undo puts it back", (await page.locator('[data-cell="0,2"]').count()) === 1 && (await page.locator('[data-cell="0,1"]').count()) === 0);

// Save and plan.
await page.locator('button:text-is("Save")').click();
await page.waitForSelector('[data-save-state="saved"]', { timeout: 15000 });
check("saving says so", /Saved as/.test(await page.locator("aside").innerText()));
await page.locator('button:has-text("Plan with it")').click();
await page.waitForSelector("section.ttm-tree .ttm-node", { timeout: 15000 });
const planned = await page.locator("section.ttm-tree").evaluateAll((els) => els.map((e) => e.getAttribute("aria-label")));
check("the planner shows the designed tree", planned.join() === "Tree 1", planned.join());
// Waited for: the count arrives a moment after the trees do, and an empty value reads as 0.
await page.waitForFunction(() => document.querySelector("[data-total]")?.getAttribute("data-total"), null, { timeout: 15000 });
const count = Number(await page.locator("[data-total]").getAttribute("data-total"));
check("and counts its builds", count > 1, String(count));

// The count is the design's, checked against the rules by hand: 3+2+1+1 = 7 ranks, gates of
// 0 and 5, so at the full budget of 7 only one build exists, with two sides for the choice.
check("every rank bought is exactly the one build, both choice sides", count === 2, String(count));

// A link opens the same project in a fresh browser.
const link = page.url();
check("the link names the custom tree", /t=custom/.test(link), link);
const other = await browser.newContext({ viewport: { width: 1600, height: 1000 } });
const fresh = await other.newPage();
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector("section.ttm-tree .ttm-node", { timeout: 20000 });
check("a fresh browser opens it from the link", (await fresh.locator("section.ttm-tree .ttm-node").count()) === 4);
check("and keeps it in its own project list",
  (await fresh.locator('[aria-label="Projects"] button:has-text("Test brew")').count()) === 1);
await other.close();
await page.screenshot({ path: "shots/editor-plan.png" });

// Fidelity: a copy of Forever's Warrior trees counts exactly like the originals.
await page.locator('button:text-is("WoW Forever")').click();
await page.locator('button:text-is("Warrior")').click();
await page.waitForSelector('section[aria-label="Arms"] .ttm-node');
await page.waitForTimeout(800);
await page.locator('button:has-text("Edit a copy in the tree editor")').click();
await page.waitForSelector('section[aria-label="Tree grid"] .editor-node');
const tabs = await page.locator('[aria-label="Trees"] [role="tab"]').allInnerTexts();
check("a copy brings all three tabs into the editor", tabs.join() === "Arms,Fury,Protection", tabs.join());
check("with their talents", (await page.locator(".editor-node").count()) > 10);
await page.screenshot({ path: "shots/editor-copy.png" });
await page.locator('button:text-is("Save")').click();
await page.waitForSelector('[data-save-state="saved"]', { timeout: 20000 });
const pid = /Saved as ([0-9a-f]{8})/.exec(await page.locator("aside").innerText())?.[1];
const projects = await page.evaluate(() => JSON.parse(localStorage.getItem("ttm.projects.v1") || "[]"));
const saved = projects.find((p) => p.savedAs?.startsWith(pid ?? "-"))?.savedAs;
check("the copy saves", Boolean(saved), String(pid));
if (saved) {
  let same = 0;
  const budgets = [5, 11, 20, 31];
  for (const [i, name] of ["arms", "fury", "protection"].entries()) {
    for (const points of budgets) {
      const a = await api("/counts", { treeKey: `forever/warrior/${name}`, points });
      const b = await api("/counts", { treeKey: `custom/${saved}/${i}`, points });
      if (a.sets === b.sets && a.builds === b.builds) same++;
      else console.log(`   ${name} at ${points}: original ${a.sets}, copy ${b.sets}`);
    }
  }
  check("the copy counts exactly like the original, every tab and budget", same === 12, `${same} of 12`);
}

// Retail: a new project copied from a real spec. Barriers come across as barriers, granted
// talents stay granted, and the copy counts exactly like the original.
await page.locator('button:text-is("Custom")').click();
await page.locator('[aria-label="Projects"] button:has-text("New project")').click();
const picker = page.locator('section[aria-label="Start from"]');
await picker.locator('button:text-is("Retail")').click();
await picker.locator('select[aria-label="Template class"]').selectOption("Death Knight");
await picker.locator('select[aria-label="Template spec"]').selectOption("Blood");
await picker.locator('button:has-text("Copy 2 trees")').click();
await page.waitForSelector('section[aria-label="Tree grid"] .editor-node');
const retailTabs = await page.locator('[aria-label="Trees"] [role="tab"]').allInnerTexts();
check("a retail template brings its class and spec trees", retailTabs.length === 2, retailTabs.join());
check("with retail's gates as barriers", (await page.locator("[data-barrier]").count()) >= 2);
check("and its granted talents still free", (await page.locator(".editor-node[data-granted]").count()) >= 1);
check("on a retail-width grid", (await page.locator('[data-cell^="9,"], .editor-node').count()) > 0 &&
  (await page.evaluate(() => Math.max(...[...document.querySelectorAll("[data-cell]")].map((e) => Number(e.dataset.cell.split(",")[1]))))) >= 8);

// A blank retail tree beside them: two barriers to start, and one more from the margin.
await page.locator('[aria-label="Trees"] button:text-is("+ tree")').click();
await picker.locator('button:has-text("Retail style")').click();
await page.waitForSelector('[aria-label="Tree style"]');
check("a blank retail tree starts with two barriers", (await page.locator("[data-barrier]").count()) === 2);
await page.locator('button[aria-label="Add a barrier above row 2"]').click();
const added = await page.locator('input[aria-label="Points to pass the barrier above row 2"]').inputValue();
check("the margin adds a barrier below the first", (await page.locator("[data-barrier]").count()) === 3 && added === "5", added);
await page.locator('button:text-is("Remove this tree")').click();
await page.screenshot({ path: "shots/editor-retail.png" });

await page.locator('button:text-is("Save")').click();
await page.waitForSelector('[data-save-state="saved"]', { timeout: 20000 });
const rpid = /Saved as ([0-9a-f]{8})/.exec(await page.locator("aside").innerText())?.[1];
const rsaved = (await page.evaluate(() => JSON.parse(localStorage.getItem("ttm.projects.v1") || "[]")))
  .find((p) => p.savedAs?.startsWith(rpid ?? "-"))?.savedAs;
check("the retail copy saves", Boolean(rsaved), String(rpid));
if (rsaved) {
  let same = 0;
  const budgets = [8, 15, 25, 34];
  for (const [i, key] of ["retail/6/250/class", "retail/6/250/spec"].entries()) {
    for (const points of budgets) {
      const a = await api("/counts", { treeKey: key, points });
      const b = await api("/counts", { treeKey: `custom/${rsaved}/${i}`, points });
      if (a.sets === b.sets && a.builds === b.builds) same++;
      else console.log(`   ${key} at ${points}: original ${a.sets}/${a.builds}, copy ${b.sets}/${b.builds}`);
    }
  }
  check("the retail copy counts exactly like the original, both trees and every budget", same === 8, `${same} of 8`);
}

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall editor checks passed");
process.exit(failures.length ? 1 : 0);
