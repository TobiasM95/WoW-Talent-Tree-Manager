import { createContext, useContext } from "react";

/**
 * Every talent of the current specialisation, by node id, for anything that has to name a
 * talent in *another* tree -- the tooltip, when a hero talent says which ability it modifies.
 *
 * A context rather than a prop, because the one consumer sits three components below the
 * one place that has all the trees, and nothing in between cares.
 */
export interface NamedNode {
  name: string;
  /** "Class", "Spec" or "Hero". */
  tree: string;
}

export const NodeNames = createContext<ReadonlyMap<number, NamedNode>>(new Map());

export const useNodeNames = () => useContext(NodeNames);
