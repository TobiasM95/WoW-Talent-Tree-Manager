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
import { useDrag } from "../DragProvider";

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

  const { isDragging } = useDrag();

  const descriptionArray = JSON.parse(data.description);

  return (
    <Tooltip
      title={
        isDragging ? (
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
      <Box
        className="passive-nodes"
        width={`${data.size}px`}
        height={`${data.size}px`}
        sx={{ opacity: data.opacity }}
      >
        <Box
          className="circle-shape"
          bgcolor={data.borderColor}
          width={data.size}
          height={data.size}
          display="flex"
          justifyContent="center"
          alignItems="center"
        >
          <Box
            className="circle-shape"
            component="img"
            src={`/preset_icons/${data.iconName}`}
            width="80%"
            height="80%"
          />
        </Box>
        <Box position="absolute" top="60%" left="75%">
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
            {`${data.pointsDisplayLow} / ${data.pointsDisplayHigh}`}
          </Paper>
        </Box>
        <Handle
          type="target"
          position={Position.Top}
          isConnectable={data.isConnectable}
        />
        <Handle
          type="source"
          position={Position.Bottom}
          isConnectable={data.isConnectable}
        />
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

  const { isDragging } = useDrag();

  return (
    <Tooltip
      title={
        isDragging ? (
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
      <Box
        className="active-nodes"
        width={`${data.size}px`}
        height={`${data.size}px`}
        sx={{ opacity: data.opacity }}
      >
        <Box
          bgcolor={data.borderColor}
          width={data.size}
          height={data.size}
          display="flex"
          justifyContent="center"
          alignItems="center"
        >
          <Box
            component="img"
            src={`/preset_icons/${data.iconName}`}
            width="80%"
            height="80%"
          />
        </Box>
        <Box position="absolute" top="60%" left="75%">
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
            {`${data.pointsDisplayLow} / ${data.pointsDisplayHigh}`}
          </Paper>
        </Box>
        <Handle
          type="target"
          position={Position.Top}
          isConnectable={data.isConnectable}
        />
        <Handle
          type="source"
          position={Position.Bottom}
          isConnectable={data.isConnectable}
        />
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

  const { isDragging } = useDrag();

  return (
    <Tooltip
      title={
        isDragging ? (
          false
        ) : data.points === 0 ? (
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
        ) : (
          <Fragment>
            <Typography variant="h3" fontWeight="bold">
              {data.points === 1 ? data.name : data.nameSwitch}
            </Typography>
            <Image
              src={`/preset_icons/${
                data.points === 1 ? data.iconName : data.iconNameSwitch
              }`}
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
              {data.points === 1 ? data.description : data.descriptionSwitch}
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
      <Box
        className="switch-nodes"
        width={`${data.size}px`}
        height={`${data.size}px`}
        sx={{ opacity: data.opacity }}
      >
        <Box
          className="octagon-shape"
          bgcolor={data.borderColor}
          width={`${data.size}px`}
          height={`${data.size}px`}
        >
          {data.points === 0 ? (
            <Fragment>
              <Box
                className="octagon-shape-left"
                component="img"
                src={`/preset_icons/${data.iconName}`}
                position="absolute"
                sx={{
                  top: `${0.1 * data.size}px`,
                  left: `${0.1 * data.size}px`,
                }}
                width={`${0.8 * data.size}px`}
                height={`${0.8 * data.size}px`}
              />
              <Box
                className="octagon-shape-right"
                component="img"
                src={`/preset_icons/${data.iconNameSwitch}`}
                position="absolute"
                sx={{
                  top: `${0.1 * data.size}px`,
                  left: `${0.1 * data.size}px`,
                }}
                width={`${0.8 * data.size}px`}
                height={`${0.8 * data.size}px`}
              />
            </Fragment>
          ) : (
            <Fragment>
              <Box
                className="octagon-shape"
                component="img"
                src={`/preset_icons/${
                  data.points === 1 ? data.iconName : data.iconNameSwitch
                }`}
                position="absolute"
                sx={{
                  top: `${0.1 * data.size}px`,
                  left: `${0.1 * data.size}px`,
                }}
                width={`${0.8 * data.size}px`}
                height={`${0.8 * data.size}px`}
              />
            </Fragment>
          )}
        </Box>
        <Box position="absolute" top="60%" left="75%">
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
            {`${data.pointsDisplayLow} / ${data.pointsDisplayHigh}`}
          </Paper>
        </Box>
        <Handle
          type="target"
          position={Position.Top}
          isConnectable={data.isConnectable}
        />
        <Handle
          type="source"
          position={Position.Bottom}
          isConnectable={data.isConnectable}
        />
      </Box>
    </Tooltip>
  );
}
