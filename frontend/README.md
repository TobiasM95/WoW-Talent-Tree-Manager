# Frontend

React + Tailwind. One screen, two modes, and one arc: count the possibility space,
narrow it to something simmable, then hand the survivors to SimulationCraft.

**Build** spends points by hand across all three of a specialisation's trees, under the
same rules the solver counts under. **Explore** paints constraints on the tree the solver
is pointed at, watches the count move as they land, enumerates the matches, and steps
through them on the tree itself. That shape follows from the
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
npm run test:loadout                          # spending rules, against the API
npm run test:string                           # the Blizzard talent-string codec
npm run test:simc                             # SimulationCraft export
npm test                                      # drives the real interactions
npm run test:all                              # all six
```

Production is Caddy serving the built assets and proxying `/api`:

```bash
docker compose --profile web up -d --build web   # http://localhost:8081
```

## Design

The full brief is [`../docs/02-target/ui-direction.md`](../docs/02-target/ui-direction.md).
What it comes down to here:

**Star chart.** A talent tree is a constellation, and this tool's job is counting the
stars in it -- so the interface is astral cartography rather than tavern woodgrain. That
belongs to the genre rather than to any one game, and it happens to describe exactly what
the app does.

Dark is **the void**: near-black with a blue cast, the tree drawn in light against a
generated star field, brass only where structure has to be read. Light is **the plate**:
an engraved star atlas on laid paper, where the same constellation is copper-plate
linework and nothing glows, because nothing glows on paper. Two coherent worlds, not one
plus an inversion.

This replaced a first attempt that put a bronze gradient border around every panel and a
display face in letterspaced small caps on every heading. Both ideas were fine once;
applied uniformly they read as a skin on a dashboard rather than as a designed surface.
Brass now appears in three places -- the rules, the corner brackets marking the active
tree, and the node pips -- and the display face only at large sizes in normal case.

Tokens are defined in three layers in `styles/tokens.css`: bare `:root` (light), then
`prefers-color-scheme: dark` guarded by `:root:not([data-theme="light"])`, then an explicit
`:root[data-theme="dark"]`. The middle layer is the one that is easy to skip and the one
that matters — without it a visitor who has never touched the toggle and whose OS is dark
gets the light theme.

**All three trees, always.** A build is class plus spec plus hero, so choosing a spec
talent with the class tree behind a dropdown is choosing blind. The solver works one tree
at a time -- a property of the engine, not of the product -- so that tree is *marked*
rather than isolated: a lit rule in the class colour, corner brackets, and the others
dimmed just enough to say "not this one" while staying readable. Clicking an inactive
tree points the solver at it rather than painting a constraint, and constraints are put
away per tree so going across and back does not lose them.

Class and spec are chosen from a rail of names, not a dropdown. There are thirteen
classes and three or four specs -- a number you can simply show -- and it lets class
colour do the work of a label.

**Shape carries meaning.** Circle for a single talent, split square for a choice node,
hexagon for a tiered one, square for a hero sub-tree. The silhouette says what kind of node
it is before you read anything, which replaces a legend and frees colour to carry
constraint state instead.

**Nothing is lifted.** The framing, grain, ornament and node treatments are CSS gradients
and a generated SVG noise filter — no textures, no game UI art, no look-alike typefaces.
Cormorant Garamond (a plate-engraving face, large sizes only), IBM Plex Sans for reading
and IBM Plex Mono for numbers -- a count is a measurement and should read like an
instrument. All three SIL OFL. Talent icons are the
game data the tool exists to display; everything around them is ours.

## Structure

| | |
|---|---|
| `lib/api.ts` | Typed client. Every field checked against a live response. |
| `lib/constraints.ts` | The constraint set and the click-cycling rules. |
| `lib/theme.ts` | Theme choice, stamped on `<html data-theme>`. |
| `components/SpecRail.tsx` | Class and specialisation selection. |
| `components/TreePane.tsx` | One tree, with its heading and active state. |
| `lib/share.ts` | The whole view, encoded into the URL. |
| `lib/loadout.ts` | Spending points by hand, under the solver's rules. |
| `lib/loadoutString.ts` | Blizzard's talent string, in and out. |
| `lib/simc.ts` | An enumerated result set as SimulationCraft profilesets. |
| `components/LoadoutString.tsx` | Paste a build in, copy one out. |
| `components/SimcExport.tsx` | Hand the matching builds to the sim. |
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

**The budget starts at the tree's cap and follows it until someone moves the slider.**
A player arriving at a talent tree is looking at a full build; a default below the cap
shows a tree half of which is unreachable and a count that is not the one they came for.
Once the slider is touched that is a deliberate choice and survives, including onto a
tree whose cap is smaller.

**Clear empties the canvas, not just the constraint set.** After an enumeration every
node carries the inspected build, which takes over its appearance — so resetting the
constraints alone looked like a button that did nothing.

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

**A hand-built loadout must be one the counter counts.** That is the property the
spending rules exist to hold, and `npm run test:loadout` asserts it by building real
loadouts on real trees and asking the API to count them. If the two ever disagree the
tool is telling a player they have made something it also says does not exist -- the kind
of quiet contradiction nobody notices until it has shipped for months. So the rules are
lifted from the counting DP rather than from a reading of the game:

1. total spent is below the tree's cap;
2. total spent is at least the node's `pointsRequired`, measured *before* this point;
3. the first point in a node needs a way in -- the node is a root, or some parent is
   **fully ranked**. Not merely taken: the DP attaches a node's children to its *last*
   rank, so a two-rank talent gates its children until both points are in it.

Placement is greedy, and exact because of it: taking a point never removes an option, so
if any order can place a point, placing what is available now cannot prevent it. That is
also what makes a refund cascade correctly -- re-place what remains and whatever no longer
has a way in simply does not land.

**The SimC export is profilesets, not a profile.** A talent comparison is only meaningful
against real gear, a real rotation and a real fight length, none of which this tool knows. So
the export is lines to paste into the profile a player already sims with:

```
profileset."ttm_001"+=talents=CoPAAAAAAAAA...
```

A minimal runnable profile is offered behind a checkbox, for someone who has none yet.

**Every line is a whole character.** The solver varies one tree at a time, so each exported
build is the enumerated tree's points combined with whatever the other two hold — the same
"hold these fixed, vary this one" model the counting gate works in. That is why the panel
refuses to export until a base loadout exists: without one it would produce a character with
two empty trees, which sims happily and means nothing.

Duplicates are collapsed. The enumerator works in *selections*, with choice-node sides
unresolved, so two different selections can encode to the same talent string once a side is
chosen — and simming the same character twice is wasted time.

**The caps are human, not technical.** The counting gate already refused anything unbounded;
50/200/1000 is what a person will actually wait for, since SimulationCraft runs each
profileset as a full simulation.

**The talent string is verified against one the game exported.** A Feral Druid build --
full spec tree, half-spent class tree, partly-spent hero tree -- decodes and re-encodes byte
for byte. That single string caught three faults a round trip never could, because encode and
decode shared the mistake each time:

- **A missing `purchased` bit.** Selected and purchased are separate: a *granted* talent is
  selected without being purchased and reads no further bits. The real string had five, and
  without that bit the stream desynchronised almost immediately -- a talent holding 24 of 1
  points.
- **The hero-tree selector.** One id in the node order is not a talent but a chooser, written
  as a choice node whose index picks a sub-tree. Without it a string round-trips every talent
  and still loses which hero tree they belong to.
- **Class trees are per specialisation.** Which talents are granted differs between them, so
  matching a class tree on class alone picked the wrong one -- which the app was doing too.

Importing still validates hard, because a string this tool cannot read must be refused rather
than turned into a build that looks right and is not. A wrong specialisation, another class, a
newer format version, a truncated string, or ranks that do not fit are each rejected with a
message naming the problem. The bit order is worth stating because it is what a
reimplementation gets backwards: bits pack least-significant-first *within* each 6-bit
character.

**Granted talents are free, and that has consequences everywhere.** The DP removes them from
the graph entirely, so no build spends a point on one; the API refuses a constraint naming one
rather than silently dropping it; and the spending rules will not let a point go into one. All
three had to agree, and they did not before this.

**`fullNodeOrder` belongs to the class, not the tree.** 206 entries for a Death Knight
against 114 nodes across its three trees, because it also contains the other specs'
nodes, and it is identical for every spec of a class. A string emits one entry per id in
that list, selected or not, so it cannot be rebuilt from the split trees -- the nodes
that are not ours still occupy their place in the stream. It is carried on the spec tree,
because that is what the string's header identifies.

**A capstone has no edges.** Every retail spec tree ends in a talent with no parents
and no children, opened purely by the final point gate. The spending rules reach it
because rule 3 exempts a node with no parents, not because anything special was written
for it — but it is the shape most likely to be handled by accident, so
`npm run test:loadout` checks that it is closed before its gate and open on the gate
alone. See [`../docs/02-target/raidbots-live-schema.md`](../docs/02-target/raidbots-live-schema.md).

**Refunds cascade rather than refusing.** Taking a point out of the middle of a tree can
strand everything below it, and a tool that answers "no" leaves the player to work out
which of thirty talents is the obstacle. Re-placing what remains says instead: here is
what your build becomes.

**Statistics and a single build cannot share the canvas.** They answer different questions
— "what do all of these have in common" and "what does this one do" — and drawing both would
make neither legible. One switch moves between them: turning the heat map on releases the
selected build, and picking a build turns the heat map off. Without that, stepping into a
build was a one-way door whose only exit also discarded the constraints.

**Build and explore cannot share the canvas either**, and they cannot share an attribute.
`spent` means what the player has taken in one and what a finished build contains in the
other, and the treatments are opposites -- an untaken talent you could still take must
stay inviting, one absent from a finished build should recede. They collided once, which
made a tree with no points in it nearly invisible.

**The URL is the state, and it uses node ids.** The tree, the budget, every constraint and
the build being inspected all live in the query string, so a link reproduces the screen
rather than the front page — 124 characters for a 27-point build with nine constraints. Ids
are base36 for length, never positional: the legacy format stored points positionally and a
regeneration silently reassigned them to different talents, and a link outlives more
revisions than a database row does. The URL is *replaced* rather than pushed, because
painting constraints is a dozen clicks and nobody thinks of it as navigation.

It carries the loadout for all three trees, which hero tree is showing, and **the mode**.
The mode is carried rather than inferred: a link can hold both a hand-built loadout and an
enumerated build, and guessing from which fields are present produced a link that reopened
showing neither.

**Trees are scaled by grid pitch, not overall width.** Normalising total width made every
tree the same number of pixels across regardless of its column count, which stretched a
four-column hero tree to the width of a seven-column class tree and then shrank it to
nothing in a narrow pane. Scaling from the smallest gap between adjacent columns gives
every tree the same node *density*, so a hero tree is simply narrower -- which is what it
is.

**Big numbers use the body face, small ones the mono face.** IBM Plex Mono centres its
comma in a full advance width, so at display size "50,944" reads as "50 , 944":
correct monospacing, wrong typography.

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
