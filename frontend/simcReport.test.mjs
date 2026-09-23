/**
 * The SimulationCraft report reader, against a report SimulationCraft actually wrote.
 *
 *   node simcReport.test.mjs
 *
 * `fixtures/simc-report.json` is the profileset section of a real run: SimC 1210-01, 67
 * profilesets this app exported from a real enumeration, simmed over SimC's own sample Blood
 * Death Knight profile at 100 iterations. Reading the writer's source said what the keys are
 * called; running it taught the rest — results come back unordered, a zero-mean profileset is
 * dropped rather than reported as zero, and the metric is a sentence rather than a token.
 *
 * A parser written against a guessed schema is how `posX` once got into the API types, so
 * this one is checked against the artefact instead of against my reading of it.
 */
import { readFile } from "node:fs/promises";
import { createServer } from "vite";

const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
const R = await server.ssrLoadModule("/src/lib/simcReport.ts");

const failures = [];
const check = (name, ok, detail = "") => {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}${ok || !detail ? "" : ` -- ${detail}`}`);
  if (!ok) failures.push(name);
};
const threw = (fn, wanted) => {
  try {
    fn();
    return false;
  } catch (exc) {
    return exc instanceof R.SimcReportError && (!wanted || exc.message.includes(wanted));
  }
};

const text = await readFile(new URL("./fixtures/simc-report.json", import.meta.url), "utf8");

/* --- the real report ----------------------------------------------------- */

const report = R.parseReport(text);
check("a real report parses", report.results.length === 67, `${report.results.length} results`);
check("the metric comes through", report.metric === "Damage per Second", report.metric);
check("the version comes through", report.version === "1210-01", String(report.version));
check(
  "the baseline is the profile's own number",
  Math.round(report.baseline) === 69519,
  String(report.baseline),
);
check("and it names the player", /Death_Knight_Blood/.test(report.player ?? ""), report.player);

const first = report.results[0];
check("every row carries a mean", report.results.every((r) => r.mean > 0));
check(
  "and an error bar",
  report.results.every((r) => r.error > 0),
  `${first.name} ±${first.error.toFixed(1)}`,
);
check(
  "the report is not in name order",
  report.results[0].name !== "ttm_01",
  `first row is ${report.results[0].name}`,
);

/* --- ranking ------------------------------------------------------------- */

const ranking = R.rank(report, 67);
check("every profileset is matched to its line", ranking.builds.length === 67);
check("nothing is left over", ranking.unmatched.length === 0 && ranking.missing.length === 0);
check(
  "the best build is the highest mean",
  ranking.best.mean === Math.max(...report.results.map((r) => r.mean)),
  `${ranking.best.mean.toFixed(0)} on line ${ranking.best.line}`,
);
check("the best build is rank 0 and behind nothing", ranking.best.rank === 0 && ranking.best.behind === 0);
check(
  "the worst is last and says how far behind",
  ranking.worst.rank === 66 && ranking.worst.behind < 0,
  `${(ranking.worst.behind * 100).toFixed(1)}%`,
);
check(
  "the spread is reported",
  Math.abs(ranking.spread - 0.0587) < 0.002,
  `${(ranking.spread * 100).toFixed(1)}%`,
);
check("a line can be looked up directly", ranking.byLine.get(ranking.best.line) === ranking.best);

/*
  The number that decides whether this is worth reading at all.

  The spread across 67 builds is 5.9%, and each mean carries an error bar of about ±0.8%.
  So the top build is separated from the middle of the pack, but neighbouring builds are
  not separated from each other — and a ranking that presents rank 4 as beating rank 5 when
  the intervals overlap is telling the player something the sim did not say.
*/
const top = ranking.builds[0];
const second = ranking.builds[1];
check(
  "adjacent builds can be within each other's error",
  Math.abs(top.mean - second.mean) < top.error + second.error,
  `${(top.mean - second.mean).toFixed(0)} apart, ±${top.error.toFixed(0)} each — the UI has to say so`,
);

/* --- a report from somewhere else ---------------------------------------- */

check(
  "a report from a different export is refused, not silently half-matched",
  threw(() => R.rank(report, 67, "other"), "came from this export"),
);

const partial = { ...report, results: report.results.slice(0, 10) };
const short = R.rank(partial, 67);
check("a partial report ranks what it has", short.builds.length === 10);
check("and names the lines it is missing", short.missing.length === 57, `${short.missing.length}`);

/* --- the v3 writer ------------------------------------------------------- */

/*
  Built by hand from `profileset_json3` in SimC's source rather than from a run, because
  which writer you get depends on whether the player passed json2= or json3=, and a player
  who passes json3= should not be told their report is unreadable.
*/
const v3 = JSON.stringify({
  version: "1210-01",
  sim: {
    profilesets: {
      results: [
        { name: "ttm_1", metrics: [{ metric: "Damage per Second", mean: 200, mean_error: 3 }] },
        { name: "ttm_2", metrics: [{ metric: "Damage per Second", mean: 100, mean_error: 2 }] },
      ],
    },
    players: [{ name: "P", collected_data: { dps: { mean: 150 } } }],
  },
});
const parsed3 = R.parseReport(v3);
const ranked3 = R.rank(parsed3, 2);
check("a v3 report reads too", ranked3.builds.length === 2);
check("with its per-metric means", ranked3.best.mean === 200 && ranked3.best.line === 1);
check("and its error bars", ranked3.best.error === 3);

/* --- what a player can do wrong ------------------------------------------ */

check("an HTML report is named as such", threw(() => R.parseReport("<html>"), "not JSON"));
check("a non-report is refused", threw(() => R.parseReport("{}"), "not a SimulationCraft report"));
check(
  "a single-character sim is explained",
  threw(() => R.parseReport(JSON.stringify({ sim: { players: [] } })), "no profilesets"),
);
check(
  "an empty profileset block is explained",
  threw(
    () => R.parseReport(JSON.stringify({ sim: { profilesets: { results: [] } } })),
    "none of them produced a result",
  ),
);

/* --- per-talent impact ---------------------------------------------------- */

/*
  Synthetic, because the arithmetic is the thing being checked and a real set cannot say
  what the answer should be. Four builds, one talent (1) in the two good ones and one
  talent (2) in the two bad ones, one talent (3) in all four.
*/
const synthetic = R.rank(
  {
    metric: "Damage per Second",
    baseline: null,
    player: null,
    version: null,
    results: [
      { name: "ttm_1", mean: 100, error: 1, min: 0, max: 0, iterations: 10 },
      { name: "ttm_2", mean: 120, error: 1, min: 0, max: 0, iterations: 10 },
      { name: "ttm_3", mean: 80, error: 1, min: 0, max: 0, iterations: 10 },
      { name: "ttm_4", mean: 60, error: 1, min: 0, max: 0, iterations: 10 },
    ],
  },
  4,
);
const nodes = { 1: [1, 3], 2: [1, 3], 3: [2, 3], 4: [2, 3] };
const impacts = R.impact(synthetic, (line) => nodes[line]);
const byNode = new Map(impacts.map((i) => [i.nodeId, i]));

check("a talent taken by every build is left out", !byNode.has(3), "nothing to compare it against");
check("the good talent comes first", impacts[0].nodeId === 1);
check(
  "with the mean of the builds taking it",
  byNode.get(1).withIt === 110 && byNode.get(1).without === 70,
  `${byNode.get(1).withIt} vs ${byNode.get(1).without}`,
);
check(
  "and the difference as a fraction",
  Math.abs(byNode.get(1).delta - 110 / 70 + 1) < 1e-9,
  `${(byNode.get(1).delta * 100).toFixed(1)}%`,
);
check("the bad talent is the mirror of it", Math.abs(byNode.get(2).delta + 40 / 110) < 1e-9);
check("and both know how many builds take them", byNode.get(1).taken === 2);

/* --- impact on the real set ----------------------------------------------- */

/*
  The real 67-build set, with the builds it came from, is not in this fixture -- so this
  checks the property that holds whatever the builds are: every talent the set is split on
  gets a number, and the numbers are ordered.
*/
const fakeNodes = (line) => [line % 5, (line % 7) + 10];
const realImpacts = R.impact(ranking, fakeNodes);
check(
  "impacts over the real set are ordered best first",
  realImpacts.every((x, i) => i === 0 || realImpacts[i - 1].delta >= x.delta),
  `${realImpacts.length} talents`,
);

await server.close();
console.log(
  failures.length ? `\n${failures.length} FAILED: ${failures.join(", ")}` : "\nall report tests passed",
);
process.exit(failures.length ? 1 : 0);
