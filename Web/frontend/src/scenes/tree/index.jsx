import { Box, useTheme } from "@mui/material";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import TreeViewer from "../../components/TreeViewer";

const Tree = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
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
          gridRow="span 12"
          backgroundColor={colors.primary[400]}
          display="flex"
          alignItems="center"
          justifyContent="center"
          sx={{ borderRadius: "10px" }}
        >
          <TreeViewer />
        </Box>
      </Box>
    </Box>
  );
};

export default Tree;
