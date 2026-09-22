import type { TreeDetail } from "./api";
import type { Points } from "./loadout";
import { encode } from "./loadoutString";

/**
 * SimulationCraft export.
 *
 * The output is **profilesets**: one line per build, each overriding nothing but the talent
 * string. That shape is deliberate. A talent comparison is only meaningful against real gear,
 * a real rotation and a real fight length, none of which this tool knows — so the export is
 * something to paste into the profile a player already sims with, rather than a profile that
 * pretends to be complete.
 *
 *   profileset."b0001"+=talents=CcGAAAAAAAA...
 *
 * A minimal header is offered separately, for someone who has no profile yet and wants
 * something that runs.
 *
 * **Each line is a whole character, not one tree.** The solver enumerates one tree at a time,
 * so every exported build is the enumerated tree's points combined with whatever the other
 * two hold. That is the same "hold these fixed, vary this one" model the counting gate works
 * in, and it is why a base loadout has to exist before an export means anything.
 */

export interface SimcInput {
  /** The spec tree, which carries the node order and spec id. */
  spec: TreeDetail;
  /** Every tree of the specialisation, including both hero trees. */
  trees: TreeDetail[];
  /** The tree being varied: its points are replaced per build. */
  varying: TreeDetail;
  /** Points in the other trees, held fixed across every line. */
  base: Points;
  /** Choice-node sides from the base loadout. */
  choices: Record<string, number>;
  /** Which hero sub-tree the loadout uses. */
  heroSubTreeId: number | null;
  /** One enumerated result per line: nodeId -> points within the varying tree. */
  builds: Points[];
}

/** SimC spells class and spec as lowercase tokens: "Death Knight" -> death_knight. */
export const token = (name: string): string =>
  name.toLowerCase().replace(/[^a-z0-9]+/g, "_").replace(/^_|_$/g, "");

/**
 * The talent strings for a set of enumerated builds.
 *
 * Deduplicated, because two different sets can encode to the same string: the enumerator
 * works in *selections* with choice-node sides unresolved, and a side this export has to
 * pick can collapse two of them together. Simming the same character twice is wasted time,
 * so they are merged and the count is reported.
 */
export function buildStrings(input: SimcInput): { strings: string[]; collapsed: number } {
  const { spec, trees, varying, base, choices, heroSubTreeId, builds } = input;
  const varyingIds = new Set(varying.nodes.map((n) => n.nodeId));

  // Everything the varying tree does not own, kept exactly as the player set it.
  const fixed: Points = {};
  for (const [id, points] of Object.entries(base)) {
    if (!varyingIds.has(Number(id))) fixed[id] = points;
  }

  const seen = new Set<string>();
  const strings: string[] = [];
  for (const build of builds) {
    const text = encode({
      spec,
      trees,
      points: { ...fixed, ...build },
      choices,
      heroSubTreeId,
    });
    if (seen.has(text)) continue;
    seen.add(text);
    strings.push(text);
  }
  return { strings, collapsed: builds.length - strings.length };
}

export interface ProfilesetOptions {
  /** Prefixed to each profileset name, so several exports can share one file. */
  label?: string;
  /** Include a minimal runnable profile above the profilesets. */
  withProfile?: boolean;
  className: string;
  specName: string;
  /** What the enumeration was, for the comment at the top. */
  note?: string;
}

/**
 * The text to paste into SimulationCraft.
 *
 * Names are zero-padded so they sort in the order they were enumerated, which is the order
 * the results browser shows them in -- otherwise "build 10" lands between 1 and 2 in SimC's
 * report and the two views stop agreeing.
 */
export function profilesets(strings: string[], options: ProfilesetOptions): string {
  const { label = "ttm", withProfile = false, className, specName, note } = options;
  const width = String(strings.length).length;
  const lines: string[] = [];

  lines.push(`# ${strings.length} talent builds from WoW Talent Tree Manager`);
  lines.push(`# ${className} — ${specName}`);
  if (note) lines.push(`# ${note}`);
  lines.push("#");
  lines.push("# Paste below your own profile: these lines change nothing but the talents,");
  lines.push("# so the gear, rotation and fight length stay yours.");
  lines.push("");

  if (withProfile) {
    // Enough to run, and no more. Anything else here would be a guess about a character
    // this tool has never seen.
    lines.push(`${token(className)}="TTM_${token(specName)}"`);
    lines.push("level=80");
    lines.push(`spec=${token(specName)}`);
    lines.push(`talents=${strings[0] ?? ""}`);
    lines.push("");
  }

  strings.forEach((text, i) => {
    lines.push(`profileset."${label}_${String(i + 1).padStart(width, "0")}"+=talents=${text}`);
  });
  return lines.join("\n") + "\n";
}
