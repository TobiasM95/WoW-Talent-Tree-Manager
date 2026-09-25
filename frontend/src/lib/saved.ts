/**
 * Named loadouts, kept in this browser.
 *
 * A saved entry is a share link with a name on it. The link already carries everything --
 * every tree fixed or open, its build, its search, the hero trees, the sim limit -- and the
 * share suite already proves it round-trips, so saving is copying a string rather than
 * inventing a second format that could drift from the first.
 *
 * It lives in localStorage, which means it is per browser and can vanish (private windows,
 * cleared site data). That is the agreed trade: no accounts, and nothing here is precious that
 * a copied link could not also keep. Every access is guarded, because storage can throw.
 */

export interface SavedLoadout {
  id: string;
  name: string;
  /** The query string a share link would carry, `?t=...`. */
  query: string;
  /** The spec tree it was made for, to list saves under the spec on screen. */
  spec: string;
  savedAt: number;
  /** Data revision at save time: a later one means the talents may have changed since. */
  revision: number | null;
}

const KEY = "ttm.saved.v1";

export function readSaved(): SavedLoadout[] {
  try {
    const raw = window.localStorage.getItem(KEY);
    const list = raw ? (JSON.parse(raw) as unknown) : [];
    return Array.isArray(list)
      ? list.filter(
          (x): x is SavedLoadout =>
            typeof x === "object" && x !== null && typeof x.id === "string" && typeof x.query === "string",
        )
      : [];
  } catch {
    return [];
  }
}

/** False when the browser refused -- full storage, or storage switched off. */
export function writeSaved(list: SavedLoadout[]): boolean {
  try {
    window.localStorage.setItem(KEY, JSON.stringify(list));
    return true;
  } catch {
    return false;
  }
}

export const newId = () =>
  typeof crypto !== "undefined" && "randomUUID" in crypto
    ? crypto.randomUUID()
    : `${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
