/**
 * Counting a class whose trees share one point pool, as WoW Forever's three tabs share 51.
 *
 * Counting each tab at a budget of its own and multiplying is wrong for a pool: no tab has a
 * budget of its own. A build is a split of the pool across the tabs -- 31/20/0 -- plus a
 * build of each tab at its share. So the class's count at P points is
 *
 *     sum over a + b + c = P of  arms(a) x fury(b) x protection(c)
 *
 * where each factor is that tab's count at exactly that many points, under whatever has been
 * painted on it. The server's DP yields a tab's count at every total in one run, so this is a
 * few thousand multiplications on the client, and exact: BigInt, since a class's count passes
 * 2^53 long before it passes anything a person would want to read.
 */

export interface Split {
  /** Points in each tab, in tab order. */
  split: number[];
  builds: number;
}

export interface PoolCount {
  builds: number;
  /** The splits with the most builds, most first. */
  top: Split[];
  /** How many distinct splits have any build at all. */
  splits: number;
}

/**
 * @param spreads  per tab: builds at exactly k points, k = 0.. (index 0 is the empty tab)
 * @param budget   points spent across all tabs
 * @param exact    per tab: points it must hold, or null for any
 */
export function poolCount(spreads: number[][], budget: number, exact: (number | null)[] = [], keep = 5): PoolCount {
  let total = 0n;
  let splits = 0;
  const top: { split: number[]; builds: bigint }[] = [];
  const at = (t: number, k: number) => (exact[t] !== null && exact[t] !== undefined && exact[t] !== k ? 0 : (spreads[t]![k] ?? 0));

  const walk = (t: number, left: number, split: number[], product: bigint) => {
    if (t === spreads.length - 1) {
      const n = at(t, left);
      if (!n) return;
      const builds = product * BigInt(n);
      total += builds;
      splits++;
      top.push({ split: [...split, left], builds });
      if (top.length > keep * 4) prune();
      return;
    }
    const most = Math.min(left, spreads[t]!.length - 1);
    for (let k = 0; k <= most; k++) {
      const n = at(t, k);
      if (n) walk(t + 1, left - k, [...split, k], product * BigInt(n));
    }
  };
  const prune = () => {
    top.sort((a, b) => (b.builds > a.builds ? 1 : b.builds < a.builds ? -1 : 0));
    top.length = Math.min(top.length, keep);
  };

  if (spreads.length) walk(0, budget, [], 1n);
  prune();
  return { builds: Number(total), top: top.map((s) => ({ split: s.split, builds: Number(s.builds) })), splits };
}

/** The most points the tabs can hold between them. */
export const poolSlots = (spreads: number[][]) => spreads.reduce((a, s) => a + Math.max(0, s.length - 1), 0);
