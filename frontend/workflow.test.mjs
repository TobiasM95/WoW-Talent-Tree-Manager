/**
 * The whole workflow, the way a player does it, against the real stack and a real sim.
 *
 *   node workflow.test.mjs [url]
 *
 *   1. Narrow: paste the talent string you play, which fixes all three trees. Open the one
 *      tree you want to explore and narrow it until the product fits the sim limit.
 *   2. Simulate: download the file, run SimulationCraft on it, drop the report back.
 *   3. Analyse: the ranking, what each talent was worth, and each choice node's sides.
 *
 * SimulationCraft runs in Docker when the image is present -- over SimC's own sample Blood
 * Death Knight, whose talent string is also the one imported here, so the fixed trees are a
 * real character's. Without Docker the sim step is reported as skipped rather than faked: a
 * fabricated report would only prove the reader agrees with itself.
 */
import { chromium } from "playwright";
import { execFileSync } from "node:child_process";
import { mkdtemp, readFile, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

/*
  Defaults to the built container rather than the dev server: 8081 is what the compose file
  serves and what every verification in this repo runs against.
*/
const url = process.argv[2] ?? "http://localhost:8081";
const shots = "shots";

// SimulationCraft's MID1 sample Blood Death Knight (San'layn), from its own profile.
const BLOOD =
  "CoPAAAAAAAAAAAAAAAAAAAAAAwYWmZmxMmZmhZZmZmmZxMjxMAAAAAzMzMzwMDzYMDAjZmZGAAADMwM20YZDklBsBYGzAAAmZwgB";

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

const hasSimc = () => {
  try {
    execFileSync("docker", ["image", "inspect", "simulationcraftorg/simc:latest"], { stdio: "ignore" });
    return true;
  } catch {
    return false;
  }
};

const total = (page) => page.locator("[data-total]").getAttribute("data-total");
const waitTotal = async (page, predicate, timeout = 15000) => {
  const deadline = Date.now() + timeout;
  for (;;) {
    const value = Number((await total(page)) || NaN);
    if (predicate(value)) return value;
    if (Date.now() > deadline) return value;
    await page.waitForTimeout(150);
  }
};

// The web container answers at once; the API behind it can take several seconds more
// after a rebuild. Waiting on the page alone failed the first run after every deploy.
await waitForServer(url);
await waitForServer(`${url}/api/health`);
const browser = await chromium.launch();
const context = await browser.newContext({ viewport: { width: 1600, height: 1000 }, acceptDownloads: true });
const page = await context.newPage();
page.on("pageerror", (e) => check("no page error", false, String(e)));

/* --- 1. narrow ------------------------------------------------------------ */

await page.goto(`${url}/?t=retail/6/250/spec`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node", { timeout: 20000 });
await page.waitForTimeout(1200);

const opening = await waitTotal(page, (v) => Number.isFinite(v));
check("every tree starts open, and the space is counted", opening > 1e6, String(opening));
await page.screenshot({ path: `${shots}/flow-1-open.png` });

// Paste the build you play.
await page.locator("textarea").first().fill(BLOOD);
await page.locator('button:has-text("Import")').click();
await page.waitForTimeout(900);
const fixedCount = await page.locator('.seg button[aria-pressed="true"]:text-is("Fixed")').count();
check("importing a talent string fixes all three trees", fixedCount === 3, `${fixedCount} fixed`);
check("and the space is exactly one build", (await waitTotal(page, (v) => v === 1)) === 1);

// Open the hero tree and give it a budget that leaves real choices.
const heroPane = page.locator('section[aria-label="Hero"]');
await heroPane.locator('.seg button:text-is("Open")').click();
await page.waitForTimeout(300);
for (let i = 0; i < 5; i++) {
  await heroPane.locator('button[aria-label^="Spend one point fewer"]').click();
  await page.waitForTimeout(60);
}
// Waited for rather than sampled: each stepper click fires a count, and the first value in
// range is an intermediate budget's, not the one the stepping ends on.
await page.waitForSelector('section[aria-label="Hero"] >> text=8/13');
const narrowed = await waitTotal(page, (v) => v === 580);
check(
  "opening one tree multiplies only that tree, choice sides included",
  narrowed === 580,
  `${narrowed} (the API counts 580 builds for this hero tree at 8 points)`,
);

// The fixed trees keep their points while the hero tree is open.
const classSpent = await page.locator('section[aria-label="Class"] .ttm-node[data-spent="yes"]').count();
check("the fixed trees still show their builds", classSpent > 10, `${classSpent} class talents lit`);

// Switch hero back to fixed and open again: nothing is lost either way.
await heroPane.locator('.seg button:text-is("Fixed")').click();
await page.waitForTimeout(300);
const heroFixedLit = await heroPane.locator('.ttm-node[data-spent="yes"]').count();
await heroPane.locator('.seg button:text-is("Open")').click();
await page.waitForTimeout(600);
check("flipping a tree back to fixed returns its build", heroFixedLit > 5, `${heroFixedLit} lit`);
check(
  "and back to open returns its search",
  (await heroPane.locator("text=8/13").count()) > 0 && (await waitTotal(page, (v) => v === 580)) === 580,
);

await page.screenshot({ path: `${shots}/flow-2-narrowed.png` });

// Painting a constraint on the open tree moves the count.
await heroPane.locator(".ttm-node").nth(3).click();
const painted = await waitTotal(page, (v) => v !== 580);
check("painting a constraint on the open tree moves the count", painted !== 580 && painted > 0, String(painted));
await heroPane.locator(".ttm-node").nth(3).click({ button: "right" });
check("and right-click takes it back", (await waitTotal(page, (v) => v === 580)) === 580);

// The link carries all of it.
const link = page.url();
const fresh = await context.newPage();
await fresh.goto(link, { waitUntil: "networkidle" });
await fresh.waitForSelector(".ttm-node", { timeout: 20000 });
check("a shared link reopens the same space", (await waitTotal(fresh, (v) => v === 580)) === 580);
await fresh.close();

/* --- 2. simulate ---------------------------------------------------------- */

await page.locator('button:has-text("Simulate 580 builds")').click();
await page.waitForSelector('button:has-text("Download")', { timeout: 60000 });
await page.waitForFunction(() => !document.querySelector('button:has-text("Download")')?.disabled, null, {
  timeout: 120000,
}).catch(() => {});
await page.waitForFunction(
  () => [...document.querySelectorAll("h2")].some((h) => /Simulate 580 builds/.test(h.textContent ?? "")),
  null,
  { timeout: 120000 },
);
check("arriving at Simulate lists the builds without another button", true);
// The builds can be flicked through on the trees while the sim runs.
const heroLitAt = () => page.locator('section[aria-label="Hero"] .ttm-node[data-spent="yes"]').evaluateAll(
  (els) => els.map((e) => e.getAttribute("aria-label")).sort().join("|"),
);
const firstBuild = await heroLitAt();
check("the first build is drawn on the trees", firstBuild.length > 0);
check(
  "and the fixed trees are drawn too",
  (await page.locator('section[aria-label="Class"] .ttm-node[data-spent="yes"]').count()) > 10,
);
await page.locator('button[aria-label="Next build"]').click();
await page.waitForTimeout(200);
check("next steps to build 2", (await page.locator("[data-browse-index]").getAttribute("data-browse-index")) === "2");
check("and says what changed", /vs the build before|Same talents/.test(await page.locator('section[aria-label="Browse the builds"]').innerText()));
await page.locator('section[aria-label="Browse the builds"]').focus();
for (let i = 0; i < 3; i++) await page.keyboard.press("Shift+ArrowRight");
check("shift+arrow jumps ten", (await page.locator("[data-browse-index]").getAttribute("data-browse-index")) === "32");
check("and the trees follow", (await heroLitAt()) !== firstBuild);
await page.screenshot({ path: `${shots}/flow-3-simulate.png` });

const [download] = await Promise.all([
  page.waitForEvent("download"),
  page.locator('button:has-text("Download")').click(),
]);
const dir = await mkdtemp(join(tmpdir(), "ttm-flow-"));
const simcFile = join(dir, "builds.simc");
await download.saveAs(simcFile);
const text = await readFile(simcFile, "utf8");
const lines = text.split("\n").filter((l) => l.startsWith("profileset."));
check("the file has one profileset per build", lines.length === 580, `${lines.length} lines`);

if (!hasSimc()) {
  console.log("   SimulationCraft image not present: sim and analysis steps skipped, not faked");
} else {
  console.log(`   running SimulationCraft on ${lines.length} profilesets…`);
  execFileSync(
    "docker",
    [
      "run", "--rm", "-v", `${dir}:/data`, "simulationcraftorg/simc:latest",
      "profiles/MID1/MID1_Death_Knight_Blood.simc", "/data/builds.simc",
      "iterations=40", "threads=8", "json2=/data/report.json",
    ],
    { stdio: "ignore", env: { ...process.env, MSYS_NO_PATHCONV: "1" } },
  );
  const report = JSON.parse(await readFile(join(dir, "report.json"), "utf8"));
  check(
    "SimulationCraft accepted every build",
    report.sim.profilesets.results.length === 580,
    `${report.sim.profilesets.results.length} results`,
  );

  await page.locator('input[aria-label="SimulationCraft report"]').setInputFiles(join(dir, "report.json"));

  /* --- 3. analyse --------------------------------------------------------- */

  await page.waitForSelector("text=Simmed", { timeout: 20000 });
  check("a report goes straight to the analysis", true);
  await page.waitForTimeout(800);
  await page.screenshot({ path: `${shots}/flow-4-builds.png` });

  const rowsShown = await page.locator(".ttm-table tbody tr").count();
  check("the ranking is a table", rowsShown >= 100, `${rowsShown} rows`);
  const heroLit = await page.locator('section[aria-label="Hero"] .ttm-node[data-spent="yes"]').count();
  check("the selected build is drawn on the trees", heroLit > 5, `${heroLit} hero talents lit`);

  await page.locator('button[role="tab"]:has-text("Talent value")').click();
  await page.waitForTimeout(600);
  const valued = await page.locator('section[aria-label="Hero"] .ttm-node[data-impact]').count();
  check("talent value is painted on the tree", valued > 0, `${valued} talents`);
  await page.screenshot({ path: `${shots}/flow-5-talents.png` });

  await page.locator('button[role="tab"]:has-text("Choice nodes")').click();
  await page.waitForTimeout(400);
  const duelsShown = await page.locator("aside li:has-text('builds')").count();
  check("each free choice node's two sides are compared", duelsShown > 0, `${duelsShown} duels`);
  await page.screenshot({ path: `${shots}/flow-6-choices.png` });

  // A report from some other export is refused in words, not half-matched.
  await page.locator('button:has-text("Load another report")').click();
  const wrong = join(dir, "wrong.json");
  await writeFile(
    wrong,
    JSON.stringify({
      sim: { profilesets: { metric: "Damage per Second", results: [{ name: "somebody_else", mean: 1 }] }, players: [] },
    }),
  );
  await page.locator('input[aria-label="SimulationCraft report"]').setInputFiles(wrong);
  await page.waitForTimeout(600);
  check(
    "a report from another export is refused in words",
    /came from this export/i.test(await page.locator("body").innerText()),
  );
  await page.locator('nav[aria-label="Workflow"] button:has-text("Analyse")').click();
  await page.waitForTimeout(400);

  // Take a build back to narrowing.
  await page.locator('button[role="tab"]:has-text("Builds")').click();
  await page.locator(".ttm-table tbody tr").nth(4).click();
  await page.locator('button:has-text("Use this build")').click();
  await page.waitForTimeout(800);
  check("using a build fixes all three trees to it", (await waitTotal(page, (v) => v === 1)) === 1);
}

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nthe workflow holds");
process.exit(failures.length ? 1 : 0);
