/**
 * Shared by the browser suites: wait for the site, and reference counts to check the page
 * against.
 *
 * The site has no API of its own any more. A suite that wants to know how many builds a
 * search has asks the counter directly, in Node, over the site's own data files -- the same
 * files the page reads. That checks the page's plumbing (what it asks, how it pools, what it
 * shows); whether the counter itself is right is the parity suite's job, against the C++
 * engine (parity.test.mjs).
 */
import { createServer } from "vite";

export async function waitForServer(target, timeoutMs = 30000) {
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

/** The site is up once the page and its data answer. */
export async function waitForSite(url) {
  await waitForServer(url);
  await waitForServer(`${url}/data/retail/index.json`);
}

let engine = null;
async function load() {
  if (!engine) {
    // Only to load the engine's TypeScript: no app config, no dependency scan, no websocket,
    // so it cannot collide with a dev server or another suite running beside it.
    const server = await createServer({
      configFile: false,
      server: { middlewareMode: true, hmr: false, ws: false },
      optimizeDeps: { noDiscovery: true, include: [] },
      appType: "custom",
      logLevel: "error",
    });
    engine = { server, service: await server.ssrLoadModule("/src/engine/service.ts") };
  }
  return engine;
}

const trees = new Map();
/** A tree as the page sees it: a data file, or a saved custom project's tree. */
export async function treeOf(url, key) {
  if (!trees.has(key)) {
    if (key.startsWith("custom/")) {
      const saved = await (await fetch(`${url}/api/custom-trees/${key.split("/")[1]}`)).json();
      for (const t of saved.trees) trees.set(t.key, t);
    } else {
      const r = await fetch(`${url}/data/trees/${key}.json`);
      if (!r.ok) throw new Error(`no tree ${key}`);
      trees.set(key, await r.json());
    }
  }
  return trees.get(key);
}

/**
 * Sets and builds for a search, as the old POST /api/counts answered: `{sets, builds}`, or
 * `{status, detail}` for a search the API would have refused.
 */
export async function count(url, body) {
  const { service } = await load();
  const { treeKey, points, levelCap: _levelCap, ...search } = body;
  try {
    return service.countAt(treeKey, await treeOf(url, treeKey), search, points);
  } catch (e) {
    if (e instanceof service.SearchError) return { status: e.status, detail: e.message };
    throw e;
  }
}

/** Builds at every point total 0..k for a tree, under a search. */
export async function spread(url, key, search = {}) {
  const { service } = await load();
  return service.spreadOf(key, await treeOf(url, key), search).builds;
}

export async function close() {
  if (engine) await engine.server.close();
  engine = null;
}

/** The old API's read paths, answered from the site's files: /trees and /trees/<key>. */
export async function siteGet(url, path) {
  if (path === "/trees" || path.startsWith("/trees?")) {
    const game = new URLSearchParams(path.split("?")[1] ?? "").get("game") ?? "retail";
    return (await (await fetch(`${url}/data/${game}/index.json`)).json()).trees;
  }
  if (path.startsWith("/trees/")) {
    const r = await fetch(`${url}/data/trees/${path.slice("/trees/".length)}.json`);
    if (!r.ok) throw new Error(`${path}: ${r.status}`);
    return r.json();
  }
  throw new Error(`no site path for ${path}`);
}

/** Every build a search matches, as the old /solve listed them. */
export async function listBuilds(url, key, body, limit = 20000) {
  const { service } = await load();
  const { points, levelCap: _levelCap, maxResults: _max, ...search } = body;
  return service.listAt(key, await treeOf(url, key), search, points, limit);
}
