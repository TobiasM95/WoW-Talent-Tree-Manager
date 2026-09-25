import { useState } from "react";
import { newId, readSaved, writeSaved, type SavedLoadout } from "../lib/saved";

/**
 * Save what is on screen under a name, and open it again later.
 *
 * Listed per specialisation, since a Blood save is noise while planning Frost. Deleting has
 * an undo rather than a confirmation dialog: a dialog interrupts every deliberate delete to
 * guard against the rare accidental one, and undo guards against the accident without
 * taxing the rest.
 */

export interface SavedPanelProps {
  /** The current state as a link's query string. */
  query: string;
  spec: string | null;
  revision: number | null;
  onOpen: (saved: SavedLoadout) => void;
}

const when = (t: number) =>
  new Date(t).toLocaleDateString(undefined, { day: "numeric", month: "short" });

export function SavedPanel({ query, spec, revision, onOpen }: SavedPanelProps) {
  const [list, setList] = useState<SavedLoadout[]>(() => readSaved());
  const [name, setName] = useState("");
  const [removed, setRemoved] = useState<SavedLoadout | null>(null);
  const [failed, setFailed] = useState(false);

  const commit = (next: SavedLoadout[]) => {
    setFailed(!writeSaved(next));
    setList(next);
  };

  const mine = list.filter((s) => s.spec === spec).sort((a, b) => b.savedAt - a.savedAt);
  const trimmed = name.trim();
  const existing = mine.find((s) => s.name === trimmed);

  const save = () => {
    if (!trimmed || !spec) return;
    const entry: SavedLoadout = {
      id: existing?.id ?? newId(),
      name: trimmed,
      query,
      spec,
      savedAt: Date.now(),
      revision,
    };
    // Same name replaces, so re-saving a build under its name updates it.
    commit([entry, ...list.filter((s) => s.id !== entry.id)]);
    setName("");
    setRemoved(null);
  };

  return (
    <section className="panel p-3.5">
      <span className="label">Saved</span>
      <form
        className="mt-2 flex gap-1.5"
        onSubmit={(event) => {
          event.preventDefault();
          save();
        }}
      >
        <input
          className="min-w-0 flex-1 rounded-[2px] px-2 py-1 text-[12px]"
          style={{
            background: "var(--panel-sunken)",
            border: "1px solid color-mix(in srgb, var(--brass) 30%, transparent)",
            color: "var(--ink)",
          }}
          value={name}
          onChange={(event) => setName(event.target.value)}
          placeholder="Name this setup"
          aria-label="Name for the saved setup"
          maxLength={60}
        />
        <button type="submit" className="btn" disabled={!trimmed || !spec}>
          {existing ? "Update" : "Save"}
        </button>
      </form>

      {mine.length > 0 && (
        <ul className="mt-2 space-y-0.5">
          {mine.map((s) => (
            <li key={s.id} className="group flex items-baseline gap-2 text-[12px]">
              <button
                type="button"
                className="truncate text-left text-ink hover:underline"
                onClick={() => onOpen(s)}
                title="Open this setup"
              >
                {s.name}
              </button>
              {revision !== null && s.revision !== null && s.revision !== revision && (
                <span
                  className="chip shrink-0 !text-[9.5px]"
                  style={{ color: "var(--any-of)" }}
                  title="Saved on an earlier data revision: talents may have changed since"
                >
                  older data
                </span>
              )}
              <span className="num ml-auto shrink-0 text-[10.5px] text-ink-faint">{when(s.savedAt)}</span>
              <button
                type="button"
                className="shrink-0 text-[11px] text-ink-faint hover:text-ink"
                onClick={() => {
                  setRemoved(s);
                  commit(list.filter((x) => x.id !== s.id));
                }}
                aria-label={`Delete ${s.name}`}
                title="Delete"
              >
                ×
              </button>
            </li>
          ))}
        </ul>
      )}

      {removed && (
        <p className="mt-2 text-[11px] text-ink-soft">
          Deleted “{removed.name}”.{" "}
          <button
            type="button"
            className="underline"
            onClick={() => {
              commit([removed, ...list]);
              setRemoved(null);
            }}
          >
            Undo
          </button>
        </p>
      )}

      <p className="mt-2 text-[10.5px] leading-snug text-ink-faint">
        {failed
          ? "This browser refused to store it — private window, or storage switched off. A copied link still works."
          : "Kept in this browser only. Copy the link to keep a setup anywhere else."}
      </p>
    </section>
  );
}
