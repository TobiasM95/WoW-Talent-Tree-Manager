import {
  Box,
  Typography,
  useTheme,
  Button,
  Divider,
  TextField,
  InputAdornment,
  IconButton,
  Tabs,
  Tab,
  CircularProgress,
  Tooltip,
} from "@mui/material";
import { useParams, useNavigate } from "react-router-dom";
import { useRef, useState, Fragment, useEffect } from "react";
import { useQuery } from "@tanstack/react-query";
import { tokens } from "../../theme";
import CustomAccordion from "../../components/CustomAccordion";
import BuildViewer from "../../components/tree_components/BuildViewer";
import TreeViewer from "../../components/tree_components/TreeViewer";
import { contentAPI } from "../../api/contentAPI";
import { treeAPI } from "../../api/treeAPI";
import SearchIcon from "@mui/icons-material/Search";
import CheckIcon from "@mui/icons-material/Check";
import ClearIcon from "@mui/icons-material/Clear";
import { classes, classSpecs } from "../../data/classData";
import MarkdownTextBox from "../../components/MarkdownTextBox";
import CustomDataGrid from "../../components/CustomDataGrid";

const Viewer = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [selectedClass, setSelectedClass] = useState(null);
  const [selectedSpec, setSelectedSpec] = useState(null);
  const [assignedPoints, setAssignedPoints] = useState([0, 0]);
  const { contentID } = useParams();
  const contentIDInputRef = useRef("");
  const navigate = useNavigate();

  const [category, setCategory] = useState("TREE");
  const handleTabChange = (event, newCategory) => {
    setCategory(newCategory);
  };

  const validateAndCountAssignedPoints = async (originalData) => {
    if (originalData.msg.contentType !== "BUILD") {
      return originalData;
    }
    let validatedContentData = structuredClone(originalData);
    let validAssignedPoints = [0, 0];

    for (let i = 0; i < 2; i++) {
      let assignedSkills = validatedContentData.msg.build.assignedSkills[i];
      let talents =
        i === 0
          ? validatedContentData.msg.tree.classTalents
          : validatedContentData.msg.tree.specTalents;
      let deleteSkills = [];
      for (let key in assignedSkills) {
        if (
          assignedSkills.hasOwnProperty(key) &&
          talents.hasOwnProperty(key) &&
          talents[key].pre_filled == 0
        ) {
          if (assignedSkills[key] < 0) {
            deleteSkills.push(key);
          } else if (assignedSkills[key] > talents[key].max_points) {
            assignedSkills[key] = talents[key].max_points;
            validAssignedPoints[i] += talents[key].max_points;
          } else {
            validAssignedPoints[i] += assignedSkills[key];
          }
        } else if (!talents.hasOwnProperty(key)) {
          deleteSkills.push(key);
        }
      }
      for (const key of deleteSkills) {
        delete assignedSkills[key];
      }
    }
    setAssignedPoints(validAssignedPoints);

    return validatedContentData;
  };

  const {
    isPending: viewerContentIsPending,
    isFetching: viewerContentIsFetching,
    error: viewerContentError,
    data: viewerContentData,
  } = useQuery({
    queryKey: ["viewerQueryContent" + contentID],
    queryFn: () =>
      contentAPI
        .get(contentID)
        .then((response) => {
          return validateAndCountAssignedPoints(response);
        })
        .then((response) => {
          setCategory(response.msg.contentType);
          return response;
        }),
    refetchOnWindowFocus: false,
  });

  const { refetch: presetTreeIDRefetch } = useQuery({
    queryKey: ["presetTreeContentIDQuery"],
    queryFn: () =>
      treeAPI
        .getPresetTreeContentID(selectedClass, selectedSpec)
        .then((response) => {
          if (response.success === true) {
            navigate(`/viewer/${response.msg}`);
          }
          return {};
        }),
    refetchOnWindowFocus: false,
    enabled: false,
    retry: false,
  });
  useEffect(() => {
    if (selectedClass && selectedSpec) {
      presetTreeIDRefetch();
    }
  }, [selectedSpec]);

  if (!contentID) {
    return (
      <Box
        height="75vh"
        display="flex"
        flexDirection="column"
        justifyContent="center"
        alignItems="center"
      >
        <Typography variant="h2" fontWeight="bold">
          Select your content
        </Typography>
        <Box
          display="flex"
          flexDirection="row"
          justifyContent="center"
          alignItems="center"
          my="20px"
        >
          <Button
            sx={{ mx: "10px" }}
            variant="outlined"
            onClick={() => {
              navigate("/");
            }}
          >
            <Typography>Popular Content</Typography>
          </Button>
          <Button
            sx={{ mx: "10px" }}
            variant="outlined"
            onClick={() => {
              navigate("/workspace");
            }}
          >
            <Typography>Workspace</Typography>
          </Button>
        </Box>
        <Divider style={{ width: "10%" }}>or</Divider>

        <TextField
          id="contentIDInput"
          label="Content ID"
          inputRef={contentIDInputRef}
          sx={{ my: "20px", width: "300px" }}
          InputProps={{
            endAdornment: (
              <InputAdornment position="end">
                <IconButton
                  aria-label="go"
                  onClick={() => {
                    if (
                      contentIDInputRef.current.value &&
                      contentIDInputRef.current.value.length > 0
                    ) {
                      navigate(`/viewer/${contentIDInputRef.current.value}`);
                    }
                  }}
                  edge="end"
                >
                  <SearchIcon />
                </IconButton>
              </InputAdornment>
            ),
          }}
        />
        <Divider style={{ width: "10%" }}>or</Divider>
        <Box display="flex" flexShrink="1" justifyContent="center">
          {classes.map((name) => {
            const dimmed = selectedClass && selectedClass !== name;
            const highlighted = selectedClass && selectedClass === name;
            return (
              <Tooltip
                title={name.charAt(0).toUpperCase() + name.slice(1)}
                key={`classButton${name}tooltip`}
                slotProps={{
                  tooltip: {
                    sx: {
                      bgcolor:
                        theme.palette.mode === "dark"
                          ? `${colors.primary[500]}`
                          : "#fff",
                      "& .MuiTooltip-arrow": {
                        color: "common.black",
                      },
                      border: `1px solid ${colors.grey[100]}`,
                      color: `${colors.grey[100]}`,
                      fontSize: 14,
                    },
                  },
                }}
              >
                <Button
                  key={`classButton${name}`}
                  sx={{
                    minWidth: "16px",
                    maxWidth: "64px",
                    backgroundColor: highlighted ? "#ffffff33" : "none",
                    my: "5px",
                  }}
                  onClick={() => {
                    setSelectedClass(name);
                    setSelectedSpec(null);
                  }}
                >
                  <img
                    src={`/class_icons/${name}.png`}
                    style={{
                      minWidth: "16px",
                      opacity: dimmed ? "0.4" : "1",
                    }}
                  />
                </Button>
              </Tooltip>
            );
          })}
        </Box>
        {selectedClass && (
          <Box display="flex" flexShrink="1" justifyContent="center">
            {classSpecs[selectedClass].map((name) => {
              const dimmed = selectedSpec && selectedSpec !== name;
              const highlighted = selectedSpec && selectedSpec === name;
              return (
                <Tooltip
                  title={name.charAt(0).toUpperCase() + name.slice(1)}
                  key={`specButton${name}tooltip`}
                  slotProps={{
                    tooltip: {
                      sx: {
                        bgcolor:
                          theme.palette.mode === "dark"
                            ? `${colors.primary[500]}`
                            : "#fff",
                        "& .MuiTooltip-arrow": {
                          color: "common.black",
                        },
                        border: `1px solid ${colors.grey[100]}`,
                        color: `${colors.grey[100]}`,
                        fontSize: 14,
                      },
                    },
                  }}
                >
                  <Button
                    key={`specButton${name}`}
                    sx={{
                      minWidth: "16px",
                      maxWidth: "64px",
                      backgroundColor: highlighted ? "#ffffff33" : "none",
                      my: "5px",
                    }}
                    onClick={() => {
                      setSelectedSpec(name);
                    }}
                  >
                    <img
                      src={`/spec_icons/${selectedClass}/${name}.png`}
                      style={{
                        minWidth: "16px",
                        opacity: dimmed ? "0.4" : "1",
                      }}
                    />
                  </Button>
                </Tooltip>
              );
            })}
          </Box>
        )}
      </Box>
    );
  }

  if (viewerContentIsPending || viewerContentIsFetching) {
    return (
      <Box
        height="75vh"
        width="100%"
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <CircularProgress></CircularProgress>
      </Box>
    );
  }
  if (viewerContentError || !viewerContentData.success) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center">
        <Typography color={colors.redAccent[400]}>
          There was an viewerContentError fetching your content. Please try
          again later.
        </Typography>
      </Box>
    );
  }
  return (
    <Box m="20px" height="88dvh" overflow="auto">
      <Box display="flex" justifyContent="space-between">
        <Box display="flex">
          <Button
            sx={{
              borderColor: colors.grey[100],
              color: colors.grey[100],
              height: "50%",
              mx: "10px",
            }}
            variant="outlined"
            onClick={() => {
              navigate("/viewer");
            }}
          >
            <Typography>{"<"} Back to menu</Typography>
          </Button>
          <Button
            sx={{
              borderColor: colors.greenAccent[500],
              color: colors.greenAccent[500],
              height: "50%",
            }}
            variant="outlined"
            //onClick={buttonLogout}
          >
            <Typography>Import to Workspace</Typography>
          </Button>
        </Box>
        <Tabs
          value={category}
          onChange={handleTabChange}
          centered
          sx={{ mb: "20px" }}
        >
          <Tab label="Tree" value="TREE" />
          {viewerContentData.msg.contentType !== "TREE" && (
            <Tab label="Loadout" value="LOADOUT" />
          )}
          {viewerContentData.msg.contentType === "BUILD" && (
            <Tab label="Build" value="BUILD" />
          )}
        </Tabs>
        <Box display="flex" visibility="hidden">
          <Button
            sx={{
              mx: "10px",
            }}
            variant="outlined"
          >
            <Typography>{"<"} Back to menu</Typography>
          </Button>
          <Button
            //variant outlined is necessary
            variant="outlined"
          >
            <Typography>Import to Workspace</Typography>
          </Button>
        </Box>
      </Box>
      {category === "TREE" && (
        <Fragment>
          <Box
            display="grid"
            gridTemplateColumns="repeat(12, 1fr)"
            gridTemplateRows="repeat(12, 1fr)"
            gridAutoRows="140px"
            gap="20px"
            height="70vh"
          >
            <Box
              gridColumn="span 6"
              gridRow="span 12"
              backgroundColor={colors.primary[400]}
              display="flex"
              alignItems="center"
              justifyContent="center"
              sx={{ borderRadius: "10px" }}
              border={1}
              borderColor={colors.blueAccent[700]}
            >
              <TreeViewer treeData={viewerContentData.msg.tree.classTalents} />
            </Box>
            <Box
              gridColumn="span 6"
              gridRow="span 12"
              backgroundColor={colors.primary[400]}
              display="flex"
              alignItems="center"
              justifyContent="center"
              sx={{ borderRadius: "10px" }}
              border={1}
              borderColor={colors.blueAccent[700]}
            >
              <TreeViewer treeData={viewerContentData.msg.tree.specTalents} />
            </Box>
          </Box>
          <Box mt="10px">
            <CustomAccordion
              summary={"Tree details  (click to expand)"}
              content={
                <Fragment>
                  <Typography variant="h3">
                    {viewerContentData.msg.tree.name}
                  </Typography>
                  <Typography>
                    Number of talents (class / spec):{" "}
                    {
                      Object.keys(viewerContentData.msg.tree.classTalents)
                        .length
                    }{" "}
                    /{" "}
                    {Object.keys(viewerContentData.msg.tree.specTalents).length}
                  </Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {viewerContentData.msg.tree.isImported ? (
                      <CheckIcon />
                    ) : (
                      <ClearIcon />
                    )}
                  </Box>
                  <Typography>Description:</Typography>
                  <MarkdownTextBox
                    text={viewerContentData.msg.tree.description}
                    defaultText="No description set"
                  />
                </Fragment>
              }
              theme={theme}
              colors={colors}
            />
          </Box>
        </Fragment>
      )}
      {category === "LOADOUT" && (
        <Fragment>
          <Box
            display="grid"
            gridTemplateColumns="repeat(12, 1fr)"
            gap="20px"
            height="80vh"
          >
            {viewerContentData.msg.loadout ? (
              <Fragment>
                <Box
                  gridColumn="span 8"
                  backgroundColor={colors.primary[400]}
                  display="flex"
                  flexDirection="column"
                  alignItems="flex-start"
                  justifyContent="start"
                  padding="20px"
                  sx={{ borderRadius: "10px" }}
                  border={1}
                  borderColor={colors.blueAccent[700]}
                  overflow="auto"
                  height="80vh"
                >
                  <Typography variant="h3">
                    {viewerContentData.msg.loadout.name}
                  </Typography>
                  <Typography>
                    Tree name: {viewerContentData.msg.loadout.treeName}
                  </Typography>
                  <Typography>
                    Number of builds in this loadout:{" "}
                    {
                      Object.keys(viewerContentData.msg.loadout.builds.rows)
                        .length
                    }
                  </Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {viewerContentData.msg.loadout.isImported ? (
                      <CheckIcon />
                    ) : (
                      <ClearIcon />
                    )}
                  </Box>
                  <Typography>Description:</Typography>
                  <Box width="100%">
                    <MarkdownTextBox
                      text={viewerContentData.msg.loadout.description}
                      defaultText="No description set"
                    />
                  </Box>
                </Box>
                <Box
                  gridColumn="span 4"
                  backgroundColor={colors.primary[400]}
                  sx={{ borderRadius: "0px 10px 10px 0px" }}
                  border={1}
                  borderColor={colors.blueAccent[700]}
                  height="80vh"
                  p="10px"
                >
                  <Typography variant="h2" sx={{ mb: "10px" }}>
                    Builds in this Loadout:
                  </Typography>
                  <CustomDataGrid
                    columns={viewerContentData.msg.loadout.builds.columns}
                    data={viewerContentData.msg.loadout.builds.rows}
                    rowIDCol={"actions"}
                    height={"72vh"}
                  />
                </Box>
              </Fragment>
            ) : (
              <Box
                gridColumn="span 12"
                display="flex"
                flexDirection="column"
                justifyContent="center"
                alignItems="center"
              >
                <Typography variant="h3">
                  This build does not belong to a loadout
                </Typography>
              </Box>
            )}
          </Box>
        </Fragment>
      )}
      {category === "BUILD" && (
        <Fragment>
          <Box
            display="grid"
            gridTemplateColumns="repeat(12, 1fr)"
            gridTemplateRows="repeat(12, 1fr)"
            gridAutoRows="140px"
            gap="20px"
            height="70vh"
          >
            <Box
              gridColumn="span 6"
              gridRow="span 12"
              backgroundColor={colors.primary[400]}
              display="flex"
              alignItems="center"
              justifyContent="center"
              sx={{ borderRadius: "10px" }}
              border={1}
              borderColor={colors.blueAccent[700]}
            >
              <BuildViewer
                treeData={viewerContentData.msg.tree.classTalents}
                assignedSkills={viewerContentData.msg.build.assignedSkills}
                isClassTree={true}
              />
            </Box>
            <Box
              gridColumn="span 6"
              gridRow="span 12"
              backgroundColor={colors.primary[400]}
              display="flex"
              alignItems="center"
              justifyContent="center"
              sx={{ borderRadius: "10px" }}
              border={1}
              borderColor={colors.blueAccent[700]}
              height="70vh"
            >
              <BuildViewer
                treeData={viewerContentData.msg.tree.specTalents}
                assignedSkills={viewerContentData.msg.build.assignedSkills}
                isClassTree={false}
              />
            </Box>
          </Box>
          <Box mt="10px">
            <CustomAccordion
              summary={"Build details  (click to expand)"}
              content={
                <Fragment>
                  <Typography variant="h3">
                    {viewerContentData.msg.build.name}
                  </Typography>
                  <Typography>
                    Assigned skillpoints (class / spec): {assignedPoints[0]}{" "}
                    {assignedPoints[1]}
                  </Typography>
                  <Typography>
                    Required level: {viewerContentData.msg.build.levelCap}
                  </Typography>
                  <Typography>
                    Loadout name: {viewerContentData.msg.build.loadoutName}
                  </Typography>
                  <Typography>
                    Tree name: {viewerContentData.msg.build.treeName}
                  </Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {viewerContentData.msg.build.isImported ? (
                      <CheckIcon />
                    ) : (
                      <ClearIcon />
                    )}
                  </Box>
                  <Typography>Description:</Typography>
                  <MarkdownTextBox
                    text={viewerContentData.msg.build.description}
                    defaultText="No description set"
                  />
                </Fragment>
              }
              theme={theme}
              colors={colors}
            />
          </Box>
        </Fragment>
      )}
    </Box>
  );
};

export default Viewer;
