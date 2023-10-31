import { Box, useTheme, Typography, CircularProgress } from "@mui/material";
import { useEffect, useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { treeAPI } from "../../api/treeAPI";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import TreeViewer from "../../components/TreeViewer";
import ToggleButtons from "../../components/ToggleButtons";

const Tree = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  const [treeName, setTreeName] = useState("");
  const [treeDescription, setTreeDescription] = useState("");

  const queryTree = async () => {
    return treeAPI.get("8bf345aa-0bd8-4e67-ae1f-7103f1712887");
  };

  const query = useQuery({
    queryKey: ["treeViewerQueryTree"],
    queryFn: () => queryTree(),
    refetchOnWindowFocus: false,
  });

  useEffect(() => {
    if (query.data) {
      setTreeName(query.data[1].name);
      setTreeDescription(query.data[1].description);
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
          <ToggleButtons selection={["Class Tree", "Spec Tree", treeName]} />
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
            <TreeViewer treeData={query.data[1].classTalents} />
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
            <TreeViewer treeData={query.data[1].specTalents} />
          )}
        </Box>
      </Box>
    </Box>
  );
};

export default Tree;
