import { Box, Button, IconButton, Typography, useTheme } from "@mui/material";
import { useContext } from "react";
import { NavLink } from "react-router-dom";
import { ColorModeContext, tokens } from "../../theme";
import { useAuth } from "../../components/AuthProvider";
import { InputBase } from "@mui/material";
import LightModeOutlinedIcon from "@mui/icons-material/LightModeOutlined";
import DarkModeOutlinedIcon from "@mui/icons-material/DarkModeOutlined";
import NotificationsOutlinedIcon from "@mui/icons-material/NotificationsOutlined";
import SettingsOutlinedIcon from "@mui/icons-material/SettingsOutlined";
import PersonOutlinedIcon from "@mui/icons-material/PersonOutlined";
import SearchIcon from "@mui/icons-material/Search";

const Topbar = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const colorMode = useContext(ColorModeContext);

  const { loginState, setLogout } = useAuth();

  const buttonLogout = async () => {
    const error_information = await setLogout();
    console.log("logout status", error_information);
  };

  return (
    <Box display="flex" justifyContent="space-between" p={2}>
      {/* SEARCHBAR*/}
      <Box
        display="flex"
        backgroundColor={colors.primary[400]}
        borderRadius="10px"
      >
        <InputBase sx={{ ml: 2, flex: 1 }} placeholder="Search" />
        <IconButton type="button" sx={{ p: 1 }}>
          <SearchIcon />
        </IconButton>
      </Box>
      {/* ICONS */}
      <Box display="flex">
        {loginState === true && (
          <Button
            sx={{
              backgroundColor: colors.greenAccent[500],
              color: "#000",
              fontSize: "14px",
              fontWeight: "bold",
              padding: "10px 20px",
            }}
            onClick={buttonLogout}
          >
            <Typography>Logout</Typography>
          </Button>
        )}
        {loginState === false && (
          <Button
            sx={{
              backgroundColor: colors.greenAccent[500],
              color: "#000",
              fontSize: "14px",
              fontWeight: "bold",
              padding: "10px 20px",
            }}
            component={NavLink}
            to={"/login"}
          >
            <Typography>Login</Typography>
          </Button>
        )}
        <IconButton onClick={colorMode.toggleColorMode}>
          {theme.palette.mode === "dark" ? (
            <LightModeOutlinedIcon />
          ) : (
            <DarkModeOutlinedIcon />
          )}
        </IconButton>
        <IconButton>
          <NotificationsOutlinedIcon />
        </IconButton>
        <IconButton>
          <SettingsOutlinedIcon />
        </IconButton>
        <IconButton>
          <PersonOutlinedIcon />
        </IconButton>
      </Box>
    </Box>
  );
};

export default Topbar;
