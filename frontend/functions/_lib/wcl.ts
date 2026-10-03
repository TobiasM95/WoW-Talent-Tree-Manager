/**
 * What top players actually run, from WarcraftLogs' public API (v2).
 *
 * A port of services/ingest/ttm_ingest/wcl.py and the API's /popular endpoints, run as a
 * Cloudflare Pages Function so the client secret never reaches a browser.
 *
 * The client-credentials flow reads public data -- rankings, reports -- with no user login and
 * so no redirect URL. Rankings with `includeCombatantInfo` carry each player's talents as
 * trait entry ids, which name not only the node but, for a choice node, which side.
 *
 * Two things learned from real data and kept: granted talents are listed and are dropped, and
 * a tiered node is listed once per rank, so ranks are summed, not overwritten.
 *
 * Budget: 3,600 points an hour across every visitor, one key. Answers are cached for six hours.
 */

export interface Env {
  WCL_CLIENT_ID?: string;
  WCL_CLIENT_SECRET?: string;
  PROJECTS: KVNamespace;
  ASSETS: Fetcher;
}

const TOKEN_URL = "https://www.warcraftlogs.com/oauth/token";
const API_URL = "https://www.warcraftlogs.com/api/v2/client";

export class WclError extends Error {
  constructor(message: string, readonly status = 502) {
    super(message);
  }
}

let token: { value: string; expires: number } | null = null;

async function accessToken(env: Env): Promise<string> {
  if (token && token.expires > Date.now() + 60_000) return token.value;
  if (!env.WCL_CLIENT_ID || !env.WCL_CLIENT_SECRET) {
    throw new WclError("WarcraftLogs is not configured: set WCL_CLIENT_ID and WCL_CLIENT_SECRET", 503);
  }
  const r = await fetch(TOKEN_URL, {
    method: "POST",
    headers: {
      Authorization: `Basic ${btoa(`${env.WCL_CLIENT_ID}:${env.WCL_CLIENT_SECRET}`)}`,
      "Content-Type": "application/x-www-form-urlencoded",
    },
    body: "grant_type=client_credentials",
  });
  if (!r.ok) throw new WclError(`WarcraftLogs refused the credentials (${r.status})`);
  const body = (await r.json()) as { access_token: string; expires_in?: number };
  token = { value: body.access_token, expires: Date.now() + (body.expires_in ?? 3600) * 1000 };
  return token.value;
}

async function gql<T>(env: Env, query: string, variables: Record<string, unknown> = {}): Promise<T> {
  const r = await fetch(API_URL, {
    method: "POST",
    headers: { Authorization: `Bearer ${await accessToken(env)}`, "Content-Type": "application/json" },
    body: JSON.stringify({ query, variables }),
  });
  if (!r.ok) throw new WclError(`WarcraftLogs answered ${r.status}`);
  const out = (await r.json()) as { data: T; errors?: { message?: string }[] };
  if (out.errors?.length) throw new WclError(out.errors[0]!.message ?? "query failed");
  return out.data;
}

export interface Zone {
  zoneId: number;
  name: string;
  kind: "raid" | "dungeon";
  difficulties: { id: number; name: string }[];
  encounters: { id: number; name: string }[];
}

/** The newest expansion's live raids and Mythic+ seasons, with their encounters. */
export async function currentContent(env: Env): Promise<Zone[]> {
  type Raw = { worldData: { expansions: { id: number; zones: { id: number; name: string; frozen: boolean; difficulties: { id: number; name: string }[]; encounters: { id: number; name: string }[] }[] }[] } };
  const data = await gql<Raw>(env, "{ worldData { expansions { id name zones { id name frozen difficulties { id name } encounters { id name } } } } }");
  const expansions = [...data.worldData.expansions].sort((a, b) => b.id - a.id);
  const out: Zone[] = [];
  for (const zone of expansions[0]?.zones ?? []) {
    if (zone.frozen || !zone.encounters.length) continue;
    if (["PTR", "Beta", "Complete Raid", "Dummy"].some((w) => zone.name.includes(w))) continue;
    const dungeon = zone.name.includes("Mythic+");
    out.push({
      zoneId: zone.id,
      name: zone.name,
      kind: dungeon ? "dungeon" : "raid",
      difficulties: dungeon ? [{ id: 10, name: "Mythic+" }] : zone.difficulties.filter((d) => d.id === 5 || d.id === 4),
      encounters: zone.encounters,
    });
  }
  return out;
}

/** WarcraftLogs spells classes and specs without spaces: "Death Knight" -> "DeathKnight". */
const wclName = (text: string) => text.replace(/ /g, "");

export interface Ranked {
  name?: string;
  amount?: number;
  report?: { code?: string };
  talents?: { talentID: number; points: number }[];
}

export async function rankings(env: Env, className: string, specName: string, encounter: number, difficulty: number, pages = 1): Promise<Ranked[]> {
  const query =
    "query($c:String!,$s:String!,$e:Int!,$d:Int,$p:Int,$m:CharacterRankingMetricType){" +
    " worldData { encounter(id:$e) { characterRankings(className:$c, specName:$s," +
    " difficulty:$d, page:$p, metric:$m, includeCombatantInfo:true) } } }";
  const rows: Ranked[] = [];
  for (let page = 1; page <= pages; page++) {
    const data = await gql<{ worldData: { encounter: { characterRankings: { error?: string; rankings?: Ranked[]; hasMorePages?: boolean } | null } } }>(
      env,
      query,
      { c: wclName(className), s: wclName(specName), e: encounter, d: difficulty, p: page, m: "dps" },
    );
    const ranked = data.worldData.encounter.characterRankings ?? {};
    if (ranked.error) throw new WclError(ranked.error);
    rows.push(...(ranked.rankings ?? []));
    if (!ranked.hasMorePages) break;
  }
  return rows;
}

// --- in this tool's terms ------------------------------------------------------------------

export interface TreeNode {
  nodeId: number;
  name: string;
  kind: string;
  maxPoints: number;
  pointsRequired: number;
  preFilled: boolean;
  parents: number[];
  entries: { entryId: number | null }[];
}
export interface Tree {
  key: string;
  kind: string;
  className: string;
  specName: string | null;
  pointCap: number | null;
  nodes: TreeNode[];
}

type Index = Map<number, [string, TreeNode, number]>;

/** Every trait entry id in a spec's trees -> (tree key, node, which alternative). */
export function entryIndex(trees: Tree[]): Index {
  const out: Index = new Map();
  for (const tree of trees) {
    for (const node of tree.nodes) {
      (node.entries ?? []).forEach((e, i) => {
        if (e.entryId !== null && e.entryId !== undefined) out.set(Number(e.entryId), [tree.key, node, i]);
      });
    }
  }
  return out;
}

export interface Player {
  name?: string;
  amount?: number;
  points: Record<string, Record<string, number>>;
  choices: Record<string, number>;
}

/** One ranked player as a build in our terms: points per tree, choice sides, hero tree. */
export function convert(row: Ranked, index: Index): Player {
  const points: Player["points"] = {};
  const choices: Player["choices"] = {};
  for (const talent of row.talents ?? []) {
    const hit = index.get(Number(talent.talentID));
    if (!hit) continue;
    const [key, node, side] = hit;
    if (node.preFilled) continue; // granted: part of the character, not of the build
    const tree = (points[key] ??= {});
    const id = String(node.nodeId);
    if (node.kind === "choice" && (node.entries ?? []).length >= 2) {
      tree[id] = Number(talent.points);
      choices[id] = side;
    } else {
      // A tiered node reports one entry per rank: sum them, never take the last.
      tree[id] = (tree[id] ?? 0) + Number(talent.points);
    }
  }
  return { name: row.name, amount: row.amount, points, choices };
}

/** Why a build breaks our copy of a tree's rules, or null. Real builds test our data. */
export function problems(tree: Tree, build: Record<string, number>, cap: number | null): string | null {
  const nodes = new Map(tree.nodes.map((n) => [String(n.nodeId), n]));
  const granted = new Set([...nodes].filter(([, n]) => n.preFilled).map(([k]) => k));
  const spent = Object.values(build).reduce((a, b) => a + b, 0);
  if (cap !== null && spent > cap) return `spends ${spent}, over the cap of ${cap}`;
  for (const [key, rank] of Object.entries(build)) {
    const node = nodes.get(key);
    if (!node) return `talent ${key} is not in the tree`;
    if (rank > node.maxPoints) return `${node.name} at ${rank} of ${node.maxPoints}`;
  }
  // Gates, in the gate-first order any valid spending can be rearranged into.
  let soFar = 0;
  for (const key of Object.keys(build).sort((a, b) => nodes.get(a)!.pointsRequired - nodes.get(b)!.pointsRequired)) {
    const node = nodes.get(key)!;
    if (soFar < node.pointsRequired) return `${node.name} needs ${node.pointsRequired} spent before it, has ${soFar}`;
    soFar += build[key]!;
  }
  for (const key of Object.keys(build)) {
    const parents = nodes.get(key)!.parents.map(String).filter((p) => nodes.has(p));
    if (parents.length && !parents.some((p) => granted.has(p) || (build[p] ?? 0) >= nodes.get(p)!.maxPoints)) {
      return `${nodes.get(key)!.name} without a full-rank parent`;
    }
  }
  return null;
}

const sortedJson = (value: unknown): string =>
  Array.isArray(value)
    ? `[${value.map(sortedJson).join(",")}]`
    : value && typeof value === "object"
      ? `{${Object.keys(value as object).sort().map((k) => `${JSON.stringify(k)}:${sortedJson((value as Record<string, unknown>)[k])}`).join(",")}}`
      : JSON.stringify(value);

/** The players summarised: hero split, pick rates, choice sides, every distinct build. */
export function summarise(players: Player[], byKey: Map<string, Tree>, zone: Zone, encounters: Zone["encounters"], difficulty: number) {
  const n = players.length;
  const heroes = new Map<string | null, number>();
  const taken = new Map<string, number>();
  const ranks = new Map<string, number>();
  const sides = new Map<string, [number, number]>();
  const builds = new Map<string, { count: number; best: number | null; median?: number; amounts: number[]; points: Player["points"]; choices: Player["choices"]; hero: string | null; example?: string }>();
  const illegal: string[] = [];
  for (const p of players) {
    const hero = Object.keys(p.points).find((k) => k.includes("/hero/")) ?? null;
    heroes.set(hero, (heroes.get(hero) ?? 0) + 1);
    for (const [key, pts] of Object.entries(p.points)) {
      const tree = byKey.get(key);
      const problem = tree ? problems(tree, pts, tree.pointCap) : "unknown tree";
      if (problem && illegal.length < 12) {
        illegal.push(`${p.name} (${key.includes("/hero/") ? "hero" : key.split("/").pop()}): ${problem}`);
      }
      for (const [node, rank] of Object.entries(pts)) {
        taken.set(node, (taken.get(node) ?? 0) + 1);
        ranks.set(node, (ranks.get(node) ?? 0) + rank);
      }
    }
    for (const [node, side] of Object.entries(p.choices)) {
      const s = sides.get(node) ?? [0, 0];
      s[side === 1 ? 1 : 0]++;
      sides.set(node, s);
    }
    const signature = sortedJson([p.points, p.choices]);
    let entry = builds.get(signature);
    if (!entry) {
      entry = { count: 0, best: null, amounts: [], points: p.points, choices: p.choices, hero, example: p.name };
      builds.set(signature, entry);
    }
    entry.count++;
    entry.amounts.push(p.amount ?? 0);
  }
  // Every distinct build, most common first: real loadouts, offered to sim as they are.
  const common = [...builds.values()].sort((a, b) => b.count - a.count || Math.max(...b.amounts) - Math.max(...a.amounts));
  const out = common.map(({ amounts, ...b }) => {
    const sorted = [...amounts].sort((x, y) => x - y);
    return { ...b, best: sorted[sorted.length - 1]!, median: sorted[Math.floor(sorted.length / 2)]! };
  });
  return {
    zone: { id: zone.zoneId, name: zone.name },
    encounters: encounters.map((e) => e.name),
    difficulty,
    players: n,
    heroes: [...heroes].sort((a, b) => b[1] - a[1]).map(([key, count]) => ({ key, count })),
    pickRates: Object.fromEntries([...taken].map(([node, c]) => [node, { share: c / n, meanRank: ranks.get(node)! / c }])),
    choiceSides: Object.fromEntries([...sides].map(([node, [a, b]]) => [node, [a / (a + b), b / (a + b)]])),
    builds: out,
    distinctBuilds: builds.size,
    illegal,
    fetchedAt: Date.now() / 1000,
  };
}

/** Serve a cached answer while fresh; otherwise compute, cache for `ttl` seconds, serve. */
export async function cached<T>(request: Request, key: string, ttl: number, compute: () => Promise<T>): Promise<Response> {
  const cache = (caches as unknown as { default: Cache }).default;
  const cacheKey = new Request(new URL(`/__cache/${encodeURIComponent(key)}`, request.url).toString());
  const hit = await cache.match(cacheKey);
  if (hit) return hit;
  const body = await compute();
  const response = new Response(JSON.stringify(body), {
    headers: { "Content-Type": "application/json", "Cache-Control": `public, max-age=${ttl}` },
  });
  await cache.put(cacheKey, response.clone());
  return response;
}

export const json = (body: unknown, status = 200) =>
  new Response(JSON.stringify(body), { status, headers: { "Content-Type": "application/json" } });

/** The API's error shape, so the page reads it as it always did. */
export const failure = (status: number, detail: string) => json({ detail }, status);
