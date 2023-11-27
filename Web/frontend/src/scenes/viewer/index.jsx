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
} from "@mui/material";
import { useParams, useNavigate } from "react-router-dom";
import { useRef, useState, Fragment } from "react";
import { useQuery } from "@tanstack/react-query";
import { tokens } from "../../theme";
import CustomAccordion from "../../components/CustomAccordion";
import BuildViewer from "../../components/tree_components/BuildViewer";
import TreeViewer from "../../components/tree_components/TreeViewer";
import { contentAPI } from "../../api/contentAPI";
import SearchIcon from "@mui/icons-material/Search";
import CheckIcon from "@mui/icons-material/Check";
import ClearIcon from "@mui/icons-material/Clear";
import MarkdownTextBox from "../../components/MarkdownTextBox";
import CustomDataGrid from "../../components/CustomDataGrid";

const Viewer = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const { contentID } = useParams();
  const contentIDInputRef = useRef("");
  const navigate = useNavigate();

  const [category, setCategory] = useState("TREE");
  const handleTabChange = (event, newCategory) => {
    setCategory(newCategory);
  };

  const { isPending, isLoading, isFetching, error, data } = useQuery({
    queryKey: ["viewerQueryContent" + contentID],
    queryFn: () =>
      contentAPI.get(contentID).then((response) => {
        setCategory(response.msg.contentType);
        return response;
      }),
    refetchOnWindowFocus: false,
  });

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
      </Box>
    );
  }

  if (isPending || isFetching) {
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
  if (error || !data.success) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center">
        <Typography color={colors.redAccent[400]}>
          There was an error fetching your content. Please try again later.
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
          {data.msg.contentType !== "TREE" && (
            <Tab label="Loadout" value="LOADOUT" />
          )}
          {data.msg.contentType === "BUILD" && (
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
              <TreeViewer treeData={data.msg.tree.classTalents} />
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
              <TreeViewer treeData={data.msg.tree.specTalents} />
            </Box>
          </Box>
          <Box mt="10px">
            <CustomAccordion
              summary={"Tree details  (click to expand)"}
              content={
                <Fragment>
                  <Typography variant="h3">{data.msg.tree.name}</Typography>
                  <Typography>
                    Number of talents (class / spec):{" "}
                    {Object.keys(data.msg.tree.classTalents).length} /{" "}
                    {Object.keys(data.msg.tree.specTalents).length}
                  </Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {data.msg.tree.isImported ? <CheckIcon /> : <ClearIcon />}
                  </Box>
                  <Typography>Description:</Typography>
                  <MarkdownTextBox
                    text={data.msg.tree.description}
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
            {data.msg.loadout ? (
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
                  <Typography variant="h3">{data.msg.loadout.name}</Typography>
                  <Typography>
                    Tree name: {data.msg.loadout.treeName}
                  </Typography>
                  <Typography>
                    Number of builds in this loadout:{" "}
                    {Object.keys(data.msg.loadout.builds).length}
                  </Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {data.msg.loadout.isImported ? (
                      <CheckIcon />
                    ) : (
                      <ClearIcon />
                    )}
                  </Box>
                  <Typography>Description:</Typography>
                  <Box width="100%">
                    <MarkdownTextBox
                      text={data.msg.loadout.description}
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
                    columns={data.msg.loadout.builds.columns}
                    data={data.msg.loadout.builds.rows}
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
                treeData={data.msg.tree.classTalents}
                assignedSkills={data.msg.build.assignedSkills}
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
                treeData={data.msg.tree.specTalents}
                assignedSkills={data.msg.build.assignedSkills}
                isClassTree={false}
              />
            </Box>
          </Box>
          <Box mt="10px">
            <CustomAccordion
              summary={"Build details  (click to expand)"}
              content={
                <Fragment>
                  <Typography variant="h3">{data.msg.build.name}</Typography>
                  <Typography>
                    Assigned skillpoints (class / spec):{" "}
                    {Object.values(data.msg.build.assignedSkills[0]).reduce(
                      (accumulator, currentValue) => {
                        return accumulator + currentValue;
                      },
                      0
                    ) -
                      Object.values(data.msg.tree.classTalents).reduce(
                        (acc, obj) =>
                          obj.pre_filled === 1 ? acc + obj.max_points : acc,
                        0
                      )}{" "}
                    /{" "}
                    {Object.values(data.msg.build.assignedSkills[1]).reduce(
                      (accumulator, currentValue) => {
                        return accumulator + currentValue;
                      },
                      0
                    )}
                  </Typography>
                  <Typography>
                    Required level: {data.msg.build.levelCap}
                  </Typography>
                  <Typography>
                    Loadout name: {data.msg.build.loadoutName}
                  </Typography>
                  <Typography>Tree name: {data.msg.build.treeName}</Typography>
                  <Box display="flex" flexDirection="row">
                    <Typography>Imported:</Typography>
                    {data.msg.build.isImported ? <CheckIcon /> : <ClearIcon />}
                  </Box>
                  <Typography>Description:</Typography>
                  <MarkdownTextBox
                    text={data.msg.build.description}
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
