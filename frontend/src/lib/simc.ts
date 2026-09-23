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

/**
 * The talent strings for a set of enumerated builds.
 *
 * Deduplicated, because two different sets can encode to the same string: the enumerator
 * works in *selections* with choice-node sides unresolved, and a side this export has to
 * pick can collapse two of them together. Simming the same character twice is wasted time,
 * so they are merged and the count is reported.
 */
export interface BuiltStrings {
  strings: string[];
  collapsed: number;
  /**
   * The varying tree's points behind each line, parallel to `strings`.
   *
   * Kept because the export is only half the round trip: a report comes back naming
   * `ttm_07`, and turning that into "these talents scored this" needs to know which build
   * line 7 was. Duplicates collapse to one line, and they collapse precisely because they
   * are the same character, so the first of them stands for all.
   */
  points: Points[];
  /**
   * The 1-based line each *input* build ended up on, parallel to `builds`.
   *
   * Not the same as the line number, and that is the whole point of carrying it: the results
   * browser steps through the job's builds while a report names export lines, and the two
   * indices drift apart the moment two selections collapse onto one talent string. Without
   * this, build 40 would be shown the score of a different build.
   */
  lineOf: number[];
}

export function buildStrings(input: SimcInput): BuiltStrings {
  const { spec, trees, varying, base, choices, heroSubTreeId, builds } = input;
  const varyingIds = new Set(varying.nodes.map((n) => n.nodeId));

  // Everything the varying tree does not own, kept exactly as the player set it.
  const fixed: Points = {};
  for (const [id, points] of Object.entries(base)) {
    if (!varyingIds.has(Number(id))) fixed[id] = points;
  }

  const seen = new Map<string, number>();
  const strings: string[] = [];
  const points: Points[] = [];
  const lineOf: number[] = [];
  for (const build of builds) {
    const text = encode({
      spec,
      trees,
      points: { ...fixed, ...build },
      choices,
      heroSubTreeId,
    });
    const already = seen.get(text);
    if (already !== undefined) {
      lineOf.push(already);
      continue;
    }
    strings.push(text);
    points.push(build);
    seen.set(text, strings.length);
    lineOf.push(strings.length);
  }
  return { strings, collapsed: builds.length - strings.length, points, lineOf };
}

export interface ProfilesetOptions {
  /** Prefixed to each profileset name, so several exports can share one file. */
  label?: string;
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
 * report and the two views stop agreeing. It is also what lets the report be read back:
 * `ttm_07` is line 7 whatever order the results come in, and they do come back unordered.
 *
 * There is no "minimal profile" option any more. There was one, and it did not run: a
 * character with no weapon fails initialisation outright, and even past that, talents simmed
 * on a naked level-80 body produce a number that means nothing. SimulationCraft ships a
 * sample profile per specialisation, which is a real character -- so the file says to use
 * one rather than pretending to be one.
 */
export function profilesets(strings: string[], options: ProfilesetOptions): string {
  const { label = "ttm", className, specName, note } = options;
  const width = String(strings.length).length;
  const lines: string[] = [];

  lines.push(`# ${strings.length} talent builds from WoW Talent Tree Manager`);
  lines.push(`# ${className} - ${specName}`);
  if (note) lines.push(`# ${note}`);
  lines.push("#");
  lines.push("# Paste below your own profile: these lines change nothing but the talents,");
  lines.push("# so the gear, rotation and fight length stay yours.");
  lines.push("#");
  lines.push("# No profile of your own? SimulationCraft ships one per specialisation. From");
  lines.push("# the folder simc runs in:");
  lines.push(`#   simc profiles/<latest>/..._${className.replace(/ /g, "_")}_${specName.replace(/ /g, "_")}.simc this-file.simc`);
  lines.push("#");
  lines.push("# Then read the report back here: add json2=report.json to that command.");
  lines.push("");

  strings.forEach((text, i) => {
    lines.push(`profileset."${label}_${String(i + 1).padStart(width, "0")}"+=talents=${text}`);
  });
  return lines.join("\n") + "\n";
}
