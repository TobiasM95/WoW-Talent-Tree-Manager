import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { ChoiceSide, Constraints, TalentNode, TreeDetail } from "./api";
import type { ShareState } from "./share";
import type { NodeState } from "../components/TalentNode";

/**
 * The constraint set a user paints onto the tree.
 *
 * This is the app's central interaction, because the filter *is* the selection mechanism:
 * a user is not sampling builds, they are describing the builds they want and expecting all
 * of them. So the model is deliberately small and everything is reversible.
 *
 * Clicking a node cycles neutral -> required -> excluded -> neutral. Right-click or
 * shift-click cycles the other way. Two directions on one control, because reaching for a
 * mode switch between every node would dominate the gesture.
 *
 * Choice nodes take a side instead: their point is the alternative, not the node.
 *
 * Groups ("at least one of these", "exactly one of these") are a separate mode, and there is
 * exactly one of each. That is not a UI simplification -- the engine's filter holds a single
 * value per talent, so listing supports one group of each kind, and offering more in the
 * canvas would produce constraints the counter honours and the enumerator silently drops.
 */

export type GroupMode = "none" | "atLeastOne" | "exactlyOne";

export interface ConstraintState {
  points: number;
  required: ReadonlySet<number>;
  excluded: ReadonlySet<number>;
  sides: ReadonlyMap<number, ChoiceSide>;
  atLeastOne: ReadonlySet<number>;
  exactlyOne: ReadonlySet<number>;
  groupMode: GroupMode;
}

const SIDE_CYCLE: ChoiceSide[] = ["a", "b", "none"];

export function useConstraints(tree: TreeDetail | null, initial?: ShareState) {
  const cap = tree?.pointCap ?? tree?.maxPointsInTree ?? 30;
  const [points, setPointsState] = useState(() => initial?.points ?? cap);

  /*
    The budget follows the tree's cap until someone moves it.

    A player arriving at a talent tree is looking at a full build, not a partial one -- a
    default of 20 out of 34 shows a tree half of which is unreachable and a count that is not
    the count they came for. But once the slider has been touched that is a deliberate choice
    and must survive, including across a tree whose cap is smaller.
  */
  const touched = useRef(Boolean(initial?.points));
  const setPoints = useCallback((value: number) => {
    touched.current = true;
    setPointsState(value);
  }, []);
  const [required, setRequired] = useState<Set<number>>(() => new Set(initial?.required));
  const [excluded, setExcluded] = useState<Set<number>>(() => new Set(initial?.excluded));
  const [sides, setSides] = useState<Map<number, ChoiceSide>>(
    () => new Map(initial?.sides),
  );
  const [atLeastOne, setAtLeastOne] = useState<Set<number>>(
    () => new Set(initial?.atLeastOne),
  );
  const [exactlyOne, setExactlyOne] = useState<Set<number>>(
    () => new Set(initial?.exactlyOne),
  );
  const [groupMode, setGroupMode] = useState<GroupMode>("none");

  // A tree's cap differs per kind -- a hero tree grants 13 and a class tree 34. An untouched
  // budget simply follows the new cap; a deliberate one is kept where it still fits.
  useEffect(() => {
    setPointsState((current) => (touched.current ? Math.min(current, cap) : cap));
  }, [cap]);

  const reset = useCallback(() => {
    setRequired(new Set());
    setExcluded(new Set());
    setSides(new Map());
    setAtLeastOne(new Set());
    setExactlyOne(new Set());
    setGroupMode("none");
  }, []);

  /**
   * Adopt a constraint set wholesale, for a link that was opened or pasted.
   *
   * Separate from `reset` because the two are opposites: one empties the canvas, the other
   * fills it from somewhere else, and conflating them made "clear" and "load a share link"
   * the same code path with a flag.
   */
  const adopt = useCallback((state: ShareState) => {
    // A stash without a budget is a tree this session has not set one for, so the next
    // effect puts it back on that tree's cap.
    touched.current = Boolean(state.points);
    setRequired(new Set(state.required));
    setExcluded(new Set(state.excluded));
    setSides(new Map(state.sides));
    setAtLeastOne(new Set(state.atLeastOne));
    setExactlyOne(new Set(state.exactlyOne));
    setGroupMode("none");
    if (state.points) setPointsState(state.points);
  }, []);

  const activate = useCallback(
    (node: TalentNode, alternate: boolean) => {
      const id = node.nodeId;

      if (groupMode !== "none") {
        const setter = groupMode === "atLeastOne" ? setAtLeastOne : setExactlyOne;
        setter((previous) => {
          const next = new Set(previous);
          if (next.has(id)) next.delete(id);
          else next.add(id);
          return next;
        });
        // A node cannot be in a group and individually constrained at the same time: the
        // group already says what should happen to it.
        setRequired((p) => remove(p, id));
        setExcluded((p) => remove(p, id));
        return;
      }

      if (node.kind === "choice" && node.entries.length >= 2) {
        setSides((previous) => {
          const next = new Map(previous);
          const current = previous.get(id);
          const index = current ? SIDE_CYCLE.indexOf(current) : -1;
          const step = alternate ? -1 : 1;
          const at = index + step;
          if (at < 0 || at >= SIDE_CYCLE.length) next.delete(id);
          else next.set(id, SIDE_CYCLE[at]!);
          return next;
        });
        return;
      }

      // neutral -> required -> excluded -> neutral, or the reverse.
      const isRequired = required.has(id);
      const isExcluded = excluded.has(id);
      const order = alternate ? -1 : 1;
      const at = (isRequired ? 1 : isExcluded ? 2 : 0) + order;
      const landing = ((at % 3) + 3) % 3;

      setRequired((p) => (landing === 1 ? add(p, id) : remove(p, id)));
      setExcluded((p) => (landing === 2 ? add(p, id) : remove(p, id)));
      setAtLeastOne((p) => remove(p, id));
      setExactlyOne((p) => remove(p, id));
    },
    [groupMode, required, excluded],
  );

  /** What each node should look like. One pass, so the canvas gets a ready map. */
  const states = useMemo(() => {
    const map = new Map<number, NodeState>();
    for (const id of required) map.set(id, "required");
    for (const id of excluded) map.set(id, "excluded");
    for (const id of atLeastOne) map.set(id, "grouped");
    for (const id of exactlyOne) map.set(id, "grouped");
    return map;
  }, [required, excluded, atLeastOne, exactlyOne]);

  /**
   * The request body. A group of one is dropped rather than sent: "at least one of [X]" is
   * just "require X", and the API rejects a single-node group -- correctly, since it almost
   * always means the user is still building the group.
   */
  const payload = useMemo<Constraints>(() => {
    const body: Constraints = { points };
    if (required.size) body.mustHave = [...required];
    if (excluded.size) body.mustNotHave = [...excluded];
    if (sides.size) body.choiceSides = Object.fromEntries([...sides].map(([k, v]) => [String(k), v]));
    if (atLeastOne.size > 1) body.atLeastOneOf = [[...atLeastOne]];
    if (exactlyOne.size > 1) body.exactlyOneOf = [[...exactlyOne]];
    return body;
  }, [points, required, excluded, sides, atLeastOne, exactlyOne]);

  const pending = useMemo(
    () =>
      [
        atLeastOne.size === 1 ? "at-least-one group needs a second talent" : null,
        exactlyOne.size === 1 ? "exactly-one group needs a second talent" : null,
      ].filter(Boolean) as string[],
    [atLeastOne, exactlyOne],
  );

  return {
    points,
    setPoints,
    cap,
    required,
    excluded,
    sides,
    atLeastOne,
    exactlyOne,
    groupMode,
    setGroupMode,
    activate,
    reset,
    adopt,
    states,
    payload,
    pending,
    count:
      required.size + excluded.size + sides.size + atLeastOne.size + exactlyOne.size,
  };
}

const add = (set: Set<number>, id: number) => {
  if (set.has(id)) return set;
  const next = new Set(set);
  next.add(id);
  return next;
};

const remove = (set: Set<number>, id: number) => {
  if (!set.has(id)) return set;
  const next = new Set(set);
  next.delete(id);
  return next;
};
