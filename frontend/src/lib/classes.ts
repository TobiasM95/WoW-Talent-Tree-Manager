/**
 * Class identity as palette.
 *
 * A Druid tree and a Death Knight tree should not look the same, and the colour is already
 * known to the player -- it replaces a label. These are the game's published class colours:
 * functional identifiers rather than artwork, the same category as the talent data itself.
 *
 * Each class gets two values, because a single hex cannot serve both themes. Several class
 * colours are pale by design (Priest is near-white, Rogue and Monk are high-key) and vanish
 * on parchment; the `ink` variant is the same hue pushed down in luminosity until it reads
 * as pigment. Neither is used for text -- they tint framing and accents, where a 3:1 contrast
 * is the bar rather than 4.5:1.
 */
import type { CSSProperties } from "react";


export interface ClassColour {
  /** For dark grounds: the colour as emitted light. */
  glow: string;
  /** For light grounds: the same hue as pigment on parchment. */
  ink: string;
}

const PALETTE: Record<string, ClassColour> = {
  "Death Knight": { glow: "#c41e3a", ink: "#8f1528" },
  "Demon Hunter": { glow: "#a330c9", ink: "#73228c" },
  Druid: { glow: "#ff7c0a", ink: "#a34d00" },
  Evoker: { glow: "#33937f", ink: "#1f6154" },
  Hunter: { glow: "#aad372", ink: "#587038" },
  Mage: { glow: "#3fc7eb", ink: "#1f7791" },
  Monk: { glow: "#00ff98", ink: "#00795f" },
  Paladin: { glow: "#f48cba", ink: "#a8436d" },
  Priest: { glow: "#ffffff", ink: "#4a4a52" },
  Rogue: { glow: "#fff468", ink: "#7d6f00" },
  Shaman: { glow: "#0070dd", ink: "#004f9c" },
  Warlock: { glow: "#8788ee", ink: "#4a4bab" },
  Warrior: { glow: "#c69b6d", ink: "#7d5a33" },
};

const FALLBACK: ClassColour = { glow: "#a8823f", ink: "#6f5227" };

export const classColour = (className: string | null | undefined): ClassColour =>
  (className && PALETTE[className]) || FALLBACK;

/**
 * Sets the class tint as a CSS variable so styling can use it without every component
 * needing to know which theme is active. `--class-tint` resolves to the right one because
 * both are published and the stylesheet picks per theme.
 */
export const classTintStyle = (className: string | null | undefined) => {
  const { glow, ink } = classColour(className);
  return {
    "--class-glow": glow,
    "--class-ink": ink,
  } as CSSProperties;
};
