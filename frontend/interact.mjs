/**
 * Drives the real interactions against the real stack, and screenshots the states that
 * only exist mid-flight.
 *
 *   node interact.mjs [url] [outDir]
 *
 * A build passing and a page rendering say nothing about whether clicking a talent actually
 * changes the number, whether a tooltip lands on screen, or what an enumeration looks like
 * while it runs. Those are the states worth checking, and the last two cannot be captured
 * by loading a page and waiting.
 */
import { chromium } from "playwright";
import { mkdir } from "node:fs/promises";

const url = process.argv[2] ?? "http://localhost:5173";
const outDir = process.argv[3] ?? "shots";
await mkdir(outDir, { recursive: true });

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};

const browser = await chromium.launch();
const context = await browser.newContext({
  viewport: { width: 1440, height: 900 },
  colorScheme: "dark",
});
const page = await context.newPage();
page.on("pageerror", (error) => failures.push(`page error: ${error.message}`));

await page.goto(url, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");

const countText = () => page.locator(".display.tabular").first().innerText();
const stale = () => page.locator(".ttm-node[data-stale]").count();

/*
  Wait for the gate to actually answer.

  Waiting only for the stale flag to clear is a race: React commits the constraint change
  and sets the flag a tick after the click event returns, so a check that runs immediately
  sees a clean canvas and reads the *previous* count. That produced a "before and after are
  identical" failure that looked like an app bug and was a test bug.

  So: wait for the flag to appear, then for it to clear.
*/
const settled = async () => {
  await page
    .waitForFunction(() => !!document.querySelector(".ttm-node[data-stale]"), { timeout: 2000 })
    .catch(() => {
      /* nothing was in flight -- the payload did not change */
    });
  await page.waitForFunction(
    () => !document.querySelector(".ttm-node[data-stale]"),
    { timeout: 15000 },
  );
};

await settled();
const baseline = await countText();
check("a count is shown on load", /\d/.test(baseline), baseline);

// --- painting constraints --------------------------------------------------
/*
  Find a talent that is genuinely optional at this budget, which is narrower than "any
  talent".

  Two degenerate cases have to be excluded or the assertions pass for the wrong reason. The
  top rows of a spec tree are in *every* build at a realistic budget, so requiring one
  changes nothing. A talent deep behind a point gate is in *no* build at this budget, so
  requiring it yields zero and barring it changes nothing -- which still satisfies a naive
  "the number moved" check while testing almost nothing.

  So: require it, and insist the result is both non-zero and strictly smaller.
*/
const nodes = page.locator(".ttm-node");
const total = await nodes.count();
const asNumber = (text) => Number(text.replace(/[^0-9]/g, ""));
const base = asNumber(baseline);

let chosen = null;
for (let i = total - 1; i >= 0 && !chosen; i--) {
  const node = nodes.nth(i);
  await node.click({ force: true });
  await settled();
  const value = asNumber(await countText());
  if (value > 0 && value < base) chosen = { index: i, required: value };
  else {
    // Cycle it back to neutral rather than leaving a constraint behind.
    await node.click({ force: true });
    await node.click({ force: true });
    await settled();
  }
}
check(
  "requiring an optional talent narrows the count without emptying it",
  Boolean(chosen),
  `from ${base}`,
);

check(
  "the talent reads as required",
  (await page.locator('.ttm-node[data-state="required"]').count()) === 1,
);

if (chosen) {
  // Barring the same talent must also change the count, and differently: every build either
  // takes it or does not, so required + barred should account for the whole space.
  await nodes.nth(chosen.index).click({ force: true });
  await settled();
  const barred = asNumber(await countText());
  check("barring it gives a different, non-empty count",
        barred > 0 && barred !== chosen.required, `${barred} vs ${chosen.required}`);
  check(
    "the talent reads as barred",
    (await page.locator('.ttm-node[data-state="excluded"]').count()) === 1,
  );
  await page.screenshot({ path: `${outDir}/state-constraints.png` });
}

// --- tooltip ---------------------------------------------------------------
await page.locator('button:has-text("Clear")').click();
await settled();
await nodes.nth(Math.floor(total / 2)).hover({ force: true });
await page.waitForSelector(".ttm-tooltip", { timeout: 5000 }).catch(() => {});
const tip = page.locator(".ttm-tooltip");
if (await tip.count()) {
  const box = await tip.boundingBox();
  const view = page.viewportSize();
  check(
    "the tooltip stays on screen",
    box.x >= 0 && box.y >= 0 && box.x + box.width <= view.width && box.y + box.height <= view.height,
    JSON.stringify(box),
  );
  check("the tooltip has real text", (await tip.innerText()).length > 20);
  await page.screenshot({ path: `${outDir}/state-tooltip.png` });
} else {
  check("the tooltip appears", false, "no .ttm-tooltip");
}

// --- the gate refuses an oversized listing ---------------------------------
await page.mouse.move(0, 0);
const slider = page.locator("#points");
await slider.fill("30");
await settled();
const solve = page.locator('button:has-text("Enumerate matching builds")');
check("the gate disables enumeration when it is too large", await solve.isDisabled());
await page.screenshot({ path: `${outDir}/state-too-large.png` });

// --- a real enumeration ----------------------------------------------------
await slider.fill("18");
await settled();
check("a small enough budget re-enables it", await solve.isEnabled());
await solve.click();

await page.waitForSelector(".ttm-tooltip", { state: "detached" }).catch(() => {});

/*
  Two legitimate outcomes, and the test has to allow both.

  An identical request is deduplicated server-side, so re-running this suite hands back the
  finished job from last time -- instantly, with no phases to observe. That is the cache
  working, not a failure, and it is worth asserting rather than engineering around.

  Comparisons are case-insensitive because the state label is uppercased in CSS, and
  innerText reports the rendered text. A case-sensitive check here passed for the wrong
  reason once already.
*/
const panel = () => page.locator("section", { hasText: "Enumeration" }).first();
const phases = new Set();
let firstState = (await panel().innerText()).toLowerCase();
let shot = false;

for (let i = 0; i < 300; i++) {
  if (!(await panel().count())) break;
  const text = (await panel().innerText()).toLowerCase();
  for (const phase of ["searching the tree", "storing builds", "finishing up"]) {
    if (text.includes(phase)) phases.add(phase);
  }
  if (!shot && /storing builds|searching the tree/.test(text)) {
    await page.screenshot({ path: `${outDir}/state-running.png` });
    shot = true;
  }
  if (/complete|failed|cancelled|partial/.test(text)) break;
  await page.waitForTimeout(100);
}

const final = (await panel().innerText()).toLowerCase();
check("the job completes", final.includes("complete"), final.split("\n").join(" | "));

const wasCached = /complete/.test(firstState);
if (wasCached) {
  check("an identical request is served from the previous job", true);
  console.log("     (deduplicated: no phases to observe this run)");
} else {
  check("progress reported at least one phase", phases.size >= 1, [...phases].join(", "));
}

await page.screenshot({ path: `${outDir}/state-done.png` });

await browser.close();

console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all interaction checks passed");
