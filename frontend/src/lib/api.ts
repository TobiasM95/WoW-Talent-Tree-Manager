import { countAt, listAt, SearchError, spreadOf } from "../engine/service";

/**
 * Where the page's data comes from. There is no server of ours: the site is static.
 *
 *   - Trees, their indexes and icons are files, built by CI from the ingest
 *     (tools/site/build_data.py) and served by Cloudflare Pages.
 *   - Counting and listing builds run here, in the page (src/engine), kept at parity with
 *     the C++ engine by the release CI.
 *   - Two things need a server and get a Cloudflare Pages Function each, under /api: top
 *     players from WarcraftLogs, whose key must stay secret, and saved custom projects.
 *
 * The function names and shapes are the old API's, so the page above this barely changed.
 */

const BASE = "/api";

export class ApiError extends Error {
  constructor(
    readonly status: number,
    readonly detail: string,
  ) {
    super(detail || `HTTP ${status}`);
  }
}

export async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(BASE + path, {
    ...init,
    headers: { "Content-Type": "application/json", ...init?.headers },
  });
  if (!response.ok) {
    // The API puts the useful sentence in `detail`; surfacing it beats "Bad Request",
    // since these are messages written for a person (which node, which budget, why).
    let detail = "";
    try {
      detail = ((await response.json()) as { detail?: string }).detail ?? "";
    } catch {
      detail = response.statusText;
    }
    throw new ApiError(response.status, detail);
  }
  return (await response.json()) as T;
}

/* --- health ---------------------------------------------------------------- */

export interface Health {
  status: string;
  revision: number;
  trees: number;
  nodes: number;
  descriptionCoverage: number | null;
  iconCoverage: number | null;
  dataAgeSeconds: number;
}

interface GameIndex {
  game: string;
  revision: number;
  fetchedAt: string | null;
  attribution: string | null;
  trees: TreeSummary[];
  nodeCount: number;
}

/** A static file, fetched once per page load. */
const files = new Map<string, Promise<unknown>>();
function file<T>(path: string): Promise<T> {
  let hit = files.get(path);
  if (!hit) {
    hit = fetch(path).then(async (r) => {
      if (!r.ok) throw new ApiError(r.status, r.status === 404 ? `no ${path}` : r.statusText);
      return r.json();
    });
    hit.catch(() => files.delete(path));
    files.set(path, hit);
  }
  return hit as Promise<T>;
}

const indexOf = (game: Game) => file<GameIndex>(`/data/${game}/index.json`);

/** Each game is its own revision with its own age. */
export const getHealth = async (game: Game = "retail"): Promise<Health> => {
  const index = await indexOf(game === "custom" ? "retail" : game);
  return {
    status: "ok",
    revision: index.revision,
    trees: index.trees.length,
    nodes: index.nodeCount,
    descriptionCoverage: null,
    iconCoverage: null,
    dataAgeSeconds: index.fetchedAt ? Math.max(0, (Date.now() - Date.parse(index.fetchedAt)) / 1000) : 0,
  };
};

/* --- trees ----------------------------------------------------------------- */

/** `tab` is a classic talent tab: one of a class's three trees, sharing one point pool. */
export type TreeKind = "class" | "spec" | "hero" | "tab";

/**
 * Which game's trees. Retail has class, spec and hero trees; Forever has three tabs a class;
 * custom trees are a player's own project of one to three.
 */
export type Game = "retail" | "forever" | "custom";

export interface TreeSummary {
  key: string;
  kind: TreeKind;
  name: string;
  className: string;
  specName: string | null;
  subTreeId: number | null;
  nodeCount: number;
  maxPointsInTree: number;
  pointCap: number | null;
  /** A classic tab's place among its class's three; null for retail trees. */
  order?: number | null;
  /** A custom retail-style hero tree: the specs that may take it. */
  heroSpecs?: string[] | null;
}

export type NodeKind = "single" | "choice" | "tiered" | "subtree";

export interface TalentEntry {
  entryId: number;
  definitionId: number | null;
  spellId: number | null;
  visibleSpellId: number | null;
  name: string;
  kind: string;
  icon: string | null;
  index: number | null;
  maxRanks: number | null;
  /** Description per rank. Empty when the ingest ran without --descriptions. */
  ranks: string[];
}

/**
 * Talent node, exactly as the API sends it.
 *
 * `pos` is nested, and is the game's own coordinate space (tens of thousands of units) --
 * not pixels. The canvas normalises it. Hand-writing these interfaces against a guess is
 * how `posX` ended up here once; every field below is checked against a live response.
 */
export interface TalentNode {
  nodeId: number;
  kind: NodeKind;
  name: string;
  pos: { x: number; y: number };
  row: number | null;
  col: number | null;
  maxPoints: number;
  pointsRequired: number;
  parents: number[];
  children: number[];
  entries: TalentEntry[];
  /** A tiered node's ranks by character level ({level, maxRanks} steps); null otherwise. */
  rankLevels: { level: number; maxRanks: number }[] | null;
  /** A root of the tree: reachable with nothing else spent. */
  entryNode: boolean;
  /** Granted rather than chosen, so it costs no point. */
  preFilled: boolean;
  /** A specific prerequisite node, distinct from the row's point gate. */
  requiresNode: number | null;
  freeLevel: number | null;
  localId: number | null;
  subTreeId: number | null;
}

export interface TreeGating {
  [pointsRequired: string]: number;
}

export interface TreeDetail extends TreeSummary {
  id: string;
  game: string;
  classId: number;
  specId: number | null;
  traitTreeId: number | null;
  description: string | null;
  schemaVersion: number;
  gating: TreeGating | null;
  source: Record<string, unknown> | null;
  /**
   * The order Blizzard's loadout string walks. Present on spec trees only.
   *
   * It belongs to the *class's* trait tree rather than to this one: 206 entries for a Death
   * Knight against 114 nodes across its three trees, because it also contains the other
   * specs' nodes, and it is identical for every spec of a class. A loadout string emits one
   * entry per id in this list, selected or not, so it cannot be rebuilt from the split trees.
   */
  fullNodeOrder: number[] | null;
  /**
   * The node recording *which* hero tree a loadout uses, and the sub-tree ids its choice
   * index selects, in that index's order. Present on spec trees only.
   *
   * It is not a talent and appears in none of our trees — it is a chooser, written into the
   * loadout string as a choice node. Without it a string round-trips every talent and still
   * loses which hero tree they belong to.
   */
  subTreeSelector: { nodeId: number; subTreeIds: number[] } | null;
  /**
   * Forever: the one point pool a class's three tabs share (51), and the points each row
   * needs. Recorded on the data rather than assumed in the client, since the export does not
   * state them and they are one edit away from changing.
   */
  sharedPointCap?: number;
  pointsPerRow?: number;
  /** Forever and custom: this tab's place among its project's trees. */
  order?: number;
  /** Custom: the saved project this tree belongs to. */
  project?: string;
  /** Whose data this is, when the licence asks to say so. */
  attribution?: { name: string; url: string; license: string };
  nodes: TalentNode[];
}

/** A game's trees. Custom projects are not listed: they are opened by link. */
export const listTrees = async (params: { game?: Game; kind?: TreeKind } = {}): Promise<TreeSummary[]> => {
  const game = params.game ?? "retail";
  if (game === "custom") throw new ApiError(400, "custom trees are listed per project: open one by its link");
  const trees = (await indexOf(game)).trees;
  return params.kind ? trees.filter((t) => t.kind === params.kind) : trees;
};

/** Custom trees are built from their saved design (lib/projects.ts) and remembered here. */
const customTrees = new Map<string, TreeDetail>();
export const rememberCustomTrees = (trees: TreeDetail[]) => trees.forEach((t) => customTrees.set(t.key, t));

export const getTree = async (key: string): Promise<TreeDetail> => {
  if (key.startsWith("custom/")) {
    if (!customTrees.has(key)) {
      const { getProject } = await import("./projects");
      await getProject(key.split("/")[1]!);
    }
    const tree = customTrees.get(key);
    if (!tree) throw new ApiError(404, `no custom tree '${key}'`);
    return tree;
  }
  return file<TreeDetail>(`/data/trees/${key}.json`);
};

/** The engine's errors, in the shape the page has always read. */
function engine<T>(run: () => T): T {
  try {
    return run();
  } catch (e) {
    if (e instanceof SearchError) throw new ApiError(e.status, e.message);
    throw e;
  }
}

/* --- the gate -------------------------------------------------------------- */

export type ChoiceSide = "a" | "b" | "none";

export interface Constraints {
  points: number;
  levelCap?: number;
  mustHave?: number[];
  mustNotHave?: number[];
  choiceSides?: Record<string, ChoiceSide>;
  atLeastOneOf?: number[][];
  exactlyOneOf?: number[][];
  /** Multi-rank talents: at least / at most so many ranks. */
  rankMin?: Record<string, number>;
  rankMax?: Record<string, number>;
}

export interface CountResult {
  treeKey: string;
  points: number;
  levelCap: number;
  sets: number;
  builds: number;
  filtered: boolean;
  source: "precomputed" | "computed";
  elapsedMs: number;
  listable: boolean;
  listingLimit: number;
}

export const countBuilds = async (treeKey: string, constraints: Constraints): Promise<CountResult> => {
  const tree = await getTree(treeKey);
  const started = performance.now();
  const { points, levelCap: _levelCap, ...search } = constraints;
  const { sets, builds } = engine(() => countAt(treeKey, tree, search, points));
  const filtered = Object.values(search).some((v) => v && (Array.isArray(v) ? v.length : Object.keys(v).length));
  return {
    treeKey,
    points,
    levelCap: 90,
    sets,
    builds,
    filtered,
    source: "computed",
    elapsedMs: performance.now() - started,
    listable: sets > 0,
    listingLimit: Number.MAX_SAFE_INTEGER,
  };
};

export interface SpreadResult {
  treeKey: string;
  /** Index k: selections spending exactly k points; index 0 is the empty tree. */
  sets: number[];
  builds: number[];
}

/** A tree's counts at every point total at once, for trees that share a pool. */
export const countSpread = async (treeKey: string, constraints: Constraints): Promise<SpreadResult> => {
  const tree = await getTree(treeKey);
  const { points: _points, levelCap: _levelCap, ...search } = constraints;
  return { treeKey, ...engine(() => spreadOf(treeKey, tree, search)) };
};

/* --- listing builds ------------------------------------------------------- */

/** Every build a search matches, keyed by talent id, granted talents left out. */
export const listBuilds = async (treeKey: string, constraints: Constraints, limit: number): Promise<Record<string, number>[]> => {
  const tree = await getTree(treeKey);
  const { points, levelCap: _levelCap, ...search } = constraints;
  return engine(() => listAt(treeKey, tree, search, points, limit));
};

/* --- icons ----------------------------------------------------------------- */

/**
 * Icons are static files, one per name at 56px, cached hard by the browser; smaller sizes are
 * the same file scaled by CSS. A missing icon is expected (upstream has no art for ~1% of
 * names), so callers render the talent without one rather than treating it as an error.
 */
export const iconUrl = (name: string | null, _size: 18 | 36 | 56 = 56) => (name ? `/icons/${name}.jpg` : null);

/** Icon names that have an image, for the editor's picker. */
export const searchIcons = async (term: string, limit = 48): Promise<string[]> => {
  const all = await file<string[]>("/data/icons.json");
  const words = term.toLowerCase().split(/\s+/).filter(Boolean);
  return all.filter((n) => words.every((w) => n.includes(w))).slice(0, limit);
};

/* --- what top players run (WarcraftLogs) ------------------------------------ */

export interface PopularContent {
  zoneId: number;
  name: string;
  kind: "raid" | "dungeon";
  difficulties: { id: number; name: string }[];
  encounters: { id: number; name: string }[];
}

export interface PopularBuild {
  count: number;
  best: number;
  median: number;
  /** Tree key -> nodeId -> points, granted talents excluded. */
  points: Record<string, Record<string, number>>;
  /** nodeId -> which alternative of a choice node, 0 or 1. */
  choices: Record<string, number>;
  hero: string | null;
  example: string;
}

export interface Popular {
  zone: { id: number; name: string };
  encounters: string[];
  difficulty: number;
  players: number;
  heroes: { key: string | null; count: number }[];
  /** nodeId -> share of players taking it, and their mean rank. */
  pickRates: Record<string, { share: number; meanRank: number }>;
  /** nodeId -> [share on the left alternative, share on the right], among those taking it. */
  choiceSides: Record<string, [number, number]>;
  builds: PopularBuild[];
  distinctBuilds: number;
  /** Real builds our tree data would not allow: a sign the data has drifted. */
  illegal: string[];
  fetchedAt: number;
}

export const getPopularContent = () => request<PopularContent[]>("/popular/content");

export const getPopular = (specKey: string, zone: number, encounter: string, difficulty: number) =>
  request<Popular>(
    `/popular/${specKey}?zone=${zone}&encounter=${encodeURIComponent(encounter)}&difficulty=${difficulty}`,
  );
