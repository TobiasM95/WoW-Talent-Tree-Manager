import { normalise, type Design } from "./design";
import { request, type TreeSummary } from "./api";

/**
 * The player's custom projects, kept in this browser.
 *
 * Two copies of a project matter and they are different things: the **draft** is what the
 * editor is showing, changed with every click; the **saved** version is an immutable,
 * content-addressed copy on the server, which is what the solver counts and a link points at.
 * Saving turns the current draft into a new saved version. Editing after that leaves the
 * saved version untouched until the next save, which is why the planner keeps working on the
 * last saved trees while the draft is mid-edit.
 */

export interface Project {
  /** Local identity; stable across saves, since each save has a new server id. */
  id: string;
  draft: Design;
  /** The server id of the last saved version, if there is one. */
  savedAs: string | null;
  /** The draft as of the last save, to tell whether there is anything unsaved. */
  savedDraft: string | null;
  updatedAt: number;
}

const KEY = "ttm.projects.v1";

export function readProjects(): Project[] {
  try {
    const raw = window.localStorage.getItem(KEY);
    const list = raw ? (JSON.parse(raw) as unknown) : [];
    return Array.isArray(list)
      ? (list as Project[])
          .filter((p) => p && typeof p.id === "string" && p.draft)
          .map((p) => ({ ...p, draft: normalise(p.draft) }))
      : [];
  } catch {
    return [];
  }
}

export function writeProjects(list: Project[]): boolean {
  try {
    window.localStorage.setItem(KEY, JSON.stringify(list));
    return true;
  } catch {
    return false;
  }
}

export const isDirty = (p: Project) => p.savedDraft !== JSON.stringify(p.draft);

export interface SavedProject {
  project: string;
  name: string;
  sharedPointCap: number | null;
  trees: TreeSummary[];
  warnings?: string[];
  design?: Design;
}

export const saveProject = (design: Design) =>
  request<SavedProject>("/custom-trees", { method: "POST", body: JSON.stringify(design) });

export const getProject = (id: string) => request<SavedProject>(`/custom-trees/${id}`);

/** The server id inside a custom tree's key: custom/<id>/<n>. */
export const projectOfKey = (key: string | null | undefined) =>
  key?.match(/^custom\/([0-9a-f]{16})\//)?.[1] ?? null;
