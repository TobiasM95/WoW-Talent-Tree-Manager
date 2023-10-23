import {
  Box,
  Typography,
  useTheme,
  Button,
  CircularProgress,
} from "@mui/material";
import AddCircleOutlineOutlinedIcon from "@mui/icons-material/AddCircleOutlineOutlined";
import { useQuery } from "@tanstack/react-query";
import { workspaceAPI } from "../../api/workspaceAPI";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import CustomDataGrid from "../../components/CustomDataGrid";

const Workspace = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  const query = useQuery({
    queryKey: ["workspaceQueryWorkspace"],
    queryFn: () => workspaceAPI.get(),
  });

  return (
    <Box m="20px">
      <Header
        title="WORKSPACE"
        subtitle="View your trees, loadouts and builds"
      />
      <Box
        display="grid"
        gridTemplateColumns="repeat(12, 1fr)"
        gridAutoRows="75vh"
        gap="20px"
      >
        {[
          ["TREES", "Create tree", "treeColumns", "treeData"],
          ["LOADOUTS", "Create loadout", "loadoutColumns", "loadoutData"],
          ["BUILDS", "Create build", "buildColumns", "buildData"],
        ].map(([header, createStr, colsName, dataName]) => (
          <Box
            gridColumn="span 4"
            backgroundColor={colors.primary[400]}
            display="flex"
            flexDirection="column"
            justifyContent="space-between"
            sx={{ borderRadius: "20px" }}
            key={header + createStr}
          >
            <Box
              width="100%"
              height="30px"
              border={1}
              borderRadius="20px 20px 0px 0px"
              borderColor={colors.grey[100]}
              display="flex"
              flexDirection="column"
              justifyContent="center"
            >
              <Typography
                variant="h4"
                fontWeight="bold"
                textAlign={"center"}
                color={colors.grey[100]}
              >
                {header}
              </Typography>
            </Box>
            {query.isPending && (
              <Box>
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
                columns={query.data[colsName]}
                data={query.data[dataName]}
                rowIDCol={"actions"}
              />
            )}
            <Button
              width="100%"
              variant="text"
              sx={{
                border: 1,
                borderRadius: "0px 0px 20px 20px",
                height: "30px",
                color: colors.greenAccent[400],
              }}
              startIcon={<AddCircleOutlineOutlinedIcon />}
            >
              <Typography>{createStr}</Typography>
            </Button>
          </Box>
        ))}
      </Box>
    </Box>
  );
};

export default Workspace;
