import { Box, useTheme, Typography, CircularProgress } from "@mui/material";
import { useEffect, useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { buildAPI } from "../../api/buildAPI";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import BuildViewer from "../../components/tree_components/BuildViewer";
import ToggleButtons from "../../components/ToggleButtons";

const Build = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  const [treeName, setTreeName] = useState("");
  const [loadoutName, setLoadoutName] = useState("");
  const [buildName, setBuildName] = useState("");

  const queryTree = async () => {
    return buildAPI.get("25998983-f381-48ee-8194-161840871a68");
  };

  const query = useQuery({
    queryKey: ["buildViewerQueryBuild"],
    queryFn: () => queryTree(),
    refetchOnWindowFocus: false,
  });

  useEffect(() => {
    if (query.data) {
      setBuildName(query.data.name);
      setLoadoutName(query.data.loadoutName);
      setTreeName(query.data.treeName);
    }
  }, [query.data]);

  return (
    <Box m="20px">
      <Box display="flex" justifyContent="space-between" alignItems="center">
        <Header title="TREE" subtitle="View all your Talent Trees" />
      </Box>

      {/* Grid and charts */}
      <Box
        display="grid"
        gridTemplateColumns="repeat(12, 1fr)"
        gridTemplateRows="repeat(12, 1fr)"
        gridAutoRows="140px"
        gap="20px"
        height="75vh"
      >
        {/* ROW 1*/}
        <Box
          gridColumn="span 6"
          display="flex"
          alignItems="center"
          justifyContent="center"
        >
          <ToggleButtons
            selection={[
              "Class Tree",
              "Spec Tree",
              treeName ? treeName : "Unknown tree name",
              loadoutName ? loadoutName : "Unknown loadout name",
              buildName ? buildName : "Unknown build name",
            ]}
          />
        </Box>
        <Box
          gridColumn="span 6"
          display="flex"
          alignItems="center"
          justifyContent="space-between"
        ></Box>
        {/* ROW 2*/}
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
          {query.isPending && (
            <Box display="flex" flexDirection={"row"} justifyContent={"center"}>
              <CircularProgress />
            </Box>
          )}
          {query.error && (
            <Box>
              <Typography>
                "An error has occurred: " + {query.error.message}
              </Typography>
            </Box>
          )}
          {query.error === null && query.isPending === false && query.data && (
            <BuildViewer
              treeData={query.data.classTalents}
              assignedSkills={query.data.assignedSkills}
              isClassTree={true}
            />
          )}
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
          {query.isPending && (
            <Box display="flex" flexDirection={"row"} justifyContent={"center"}>
              <CircularProgress />
            </Box>
          )}
          {query.error && (
            <Box>
              <Typography>
                "An error has occurred: " + {query.error.message}
              </Typography>
            </Box>
          )}
          {query.error === null && query.isPending === false && query.data && (
            <BuildViewer
              treeData={query.data.specTalents}
              assignedSkills={query.data.assignedSkills}
              isClassTree={false}
            />
          )}
        </Box>
      </Box>
    </Box>
  );
};

export default Build;
