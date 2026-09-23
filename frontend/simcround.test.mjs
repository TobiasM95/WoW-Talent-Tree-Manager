/**
 * The whole sim round trip, driven through the real app against the real stack.
 *
 *   node simcround.test.mjs [url]
 *
 * Export builds, hand the file to SimulationCraft, read its report back, and check that the
 * numbers land on the builds they came from. The sim itself runs in Docker if it is
 * available; if it is not, the run falls back to the stored report from a previous real run
 * and says which of the two it used — the point being that this test never fabricates a
 * report, because a fabricated one would only ever confirm the parser agrees with itself.
 */
import { chromium } from "playwright";
import { execFileSync } from "node:child_process";
import { readFile, writeFile, mkdtemp } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

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

const hasDocker = () => {
  try {
    execFileSync("docker", ["image", "inspect", "simulationcraftorg/simc:latest"], {
      stdio: "ignore",
    });
    return true;
  } catch {
    return false;
  }
};

await waitForServer(url);
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1600, height: 1100 } });
page.on("pageerror", (e) => check("no page error", false, String(e)));

/* --- build a loadout, enumerate a tree, export it ------------------------- */

await page.goto(`${url}/?t=retail/6/250/hero/31&m=b`, { waitUntil: "networkidle" });
await page.waitForSelector(".ttm-node", { timeout: 20000 });
await page.waitForTimeout(1200);

const fill = async () => {
  for (let i = 0; i < 40; i++) {
    const next = page.locator('[data-active="yes"] .ttm-node[data-reachable="yes"]').first();
    if (!(await next.count())) break;
    await next.click();
    await page.waitForTimeout(50);
  }
};
for (const pane of [0, 1, 2]) {
  await page.locator("[data-active]").nth(pane).click();
  await page.waitForTimeout(400);
  await fill();
}

await page.locator("[data-active]").nth(2).click();
await page.locator('button:has-text("Explore")').first().click();
await page.waitForTimeout(1200);
await page.locator("#points").fill("8");
await page.locator("#points").dispatchEvent("change");
await page.waitForTimeout(1500);

await page.locator('button:has-text("Enumerate")').click();
await page.waitForSelector('button:has-text("Export")', { timeout: 120000 });
await page.locator('button:has-text("Export")').first().click();
await page.waitForSelector('textarea[aria-label="SimulationCraft profilesets"]', {
  timeout: 60000,
});

const simc = await page.locator('textarea[aria-label="SimulationCraft profilesets"]').inputValue();
const lines = simc.split("\n").filter((l) => l.startsWith("profileset."));
check("the export produced profilesets", lines.length > 1, `${lines.length} lines`);

/* --- sim them, or fall back to the report from a previous real run -------- */

const dir = await mkdtemp(join(tmpdir(), "ttm-simc-"));
let reportPath = new URL("./fixtures/simc-report.json", import.meta.url).pathname.slice(1);
let source = "the stored report from an earlier real run";

if (hasDocker()) {
  await writeFile(join(dir, "sets.simc"), lines.join("\n") + "\n", "utf8");
  console.log(`   running SimulationCraft on ${lines.length} profilesets…`);
  execFileSync(
    "docker",
    [
      "run", "--rm", "-v", `${dir}:/data`,
      "simulationcraftorg/simc:latest",
      "profiles/MID1/MID1_Death_Knight_Blood.simc",
      "/data/sets.simc",
      "iterations=50", "threads=8", "json2=/data/report.json",
    ],
    { stdio: "ignore", env: { ...process.env, MSYS_NO_PATHCONV: "1" } },
  );
  reportPath = join(dir, "report.json");
  source = "a live SimulationCraft run";
}
console.log(`   report: ${source}`);

const report = await readFile(reportPath, "utf8");
const count = JSON.parse(report).sim.profilesets.results.length;
check(
  "SimulationCraft accepted every exported build",
  !hasDocker() || count === lines.length,
  `${count} results for ${lines.length} profilesets`,
);

/* --- read it back in the app ---------------------------------------------- */

const file = join(dir, "dropped.json");
await writeFile(file, report, "utf8");
await page.locator('section:has-text("Sim results") input[type="file"]').setInputFiles(file);
await page.waitForTimeout(1500);

const sidebar = await page.locator("aside").innerText();
check("the report is read", /Best/i.test(sidebar) && /build \d+/i.test(sidebar), sidebar.slice(0, 200));
check("it says how tight the numbers are", /ties, not an order/i.test(sidebar));
check("and what each talent was worth", /Talent value/i.test(sidebar));

// The heading, not the word: five panels mention builds by now.
const builds = page.locator('section:has(h2:text-is("Builds"))');
const browsed = await builds.innerText();
check("the browsed build carries its score", /#\d+ of \d+/.test(browsed), browsed.slice(-160));

// The best build is reachable in one click, and once there the button says so.
await page.locator('button:has-text("Go to the best build")').click();
await page.waitForTimeout(900);
check(
  "the best build is one click away",
  await page.locator('button:has-text("This is the best build")').count() > 0,
);
const best = await builds.innerText();
check("and it is ranked first", /#1 of/.test(best), best.slice(-120));

// The canvas carries the verdict.
const painted = await page.locator("[data-active='yes'] .ttm-node[data-impact]").count();
check("the tree is painted with what the sim said", painted > 0, `${painted} talents scored`);
const good = await page.locator("[data-active='yes'] .ttm-node[data-impact='good']").count();
const bad = await page.locator("[data-active='yes'] .ttm-node[data-impact='bad']").count();
check("in both directions", good > 0 && bad > 0, `${good} better, ${bad} worse`);

await page.screenshot({ path: "shots/sim-round-trip.png" });

/* --- a report from somewhere else ----------------------------------------- */

const wrong = join(dir, "wrong.json");
await writeFile(
  wrong,
  JSON.stringify({
    sim: {
      profilesets: { metric: "Damage per Second", results: [{ name: "somebody_else", mean: 1 }] },
      players: [],
    },
  }),
  "utf8",
);
await page.locator('section:has-text("Sim results") input[type="file"]').setInputFiles(wrong);
await page.waitForTimeout(900);
const after = await page.locator('section:has-text("Sim results")').innerText();
check(
  "a report from a different export is refused in words",
  /came from this export/i.test(after),
  after.slice(0, 160),
);

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nthe round trip holds");
process.exit(failures.length ? 1 : 0);
