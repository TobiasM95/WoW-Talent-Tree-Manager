import { createContext, useCallback, useEffect, useState } from "react";
import { Box, useTheme } from "@mui/material";
import { tokens } from "../theme";
import { treeViewerSettings } from "../data/settings";
import { insertDividerLines } from "./tree_components/utils";
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
  DividerNode,
  PassiveNode,
  ActiveNode,
  SwitchNode,
} from "./tree_components/NodeTypes";

const proOptions = { hideAttribution: true };

const typeToNodeType = {
  DIVIDER: "dividerNode",
  PASSIVE: "passiveNode",
  ACTIVE: "activeNode",
  SWITCH: "switchNode",
};
const nodeTypes = {
  dividerNode: DividerNode,
  passiveNode: PassiveNode,
  activeNode: ActiveNode,
  switchNode: SwitchNode,
};
export const DragProvider = createContext(null);

const TreeObjectToFlowNode = (treeObject) => {
  return {
    id: "n" + treeObject.order_id,
    type: typeToNodeType[treeObject.talent_type],
    position: {
      x: 0.5 * (treeObject.column + 1) * treeViewerSettings.gridSpace,
      y: 0.5 * (treeObject.row + 1) * treeViewerSettings.gridSpace,
    },
    data: {
      id: treeObject.order_id,
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
      row: treeObject.row,
      column: treeObject.column,
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
      source: `n${treeObject.order_id}`,
      target: `n${childIndex}`,
      type: "straight",
      markerEnd: {
        type: MarkerType.ArrowClosed,
        width: 10,
        height: 10,
        color: "#888888",
      },
      style: {
        strokeWidth: 4,
        stroke: "#888888",
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
  const [isDragging, setIsDragging] = useState(false);

  useEffect(() => {
    var newNodes = [];
    var newEdges = [];
    for (const rawNode of treeData) {
      newNodes.push(TreeObjectToFlowNode(rawNode));
      newEdges.push(...TreeObjectToFlowEdges(rawNode));
    }
    insertDividerLines(newNodes, newEdges);
    setNodes(newNodes);
    setEdges(newEdges);
  }, [treeData]);

  const onConnect = useCallback(
    (params) => setEdges((eds) => addEdge(params, eds)),
    [setEdges]
  );

  const onNodeDragStart = (event, draggedNode) => {
    setIsDragging(true);
  };

  const onNodeDragStop = (event, draggedNode) => {
    setIsDragging(false);
    for (var node of nodes) {
      if (node.id === draggedNode.id) {
        node.data.row =
          (2 * node.position.y) / treeViewerSettings.gridSpace - 1;
        node.data.column =
          (2 * node.position.x) / treeViewerSettings.gridSpace - 1;
      }
    }
    var newNodes = [...nodes];
    var newEdges = [...edges];
    insertDividerLines(newNodes, newEdges);
    setNodes(newNodes);
    setEdges(newEdges);
  };

  return (
    <DragProvider.Provider value={isDragging}>
      <Box sx={{ width: "100%", height: "100%" }} p="5px">
        <ReactFlow
          proOptions={proOptions}
          nodeTypes={nodeTypes}
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          onNodeDragStop={onNodeDragStop}
          onNodeDragStart={onNodeDragStart}
          snapGrid={[
            treeViewerSettings.gridSpace,
            treeViewerSettings.gridSpace,
          ]}
          snapToGrid
          fitView
        >
          <Controls />
          {/* <MiniMap /> */}
          <Background variant="dots" gap={24} size={1} />
        </ReactFlow>
      </Box>
    </DragProvider.Provider>
  );
};

export default TreeViewer;
