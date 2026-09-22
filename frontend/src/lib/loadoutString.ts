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
 *     selected           1 bit
 *     if selected:
 *       partiallyRanked  1 bit
 *       if partial:      6 bits of ranks purchased
 *       isChoiceNode     1 bit
 *       if choice:       2 bits of choice index
 *
 * Bits are packed least-significant-first within each 6-bit character, which is the part a
 * reimplementation usually gets backwards.
 *
 * **This format is not verified against a string produced by the game.** It is the format
 * every community tool implements and the one SimulationCraft parses, but nothing here has
 * round-tripped a real one. So decoding validates hard rather than trusting: a wrong reading
 * of the layout produces a nonsense spec id, out-of-range ranks, or nodes this tree does not
 * have, and every one of those is refused with a reason. A rejected paste is a far better
 * outcome than a build that is quietly wrong, and pasting one real string is all it takes to
 * confirm the layout.
 *
 * The tree hash is written as zeroes. The payload carries no hash to copy, and the tools
 * that emit these strings do the same.
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

  /** Bits left over. A handful is padding; a lot means the layout was misread. */
  get remaining(): number {
    return this.bits.length - this.at;
  }
}

export interface EncodeInput {
  /** The spec tree, which carries `fullNodeOrder` and the spec id. */
  spec: TreeDetail;
  /** Every tree the loadout spans, so a node's ranks and kind can be looked up. */
  trees: TreeDetail[];
  /** nodeId -> points, across all of them. */
  points: Points;
  /** nodeId -> which alternative of a choice node, 0 or 1. */
  choices?: Record<string, number>;
}

function index(trees: TreeDetail[]) {
  const byId = new Map<number, TreeDetail["nodes"][number]>();
  for (const tree of trees) for (const node of tree.nodes) byId.set(node.nodeId, node);
  return byId;
}

export function encode({ spec, trees, points, choices = {} }: EncodeInput): string {
  const order = spec.fullNodeOrder;
  if (!order || order.length === 0) {
    throw new LoadoutStringError(
      "This spec has no node order recorded, so a loadout string cannot be built.",
    );
  }
  const byId = index(trees);
  const writer = new Writer();

  writer.write(VERSION, HEADER_BITS.version);
  writer.write(spec.specId ?? 0, HEADER_BITS.specId);
  // No hash is published with the talent data, and the tools that emit these strings write
  // zeroes here too.
  for (let i = 0; i < HEADER_BITS.treeHash / 8; i++) writer.write(0, 8);

  for (const nodeId of order) {
    const node = byId.get(nodeId);
    const spentHere = points[String(nodeId)] ?? 0;
    // A node that is not in this spec's trees always writes a zero bit: it belongs to a
    // sibling spec and still occupies its place in the stream.
    if (!node || spentHere <= 0) {
      writer.write(0, 1);
      continue;
    }
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
  /** Selected ids that this spec's trees do not contain, which should be none. */
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
    Checked before anything else is read, because they are what distinguishes "a real string
    for a different character" from "this code has the layout wrong". A misread layout
    almost never yields a plausible spec id.
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
  const points: Points = {};
  const choices: Record<string, number> = {};
  const unknown: number[] = [];

  for (const nodeId of order) {
    if (reader.read(1) === 0) continue;
    const partial = reader.read(1) === 1;
    const ranks = partial ? reader.read(RANK_BITS) : 0;
    const isChoice = reader.read(1) === 1;
    const choice = isChoice ? reader.read(CHOICE_BITS) : 0;

    const node = byId.get(nodeId);
    if (!node) {
      // Selected, but not in this spec's trees. One or two can be a sibling spec's node in a
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
  return { specId, version, points, choices, unknown };
}
