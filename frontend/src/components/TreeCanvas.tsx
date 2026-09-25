import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState } from "react";
import type { TalentNode as NodeData, TreeDetail } from "../lib/api";
import { TalentNode, type NodeState } from "./TalentNode";
import { Tooltip } from "./Tooltip";

/**
 * The tree, pannable and zoomable, and the surface on which constraints are painted.
 *
 * Layout comes from upstream (`pos` is the game's own coordinate space), so this does no
 * graph layout of its own -- it normalises the coordinates into a local box and draws.
 *
 * Pan and zoom are one CSS transform on one wrapper. With ~100 nodes that keeps a drag to a
 * single composited step instead of re-laying out a hundred absolutely-positioned boxes per
 * frame, which is the requirement the UI direction sets for ornament not costing latency.
 */

export interface TreeCanvasProps {
  tree: TreeDetail;
  states: Map<number, NodeState>;
  sides: Map<number, "a" | "b" | "none">;
  stale?: boolean;
  /** nodeId -> points, for the build currently being inspected. */
  build?: Record<string, number> | null;
  /** nodeId -> share of the result set, when statistics are being shown. */
  shares?: Map<number, number> | null;
  /** Per-node sim impact, -1..1, scaled to the widest swing in the set. */
  impacts?: Map<number, number> | null;
  /** Build mode: which nodes can take another point right now. */
  reachable?: Set<number> | null;
  /** True while points are being spent by hand. */
  editing?: boolean;
  onActivate: (node: NodeData, alternate: boolean) => void;
}

const PAD = 44;
// Pixels between adjacent talent columns at zoom 1. Two node widths, so neighbours are
// clearly separate without the tree spreading out.
const COLUMN_PITCH = 86;
// Low enough that a full spec tree fits a phone viewport. A higher floor looks tidier on a
// desktop and silently crops the tree on a small screen, which is worse than small icons.
const MIN_ZOOM = 0.2;
const MAX_ZOOM = 3;
// The fit stops a little above natural size: a four-talent custom tree fitted to a whole pane
// came up at 3x, every talent the size of a fist. The wheel still zooms to MAX_ZOOM.
const FIT_MAX_ZOOM = 1.4;
// A whole tree pressed flat against the panel edge reads as clipped even when it is not.
const FIT_MARGIN = 0.97;

interface Layout {
  nodes: Map<number, { x: number; y: number }>;
  width: number;
  height: number;
}

function layoutOf(tree: TreeDetail): Layout {
  const xs = tree.nodes.map((n) => n.pos.x);
  const ys = tree.nodes.map((n) => n.pos.y);
  const minX = Math.min(...xs);
  const minY = Math.min(...ys);
  const maxX = Math.max(...xs);
  const maxY = Math.max(...ys);

  /*
    Scale from the grid pitch, not from the tree overall width.

    Normalising total width to a constant makes every tree the same number of pixels across
    regardless of how many columns it has -- which stretches a four-column hero tree to the
    width of a seven-column class tree, and then the fit shrinks it to nothing in a narrow
    pane. Deriving the scale from the pitch gives every tree the same node *density*
    instead, so a hero tree is simply narrower than a class tree, which is what it is.

    The pitch is the closest any two talents come to each other, measured as a distance
    rather than as a gap between column coordinates. A hero tree is a diamond: its middle
    rows sit half a step off the rows above and below, so its column coordinates are 300
    apart while no two talents are ever closer than 600. Reading the pitch off those
    coordinates halved the scale and rendered the whole hero tree at sixteen pixels a node,
    which is not a talent tree, it is a row of stamps.
  */
  let pitch = Infinity;
  for (let i = 0; i < tree.nodes.length; i++) {
    const a = tree.nodes[i]!.pos;
    for (let j = i + 1; j < tree.nodes.length; j++) {
      const b = tree.nodes[j]!.pos;
      const d = Math.hypot(a.x - b.x, a.y - b.y);
      if (d > 0) pitch = Math.min(pitch, d);
    }
  }
  // One node, or every node stacked on one point: nothing to measure, so fall back to the
  // tree own height and let the fit do the rest.
  if (!Number.isFinite(pitch) || pitch <= 0) {
    pitch = Math.max((maxY - minY) / Math.max(tree.nodes.length - 1, 1), 1);
  }
  const scale = COLUMN_PITCH / pitch;

  const nodes = new Map<number, { x: number; y: number }>();
  for (const node of tree.nodes) {
    nodes.set(node.nodeId, {
      x: (node.pos.x - minX) * scale + PAD,
      y: (node.pos.y - minY) * scale + PAD,
    });
  }
  return {
    nodes,
    width: (maxX - minX) * scale + PAD * 2,
    height: (maxY - minY) * scale + PAD * 2,
  };
}

export function TreeCanvas({
  tree,
  states,
  sides,
  stale,
  build,
  shares,
  impacts,
  reachable,
  editing,
  onActivate,
}: TreeCanvasProps) {
  const viewport = useRef<HTMLDivElement>(null);
  const [view, setView] = useState({ x: 0, y: 0, zoom: 1 });
  const [panning, setPanning] = useState(false);
  const [hovered, setHovered] = useState<{ node: NodeData; el: HTMLElement } | null>(null);

  const layout = useMemo(() => layoutOf(tree), [tree]);
  // Point-gate lookup, so an edge can be drawn as a gate crossing without a find() per edge.
  const gateOf = useMemo(
    () => new Map(tree.nodes.map((n) => [n.nodeId, n.pointsRequired])),
    [tree.nodes],
  );

  /*
    Fit the tree to the viewport, on load and whenever either changes.

    Driven by a ResizeObserver rather than a one-shot measurement, because the first layout
    pass can report a height of zero: this panel is a flex child, and measuring before the
    flex layout resolves produces a clamped zoom and an offset that parks the tree off
    screen. That is not a hypothetical -- it left the canvas blank at phone width, where the
    column layout settles a frame later than the desktop one.
  */
  useLayoutEffect(() => {
    const element = viewport.current;
    if (!element) return;

    const fit = () => {
      const { width, height } = element.getBoundingClientRect();
      if (width < 1 || height < 1) return; // not laid out yet; the observer will call back
      const zoom = Math.min(
        FIT_MAX_ZOOM,
        Math.max(
          MIN_ZOOM,
          Math.min(width / layout.width, height / layout.height) * FIT_MARGIN,
        ),
      );
      setView({
        x: (width - layout.width * zoom) / 2,
        y: (height - layout.height * zoom) / 2,
        zoom,
      });
    };

    fit();
    const observer = new ResizeObserver(fit);
    observer.observe(element);
    return () => observer.disconnect();
  }, [layout]);

  const onPointerDown = useCallback((event: React.PointerEvent) => {
    // Only background drags pan; a drag starting on a node is a click on that node.
    if ((event.target as HTMLElement).closest(".ttm-node")) return;
    event.currentTarget.setPointerCapture(event.pointerId);
    setPanning(true);
  }, []);

  const onPointerMove = useCallback(
    (event: React.PointerEvent) => {
      if (!panning) return;
      setView((v) => ({ ...v, x: v.x + event.movementX, y: v.y + event.movementY }));
    },
    [panning],
  );

  const endPan = useCallback(() => setPanning(false), []);

  // Zoom toward the cursor, so the thing being inspected stays under the pointer.
  const onWheel = useCallback((event: React.WheelEvent) => {
    event.preventDefault();
    const box = event.currentTarget.getBoundingClientRect();
    const px = event.clientX - box.left;
    const py = event.clientY - box.top;
    setView((v) => {
      const zoom = Math.min(
        MAX_ZOOM,
        Math.max(MIN_ZOOM, v.zoom * (event.deltaY < 0 ? 1.12 : 1 / 1.12)),
      );
      const k = zoom / v.zoom;
      return { zoom, x: px - (px - v.x) * k, y: py - (py - v.y) * k };
    });
  }, []);

  // A hovered node's tooltip must not survive a pan or a re-render of the world.
  useEffect(() => {
    if (panning) setHovered(null);
  }, [panning]);

  const onHover = useCallback((node: NodeData | null, el: HTMLElement | null) => {
    setHovered(node && el ? { node, el } : null);
  }, []);

  return (
    <div
      ref={viewport}
      className="ttm-canvas"
      data-panning={panning ? "" : undefined}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={endPan}
      onPointerCancel={endPan}
      onWheel={onWheel}
      role="group"
      aria-label={`${tree.name} talent tree, ${tree.nodes.length} talents`}
    >
      <div
        className="ttm-canvas-world"
        style={{
          width: layout.width,
          height: layout.height,
          transform: `translate3d(${view.x}px, ${view.y}px, 0) scale(${view.zoom})`,
        }}
      >
        <svg className="ttm-edges" width={layout.width} height={layout.height}>
          {tree.nodes.flatMap((node) =>
            node.parents.map((parentId) => {
              const from = layout.nodes.get(parentId);
              const to = layout.nodes.get(node.nodeId);
              // Cross-tree prerequisites exist in the live data: a parent can live in a
              // different tree and therefore have no position here. Skipping the edge is
              // correct -- there is nothing on this canvas to draw it to.
              if (!from || !to) return null;
              // A jump in pointsRequired means this edge crosses a gate.
              const gate = (gateOf.get(parentId) ?? 0) < node.pointsRequired;
              return (
                <line
                  key={`${parentId}-${node.nodeId}`}
                  className="ttm-edge"
                  data-gate={gate ? "" : undefined}
                  x1={from.x}
                  y1={from.y}
                  x2={to.x}
                  y2={to.y}
                />
              );
            }),
          )}
        </svg>

        {tree.nodes.map((node) => (
          <TalentNode
            key={node.nodeId}
            node={node}
            x={layout.nodes.get(node.nodeId)?.x ?? 0}
            y={layout.nodes.get(node.nodeId)?.y ?? 0}
            state={states.get(node.nodeId) ?? "neutral"}
            side={sides.get(node.nodeId)}
            stale={stale}
            spent={build ? (build[String(node.nodeId)] ?? 0) : undefined}
            share={shares ? (shares.get(node.nodeId) ?? 0) : undefined}
            impact={impacts?.get(node.nodeId)}
            reachable={reachable ? reachable.has(node.nodeId) : undefined}
            editing={editing}
            onActivate={onActivate}
            onHover={onHover}
          />
        ))}
      </div>

      {hovered && <Tooltip node={hovered.node} anchor={hovered.el} />}
    </div>
  );
}
