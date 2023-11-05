import { useCallback, useEffect } from "react";
import { Box, useTheme } from "@mui/material";
import { tokens } from "../../theme";
import { treeViewerSettings } from "../../data/settings";
import { insertDividerLines } from "./utils";
import ReactFlow, {
  // MiniMap,
  Controls,
  Background,
  useNodesState,
  useEdgesState,
  addEdge,
  MarkerType,
} from "reactflow";
import { useDrag } from "../DragProvider";

import "reactflow/dist/style.css";

import { DividerNode, PassiveNode, ActiveNode, SwitchNode } from "./NodeTypes";

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
      childIDs: treeObject.child_ids,
      parentIDs: treeObject.parent_ids,
      isConnectable: false,
    },
  };
};

const TreeObjectToFlowEdges = (treeObject, treeData, colors) => {
  var edges = [];
  for (const childIndex of treeObject.child_ids) {
    if (!treeData.hasOwnProperty(childIndex)) {
      continue;
    }
    const goldArrow =
      treeObject.pre_filled === 1 && treeData[childIndex].pre_filled === 1;
    edges.push({
      id: `e${treeObject.order_id}-${childIndex}`,
      source: `n${treeObject.order_id}`,
      target: `n${childIndex}`,
      type: "straight",
      markerEnd: {
        type: MarkerType.ArrowClosed,
        width: 10,
        height: 10,
        color: goldArrow ? colors.treeColors.gold : colors.treeColors.grey,
      },
      style: {
        strokeWidth: 4,
        stroke: goldArrow ? colors.treeColors.gold : colors.treeColors.grey,
      },
    });
  }
  return edges;
};

const BuildViewer = ({ treeData }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [nodes, setNodes, onNodesChange] = useNodesState([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);
  const { isDragging, isReadyToDrag, setIsDragging, setDragReady } = useDrag();

  useEffect(() => {
    setDragReady(false);
  }, []);

  useEffect(() => {
    var newNodes = [];
    var newEdges = [];
    for (const [order_id, rawNode] of Object.entries(treeData)) {
      newNodes.push(TreeObjectToFlowNode(rawNode, colors));
      newEdges.push(...TreeObjectToFlowEdges(rawNode, treeData, colors));
    }
    insertDividerLines(newNodes, newEdges, colors);
    setNodes(newNodes);
    setEdges(newEdges);
  }, [treeData]);

  const onConnect = useCallback(
    (params) => setEdges((eds) => addEdge(params, eds)),
    [setEdges]
  );

  const onMoveStart = () => {
    console.log("Start");
    if (!isReadyToDrag) {
      setDragReady(true);
      return;
    }
    if (!isDragging) {
      setIsDragging(true);
    }
  };

  const onMoveEnd = () => {
    setIsDragging(false);
  };

  // these have to be moved to the tree editor later
  // const onNodeDragStart = (event, draggedNode) => {
  //   setIsDragging(true);
  //   console.log("on node drag start");
  // };

  // const onNodeDragStop = (event, draggedNode) => {
  //   setIsDragging(false);
  //   console.log("on node drag stop");
  //   for (var node of nodes) {
  //     if (node.id === draggedNode.id) {
  //       node.data.row =
  //         (2 * node.position.y) / treeViewerSettings.gridSpace - 1;
  //       node.data.column =
  //         (2 * node.position.x) / treeViewerSettings.gridSpace - 1;
  //     }
  //   }
  //   var newNodes = [...nodes];
  //   var newEdges = [...edges];
  //   insertDividerLines(newNodes, newEdges, colors);
  //   setNodes(newNodes);
  //   setEdges(newEdges);
  // };

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
        onMove={onMoveStart}
        onMoveEnd={onMoveEnd}
        snapGrid={[treeViewerSettings.gridSpace, treeViewerSettings.gridSpace]}
        snapToGrid
        fitView
        nodesDraggable={false}
      >
        <Controls
          showInteractive={false}
          onZoomIn={() => {
            setTimeout(onMoveEnd, 500);
          }}
          onZoomOut={() => {
            setTimeout(onMoveEnd, 500);
          }}
          onFitView={() => {
            setTimeout(onMoveEnd, 500);
          }}
        />
        {/* <MiniMap /> */}
        <Background variant="dots" gap={24} size={1} />
      </ReactFlow>
    </Box>
  );
};

export default BuildViewer;
