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

/**
 * Wait for the target to answer before opening a browser at it.
 *
 * A freshly created container takes a second or two to bind, and both of these suites have
 * already reported a false failure against one that was still starting. A test that fails
 * because of its own timing teaches the wrong lesson twice: once when it fails, and again
 * when someone learns to re-run it rather than read it.
 */
async function waitForServer(target, timeoutMs = 30000) {
  const deadline = Date.now() + timeoutMs;
  for (;;) {
    try {
      const response = await fetch(target, { redirect: "manual" });
      if (response.status < 500) return;
    } catch {
      /* not listening yet */
    }
    if (Date.now() > deadline) throw new Error(`${target} did not respond within ${timeoutMs}ms`);
    await new Promise((resolve) => setTimeout(resolve, 250));
  }
}

await waitForServer(url);

const browser = await chromium.launch();
const context = await browser.newContext({
  viewport: { width: 1440, height: 900 },
  colorScheme: "dark",
});
const page = await context.newPage();
page.on("pageerror", (error) => failures.push(`page error: ${error.message}`));

await page.goto(url, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");

const countText = () => page.locator(".num-display").first().innerText();

/*
  Every assertion below is about the tree the solver is pointed at, and there are now three
  trees on screen. Scoping to the active pane is not tidiness: clicking a talent in an
  inactive pane points the solver at that pane instead of painting a constraint, which is
  correct behaviour and would make a naive "click any node" test do something else entirely.
*/
const activePane = () => page.locator('.ttm-tree[data-active="yes"]');
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
const nodes = activePane().locator(".ttm-node");
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
  (await activePane().locator('.ttm-node[data-state="required"]').count()) === 1,
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
    (await activePane().locator('.ttm-node[data-state="excluded"]').count()) === 1,
  );
  await page.screenshot({ path: `${outDir}/state-constraints.png` });
}

// --- three trees, one solver -----------------------------------------------
/*
  All three trees are on screen; the solver is pointed at one. Two things have to hold, and
  both are easy to get wrong:

  - clicking in an inactive pane *re-points* the solver rather than painting a constraint,
    because painting into a tree the count does not cover would be a silent lie;
  - constraints are put away per tree rather than thrown away, so going to the class tree
    and back does not lose what was painted on the spec.
*/
const paneTitles = () =>
  page.locator(".ttm-tree header").evaluateAll((els) =>
    els.map((el) => el.textContent.trim().split(/\s{2,}|\n/)[0]),
  );
check("all three trees are on screen", (await page.locator(".ttm-tree").count()) === 3,
      JSON.stringify(await paneTitles()));
check("exactly one is active",
      (await page.locator('.ttm-tree[data-active="yes"]').count()) === 1);

const before = await activePane().getAttribute("aria-label");
const painted = await activePane().locator('.ttm-node[data-state="excluded"]').count();

const other = page.locator('.ttm-tree[data-active="no"]').first();
await other.locator(".ttm-node").first().click({ force: true });
await page.waitForTimeout(500);
const after = await activePane().getAttribute("aria-label");
check("clicking an inactive tree points the solver at it", after !== before,
      `${before} -> ${after}`);
check("it did not paint a constraint there",
      (await activePane().locator('.ttm-node[data-state]:not([data-state="neutral"])').count()) === 0);

// Back again: what was painted on the first tree must still be there.
await page.locator(".ttm-tree", { hasText: before.split(" ")[0] }).first()
  .locator("header button").first().click();
await settled();
check("constraints survive pointing the solver elsewhere and back",
      (await activePane().locator('.ttm-node[data-state="excluded"]').count()) === painted,
      `${painted} before`);

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

// --- inspecting the results ------------------------------------------------
/*
  A list of builds is not the useful artefact -- a build is twenty talents, and twenty rows
  of numbers tell nobody anything. The tree is. So what is checked here is that selecting a
  build actually paints it onto the canvas, and that stepping to the next one paints a
  different set.
*/
const browser_ = page.locator("section", { hasText: "Builds" }).first();
check("a results browser appears for a finished job", (await browser_.count()) === 1);

if (await browser_.count()) {
  await page.waitForFunction(
    () => !!document.querySelector('.ttm-tree[data-active="yes"] .ttm-node[data-spent]'),
    { timeout: 10000 },
  );
  const taken = () => activePane().locator('.ttm-node[data-spent="yes"]').count();
  const untaken = () => activePane().locator('.ttm-node[data-spent="no"]').count();

  const firstTaken = await taken();
  check("the selected build is drawn on the tree", firstTaken > 0, `${firstTaken} lit`);
  check("talents outside the build recede", (await untaken()) > 0);

  // Every build spends the whole budget, so the count of lit nodes is not the signal --
  // *which* nodes are lit is. Compare the identity of the set, not its size.
  const litIds = async () =>
    (await activePane().locator('.ttm-node[data-spent="yes"]').evaluateAll((els) =>
      els.map((el) => el.getAttribute("aria-label")).sort().join("|"),
    ));
  const before = await litIds();
  await page.locator('button[aria-label="Next build"]').click();
  await page.waitForTimeout(400);
  check("stepping to the next build changes which talents are lit",
        (await litIds()) !== before);

  // The panel stack grows twice over a job's life, so the column has to stay reachable.
  const aside = page.locator("aside").first();
  const fits = await aside.evaluate((el) => {
    const box = el.getBoundingClientRect();
    return box.bottom <= window.innerHeight + 1;
  });
  check("the sidebar stays within the viewport once results arrive", fits);

  await page.screenshot({ path: `${outDir}/state-build.png` });
}

// --- talent statistics -----------------------------------------------------
/*
  The analytical payoff of enumerating rather than sampling, so it gets checked as such:
  the tree must actually be painted with frequencies, and the talents every build takes must
  be distinguished from the ones that are a real choice.
*/
const statsPanel = page.locator("section", { hasText: "What they share" }).first();
check("statistics appear for a finished job", (await statsPanel.count()) === 1);

if (await statsPanel.count()) {
  // A build may already be selected from the step above, and the two readings cannot share
  // a canvas. The checkbox is the switch between them, so use it rather than reaching for
  // Clear -- which would also discard the constraints.
  await statsPanel.locator('input[type="checkbox"]').check();
  await page.waitForFunction(
    () => !!document.querySelector('.ttm-tree[data-active="yes"] .ttm-node[data-share]'),
    { timeout: 10000 },
  ).catch(() => {});

  const shared = await activePane().locator(".ttm-node[data-share]").count();
  check("every talent carries a frequency", shared > 0, `${shared} nodes`);

  const settled = await activePane().locator('.ttm-node[data-share="all"]').count();
  check("talents in every build are marked settled", settled > 0, `${settled}`);

  const text = await statsPanel.innerText();
  check("the panel names where the choice is", /where the choice actually is/i.test(text),
        text.split("\n").join(" | ").slice(0, 120));

  await page.screenshot({ path: `${outDir}/state-stats.png` });

  // Inspecting one build and reading the whole set are different questions. Showing both at
  // once would make neither legible, so picking a build has to take the canvas back.
  await page.locator('button[aria-label="Next build"]').click();
  await page.waitForTimeout(400);
  check("picking a build takes the canvas back from the heat map",
        (await activePane().locator(".ttm-node[data-share]").count()) === 0);
}

// --- sharing ---------------------------------------------------------------
/*
  The point of rebuilding this as a web app was that the tool should be a URL. That only
  means something if opening the URL reproduces the screen, so this loads the link the app
  produced into a *fresh page* and checks the state came back -- the tree, the build being
  inspected, and which talents it lights.
*/
const shareUrl = page.url();
check("the address bar carries state", shareUrl.includes("?"), shareUrl);

if (shareUrl.includes("?")) {
  const litHere = await activePane().locator('.ttm-node[data-spent="yes"]').evaluateAll((els) =>
    els.map((el) => el.getAttribute("aria-label")).sort().join("|"),
  );

  const fresh = await context.newPage();
  await fresh.goto(shareUrl, { waitUntil: "networkidle" });
  await fresh.waitForSelector(".ttm-node");
  await fresh.waitForFunction(
    () => !!document.querySelector('.ttm-tree[data-active="yes"] .ttm-node[data-spent="yes"]'),
    { timeout: 10000 },
  ).catch(() => {});

  const litThere = await fresh
    .locator('.ttm-tree[data-active="yes"] .ttm-node[data-spent="yes"]')
    .evaluateAll((els) =>
    els.map((el) => el.getAttribute("aria-label")).sort().join("|"),
  );
  check("a shared link reopens on the same build", litThere === litHere && litThere.length > 0,
        `${litThere.split("|").length} vs ${litHere.split("|").length} talents`);

  const budgetHere = await page.locator("#points").inputValue();
  const budgetThere = await fresh.locator("#points").inputValue();
  check("a shared link restores the point budget", budgetThere === budgetHere,
        `${budgetThere} vs ${budgetHere}`);

  // The picker is a rail of names rather than a <select>, so "the same tree" is read off
  // the heading of whichever pane the solver is pointed at.
  const paneTitle = (target) =>
    target.locator('.ttm-tree[data-active="yes"] header').first().innerText();
  const treeHere = await paneTitle(page);
  const treeThere = await paneTitle(fresh);
  check("a shared link restores the tree", treeThere === treeHere,
        `${treeThere} vs ${treeHere}`);

  await fresh.screenshot({ path: `${outDir}/state-shared.png` });

  // A link that has been mangled in transit must still open something usable.
  const mangled = await context.newPage();
  await mangled.goto(shareUrl.replace(/([?&]r=)[^&]*/, "$1zz-!!").replace(/([&?]p=)\d+/, "$1abc"));
  await mangled.waitForSelector(".ttm-node", { timeout: 15000 });
  const errors = [];
  mangled.on("pageerror", (error) => errors.push(error.message));
  await mangled.waitForTimeout(800);
  check("a mangled link still opens a usable page", errors.length === 0, errors.join("; "));
  await mangled.close();
  await fresh.close();
}

await browser.close();

console.log();
if (failures.length) {
  console.error(`${failures.length} failed: ${failures.join(", ")}`);
  process.exit(1);
}
console.log("all interaction checks passed");
