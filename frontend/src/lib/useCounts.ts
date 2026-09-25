import { useEffect, useRef, useState } from "react";
import { ApiError, countBuilds, type Constraints } from "./api";

/**
 * A live count per open tree.
 *
 * Each tree is counted on its own, because each has its own search, and debounced, because
 * a budget stepper or a burst of painting fires a request per click. A response that arrives
 * after a newer request for the same tree is dropped -- a slow early answer landing last would
 * show a count for a search the player has already moved past.
 *
 * A fixed tree is not counted: it contributes exactly one build by definition.
 */

export interface TreeCount {
  builds: number | null;
  sets: number | null;
  stale: boolean;
  error: string | null;
}

export interface CountRequest {
  key: string;
  /** Null for a fixed tree, or a search still being written (a group of one). */
  payload: Constraints | null;
}

const DEBOUNCE_MS = 160;

export function useCounts(requests: CountRequest[]): Record<string, TreeCount> {
  const [counts, setCounts] = useState<Record<string, TreeCount>>({});
  const latest = useRef<Record<string, number>>({});
  // One stable string per request set, so an effect re-runs when a search changes and not
  // when the array identity does.
  const signature = JSON.stringify(requests);

  useEffect(() => {
    const timers: ReturnType<typeof setTimeout>[] = [];
    for (const { key, payload } of requests) {
      if (!payload) continue;
      const mine = (latest.current[key] ?? 0) + 1;
      latest.current[key] = mine;
      setCounts((previous) => ({
        ...previous,
        [key]: { ...(previous[key] ?? { builds: null, sets: null, error: null }), stale: true },
      }));
      timers.push(
        setTimeout(() => {
          void countBuilds(key, payload)
            .then((result) => {
              if (latest.current[key] !== mine) return;
              setCounts((previous) => ({
                ...previous,
                [key]: { builds: result.builds, sets: result.sets, stale: false, error: null },
              }));
            })
            .catch((error: unknown) => {
              if (latest.current[key] !== mine) return;
              setCounts((previous) => ({
                ...previous,
                [key]: {
                  builds: null,
                  sets: null,
                  stale: false,
                  error: error instanceof ApiError ? error.detail : String(error),
                },
              }));
            });
        }, DEBOUNCE_MS),
      );
    }
    return () => timers.forEach(clearTimeout);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [signature]);

  return counts;
}
