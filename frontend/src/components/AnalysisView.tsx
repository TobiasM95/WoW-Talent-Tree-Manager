import { useMemo, useState } from "react";
import type { TreeDetail } from "../lib/api";
import * as loadout from "../lib/loadout";
import { duels, impact, type Duel, type Ranking, type TalentImpact } from "../lib/simcReport";
import { formatCount, traits, type Character } from "../lib/space";
import type { Sim } from "./SimulateView";
import { TreePane } from "./TreePane";

/**
 * Step three: what the sim said.
 *
 * A page, not a sidebar panel, because this is where the tool pays off and it needs the room:
 * the ranking, what each talent was worth, and each choice node's two sides compared -- with
 * the three trees beside it, drawn either as the build selected in the table or as a heat
 * map of talent value.
 *
 * Two limits are stated wherever a number is read rather than once in a footnote. A sim
 * reports a mean with an error bar, so a difference smaller than that is a tie; and talent
 * values are observational, so talents the trees never separate share a score.
 */

export interface AnalysisTree {
  key: string;
  label: string;
  tree: TreeDetail;
}

export interface AnalysisViewProps {
  sim: Sim;
  ranking: Ranking;
  trees: AnalysisTree[];
  onUse: (character: Character) => void;
  onBack: () => void;
  onAnother: () => void;
}

type Tab = "builds" | "talents" | "choices";

const pct = (n: number, digits = 1) =>
  `${n > 0 ? "+" : n < 0 ? "−" : "±"}${Math.abs(n * 100).toFixed(digits)}%`;
const fmt = (n: number) => Math.round(n).toLocaleString("en-US");

const PAGE = 100;

export function AnalysisView({ sim, ranking, trees, onUse, onBack, onAnother }: AnalysisViewProps) {
  const [tab, setTab] = useState<Tab>("builds");
  const [selected, setSelected] = useState(ranking.best.line);
  const [shown, setShown] = useState(PAGE);
  const [copied, setCopied] = useState(false);

  const byLine = useMemo(() => new Map(sim.characters.map((c) => [c.line, c])), [sim]);
  const keys = useMemo(() => (line: number) => {
    const c = byLine.get(line);
    return c ? traits(c) : undefined;
  }, [byLine]);

  const impacts = useMemo(() => impact(ranking, keys), [ranking, keys]);
  const choiceDuels = useMemo(() => duels(ranking, keys), [ranking, keys]);

  /*
    How big a difference has to be before it is more than sim noise.

    Each build's mean carries its own error; a *group* mean over n builds carries roughly
    that divided by sqrt(n), and a difference of two groups adds the two in quadrature. It is
    an honest floor for sim noise and says nothing about the other limit -- talents taken
    together cannot be told apart -- which is why that one is stated in words instead.
  */
  const relError = useMemo(() => {
    const errs = ranking.builds.map((b) => (b.mean > 0 ? b.error / b.mean : 0)).sort((a, b) => a - b);
    return errs[Math.floor(errs.length / 2)] ?? 0;
  }, [ranking]);
  const noise = (nA: number, nB: number) => relError * Math.sqrt(1 / Math.max(1, nA) + 1 / Math.max(1, nB));

  const nodeTree = useMemo(() => {
    const map = new Map<number, AnalysisTree>();
    for (const t of trees) for (const n of t.tree.nodes) map.set(n.nodeId, t);
    return map;
  }, [trees]);
  const nodeOf = (id: number) => nodeTree.get(id)?.tree.nodes.find((n) => n.nodeId === id);

  // Node-level values, per tree, and the heat each tree is painted with.
  const perTree = useMemo(() => {
    const out = new Map<string, TalentImpact[]>();
    for (const row of impacts) {
      if (row.side !== undefined) continue;
      const t = nodeTree.get(row.nodeId);
      if (!t) continue;
      out.set(t.key, [...(out.get(t.key) ?? []), row]);
    }
    return out;
  }, [impacts, nodeTree]);

  const heat = useMemo(() => {
    const out = new Map<string, Map<number, number>>();
    for (const [key, rows] of perTree) {
      const widest = Math.max(1e-9, ...rows.map((r) => Math.abs(r.delta)));
      out.set(key, new Map(rows.map((r) => [r.nodeId, r.delta / widest])));
    }
    return out;
  }, [perTree]);

  const selectedCharacter = byLine.get(selected) ?? null;
  const selectedRank = ranking.byLine.get(selected) ?? null;
  const best = byLine.get(ranking.best.line)!;

  // What a build does differently from the best one, in talent names.
  const differences = (c: Character) => {
    const mine = new Set(traits(c).filter((k) => !k.includes(":")));
    const theirs = new Set(traits(best).filter((k) => !k.includes(":")));
    const added = [...mine].filter((k) => !theirs.has(k)).map((k) => nodeOf(Number(k))?.name ?? k);
    const dropped = [...theirs].filter((k) => !mine.has(k)).map((k) => nodeOf(Number(k))?.name ?? k);
    const sides = Object.entries(c.choices)
      .filter(([id, side]) => best.choices[id] !== undefined && best.choices[id] !== side)
      .map(([id, side]) => nodeOf(Number(id))?.entries[side]?.name ?? id);
    return { added, dropped, sides };
  };

  // "+Incite Terror −Transfusion · Desecrate": the table's reason to exist.
  const summary = (line: number) => {
    const c = byLine.get(line);
    if (!c) return "";
    const d = differences(c);
    return [
      ...d.added.map((n) => `+${n}`),
      ...d.dropped.map((n) => `−${n}`),
      ...d.sides,
    ].join("  ");
  };

  const copyString = () => {
    const text = sim.strings[selected - 1];
    if (!text) return;
    void navigator.clipboard?.writeText(text).then(() => {
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    });
  };

  return (
    <div className="flex min-h-0 flex-1 flex-col gap-2 p-2 md:p-2.5">
      {/* The headline: what was simmed, what won, and how much the numbers can be trusted. */}
      <section className="panel flex flex-wrap items-center gap-x-6 gap-y-2 px-4 py-3">
        <div>
          <div className="label">Simmed</div>
          <div className="num text-[18px] text-ink">{formatCount(ranking.builds.length)} builds</div>
        </div>
        <div>
          <div className="label">Best · {ranking.metric}</div>
          <div className="num text-[18px]" style={{ color: "var(--star)" }}>
            {fmt(ranking.best.mean)}
            <span className="text-[12px] text-ink-faint"> ±{fmt(ranking.best.error)}</span>
          </div>
        </div>
        <div>
          <div className="label">Spread</div>
          <div className="num text-[18px] text-ink">{pct(ranking.spread).replace("+", "")}</div>
        </div>
        <div>
          <div className="label">Sim noise</div>
          <div className="num text-[18px] text-ink">±{(relError * 100).toFixed(1)}%</div>
        </div>
        {ranking.baseline !== null && ranking.baseline > 0 && (
          <div>
            <div className="label">Your profile</div>
            <div className="num text-[18px] text-ink">
              {fmt(ranking.baseline)}{" "}
              <span className="text-[12px] text-ink-faint">best is {pct(ranking.best.mean / ranking.baseline - 1)}</span>
            </div>
          </div>
        )}
        {(ranking.missing.length > 0 || ranking.unmatched.length > 0) && (
          <p className="text-[11.5px]" style={{ color: "var(--any-of)" }}>
            {ranking.missing.length > 0 && `${ranking.missing.length} builds missing from the report. `}
            {ranking.unmatched.length > 0 && `${ranking.unmatched.length} results from another export ignored.`}
          </p>
        )}
        <div className="ml-auto flex gap-1.5">
          <button type="button" className="btn" onClick={onAnother}>Load another report</button>
          <button type="button" className="btn" onClick={onBack}>← Narrow</button>
        </div>
      </section>

      <div className="flex min-h-0 flex-1 flex-col gap-2 md:flex-row md:gap-2.5">
        {/* The trees: the selected build, or the value of each talent. */}
        <div className="flex min-h-[28rem] flex-1 flex-col gap-2 md:min-h-0 md:flex-row md:gap-2.5">
          {trees.map((t) => {
            const part = selectedCharacter?.parts[t.key];
            // A tree every build agrees on has no heat to show, and drawing it bare read as
            // "every talent taken". It shows the build instead, which is the true answer.
            const treeHeat = heat.get(t.key);
            const showBuild = part && (tab === "builds" || !treeHeat || treeHeat.size === 0);
            const drawn = showBuild ? { ...part.points } : null;
            if (drawn) {
              for (const id of loadout.grantedRoots(t.tree)) {
                drawn[String(id)] = t.tree.nodes.find((n) => n.nodeId === id)?.maxPoints ?? 1;
              }
            }
            const sides = new Map<number, "a" | "b" | "none">(
              showBuild
                ? Object.entries(part.choices).map(([id, s]) => [Number(id), s === 1 ? "b" : "a"])
                : [],
            );
            return (
              <TreePane
                key={t.key}
                tree={t.tree}
                title={t.label}
                build={drawn}
                sides={sides}
                impacts={showBuild ? null : (treeHeat ?? null)}
                className={t.label === "Hero" ? "md:w-[18rem] md:shrink-0" : "flex-1"}
              />
            );
          })}
        </div>

        <aside className="flex w-full shrink-0 flex-col gap-2 md:w-[27rem] md:min-h-0">
          <div className="seg w-full" role="tablist">
            {([
              ["builds", `Builds`],
              ["talents", "Talent value"],
              ["choices", `Choice nodes (${choiceDuels.length})`],
            ] as const).map(([id, label]) => (
              <button
                key={id}
                type="button"
                role="tab"
                className="flex-1 !py-1.5 !text-[12px]"
                aria-pressed={tab === id}
                aria-selected={tab === id}
                onClick={() => setTab(id)}
              >
                {label}
              </button>
            ))}
          </div>

          {tab === "builds" && selectedCharacter && selectedRank && (
            <section className="panel p-3.5">
              <div className="flex items-baseline justify-between gap-2">
                <span className="label">
                  Build <span className="num normal-case tracking-normal">ttm_{String(selected).padStart(Math.max(2, String(sim.characters.length).length), "0")}</span>
                </span>
                <span className="num text-[11px] text-ink-faint">
                  #{selectedRank.rank + 1} of {formatCount(ranking.builds.length)}
                </span>
              </div>
              <div className="mt-1 num text-[20px] text-ink">
                {fmt(selectedRank.mean)}
                <span className="text-[12px] text-ink-faint"> ±{fmt(selectedRank.error)}</span>
                {selectedRank.rank > 0 && (
                  <span className="ml-2 text-[12px]" style={{ color: "var(--barred)" }}>
                    {pct(selectedRank.behind)} vs best
                  </span>
                )}
              </div>
              {selectedRank.rank > 0 && (() => {
                const d = differences(selectedCharacter);
                const any = d.added.length + d.dropped.length + d.sides.length;
                return any ? (
                  <p className="mt-1.5 text-[11.5px] leading-snug text-ink-soft">
                    Against the best: {d.added.length > 0 && <>takes <span className="text-ink">{d.added.join(", ")}</span>. </>}
                    {d.dropped.length > 0 && <>skips <span className="text-ink">{d.dropped.join(", ")}</span>. </>}
                    {d.sides.length > 0 && <>picks <span className="text-ink">{d.sides.join(", ")}</span>.</>}
                  </p>
                ) : null;
              })()}
              {selectedRank.rank > 0 &&
                ranking.best.mean - selectedRank.mean < ranking.best.error + selectedRank.error && (
                  <p className="mt-1 text-[11px]" style={{ color: "var(--any-of)" }}>
                    Within the error bars of the best — a tie, not a loss.
                  </p>
                )}
              <div className="mt-2.5 flex gap-1.5">
                <button type="button" className="btn flex-1" onClick={copyString}>
                  {copied ? "Copied" : "Copy talent string"}
                </button>
                <button
                  type="button"
                  className="btn flex-1"
                  onClick={() => onUse(selectedCharacter)}
                  title="Fix all three trees to this build and go back to narrowing"
                >
                  Use this build
                </button>
              </div>
            </section>
          )}

          <section className="panel min-h-0 flex-1 overflow-y-auto" style={{ maxHeight: "100%" }}>
            {tab === "builds" && (
              <>
                <table className="ttm-table">
                  <thead>
                    <tr>
                      <th>#</th>
                      <th className="!text-right">{ranking.metric.split(" ")[0] ?? "Mean"}</th>
                      <th className="!text-right">vs best</th>
                      <th>Differs from #1</th>
                    </tr>
                  </thead>
                  <tbody>
                    {ranking.builds.slice(0, shown).map((b) => (
                      <tr
                        key={b.line}
                        aria-selected={b.line === selected}
                        onClick={() => setSelected(b.line)}
                      >
                        <td className="num">{b.rank + 1}</td>
                        <td className="num !text-right whitespace-nowrap text-ink">
                          {fmt(b.mean)} <span className="text-ink-faint">±{fmt(b.error)}</span>
                        </td>
                        <td
                          className="num !text-right"
                          style={{
                            color:
                              b.rank === 0
                                ? "var(--star)"
                                : ranking.best.mean - b.mean < ranking.best.error + b.error
                                  ? "var(--ink-faint)"
                                  : "var(--ink-soft)",
                          }}
                        >
                          {b.rank === 0 ? "best" : pct(b.behind)}
                        </td>
                        <td className="max-w-[11rem] truncate text-[11px]" title={summary(b.line)}>
                          {b.rank === 0 ? "" : summary(b.line)}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                {shown < ranking.builds.length && (
                  <button type="button" className="btn m-2 w-[calc(100%-1rem)]" onClick={() => setShown(shown + PAGE * 5)}>
                    Show more ({formatCount(ranking.builds.length - shown)} left)
                  </button>
                )}
              </>
            )}

            {tab === "talents" && (
              <div className="p-3.5">
                <p className="text-[11.5px] leading-snug text-ink-soft">
                  Builds taking each talent against builds that don't. Faded rows are within sim
                  noise. Talents the trees never separate share a score.
                </p>
                {trees.map((t) => {
                  const rows = perTree.get(t.key) ?? [];
                  const widest = Math.max(1e-9, ...rows.map((r) => Math.abs(r.delta)));
                  return (
                    <div key={t.key} className="mt-3">
                      <div className="label !text-[10px]">{t.label}</div>
                      {rows.length === 0 ? (
                        <p className="mt-1 text-[11.5px] text-ink-faint">
                          Every build takes the same talents here.
                        </p>
                      ) : (
                        <ul className="mt-1 space-y-1">
                          {rows.map((r) => (
                            <ValueRow
                              key={r.key}
                              name={nodeOf(r.nodeId)?.name ?? String(r.nodeId)}
                              delta={r.delta}
                              scale={widest}
                              faded={Math.abs(r.delta) < noise(r.taken, ranking.builds.length - r.taken)}
                              detail={`${fmt(r.withIt)} with, ${fmt(r.without)} without · taken by ${r.taken}`}
                            />
                          ))}
                        </ul>
                      )}
                    </div>
                  );
                })}
              </div>
            )}

            {tab === "choices" && (
              <div className="p-3.5">
                <p className="text-[11.5px] leading-snug text-ink-soft">
                  Each choice node, left against right, over the builds that took it. Both sides
                  of every free choice node were simmed.
                </p>
                {choiceDuels.length === 0 ? (
                  <p className="mt-2 text-[11.5px] text-ink-faint">
                    No choice node was simmed on both sides — each was pinned, or not taken.
                  </p>
                ) : (
                  <ul className="mt-2 space-y-2.5">
                    {choiceDuels.map((d) => (
                      <DuelRow
                        key={d.nodeId}
                        duel={d}
                        names={nodeOf(d.nodeId)?.entries.map((e) => e.name) ?? ["left", "right"]}
                        faded={Math.abs(d.delta) < noise(d.left.n, d.right.n)}
                      />
                    ))}
                  </ul>
                )}
              </div>
            )}
          </section>
        </aside>
      </div>
    </div>
  );
}

function ValueRow({
  name,
  delta,
  scale,
  faded,
  detail,
}: {
  name: string;
  delta: number;
  scale: number;
  faded: boolean;
  detail: string;
}) {
  const width = Math.min(50, (Math.abs(delta) / scale) * 50);
  const good = delta >= 0;
  return (
    <li className="grid grid-cols-[1fr_6rem_3.6rem] items-center gap-2 text-[11.5px]" style={{ opacity: faded ? 0.45 : 1 }} title={detail}>
      <span className="truncate text-ink">{name}</span>
      <span className="vbar">
        <span
          style={{
            left: good ? "50%" : `${50 - width}%`,
            width: `${width}%`,
            background: good ? "var(--must)" : "var(--barred)",
          }}
        />
      </span>
      <span className="num text-right" style={{ color: good ? "var(--must)" : "var(--barred)" }}>
        {pct(delta)}
      </span>
    </li>
  );
}

function DuelRow({ duel, names, faded }: { duel: Duel; names: string[]; faded: boolean }) {
  const rightWins = duel.delta > 0;
  const side = (i: 0 | 1) => {
    const wins = (i === 1) === rightWins && !faded;
    const stat = i === 0 ? duel.left : duel.right;
    return (
      <div className="flex items-baseline gap-2">
        <span className={`truncate ${wins ? "text-ink" : "text-ink-soft"}`} style={{ fontWeight: wins ? 600 : 400 }}>
          {names[i]}
        </span>
        <span className="num ml-auto shrink-0 text-[10.5px] text-ink-faint">
          {Math.round(stat.mean).toLocaleString("en-US")} · {stat.n} builds
        </span>
      </div>
    );
  };
  return (
    <li
      className="grid grid-cols-[1fr_4.5rem] items-center gap-2 border-b border-[color-mix(in_srgb,var(--brass)_12%,transparent)] pb-2 text-[11.5px]"
      style={{ opacity: faded ? 0.55 : 1 }}
    >
      <div className="min-w-0">
        {side(0)}
        {side(1)}
      </div>
      <span
        className="num text-right text-[12px]"
        style={{ color: faded ? "var(--ink-faint)" : "var(--star)" }}
        title={faded ? "The difference is within sim noise" : undefined}
      >
        {faded ? "tie" : pct(Math.abs(duel.delta))}
      </span>
    </li>
  );
}
