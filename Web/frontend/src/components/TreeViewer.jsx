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
      source: `${treeObject.order_id}`,
      target: `${childIndex}`,
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

  const insertDividerLines = (newNodes, newEdges) => {
    // we need min col max col, we need appropriate divider heights
    newNodes.push({
      id: "d1",
      type: "dividerNode",
      draggable: false,
      connectable: false,
      position: {
        x:
          0.5 *
          0 *
          (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
        y:
          0.5 *
          (6.4 + 1) *
          (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
      },
      data: {
        size: treeViewerSettings.nodeSize,
        position: "left",
      },
    });
    newNodes.push({
      id: "d2",
      type: "dividerNode",
      draggable: false,
      connectable: false,
      position: {
        x:
          0.5 *
          20 *
          (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
        y:
          0.5 *
          (6.4 + 1) *
          (treeViewerSettings.nodeSize + treeViewerSettings.gridSpace),
      },
      data: {
        size: treeViewerSettings.nodeSize,
        position: "right",
      },
    });

    newEdges.unshift({
      id: `de1-2`,
      source: `d1`,
      target: `d2`,
      type: "straight",
      style: {
        strokeWidth: 3,
        stroke: "#a1a1ff",
      },
    });
  };

  useEffect(() => {
    // console.log(treeData);
    var newNodes = [];
    var newEdges = [];
    for (const rawNode of treeData) {
      newNodes.push(TreeObjectToFlowNode(rawNode));
      newEdges.push(...TreeObjectToFlowEdges(rawNode));
    }
    insertDividerLines(newNodes, newEdges);
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

  const onMoveEnd = () => {
    console.log("on move end");
  };

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
        onNodeDragStop={onMoveEnd}
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
