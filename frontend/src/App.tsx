import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  getHealth,
  getTree,
  listTrees,
  type Game,
  type Health,
  type TalentNode,
  type TreeDetail,
  type TreeSummary,
} from "./lib/api";
import { classTintStyle } from "./lib/classes";
import { NodeNames, type NamedNode } from "./lib/nodeNames";
import type { TreeInput } from "./lib/enumerate";
import * as loadout from "./lib/loadout";
import { decode, encode, syncUrl, type Role, type Shared } from "./lib/share";
import type { SavedLoadout } from "./lib/saved";
import type { Ranking } from "./lib/simcReport";
import type { Character } from "./lib/space";
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
import { EditorView } from "./components/EditorView";
import { ProjectRail } from "./components/ProjectRail";
import { emptyDesign, fromTrees, type Design } from "./lib/design";
import {
  getProject,
  isDirty,
  projectOfKey,
  readProjects,
  saveProject,
  writeProjects,
  type Project,
} from "./lib/projects";
import { ApiError } from "./lib/api";
import { newId } from "./lib/saved";
import { ShapeKey } from "./components/Legend";
import { LoadoutString } from "./components/LoadoutString";
import { PaintTools } from "./components/PaintTools";
import { PoolCard } from "./components/PoolCard";
import { PopularPanel } from "./components/PopularPanel";
import type { Popular, PopularBuild } from "./lib/api";
import { ShareButton } from "./components/ShareButton";
import { SavedPanel } from "./components/SavedPanel";
import { SimulateView, type Sim } from "./components/SimulateView";
import { SpaceCard, totalOf, type SpaceRow } from "./components/SpaceCard";
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

type Step = "design" | "narrow" | "simulate" | "analyse";

const DEFAULT_LIMIT = 10000;
const STALE_AFTER_S = 14 * 24 * 3600;

const formatAge = (seconds: number) => {
  const days = Math.floor(seconds / 86400);
  if (days >= 1) return `${days} day${days === 1 ? "" : "s"}`;
  const hours = Math.floor(seconds / 3600);
  return hours >= 1 ? `${hours} hour${hours === 1 ? "" : "s"}` : "under an hour";
};

// The URL is the *initial* state; after that the app owns it and writes back.
const SHARED = decode(window.location.search);

const kindOrder = (kind: string) => ["class", "spec", "hero", "tab"].indexOf(kind);

const LABEL: Record<Role, string> = { class: "Class", spec: "Spec", hero: "Hero", hero2: "Hero" };

/** A link names a tree, and a Forever tree's key says so. */
const STARTING_GAME: Game = SHARED.spec?.startsWith("forever/")
  ? "forever"
  : SHARED.spec?.startsWith("custom/")
    ? "custom"
    : "retail";

export default function App() {
  const { resolved, toggle } = useTheme();
  /*
    Which game's trees. Retail: a spec's class, spec and hero trees, each with its own points.
    WoW Forever: a class's three talent tabs, sharing one pool of 51 -- vanilla's shape. Both
    run on the same solver and canvas; what differs is where the panes come from and how their
    budgets relate.
  */
  const [game, setGame] = useState<Game>(STARTING_GAME);
  const forever = game === "forever";
  const custom = game === "custom";
  /** Forever and custom trees are tabs: a set of trees taking the three panes in order. */
  const tabbed = game !== "retail";

  /*
    Custom projects: the player's own trees, designed in the editor. The list and every draft
    live in this browser; each saved version lives on the server, content-addressed, because
    the solver has to be able to read it.
  */
  const [projects, setProjects] = useState<Project[]>(() => readProjects());
  const [projectId, setProjectId] = useState<string | null>(() => readProjects()[0]?.id ?? null);
  const project = projects.find((p) => p.id === projectId) ?? null;
  useEffect(() => {
    writeProjects(projects);
  }, [projects]);
  const updateProject = useCallback((id: string, change: (p: Project) => Project) => {
    setProjects((list) => list.map((p) => (p.id === id ? { ...change(p), updatedAt: Date.now() } : p)));
  }, []);
  const addProject = useCallback((draft: Design, savedAs: string | null = null) => {
    const entry: Project = {
      id: newId(),
      draft,
      savedAs,
      savedDraft: savedAs ? JSON.stringify(draft) : null,
      updatedAt: Date.now(),
    };
    setProjects((list) => [entry, ...list]);
    setProjectId(entry.id);
    return entry;
  }, []);
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
  /**
   * A ready list of characters for the Simulate step, instead of enumerating the trees: the
   * builds top players run. Cleared whenever a sim is started from the trees themselves.
   */
  const [preset, setPreset] = useState<{ signature: string; characters: Character[]; description: string } | null>(null);
  /** Top players' pick rates, painted on the trees while this is set. */
  const [popularHeat, setPopularHeat] = useState<Popular | null>(null);
  const [sim, setSim] = useState<Sim | null>(null);
  const [analysis, setAnalysis] = useState<{ sim: Sim; ranking: Ranking } | null>(null);

  // --- which trees belong to the current spec -----------------------------
  /*
    A class tree belongs to a *specialisation*, not just a class: which talents are granted
    for free differs between them, so matching on class alone showed the wrong granted
    talents and produced strings the game would not accept.
  */
  const group = useMemo(() => {
    if (tabbed) {
      // A class's three tabs, in the game's own order, take the three panes. The third sits
      // in the hero slot purely as a position -- it is not a hero tree and nothing treats it
      // as one: there is exactly one, so there is nothing to choose or pool. A custom
      // project's list holds only its own trees, so it needs no class to filter by.
      const tabs = trees
        .filter((t) => t.kind === "tab" && (custom || t.className === className))
        .sort((a, b) => (a.order ?? 0) - (b.order ?? 0));
      return { class: tabs[0] ?? null, spec: tabs[1] ?? null, heroes: tabs[2] ? [tabs[2]] : [] };
    }
    const mine = trees.filter((t) => t.className === className && t.specName === specName);
    return {
      class: mine.find((t) => t.kind === "class") ?? null,
      spec: mine.find((t) => t.kind === "spec") ?? null,
      heroes: mine.filter((t) => t.kind === "hero"),
    };
  }, [trees, className, specName, tabbed, custom]);
  const hero = group.heroes.find((h) => h.key === heroKey) ?? null;

  const roles = useMemo(
    () =>
      (
        [
          ["class", group.class],
          ["spec", group.spec],
          ["hero", hero],
        ] as [Role, TreeSummary | null][]
      ).filter((r): r is [Role, TreeSummary] => r[1] !== null),
    [group.class, group.spec, hero],
  );

  /*
    Both hero trees in one search.

    A player's first question about hero talents is which tree, not which talents -- and each
    hero tree already keeps its own fixed build or search, since work is kept per tree. So
    "both" just adds the other one to the trees that count: the hero factor becomes a sum,
    class x spec x (San'layn + Deathbringer), and the picker chooses which one is being edited.
  */
  const [bothHeroes, setBothHeroes] = useState(SHARED.both);
  const otherHero = group.heroes.find((h) => h.key !== heroKey) ?? null;
  const members = useMemo(
    () => (bothHeroes && otherHero ? [...roles, ["hero2", otherHero] as [Role, TreeSummary]] : roles),
    [roles, bothHeroes, otherHero],
  );

  // --- bootstrap ----------------------------------------------------------
  // Per game: each has its own trees, its own revision and its own data age.
  const firstLoad = useRef(true);
  useEffect(() => {
    let cancelled = false;
    void (async () => {
      try {
        if (game === "custom") {
          // Custom trees are not listed by game; they come from the chosen project, below.
          const h = await getHealth("retail");
          if (!cancelled) setHealth(h);
          firstLoad.current = false;
          return;
        }
        const [h, list] = await Promise.all([getHealth(game), listTrees({ game })]);
        if (cancelled) return;
        setHealth(h);
        setTrees(list);
        // The link's tree on first load; afterwards, keep the class if this game has it.
        const linked = firstLoad.current ? list.find((t) => t.key === SHARED.spec) : undefined;
        firstLoad.current = false;
        const start =
          linked ??
          list.find((t) => t.className === className && (t.kind === "spec" || t.kind === "tab")) ??
          list.find((t) => t.kind === "spec" || t.kind === "tab") ??
          list[0];
        if (start) {
          setClassName(start.className);
          setSpecName(start.kind === "tab" ? null : start.specName);
        }
      } catch (error) {
        if (!cancelled) setLoadError(error instanceof Error ? error.message : String(error));
      }
    })();
    return () => {
      cancelled = true;
    };
    // The class is read, not watched: switching class must not refetch the tree list.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [game]);

  // The chosen project's last saved version is what the planner and the counter use.
  const savedAs = custom ? (project?.savedAs ?? null) : null;
  useEffect(() => {
    if (!custom) return;
    if (!savedAs) {
      setTrees([]);
      return;
    }
    let cancelled = false;
    void getProject(savedAs)
      .then((saved) => {
        if (cancelled) return;
        setTrees(saved.trees);
        setClassName(saved.name);
        setSpecName(null);
      })
      .catch((error: unknown) => {
        if (!cancelled) setNote(`Could not load the saved project: ${error instanceof Error ? error.message : String(error)}`);
      });
    return () => {
      cancelled = true;
    };
  }, [custom, savedAs]);

  // A link to a custom tree opens its project -- adding it to this browser's list if it is new.
  useEffect(() => {
    const linked = projectOfKey(SHARED.spec);
    if (!linked) return;
    const known = readProjects().find((p) => p.savedAs === linked);
    if (known) {
      setProjectId(known.id);
      return;
    }
    void getProject(linked)
      .then((saved) => {
        if (saved.design) addProject(saved.design, linked);
      })
      .catch(() => setNote("That link names a custom project this server does not have."));
    // Once, for the link the page was opened with.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // The custom game always has a project to show, even the first time.
  useEffect(() => {
    if (custom && projects.length === 0) addProject(emptyDesign());
  }, [custom, projects.length, addProject]);

  const switchGame = useCallback((next: Game) => {
    setGame(next);
    setSim(null);
    setAnalysis(null);
    setStep(next === "custom" ? "design" : "narrow");
    setNote(null);
    setBothHeroes(false);
  }, []);

  useEffect(() => {
    if (group.heroes.length === 0) return;
    setHeroKey((current) =>
      current && group.heroes.some((h) => h.key === current) ? current : group.heroes[0]!.key,
    );
  }, [group.heroes]);

  /*
    A link's trees are by role; map them onto keys once the keys are known.

    The same path opens a saved setup: a save *is* a link, so it is decoded into `incoming` and
    seeded exactly as a link is -- one path, so the two can never disagree. A link seeds under
    anything already on screen; a save replaces the spec's trees outright, including any the
    save leaves empty, since that emptiness is part of what was saved.
  */
  const seeded = useRef(false);
  const incoming = useRef<{ shared: Shared; replace: boolean }>({ shared: SHARED, replace: false });
  const [seedTick, setSeedTick] = useState(0);
  useEffect(() => {
    const primary = group.spec ?? (tabbed ? group.class : null);
    if (seeded.current || !primary || !group.class || (group.heroes.length && !hero)) return;
    const { shared, replace } = incoming.current;
    if (shared.spec && shared.spec !== primary.key) return; // still switching spec
    if (shared.hero && hero && shared.hero !== hero.key && group.heroes.some((h) => h.key === shared.hero)) return;
    seeded.current = true;
    const next: Record<string, TreeWork> = {};
    if (replace) {
      for (const k of [group.class.key, group.spec?.key, ...group.heroes.map((h) => h.key)].filter(Boolean) as string[]) {
        next[k] = emptyWork(forever ? "fixed" : "open");
      }
    }
    for (const [role, summary] of roles) {
      const work = shared.work[role];
      if (work) next[summary.key] = work;
    }
    const second = group.heroes.find((h) => h.key === shared.hero2);
    if (second && shared.work.hero2) next[second.key] = shared.work.hero2;
    if (Object.keys(next).length) {
      setWork((previous) => (replace ? { ...previous, ...next } : { ...next, ...previous }));
    }
  }, [group.spec, group.class, group.heroes, hero, roles, seedTick, forever, tabbed]);

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

  /*
    What an untouched tree is. Retail trees start open: the space is the thing to narrow.
    Forever tabs start fixed and empty -- a vanilla calculator, where the first thing anyone
    does is spend points by hand -- and one can be opened to explore from there.
  */
  const defaultMode = forever ? "fixed" : "open";
  const workOf = useCallback((key: string) => work[key] ?? emptyWork(defaultMode), [work, defaultMode]);
  const update = useCallback(
    (key: string, change: (w: TreeWork) => TreeWork) =>
      setWork((previous) => ({ ...previous, [key]: change(previous[key] ?? emptyWork(defaultMode)) })),
    [defaultMode],
  );

  const selectSpec = useCallback((nextClass: string, nextSpec: string | null) => {
    setClassName(nextClass);
    // Clicking a retail class names no spec; it opens on the class's first one. Forever has
    // no specs, and null is exactly right there.
    setSpecName(
      nextSpec ??
        trees.find((t) => t.className === nextClass && t.kind === "spec")?.specName ??
        null,
    );
    setSim(null);
    setAnalysis(null);
    setStep("narrow");
    setNote(null);
  }, [trees]);

  // --- the shared point pool ------------------------------------------------
  /*
    Forever's three tabs draw on one pool of 51, so no tab has a cap of its own: each may
    use what the other two leave. A fixed tab commits the points it has spent; an open tab
    commits an explicit budget, or, left at its default, takes what is left in tab order.
    Retail trees have separate budgets and this is simply their own cap.
  */
  // Forever always pools; a custom project pools when its designer chose one.
  const pool = forever
    ? (group.class ? (loaded[group.class.key]?.sharedPointCap ?? 51) : 51)
    : custom && group.class
      ? (loaded[group.class.key]?.sharedPointCap ?? null)
      : null;
  const allowance = useMemo(() => {
    const out = new Map<string, number>();
    if (pool === null) return out;
    const tabs = [group.class, group.spec, ...group.heroes].filter(Boolean) as TreeSummary[];
    const used = new Map<string, number>();
    for (const t of tabs) {
      const w = workOf(t.key);
      used.set(
        t.key,
        w.mode === "fixed" ? loadout.total(w.points) : w.search.budget !== null ? Math.min(w.search.budget, capOf(t)) : 0,
      );
    }
    let left = pool - [...used.values()].reduce((a, b) => a + b, 0);
    for (const t of tabs) {
      const w = workOf(t.key);
      if (w.mode === "open" && w.search.budget === null) {
        const take = Math.max(0, Math.min(capOf(t), left));
        used.set(t.key, take);
        left -= take;
      }
    }
    for (const t of tabs) {
      const others = [...used].filter(([k]) => k !== t.key).reduce((a, [, v]) => a + v, 0);
      out.set(t.key, Math.max(0, Math.min(capOf(t), pool - others)));
    }
    return out;
  }, [pool, group.class, group.spec, group.heroes, workOf]);
  const capFor = useCallback(
    (summary: TreeSummary) => allowance.get(summary.key) ?? capOf(summary),
    [allowance],
  );
  const labelOf = useCallback(
    (role: Role, summary: TreeSummary) => (tabbed ? summary.name : LABEL[role]),
    [tabbed],
  );

  // --- per-tree counts ----------------------------------------------------
  const countRequests = useMemo(
    () =>
      members.map(([, summary]) => {
        const w = workOf(summary.key);
        return {
          key: summary.key,
          payload:
            w.mode === "open" && pendingOf(w).length === 0 ? payloadOf(w, capFor(summary)) : null,
        };
      }),
    [members, workOf, capFor],
  );
  const counts = useCounts(countRequests);

  const rows: SpaceRow[] = members.map(([role, summary]) => {
    const w = workOf(summary.key);
    const c = counts[summary.key];
    return {
      key: summary.key,
      // Two hero rows need their names; one does not.
      label: role.startsWith("hero") && bothHeroes ? summary.name : labelOf(role, summary),
      op: role === "hero2" ? ("+" as const) : undefined,
      fixed: w.mode === "fixed",
      builds: w.mode === "fixed" ? 1 : (c?.builds ?? null),
      stale: w.mode === "open" && (c?.stale ?? true),
      error: w.mode === "open" ? (c?.error ?? null) : null,
    };
  });
  const pending = members.flatMap(([role, s]) =>
    pendingOf(workOf(s.key)).map((p) => `${p} in the ${labelOf(role, s).toLowerCase()} tree`),
  );
  const total = rows.length >= 3 ? totalOf(rows) : null;
  // Two hero trees at different budgets compare the extra points, not the trees.
  const heroBudget = (summary: TreeSummary | null) => {
    if (!summary) return null;
    const w = workOf(summary.key);
    return w.mode === "fixed" ? loadout.total(w.points) : budgetOf(w, capOf(summary));
  };
  const heroHint =
    bothHeroes && hero && otherHero && heroBudget(hero) !== heroBudget(otherHero)
      ? `${hero.name} spends ${heroBudget(hero)} points and ${otherHero.name} ${heroBudget(otherHero)} — give them the same budget to compare the trees fairly.`
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
        const result = spend(w, tree, capFor(summary), node, alternate);
        update(summary.key, () => result.work);
        setNote(result.note);
      } else {
        update(summary.key, (current) => paint(current, node, tool, alternate));
        setNote(null);
      }
    },
    [loaded, workOf, update, tool, capFor],
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
      setBothHeroes(false);
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
      setBothHeroes(false);
      setNote(`Build ${character.line} is now fixed on all three trees.`);
      setStep("narrow");
    },
    [loaded, trees, group.heroes],
  );

  /** A build the top players run, taken as the loadout: every tree fixed to it. */
  const usePopular = useCallback(
    (build: PopularBuild) => {
      setWork((previous) => {
        const next = { ...previous };
        for (const [key, points] of Object.entries(build.points)) {
          const tree = loaded[key];
          const summary = trees.find((t) => t.key === key);
          if (!tree || !summary) continue;
          next[key] = fixedFrom(previous[key] ?? emptyWork(), tree, capOf(summary), points, build.choices);
        }
        return next;
      });
      if (build.hero) setHeroKey(build.hero);
      setBothHeroes(false);
      setPopularHeat(null);
      setNote(`Fixed to a build ${build.count} top players run (e.g. ${build.example}).`);
    },
    [loaded, trees],
  );

  /*
    Narrow to what the top players have not settled.

    A talent taken by at least `agreement` of them is required, one taken by no more than the
    rest is barred, and a choice node they agree on is pinned to its side; everything between
    stays open. On their most common hero tree. What is left is the part of the tree the best
    players still disagree about -- which is exactly the part worth simming.
  */
  const narrowToContested = useCallback(
    (popular: Popular, agreement: number) => {
      const heroTop = popular.heroes.find((h) => h.key)?.key ?? heroKey;
      if (heroTop) setHeroKey(heroTop);
      setBothHeroes(false);
      const keys = [group.class?.key, group.spec?.key, heroTop].filter(Boolean) as string[];
      // Computed here, not inside the state updater: React runs that later, and the note
      // below needs the numbers now.
      let settled = 0;
      let open = 0;
      const entries: Record<string, TreeWork> = {};
      for (const key of keys) {
        const tree = loaded[key];
        if (!tree) continue;
        const required: number[] = [];
        const excluded: number[] = [];
        const sides: Record<string, "a" | "b"> = {};
        for (const node of tree.nodes) {
          if (node.preFilled || node.kind === "subtree") continue;
          const share = popular.pickRates[String(node.nodeId)]?.share ?? 0;
          if (share >= agreement) {
            const split = popular.choiceSides[String(node.nodeId)];
            if (node.kind === "choice" && split && split[0] >= agreement) sides[String(node.nodeId)] = "a";
            else if (node.kind === "choice" && split && split[1] >= agreement) sides[String(node.nodeId)] = "b";
            else required.push(node.nodeId);
            settled++;
          } else if (share <= 1 - agreement) {
            excluded.push(node.nodeId);
            settled++;
          } else {
            open++;
          }
        }
        entries[key] = { ...workOf(key), mode: "open", search: { ...EMPTY_SEARCH, required, excluded, sides } };
      }
      setWork((previous) => ({ ...previous, ...entries }));
      setPopularHeat(null);
      setNote(
        `Narrowed to what top players contest: ${settled} talents settled at ${Math.round(agreement * 100)}% agreement, ${open} left open.`,
      );
    },
    [group.class, group.spec, heroKey, loaded, workOf],
  );

  /** Sim the builds top players actually run, as they are. */
  const simTopBuilds = useCallback(
    (popular: Popular) => {
      const characters: Character[] = popular.builds.slice(0, limit).map((b, i) => {
        const parts: Character["parts"] = {};
        const points: loadout.Points = {};
        const choices: Record<string, number> = {};
        for (const [key, pts] of Object.entries(b.points)) {
          const ids = new Set((loaded[key]?.nodes ?? []).map((n) => String(n.nodeId)));
          const mine = Object.fromEntries(Object.entries(b.choices).filter(([id]) => ids.has(id)));
          parts[key] = { points: pts, choices: mine, tree: key };
          Object.assign(points, pts);
          Object.assign(choices, mine);
        }
        const heroSubTreeId = b.hero ? (trees.find((t) => t.key === b.hero)?.subTreeId ?? null) : null;
        return { line: i + 1, parts, points, choices, heroSubTreeId };
      });
      const where = popular.encounters.length > 1 ? popular.zone.name : popular.encounters[0];
      setPreset({
        signature: `popular:${group.spec?.key}:${popular.zone.id}:${popular.encounters.join(",")}:${popular.difficulty}:${popular.fetchedAt}:${limit}`,
        characters,
        description: `The ${characters.length.toLocaleString("en-US")} distinct builds the top ${popular.players.toLocaleString("en-US")} players run in ${where}${popular.builds.length > characters.length ? ", most common first" : ""}.`,
      });
      setPopularHeat(null);
      setStep("simulate");
    },
    [limit, loaded, trees, group.spec],
  );

  // --- the simulation step ------------------------------------------------
  const inputs: TreeInput[] = members
    .filter(([, s]) => loaded[s.key])
    .map(([role, s]) => ({
      key: s.key,
      tree: loaded[s.key]!,
      work: workOf(s.key),
      cap: capFor(s),
      // Both hero trees fill one factor of the product, and each names its own sub-tree.
      ...(role.startsWith("hero") ? { slot: "hero", hero: s.subTreeId } : {}),
    }));
  const signature = JSON.stringify({
    limit,
    hero: hero?.subTreeId ?? null,
    both: bothHeroes,
    trees: inputs.map((i) =>
      i.work.mode === "fixed"
        ? { key: i.key, points: i.work.points, picks: i.work.picks }
        : { key: i.key, search: payloadOf(i.work, i.cap), sides: i.work.search.sides },
    ),
  });
  const labels = Object.fromEntries(
    members.map(([role, s]) => [s.key, role.startsWith("hero") && bothHeroes ? s.name : labelOf(role, s)]),
  );
  // Only the trees this character can have: the sibling spec's hero-talent targets are not
  // in here, which is what keeps the tooltip from naming abilities this spec never gets.
  const nodeNames = useMemo(() => {
    const map = new Map<number, NamedNode>();
    for (const [role, s] of members) {
      for (const n of loaded[s.key]?.nodes ?? []) map.set(n.nodeId, { name: n.name, tree: labelOf(role, s) });
    }
    return map;
  }, [members, loaded, labelOf]);
  const allTrees = wanted.map((k) => loaded[k]).filter(Boolean) as TreeDetail[];
  const specTree = group.spec ? (loaded[group.spec.key] ?? null) : null;

  // --- the link -----------------------------------------------------------
  const sharedNow = useMemo((): Shared | null => {
    const primary = group.spec ?? (tabbed ? group.class : null);
    if (!primary) return null;
    const out: Partial<Record<Role, TreeWork>> = {};
    for (const [role, s] of roles) if (work[s.key]) out[role] = work[s.key];
    // The other hero tree rides along whenever it holds something, simmed or not, so a
    // link never loses what was painted on the tree not currently shown.
    if (otherHero && work[otherHero.key]) out.hero2 = work[otherHero.key];
    return {
      spec: primary.key,
      hero: heroKey,
      hero2: otherHero?.key ?? null,
      both: bothHeroes,
      work: out,
      limit: limit === DEFAULT_LIMIT ? null : limit,
    };
  }, [group.spec, group.class, tabbed, heroKey, otherHero, bothHeroes, roles, work, limit]);
  useEffect(() => {
    if (sharedNow && seeded.current) syncUrl(sharedNow);
  }, [sharedNow]);

  /** Open a saved setup: decode its link and seed it as a link would be, replacing. */
  const openSaved = useCallback(
    (saved: SavedLoadout) => {
      const shared = decode(saved.query);
      const savedGame: Game = shared.spec?.startsWith("forever/") ? "forever" : "retail";
      if (savedGame !== game) {
        // The list for the other game has to load first; the link path does the rest.
        window.location.search = saved.query;
        return;
      }
      const summary = trees.find((t) => t.key === shared.spec);
      if (!summary) {
        setNote(`“${saved.name}” is for a specialisation this data no longer has.`);
        return;
      }
      incoming.current = { shared, replace: true };
      seeded.current = false;
      setClassName(summary.className);
      setSpecName(summary.specName);
      if (shared.hero) setHeroKey(shared.hero);
      setBothHeroes(shared.both);
      setLimit(shared.limit ?? DEFAULT_LIMIT);
      setSeedTick((t) => t + 1);
      setStep("narrow");
      setNote(`Opened “${saved.name}”.`);
    },
    [trees, game],
  );

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
    const cap = capFor(summary);
    const c = counts[summary.key];
    return (
      <TreePane
        key={summary.key}
        tree={tree}
        title={labelOf(role, summary)}
        subtitle={tabbed ? null : role === "class" ? className : role === "spec" ? specName : null}
        mode={w.mode}
        onMode={(m) => onMode(summary.key, m)}
        points={{ value: w.mode === "fixed" ? loadout.total(w.points) : budgetOf(w, cap), cap, shared: forever }}
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
        states={popularHeat ? undefined : statesOf(w)}
        sides={popularHeat ? undefined : sidesOf(w)}
        shares={
          popularHeat && tree
            ? new Map(tree.nodes.map((n) => [n.nodeId, popularHeat.pickRates[String(n.nodeId)]?.share ?? 0]))
            : null
        }
        build={popularHeat ? null : w.mode === "fixed" ? drawnBuild(w, tree) : null}
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
    <NodeNames.Provider value={nodeNames}>
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
        {/* Which game. Two words, because the two are different enough that a player
            should always know which trees they are looking at. */}
        <div className="seg shrink-0" role="group" aria-label="Game">
          {([
            ["retail", "Retail"],
            ["forever", "WoW Forever"],
            ["custom", "Custom"],
          ] as const).map(([id, label]) => (
            <button key={id} type="button" aria-pressed={game === id} onClick={() => switchGame(id)}>
              {label}
            </button>
          ))}
        </div>

        {custom ? (
          <ProjectRail
            projects={projects}
            current={projectId}
            onSelect={(id) => {
              setProjectId(id);
              setStep("design");
              setNote(null);
            }}
            onNew={() => {
              addProject(emptyDesign(`Project ${projects.length + 1}`));
              setStep("design");
            }}
            onRemove={(id) => {
              const rest = projects.filter((p) => p.id !== id);
              setProjects(rest);
              setProjectId(rest[0]?.id ?? null);
            }}
          />
        ) : (
          <SpecRail trees={trees} className={className} specName={specName} onSelect={selectSpec} />
        )}

        {/* The workflow, in order. A step is reachable once the one before it has produced
            what it needs: something simmable, then a report. */}
        <nav className="steps ml-auto" aria-label="Workflow">
          {(custom
            ? ([
                ["design", "Design", true],
                ["narrow", "Plan", Boolean(project?.savedAs)],
              ] as const)
            : ([
                ["narrow", "Narrow", true],
                ["simulate", "Simulate", !forever && (simmable || sim !== null || preset !== null)],
                ["analyse", "Analyse", !forever && analysis !== null],
              ] as const)
          ).map(([id, label, enabled], i) => (
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
                    : custom
                      ? "Save the project first"
                      : forever
                      ? "SimulationCraft does not simulate WoW Forever"
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

      {step === "design" && custom && project && (
        <EditorView
          draft={project.draft}
          dirty={isDirty(project)}
          saved={Boolean(project.savedAs)}
          onChange={(draft) => updateProject(project.id, (p) => ({ ...p, draft }))}
          onSave={async () => {
            try {
              const saved = await saveProject(project.draft);
              updateProject(project.id, (p) => ({ ...p, savedAs: saved.project, savedDraft: JSON.stringify(p.draft) }));
              return {
                ok: true,
                message: `Saved as ${saved.project.slice(0, 8)}… — the planner now uses this version.`,
                warnings: saved.warnings,
              };
            } catch (error) {
              return { ok: false, message: error instanceof ApiError ? error.detail : String(error) };
            }
          }}
          onPlan={project.savedAs ? () => setStep("narrow") : null}
        />
      )}

      {step === "narrow" && (
        <div className="flex min-h-0 flex-1 flex-col gap-2 p-2 md:flex-row md:gap-2.5 md:p-2.5">
          {group.class && pane("class", group.class, { className: "min-h-[22rem] flex-1 md:min-h-0" })}
          {group.spec && pane("spec", group.spec, { className: "min-h-[22rem] flex-1 md:min-h-0" })}
          {hero &&
            pane("hero", hero, {
              className: tabbed
                ? "min-h-[22rem] flex-1 md:min-h-0"
                : "min-h-[18rem] md:min-h-0 md:w-[20rem] md:shrink-0",
              children: tabbed ? null : (
                <span className="ml-auto flex shrink-0 items-center gap-1">
                  {group.heroes.map((h) => (
                    <button
                      key={h.key}
                      type="button"
                      className="rail-item !px-1 !text-[10.5px]"
                      aria-pressed={h.key === heroKey}
                      onClick={() => setHeroKey(h.key)}
                      title={bothHeroes ? `Edit ${h.name} — both are simmed` : h.name}
                    >
                      {h.name}
                    </button>
                  ))}
                  {group.heroes.length > 1 && (
                    <button
                      type="button"
                      className="chip !cursor-pointer"
                      aria-pressed={bothHeroes}
                      onClick={() => setBothHeroes((b) => !b)}
                      style={bothHeroes ? { color: "var(--star-bright)", borderColor: "var(--star)" } : undefined}
                      title={
                        bothHeroes
                          ? "Both hero trees are simmed. The names choose which one you are editing."
                          : "Sim both hero trees together, to see which is better"
                      }
                    >
                      {bothHeroes ? "both ✓" : "both"}
                    </button>
                  )}
                </span>
              ),
            })}

          <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[19rem] md:gap-2.5 md:overflow-y-auto md:pr-1">
            {tabbed && pool !== null && (
              <PoolCard
                pool={pool}
                firstLevel={forever ? 10 : null}
                tabs={roles.map(([, s]) => {
                  const w = workOf(s.key);
                  return { name: s.name, points: w.mode === "fixed" ? loadout.total(w.points) : budgetOf(w, capFor(s)) };
                })}
              />
            )}

            <SpaceCard
              rows={rows}
              limit={limit}
              onLimit={setLimit}
              pending={pending}
              onSimulate={() => {
                setPreset(null);
                setStep("simulate");
              }}
              hint={heroHint}
              unavailable={
                forever
                  ? "SimulationCraft does not simulate WoW Forever, so this counts the builds but cannot sim them."
                  : custom
                    ? "A custom tree has no spells for SimulationCraft to sim, so this counts the builds but cannot sim them."
                    : null
              }
            />

            {note && (
              <p className="panel px-3.5 py-2 text-[11.5px] leading-snug" style={{ color: "var(--brass-bright)" }}>
                {note}
              </p>
            )}

            {!tabbed && group.spec && (
              <PopularPanel
                specKey={group.spec.key}
                heroName={(key) => trees.find((t) => t.key === key)?.name ?? "No hero tree"}
                onUse={usePopular}
                onNarrow={narrowToContested}
                heat={popularHeat !== null}
                onHeat={setPopularHeat}
                onSimTop={simTopBuilds}
                limit={limit}
              />
            )}

            <PaintTools
              tool={tool}
              onTool={setTool}
              anyOpen={anyOpen}
              onClear={clearSearches}
              clearable={anyConstraint}
              counts={paintCounts}
            />

            {!tabbed && (
            <LoadoutString
              spec={specTree}
              trees={allTrees}
              points={fixedPoints}
              choices={fixedPicks}
              heroSubTreeId={hero?.subTreeId ?? null}
              onImport={onImportString}
              exportable={allFixed}
            />
            )}

            <SavedPanel
              query={sharedNow ? encode(sharedNow) : ""}
              spec={group.spec?.key ?? null}
              revision={health?.revision ?? null}
              onOpen={openSaved}
            />

            {!custom && roles.every(([, s]) => loaded[s.key]) && (
              <button
                type="button"
                className="btn"
                title="Copy these trees into the tree editor, to change them and count the result"
                onClick={() => {
                  const copy = fromTrees(
                    forever ? `${className} (copy)` : `${specName} ${className} (copy)`,
                    roles.map(([, s]) => loaded[s.key]!),
                    pool,
                  );
                  addProject(copy);
                  switchGame("custom");
                }}
              >
                Edit a copy in the tree editor
              </button>
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
              <ShapeKey
                kinds={forever ? ["passive", "active"] : custom ? ["passive", "active", "choice"] : undefined}
              />
            </section>

            <footer className="px-1 pb-1 text-[10.5px] leading-relaxed text-ink-faint">
              {forever && (
                <>
                  WoW Forever talent data from{" "}
                  <a className="underline" href="https://talentsforever.com" target="_blank" rel="noreferrer">
                    talentsforever.com
                  </a>
                  , read from the beta client, under{" "}
                  <a
                    className="underline"
                    href="https://creativecommons.org/licenses/by/4.0/"
                    target="_blank"
                    rel="noreferrer"
                  >
                    CC BY 4.0
                  </a>
                  . It is beta data and will change before launch.{" "}
                </>
              )}
              A fan project. Not affiliated with or endorsed by Blizzard Entertainment.
              {health && (
                <>
                  {" "}
                  Data revision <span className="num">{health.revision}</span>
                  {/* Talent data goes stale silently: a patch lands, the ingest does not run,
                      and every count is for last week's trees. Saying how old it is -- and
                      saying it loudly past a fortnight -- is the alert a player can act on. */}
                  {health.dataAgeSeconds !== null && health.dataAgeSeconds !== undefined && (
                    <span
                      style={{ color: health.dataAgeSeconds > STALE_AFTER_S ? "var(--any-of)" : undefined }}
                      title={health.dataAgeSeconds > STALE_AFTER_S ? "Talents may predate the latest patch" : undefined}
                    >
                      , {formatAge(health.dataAgeSeconds)} old
                      {health.dataAgeSeconds > STALE_AFTER_S && " — may predate the latest patch"}
                    </span>
                  )}
                  .
                </>
              )}
            </footer>
          </aside>
        </div>
      )}

      {step === "simulate" && specTree && className && specName && (
        <div className="flex min-h-0 flex-1">
          <SimulateView
            signature={preset?.signature ?? signature}
            preset={preset}
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
          trees={[...new Set(analysis.sim.characters.flatMap((c) => Object.keys(c.parts)))]
            .filter((k) => loaded[k])
            // Class, spec, hero, whatever order the builds happened to list their trees in.
            .sort((a, b) => kindOrder(loaded[a]!.kind) - kindOrder(loaded[b]!.kind))
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
    </NodeNames.Provider>
  );
}
