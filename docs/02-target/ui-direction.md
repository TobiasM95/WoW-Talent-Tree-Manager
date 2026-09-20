# UI direction

Status: direction set by the owner, to be applied in Phase 3. Recorded now so it is not
quietly defaulted away when the UI work starts.

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

## A note on legal and visual sourcing

Do not lift Blizzard's UI assets — textures, frames, fonts — into the project. Build an
original look *informed by* that language. Talent icons are a separate matter: they are
game data the tool must display, sourced through the ingest, the same way every other
community tool does it.

## Open

- Whether to commit to a single dark world, or support a light theme too. A committed
  single look is legitimate for a page with this much atmosphere, but it needs to be a
  decision rather than an omission.
- Type. WoW's own faces (Friz Quadrata and successors) are not licensable here, so this
  needs a pairing that reads as fantasy without being a knock-off, and that stays
  readable at 12px in a tooltip.
