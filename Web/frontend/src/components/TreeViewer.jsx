import { useCallback } from "react";
import { Box, useTheme } from "@mui/material";
import { tokens } from "../theme";
import ReactFlow, {
  // MiniMap,
  Controls,
  Background,
  useNodesState,
  useEdgesState,
  addEdge,
} from "reactflow";
import { initialEdges, initialNodes } from "../data/mockTree";

import "reactflow/dist/style.css";

import "./tree_components/node-styles.css";
import PassiveNode from "./tree_components/PassiveNode";

const proOptions = { hideAttribution: true };

const nodeTypes = { passiveNode: PassiveNode };

const TreeViewer = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [nodes, setNodes, onNodesChange] = useNodesState(initialNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState(initialEdges);

  const onConnect = useCallback(
    (params) => setEdges((eds) => addEdge(params, eds)),
    [setEdges]
  );

  return (
    <Box sx={{ width: "100%", height: "100%" }} p="5px">
      <ReactFlow
        proOptions={proOptions}
        nodeTypes={nodeTypes}
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onEdgesChange={onEdgesChange}
        onConnect={onConnect}
        snapGrid={[80, 80]}
        snapToGrid
        fitView
      >
        <Controls />
        {/* <MiniMap /> */}
        <Background variant="dots" gap={24} size={1} />
      </ReactFlow>
    </Box>
  );
};

export default TreeViewer;
