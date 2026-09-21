# Frontend

React + Tailwind. One screen: pick a tree, paint constraints on it, watch the count, then
enumerate. That shape follows from the product model — the count is free and answered
inline, so it belongs beside the canvas being painted rather than behind a "calculate"
step. The number moving as constraints land *is* the feedback loop.

```bash
docker compose up -d postgres api worker      # the services it talks to
npm install
npm run dev                                   # http://localhost:5173

npm run typecheck
npm run shots                                 # both themes, desktop + phone
npm test                                      # drives the real interactions
```

Production is Caddy serving the built assets and proxying `/api`:

```bash
docker compose --profile web up -d --build web   # http://localhost:8080
```

## Design

The full brief is [`../docs/02-target/ui-direction.md`](../docs/02-target/ui-direction.md).
What it comes down to here:

**Two coherent themes, not one plus an inversion.** Dark is carved stone and worked bronze
lit from within; state reads as light emitted. Light is inked parchment and daylight on
stone; state reads as saturation and ink weight, because nothing glows in daylight. The
arcane accent keeps its hue across both and shifts luminosity instead, so the solver's
colour stays the solver's colour.

Tokens are defined in three layers in `styles/tokens.css`: bare `:root` (light), then
`prefers-color-scheme: dark` guarded by `:root:not([data-theme="light"])`, then an explicit
`:root[data-theme="dark"]`. The middle layer is the one that is easy to skip and the one
that matters — without it a visitor who has never touched the toggle and whose OS is dark
gets the light theme.

**Shape carries meaning.** Circle for a single talent, split square for a choice node,
hexagon for a tiered one, square for a hero sub-tree. The silhouette says what kind of node
it is before you read anything, which replaces a legend and frees colour to carry
constraint state instead.

**Nothing is lifted.** The framing, grain, ornament and node treatments are CSS gradients
and a generated SVG noise filter — no textures, no game UI art, no look-alike typefaces.
Cinzel (Roman inscriptional capitals) and Alegreya Sans, both SIL OFL. Talent icons are the
game data the tool exists to display; everything around them is ours.

## Structure

| | |
|---|---|
| `lib/api.ts` | Typed client. Every field checked against a live response. |
| `lib/constraints.ts` | The constraint set and the click-cycling rules. |
| `lib/theme.ts` | Theme choice, stamped on `<html data-theme>`. |
| `lib/classes.ts` | Class colours, one value per theme. |
| `components/TreeCanvas.tsx` | Pan, zoom, edges, node placement. |
| `components/TalentNode.tsx` | One talent; shape, icons, state. |
| `components/CountGate.tsx` | The pre-flight count and the listable verdict. |
| `components/JobPanel.tsx` | A running enumeration, per phase. |

## Things worth knowing before changing this

**The canvas fills its panel absolutely, and needs a ResizeObserver.** Every child of
`.ttm-canvas` is absolutely positioned, so its own auto height is zero and a percentage
height cannot resolve against a parent sized only by `min-height`. It measured as a 0px
viewport, the fit maths produced an off-screen offset, and the canvas came up blank at
phone width while looking fine on desktop — where the flex row happened to give it a
definite height. Both halves of that are fixed and both are load-bearing.

**Pan and zoom are one transform on one element.** A hundred absolutely-positioned nodes
re-laid-out per frame is exactly the "ornament costs interaction latency" failure the
design brief rules out.

**The count is debounced and superseded requests are discarded.** Dragging the point slider
fires a request per step; without the guard a slow early one lands last and shows a count
for a budget the user has already moved past.

**Groups are limited to one of each kind on purpose.** The engine's filter holds a single
value per talent, so *listing* supports one at-least-one group and one exactly-one group.
Offering more in the canvas would produce constraints the counter honours and the
enumerator silently drops.

**A missing icon is normal.** Upstream has no art for about 1% of names. Those render with
a hatched fill and the node still reads as a talent.

## Checking it

`npm run shots` captures both themes at desktop and phone width and **fails on any console
error** — a blank-looking panel and a thrown exception are indistinguishable in a still
image.

`npm test` drives the real thing against the real stack: clicking a talent must move the
count, clicking again must bar it, the tooltip must land on screen, the gate must refuse an
oversized listing, and a real enumeration must complete. Both scripts take a URL, so they
run against the dev server or the production container:

```bash
npm test http://localhost:8080
```

Two things that suite has to handle, which are properties of the system rather than
awkwardness:

- **Degenerate talents.** The top rows of a spec tree are in *every* build at a realistic
  budget and talents behind a point gate are in *none*, so "the number changed" passes for
  both while testing almost nothing. The test insists the narrowed count is non-zero and
  strictly smaller.
- **Deduplication.** An identical request is served from the previous job, instantly, with
  no phases to observe. That is the cache working, so the test asserts it rather than
  working around it.
