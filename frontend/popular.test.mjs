/**
 * Top players, in the browser: see it, take one, narrow to the question.
 *
 *   node popular.test.mjs [url]
 *
 * Needs WarcraftLogs keys on the API; without them it says it skipped, rather than failing,
 * since that is a configuration and not a fault.
 */
import { chromium } from "playwright";
import { execFileSync } from "node:child_process";
import { mkdtemp, readFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

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
await waitForServer(`${url}/data/retail/index.json`);
if ((await fetch(`${url}/api/popular/content`)).status === 503) {
  console.log("WarcraftLogs is not configured on this API: skipped");
  process.exit(0);
}

const total = async (page) => Number((await page.locator("[data-total]").getAttribute("data-total")) || NaN);
const waitTotal = async (page, predicate, timeout = 20000) => {
  const deadline = Date.now() + timeout;
  for (;;) {
    const v = await total(page);
    if (predicate(v) || Date.now() > deadline) return v;
    await page.waitForTimeout(150);
  }
};

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("pageerror", (e) => check("no page error", false, String(e)));

await page.goto(`${url}/?t=retail/6/250/spec`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node");
const panel = page.locator('section[aria-label="Top players"]');
await panel.locator('button:text-is("Load")').click();
// The number, not the words: the panel's own heading already says "top players".
await panel.getByText(/\d+ top players/).waitFor({ timeout: 120000 });
const summary = await panel.innerText();
const players = Number(/([\d,]+) top players/.exec(summary)?.[1].replace(/,/g, "") ?? 0);
check("top players are read", players >= 100, String(players));
check("with the hero trees they chose", /San.layn/.test(summary) && /Deathbringer/.test(summary));
check("and no real build breaks our tree data", !/break this tree data/.test(summary));

// See it: pick rates painted on the trees.
await panel.locator('label:has-text("show pick rates") input').check();
await page.waitForTimeout(400);
const painted = await page.locator('.ttm-node[data-share]').count();
check("pick rates paint the trees", painted > 50, `${painted} talents`);
await panel.locator('label:has-text("show pick rates") input').uncheck();

// Take one: the most common build becomes the loadout.
await panel.locator('button:text-is("use")').first().click();
await page.waitForTimeout(600);
check("using a top build fixes every tree to it", (await waitTotal(page, (v) => v === 1)) === 1);
const fixed = await page.locator('.seg button[aria-pressed="true"]:text-is("Fixed")').count();
check("all three trees fixed", fixed === 3, String(fixed));

// Narrow to the question: what remains is what the top players contest.
await panel.locator('button:has-text("Narrow to the contested talents")').click();
const narrowed = await waitTotal(page, (v) => Number.isFinite(v) && v > 1 && v < 1e12);
if (process.env.DEBUG_NARROW) {
  console.log("   url:", decodeURIComponent(page.url()).slice(0, 400));
  console.log("   counts:", await page.$$eval("[data-count-for]", (els) => els.map((e) => `${e.dataset.countFor}=${e.textContent}`)));
}
check("narrowing leaves a real, open question", narrowed > 1, String(narrowed));
// Not "simmable": how far consensus narrows depends on the spec -- Blood's class tree keeps
// 2,264 builds of utility picks top players split on. What must hold is that the consensus
// collapses the space by many orders of magnitude from the untouched trees.
check("the consensus collapses the space by orders of magnitude", narrowed < 9e16 / 1e6, String(narrowed));
check("and it says what it settled", /talents settled at 90% agreement/.test(await page.locator("aside").innerText()));
const open = await page.locator('.seg button[aria-pressed="true"]:text-is("Open")').count();
check("every tree is open, painted with the consensus", open === 3, String(open));
console.log(`   ${players} top players narrow Blood to ${narrowed.toLocaleString("en-US")} builds`);
await page.screenshot({ path: "shots/popular.png" });

// The most direct question: of the builds that actually win, which is best on this character?
const simButton = panel.getByRole("button", { name: /Sim .* top builds/ });
const offered = Number(/Sim ([\d,]+)/.exec(await simButton.innerText())?.[1].replace(/,/g, "") ?? 0);
await simButton.click();
await page.waitForFunction(
  () => [...document.querySelectorAll("h2")].some((h) => /Simulate [\d,]+ builds/.test(h.textContent ?? "")),
  null,
  { timeout: 60000 },
);
check("the top builds go straight to Simulate, no enumeration", /distinct builds the top/.test(await page.locator("body").innerText()));
const [download] = await Promise.all([page.waitForEvent("download"), page.locator('button:has-text("Download")').click()]);
const dir = await mkdtemp(join(tmpdir(), "ttm-top-"));
await download.saveAs(join(dir, "top.simc"));
const lines = (await readFile(join(dir, "top.simc"), "utf8")).split("\n").filter((l) => l.startsWith("profileset."));
check("one profileset per distinct top build", lines.length === offered, `${lines.length} of ${offered}`);

let simc = true;
try {
  execFileSync("docker", ["image", "inspect", "simulationcraftorg/simc:latest"], { stdio: "ignore" });
} catch {
  simc = false;
}
if (simc) {
  console.log(`   running SimulationCraft on ${lines.length} top builds…`);
  execFileSync("docker", [
    "run", "--rm", "-v", `${dir}:/data`, "simulationcraftorg/simc:latest",
    "profiles/MID1/MID1_Death_Knight_Blood.simc", "/data/top.simc",
    "iterations=30", "threads=8", "json2=/data/report.json",
  ], { stdio: "ignore", env: { ...process.env, MSYS_NO_PATHCONV: "1" } });
  const report = JSON.parse(await readFile(join(dir, "report.json"), "utf8"));
  check("SimulationCraft accepted every real top build", report.sim.profilesets.results.length === lines.length,
    `${report.sim.profilesets.results.length} of ${lines.length}`);
  await page.locator('input[aria-label="SimulationCraft report"]').setInputFiles(join(dir, "report.json"));
  await page.waitForSelector("text=Simmed", { timeout: 20000 });
  const body = await page.locator("body").innerText();
  check("the analysis compares the hero trees top players split between", /hero tree/i.test(body));
  await page.screenshot({ path: "shots/popular-analysis.png" });
} else {
  console.log("   SimulationCraft image not present: sim step skipped, not faked");
}

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall top-player checks passed");
process.exit(failures.length ? 1 : 0);
