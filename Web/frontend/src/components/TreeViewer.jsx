import { useCallback, useEffect } from "react";
import { Box, useTheme } from "@mui/material";
import { tokens } from "../theme";
import { treeViewerSettings } from "../data/settings";
import ReactFlow, {
  // MiniMap,
  Controls,
  Background,
  useNodesState,
  useEdgesState,
  addEdge,
  MarkerType,
} from "reactflow";

import "reactflow/dist/style.css";

import {
  PassiveNode,
  ActiveNode,
  SwitchNode,
} from "./tree_components/NodeTypes";

const proOptions = { hideAttribution: true };

const typeToNodeType = {
  PASSIVE: "passiveNode",
  ACTIVE: "activeNode",
  SWITCH: "switchNode",
};
const nodeTypes = {
  passiveNode: PassiveNode,
  activeNode: ActiveNode,
  switchNode: SwitchNode,
};

const TreeObjectToFlowNode = (treeObject) => {
  return {
    id: "" + treeObject.order_id,
    type: typeToNodeType[treeObject.talent_type],
    position: {
      x:
        0.5 *
        (treeObject.column + 1) *
        (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
      y:
        0.5 *
        (treeObject.row + 1) *
        (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
    },
    data: {
      size: treeViewerSettings.nodeSize,
      description: treeObject.description,
      descriptionSwitch: treeObject.description_switch,
      iconName: treeObject.icon_name,
      iconNameSwitch: treeObject.icon_name_switch,
      maxPoints: treeObject.max_points,
      name: treeObject.name,
      nameSwitch: treeObject.name_switch,
      nodeID: treeObject.node_id,
      preFilled: treeObject.pre_filled,
      requiredPoints: treeObject.required_points,
      // for completeness sake
      childIDs: treeObject.child_ids,
      parentIDs: treeObject.parent_ids,
    },
  };
};

const TreeObjectToFlowEdges = (treeObject) => {
  var edges = [];
  for (const childIndex of treeObject.child_ids) {
    edges.push({
      id: `e${treeObject.order_id}-${childIndex}`,
      source: `${treeObject.order_id}`,
      target: `${childIndex}`,
      type: "straight",
      markerEnd: {
        type: MarkerType.ArrowClosed,
        width: 10,
        height: 10,
        color: "#cca100",
      },
      style: {
        strokeWidth: 4,
        stroke: "#cca100",
      },
    });
  }
  return edges;
};

const TreeViewer = ({ treeData }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [nodes, setNodes, onNodesChange] = useNodesState([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);

  useEffect(() => {
    // console.log(treeData);
    var newNodes = [];
    var newEdges = [];
    for (const rawNode of treeData) {
      newNodes.push(TreeObjectToFlowNode(rawNode));
      newEdges.push(...TreeObjectToFlowEdges(rawNode));
    }
    setNodes(newNodes);
    setEdges(newEdges);
    console.log("rerender...");
    // console.log(newNodes);
    // console.log(newEdges);
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
