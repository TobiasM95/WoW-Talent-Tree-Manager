import { useEffect, useMemo, useState } from "react";
import {
  ApiError,
  getPopular,
  getPopularContent,
  type Popular,
  type PopularBuild,
  type PopularContent,
} from "../lib/api";

/**
 * What the top-ranked players of this spec run, from WarcraftLogs.
 *
 * Three uses, in the order a player reaches for them:
 *
 *   - **see it**: which hero tree the top players chose, and each talent's pick rate painted
 *     on the trees;
 *   - **take one**: the builds that recur, one click from being your loadout;
 *   - **narrow to the question**: require what nearly all of them take, bar what nearly none
 *     do, and leave only the contested talents open -- the part genuinely worth simming.
 *
 * The last is why this belongs in Narrow rather than beside it. The top players have already
 * settled most of a tree; the count that remains after their consensus is the real question.
 */

export interface PopularPanelProps {
  specKey: string;
  heroName: (key: string | null) => string;
  onUse: (build: PopularBuild) => void;
  onNarrow: (popular: Popular, agreement: number) => void;
  heat: boolean;
  onHeat: (popular: Popular | null) => void;
  /** Sim the distinct builds as they are, up to the sim limit. */
  onSimTop: (popular: Popular) => void;
  limit: number;
}

const pct = (n: number) => `${Math.round(n * 100)}%`;
const fmt = (n: number) => Math.round(n).toLocaleString("en-US");

export function PopularPanel({ specKey, heroName, onUse, onNarrow, heat, onHeat, onSimTop, limit }: PopularPanelProps) {
  const [content, setContent] = useState<PopularContent[] | null>(null);
  const [choice, setChoice] = useState<string>("");
  const [encounter, setEncounter] = useState("all");
  const [data, setData] = useState<Popular | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [agreement, setAgreement] = useState(0.9);

  useEffect(() => {
    void getPopularContent()
      .then((list) => {
        setContent(list);
        const raid = list.find((c) => c.kind === "raid") ?? list[0];
        if (raid) setChoice(`${raid.zoneId}:${raid.difficulties[0]?.id ?? 5}`);
      })
      .catch((exc: unknown) => setError(exc instanceof ApiError ? exc.detail : String(exc)));
  }, []);

  const [zoneId, difficulty] = choice.split(":").map(Number);
  const zone = content?.find((c) => c.zoneId === zoneId) ?? null;

  // A new spec or fight makes the old answer wrong, not just old.
  useEffect(() => {
    setData(null);
    onHeat(null);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [specKey, choice, encounter]);

  const load = () => {
    if (!zone) return;
    setLoading(true);
    setError(null);
    void getPopular(specKey, zone.zoneId, encounter, difficulty!)
      .then((d) => setData(d))
      .catch((exc: unknown) => setError(exc instanceof ApiError ? exc.detail : String(exc)))
      .finally(() => setLoading(false));
  };

  const contested = useMemo(() => {
    if (!data) return 0;
    return Object.values(data.pickRates).filter((r) => r.share > 1 - agreement && r.share < agreement).length;
  }, [data, agreement]);

  return (
    <section className="panel p-3.5" aria-label="Top players">
      <div className="flex items-baseline justify-between gap-2">
        <span className="label">Top players</span>
        <a
          className="text-[10.5px] text-ink-faint underline"
          href="https://www.warcraftlogs.com"
          target="_blank"
          rel="noreferrer"
        >
          WarcraftLogs
        </a>
      </div>

      {content && content.length > 0 && (
        <div className="mt-2 flex flex-col gap-1.5">
          <select
            className="field"
            value={choice}
            onChange={(event) => {
              setChoice(event.target.value);
              setEncounter("all");
            }}
            aria-label="Content"
          >
            {content.flatMap((c) =>
              c.difficulties.map((d) => (
                <option key={`${c.zoneId}:${d.id}`} value={`${c.zoneId}:${d.id}`}>
                  {c.kind === "dungeon" ? c.name : `${c.name} · ${d.name}`}
                </option>
              )),
            )}
          </select>
          <div className="flex gap-1.5">
            <select
              className="field min-w-0 flex-1"
              value={encounter}
              onChange={(event) => setEncounter(event.target.value)}
              aria-label="Encounter"
            >
              <option value="all">{zone?.kind === "dungeon" ? "All dungeons" : "All bosses"}</option>
              {zone?.encounters.map((e) => (
                <option key={e.id} value={String(e.id)}>
                  {e.name}
                </option>
              ))}
            </select>
            <button type="button" className="btn" onClick={load} disabled={loading || !zone}>
              {loading ? "Reading…" : data ? "Reload" : "Load"}
            </button>
          </div>
        </div>
      )}

      {error && (
        <p className="mt-2 text-[11.5px] leading-snug" style={{ color: "var(--barred)" }}>
          {error}
        </p>
      )}

      {data && (
        <>
          <p className="mt-2.5 text-[11.5px] text-ink-soft">
            <span className="num text-ink">{fmt(data.players)}</span> top players,{" "}
            <span className="num">{fmt(data.distinctBuilds)}</span> distinct builds.
          </p>

          {/* Hero trees: the first choice anyone makes, and the one with the clearest answer. */}
          <ul className="mt-2 space-y-1">
            {data.heroes.map((h) => (
              <li key={h.key ?? "none"} className="text-[11.5px]">
                <div className="flex items-baseline justify-between">
                  <span className="text-ink">{heroName(h.key)}</span>
                  <span className="num text-ink-faint">{pct(h.count / data.players)}</span>
                </div>
                <div className="vbar mt-0.5 !h-1">
                  <span style={{ left: 0, width: `${(h.count / data.players) * 100}%`, background: "var(--class-tint)" }} />
                </div>
              </li>
            ))}
          </ul>

          {/* The most direct question of all: of the builds that actually win, which is best on
              *your* character? Real loadouts, no combinatorics -- straight to the sim. */}
          <button type="button" className="btn btn-primary mt-2.5" onClick={() => onSimTop(data)}>
            Sim {Math.min(data.distinctBuilds, limit).toLocaleString("en-US")}
            {data.distinctBuilds > limit ? " most common" : ""} top builds →
          </button>

          <label className="mt-2.5 flex items-center gap-1.5 text-[11.5px] text-ink-soft">
            <input type="checkbox" checked={heat} onChange={(event) => onHeat(event.target.checked ? data : null)} />
            show pick rates on the trees
          </label>

          {/* Narrow to the question the top players have not settled. */}
          <div className="mt-3 border-t border-[color-mix(in_srgb,var(--brass)_15%,transparent)] pt-2.5">
            <div className="flex items-baseline justify-between text-[11.5px]">
              <span className="text-ink-soft">Settled when</span>
              <span className="num text-ink">{pct(agreement)} agree</span>
            </div>
            <input
              type="range"
              min={0.6}
              max={0.99}
              step={0.01}
              value={agreement}
              onChange={(event) => setAgreement(Number(event.target.value))}
              className="w-full"
              aria-label="Agreement needed to settle a talent"
            />
            <p className="text-[11px] leading-snug text-ink-faint">
              {contested} talent{contested === 1 ? " is" : "s are"} contested at this line. Narrowing
              requires the rest as the top players have them, on their most common hero tree.
            </p>
            <button type="button" className="btn mt-2 w-full" onClick={() => onNarrow(data, agreement)}>
              Narrow to the contested talents
            </button>
          </div>

          <div className="mt-3 border-t border-[color-mix(in_srgb,var(--brass)_15%,transparent)] pt-2.5">
            <span className="text-[11.5px] text-ink-soft">Builds that recur</span>
            <ul className="mt-1 space-y-1">
              {data.builds.slice(0, 6).map((b, i) => (
                <li key={i} className="flex items-baseline gap-2 text-[11.5px]">
                  <span className="num w-8 text-ink">{b.count}×</span>
                  <span className="truncate text-ink-faint" title={`e.g. ${b.example}`}>
                    {heroName(b.hero)} · best {fmt(b.best)}
                  </span>
                  <button type="button" className="ml-auto shrink-0 text-[11px] underline" onClick={() => onUse(b)}>
                    use
                  </button>
                </li>
              ))}
            </ul>
          </div>

          {data.illegal.length > 0 && (
            <p className="mt-2 text-[11px] leading-snug" style={{ color: "var(--any-of)" }} title={data.illegal.join("\n")}>
              {data.illegal.length} real build{data.illegal.length === 1 ? "" : "s"} break this tree data&apos;s
              rules — the talent data may be out of date.
            </p>
          )}
        </>
      )}
    </section>
  );
}
