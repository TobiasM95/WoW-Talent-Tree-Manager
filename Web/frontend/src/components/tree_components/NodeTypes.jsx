import { Box, Paper, useTheme } from "@mui/material";
//import { useCallback } from "react";
import { Handle, Position } from "reactflow";
import Image from "mui-image";
import { tokens } from "../../theme";
import { treeNodeSettings } from "../../data/settings";

import "./treeComponentTheming.css";

export function PassiveNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  return (
    <Box className="passive-nodes" width={data.size} height={data.size}>
      <Box
        className="circle-shape"
        bgcolor={data.preFilled ? "#cca100" : "#000000"}
        width={data.size}
        height={data.size}
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <Image
          className="circle-shape"
          src={`/preset_icons/${data.iconName}`}
          width="90%"
          height="90%"
          duration={treeNodeSettings.imageFadeInDuration}
        />
      </Box>
      <Box position="absolute" top="75%" left="65%">
        <Paper
          variant="outlined"
          square
          sx={{
            width: `${data.size * 0.65}px`,
            fontSize: `${data.size * 0.3}px`,
            fontWeight: "600",
            textAlign: "center",
            color: colors.grey[200],
          }}
        >
          {`${data.preFilled ? data.maxPoints : "0"} / ${data.maxPoints}`}
        </Paper>
      </Box>
      <Handle type="target" position={Position.Top} isConnectable={true} />
      <Handle type="source" position={Position.Bottom} isConnectable={true} />
    </Box>
  );
}

export function ActiveNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  return (
    <Box className="active-nodes" width={data.size} height={data.size}>
      <Box
        bgcolor={data.preFilled ? "#cca100" : "#000000"}
        width={data.size}
        height={data.size}
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <Image
          src={`/preset_icons/${data.iconName}`}
          width="90%"
          height="90%"
          duration={treeNodeSettings.imageFadeInDuration}
        />
      </Box>
      <Box position="absolute" top="75%" left="65%">
        <Paper
          variant="outlined"
          square
          sx={{
            width: `${data.size * 0.65}px`,
            fontSize: `${data.size * 0.3}px`,
            fontWeight: "600",
            textAlign: "center",
            color: colors.grey[200],
          }}
        >
          {`${data.preFilled ? data.maxPoints : "0"} / ${data.maxPoints}`}
        </Paper>
      </Box>
      <Handle type="target" position={Position.Top} isConnectable={true} />
      <Handle type="source" position={Position.Bottom} isConnectable={true} />
    </Box>
  );
}

export function SwitchNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  return (
    <Box className="switch-nodes" width={data.size} height={data.size}>
      <Box
        className="octagon-shape"
        bgcolor={data.preFilled ? "#cca100" : "#000000"}
        width={`${data.size}px`}
        height={`${data.size}px`}
      >
        <Image
          className="octagon-shape-left"
          position="absolute"
          sx={{ top: `${0.05 * data.size}px`, left: `${0.05 * data.size}px` }}
          src={`/preset_icons/${data.iconName}`}
          width={`${0.9 * data.size}px`}
          height={`${0.9 * data.size}px`}
          duration={treeNodeSettings.imageFadeInDuration}
        />
        <Image
          className="octagon-shape-right"
          position="absolute"
          sx={{ top: `${0.05 * data.size}px`, left: `${0.05 * data.size}px` }}
          src={`/preset_icons/${data.iconNameSwitch}`}
          width={`${0.9 * data.size}px`}
          height={`${0.9 * data.size}px`}
          duration={treeNodeSettings.imageFadeInDuration}
        />
      </Box>
      <Box position="absolute" top="75%" left="65%">
        <Paper
          variant="outlined"
          square
          sx={{
            width: `${data.size * 0.65}px`,
            fontSize: `${data.size * 0.3}px`,
            fontWeight: "600",
            textAlign: "center",
            color: colors.grey[200],
          }}
        >
          {`${data.preFilled ? data.maxPoints : "0"} / ${data.maxPoints}`}
        </Paper>
      </Box>
      <Handle type="target" position={Position.Top} isConnectable={true} />
      <Handle type="source" position={Position.Bottom} isConnectable={true} />
    </Box>
  );
}
