export const initialNodes = [
  {
    id: "1",
    type: "passiveNode",
    position: { x: 0, y: 0 },
    data: { size: 80 },
  },
  {
    id: "2",
    type: "passiveNode",
    position: { x: -120, y: 120 },
    data: { size: 80 },
  },
  {
    id: "3",
    type: "passiveNode",
    position: { x: 0, y: 120 },
    data: { size: 80 },
  },
  {
    id: "4",
    type: "passiveNode",
    position: { x: 120, y: 120 },
    data: { size: 80 },
  },
];
export const initialEdges = [
  { id: "e1-2", source: "1", target: "2" },
  { id: "e1-3", source: "1", target: "3" },
  { id: "e1-4", source: "1", target: "4" },
];
