export const initialNodes = [
  {
    id: "1",
    type: "passiveNode",
    position: { x: 0, y: 0 },
    data: { value: 123 },
  },
  {
    id: "2",
    type: "passiveNode",
    position: { x: 0, y: 160 },
    data: { label: "2" },
  },
];
export const initialEdges = [{ id: "e1-2", source: "1", target: "2" }];
