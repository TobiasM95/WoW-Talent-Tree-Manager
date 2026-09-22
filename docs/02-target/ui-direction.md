# UI direction

Status: **applied** (2026-09-21). Direction set by the owner and now implemented in
[`../../frontend/`](../../frontend/); this document remains the brief the implementation is
answerable to. The one open item below, type, is resolved: Cinzel for display and Alegreya
Sans for everything that gets read, both SIL OFL.

## The brief

This is a tool for a fantasy game. The interface should feel like it belongs to that
world — distinctive and fun to use. It should not look like a generic web app that
happens to contain talent data.

## What to avoid

Explicitly rejected, because they are what a design defaults to when nobody decides:

- **"Modern SaaS"** — neutral greys, a blue-violet accent, evenly rounded cards, a soft
  shadow on everything, Inter or Space Grotesk.
- **Brutalist / raw-HTML** — hairline rules, monospace labels, hard black borders,
  deliberate starkness.
- **Editorial serif** — warm cream ground, a large display serif, a terracotta accent.
  Worth naming specifically: the frontier-sweep explainer in
  [`../explainers/frontier-sweep.html`](../explainers/frontier-sweep.html) is exactly
  this, and while it suits a technical document, it is **not** the product's direction.
- Emoji as iconography or section markers.

None of these are bad in themselves. They are wrong *here*, and they are what gets
produced by default.

## Where to draw from instead

WoW has a specific, well-developed visual language, and the talent tree screen has its
own within that. Source the design from it rather than from web convention:

- **Dark, materially-textured panels** rather than flat neutral surfaces. The in-game UI
  reads as carved stone, tooled leather and hammered metal, not as paper.
- **Metal framing.** Gold and bronze edging, corner ornament, beveled inner borders. The
  frame is part of the design, not a 1px line.
- **Node shapes carry meaning.** The game already encodes type in silhouette — circles,
  squares, octagons, split diamonds for choice nodes. Reuse that vocabulary; players
  read it instantly, and it replaces a legend.
- **Light as state.** A selected talent in WoW *glows*; an unaffordable one is dim and
  desaturated; a locked row reads as inert. Encode state in illumination rather than in
  border colour alone.
- **Class identity as palette.** Each class has an established colour. A Druid tree and a
  Death Knight tree should not look the same, and the colour is already known to the
  player.
- **Arcane/astral accents** for the solver, which is the app's distinctive feature.
  Counting the possibility space is a genuinely magical-feeling operation; it can look
  like one.

## Constraints that still apply

Distinctive is not the same as unusable. The tree canvas is a dense working surface, so:

- Text stays legible at small sizes against textured ground — ornament must not eat
  contrast. Check real contrast ratios, not vibes.
- Interactive affordances stay obvious. A node that can take a point must look like it.
- Ornament must not cost interaction latency on a ~100-node canvas with pan and zoom.
- It must work at phone width, where heavy framing is the first thing to break.
- Tooltips carry real content (per-rank descriptions, spell data) and are read constantly.
  They need to be *readable* first and decorative second.

## Vibe, not imitation — and no licensing exposure

The goal is the *feeling* of a fantasy game interface, reached with entirely original
material. Explicitly:

- **No Blizzard assets.** No lifted textures, frames, borders, cursors or UI art, and no
  attempt to recreate them closely enough to be mistaken for them.
- **No Blizzard typefaces.** Friz Quadrata and its successors are not licensable here, and
  a deliberate look-alike is the same problem wearing a different hat.
- **Nothing implying endorsement.** No Blizzard logos or wordmarks, and the tool says
  plainly that it is a fan project.
- **Talent icons are different**, and fine: they are game data the tool exists to display,
  fetched through the ingest, as every community tool does. Everything *around* them is
  ours.

So: draw on the genre, not the game. Carved stone, worked metal, arcane light and inked
parchment are the vocabulary of fantasy interfaces generally, not Blizzard's property. The
test is whether a stranger would call it "a fantasy app" rather than "a WoW screenshot".

## Both themes, and both properly

Dark and light are both required — not a dark design with the colours inverted.

The atmosphere has to survive the switch, which means designing two coherent worlds rather
than one plus a fallback:

- **Dark**: the obvious register. Deep grounds, luminous accents, state read as light
  emitted.
- **Light**: not "dark with a white background". The natural analogue is inked parchment
  and daylight on stone — warm ground, dark ink, accents that read as pigment and leaf
  rather than glow. State is read as saturation and weight instead of emission.

Both need the same contrast discipline over texture, and the accent must work on both
grounds — shift it toward analogous or drop saturation rather than swapping hue between
themes. Implementation follows the token pattern in the artifact design rules: a complete
palette on bare `:root`, redefined under `prefers-color-scheme: dark` and again under an
explicit `[data-theme]` stamp, so an un-stamped system-preference visitor gets a correct
theme too.

## The direction it landed on: star chart

The first implementation answered this brief literally — carved stone, bronze framing,
inscriptional capitals — and the owner's verdict was the useful one: *"a weird in-between of
not looking WoW-themed enough and just looking bad/outdated."* That is what happens when a
theme is applied uniformly. A bronze gradient border around every panel and a display face in
letterspaced small caps on every heading is a skin on a dashboard, not a designed surface.

The second answer is more specific, and specificity is what was missing. **A talent tree is a
constellation** — nodes and the lines between them — and this tool's whole job is counting
the stars in it. So: astral cartography. Fantasy through astronomy and divination rather than
through tavern woodgrain, which belongs to the genre rather than to any one game, and which
happens to describe exactly what the app does.

- **Dark is the void.** Near-black with a blue cast, a generated star field behind the tree,
  edges as thin lines of light and nodes as discs with a halo.
- **Light is the plate.** An engraved star atlas on laid paper: the same constellation as
  copper-plate linework, state read as ink weight, nothing glowing, because nothing glows on
  paper.
- **Brass in three places only** — the rules, the corner brackets that mark which tree the
  solver is aimed at, and the node pips. Used everywhere it is a skin; used sparingly it is a
  material.

### Layout: all three trees

A build is class plus spec plus hero, so choosing a spec talent with the class tree behind a
dropdown is choosing blind. All three are on screen, and class and spec are picked from a
rail of names — there are thirteen classes and three or four specs, a number you can simply
show, and it lets class colour do the work of a label.

The solver works one tree at a time, which is a property of the engine rather than of the
product, so that tree is marked rather than isolated.

### Type: resolved

- **Cormorant Garamond** for display, at large sizes and in normal case only. A
  plate-engraving face; letterspaced small caps would flatten it, which is the mistake the
  first attempt made.
- **IBM Plex Sans** for everything that gets read, legible at 11px, which is where most of
  this app's text lives.
- **IBM Plex Mono** for numbers. A count is a measurement and should read like an instrument
  — with one exception: the mono comma is centred in a full advance width, so display-size
  numbers use the body face with tabular figures instead.

All three SIL Open Font License, which does permit use in a web product — the distinction
the open item was pointing at.

## How it came out

See [`../../frontend/README.md`](../../frontend/README.md). The parts that answer this
brief most directly:

- Three-layer tokens, so an un-stamped visitor whose OS is dark gets the dark theme.
- Node silhouette carries type, which frees colour to carry constraint state.
- Framing, grain and ornament are CSS gradients and a generated noise filter. Nothing is
  lifted and nothing needs a licence.
- The screenshot script captures both themes at desktop and phone width and fails on a
  console error, because a design decision cannot be verified by a passing build.
