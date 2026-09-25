/**
 * Typed client for the TTM API.
 *
 * Everything goes through /api, which Vite proxies in development and a reverse proxy
 * serves in production. Same-origin either way, so there is no CORS configuration that
 * exists only for development and then has to be remembered in deployment.
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

/** Each game is its own revision with its own age, so health is asked per game. */
export const getHealth = (game: Game = "retail") => request<Health>(`/health?game=${game}`);

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
  /** Per-rank unlock levels for a tiered node; null for everything else. */
  rankLevels: number[] | null;
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

export const listTrees = (params: Record<string, string | number> = {}) => {
  const query = new URLSearchParams(
    Object.entries(params).map(([k, v]) => [k, String(v)]),
  ).toString();
  return request<TreeSummary[]>(`/trees${query ? `?${query}` : ""}`);
};

export const getTree = (key: string) => request<TreeDetail>(`/trees/${key}`);

export interface TreeCountRow {
  points: number;
  sets: number;
  builds: number;
}

export const getTreeCounts = (key: string) =>
  request<TreeCountRow[]>(`/trees/${key}/counts`);

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

export const countBuilds = (treeKey: string, constraints: Constraints) =>
  request<CountResult>("/counts", {
    method: "POST",
    body: JSON.stringify({ treeKey, ...constraints }),
  });

/* --- solve jobs ------------------------------------------------------------ */

export type JobState =
  | "queued"
  | "running"
  | "done"
  | "capped"
  | "cancelled"
  | "failed";

export type JobPhase = "solving" | "storing" | "finalizing" | null;

export interface Job {
  id: string;
  state: JobState;
  treeKey: string;
  points: number;
  expectedCount: number | null;
  resultCount: number | null;
  progress: number;
  phase: JobPhase;
  cancelRequested: boolean;
  error: string | null;
  createdAt: string;
  finishedAt: string | null;
}

export const TERMINAL_STATES: ReadonlySet<JobState> = new Set([
  "done",
  "capped",
  "cancelled",
  "failed",
]);

export const submitSolve = (
  treeKey: string,
  constraints: Constraints & { maxResults?: number; timeBudgetMs?: number },
) =>
  request<Job>("/solve", {
    method: "POST",
    body: JSON.stringify({ treeKey, ...constraints }),
  });

export const getJob = (id: string) => request<Job>(`/solve/${id}`);

export const cancelJob = (id: string) =>
  request<Job>(`/solve/${id}/cancel`, { method: "POST" });

export interface ResultPage {
  jobId: string;
  state: JobState;
  total: number;
  offset: number;
  /** nodeId (as a string key) -> points spent on that node. */
  builds: Record<string, number>[];
}

export const getResults = (id: string, offset = 0, limit = 100) =>
  request<ResultPage>(`/solve/${id}/results?offset=${offset}&limit=${limit}`);

export interface TalentStat {
  nodeId: number;
  /** How many of the job's results take this talent at all. */
  builds: number;
  /** That, as a fraction of the whole matching set. */
  share: number;
  /** Mean rank among the builds that take it, which is at least 1. */
  meanPoints: number;
  /** Taken by every matching build: the constraints already decided it. */
  mandatory: boolean;
}

export interface JobStats {
  jobId: string;
  state: JobState;
  total: number;
  /** Most common first. */
  talents: TalentStat[];
}

export const getStats = (id: string) => request<JobStats>(`/solve/${id}/stats`);

/* --- icons ----------------------------------------------------------------- */

/**
 * Icons are served with a one-year immutable cache, so this is a plain URL rather than a
 * fetch: the browser's own cache is the right cache, and after the first visit a tree
 * canvas makes no icon requests at all.
 *
 * A missing icon is expected (upstream has no art for ~1% of names), so callers must
 * render the talent without one rather than treating it as an error.
 */
export const iconUrl = (name: string | null, size: 18 | 36 | 56 = 56) =>
  name ? `${BASE}/icons/${name}?size=${size}` : null;

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
