import {
  Box,
  Paper,
  useTheme,
  Tooltip,
  Typography,
  Divider,
} from "@mui/material";
import { Fragment, useContext } from "react";
//import { useCallback } from "react";
import { Handle, Position } from "reactflow";
import Image from "mui-image";
import { tokens } from "../../theme";
import { treeNodeSettings } from "../../data/settings";
import { DragProvider } from "../TreeViewer";

import "./treeComponentTheming.css";

export function DividerNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  return (
    <Box className="divider-nodes">
      <Box
        bgcolor={
          theme.palette.mode === "dark" ? `${colors.primary[500]}` : "#fff"
        }
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <Paper variant="outlined" square={false}>
          <Typography
            marginX="10px"
            color={colors.treeColors.blue}
            fontSize={`${data.size * 0.3}px`}
            fontWeight="600"
          >
            {data.requiredPoints} points
          </Typography>
        </Paper>
      </Box>
      {data.position === "right" && (
        <Handle type="target" position={Position.Left} isConnectable={false} />
      )}
      {data.position === "left" && (
        <Handle type="source" position={Position.Right} isConnectable={false} />
      )}
    </Box>
  );
}

export function PassiveNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  const isDragged = useContext(DragProvider);

  const descriptionArray = JSON.parse(data.description);

  return (
    <Tooltip
      title={
        isDragged ? (
          false
        ) : (
          <Fragment>
            <Typography variant="h3" fontWeight="bold">
              {data.name}
            </Typography>
            <Image
              src={`/preset_icons/${data.iconName}`}
              width={0.5 * data.size}
              height={0.5 * data.size}
              duration={0}
            />
            <Typography>
              Id: {data.id}, Pos: ({data.row}, {data.column})
            </Typography>
            <Typography color={colors.treeColors.red}>(active)</Typography>
            <Typography sx={{ marginBottom: "10px" }}>
              Max points: {data.maxPoints}, points required:{" "}
              {data.requiredPoints}
            </Typography>
            {descriptionArray.map((d, index) => (
              <Fragment key={index + 10000}>
                <Typography
                  color={colors.treeColors.blue}
                  key={index}
                  style={{ whiteSpace: "pre-line" }}
                >
                  {d}
                </Typography>
                {index !== descriptionArray.length - 1 && (
                  <Divider sx={{ marginY: "10px" }} key={index + 100}></Divider>
                )}
              </Fragment>
            ))}
          </Fragment>
        )
      }
      disableInteractive
      placement="right"
      slotProps={{
        tooltip: {
          sx: {
            bgcolor: colors.treeColors.tooltipBg,
            "& .MuiTooltip-arrow": {
              color: "common.black",
            },
            border: `1px solid ${colors.treeColors.tooltipBorder}`,
            color: `${colors.grey[100]}`,
            fontSize: 12,
          },
        },
      }}
    >
      <Box className="passive-nodes" width={data.size} height={data.size}>
        <Box
          className="circle-shape"
          bgcolor={
            data.preFilled ? colors.treeColors.gold : colors.treeColors.black
          }
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
    </Tooltip>
  );
}

export function ActiveNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  const isDragged = useContext(DragProvider);

  return (
    <Tooltip
      title={
        isDragged ? (
          false
        ) : (
          <Fragment>
            <Typography variant="h3" fontWeight="bold">
              {data.name}
            </Typography>
            <Image
              src={`/preset_icons/${data.iconName}`}
              width={0.5 * data.size}
              height={0.5 * data.size}
              duration={0}
            />
            <Typography>
              Id: {data.id}, Pos: ({data.row}, {data.column})
            </Typography>
            <Typography color={colors.treeColors.red}>(active)</Typography>
            <Typography sx={{ marginBottom: "10px" }}>
              Max points: {data.maxPoints}, points required:{" "}
              {data.requiredPoints}
            </Typography>
            <Typography
              color={colors.treeColors.blue}
              style={{ whiteSpace: "pre-line" }}
            >
              {data.description}
            </Typography>
          </Fragment>
        )
      }
      disableInteractive
      placement="right"
      slotProps={{
        tooltip: {
          sx: {
            bgcolor: colors.treeColors.tooltipBg,
            "& .MuiTooltip-arrow": {
              color: "common.black",
            },
            border: `1px solid ${colors.treeColors.tooltipBorder}`,
            color: `${colors.grey[100]}`,
            fontSize: 12,
          },
        },
      }}
    >
      <Box className="active-nodes" width={data.size} height={data.size}>
        <Box
          bgcolor={
            data.preFilled ? colors.treeColors.gold : colors.treeColors.black
          }
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
    </Tooltip>
  );
}

export function SwitchNode({ data }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  const isDragged = useContext(DragProvider);

  return (
    <Tooltip
      title={
        isDragged ? (
          false
        ) : (
          <Fragment>
            <Typography variant="h3" fontWeight="bold">
              {data.name}
            </Typography>
            <Image
              src={`/preset_icons/${data.iconName}`}
              width={0.5 * data.size}
              height={0.5 * data.size}
              duration={0}
            />
            <Typography>
              Id: {data.id}, Pos: ({data.row}, {data.column})
            </Typography>
            <Typography color={colors.treeColors.red}>(switch)</Typography>
            <Typography sx={{ marginBottom: "10px" }}>
              Max points: {data.maxPoints}, points required:{" "}
              {data.requiredPoints}
            </Typography>
            <Typography
              color={colors.treeColors.blue}
              style={{ whiteSpace: "pre-line" }}
            >
              {data.description}
            </Typography>
            <Divider sx={{ marginY: "10px" }}></Divider>
            <Typography variant="h3" fontWeight="bold">
              {data.nameSwitch}
            </Typography>
            <Image
              src={`/preset_icons/${data.iconNameSwitch}`}
              width={0.5 * data.size}
              height={0.5 * data.size}
              duration={0}
            />
            <Typography>
              Id: {data.id}, Pos: ({data.row}, {data.column})
            </Typography>
            <Typography color={colors.treeColors.red}>(switch)</Typography>
            <Typography sx={{ marginBottom: "10px" }}>
              Max points: {data.maxPoints}, points required:{" "}
              {data.requiredPoints}
            </Typography>
            <Typography
              color={colors.treeColors.blue}
              style={{ whiteSpace: "pre-line" }}
            >
              {data.descriptionSwitch}
            </Typography>
          </Fragment>
        )
      }
      disableInteractive
      placement="right"
      slotProps={{
        tooltip: {
          sx: {
            bgcolor: colors.treeColors.tooltipBg,
            "& .MuiTooltip-arrow": {
              color: "common.black",
            },
            border: `1px solid ${colors.treeColors.tooltipBorder}`,
            color: `${colors.grey[100]}`,
            fontSize: 12,
          },
        },
      }}
    >
      <Box className="switch-nodes" width={data.size} height={data.size}>
        <Box
          className="octagon-shape"
          bgcolor={
            data.preFilled ? colors.treeColors.gold : colors.treeColors.black
          }
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
    </Tooltip>
  );
}
