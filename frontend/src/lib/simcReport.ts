/**
 * Reading a SimulationCraft JSON report back.
 *
 * The return leg. The export hands SimC a profileset per build; this reads the numbers it
 * produced and puts them back on the builds they came from, which is the only step that can
 * answer "which of these is best" — the enumeration knows what is *possible*, never what is
 * good.
 *
 * The shape below is not a guess. It was read out of SimC's own writer
 * (`engine/report/json/report_json.cpp`, `profileset_json2` / `profileset_json3`) and then
 * checked against a report produced by running SimulationCraft 1210-01 on an export from
 * this app. Both writer versions are handled because which one you get depends on the
 * `json2=` / `json3=` option the player used:
 *
 *   v2  results[] = { name, mean, min, max, stddev, mean_stddev, mean_error, median, ... }
 *   v3  results[] = { name, metrics: [ { metric, mean, ... } ] }
 *
 * Two things the real file taught that the source alone did not make obvious: the results
 * come back in no particular order, and a profileset whose mean is zero is dropped from a v2
 * report entirely rather than reported as zero.
 */

export interface SimcResult {
  name: string;
  mean: number;
  /** Confidence interval half-width at the sim's confidence level. 0 when not reported. */
  error: number;
  min: number;
  max: number;
  iterations: number;
}

export interface SimcReport {
  /** "Damage per Second", "Healing per Second" — SimC's own words for what it measured. */
  metric: string;
  results: SimcResult[];
  /** The profile's own number, before any profileset replaced its talents. */
  baseline: number | null;
  /** Which player the baseline came from, for the UI to name. */
  player: string | null;
  version: string | null;
}

const num = (v: unknown): number => (typeof v === "number" && Number.isFinite(v) ? v : 0);

function asRecord(v: unknown): Record<string, unknown> | null {
  return typeof v === "object" && v !== null && !Array.isArray(v)
    ? (v as Record<string, unknown>)
    : null;
}

/**
 * One result row, from either writer.
 *
 * A v3 row carries a `metrics` array because SimC can rank on several metrics at once; the
 * first is the one it sorted by, which is the one the player asked for.
 */
function resultOf(raw: unknown): SimcResult | null {
  const row = asRecord(raw);
  if (!row || typeof row.name !== "string") return null;

  const metrics = Array.isArray(row.metrics) ? asRecord(row.metrics[0]) : null;
  const source = metrics ?? row;
  if (typeof source.mean !== "number") return null;

  return {
    name: row.name,
    mean: num(source.mean),
    // mean_error is mean_stddev scaled by the sim's confidence estimator, which is the
    // number SimC itself prints as the error bar. Falling back to the raw stddev of the
    // mean would quietly show a tighter interval than the sim claims.
    error: num(source.mean_error) || num(source.mean_stddev),
    min: num(source.min),
    max: num(source.max),
    iterations: num(source.iterations),
  };
}

export class SimcReportError extends Error {}

/**
 * Parse a report, or say precisely what is wrong with it.
 *
 * The errors matter more than usual here: the file arrives from another program, by hand,
 * after a sim that may have taken an hour, and "invalid JSON" would leave a player with no
 * idea whether they exported wrong, simmed wrong or dropped the wrong file.
 */
export function parseReport(text: string): SimcReport {
  let json: unknown;
  try {
    json = JSON.parse(text);
  } catch {
    throw new SimcReportError(
      "That is not JSON. SimulationCraft writes this file with the json2= option — an HTML report cannot be read.",
    );
  }

  const root = asRecord(json);
  const sim = root && asRecord(root.sim);
  if (!sim) {
    throw new SimcReportError("No sim section — this is not a SimulationCraft report.");
  }

  const profilesets = asRecord(sim.profilesets);
  if (!profilesets || !Array.isArray(profilesets.results)) {
    throw new SimcReportError(
      "The report has no profilesets. It looks like a single-character sim: the export produces one profileset per build, and all of them have to be in the same run.",
    );
  }

  const results = profilesets.results.map(resultOf).filter((r): r is SimcResult => r !== null);
  if (!results.length) {
    throw new SimcReportError("The report has profilesets, but none of them produced a result.");
  }

  const players = Array.isArray(sim.players) ? sim.players : [];
  const first = asRecord(players[0]);
  const dps = first && asRecord(asRecord(first.collected_data)?.dps);

  return {
    metric: typeof profilesets.metric === "string" ? profilesets.metric : "the sim metric",
    results,
    baseline: dps ? num(dps.mean) : null,
    player: first && typeof first.name === "string" ? first.name : null,
    version: root && typeof root.version === "string" ? root.version : null,
  };
}

export interface RankedBuild {
  /** Index into the export's deduplicated string list, which is the profileset number. */
  line: number;
  mean: number;
  error: number;
  /** 0 is the best build in the report. */
  rank: number;
  /** Difference from the best, as a fraction: -0.031 is 3.1% behind. */
  behind: number;
}

export interface Ranking {
  metric: string;
  builds: RankedBuild[];
  /** Keyed by line number, for a browser stepping through builds. */
  byLine: Map<number, RankedBuild>;
  best: RankedBuild;
  worst: RankedBuild;
  /** How much the whole set spans, as a fraction of the worst. */
  spread: number;
  baseline: number | null;
  /** Profileset names the report had that this export did not produce. */
  unmatched: string[];
  /** Lines this export produced that the report has no number for. */
  missing: number[];
}

/**
 * Line up a report against the export that produced it.
 *
 * Matching is by profileset name, which is where the zero-padding in the export earns its
 * keep: `ttm_07` is line 7 whatever order the report lists it in. A name that does not
 * parse, or a line the report never mentions, is reported rather than dropped — a player
 * who simmed the wrong file should be told, not silently shown a ranking of four builds.
 */
export function rank(report: SimcReport, lines: number, label = "ttm"): Ranking {
  const prefix = new RegExp(`^${label}_(\\d+)$`);
  const seen = new Map<number, SimcResult>();
  const unmatched: string[] = [];

  for (const result of report.results) {
    const match = prefix.exec(result.name);
    const line = match ? Number(match[1]) : NaN;
    if (!Number.isFinite(line) || line < 1 || line > lines) {
      unmatched.push(result.name);
      continue;
    }
    seen.set(line, result);
  }

  if (!seen.size) {
    throw new SimcReportError(
      `None of the ${report.results.length} profilesets in this report came from this export. Names here look like "${report.results[0]?.name ?? "?"}", and the export writes "${label}_1".`,
    );
  }

  const builds: RankedBuild[] = [...seen.entries()]
    .map(([line, result]) => ({
      line,
      mean: result.mean,
      error: result.error,
      rank: 0,
      behind: 0,
    }))
    .sort((a, b) => b.mean - a.mean);

  const best = builds[0]!;
  builds.forEach((build, i) => {
    build.rank = i;
    build.behind = best.mean > 0 ? build.mean / best.mean - 1 : 0;
  });

  const worst = builds[builds.length - 1]!;
  const missing: number[] = [];
  for (let line = 1; line <= lines; line++) if (!seen.has(line)) missing.push(line);

  return {
    metric: report.metric,
    builds,
    byLine: new Map(builds.map((b) => [b.line, b])),
    best,
    worst,
    spread: worst.mean > 0 ? best.mean / worst.mean - 1 : 0,
    baseline: report.baseline,
    unmatched,
    missing,
  };
}

export interface TalentImpact {
  nodeId: number;
  /** Mean of the builds that take it. */
  withIt: number;
  /** Mean of the builds that do not. */
  without: number;
  /** withIt - without, as a fraction of `without`. */
  delta: number;
  /** How many of the ranked builds take it. */
  taken: number;
}

/**
 * What each talent is worth across the set that was simmed.
 *
 * The mean of the builds taking a talent against the mean of those that do not. This is the
 * number the whole tool exists to produce and it cannot be got any other way: a sim ranks
 * whole characters, so the value of one talent only appears once you have simmed a set of
 * builds that differ in a controlled way — which is exactly what an enumeration under
 * constraints is.
 *
 * It is an *observational* difference, not a controlled one. Talents that are only ever
 * taken together cannot be told apart by it, and a talent taken by every build has nothing
 * to compare against and is left out. Saying so is part of the result.
 */
export function impact(
  ranking: Ranking,
  buildsForLine: (line: number) => Iterable<number> | undefined,
): TalentImpact[] {
  const sums = new Map<number, { withSum: number; withN: number }>();
  let total = 0;
  let n = 0;

  for (const build of ranking.builds) {
    const nodes = buildsForLine(build.line);
    if (!nodes) continue;
    total += build.mean;
    n += 1;
    for (const nodeId of new Set(nodes)) {
      const entry = sums.get(nodeId) ?? { withSum: 0, withN: 0 };
      entry.withSum += build.mean;
      entry.withN += 1;
      sums.set(nodeId, entry);
    }
  }

  const out: TalentImpact[] = [];
  for (const [nodeId, { withSum, withN }] of sums) {
    // Taken by everything, or by nothing: there is no comparison to make, and reporting
    // "+0%" for a talent every build has would read as "this talent does nothing".
    if (withN === 0 || withN === n) continue;
    const withIt = withSum / withN;
    const without = (total - withSum) / (n - withN);
    out.push({
      nodeId,
      withIt,
      without,
      delta: without > 0 ? withIt / without - 1 : 0,
      taken: withN,
    });
  }
  return out.sort((a, b) => b.delta - a.delta);
}
