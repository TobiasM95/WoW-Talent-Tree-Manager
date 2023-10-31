import { treeViewerSettings } from "../../data/settings";

export function insertDividerLines(newNodes, newEdges) {
  const { separators, minMaxColumns } = extractDividerInfo(newNodes);
  for (var i = newNodes.length - 1; i >= 0; --i) {
    if (newNodes[i].id.startsWith("d")) {
      newNodes.splice(i, 1);
    }
  }
  for (i = newEdges.length - 1; i >= 0; --i) {
    if (newEdges[i].id.startsWith("de")) {
      newEdges.splice(i, 1);
    }
  }
  for (const separator of separators) {
    newNodes.push({
      id: `d${separator[0]}left`,
      type: "dividerNode",
      draggable: false,
      connectable: false,
      position: {
        x: 0.5 * (minMaxColumns[0] - 1) * treeViewerSettings.gridSpace,
        y: 0.5 * (separator[1] + 1.4) * treeViewerSettings.gridSpace,
      },
      data: {
        size: treeViewerSettings.nodeSize,
        position: "left",
        requiredPoints: separator[0],
      },
    });
    newNodes.push({
      id: `d${separator[0]}right`,
      type: "dividerNode",
      draggable: false,
      connectable: false,
      position: {
        x: 0.5 * (minMaxColumns[1] + 3) * treeViewerSettings.gridSpace,
        y: 0.5 * (separator[1] + 1.4) * treeViewerSettings.gridSpace,
      },
      data: {
        size: treeViewerSettings.nodeSize,
        position: "right",
        requiredPoints: separator[0],
      },
    });

    newEdges.unshift({
      id: `de${separator[0]}l-${separator[0]}r`,
      source: `d${separator[0]}left`,
      target: `d${separator[0]}right`,
      type: "straight",
      style: {
        strokeWidth: 3,
        stroke: "#a1a1ff",
      },
    });
  }
}

function extractDividerInfo(nodes) {
  const { pointThresholds, minMaxRows, minMaxColumns } =
    extractPointThresholdsMinMaxRows(nodes);
  var dividerInfo = [];
  for (let i = 0; i < pointThresholds.length; i++) {
    dividerInfo.push([minMaxRows[1] + 1, minMaxRows[0] - 1]);
  }
  for (const node of nodes) {
    const id = pointThresholds.findIndex(
      (element) => element === node.data.requiredPoints
    );
    if (node.data.row < dividerInfo[id][0]) {
      dividerInfo[id][0] = node.data.row;
    }
    if (node.data.row > dividerInfo[id][1]) {
      dividerInfo[id][1] = node.data.row;
    }
  }
  var separators = [];
  for (let i = 1; i < pointThresholds.length; i++) {
    var valid = true;
    var maxDividerRow = minMaxRows[0] - 1;
    for (let j = 0; j < i; j++) {
      if (dividerInfo[j][1] >= dividerInfo[i][0]) {
        valid = false;
      }
      if (dividerInfo[j][1] > maxDividerRow) {
        maxDividerRow = dividerInfo[j][1];
      }
    }
    if (valid) {
      separators.push([
        pointThresholds[i],
        0.5 * (maxDividerRow + dividerInfo[i][0]),
      ]);
    }
  }
  return {
    separators: separators,
    minMaxColumns: minMaxColumns,
  };
}

function extractPointThresholdsMinMaxRows(nodes) {
  var thresholds = [];
  var minMaxRows = [null, null];
  var minMaxColumns = [null, null];
  for (const node of nodes) {
    if (!thresholds.includes(node.data.requiredPoints)) {
      thresholds.push(node.data.requiredPoints);
    }
    if (minMaxRows[0] === null || node.data.row < minMaxRows[0]) {
      minMaxRows[0] = node.data.row;
    }
    if (minMaxRows[1] === null || node.data.row > minMaxRows[1]) {
      minMaxRows[1] = node.data.row;
    }
    if (minMaxColumns[0] === null || node.data.column < minMaxColumns[0]) {
      minMaxColumns[0] = node.data.column;
    }
    if (minMaxColumns[1] === null || node.data.column > minMaxColumns[1]) {
      minMaxColumns[1] = node.data.column;
    }
  }
  return {
    pointThresholds: thresholds.sort(function (a, b) {
      return a - b;
    }),
    minMaxRows: minMaxRows,
    minMaxColumns: minMaxColumns,
  };
}
