# Frontend

React + Tailwind. One screen: pick a tree, paint constraints on it, watch the count,
enumerate, then step through the builds on the tree itself. That shape follows from the
product model — the count is free and answered inline, so it belongs beside the canvas
being painted rather than behind a "calculate" step. The number moving as constraints land
*is* the feedback loop.

```bash
docker compose up -d postgres api worker      # the services it talks to
npm install
npm run dev                                   # http://localhost:5173

npm run typecheck
npm run shots                                 # both themes, desktop + phone
npm run test:share                            # the share codec, no browser
npm test                                      # drives the real interactions
npm run test:all                              # all three
```

Production is Caddy serving the built assets and proxying `/api`:

```bash
docker compose --profile web up -d --build web   # http://localhost:8081
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
| `lib/share.ts` | The whole view, encoded into the URL. |
| `lib/classes.ts` | Class colours, one value per theme. |
| `components/TreeCanvas.tsx` | Pan, zoom, edges, node placement. |
| `components/TalentNode.tsx` | One talent; shape, icons, state. |
| `components/CountGate.tsx` | The pre-flight count and the listable verdict. |
| `components/JobPanel.tsx` | A running enumeration, per phase. |
| `components/ResultsBrowser.tsx` | A cursor over the enumerated builds. |
| `components/ShareButton.tsx` | Copy the current link, with a fallback. |
| `components/StatsPanel.tsx` | What every matching build shares, and where the choice is. |

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

**The results browser is a cursor, not a list.** A build is twenty talents; twenty rows of
numbers tell nobody anything. Selecting a build paints it onto the canvas — taken talents
lit with their point counts, everything else receding — and arrow keys step through them, so
moving between builds animates the difference. That is the question a person actually has
after asking for every build matching their constraints. Pages of 100, because "every
matching build" reaches two million.

**Statistics and a single build cannot share the canvas.** They answer different questions
— "what do all of these have in common" and "what does this one do" — and drawing both would
make neither legible. One switch moves between them: turning the heat map on releases the
selected build, and picking a build turns the heat map off. Without that, stepping into a
build was a one-way door whose only exit also discarded the constraints.

**The URL is the state, and it uses node ids.** The tree, the budget, every constraint and
the build being inspected all live in the query string, so a link reproduces the screen
rather than the front page — 124 characters for a 27-point build with nine constraints. Ids
are base36 for length, never positional: the legacy format stored points positionally and a
regeneration silently reassigned them to different talents, and a link outlives more
revisions than a database row does. The URL is *replaced* rather than pushed, because
painting constraints is a dozen clicks and nobody thinks of it as navigation.

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
npm test http://localhost:8081
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
- **Every build spends the whole budget.** So the *number* of lit talents is not a signal
  that stepping to the next build did anything — the test compares which talents are lit.

Share links get a suite of their own (`npm run test:share`) because that is where a mistake
is silent: a link that loses a constraint still opens, shows a plausible tree and gives the
wrong answer, with nothing for a person to notice. It round-trips every field, checks that
multi-rank point counts are not flattened to 1, and feeds in mangled query strings — links
get truncated and hand-edited in chat clients, and garbage has to produce an empty view
rather than a wrong one. `npm test` then opens a real link in a fresh page and checks the
same talents light up.
