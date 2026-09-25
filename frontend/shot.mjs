/**
 * Screenshot + console check against the running dev server.
 *
 *   node shot.mjs [url] [outDir]
 *
 * Exists because a design decision cannot be verified by a passing build. It captures both
 * themes at desktop and phone width and fails loudly on any console error, which is the
 * class of bug a screenshot alone hides (a blank-looking panel and a thrown exception look
 * identical in a still image).
 */
import { chromium } from "playwright";
import { mkdir } from "node:fs/promises";

/*
  Defaults to the built container rather than the dev server.

  `npm run test:all` used to fail here whenever nothing happened to be running on 5173,
  which says nothing about the app. 8081 is what the compose file serves and what every
  verification in this repo runs against; pass a URL to point somewhere else.
*/
const url = process.argv[2] ?? "http://localhost:8081";
const outDir = process.argv[3] ?? "shots";
await mkdir(outDir, { recursive: true });

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

// The web container answers at once; the API behind it can take several seconds more
// after a rebuild. Waiting on the page alone failed the first run after every deploy.
await waitForServer(url);
await waitForServer(`${url}/api/health`);

const browser = await chromium.launch();
const problems = [];

const views = [
  { name: "desktop", width: 1440, height: 900 },
  { name: "phone", width: 390, height: 844 },
];

for (const theme of ["dark", "light"]) {
  for (const view of views) {
    const context = await browser.newContext({
      viewport: { width: view.width, height: view.height },
      colorScheme: theme,
      deviceScaleFactor: 1,
    });
    const page = await context.newPage();
    page.on("console", (message) => {
      if (message.type() === "error") problems.push(`[${theme}/${view.name}] ${message.text()}`);
    });
    page.on("pageerror", (error) => problems.push(`[${theme}/${view.name}] ${error.message}`));

    await page.goto(url, { waitUntil: "networkidle" });
    // Wait for a tree to actually be on the canvas; screenshotting the loading state would
    // tell us nothing about the design.
    await page
      .waitForSelector(".ttm-node", { timeout: 15000 })
      .catch(() => problems.push(`[${theme}/${view.name}] no talent nodes rendered`));
    await page.waitForTimeout(700);

    const file = `${outDir}/${theme}-${view.name}.png`;
    await page.screenshot({ path: file });
    console.log(`wrote ${file}`);
    await context.close();
  }
}

await browser.close();

if (problems.length) {
  console.error("\nconsole/page problems:");
  for (const problem of problems) console.error("  " + problem);
  process.exit(1);
}
console.log("\nno console errors");
