import type { TreeDetail } from "./api";
import type { Points } from "./loadout";

/**
 * Blizzard's talent loadout string: the thing you paste into the game, into SimulationCraft,
 * or into any other talent tool.
 *
 * A base64 bit stream over the *class trait tree's* node order:
 *
 *   version      8 bits   (2 today)
 *   specId      16 bits
 *   treeHash   128 bits
 *   then, per id in fullNodeOrder:
 *     selected             1 bit
 *     if selected:
 *       purchased          1 bit
 *       if purchased:
 *         partiallyRanked  1 bit
 *         if partial:      6 bits of ranks purchased
 *         isChoiceNode     1 bit
 *         if choice:       2 bits of choice index
 *
 * Bits pack least-significant-first *within* each 6-bit character, which is the part a
 * reimplementation usually gets backwards.
 *
 * **Confirmed against a string exported from the game** — a Feral Druid build with a full
 * spec tree, a half-spent class tree and a partly-spent hero tree. Two things the first
 * reading got wrong, neither of which a round trip could have caught, because encode and
 * decode shared the mistake:
 *
 * - **The `purchased` bit.** Selected and purchased are separate: a *granted* talent is
 *   selected without being purchased and reads no further bits. The real string had five —
 *   Rake, Rip, Swipe, Ravage, Thriving Growth — and without that bit the stream
 *   desynchronised almost immediately, producing a talent holding 24 of 1 points.
 * - **The hero-tree selector.** One id in the node order is not a talent at all but a
 *   chooser, written as a choice node whose index picks a sub-tree. Without it a string
 *   round-trips every talent and still loses which hero tree they belong to.
 *
 * The tree hash is written as zeroes, which is what the game's own string carries too.
 *
 * Decoding still validates hard. The header's version and spec id are what distinguish "a
 * real string for another character" from "this code has the layout wrong", and a rejected
 * paste is a far better outcome than a build that looks right and is not.
 */

const ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const VERSION = 2;
const HEADER_BITS = { version: 8, specId: 16, treeHash: 128 } as const;
const RANK_BITS = 6;
const CHOICE_BITS = 2;

export class LoadoutStringError extends Error {}

class Writer {
  private bits: number[] = [];

  write(value: number, width: number): void {
    for (let i = 0; i < width; i++) this.bits.push((value >> i) & 1);
  }

  toString(): string {
    let out = "";
    for (let i = 0; i < this.bits.length; i += 6) {
      let chunk = 0;
      for (let b = 0; b < 6; b++) chunk |= (this.bits[i + b] ?? 0) << b;
      out += ALPHABET[chunk];
    }
    return out;
  }
}

class Reader {
  private at = 0;
  private readonly bits: number[] = [];

  constructor(text: string) {
    for (const ch of text.trim()) {
      const value = ALPHABET.indexOf(ch);
      if (value < 0) throw new LoadoutStringError(`"${ch}" is not part of a loadout string.`);
      for (let b = 0; b < 6; b++) this.bits.push((value >> b) & 1);
    }
  }

  read(width: number): number {
    if (this.at + width > this.bits.length) {
      throw new LoadoutStringError("The string ends mid-talent — it looks truncated.");
    }
    let value = 0;
    for (let i = 0; i < width; i++) value |= (this.bits[this.at + i] ?? 0) << i;
    this.at += width;
    return value;
  }

  /** Bits left over. A handful is padding to the character boundary; more is a misread. */
  get remaining(): number {
    return this.bits.length - this.at;
  }
}

export interface EncodeInput {
  /** The spec tree, which carries `fullNodeOrder`, the spec id and the hero-tree selector. */
  spec: TreeDetail;
  /**
   * Every tree of this specialisation, **including both hero trees**.
   *
   * Both, because granted talents are written whether or not their tree was chosen: the real
   * string marks Thriving Growth granted while the build uses the other hero tree entirely.
   * Passing only the selected one drops it, and the string stops matching.
   */
  trees: TreeDetail[];
  /** nodeId -> points, across all of them. */
  points: Points;
  /** nodeId -> which alternative of a choice node, 0 or 1. */
  choices?: Record<string, number>;
  /** Which hero sub-tree the loadout uses, written through the selector node. */
  heroSubTreeId?: number | null;
}

function index(trees: TreeDetail[]) {
  const byId = new Map<number, TreeDetail["nodes"][number]>();
  for (const tree of trees) for (const node of tree.nodes) byId.set(node.nodeId, node);
  return byId;
}

export function encode({
  spec,
  trees,
  points,
  choices = {},
  heroSubTreeId = null,
}: EncodeInput): string {
  const order = spec.fullNodeOrder;
  if (!order || order.length === 0) {
    throw new LoadoutStringError(
      "This spec has no node order recorded, so a loadout string cannot be built.",
    );
  }
  const byId = index(trees);
  const selector = spec.subTreeSelector;
  const writer = new Writer();

  writer.write(VERSION, HEADER_BITS.version);
  writer.write(spec.specId ?? 0, HEADER_BITS.specId);
  for (let i = 0; i < HEADER_BITS.treeHash / 8; i++) writer.write(0, 8);

  for (const nodeId of order) {
    // The hero-tree chooser: not a talent, written as a choice whose index names a sub-tree.
    if (selector && nodeId === selector.nodeId) {
      const at = heroSubTreeId == null ? -1 : selector.subTreeIds.indexOf(heroSubTreeId);
      if (at < 0) {
        writer.write(0, 1);
      } else {
        writer.write(1, 1); // selected
        writer.write(1, 1); // purchased
        writer.write(0, 1); // fully ranked
        writer.write(1, 1); // a choice node
        writer.write(at, CHOICE_BITS);
      }
      continue;
    }

    const node = byId.get(nodeId);
    // Not in this specialisation's trees: it belongs to a sibling spec and still occupies
    // its place in the stream.
    if (!node) {
      writer.write(0, 1);
      continue;
    }
    // Granted: selected, never purchased, and no further bits.
    if (node.preFilled) {
      writer.write(1, 1);
      writer.write(0, 1);
      continue;
    }
    const spentHere = points[String(nodeId)] ?? 0;
    if (spentHere <= 0) {
      writer.write(0, 1);
      continue;
    }
    writer.write(1, 1);
    writer.write(1, 1);
    const partial = spentHere < node.maxPoints;
    writer.write(partial ? 1 : 0, 1);
    if (partial) writer.write(spentHere, RANK_BITS);
    const isChoice = node.kind === "choice";
    writer.write(isChoice ? 1 : 0, 1);
    if (isChoice) writer.write(choices[String(nodeId)] ?? 0, CHOICE_BITS);
  }
  return writer.toString();
}

export interface Decoded {
  specId: number;
  version: number;
  points: Points;
  choices: Record<string, number>;
  /** Which hero sub-tree the string names, read from the selector node. */
  heroSubTreeId: number | null;
  /** Granted talents the string marks. They cost nothing; listed for completeness. */
  granted: number[];
  /** Selected ids this specialisation's trees do not contain, which should be none. */
  unknown: number[];
}

export function decode(text: string, spec: TreeDetail, trees: TreeDetail[]): Decoded {
  const order = spec.fullNodeOrder;
  if (!order || order.length === 0) {
    throw new LoadoutStringError("This spec has no node order recorded to decode against.");
  }
  const cleaned = text.trim();
  if (!cleaned) throw new LoadoutStringError("Nothing to import.");

  const reader = new Reader(cleaned);
  const version = reader.read(HEADER_BITS.version);
  const specId = reader.read(HEADER_BITS.specId);
  for (let i = 0; i < HEADER_BITS.treeHash / 8; i++) reader.read(8);

  /*
    Checked before anything else is read. These are what distinguish "a real string for a
    different character" from "this code has the layout wrong": a misread layout almost never
    yields a plausible spec id.
  */
  if (version !== VERSION) {
    throw new LoadoutStringError(
      `This is a version ${version} loadout string; this tool reads version ${VERSION}.`,
    );
  }
  if (spec.specId != null && specId !== spec.specId) {
    throw new LoadoutStringError(
      `That string is for specialisation ${specId}, not ${spec.specName} (${spec.specId}).`,
    );
  }

  const byId = index(trees);
  const selector = spec.subTreeSelector;
  const points: Points = {};
  const choices: Record<string, number> = {};
  const granted: number[] = [];
  const unknown: number[] = [];
  let heroSubTreeId: number | null = null;

  for (const nodeId of order) {
    if (reader.read(1) === 0) continue;
    // Selected but not purchased: a granted talent. It costs no point and reads no further.
    if (reader.read(1) === 0) {
      granted.push(nodeId);
      continue;
    }
    const partial = reader.read(1) === 1;
    const ranks = partial ? reader.read(RANK_BITS) : 0;
    const isChoice = reader.read(1) === 1;
    const choice = isChoice ? reader.read(CHOICE_BITS) : 0;

    if (selector && nodeId === selector.nodeId) {
      heroSubTreeId = selector.subTreeIds[choice] ?? null;
      continue;
    }

    const node = byId.get(nodeId);
    if (!node) {
      // Selected, but not in this spec's trees. A few can be a sibling spec's nodes in a
      // shared tree; a flood of them means the stream is being read at the wrong offset.
      unknown.push(nodeId);
      continue;
    }
    const spentHere = partial ? ranks : node.maxPoints;
    if (spentHere < 1 || spentHere > node.maxPoints) {
      throw new LoadoutStringError(
        `${node.name} cannot hold ${spentHere} of ${node.maxPoints} points — ` +
          "the string does not match this talent tree.",
      );
    }
    points[String(nodeId)] = spentHere;
    if (isChoice) choices[String(nodeId)] = choice;
  }

  // Padding to the next character boundary is expected; five bits is the most that can be.
  if (reader.remaining > 5) {
    throw new LoadoutStringError(
      "The string is longer than this talent tree — it is probably for a different class.",
    );
  }
  if (unknown.length > order.length / 4) {
    throw new LoadoutStringError(
      "Most of that string points at talents this tree does not have.",
    );
  }
  return { specId, version, points, choices, heroSubTreeId, granted, unknown };
}
