# Frontend

React + Tailwind. Three steps, in the order a player does them:

1. **Narrow** — each of the three trees is **fixed** (you spend its points: one build) or
   **open** (you paint what you want: every build that matches). The space of whole
   characters is the product of the three, shown as the product it is, and it is simmable
   once it fits the sim limit — 10,000 builds by default, choice sides included.
2. **Simulate** — the builds are listed on arrival, downloaded as SimulationCraft
   profilesets, run under a real character, and the report is dropped back on the page.
   Beside that, every build can be flicked through on the three trees — arrow keys step,
   shift+arrow jumps ten — with what changed from the build before spelled out.
3. **Analyse** — a page, not a panel: the ranking, what each talent was worth per tree, and
   each choice node's two sides head to head, with the three trees drawn beside it.

The typical path: paste the talent string you play, which fixes all three trees; open the
one you want to explore; paint until the count fits; simulate; read the answer; "use this
build" to fix the trees to it and go round again.

**Ranks.** A multi-rank talent is painted a rank at a time. *Require / bar* steps its
minimum -- at least 1, at least 2, ... maxed, then barred -- and *At most* lowers its cap, so
"exactly 2 of 3" is two clicks and one, and a one-point dip is a cap of 1 on a free talent. The
ring shows it: one arc per rank, green up to the minimum, red past the cap, with a badge (`2+`,
`=2`, `≤1`, `1–2`). In the planner the same arcs fill gold as ranks are spent, and the arrows a
build runs along turn gold, the ones it could take next green. The counter and the engine both
already treat each rank as its own step, so a limit costs nothing to count and reaches the
listed builds -- the API suite checks the two agree on every kind of rank limit, and that the
exact-rank counts of a talent add up to all its builds. `test:ranks` paints them in the browser
against counts asked of the API directly.

**Top players.** In retail's Narrow, a panel reads what the top-ranked players of the spec
run, from WarcraftLogs -- a raid on Mythic or Heroic, one boss or all, or a Mythic+ season. It
shows the hero-tree split and each talent's pick rate on the trees, and offers three things to
do with them: **use** a build that recurs; **narrow to the contested talents** (require what
nearly all of them take, bar what nearly none do, pin the choice sides they agree on -- leaving
only the part they still disagree about); or **sim the top builds** as they are, straight into
SimulationCraft and the analysis -- "of the builds that actually win, which is best on *my*
character?" `test:popular` runs that last one for real: 525 distinct top Blood builds, every one
accepted by SimulationCraft.

**WoW Forever.** The header switches game. Forever's trees are a class's three tabs sharing
one pool of 51 points, so the class is **one build**, not three: it is fixed or open as a whole,
from the Talent points card, never tab by tab.

- **Fixed build** is a talent calculator, and where a class starts: every tab spends from the
  one pool, the readout is the split ("31 / 20 / 0") with the level it takes.
- **Open search** is one search over all three tabs. Its count is a sum over every split of the
  points to spend -- arms(a) x fury(b) x protection(c) for every a + b + c = 51 -- each tab
  counted under whatever is painted on it. A tab can be held to **exactly** so many points
  ("31 in Arms"), the points to spend can be lowered for a levelling build, and the card lists
  the splits holding the most builds. Per-tab counts are gone: alone, they mean nothing.

The first version gave each tab a slice of the pool in tab order -- Arms took 47, Fury 4,
Protection none -- and counted each alone, which is where a count of 1 or an error came from.
`test:forever` now checks the pooled count against a sum it works out itself from each tab's
per-points counts, at 51, at 49, with a tab held to 31 and with a talent required. Simulate is
off -- SimulationCraft does not sim Forever -- and the data is credited in the footer, as its
licence asks.

**The tree editor.** The third game, Custom, is your own trees: a project of one to three,
each with its own budget or all sharing one pool -- shaped like retail or like Forever. Design
on a grid (click an empty cell to add a talent, drag to move, Connect or shift-click to make one
require another), then Plan with it: the same planner, counter and share links as the real
games. "Edit a copy in the tree editor" brings any retail or Forever trees in, for the what-if
nobody else can answer -- and a copy is checked to count *exactly* like its original.

A project has one style, as the games do. **Retail style**: one class tree, one to four spec
trees and up to six hero trees, each with its own budget, on wide grids with free connections,
gated by **barriers** -- dashed lines between rows, crossed once enough is spent (a blank tree
starts with 8 and 20; + in the margin adds one, its number edits it, × removes it). Each hero
tree says which specs take it, and planning one works as retail does: pick a spec in the rail,
get its class, spec and hero trees. **Classic style**: one to three tabs of four columns, each
row opening a fixed number of points after the last, optionally sharing one pool -- Forever's
shape. Any talent can still override its own gate, and any can be **granted**: free and always
taken, like retail's starting talents.

A new project starts blank in either style or as a copy: a Forever class's tabs, or a retail
class with any of its specs and their hero trees (each hero tree once, taken by the specs it
had). "+ spec", "+ hero" or "+ tree" add one of a kind to a project, blank or copied, and only
ever of the project's own style. A retail copy keeps its gates as barriers and its granted
talents granted; `test:editor` checks a Blood + Frost copy counts exactly like the originals --
class, both specs and a hero tree, at every budget tried. One class tree serves every spec, so a
second spec plans on the first spec's class tree, whose granted talents can differ slightly.

Edits that could never be saved are refused as they are made (a requirement that would loop is
not drawn), and everything is undoable. Drafts and the project list live in this browser; a
saved version lives on the server, because the solver has to read it -- immutable and keyed by
the hash of the design, so a link to it always means the tree it was made from, and nothing is
tied to a person.

**Both hero trees at once.** The hero pane's `both` switch sims the other hero tree too, each
with its own fixed build or search — the hero factor becomes a sum, class × spec × (A + B),
and the names choose which one is being edited. The analysis then compares the trees by best
and by mean, and says plainly when they were simmed at different budgets, since that compares
the extra points rather than the trees.

### Why it is shaped like this

It was not, twice, and both failures are worth knowing:

- **One tree searched at a time, plus a "baseline".** The solver works one tree at a time,
  and I let that engine detail become the product: exporting a set of spec builds required
  hand-building a class and hero tree first, in another mode. But the trees have separate
  point budgets, so the character space is simply their product — a fixed tree is a factor
  of one. No baseline is needed, and every exported line is a whole character.
- **Two global modes, Build and Explore, owning disjoint state.** Switching mode made
  everything painted in the other one vanish from the screen. Now each tree keeps its fixed
  build *and* its search whichever it is using, so flipping a tree loses nothing, and a link
  carries both halves of all three trees.

**Choice sides are expanded, not guessed.** The solver enumerates *selections*, saying a
choice node is taken but not which side. A selection with k free choice nodes is 2^k builds,
and the API's `builds` count already counts them that way — `test:simc` checks the client's
expansion equals it exactly, pinned sides included. The first export simmed only the
selections, so one side of every choice node was quietly never simmed.

```bash
docker compose up -d postgres api worker      # the services it talks to

# pnpm only, at the version package.json pins. A `preinstall` guard refuses npm and yarn
# rather than letting a second lockfile appear.
corepack enable
pnpm install
pnpm run dev                                   # http://localhost:5173

pnpm run typecheck
pnpm run shots                                 # both themes, desktop + phone
pnpm run test:share                            # share codec and workspace rules, no browser
pnpm run test:loadout                          # spending rules, against the API
pnpm run test:string                           # the Blizzard talent-string codec
pnpm run test:simc                             # expansion == API count; every line a real character
pnpm run test:report                           # the SimC report reader, against a real report
pnpm run test:canvas                           # shapes, keylines, four colours, both themes
pnpm run test:forever                          # WoW Forever: three tabs, one pool of 51
pnpm run test:editor                           # design, connect, undo, save, plan, share, copy
pnpm run test:ranks                            # rank limits: at least, exactly, a dip; planner arcs
pnpm run test:popular                          # top players: see, use, narrow, sim them for real
pnpm test                                      # the whole workflow, with a live SimulationCraft run
pnpm run test:all                              # all of it
```

Production is Caddy serving the built assets and proxying `/api`:

```bash
docker compose --profile web up -d --build web   # http://localhost:8081
```

That one command is enough: `postgres`, `api` and `worker` come up as dependencies. Drop
`--build` to start what is already built. **The browser suites default to 8081**, this
container, rather than the dev server — a suite that fails because nothing happens to be
running on 5173 has said nothing about the app — and they wait for the API behind the proxy,
not just the page, since the API takes seconds longer to come up after a rebuild.

## Design

The full brief is [`../docs/02-target/ui-direction.md`](../docs/02-target/ui-direction.md).
What it comes down to here:

**Shape is the talent kind.** A circle is a passive, a square is an active ability, a
hexagon is a choice between two, an octagon is the hero-tree selector — which is what the
game does, and what a player reads before reading a single word. Encoding the data model
instead (one point round, two points square) told nobody anything.

**Constraint colours are sandwiched between keylines.** Four constraint kinds, four hues,
each one drawn as a ring between two near-black keylines, because black neighbours every
hue and a coloured rim laid straight onto talent artwork disappears against an icon of the
same family. The ring is three nested elements wearing the same `clip-path` rather than a
`box-shadow`, since a shadow is painted outside the border box and every hexagon clipped
its own ring away. The sidebar names all four, with the swatches built from the same
custom properties as the canvas so the legend cannot drift from what it describes.

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
| `App.tsx` | The three steps, and the per-tree workspace they share. |
| `lib/workspace.ts` | A tree fixed or open: painting, spending, the payload it sends. |
| `lib/useCounts.ts` | A live, debounced count per open tree. |
| `lib/space.ts` | Characters as the product of the trees; choice-side expansion. |
| `lib/enumerate.ts` | Open trees through the solver, into a list of characters. |
| `lib/simc.ts` | Characters as SimulationCraft profilesets. |
| `lib/simcReport.ts` | SimC's JSON report: ranking, talent value, choice duels. |
| `lib/share.ts` | All three trees, both halves of each, in the URL. Old links still open. |
| `lib/loadout.ts` | Spending points by hand, under the solver's rules. |
| `lib/loadoutString.ts` | Blizzard's talent string, in and out. |
| `lib/api.ts` | Typed client. Every field checked against a live response. |
| `lib/classes.ts`, `lib/theme.ts` | Class colours per theme; the theme on `<html data-theme>`. |
| `components/TreePane.tsx` | One tree, with its fixed/open switch, budget and count. |
| `components/EditorView.tsx` | The tree editor: grid, connections, barriers, inspector, icon picker. |
| `components/TemplatePicker.tsx` | What a project, or a tree added to one, starts from: blank, or a copy of real trees. |
| `components/ProjectRail.tsx` | Your custom projects, in place of the class rail. |
| `components/PoolCard.tsx` | A shared point pool's split, for Forever and pooled projects. |
| `components/PopularPanel.tsx` | What top players run, and three things to do with it. |
| `lib/design.ts` | A project as the editor holds it, and every edit as a pure function. |
| `lib/projects.ts` | Drafts in this browser, saved versions on the server. |
| `components/SpaceCard.tsx` | The product, its factors, and whether it is simmable. |
| `components/PaintTools.tsx` | What a click does, and what every colour means. |
| `components/SimulateView.tsx` | Step two: the file out, the report back. |
| `components/BuildViewer.tsx` | Flick through the builds on the trees. |
| `components/AnalysisView.tsx` | Step three: builds, talent value, choice nodes. |
| `components/TreeCanvas.tsx` | Pan, zoom, edges, node placement. |
| `components/TalentNode.tsx` | One talent; shape, icons, state. |
| `components/LoadoutString.tsx` | Paste a build in (fixing all three trees), copy one out. |
| `components/Legend.tsx` | Swatches and the shape key. |
| `components/SpecRail.tsx`, `ShareButton.tsx`, `Tooltip.tsx` | Selection, the link, tooltips. |

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
spending rules exist to hold, and `pnpm run test:loadout` asserts it by building real
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

**The talent string is verified against one the game exported.**

**The return leg is done.** SimulationCraft writes a JSON report with `json2=report.json`;
drop that file on the Sim results panel and every profileset lands back on the build it came
from. The file is parsed in the browser and never uploaded — a sim report describes somebody's
character and gear, which is not this tool's business.

What comes out of it:

- **A ranking.** Best, worst, how far behind, and each build's own number while stepping
  through them. One click jumps to the best.
- **What each talent was worth** — the mean of the builds taking it against the mean of those
  that do not, painted on the tree as a diverging scale and listed in a panel. This is the
  number the whole arc exists to produce: a sim ranks whole characters, so a single talent's
  value only appears once a *set* of builds that differ in a controlled way has been simmed,
  and producing exactly that set is what counting, filtering and enumerating were for.

**Both limits are stated in the UI, next to the numbers.** The sim reports a mean with an
error bar, and across a set of similar builds the intervals overlap — in the run this was
built against, the 67-build set spans 5.9% while each mean carries ±0.8%, so the top build is
ahead of the field and builds four and five are a tie. And the comparison is observational:
talents the tree never separates share a score, because nothing in the data can tell them
apart.

**The reader was built against a report SimulationCraft actually wrote.** Reading its source
(`report_json.cpp`) gave the key names; running it gave the rest — that results come back
unordered, that a zero-mean profileset is dropped rather than reported, and that the metric is
a sentence rather than a token. `fixtures/simc-report.json` is that run, and `pnpm test`
re-runs the whole loop through Docker when the SimC image is present. A Feral Druid build --
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
`pnpm run test:loadout` checks that it is closed before its gate and open on the gate
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

`pnpm run shots` captures both themes at desktop and phone width and **fails on any console
error** — a blank-looking panel and a thrown exception are indistinguishable in a still
image.

`pnpm test` is the workflow a player follows, end to end, against the real stack: paste a
real talent string (SimulationCraft's own sample Blood Death Knight), check it fixes all
three trees to exactly one build, open the hero tree and narrow it to 580, flip trees back
and forth and check nothing is lost, paint and un-paint, reopen the link in a fresh page,
simulate, download — and then, if the SimulationCraft image is present, **run the real
sim** on the downloaded file and drop its report back to check the ranking, talent value,
choice duels and "use this build". Without Docker the sim step is reported as skipped rather
than faked: a fabricated report would only prove the reader agrees with itself.

`pnpm run test:simc` holds the two claims the workflow rests on, against the live solver:
the client's expansion of selections equals the API's `builds` count exactly, and every
exported string decodes back to precisely the character it came from. That second check is
what found the solver storing talents under their neighbour's id on every tree with a
granted talent — see the worker's README.

Share links get a suite of their own (`pnpm run test:share`) because that is where a mistake
is silent: a link that loses a constraint still opens and gives the wrong answer. It
round-trips every field of all three trees, checks side 0 of a choice survives, and opens
links from before the redesign.
