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
  type TreeDetail,
  type TreeSummary,
} from "./lib/api";
import { classTintStyle } from "./lib/classes";
import { useConstraints } from "./lib/constraints";
import { useTheme } from "./lib/theme";
import { CountGate } from "./components/CountGate";
import { JobPanel } from "./components/JobPanel";
import { ResultsBrowser } from "./components/ResultsBrowser";
import { TreeCanvas } from "./components/TreeCanvas";

/**
 * The app is one screen: pick a tree, paint constraints on it, watch the count, enumerate.
 *
 * That shape follows from the product model. The count is free and answered inline, so it
 * belongs next to the canvas being painted rather than behind a "calculate" step -- the
 * number moving as constraints land is the whole feedback loop.
 */

const COUNT_DEBOUNCE_MS = 140;
const JOB_POLL_MS = 500;

export default function App() {
  const { resolved, toggle } = useTheme();
  const [health, setHealth] = useState<Health | null>(null);
  const [trees, setTrees] = useState<TreeSummary[]>([]);
  const [treeKey, setTreeKey] = useState<string | null>(null);
  const [tree, setTree] = useState<TreeDetail | null>(null);
  const [count, setCount] = useState<CountResult | null>(null);
  const [countError, setCountError] = useState<string | null>(null);
  const [counting, setCounting] = useState(false);
  const [job, setJob] = useState<Job | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  // Which enumerated build is being inspected, and its contents once its page has loaded.
  // Selection is by absolute index so paging stays inside the browser component.
  const [pick, setPick] = useState<{ index: number; build: Record<string, number> | null }>({
    index: 0,
    build: null,
  });

  const c = useConstraints(tree);

  // --- bootstrap ----------------------------------------------------------
  useEffect(() => {
    void (async () => {
      try {
        const [h, list] = await Promise.all([getHealth(), listTrees()]);
        setHealth(h);
        setTrees(list);
        const first = list.find((t) => t.kind === "spec") ?? list[0];
        if (first) setTreeKey(first.key);
      } catch (error) {
        setLoadError(error instanceof Error ? error.message : String(error));
      }
    })();
  }, []);

  useEffect(() => {
    if (!treeKey) return;
    let cancelled = false;
    void (async () => {
      try {
        const detail = await getTree(treeKey);
        if (!cancelled) {
          setTree(detail);
          c.reset();
        }
      } catch (error) {
        if (!cancelled) setLoadError(error instanceof Error ? error.message : String(error));
      }
    })();
    return () => {
      cancelled = true;
    };
    // c.reset is stable; re-running on it would refetch the tree on every constraint change.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [treeKey]);

  // --- the gate -----------------------------------------------------------
  // Debounced, and the result of a superseded request is discarded: dragging the point
  // slider fires a request per step, and without the guard a slow early one can land last
  // and show a count for a budget the user has already moved past.
  const requestId = useRef(0);
  useEffect(() => {
    if (!tree) return;
    const mine = ++requestId.current;
    setCounting(true);
    const timer = setTimeout(() => {
      void (async () => {
        try {
          const result = await countBuilds(tree.key, c.payload);
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
  }, [tree, c.payload]);

  // --- job polling --------------------------------------------------------
  useEffect(() => {
    if (!job || TERMINAL_STATES.has(job.state)) return;
    const timer = setTimeout(() => {
      void getJob(job.id)
        .then(setJob)
        .catch(() => {
          /* a transient failure should not kill the panel; the next tick retries */
        });
    }, JOB_POLL_MS);
    return () => clearTimeout(timer);
  }, [job]);

  const onPick = useCallback(
    (index: number, build: Record<string, number> | null) =>
      setPick((previous) =>
        previous.index === index && previous.build === build ? previous : { index, build },
      ),
    [],
  );

  const onSolve = useCallback(() => {
    if (!tree) return;
    setPick({ index: 0, build: null });
    void submitSolve(tree.key, c.payload)
      .then(setJob)
      .catch((error: unknown) =>
        setCountError(error instanceof ApiError ? error.detail : String(error)),
      );
  }, [tree, c.payload]);

  const onCancel = useCallback(() => {
    if (!job) return;
    void cancelJob(job.id)
      .then(setJob)
      .catch(() => {
        /* already finished; the poll will show the real state */
      });
  }, [job]);

  const grouped = useMemo(() => groupByClass(trees), [trees]);
  const current = trees.find((t) => t.key === treeKey) ?? null;

  if (loadError) {
    return (
      <main className="grain flex min-h-screen items-center justify-center p-6">
        <div className="panel framed max-w-md p-6">
          <h1 className="text-lg">Cannot reach the service</h1>
          <p className="mt-2 text-[13px] text-ink-soft">{loadError}</p>
          <p className="mt-3 text-[12px] text-ink-faint">
            The API and database run in Docker: <code>docker compose up -d api worker</code>.
          </p>
        </div>
      </main>
    );
  }

  return (
    <div
      /*
        Desktop is a fixed-height app shell: the viewport is the frame, the canvas fills
        what is left, and the sidebar scrolls inside it. `min-h-screen` alone lets the row
        grow past the viewport, which pushes the results panel off the bottom of the screen
        exactly when it starts mattering.

        Phone keeps `min-h-screen` and scrolls the page, because stacking a canvas and four
        panels into one screen height leaves nothing usable.
      */
      className="grain flex min-h-screen flex-col md:h-screen md:min-h-0 md:overflow-hidden"
      style={classTintStyle(current?.className)}
    >
      <header className="panel framed relative z-10 m-2 flex flex-wrap items-center gap-3 px-4 py-2 md:m-3">
        <h1
          className="text-[15px] md:text-[17px]"
          style={{ color: "var(--class-tint)" }}
        >
          Talent Tree Manager
        </h1>

        <label className="ml-auto flex items-center gap-2 text-[12px] text-ink-faint">
          <span className="sr-only">Tree</span>
          <select
            className="btn max-w-[15rem] truncate py-1 text-[13px] md:max-w-none"
            value={treeKey ?? ""}
            onChange={(event) => setTreeKey(event.target.value)}
          >
            {grouped.map(([className, group]) => (
              <optgroup key={className} label={className}>
                {group.map((t) => (
                  <option key={t.key} value={t.key}>
                    {t.specName ? `${t.specName} — ` : ""}
                    {labelFor(t)}
                  </option>
                ))}
              </optgroup>
            ))}
          </select>
        </label>

        <button type="button" className="btn py-1 text-[12px]" onClick={toggle}>
          {resolved() === "dark" ? "Light" : "Dark"}
        </button>
      </header>

      <div className="flex min-h-0 flex-1 flex-col gap-2 px-2 pb-2 md:flex-row md:gap-3 md:px-3 md:pb-3">
        {/* The canvas is the working surface, so it gets the room. */}
        <div className="panel relative min-h-[22rem] flex-1 overflow-hidden md:min-h-0">
          {tree ? (
            <TreeCanvas
              tree={tree}
              states={c.states}
              sides={c.sides}
              stale={counting}
              build={pick.build}
              onActivate={c.activate}
            />
          ) : (
            <div className="absolute inset-0 flex items-center justify-center text-[13px] text-ink-faint">
              Loading tree…
            </div>
          )}

          {/* Sits over the canvas, so it carries its own scrim -- on a phone the tree fills
              the panel and unbacked text lands on top of talent icons. */}
          <p className="canvas-hint pointer-events-none absolute inset-x-0 bottom-0 px-3 pt-6 pb-2 text-[11px] text-ink-faint">
            {pick.build
              ? "showing one enumerated build · clear the job to go back to painting constraints"
              : "drag to pan · scroll to zoom · click a talent to require it, again to bar it"}
          </p>
        </div>

        {/* Scrolls on its own. The panel stack grows as a job runs and then again when its
            results arrive, so a fixed column would push the last panel -- the one holding
            the results -- off the bottom of the screen exactly when it starts mattering. */}
        <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[20rem] md:gap-3 md:overflow-y-auto md:pr-1">
          <section className="panel framed grain p-4">
            <label
              className="flex items-baseline justify-between text-[13px] tracking-[0.14em] uppercase text-ink-faint"
              htmlFor="points"
            >
              Point budget
              <span className="tabular text-[16px] normal-case tracking-normal text-ink">
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
              style={{ accentColor: "var(--arcane)" }}
            />
            {current?.pointCap !== null && (
              <p className="mt-1 text-[11px] text-ink-faint">
                Cap is what the game grants by level 90.
              </p>
            )}
          </section>

          <section className="panel framed grain p-4">
            <h2 className="text-[13px] tracking-[0.14em] uppercase text-ink-faint">
              Constraints
            </h2>
            <div className="mt-2 flex flex-wrap gap-1.5">
              {(
                [
                  ["none", "Require / bar"],
                  ["atLeastOne", "At least one of"],
                  ["exactlyOne", "Exactly one of"],
                ] as const
              ).map(([mode, label]) => (
                <button
                  key={mode}
                  type="button"
                  className={`btn py-1 text-[12px] ${c.groupMode === mode ? "btn-arcane" : ""}`}
                  onClick={() => c.setGroupMode(mode)}
                  aria-pressed={c.groupMode === mode}
                >
                  {label}
                </button>
              ))}
            </div>
            <p className="mt-2 text-[12px] text-ink-soft">
              {c.groupMode === "none"
                ? "Click cycles require → bar → clear. Right-click reverses. Choice nodes cycle side."
                : "Click talents to add them to the group."}
            </p>
            <div className="mt-3 flex items-baseline justify-between">
              <span className="text-[12px] tabular text-ink-faint">
                {c.count} constraint{c.count === 1 ? "" : "s"}
              </span>
              <button
                type="button"
                className="btn py-0.5 text-[12px]"
                onClick={c.reset}
                disabled={c.count === 0}
              >
                Clear
              </button>
            </div>
          </section>

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

          {job && (
            <JobPanel job={job} onCancel={onCancel} onDismiss={() => setJob(null)} />
          )}

          {job && (job.state === "done" || job.state === "capped") && (
            <ResultsBrowser job={job} index={pick.index} onSelect={onPick} />
          )}

          <footer className="px-1 pb-1 text-[11px] leading-relaxed text-ink-faint">
            A fan project. Not affiliated with or endorsed by Blizzard Entertainment.
            {health && (
              <>
                {" "}
                Data revision <span className="tabular">{health.revision}</span>.
              </>
            )}
          </footer>
        </aside>
      </div>
    </div>
  );
}

function labelFor(tree: TreeSummary): string {
  if (tree.kind === "class") return "Class";
  if (tree.kind === "spec") return "Specialisation";
  return tree.name;
}

function groupByClass(trees: TreeSummary[]): [string, TreeSummary[]][] {
  const map = new Map<string, TreeSummary[]>();
  for (const tree of trees) {
    const list = map.get(tree.className);
    if (list) list.push(tree);
    else map.set(tree.className, [tree]);
  }
  return [...map].sort(([a], [b]) => a.localeCompare(b));
}
