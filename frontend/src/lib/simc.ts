import type { TreeDetail } from "./api";
import { encode } from "./loadoutString";
import { lineName, type Character } from "./space";

/**
 * SimulationCraft export.
 *
 * The output is **profilesets**: one line per character, each overriding nothing but the
 * talent string. That shape is deliberate. A talent comparison is only meaningful against
 * real gear, a real rotation and a real fight length, none of which this tool knows -- so the
 * export is something to put under a profile, rather than a profile that pretends to be one.
 *
 *   profileset."ttm_0042"+=talents=CoPAAAAAAAAA...
 *
 * **Every line is a whole character**: one build from each tree, with a side chosen for every
 * choice node. Nothing has to be supplied from outside the search. A tree the player fixed
 * contributes its one build to every line; an open tree contributes each of its matches.
 */

export interface ExportInput {
  /** The spec tree, which carries the node order and spec id. */
  spec: TreeDetail;
  /** Every tree of the specialisation, including both hero trees. */
  trees: TreeDetail[];
  heroSubTreeId: number | null;
  characters: Character[];
}

/** One talent string per character, in character order. */
export function talentStrings({ spec, trees, heroSubTreeId, characters }: ExportInput): string[] {
  return characters.map((c) =>
    encode({ spec, trees, points: c.points, choices: c.choices, heroSubTreeId }),
  );
}

export interface ProfilesetOptions {
  className: string;
  specName: string;
  /** What was searched, for the comment at the top. */
  note?: string;
}

/**
 * The text to hand to SimulationCraft.
 *
 * Names are zero-padded so they sort in the order they were produced, and that order is what
 * lets a report be read back: `ttm_0042` is character 42 whatever order SimC lists its results
 * in -- and it does list them unordered.
 *
 * There is no "minimal runnable profile". There was one, and it did not run: SimC does not
 * know `death_knight=` (it wants `deathknight=`), and past that a character with no weapon
 * fails initialisation. SimulationCraft ships a real character per specialisation, so the
 * header says to use one of those.
 */
export function profilesets(strings: string[], options: ProfilesetOptions): string {
  const { className, specName, note } = options;
  const width = Math.max(2, String(strings.length).length);
  const file = `${className.replace(/ /g, "_")}_${specName.replace(/ /g, "_")}`;
  const lines: string[] = [];

  lines.push(`# ${strings.length} talent builds from WoW Talent Tree Manager`);
  lines.push(`# ${className} - ${specName}`);
  if (note) lines.push(`# ${note}`);
  lines.push("#");
  lines.push("# These lines change nothing but the talents, so put them below a character:");
  lines.push("# your own profile, or one of the sample profiles SimulationCraft ships.");
  lines.push("#");
  lines.push("#   simc your-profile.simc this-file.simc json2=report.json");
  lines.push(`#   simc profiles/<latest>/..._${file}.simc this-file.simc json2=report.json`);
  lines.push("#");
  lines.push("# Then drop report.json back onto the page to see what each talent was worth.");
  lines.push("");

  strings.forEach((text, i) => {
    lines.push(`profileset."${lineName(i + 1, width)}"+=talents=${text}`);
  });
  return lines.join("\n") + "\n";
}
