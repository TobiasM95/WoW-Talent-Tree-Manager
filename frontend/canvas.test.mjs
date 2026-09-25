/**
 * What the canvas draws: node shapes, constraint rings, and the legend, in both themes.
 *
 *   node canvas.test.mjs [url]
 *
 * Checked on computed styles in a real browser, because every one of these was once wrong in
 * a way that only showed on screen: rings clipped away on every hexagon, a green rim that
 * vanished against a green icon, and two group kinds sharing one colour.
 */
import { chromium } from "playwright";

/*
  Defaults to the built container rather than the dev server: 8081 is what the compose file
  serves and what every verification in this repo runs against.
*/
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

// The web container answers at once; the API behind it can take several seconds more.
await waitForServer(url);
await waitForServer(`${url}/api/health`);
const browser = await chromium.launch();

const palettes = {};
for (const scheme of ["light", "dark"]) {
  const page = await browser.newPage({ viewport: { width: 1600, height: 1000 }, colorScheme: scheme });
  page.on("pageerror", (e) => check(`${scheme}: no page error`, false, String(e)));
  await page.goto(`${url}/?t=retail/6/250/spec`, { waitUntil: "networkidle" });
  await page.waitForSelector(".ttm-node", { timeout: 20000 });
  await page.waitForTimeout(800);

  if (scheme === "light") {
    const shapes = await page.$$eval(".ttm-node", (els) => {
      const out = {};
      for (const el of els) out[el.dataset.shape] = (out[el.dataset.shape] ?? 0) + 1;
      return out;
    });
    console.log("   shapes:", JSON.stringify(shapes));
    check("active abilities are square", (shapes.active ?? 0) > 0);
    check("choice nodes are hexagons", (shapes.choice ?? 0) > 0);
    check("passives are circles", (shapes.passive ?? 0) > 0);
    const clip = await page.$eval('.ttm-node[data-shape="choice"] .ttm-node-ring', (el) => getComputedStyle(el).clipPath);
    check("the hexagon reaches the ring layer, so its ring is not clipped away", clip.includes("polygon"), clip);
  }

  // Paint one talent required on the (open) spec tree and read its ring.
  const spec = page.locator('section[aria-label="Spec"]');
  await spec.locator('.ttm-node[data-shape="passive"]').first().click();
  await page.waitForTimeout(400);
  const ring = await page.$eval('.ttm-node[data-state="required"]', (el) => ({
    keyline: getComputedStyle(el.querySelector(".ttm-node-ring")).backgroundColor,
    band: getComputedStyle(el.querySelector(".ttm-node-band")).backgroundColor,
    inner: getComputedStyle(el.querySelector(".ttm-node-face")).boxShadow,
  }));
  check(`${scheme}: a required ring has a keyline outside its colour`, ring.keyline !== ring.band, JSON.stringify(ring));
  check(`${scheme}: and one inside it`, ring.inner.includes("inset"));

  const legend = await page.$$eval(".ttm-swatch[data-state]", (els) =>
    els.map((el) => getComputedStyle(el.querySelector(".ttm-swatch-band")).backgroundColor),
  );
  check(`${scheme}: the legend names four constraint kinds`, legend.length === 4, String(legend.length));
  check(`${scheme}: in four distinct colours`, new Set(legend).size === 4, legend.join(" "));
  palettes[scheme] = ring.band;

  const text = await page.locator("aside").innerText();
  // Described always, not only while that tool is selected.
  check(`${scheme}: at-least-one is described`, /one or more of the group/i.test(text));
  check(`${scheme}: exactly-one is described`, /one of the group only/i.test(text));
  await page.screenshot({ path: `shots/canvas-${scheme}.png` });
  await page.close();
}
check("each theme keeps its own palette", palettes.light !== palettes.dark);

await browser.close();
console.log(failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall canvas checks passed");
process.exit(failures.length ? 1 : 0);
