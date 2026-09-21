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

async function request<T>(path: string, init?: RequestInit): Promise<T> {
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

export const getHealth = () => request<Health>("/health");

/* --- trees ----------------------------------------------------------------- */

export type TreeKind = "class" | "spec" | "hero";

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
