import {
  Box,
  Typography,
  useTheme,
  Button,
  CircularProgress,
  Tabs,
  Tab,
} from "@mui/material";
import { useState } from "react";
import AddCircleOutlineOutlinedIcon from "@mui/icons-material/AddCircleOutlineOutlined";
import { useQuery } from "@tanstack/react-query";
import { workspaceAPI } from "../../api/workspaceAPI";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import CustomDataGrid from "../../components/CustomDataGrid";

const Workspace = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [category, setCategory] = useState("TREE");
  const allCategoryParams = {
    TREE: {
      header: "Trees",
      createString: "Create tree",
      colsName: "treeColumns",
      dataName: "treeData",
    },
    LOADOUT: {
      header: "Loadouts",
      createString: "Create loadout",
      colsName: "loadoutColumns",
      dataName: "loadoutData",
    },
    BUILD: {
      header: "Builds",
      createString: "Create build",
      colsName: "buildColumns",
      dataName: "buildData",
    },
  };
  const [categoryParams, setCategoryParams] = useState(
    allCategoryParams["TREE"]
  );

  const query = useQuery({
    queryKey: ["workspaceQueryWorkspace"],
    queryFn: () => workspaceAPI.get(),
  });

  const handleTabChange = (event, newCategory) => {
    setCategory(newCategory);
    setCategoryParams(allCategoryParams[newCategory]);
  };

  return (
    <Box m="20px">
      <Header
        title="WORKSPACE"
        subtitle="View your trees, loadouts and builds"
      />
      <Tabs
        value={category}
        onChange={handleTabChange}
        centered
        sx={{ mb: "20px" }}
      >
        <Tab label="Trees" value="TREE" />
        <Tab label="Loadouts" value="LOADOUT" />
        <Tab label="Builds" value="BUILD" />
      </Tabs>
      <Box
        display="grid"
        gridTemplateColumns="repeat(12, 1fr)"
        gridAutoRows="65vh"
        gap="20px"
      >
        <Box
          gridColumn="span 12"
          backgroundColor={colors.primary[400]}
          display="flex"
          flexDirection="column"
          justifyContent="space-between"
          sx={{ borderRadius: "5px" }}
          key={categoryParams.header + categoryParams.createStr}
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
          {query.error === null && query.isPending === false && (
            <CustomDataGrid
              columns={query.data.msg[categoryParams.colsName]}
              data={query.data.msg[categoryParams.dataName]}
              rowIDCol={"actions"}
            />
          )}
          <Button
            width="100%"
            variant="contained"
            sx={{
              border: 1,
              borderRadius: "0px 0px 5px 5px",
              height: "30px",
              backgroundColor: colors.greenAccent[400],
              color: colors.primary[900],
            }}
            startIcon={<AddCircleOutlineOutlinedIcon />}
          >
            <Typography>{categoryParams.createString}</Typography>
          </Button>
        </Box>
      </Box>
    </Box>
  );
};

export default Workspace;
