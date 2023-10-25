import { useCallback, useEffect } from "react";
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

import "reactflow/dist/style.css";

import PassiveNode from "./tree_components/PassiveNode";

const proOptions = { hideAttribution: true };

const nodeTypes = { passiveNode: PassiveNode };

const TreeViewer = ({ treeData }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [nodes, setNodes, onNodesChange] = useNodesState([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);

  useEffect(() => {
    console.log(treeData);
  }, [treeData]);

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
        snapGrid={[40, 40]}
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
