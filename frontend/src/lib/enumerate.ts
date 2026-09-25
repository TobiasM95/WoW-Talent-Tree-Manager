import { ApiError, getJob, getResults, submitSolve, TERMINAL_STATES, type TreeDetail } from "./api";
import type { Points } from "./loadout";
import { characters, expand, fixedVariant, type Character, type Variant } from "./space";
import { payloadOf, type TreeWork } from "./workspace";

/**
 * Turn the three trees into the list of characters to sim.
 *
 * A fixed tree is its one build. An open tree is enumerated by the solver -- the same job
 * machinery as before, with its caching, so re-entering this step with unchanged searches
 * costs nothing -- and each selection is expanded into both sides of every free choice node.
 * The characters are then the product, in a stable order.
 */

export interface TreeInput {
  key: string;
  tree: TreeDetail;
  work: TreeWork;
  cap: number;
  /**
   * Which factor of the product this tree belongs to. Trees sharing a slot are alternatives
   * -- both hero trees at once -- so their builds are pooled and the factor is their sum.
   * Absent means a slot of its own.
   */
  slot?: string;
  /** For a hero tree, its sub-tree id, which every talent string built from it must name. */
  hero?: number | null;
}

export interface Progress {
  key: string;
  state: "waiting" | "solving" | "fetching" | "done" | "failed";
  /** Builds this tree contributes, once known. */
  builds: number | null;
  error?: string;
}

const POLL_MS = 400;
const PAGE = 1000;
const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

async function selections(input: TreeInput, limit: number, signal: AbortSignal): Promise<Points[]> {
  let job = await submitSolve(input.key, { ...payloadOf(input.work, input.cap), maxResults: limit });
  while (!TERMINAL_STATES.has(job.state)) {
    if (signal.aborted) throw new DOMException("cancelled", "AbortError");
    await sleep(POLL_MS);
    job = await getJob(job.id);
  }
  if (job.state !== "done") {
    throw new Error(
      job.state === "capped"
        ? "The solver stopped before listing every build."
        : (job.error ?? `The solver ended ${job.state}.`),
    );
  }
  const out: Points[] = [];
  const total = job.resultCount ?? 0;
  for (let offset = 0; offset < total; offset += PAGE) {
    if (signal.aborted) throw new DOMException("cancelled", "AbortError");
    const page = await getResults(job.id, offset, PAGE);
    out.push(...page.builds);
    if (page.builds.length === 0) break;
  }
  return out;
}

export async function enumerate(
  inputs: TreeInput[],
  limit: number,
  onProgress: (progress: Progress[]) => void,
  signal: AbortSignal,
): Promise<Character[]> {
  const progress: Progress[] = inputs.map((i) => ({
    key: i.key,
    state: i.work.mode === "fixed" ? "done" : "waiting",
    builds: i.work.mode === "fixed" ? 1 : null,
  }));
  const report = () => onProgress(progress.map((p) => ({ ...p })));
  report();

  const tag = (input: TreeInput, list: Variant[]): Variant[] =>
    list.map((v) => ({ ...v, tree: input.key, ...(input.hero !== undefined ? { hero: input.hero } : {}) }));

  const perTree: { key: string; slot: string; variants: Variant[] }[] = await Promise.all(
    inputs.map(async (input, at) => {
      const slot = input.slot ?? input.key;
      if (input.work.mode === "fixed") {
        return {
          key: input.key,
          slot,
          variants: tag(input, [fixedVariant(input.tree, input.work.points, input.work.picks)]),
        };
      }
      try {
        progress[at]!.state = "solving";
        report();
        const list = await selections(input, limit, signal);
        progress[at]!.state = "fetching";
        report();
        const expanded = list.flatMap((sel) => expand(input.tree, sel, input.work.search.sides));
        progress[at]!.state = "done";
        progress[at]!.builds = expanded.length;
        report();
        return { key: input.key, slot, variants: tag(input, expanded) };
      } catch (error) {
        progress[at]!.state = "failed";
        progress[at]!.error = error instanceof ApiError ? error.detail : String((error as Error).message ?? error);
        report();
        throw error;
      }
    }),
  );

  // Pool the trees that share a slot, keeping slot order stable so numbering is repeatable.
  const slots: { key: string; variants: Variant[] }[] = [];
  for (const t of perTree) {
    const existing = slots.find((s) => s.key === t.slot);
    if (existing) existing.variants.push(...t.variants);
    else slots.push({ key: t.slot, variants: [...t.variants] });
  }
  return characters(slots, limit);
}
