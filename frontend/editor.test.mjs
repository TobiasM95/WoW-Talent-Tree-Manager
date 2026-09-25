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
await page.waitForSelector('section[aria-label="Tree grid"]');
check("the custom game opens on the editor", (await page.locator('nav[aria-label="Workflow"] [aria-current="step"]').innerText()).includes("Design"));
check("with a project ready to design", (await page.locator('input[aria-label="Project name"]').count()) === 1);

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

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall editor checks passed");
process.exit(failures.length ? 1 : 0);
