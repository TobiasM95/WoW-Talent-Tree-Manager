import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  getHealth,
  getTree,
  listTrees,
  type Health,
  type TalentNode,
  type TreeDetail,
  type TreeSummary,
} from "./lib/api";
import { classTintStyle } from "./lib/classes";
import type { TreeInput } from "./lib/enumerate";
import * as loadout from "./lib/loadout";
import { decode, syncUrl, type Role } from "./lib/share";
import type { Ranking } from "./lib/simcReport";
import { productOf, type Character } from "./lib/space";
import { useCounts } from "./lib/useCounts";
import { useTheme } from "./lib/theme";
import {
  budgetOf,
  capOf,
  constraintCount,
  drawnBuild,
  emptyWork,
  EMPTY_SEARCH,
  fixedFrom,
  paint,
  payloadOf,
  pendingOf,
  sidesOf,
  spend,
  statesOf,
  type Tool,
  type TreeMode,
  type TreeWork,
} from "./lib/workspace";
import { AnalysisView } from "./components/AnalysisView";
import { ShapeKey } from "./components/Legend";
import { LoadoutString } from "./components/LoadoutString";
import { PaintTools } from "./components/PaintTools";
import { ShareButton } from "./components/ShareButton";
import { SimulateView, type Sim } from "./components/SimulateView";
import { SpaceCard } from "./components/SpaceCard";
import { SpecRail } from "./components/SpecRail";
import { TreePane } from "./components/TreePane";

/**
 * Three steps, in the order a player does them:
 *
 *   **Narrow** -- each tree fixed or open, painted until the product of the three is small
 *   enough to sim;
 *   **Simulate** -- download the builds, run SimulationCraft, drop its report back;
 *   **Analyse** -- the ranking, what each talent was worth, and each choice node's sides.
 *
 * The earlier layout was organised around the engine rather than the errand: one tree searched
 * at a time, two global modes with separate state, and export, import and analysis stacked as
 * sidebar panels under the canvas. Each step here owns the whole screen while it is the one
 * being done.
 */

type Step = "narrow" | "simulate" | "analyse";

const DEFAULT_LIMIT = 10000;

// The URL is the *initial* state; after that the app owns it and writes back.
const SHARED = decode(window.location.search);

const LABEL: Record<Role, string> = { class: "Class", spec: "Spec", hero: "Hero" };

export default function App() {
  const { resolved, toggle } = useTheme();
  const [health, setHealth] = useState<Health | null>(null);
  const [trees, setTrees] = useState<TreeSummary[]>([]);
  const [className, setClassName] = useState<string | null>(null);
  const [specName, setSpecName] = useState<string | null>(null);
  const [heroKey, setHeroKey] = useState<string | null>(SHARED.hero);
  const [loaded, setLoaded] = useState<Record<string, TreeDetail>>({});
  const [loadError, setLoadError] = useState<string | null>(null);

  const [work, setWork] = useState<Record<string, TreeWork>>({});
  const [tool, setTool] = useState<Tool>("toggle");
  const [limit, setLimit] = useState(SHARED.limit ?? DEFAULT_LIMIT);
  const [note, setNote] = useState<string | null>(null);

  const [step, setStep] = useState<Step>("narrow");
  const [sim, setSim] = useState<Sim | null>(null);
  const [analysis, setAnalysis] = useState<{ sim: Sim; ranking: Ranking } | null>(null);

  // --- which trees belong to the current spec -----------------------------
  /*
    A class tree belongs to a *specialisation*, not just a class: which talents are granted
    for free differs between them, so matching on class alone showed the wrong granted
    talents and produced strings the game would not accept.
  */
  const group = useMemo(() => {
    const mine = trees.filter((t) => t.className === className && t.specName === specName);
    return {
      class: mine.find((t) => t.kind === "class") ?? null,
      spec: mine.find((t) => t.kind === "spec") ?? null,
      heroes: mine.filter((t) => t.kind === "hero"),
    };
  }, [trees, className, specName]);
  const hero = group.heroes.find((h) => h.key === heroKey) ?? null;

  const roles = useMemo(
    () =>
      ([
        ["class", group.class],
        ["spec", group.spec],
        ["hero", hero],
      ] as const).filter((r): r is readonly [Role, TreeSummary] => r[1] !== null),
    [group.class, group.spec, hero],
  );

  // --- bootstrap ----------------------------------------------------------
  useEffect(() => {
    void (async () => {
      try {
        const [h, list] = await Promise.all([getHealth(), listTrees()]);
        setHealth(h);
        setTrees(list);
        const start =
          list.find((t) => t.key === SHARED.spec) ?? list.find((t) => t.kind === "spec") ?? list[0];
        if (start) {
          setClassName(start.className);
          setSpecName(start.specName);
        }
      } catch (error) {
        setLoadError(error instanceof Error ? error.message : String(error));
      }
    })();
  }, []);

  useEffect(() => {
    if (group.heroes.length === 0) return;
    setHeroKey((current) =>
      current && group.heroes.some((h) => h.key === current) ? current : group.heroes[0]!.key,
    );
  }, [group.heroes]);

  // A link's trees are by role; map them onto keys once the keys are known.
  const seeded = useRef(false);
  useEffect(() => {
    if (seeded.current || !group.spec || !group.class || (group.heroes.length && !hero)) return;
    seeded.current = true;
    const next: Record<string, TreeWork> = {};
    for (const [role, summary] of roles) {
      const shared = SHARED.work[role];
      if (shared) next[summary.key] = shared;
    }
    if (Object.keys(next).length) setWork((previous) => ({ ...next, ...previous }));
  }, [group.spec, group.class, group.heroes, hero, roles]);

  // Every tree of the spec, both hero trees included: a talent string writes granted talents
  // whether or not their tree is the one chosen.
  const wanted = useMemo(
    () =>
      [group.class?.key, group.spec?.key, ...group.heroes.map((h) => h.key)].filter(Boolean) as string[],
    [group.class, group.spec, group.heroes],
  );

  useEffect(() => {
    let cancelled = false;
    for (const key of wanted) {
      if (loaded[key]) continue;
      void getTree(key)
        .then((detail) => {
          if (!cancelled) setLoaded((previous) => ({ ...previous, [key]: detail }));
        })
        .catch((error: unknown) => {
          if (!cancelled) setLoadError(error instanceof Error ? error.message : String(error));
        });
    }
    return () => {
      cancelled = true;
    };
  }, [wanted, loaded]);

  const workOf = useCallback((key: string) => work[key] ?? emptyWork(), [work]);
  const update = useCallback(
    (key: string, change: (w: TreeWork) => TreeWork) =>
      setWork((previous) => ({ ...previous, [key]: change(previous[key] ?? emptyWork()) })),
    [],
  );

  const selectSpec = useCallback((nextClass: string, nextSpec: string | null) => {
    setClassName(nextClass);
    setSpecName(nextSpec);
    setSim(null);
    setAnalysis(null);
    setStep("narrow");
    setNote(null);
  }, []);

  // --- per-tree counts ----------------------------------------------------
  const countRequests = useMemo(
    () =>
      roles.map(([, summary]) => {
        const w = workOf(summary.key);
        return {
          key: summary.key,
          payload:
            w.mode === "open" && pendingOf(w).length === 0 ? payloadOf(w, capOf(summary)) : null,
        };
      }),
    [roles, workOf],
  );
  const counts = useCounts(countRequests);

  const rows = roles.map(([role, summary]) => {
    const w = workOf(summary.key);
    const c = counts[summary.key];
    return {
      key: summary.key,
      label: LABEL[role],
      fixed: w.mode === "fixed",
      builds: w.mode === "fixed" ? 1 : (c?.builds ?? null),
      stale: w.mode === "open" && (c?.stale ?? true),
      error: w.mode === "open" ? (c?.error ?? null) : null,
    };
  });
  const pending = roles.flatMap(([role, s]) =>
    pendingOf(workOf(s.key)).map((p) => `${p} in the ${LABEL[role].toLowerCase()} tree`),
  );
  const total =
    rows.length === 3 && rows.every((r) => r.builds !== null && !r.error)
      ? productOf(rows.map((r) => r.builds!))
      : null;
  const simmable =
    total !== null && total > 0 && total <= limit && pending.length === 0 && !rows.some((r) => r.stale);

  // --- clicking a talent --------------------------------------------------
  const onNode = useCallback(
    (summary: TreeSummary, node: TalentNode, alternate: boolean) => {
      const tree = loaded[summary.key];
      if (!tree) return;
      const w = workOf(summary.key);
      if (w.mode === "fixed") {
        const result = spend(w, tree, capOf(summary), node, alternate);
        update(summary.key, () => result.work);
        setNote(result.note);
      } else {
        update(summary.key, (current) => paint(current, node, tool, alternate));
        setNote(null);
      }
    },
    [loaded, workOf, update, tool],
  );

  const onMode = useCallback(
    (key: string, mode: TreeMode) => {
      update(key, (w) => ({ ...w, mode }));
      setNote(null);
    },
    [update],
  );

  const clearSearches = useCallback(() => {
    setWork((previous) => {
      const next: Record<string, TreeWork> = {};
      for (const [key, w] of Object.entries(previous)) {
        next[key] = w.mode === "open" ? { ...w, search: { ...EMPTY_SEARCH, budget: w.search.budget } } : w;
      }
      return next;
    });
  }, []);

  /** A talent string fixes all three trees to what it says. */
  const onImportString = useCallback(
    (points: loadout.Points, choices: Record<string, number>, heroSubTreeId: number | null) => {
      const target = group.heroes.find((h) => h.subTreeId === heroSubTreeId) ?? hero;
      if (target) setHeroKey(target.key);
      setWork((previous) => {
        const next = { ...previous };
        for (const summary of [group.class, group.spec, target]) {
          if (!summary) continue;
          const tree = loaded[summary.key];
          if (!tree) continue;
          const mine: loadout.Points = {};
          for (const n of tree.nodes) if (points[String(n.nodeId)]) mine[String(n.nodeId)] = points[String(n.nodeId)]!;
          next[summary.key] = fixedFrom(previous[summary.key] ?? emptyWork(), tree, capOf(summary), mine, choices);
        }
        return next;
      });
      setNote("Imported: all three trees are fixed to that build. Switch one to Open to explore around it.");
    },
    [group.class, group.spec, group.heroes, hero, loaded],
  );

  /** "Use this build" from the analysis: fix every tree to it, back to narrowing. */
  const useCharacter = useCallback(
    (character: Character) => {
      setWork((previous) => {
        const next = { ...previous };
        for (const [key, part] of Object.entries(character.parts)) {
          const tree = loaded[key];
          const summary = trees.find((t) => t.key === key) ?? null;
          if (!tree || !summary) continue;
          next[key] = fixedFrom(previous[key] ?? emptyWork(), tree, capOf(summary), part.points, part.choices);
        }
        return next;
      });
      const heroPart = Object.keys(character.parts).find((k) => group.heroes.some((h) => h.key === k));
      if (heroPart) setHeroKey(heroPart);
      setNote(`Build ${character.line} is now fixed on all three trees.`);
      setStep("narrow");
    },
    [loaded, trees, group.heroes],
  );

  // --- the simulation step ------------------------------------------------
  const inputs: TreeInput[] = roles
    .filter(([, s]) => loaded[s.key])
    .map(([, s]) => ({ key: s.key, tree: loaded[s.key]!, work: workOf(s.key), cap: capOf(s) }));
  const signature = JSON.stringify({
    limit,
    hero: hero?.subTreeId ?? null,
    trees: inputs.map((i) =>
      i.work.mode === "fixed"
        ? { key: i.key, points: i.work.points, picks: i.work.picks }
        : { key: i.key, search: payloadOf(i.work, i.cap), sides: i.work.search.sides },
    ),
  });
  const labels = Object.fromEntries(roles.map(([role, s]) => [s.key, LABEL[role]]));
  const allTrees = wanted.map((k) => loaded[k]).filter(Boolean) as TreeDetail[];
  const specTree = group.spec ? (loaded[group.spec.key] ?? null) : null;

  // --- the link -----------------------------------------------------------
  useEffect(() => {
    if (!group.spec || !seeded.current) return;
    const out: Partial<Record<Role, TreeWork>> = {};
    for (const [role, s] of roles) if (work[s.key]) out[role] = work[s.key];
    syncUrl({ spec: group.spec.key, hero: heroKey, work: out, limit: limit === DEFAULT_LIMIT ? null : limit });
  }, [group.spec, heroKey, roles, work, limit]);

  const fixedPoints = Object.assign(
    {},
    ...roles.filter(([, s]) => workOf(s.key).mode === "fixed").map(([, s]) => workOf(s.key).points),
  ) as loadout.Points;
  const fixedPicks = Object.assign(
    {},
    ...roles.filter(([, s]) => workOf(s.key).mode === "fixed").map(([, s]) => workOf(s.key).picks),
  ) as Record<string, number>;
  const allFixed = roles.length === 3 && roles.every(([, s]) => workOf(s.key).mode === "fixed");
  const anyOpen = roles.some(([, s]) => workOf(s.key).mode === "open");
  const paintCounts = roles.reduce(
    (acc, [, s]) => {
      const w = workOf(s.key);
      if (w.mode !== "open") return acc;
      acc.required += w.search.required.length + Object.keys(w.search.sides).length;
      acc.excluded += w.search.excluded.length;
      acc.anyOf += w.search.atLeastOne.length;
      acc.oneOf += w.search.exactlyOne.length;
      return acc;
    },
    { required: 0, excluded: 0, anyOf: 0, oneOf: 0 },
  );
  const anyConstraint = roles.some(([, s]) => workOf(s.key).mode === "open" && constraintCount(workOf(s.key)) > 0);

  if (loadError) {
    return (
      <main className="sky flex min-h-screen items-center justify-center p-6">
        <div className="panel max-w-md p-6">
          <h1 className="display text-2xl">Cannot reach the service</h1>
          <p className="mt-2 text-[13px] text-ink-soft">{loadError}</p>
          <p className="mt-3 text-[12px] text-ink-faint">
            It runs in Docker: <code className="num">docker compose --profile web up -d</code>.
          </p>
        </div>
      </main>
    );
  }

  const pane = (role: Role, summary: TreeSummary, extra?: { className: string; children?: React.ReactNode }) => {
    const tree = loaded[summary.key] ?? null;
    const w = workOf(summary.key);
    const cap = capOf(summary);
    const c = counts[summary.key];
    return (
      <TreePane
        key={summary.key}
        tree={tree}
        title={LABEL[role]}
        subtitle={role === "class" ? className : role === "spec" ? specName : null}
        mode={w.mode}
        onMode={(m) => onMode(summary.key, m)}
        points={{ value: w.mode === "fixed" ? loadout.total(w.points) : budgetOf(w, cap), cap }}
        onBudget={(p) =>
          update(summary.key, (current) => ({
            ...current,
            search: { ...current.search, budget: p >= cap ? null : p },
          }))
        }
        count={
          w.mode === "fixed"
            ? { builds: 1, stale: false, error: null }
            : { builds: c?.builds ?? null, stale: c?.stale ?? true, error: c?.error ?? null }
        }
        states={statesOf(w)}
        sides={sidesOf(w)}
        build={w.mode === "fixed" ? drawnBuild(w, tree) : null}
        reachable={w.mode === "fixed" && tree ? loadout.available(tree, w.points, cap) : null}
        editing={w.mode === "fixed"}
        onNode={(node, alternate) => onNode(summary, node, alternate)}
        className={extra?.className}
      >
        {extra?.children}
      </TreePane>
    );
  };

  return (
    <div
      className="sky flex min-h-screen flex-col md:h-screen md:min-h-0 md:overflow-hidden"
      style={classTintStyle(className)}
    >
      <header className="flex shrink-0 flex-wrap items-center gap-x-4 gap-y-2 px-4 pt-3 pb-2.5">
        <div className="flex shrink-0 items-baseline gap-2">
          <span aria-hidden="true" className="text-[12px] leading-none" style={{ color: "var(--class-tint)" }}>
            ✦
          </span>
          <h1 className="display text-[18px] leading-none">Talent Tree Manager</h1>
        </div>
        <span
          aria-hidden="true"
          className="hidden h-7 w-px shrink-0 md:block"
          style={{ background: "color-mix(in srgb, var(--brass) 30%, transparent)" }}
        />
        <SpecRail trees={trees} className={className} specName={specName} onSelect={selectSpec} />

        {/* The workflow, in order. A step is reachable once the one before it has produced
            what it needs: something simmable, then a report. */}
        <nav className="steps ml-auto" aria-label="Workflow">
          {([
            ["narrow", "Narrow", true],
            ["simulate", "Simulate", simmable || sim !== null],
            ["analyse", "Analyse", analysis !== null],
          ] as const).map(([id, label, enabled], i) => (
            <span key={id} className="contents">
              {i > 0 && <span className="sep" aria-hidden="true" />}
              <button
                type="button"
                aria-current={step === id ? "step" : undefined}
                disabled={!enabled}
                onClick={() => setStep(id)}
                title={
                  enabled
                    ? undefined
                    : id === "simulate"
                      ? "Narrow the trees until the builds fit the sim limit"
                      : "Bring a SimulationCraft report back first"
                }
              >
                <span className="n">{i + 1}</span>
                {label}
              </button>
            </span>
          ))}
        </nav>

        <button type="button" className="btn shrink-0" onClick={toggle}>
          {resolved() === "dark" ? "Plate" : "Void"}
        </button>
      </header>

      <div className="rule mx-4 shrink-0" />

      {step === "narrow" && (
        <div className="flex min-h-0 flex-1 flex-col gap-2 p-2 md:flex-row md:gap-2.5 md:p-2.5">
          {group.class && pane("class", group.class, { className: "min-h-[22rem] flex-1 md:min-h-0" })}
          {group.spec && pane("spec", group.spec, { className: "min-h-[22rem] flex-1 md:min-h-0" })}
          {hero &&
            pane("hero", hero, {
              className: "min-h-[18rem] md:min-h-0 md:w-[20rem] md:shrink-0",
              children: (
                <span className="ml-auto flex shrink-0 items-center">
                  {group.heroes.map((h) => (
                    <button
                      key={h.key}
                      type="button"
                      className="rail-item !px-1 !text-[10.5px]"
                      aria-pressed={h.key === heroKey}
                      onClick={() => setHeroKey(h.key)}
                      title={h.name}
                    >
                      {h.name}
                    </button>
                  ))}
                </span>
              ),
            })}

          <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[19rem] md:gap-2.5 md:overflow-y-auto md:pr-1">
            <SpaceCard
              rows={rows}
              limit={limit}
              onLimit={setLimit}
              pending={pending}
              onSimulate={() => setStep("simulate")}
            />

            {note && (
              <p className="panel px-3.5 py-2 text-[11.5px] leading-snug" style={{ color: "var(--brass-bright)" }}>
                {note}
              </p>
            )}

            <PaintTools
              tool={tool}
              onTool={setTool}
              anyOpen={anyOpen}
              onClear={clearSearches}
              clearable={anyConstraint}
              counts={paintCounts}
            />

            <LoadoutString
              spec={specTree}
              trees={allTrees}
              points={fixedPoints}
              choices={fixedPicks}
              heroSubTreeId={hero?.subTreeId ?? null}
              onImport={onImportString}
            />
            {!allFixed && (
              <p className="-mt-1 px-1 text-[10.5px] leading-snug text-ink-faint">
                Importing fixes all three trees. The string above holds only the fixed ones.
              </p>
            )}

            <section className="panel p-3.5">
              <span className="label">Share</span>
              <p className="mt-1 mb-2 text-[11.5px] leading-snug text-ink-soft">
                The link opens on these three trees exactly as they are — fixed builds and
                searches both.
              </p>
              <ShareButton />
            </section>

            <section className="panel px-3.5 py-2.5">
              <ShapeKey />
            </section>

            <footer className="px-1 pb-1 text-[10.5px] leading-relaxed text-ink-faint">
              A fan project. Not affiliated with or endorsed by Blizzard Entertainment.
              {health && (
                <>
                  {" "}
                  Data revision <span className="num">{health.revision}</span>.
                </>
              )}
            </footer>
          </aside>
        </div>
      )}

      {step === "simulate" && specTree && className && specName && (
        <div className="flex min-h-0 flex-1 overflow-y-auto">
          <SimulateView
            signature={signature}
            inputs={inputs}
            labels={labels}
            limit={limit}
            spec={specTree}
            trees={allTrees}
            heroSubTreeId={hero?.subTreeId ?? null}
            className={className}
            specName={specName}
            sim={sim}
            onSim={setSim}
            onRanking={(ranking) => {
              if (!sim) return;
              setAnalysis({ sim, ranking });
              setStep("analyse");
            }}
            onBack={() => setStep("narrow")}
          />
        </div>
      )}

      {step === "analyse" && analysis && (
        <AnalysisView
          sim={analysis.sim}
          ranking={analysis.ranking}
          trees={Object.keys(analysis.sim.characters[0]?.parts ?? {})
            .filter((k) => loaded[k])
            .map((k) => {
              const role = roles.find(([, s]) => s.key === k)?.[0];
              const kind = loaded[k]!.kind as Role;
              return { key: k, label: LABEL[role ?? kind], tree: loaded[k]! };
            })}
          onUse={useCharacter}
          onBack={() => setStep("narrow")}
          onAnother={() => setStep("simulate")}
        />
      )}
    </div>
  );
}
