import { isDirty, type Project } from "../lib/projects";

/**
 * The custom game's answer to the class rail: the player's own projects.
 *
 * Kept in this browser. Removing one forgets it here; its saved versions stay on the server,
 * since they are immutable and a link someone was sent may still point at them.
 */

export interface ProjectRailProps {
  projects: Project[];
  current: string | null;
  onSelect: (id: string) => void;
  onNew: () => void;
  onRemove: (id: string) => void;
}

export function ProjectRail({ projects, current, onSelect, onNew, onRemove }: ProjectRailProps) {
  return (
    <div className="flex min-w-0 flex-wrap items-center gap-x-0.5 gap-y-0.5" role="group" aria-label="Projects">
      {projects.map((p) => (
        <span key={p.id} className="group relative inline-flex items-center">
          <button
            type="button"
            className="rail-item"
            aria-pressed={p.id === current}
            onClick={() => onSelect(p.id)}
            title={isDirty(p) ? "Has unsaved changes" : p.savedAs ? "Saved" : "Not saved yet"}
          >
            {p.draft.name || "Untitled"}
            {isDirty(p) && <span className="ml-0.5 text-[10px]" style={{ color: "var(--any-of)" }}>•</span>}
          </button>
          {p.id === current && projects.length > 1 && (
            <button
              type="button"
              className="px-0.5 text-[11px] text-ink-faint hover:text-ink"
              onClick={() => onRemove(p.id)}
              aria-label={`Forget ${p.draft.name} in this browser`}
              title="Forget this project in this browser (saved versions stay reachable by link)"
            >
              ×
            </button>
          )}
        </span>
      ))}
      <button type="button" className="rail-item" onClick={onNew}>
        + New project
      </button>
    </div>
  );
}
