import { Box, Typography, useTheme, Button } from "@mui/material";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import AddCircleOutlineOutlinedIcon from "@mui/icons-material/AddCircleOutlineOutlined";

const Workspace = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
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
        <Box
          gridColumn="span 4"
          backgroundColor={colors.primary[400]}
          display="flex"
          flexDirection="column"
          justifyContent="space-between"
          sx={{ borderRadius: "20px" }}
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
              TREES
            </Typography>
          </Box>
          <Box border={1}></Box>
          <Button
            width="100%"
            variant="text"
            sx={{
              border: 1,
              borderRadius: "0px 0px 20px 20px",
              height: "30px",
            }}
            startIcon={<AddCircleOutlineOutlinedIcon />}
          >
            Create tree
          </Button>
        </Box>
      </Box>
    </Box>
  );
};

export default Workspace;
