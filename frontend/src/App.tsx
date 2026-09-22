import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  ApiError,
  TERMINAL_STATES,
  cancelJob,
  countBuilds,
  getHealth,
  getJob,
  getTree,
  listTrees,
  submitSolve,
  type CountResult,
  type Health,
  type Job,
  type JobStats,
  type TreeDetail,
  type TreeSummary,
} from "./lib/api";
import { classTintStyle } from "./lib/classes";
import { useConstraints } from "./lib/constraints";
import * as loadout from "./lib/loadout";
import { decode, syncUrl, EMPTY, type ShareState } from "./lib/share";
import { useTheme } from "./lib/theme";
import { CountGate } from "./components/CountGate";
import { JobPanel } from "./components/JobPanel";
import { ResultsBrowser } from "./components/ResultsBrowser";
import { ShareButton } from "./components/ShareButton";
import { SpecRail } from "./components/SpecRail";
import { StatsPanel } from "./components/StatsPanel";
import { TreePane } from "./components/TreePane";

/**
 * One screen: a specialisation's three trees side by side, constraints painted on whichever
 * the solver is pointed at, and the count moving as they land.
 *
 * All three trees are shown because that is the unit a player thinks in -- a build is class
 * plus spec plus hero, and choosing a spec tree talent while the class tree is behind a
 * dropdown is choosing blind. The solver works one tree at a time, which is a property of
 * the engine rather than of the product, so that tree is marked rather than isolated.
 */

const COUNT_DEBOUNCE_MS = 140;
const JOB_POLL_MS = 500;

// Read once, at module scope. The URL is the *initial* state; after that the app owns it and
// writes back, and re-reading would fight its own writes.
const SHARED: ShareState = decode(window.location.search);

export default function App() {
  const { resolved, toggle } = useTheme();
  const [health, setHealth] = useState<Health | null>(null);
  const [trees, setTrees] = useState<TreeSummary[]>([]);
  const [className, setClassName] = useState<string | null>(null);
  const [specName, setSpecName] = useState<string | null>(null);
  const [activeKey, setActiveKey] = useState<string | null>(SHARED.tree);
  const [heroKey, setHeroKey] = useState<string | null>(null);
  const [loaded, setLoaded] = useState<Record<string, TreeDetail>>({});
  const [count, setCount] = useState<CountResult | null>(null);
  const [countError, setCountError] = useState<string | null>(null);
  const [counting, setCounting] = useState(false);
  const [job, setJob] = useState<Job | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [sharedBuild, setSharedBuild] = useState<Record<string, number> | null>(SHARED.build);
  const [stats, setStats] = useState<JobStats | null>(null);
  const [showStats, setShowStats] = useState(true);
  const [pick, setPick] = useState<{ index: number; build: Record<string, number> | null }>({
    index: 0,
    build: null,
  });

  /*
    Two modes, because they are two activities.

    **Build** spends points by hand, on all three trees at once, under the same rules the
    solver counts under -- a loadout spans class, spec and hero, so restricting it to one
    tree would produce something no player could use.

    **Explore** paints constraints on the tree the solver is pointed at, and asks how many
    builds match.

    They share the canvas and cannot share it at the same time: a lit node would mean "I took
    this" in one and "I require this" in the other.
  */
  const [mode, setMode] = useState<"build" | "explore">(SHARED.mode ?? "explore");
  const [spent, setSpent] = useState<Record<string, loadout.Points>>({});
  const [note, setNote] = useState<string | null>(null);

  const active = activeKey ? (loaded[activeKey] ?? null) : null;
  const c = useConstraints(active, SHARED);

  // --- which trees belong to the current spec -----------------------------
  const group = useMemo(() => {
    const mine = trees.filter(
      (t) => t.className === className && (t.kind === "class" || t.specName === specName),
    );
    return {
      class: mine.find((t) => t.kind === "class") ?? null,
      spec: mine.find((t) => t.kind === "spec") ?? null,
      heroes: mine.filter((t) => t.kind === "hero"),
    };
  }, [trees, className, specName]);

  // --- bootstrap ----------------------------------------------------------
  useEffect(() => {
    void (async () => {
      try {
        const [h, list] = await Promise.all([getHealth(), listTrees()]);
        setHealth(h);
        setTrees(list);

        // A tree from the link wins, but only if it still exists: a link can outlive a tree
        // upstream removed, and quietly showing a different spec is worse than falling back.
        const fromLink = list.find((t) => t.key === SHARED.tree);
        const start = fromLink ?? list.find((t) => t.kind === "spec") ?? list[0];
        if (start) {
          setClassName(start.className);
          setSpecName(
            start.specName ??
              list.find((t) => t.className === start.className && t.kind === "spec")?.specName ??
              null,
          );
          setActiveKey(start.key);
        }
      } catch (error) {
        setLoadError(error instanceof Error ? error.message : String(error));
      }
    })();
  }, []);

  // A link carries a loadout per tree kind; map those onto the keys once they are known.
  const seeded = useRef(false);
  useEffect(() => {
    if (seeded.current || !group.spec) return;
    seeded.current = true;
    const next: Record<string, loadout.Points> = {};
    if (group.class && SHARED.spent.class) next[group.class.key] = SHARED.spent.class;
    if (SHARED.spent.spec) next[group.spec.key] = SHARED.spent.spec;
    const heroTarget = SHARED.heroKey ?? group.heroes[0]?.key;
    if (heroTarget && SHARED.spent.hero) next[heroTarget] = SHARED.spent.hero;
    if (Object.keys(next).length) setSpent(next);
  }, [group.class, group.spec, group.heroes]);

  // Pick a hero tree once the spec's heroes are known, unless one is already chosen.
  useEffect(() => {
    if (group.heroes.length === 0) {
      setHeroKey(null);
      return;
    }
    setHeroKey((current) => {
      if (current && group.heroes.some((h) => h.key === current)) return current;
      const fromLink = group.heroes.find((h) => h.key === SHARED.heroKey);
      return (fromLink ?? group.heroes[0]!).key;
    });
  }, [group.heroes]);

  // --- tree definitions, fetched once each and kept -----------------------
  // Three trees are on screen and switching spec brings three more; caching by key means
  // flipping between specs does not refetch what has already been seen.
  const wanted = useMemo(
    () => [group.class?.key, group.spec?.key, heroKey].filter(Boolean) as string[],
    [group.class, group.spec, heroKey],
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

  // --- constraints are per tree, and survive switching between them -------
  const saved = useRef<Map<string, ShareState>>(new Map());
  const previousActive = useRef<string | null>(activeKey);

  const snapshot = useCallback(
    (): ShareState => ({
      tree: previousActive.current,
      points: c.points,
      required: [...c.required],
      excluded: [...c.excluded],
      sides: new Map(c.sides),
      atLeastOne: [...c.atLeastOne],
      exactlyOne: [...c.exactlyOne],
      build: null,
      spent: EMPTY.spent,
      heroKey: null,
      mode: null,
    }),
    [c.points, c.required, c.excluded, c.sides, c.atLeastOne, c.exactlyOne],
  );

  const pointSolverAt = useCallback(
    (key: string) => {
      if (key === activeKey) return;
      // Constraints belong to a tree, so they are put away rather than thrown away: pointing
      // the solver at the class tree and back must not lose what was painted on the spec.
      if (previousActive.current) saved.current.set(previousActive.current, snapshot());
      previousActive.current = key;
      setActiveKey(key);
      setJob(null);
      setStats(null);
      setSharedBuild(null);
      setPick({ index: 0, build: null });
      c.adopt(saved.current.get(key) ?? EMPTY);
    },
    [activeKey, snapshot, c],
  );

  const selectSpec = useCallback(
    (nextClass: string, nextSpec: string | null) => {
      setClassName(nextClass);
      const spec =
        nextSpec ??
        trees.find((t) => t.className === nextClass && t.kind === "spec")?.specName ??
        null;
      setSpecName(spec);
      const target = trees.find(
        (t) => t.className === nextClass && t.kind === "spec" && t.specName === spec,
      );
      if (target) pointSolverAt(target.key);
    },
    [trees, pointSolverAt],
  );

  // --- the gate -----------------------------------------------------------
  // Debounced, and superseded responses are discarded: dragging the point slider fires a
  // request per step, and a slow early one landing last would show a count for a budget the
  // user has already moved past.
  const requestId = useRef(0);
  useEffect(() => {
    if (!active) return;
    const mine = ++requestId.current;
    setCounting(true);
    const timer = setTimeout(() => {
      void (async () => {
        try {
          const result = await countBuilds(active.key, c.payload);
          if (mine === requestId.current) {
            setCount(result);
            setCountError(null);
          }
        } catch (error) {
          if (mine === requestId.current) {
            setCountError(
              error instanceof ApiError ? error.detail : String((error as Error).message),
            );
          }
        } finally {
          if (mine === requestId.current) setCounting(false);
        }
      })();
    }, COUNT_DEBOUNCE_MS);
    return () => clearTimeout(timer);
  }, [active, c.payload]);

  // --- job polling --------------------------------------------------------
  useEffect(() => {
    if (!job || TERMINAL_STATES.has(job.state)) return;
    const timer = setTimeout(() => {
      void getJob(job.id)
        .then(setJob)
        .catch(() => {
          /* transient; the next tick retries */
        });
    }, JOB_POLL_MS);
    return () => clearTimeout(timer);
  }, [job]);

  const onPick = useCallback((index: number, build: Record<string, number> | null) => {
    setPick((previous) =>
      previous.index === index && previous.build === build ? previous : { index, build },
    );
    if (build) setShowStats(false);
  }, []);

  const capOf = useCallback(
    (summary: TreeSummary | null) => summary?.pointCap ?? summary?.maxPointsInTree ?? 0,
    [],
  );

  /** Spend or refund a point. Left adds, right (or shift) removes, as players expect. */
  const onSpend = useCallback(
    (key: string, summary: TreeSummary | null, node: Parameters<typeof loadout.add>[3],
     refund: boolean) => {
      const tree = loaded[key];
      if (!tree || !summary) return;
      const cap = capOf(summary);
      const current = spent[key] ?? {};
      if (refund) {
        const result = loadout.remove(tree, current, cap, node);
        const lost = Object.keys(result.dropped).length;
        setSpent((previous) => ({ ...previous, [key]: result.points }));
        // Refunding can strand everything below; saying so beats a build quietly shrinking.
        setNote(lost ? `Removing ${node.name} also refunded ${lost} talent${lost === 1 ? "" : "s"} below it.` : null);
      } else {
        const result = loadout.add(tree, current, cap, node);
        setSpent((previous) => ({ ...previous, [key]: result.points }));
        setNote(result.reason ?? null);
      }
    },
    [loaded, spent, capOf],
  );

  /** Turn the active tree's hand-built points into constraints, and go exploring. */
  const useAsConstraints = useCallback(() => {
    if (!activeKey || !active) return;
    const points = spent[activeKey] ?? {};
    const ids = Object.keys(points).map(Number);
    if (ids.length === 0) return;
    c.adopt({
      ...EMPTY,
      tree: activeKey,
      points: loadout.total(points),
      required: ids,
    });
    setMode("explore");
  }, [activeKey, active, spent, c]);

  const onShowStats = useCallback((on: boolean) => {
    setShowStats(on);
    if (on) {
      setSharedBuild(null);
      setPick((previous) => ({ index: previous.index, build: null }));
    }
  }, []);

  const onSolve = useCallback(() => {
    if (!active) return;
    setSharedBuild(null);
    setStats(null);
    setPick({ index: 0, build: null });
    void submitSolve(active.key, c.payload)
      .then(setJob)
      .catch((error: unknown) =>
        setCountError(error instanceof ApiError ? error.detail : String(error)),
      );
  }, [active, c.payload]);

  const onCancel = useCallback(() => {
    if (!job) return;
    void cancelJob(job.id)
      .then(setJob)
      .catch(() => {
        /* already finished; the poll will show the real state */
      });
  }, [job]);

  const shownBuild = pick.build ?? sharedBuild;

  // In build mode every pane shows its own hand-spent points; in explore mode only the
  // active pane shows the enumerated build being inspected.
  const paneBuild = useCallback(
    (key: string | undefined) =>
      mode === "build" ? (key ? (spent[key] ?? {}) : {}) : key === activeKey ? shownBuild : null,
    [mode, spent, activeKey, shownBuild],
  );
  const paneReach = useCallback(
    (key: string | undefined, summary: TreeSummary | null) => {
      if (mode !== "build" || !key) return null;
      const tree = loaded[key];
      return tree ? loadout.available(tree, spent[key] ?? {}, capOf(summary)) : null;
    },
    [mode, loaded, spent, capOf],
  );
  const paneBudget = useCallback(
    (key: string | undefined, summary: TreeSummary | null) =>
      mode === "build" && key
        ? { spent: loadout.total(spent[key] ?? {}), cap: capOf(summary) }
        : null,
    [mode, spent, capOf],
  );
  const shares = useMemo(() => {
    if (!stats || !showStats || shownBuild) return null;
    return new Map(stats.talents.map((t) => [t.nodeId, t.share]));
  }, [stats, showStats, shownBuild]);

  // Keep the address bar current so it can be copied at any moment. Replaced rather than
  // pushed: painting constraints is a dozen clicks and nobody calls that navigation.
  useEffect(() => {
    if (!activeKey) return;
    previousActive.current = activeKey;
    syncUrl({
      tree: activeKey,
      points: c.points,
      required: [...c.required],
      excluded: [...c.excluded],
      sides: new Map(c.sides),
      atLeastOne: [...c.atLeastOne],
      exactlyOne: [...c.exactlyOne],
      build: shownBuild,
      spent: {
        class: group.class ? (spent[group.class.key] ?? null) : null,
        spec: group.spec ? (spent[group.spec.key] ?? null) : null,
        hero: heroKey ? (spent[heroKey] ?? null) : null,
      },
      heroKey,
      mode,
    });
  }, [activeKey, mode, group.class, group.spec, heroKey, spent, c.points, c.required, c.excluded, c.sides, c.atLeastOne, c.exactlyOne, shownBuild]);

  if (loadError) {
    return (
      <main className="sky flex min-h-screen items-center justify-center p-6">
        <div className="panel max-w-md p-6">
          <h1 className="display text-2xl">Cannot reach the service</h1>
          <p className="mt-2 text-[13px] text-ink-soft">{loadError}</p>
          <p className="mt-3 text-[12px] text-ink-faint">
            The API and database run in Docker:{" "}
            <code className="num">docker compose up -d api worker</code>.
          </p>
        </div>
      </main>
    );
  }

  const hero = group.heroes.find((h) => h.key === heroKey) ?? null;

  return (
    <div
      className="sky flex min-h-screen flex-col md:h-screen md:min-h-0 md:overflow-hidden"
      style={classTintStyle(className)}
    >
      <header className="flex shrink-0 flex-wrap items-center gap-x-4 gap-y-2 px-4 pt-3 pb-2.5">
        <div className="flex shrink-0 items-baseline gap-2">
          <span
            aria-hidden="true"
            className="text-[12px] leading-none"
            style={{ color: "var(--class-tint)" }}
          >
            ✦
          </span>
          <h1 className="display text-[18px] leading-none">Talent Tree Manager</h1>
        </div>

        {/* A hairline between the wordmark and the rails, so two unrelated things stop
            reading as one run of text. */}
        <span
          aria-hidden="true"
          className="hidden h-7 w-px shrink-0 md:block"
          style={{ background: "color-mix(in srgb, var(--brass) 30%, transparent)" }}
        />

        <SpecRail
          trees={trees}
          className={className}
          specName={specName}
          onSelect={selectSpec}
        />

        <span className="ml-auto flex shrink-0 items-center gap-1">
          {(["build", "explore"] as const).map((m) => (
            <button
              key={m}
              type="button"
              className="btn"
              aria-pressed={mode === m}
              onClick={() => {
                setMode(m);
                setNote(null);
              }}
              title={
                m === "build"
                  ? "Spend points by hand, across all three trees"
                  : "Paint constraints and count matching builds"
              }
            >
              {m === "build" ? "Build" : "Explore"}
            </button>
          ))}
        </span>

        <button type="button" className="btn shrink-0" onClick={toggle}>
          {resolved() === "dark" ? "Plate" : "Void"}
        </button>
      </header>

      <div className="rule mx-4 shrink-0" />

      {/*
        Four columns on desktop: class, specialisation, hero, and the sidebar. Class and
        spec get equal room because they are the same size and carry the same weight; hero
        gets a narrow one because a hero tree is eleven nodes in a tall diamond, which is
        exactly the shape a narrow column wants. Stacking hero under the class tree was the
        first arrangement and it squeezed both.
      */}
      <div className="flex min-h-0 flex-1 flex-col gap-2 p-2 md:flex-row md:gap-2.5 md:p-2.5">
        <TreePane
          tree={group.class ? (loaded[group.class.key] ?? null) : null}
          title="Class"
          subtitle={className}
          active={activeKey === group.class?.key}
          onActivate={() => group.class && pointSolverAt(group.class.key)}
          states={c.states}
          sides={c.sides}
          stale={counting}
          shares={shares}
          onNode={
            mode === "build"
              ? (node, refund) =>
                  group.class && onSpend(group.class.key, group.class, node, refund)
              : activeKey === group.class?.key
                ? c.activate
                : undefined
          }
          build={paneBuild(group.class?.key)}
          reachable={paneReach(group.class?.key, group.class)}
          budget={paneBudget(group.class?.key, group.class)}
          editing={mode === "build"}
          className="min-h-[20rem] flex-1 md:min-h-0"
        />

        <TreePane
          tree={group.spec ? (loaded[group.spec.key] ?? null) : null}
          title="Specialisation"
          subtitle={specName}
          active={activeKey === group.spec?.key}
          onActivate={() => group.spec && pointSolverAt(group.spec.key)}
          states={c.states}
          sides={c.sides}
          stale={counting}
          shares={shares}
          onNode={
            mode === "build"
              ? (node, refund) =>
                  group.spec && onSpend(group.spec.key, group.spec, node, refund)
              : activeKey === group.spec?.key
                ? c.activate
                : undefined
          }
          build={paneBuild(group.spec?.key)}
          reachable={paneReach(group.spec?.key, group.spec)}
          budget={paneBudget(group.spec?.key, group.spec)}
          editing={mode === "build"}
          className="min-h-[20rem] flex-1 md:min-h-0"
        />

        <TreePane
          tree={hero ? (loaded[hero.key] ?? null) : null}
          title="Hero"
          subtitle={null}
          active={activeKey === hero?.key}
          onActivate={() => hero && pointSolverAt(hero.key)}
          states={c.states}
          sides={c.sides}
          stale={counting}
          shares={shares}
          onNode={
            mode === "build"
              ? (node, refund) => hero && onSpend(hero.key, hero, node, refund)
              : activeKey === hero?.key
                ? c.activate
                : undefined
          }
          build={paneBuild(hero?.key)}
          reachable={paneReach(hero?.key, hero)}
          budget={paneBudget(hero?.key, hero)}
          editing={mode === "build"}
          className="min-h-[15rem] md:min-h-0 md:w-[16rem] md:shrink-0"
        >
          {/* Two hero trees per spec, so they are a choice rather than a fixed pane. */}
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
        </TreePane>

        <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[19rem] md:gap-2.5 md:overflow-y-auto md:pr-1">
          {mode === "build" ? (
            <section className="panel p-3.5">
              <span className="label">Loadout</span>
              <ul className="mt-2 space-y-1">
                {([
                  ["Class", group.class],
                  ["Specialisation", group.spec],
                  ["Hero", hero],
                ] as const).map(([label, summary]) => (
                  <li key={label} className="flex items-baseline justify-between text-[12px]">
                    <span className="text-ink-soft">{label}</span>
                    <span className="num text-ink">
                      {summary ? loadout.total(spent[summary.key] ?? {}) : 0}
                      <span className="text-ink-faint">/{capOf(summary ?? null)}</span>
                    </span>
                  </li>
                ))}
              </ul>
              <p className="mt-2 text-[11.5px] leading-snug text-ink-soft">
                Click to spend a point, right-click to refund. Every tree is editable — a
                loadout is all three.
              </p>
              {note && (
                <p className="mt-2 text-[11.5px] leading-snug" style={{ color: "var(--brass-bright)" }}>
                  {note}
                </p>
              )}
              <div className="mt-3 flex gap-1.5">
                {/* Acts on the tree the solver is pointed at, and says which -- otherwise a
                    disabled button is a puzzle: the points might be in a different pane. */}
                <button
                  type="button"
                  className="btn flex-1"
                  onClick={useAsConstraints}
                  disabled={!activeKey || loadout.total(spent[activeKey] ?? {}) === 0}
                  title={
                    activeKey && loadout.total(spent[activeKey] ?? {}) > 0
                      ? "Require everything you have taken here, then explore around it"
                      : `Spend points in the ${paneName(activeKey, group, hero)} tree first, or point the solver at another one`
                  }
                >
                  Explore from {paneName(activeKey, group, hero)}
                </button>
                <button
                  type="button"
                  className="btn"
                  onClick={() => {
                    setSpent({});
                    setNote(null);
                  }}
                  disabled={Object.values(spent).every((p) => loadout.total(p) === 0)}
                >
                  Clear
                </button>
              </div>
            </section>
          ) : (
          <section className="panel p-3.5">
            <label className="flex items-baseline justify-between" htmlFor="points">
              <span className="label">Point budget</span>
              <span className="num text-[15px] text-ink">
                {c.points}
                <span className="text-ink-faint">/{c.cap}</span>
              </span>
            </label>
            <input
              id="points"
              type="range"
              min={1}
              max={c.cap}
              value={c.points}
              onChange={(event) => c.setPoints(Number(event.target.value))}
              className="mt-2 w-full"
            />
            <p className="mt-1 text-[11px] text-ink-faint">
              {active ? `Solving the ${paneName(activeKey, group, hero)} tree.` : " "}
            </p>
          </section>
          )}

          {mode === "explore" && (
          <section className="panel p-3.5">
            <div className="flex items-baseline justify-between gap-2">
              <span className="label">Constraints</span>
              <button
                type="button"
                className="btn !px-2 !py-0.5 !text-[11px]"
                onClick={() => {
                  c.reset();
                  setSharedBuild(null);
                }}
                disabled={c.count === 0 && !sharedBuild}
              >
                Clear
              </button>
            </div>
            <div className="mt-2 flex flex-wrap gap-1">
              {(
                [
                  ["none", "Require / bar"],
                  ["atLeastOne", "At least one"],
                  ["exactlyOne", "Exactly one"],
                ] as const
              ).map(([mode, label]) => (
                <button
                  key={mode}
                  type="button"
                  className="btn !px-2 !py-0.5 !text-[11px]"
                  onClick={() => c.setGroupMode(mode)}
                  aria-pressed={c.groupMode === mode}
                >
                  {label}
                </button>
              ))}
            </div>
            <p className="mt-2 text-[11.5px] leading-snug text-ink-soft">
              {c.groupMode === "none"
                ? "Click a talent to require it, again to bar it. Right-click reverses. Choice nodes cycle side."
                : "Click talents to add them to the group."}
            </p>
          </section>
          )}

          {mode === "explore" && (
            <CountGate
            result={count}
            error={countError}
            stale={counting}
            pending={c.pending}
            onSolve={onSolve}
              solveDisabled={
                !count || !count.listable || count.sets === 0 || c.pending.length > 0
              }
            />
          )}

          {job && <JobPanel job={job} onCancel={onCancel} onDismiss={() => setJob(null)} />}

          {job && active && (job.state === "done" || job.state === "capped") && (
            <StatsPanel
              job={job}
              tree={active}
              showing={showStats}
              onToggle={onShowStats}
              onStats={setStats}
            />
          )}

          {job && (job.state === "done" || job.state === "capped") && (
            <ResultsBrowser job={job} index={pick.index} onSelect={onPick} />
          )}

          <section className="panel p-3.5">
            <span className="label">Share</span>
            <p className="mt-1 mb-2 text-[11.5px] leading-snug text-ink-soft">
              {shownBuild
                ? "This link opens on the build you are looking at."
                : "This link opens on these trees with these constraints."}
            </p>
            <ShareButton />
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
    </div>
  );
}

function paneName(
  activeKey: string | null,
  group: { class: TreeSummary | null; spec: TreeSummary | null },
  hero: TreeSummary | null,
): string {
  if (activeKey && activeKey === group.class?.key) return "class";
  if (activeKey && activeKey === hero?.key) return "hero";
  return "specialisation";
}
